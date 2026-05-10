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
#define _CRT_SECURE_NO_WARNINGS
#include "local_file.hpp"
#include <stdio.h>
#ifdef _WIN32
#  include <windows.h>
#  include "win_path.hpp"
#endif

// Large-file seek/tell: MSVC uses _fseeki64/_ftelli64; others use fseeko/ftello.
#ifdef _MSC_VER
  #define fs_fseek(fp, off, whence) _fseeki64((fp), (off), (whence))
  #define fs_ftell(fp)              _ftelli64(fp)
#else
  #define fs_fseek(fp, off, whence) fseeko((fp), (off64_t)(off), (whence))
  #define fs_ftell(fp)              ftello(fp)
#endif

int LocalFile::seek(int64_t offset, int whence) {
	if (!fp_) return -1;
	return (fs_fseek(fp_, offset, whence) == 0) ? 0 : -1;
}

int64_t LocalFile::tell() {
	if (!fp_) return -1;
	return (int64_t)fs_ftell(fp_);
}

size_t LocalFile::read(void* buf, size_t size) {
	if (!fp_) return 0;
	return fread(buf, 1, size, fp_);
}

size_t LocalFile::write(const void* buf, size_t size) {
	if (!fp_) return 0;
	return fwrite(buf, 1, size, fp_);
}

int LocalFile::close() {
	if (fp_) {
		int ret = fclose(fp_);
		fp_ = nullptr;
		return (ret == 0) ? 0 : -1;
	}
	return 0;
}

// Factory function (declared in local_file.hpp)
std::unique_ptr<UniFile> open_file(const char* path, const char* mode) {
#ifdef _WIN32
	// On Windows, fopen() uses the ANSI code page, not UTF-8.
	// Convert Linux-style path (/c/foo) to Windows path (C:\foo) first,
	// then convert to UTF-16 and use _wfopen().
	std::string win_path = linux_path_to_win(path);
	wchar_t wpath[MAX_PATH], wmode[16];
	if (MultiByteToWideChar(CP_UTF8, 0, win_path.c_str(), -1, wpath, MAX_PATH) <= 0)
		return std::unique_ptr<UniFile>();
	if (MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, 16) <= 0)
		return std::unique_ptr<UniFile>();
	FILE* fp = _wfopen(wpath, wmode);
#else
	FILE* fp = fopen(path, mode);
#endif
	if (!fp) return std::unique_ptr<UniFile>();
	return std::unique_ptr<UniFile>(new LocalFile(fp));
}
