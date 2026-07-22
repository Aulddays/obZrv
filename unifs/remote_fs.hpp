#pragma once
#include "unifs.hpp"
#include "remote_conn.hpp"
#include <memory>
#include <string>

class RemoteFs : public UniFs {
public:
	// Creates a connection and returns a RemoteFs; returns nullptr on failure.
	static std::unique_ptr<UniFs> open(const char* host, uint16_t port);

	bool reconnect();
	uint8_t lastStatus() const { return last_status_; }
	const std::string& host() const { return host_; }
	uint16_t port() const { return port_; }

	DirIter                  readdir(const char* path)             override;
	std::unique_ptr<UniFile> openfile(const char* path,
	                                  const char* mode)            override;
	int                      removefile(const char* path)          override;

private:
	RemoteFs(const char* host, uint16_t port, std::shared_ptr<RemoteConn> conn);
	std::string host_;
	uint16_t port_;
	std::shared_ptr<RemoteConn> conn_;
	uint8_t last_status_;
};
