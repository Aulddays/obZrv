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
#include "remote_file.hpp"
#include "client.hpp"
#include "protocol.hpp"

#include <string.h>

// Maximum bytes requested per FILE_READ message (leaves room for bytes_read prefix).
static const uint32_t MAX_READ_CHUNK = MAX_PAYLOAD_SIZE - 4;

RemoteFile::RemoteFile(UniFsClient& client, uint32_t handle)
	: client_(client), handle_(handle), closed_(false)
{}

RemoteFile::~RemoteFile() {
	close();
}

int RemoteFile::seek(int64_t offset, int whence) {
	if (closed_) return -1;

	uint8_t payload[13];
	proto_write_u32(payload, handle_);
	proto_write_i64(payload + 4, offset);
	payload[12] = (uint8_t)whence;

	UniFsClient::Response resp =
		client_.send_request(CMD_FILE_SEEK, payload, 13);

	return (resp.status == STATUS_OK) ? 0 : -1;
}

int64_t RemoteFile::tell() {
	if (closed_) return -1;

	uint8_t payload[4];
	proto_write_u32(payload, handle_);

	UniFsClient::Response resp =
		client_.send_request(CMD_FILE_TELL, payload, 4);

	if (resp.status != STATUS_OK || resp.payload.size() < 8) return -1;
	return proto_read_i64(resp.payload.data());
}

size_t RemoteFile::read(void* buf, size_t size) {
	if (closed_ || size == 0) return 0;

	size_t  total = 0;
	uint8_t* dst  = (uint8_t*)buf;

	while (size > 0) {
		uint32_t chunk = (size > MAX_READ_CHUNK) ? MAX_READ_CHUNK : (uint32_t)size;

		uint8_t payload[8];
		proto_write_u32(payload, handle_);
		proto_write_u32(payload + 4, chunk);

		UniFsClient::Response resp =
			client_.send_request(CMD_FILE_READ, payload, 8);

		if (resp.status != STATUS_OK || resp.payload.size() < 4) break;

		uint32_t n = proto_read_u32(resp.payload.data());
		if (n == 0) break;  // EOF

		if (resp.payload.size() < (size_t)(4 + n)) break;  // malformed
		memcpy(dst, resp.payload.data() + 4, n);

		dst   += n;
		total += n;
		size  -= n;

		if (n < chunk) break;  // partial read means EOF reached
	}

	return total;
}

size_t RemoteFile::write(const void* buf, size_t size) {
	if (closed_ || size == 0) return 0;

	// Clamp to max single-frame payload (data_len(4) + data(N) <= MAX_PAYLOAD_SIZE).
	uint32_t data_len = (size > MAX_PAYLOAD_SIZE - 8)
					  ? (MAX_PAYLOAD_SIZE - 8)
					  : (uint32_t)size;

	std::vector<uint8_t> payload(8 + data_len);
	proto_write_u32(payload.data(), handle_);
	proto_write_u32(payload.data() + 4, data_len);
	memcpy(payload.data() + 8, buf, data_len);

	UniFsClient::Response resp =
		client_.send_request(CMD_FILE_WRITE, payload.data(), (uint32_t)payload.size());

	if (resp.status != STATUS_OK || resp.payload.size() < 4) return 0;
	return (size_t)proto_read_u32(resp.payload.data());
}

int RemoteFile::close() {
	if (closed_) return 0;
	closed_ = true;

	uint8_t payload[4];
	proto_write_u32(payload, handle_);

	UniFsClient::Response resp =
		client_.send_request(CMD_FILE_CLOSE, payload, 4);

	return (resp.status == STATUS_OK) ? 0 : -1;
}
