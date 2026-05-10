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
#ifndef _WIN32

#include "unifs.hpp"
#include <dirent.h>

class NixFs : public UniFs {
public:
	explicit NixFs(const char* path);
	~NixFs();

	int       readdir() override;
	DirEntry* next()    override;
	int       rewind()  override;
	void      close()   override;
	int       remove(const char* name) override;

private:
	DIR*        dir_;
	std::string base_;  // directory path, used to build full paths for stat()/remove()
	DirEntry    entry_;
};

#endif // !_WIN32
