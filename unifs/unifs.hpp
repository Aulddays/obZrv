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
#include <string>
#include <memory>

class DirEntry {
public:
	enum Type : uint8_t { FILE = 0, DIR = 1, SYMLINK = 2, OTHER = 3 };

	DirEntry(const DirEntry&)            = delete;
	DirEntry& operator=(const DirEntry&) = delete;

	const char* name()  const { return name_.c_str(); }
	Type        type()  const { return type_; }
	uint64_t    size()  const { return size_; }   // valid only for Type::FILE
	// Seconds since Unix epoch (uint32_t, valid until 2106).
	// On Linux, ctime equals mtime because stat() does not expose birth time.
	uint32_t    ctime() const { return ctime_; }  // creation time (or mtime on Linux)
	uint32_t    mtime() const { return mtime_; }  // last modification time

	// Returns an independent heap-allocated copy; safe to keep after next()
	std::unique_ptr<DirEntry> copy() const {
		return std::unique_ptr<DirEntry>(new DirEntry(name_, type_, size_, ctime_, mtime_));
	}

private:
	DirEntry() : type_(OTHER), size_(0), ctime_(0), mtime_(0) {}
	DirEntry(const std::string& n, Type t, uint64_t s, uint32_t ct, uint32_t mt)
		: name_(n), type_(t), size_(s), ctime_(ct), mtime_(mt) {}

	std::string name_;
	Type        type_;
	uint64_t    size_;
	uint32_t    ctime_;
	uint32_t    mtime_;

	friend class NixFs;
	friend class WinFs;
	friend class RemoteFs;
};

class UniFs {
public:
	virtual ~UniFs() {}

	// Opens the directory for iteration.  Must be called before next()/rewind().
	// Returns 0 on success, -1 on failure (e.g. path does not exist).
	virtual int       readdir() = 0;

	// Returns the next entry; nullptr when iteration is complete.
	// The returned pointer is invalidated by the next call to next().
	virtual DirEntry* next()   = 0;
	virtual int       rewind() = 0;  // 0 on success, -1 on failure
	virtual void      close()  = 0;

	// Removes a single file named `name` within this directory.
	// `name` must be a plain filename (no path separators).
	// Returns 0 on success, -1 on failure.
	virtual int       remove(const char* name) = 0;

	UniFs(const UniFs&)            = delete;
	UniFs& operator=(const UniFs&) = delete;

protected:
	UniFs() {}
};
