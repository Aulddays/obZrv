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

#pragma once
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#include <asio/asio.hpp>

#include "protocol.hpp"
#include "unifs.hpp"
#include "unifile.hpp"

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <future>

class UniFsClient {
public:
	struct Response {
		uint8_t              status;
		std::vector<uint8_t> payload;
	};

	// Synchronously connects to host:port; returns nullptr on failure.
	static std::unique_ptr<UniFsClient> connect(const char* host, uint16_t port);
	~UniFsClient();

	// Opens a remote directory for iteration; returns nullptr on failure.
	std::unique_ptr<UniFs>   open_dir(const char* path);

	// Opens a remote file; mode follows fopen conventions; returns nullptr on failure.
	std::unique_ptr<UniFile> open_file(const char* path, const char* mode);

	// Sends a request and blocks until a response is received.
	// Used internally by RemoteFs and RemoteFile.
	Response send_request(uint8_t cmd, const uint8_t* payload, uint32_t len);

private:
	UniFsClient();

	asio::io_context   io_ctx_;
	asio::ip::tcp::socket socket_;
	asio::steady_timer    heartbeat_timer_;
	asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
	std::thread io_thread_;

	// Read state -- only accessed from the IO thread.
	uint8_t              header_buf_[FRAME_HEADER_SIZE];
	std::vector<uint8_t> payload_buf_;

	// Write queue -- shared between user threads and IO thread.
	std::mutex                       write_mutex_;
	std::deque<std::vector<uint8_t>> write_queue_;
	bool                             writing_;

	// Pending request map -- shared between user threads and IO thread.
	std::mutex                                 pending_mutex_;
	std::map<uint16_t, std::promise<Response>> pending_;
	uint16_t                                   next_req_id_;

	std::atomic<bool> connected_;

	// Must be called with pending_mutex_ held.
	uint16_t alloc_req_id_locked();

	// Enqueues a frame for sending; safe to call from any thread.
	void enqueue_write(std::vector<uint8_t> frame);

	// Starts the next async_write from the write queue; IO thread only.
	void do_write();

	// Async read pipeline; IO thread only.
	void start_read();
	void on_header();
	void on_payload(uint16_t req_id, uint8_t status);
	void complete_response(uint16_t req_id, uint8_t status,
						   const uint8_t* data, uint32_t dlen);

	void on_disconnect();
	void schedule_heartbeat();
};
