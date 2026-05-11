#pragma once
#ifndef _WIN32

#include "unifs.hpp"
#include <dirent.h>

// Internal per-readdir iteration state for Linux; created by NixFs::readdir().
class NixDirIterImpl : public DirIterImpl {
public:
	explicit NixDirIterImpl(const char* path);
	~NixDirIterImpl();

	bool            valid() const { return dir_ != nullptr; }
	const DirEntry* next()  override;

private:
	DIR*        dir_;
	std::string base_;
	DirEntry    entry_;
};

class NixFs : public UniFs {
public:
	static std::unique_ptr<UniFs> open();

	DirIter                  readdir(const char* path)             override;
	std::unique_ptr<UniFile> openfile(const char* path,
	                                  const char* mode)            override;
	int                      removefile(const char* path)          override;

private:
	NixFs() {}
};

#endif // !_WIN32
