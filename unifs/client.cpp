// obZrv
// https://github.com/Aulddays/obZrv
// 
// Copyright (c) 2020-2026 Aulddays (https://dev.aulddays.com/). All rights reserved.
//
// This file is part of obZrv.
// 
// obZrv is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// obZrv is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with obZrv. If not, see <https://www.gnu.org/licenses/>.

#include "pch.h"
#include "client.hpp"
#include "remote_fs.hpp"
#include "remote_file.hpp"

#include <string.h>
#include <string>

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

UniFsClient::UniFsClient()
	: socket_(io_ctx_)
	, heartbeat_timer_(io_ctx_)
	, work_guard_(asio::make_work_guard(io_ctx_))
	, writing_(false)
	, next_req_id_(1)
	, connected_(false)
{}

UniFsClient::~UniFsClient() {
	// Close the socket from the IO thread so no async operations race against it.
	asio::post(io_ctx_, [this]() {
		heartbeat_timer_.cancel();
		std::error_code ec;
		socket_.close(ec);
	});
	work_guard_.reset();
	if (io_thread_.joinable()) io_thread_.join();

	// Wake up any requests that did not receive a response.
	std::lock_guard<std::mutex> lock(pending_mutex_);
	for (auto& kv : pending_) {
		Response r;
		r.status = STATUS_ERR_GENERIC;
		kv.second.set_value(std::move(r));
	}
	pending_.clear();
}

// ---------------------------------------------------------------------------
// connect()
// ---------------------------------------------------------------------------

