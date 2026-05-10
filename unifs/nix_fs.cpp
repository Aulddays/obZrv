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
#ifndef _WIN32

#include "nix_fs.hpp"
#include <sys/stat.h>
#include <unistd.h>  // unlink()
#include <string.h>
#include <string>

NixFs::NixFs(const char* path)
	: dir_(nullptr)
	, base_(path)
{
	// Ensure base_ ends with '/' for straightforward name concatenation.
	if (!base_.empty() && base_.back() != '/')
		base_ += '/';
}

NixFs::~NixFs() {
	close();
}

int NixFs::readdir() {
	if (dir_) return 0;  // already open
	dir_ = ::opendir(base_.c_str());
	return dir_ ? 0 : -1;
}

DirEntry* NixFs::next() {
	if (!dir_) return nullptr;

	struct dirent* ent;
	while ((ent = ::readdir(dir_)) != nullptr) {
		// Skip "." and ".."
		if (ent->d_name[0] == '.' &&
			(ent->d_name[1] == '\0' ||
			 (ent->d_name[1] == '.' && ent->d_name[2] == '\0'))) {
			continue;
		}

		entry_.name_ = ent->d_name;
		entry_.size_ = 0;
		entry_.ctime_ = 0;
		entry_.mtime_ = 0;

		unsigned char d_type = ent->d_type;
		std::string full_path = base_ + ent->d_name;

		// Always stat() to obtain time fields; also resolves DT_UNKNOWN type.
		// Cast via uint64_t first to avoid sign-extension issues on 32-bit systems
		// where time_t is int32_t: (uint32_t)(uint64_t) is safe until year 2106.
		struct stat st;
		if (stat(full_path.c_str(), &st) == 0) {
			entry_.mtime_ = (uint32_t)(uint64_t)st.st_mtime;
			entry_.ctime_ = entry_.mtime_;  // Linux stat() has no birth time
			if (d_type == DT_UNKNOWN) {
				if (S_ISREG(st.st_mode))       d_type = DT_REG;
				else if (S_ISDIR(st.st_mode))  d_type = DT_DIR;
				else if (S_ISLNK(st.st_mode))  d_type = DT_LNK;
			}
			if (d_type == DT_REG)
				entry_.size_ = (uint64_t)st.st_size;
		}

		if (d_type == DT_REG)       entry_.type_ = DirEntry::FILE;
		else if (d_type == DT_DIR)  entry_.type_ = DirEntry::DIR;
		else if (d_type == DT_LNK)  entry_.type_ = DirEntry::SYMLINK;
		else                        entry_.type_ = DirEntry::OTHER;

		return &entry_;
	}

	return nullptr;
}

int NixFs::rewind() {
	if (!dir_) return -1;
	::rewinddir(dir_);
	return 0;
}

void NixFs::close() {
	if (dir_) {
		closedir(dir_);
		dir_ = nullptr;
	}
}

int NixFs::remove(const char* name) {
	// Reject names containing path separators.
	if (!name || !name[0] || strchr(name, '/') != nullptr) return -1;
	std::string full_path = base_ + name;
	// Use unlink() (not ::remove()) so that directories are always rejected.
	return ::unlink(full_path.c_str());
}

#endif // !_WIN32
