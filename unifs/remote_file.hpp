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
#include "unifile.hpp"
#include <stdint.h>

class UniFsClient;

class RemoteFile : public UniFile {
public:
	RemoteFile(UniFsClient& client, uint32_t handle);
	~RemoteFile();

	int     seek(int64_t offset, int whence) override;
	int64_t tell()                           override;
	size_t  read(void* buf, size_t size)     override;
	size_t  write(const void* buf, size_t size) override;
	int     close()                          override;

private:
	UniFsClient& client_;
	uint32_t     handle_;
	bool         closed_;
};
