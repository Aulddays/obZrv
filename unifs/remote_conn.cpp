#include "remote_conn.hpp"

#include <string.h>
#include <string>

// ---------------------------------------------------------------------------
// Constructor / destructor
// ---------------------------------------------------------------------------

RemoteConn::RemoteConn()
	: socket_(io_ctx_)
	, heartbeat_timer_(io_ctx_)
	, work_guard_(asio::make_work_guard(io_ctx_))
	, writing_(false)
	, next_req_id_(1)
	, connected_(false)
{}

RemoteConn::~RemoteConn() {
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

std::shared_ptr<RemoteConn> RemoteConn::connect(const char* host, uint16_t port) {
	std::shared_ptr<RemoteConn> c(new RemoteConn());

	asio::ip::tcp::resolver resolver(c->io_ctx_);
	std::error_code ec;
	auto endpoints = resolver.resolve(host, std::to_string(port), ec);
	if (ec) return std::shared_ptr<RemoteConn>();

	asio::connect(c->socket_, endpoints, ec);
	if (ec) return std::shared_ptr<RemoteConn>();

	// Disable Nagle algorithm for lower request-response latency.
	c->socket_.set_option(asio::ip::tcp::no_delay(true), ec);

	c->connected_.store(true);
	c->start_read();
	c->schedule_heartbeat();

	RemoteConn* raw = c.get();
	c->io_thread_ = std::thread([raw]() { raw->io_ctx_.run(); });

	return c;
}

// ---------------------------------------------------------------------------
// send_request -- called from any thread
// ---------------------------------------------------------------------------

RemoteConn::Response RemoteConn::send_request(uint8_t cmd,
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

uint16_t RemoteConn::alloc_req_id_locked() {
	uint16_t id = next_req_id_++;
	if (next_req_id_ == 0) next_req_id_ = 1;  // skip 0 (reserved for heartbeat)
	return id;
}

// ---------------------------------------------------------------------------
// Write queue
// ---------------------------------------------------------------------------

void RemoteConn::enqueue_write(std::vector<uint8_t> frame) {
	std::lock_guard<std::mutex> lock(write_mutex_);
	write_queue_.push_back(std::move(frame));
	if (!writing_) {
		writing_ = true;
		asio::post(io_ctx_, [this]() { do_write(); });
	}
}

void RemoteConn::do_write() {
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

void RemoteConn::start_read() {
	asio::async_read(socket_,
		asio::buffer(header_buf_, FRAME_HEADER_SIZE),
		[this](std::error_code ec, size_t) {
			if (ec) { on_disconnect(); return; }
			on_header();
		});
}

void RemoteConn::on_header() {
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

void RemoteConn::complete_response(uint16_t req_id, uint8_t status,
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

void RemoteConn::on_disconnect() {
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

void RemoteConn::schedule_heartbeat() {
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