std::unique_ptr<UniFsClient> UniFsClient::connect(const char* host, uint16_t port) {
	std::unique_ptr<UniFsClient> c(new UniFsClient());

	asio::ip::tcp::resolver resolver(c->io_ctx_);
	std::error_code ec;
	auto endpoints = resolver.resolve(host, std::to_string(port), ec);
	if (ec) return std::unique_ptr<UniFsClient>();

	asio::connect(c->socket_, endpoints, ec);
	if (ec) return std::unique_ptr<UniFsClient>();

	// Disable Nagle algorithm for lower request-response latency.
	c->socket_.set_option(asio::ip::tcp::no_delay(true), ec);

	c->connected_.store(true);
	c->start_read();
	c->schedule_heartbeat();

	UniFsClient* raw = c.get();
	c->io_thread_ = std::thread([raw]() { raw->io_ctx_.run(); });

	return c;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

std::unique_ptr<UniFs> UniFsClient::open_dir(const char* path) {
	uint16_t path_len = (uint16_t)strlen(path);
	std::vector<uint8_t> payload(2 + path_len);
	proto_write_u16(payload.data(), path_len);
	memcpy(payload.data() + 2, path, path_len);

	Response resp = send_request(CMD_FS_OPEN, payload.data(), (uint32_t)payload.size());
	if (resp.status != STATUS_OK || resp.payload.size() < 4)
		return std::unique_ptr<UniFs>();

	uint32_t handle = proto_read_u32(resp.payload.data());
	return std::unique_ptr<UniFs>(new RemoteFs(*this, handle));
}

std::unique_ptr<UniFile> UniFsClient::open_file(const char* path, const char* mode) {
	// Map fopen mode string to protocol flags.
	uint8_t flags;
	if (strcmp(mode, "r")  == 0 || strcmp(mode, "rb")  == 0) flags = FILE_FLAG_READ;
	else if (strcmp(mode, "w")  == 0 || strcmp(mode, "wb")  == 0) flags = FILE_FLAG_WRITE;
	else if (strcmp(mode, "r+") == 0 || strcmp(mode, "r+b") == 0) flags = FILE_FLAG_RDWR;
	else if (strcmp(mode, "a")  == 0 || strcmp(mode, "ab")  == 0) flags = FILE_FLAG_APPEND;
	else return std::unique_ptr<UniFile>();

	uint16_t path_len = (uint16_t)strlen(path);
	std::vector<uint8_t> payload(3 + path_len);
	payload[0] = flags;
	proto_write_u16(payload.data() + 1, path_len);
	memcpy(payload.data() + 3, path, path_len);

	Response resp = send_request(CMD_FILE_OPEN, payload.data(), (uint32_t)payload.size());
	if (resp.status != STATUS_OK || resp.payload.size() < 4)
		return std::unique_ptr<UniFile>();

	uint32_t handle = proto_read_u32(resp.payload.data());
	return std::unique_ptr<UniFile>(new RemoteFile(*this, handle));
}

// ---------------------------------------------------------------------------
// send_request -- called from any thread
// ---------------------------------------------------------------------------

UniFsClient::Response UniFsClient::send_request(uint8_t cmd,
												 const uint8_t* payload,
												 uint32_t plen) {
	Response err_resp;
	err_resp.status = STATUS_ERR_GENERIC;

	uint32_t total = FRAME_HEADER_SIZE + plen;
	std::vector<uint8_t> frame(total);

	uint16_t req_id;
	std::future<Response> fut;
	{
		// Check connected and insert into pending atomically under the same lock.
		// This prevents a race where on_disconnect() clears pending_ between the
		// connected check and the insert, leaving the promise with no one to fulfill it.
		std::lock_guard<std::mutex> lock(pending_mutex_);
		if (!connected_.load()) return err_resp;
		req_id = alloc_req_id_locked();
		std::promise<Response> prom;
		fut = prom.get_future();
		pending_[req_id] = std::move(prom);
	}

	proto_write_u32(frame.data(), total);
	proto_write_u16(frame.data() + 4, req_id);
	frame[6] = cmd;
	if (plen > 0) memcpy(frame.data() + FRAME_HEADER_SIZE, payload, plen);

	enqueue_write(std::move(frame));

	// Wait up to 60 seconds for a response.  On timeout, close the socket from
	// the IO thread so on_disconnect() runs there and cleans up all state.
	if (fut.wait_for(std::chrono::seconds(60)) == std::future_status::timeout) {
		asio::post(io_ctx_, [this]() {
			std::error_code ec;
			socket_.close(ec);  // causes pending async_read to fail -> on_disconnect()
		});
		return err_resp;
	}
	return fut.get();
}

// ---------------------------------------------------------------------------
// req_id allocation
// ---------------------------------------------------------------------------

uint16_t UniFsClient::alloc_req_id_locked() {
	uint16_t id = next_req_id_++;
	if (next_req_id_ == 0) next_req_id_ = 1;  // skip 0 (reserved for heartbeat)
	return id;
}

// ---------------------------------------------------------------------------
// Write queue
// ---------------------------------------------------------------------------

void UniFsClient::enqueue_write(std::vector<uint8_t> frame) {
	std::lock_guard<std::mutex> lock(write_mutex_);
	write_queue_.push_back(std::move(frame));
	if (!writing_) {
		writing_ = true;
		asio::post(io_ctx_, [this]() { do_write(); });
	}
}

void UniFsClient::do_write() {
	// IO thread only.
	std::vector<uint8_t>* buf;
	{
		std::lock_guard<std::mutex> lock(write_mutex_);
		if (write_queue_.empty()) { writing_ = false; return; }
		buf = &write_queue_.front();
		// Front element stays alive until the completion handler pops it.
		// deque::push_back does not invalidate references to front.
	}
	asio::async_write(socket_, asio::buffer(*buf),
		[this](std::error_code ec, size_t) {
			if (ec) { on_disconnect(); return; }
			{
				std::lock_guard<std::mutex> lock(write_mutex_);
				write_queue_.pop_front();
			}
			do_write();
		});
}

// ---------------------------------------------------------------------------
// Read pipeline (IO thread only)
// ---------------------------------------------------------------------------

void UniFsClient::start_read() {
	asio::async_read(socket_,
		asio::buffer(header_buf_, FRAME_HEADER_SIZE),
		[this](std::error_code ec, size_t) {
			if (ec) { on_disconnect(); return; }
			on_header();
		});
}

void UniFsClient::on_header() {
	uint32_t len    = proto_read_u32(header_buf_);
	uint16_t req_id = proto_read_u16(header_buf_ + 4);
	uint8_t  status = header_buf_[6];

	if (len < FRAME_HEADER_SIZE) { on_disconnect(); return; }
	uint32_t plen = len - FRAME_HEADER_SIZE;
	if (plen > MAX_PAYLOAD_SIZE)  { on_disconnect(); return; }

	payload_buf_.resize(plen);

	if (plen == 0) {
		complete_response(req_id, status, nullptr, 0);
		start_read();
		return;
	}

	asio::async_read(socket_, asio::buffer(payload_buf_),
		[this, req_id, status](std::error_code ec, size_t) {
			if (ec) { on_disconnect(); return; }
			complete_response(req_id, status,
							  payload_buf_.data(), (uint32_t)payload_buf_.size());
			start_read();
		});
}

void UniFsClient::complete_response(uint16_t req_id, uint8_t status,
									const uint8_t* data, uint32_t dlen) {
	if (req_id == 0) return;  // heartbeat echo, no pending entry

	std::lock_guard<std::mutex> lock(pending_mutex_);
	auto it = pending_.find(req_id);
	if (it == pending_.end()) return;  // unexpected response

	Response resp;
	resp.status = status;
	if (dlen > 0) resp.payload.assign(data, data + dlen);
	it->second.set_value(std::move(resp));
	pending_.erase(it);
}

// ---------------------------------------------------------------------------
// Disconnect handling
// ---------------------------------------------------------------------------

void UniFsClient::on_disconnect() {
	heartbeat_timer_.cancel();

	// Set connected_ inside pending_mutex_ so that send_request()'s combined
	// check-and-insert is atomic with respect to this drain.
	std::lock_guard<std::mutex> lock(pending_mutex_);
	connected_.store(false);
	for (auto& kv : pending_) {
		Response r;
		r.status = STATUS_ERR_GENERIC;
		kv.second.set_value(std::move(r));
	}
	pending_.clear();
}

// ---------------------------------------------------------------------------
// Heartbeat
// ---------------------------------------------------------------------------

void UniFsClient::schedule_heartbeat() {
	heartbeat_timer_.expires_after(std::chrono::seconds(HEARTBEAT_INTERVAL_SEC));
	heartbeat_timer_.async_wait([this](std::error_code ec) {
		if (ec) return;  // cancelled
		uint8_t frame[FRAME_HEADER_SIZE];
		proto_write_u32(frame, FRAME_HEADER_SIZE);
		proto_write_u16(frame + 4, 0);  // req_id = 0
		frame[6] = CMD_HEARTBEAT;
		enqueue_write(std::vector<uint8_t>(frame, frame + FRAME_HEADER_SIZE));
		schedule_heartbeat();
	});
}
