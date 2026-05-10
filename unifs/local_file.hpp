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
#include <stdio.h>
#include <memory>

class LocalFile : public UniFile {
public:
	explicit LocalFile(FILE* fp) : fp_(fp) {}
	~LocalFile() { close(); }

	int     seek(int64_t offset, int whence) override;
	int64_t tell()                           override;
	size_t  read(void* buf, size_t size)     override;
	size_t  write(const void* buf, size_t size) override;
	int     close()                          override;

private:
	FILE* fp_;
};

// Opens a file; mode follows fopen conventions ("r", "w", "r+", "rb", etc.).
// Returns nullptr on failure.
std::unique_ptr<UniFile> open_file(const char* path, const char* mode);
