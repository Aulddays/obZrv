#pragma once
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#ifndef ASIO_NO_EXCEPTIONS
#define ASIO_NO_EXCEPTIONS
#endif
#include <asio/asio.hpp>

#include "protocol.hpp"

#include <stdint.h>
#include <memory>
#include <vector>
#include <map>
#include <deque>
#include <mutex>
#include <atomic>
#include <thread>
#include <future>

// Shared connection state for remote filesystem operations.
// Kept alive by reference counting as long as any RemoteFs, RemoteFile, or
// RemoteDirIter holds a shared_ptr to it.  Destroying RemoteFs does not close
// the connection; the last shared_ptr release triggers the teardown.
class RemoteConn {
public:
	struct Response {
		uint8_t              status;
		std::vector<uint8_t> payload;
	};

	// Synchronously connects to host:port; returns nullptr on failure.
	static std::shared_ptr<RemoteConn> connect(const char* host, uint16_t port);
	~RemoteConn();

	// Sends a request and blocks until a response arrives or times out.
	// Safe to call from any thread.
	Response send_request(uint8_t cmd, const uint8_t* payload, uint32_t len);

private:
	RemoteConn();

	asio::io_context      io_ctx_;
	asio::ip::tcp::socket socket_;
	asio::steady_timer    heartbeat_timer_;
	asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
	std::thread           io_thread_;

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
	void complete_response(uint16_t req_id, uint8_t status,
	                       const uint8_t* data, uint32_t dlen);

	void on_disconnect();
	void schedule_heartbeat();
};
