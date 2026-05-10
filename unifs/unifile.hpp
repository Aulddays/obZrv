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
#include <stdint.h>
#include <stddef.h>
#include <memory>

class UniFile {
public:
	virtual ~UniFile() {}

	virtual int     seek(int64_t offset, int whence) = 0;  // 0 on success, -1 on failure
	virtual int64_t tell()                           = 0;  // -1 on failure
	virtual size_t  read(void* buf, size_t size)     = 0;  // bytes read; 0 on EOF or error
	virtual size_t  write(const void* buf, size_t size) = 0;  // bytes written
	virtual int     close()                          = 0;  // 0 on success

	UniFile(const UniFile&)            = delete;
	UniFile& operator=(const UniFile&) = delete;

protected:
	UniFile() {}
};
