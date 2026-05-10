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
#include "remote_fs.hpp"
#include "client.hpp"
#include "protocol.hpp"

#include <string.h>

RemoteFs::RemoteFs(UniFsClient& client, uint32_t handle)
	: client_(client), handle_(handle), closed_(false)
{}

RemoteFs::~RemoteFs() {
	close();
}

int RemoteFs::readdir() {
	if (closed_) return -1;

	uint8_t payload[4];
	proto_write_u32(payload, handle_);

	UniFsClient::Response resp =
		client_.send_request(CMD_FS_READDIR, payload, 4);

	return (resp.status == STATUS_OK) ? 0 : -1;
}

DirEntry* RemoteFs::next() {
	if (closed_) return nullptr;

	uint8_t payload[4];
	proto_write_u32(payload, handle_);

	UniFsClient::Response resp =
		client_.send_request(CMD_FS_NEXT, payload, 4);

	if (resp.status != STATUS_OK || resp.payload.empty()) return nullptr;

	if (resp.payload[0] == 0) return nullptr;  // no more entries

	// has_entry(1) + type(1) + size(8) + ctime(4) + mtime(4) + name_len(2) + name(N)
	if (resp.payload.size() < 20) return nullptr;

	uint8_t  type     = resp.payload[1];
	uint64_t size     = proto_read_u64(resp.payload.data() + 2);
	uint32_t ctime    = proto_read_u32(resp.payload.data() + 10);
	uint32_t mtime    = proto_read_u32(resp.payload.data() + 14);
	uint16_t name_len = proto_read_u16(resp.payload.data() + 18);

	if (resp.payload.size() < (size_t)(20 + name_len)) return nullptr;

	entry_.type_  = (DirEntry::Type)type;
	entry_.size_  = size;
	entry_.ctime_ = ctime;
	entry_.mtime_ = mtime;
	entry_.name_.assign((const char*)resp.payload.data() + 20, name_len);

	return &entry_;
}

int RemoteFs::rewind() {
	if (closed_) return -1;

	uint8_t payload[4];
	proto_write_u32(payload, handle_);

	UniFsClient::Response resp =
		client_.send_request(CMD_FS_REWIND, payload, 4);

	return (resp.status == STATUS_OK) ? 0 : -1;
}

void RemoteFs::close() {
	if (closed_) return;
	closed_ = true;

	uint8_t payload[4];
	proto_write_u32(payload, handle_);
	client_.send_request(CMD_FS_CLOSE, payload, 4);
}

int RemoteFs::remove(const char* name) {
	if (closed_ || !name || !name[0]) return -1;

	uint16_t name_len = (uint16_t)strlen(name);
	// fs_handle:u32 + name_len:u16 + name:N
	std::vector<uint8_t> payload(6 + name_len);
	proto_write_u32(payload.data(),     handle_);
	proto_write_u16(payload.data() + 4, name_len);
	memcpy(payload.data() + 6, name, name_len);

	UniFsClient::Response resp =
		client_.send_request(CMD_FS_REMOVE, payload.data(), (uint32_t)payload.size());

	return (resp.status == STATUS_OK) ? 0 : -1;
}
