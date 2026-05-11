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
