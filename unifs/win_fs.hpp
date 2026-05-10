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
#ifdef _WIN32

#include "unifs.hpp"
#include <windows.h>

class WinFs : public UniFs {
public:
	explicit WinFs(const char* path);
	~WinFs();

	int       readdir() override;
	DirEntry* next()    override;
	int       rewind()  override;
	void      close()   override;
	int       remove(const char* name) override;

private:
	// Converts UTF-8 path to a wide-character search pattern ("path\*").
	static int to_search_path(const char* path, wchar_t* buf, int buf_size);

	std::string      base_utf8_;    // original UTF-8 Windows path (for remove())
	HANDLE           handle_;
	WIN32_FIND_DATAW find_data_;
	bool             has_first_;    // FindFirstFileW result pending consumption
	wchar_t          search_path_[MAX_PATH + 4];
	DirEntry         entry_;

	// Root-mode: path was "/" -- enumerate logical drives instead.
	bool             root_mode_;
	DWORD            drives_mask_;  // bitmask from GetLogicalDrives()
	int              drive_idx_;    // current bit index (0=A, 2=C, ...)
};

#endif // _WIN32
