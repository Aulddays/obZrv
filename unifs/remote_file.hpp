#pragma once
#include "unifile.hpp"
#include "remote_conn.hpp"
#include <memory>
#include <stdint.h>

class RemoteFile : public UniFile {
public:
	RemoteFile(std::shared_ptr<RemoteConn> conn, uint32_t handle);
	~RemoteFile();

	int     seek(int64_t offset, int whence) override;
	int64_t tell()                           override;
	size_t  read(void* buf, size_t size)     override;
	size_t  write(const void* buf, size_t size) override;
	int     close()                          override;

private:
	std::shared_ptr<RemoteConn> conn_;
	uint32_t                    handle_;
	bool                        closed_;
};
