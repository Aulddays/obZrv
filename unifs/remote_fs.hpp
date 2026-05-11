#pragma once
#include "unifs.hpp"
#include "remote_conn.hpp"
#include <memory>

class RemoteFs : public UniFs {
public:
	// Creates a connection and returns a RemoteFs; returns nullptr on failure.
	static std::unique_ptr<UniFs> open(const char* host, uint16_t port);

	DirIter                  readdir(const char* path)             override;
	std::unique_ptr<UniFile> openfile(const char* path,
	                                  const char* mode)            override;
	int                      removefile(const char* path)          override;

private:
	explicit RemoteFs(std::shared_ptr<RemoteConn> conn);
	std::shared_ptr<RemoteConn> conn_;
};
