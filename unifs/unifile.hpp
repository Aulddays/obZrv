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
