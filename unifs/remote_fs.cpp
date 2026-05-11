#include "remote_fs.hpp"
#include "remote_file.hpp"
#include "protocol.hpp"
#include "unifs.hpp"  // full DirEntry definition

#include <string.h>
#include <string>

// ---------------------------------------------------------------------------
// RemoteDirIterImpl  (internal; created only by RemoteFs::readdir)
// ---------------------------------------------------------------------------

// Iterates over a pre-fetched batch of DirEntry objects.
// Entries are owned by the iterator; pointers remain valid until the next
// call to next().
class RemoteDirIterImpl : public DirIterImpl {
public:
	explicit RemoteDirIterImpl(std::vector<std::unique_ptr<DirEntry>> entries)
		: entries_(std::move(entries)), idx_(0) {}

	const DirEntry* next() override {
		if (idx_ >= entries_.size()) return nullptr;
		return entries_[idx_++].get();
	}

private:
	std::vector<std::unique_ptr<DirEntry>> entries_;
	size_t                                 idx_;
};

// ---------------------------------------------------------------------------
// RemoteFs
// ---------------------------------------------------------------------------

RemoteFs::RemoteFs(std::shared_ptr<RemoteConn> conn)
	: conn_(std::move(conn))
{}

std::unique_ptr<UniFs> RemoteFs::open(const char* host, uint16_t port) {
	std::shared_ptr<RemoteConn> conn = RemoteConn::connect(host, port);
	if (!conn) return std::unique_ptr<UniFs>();
	return std::unique_ptr<UniFs>(new RemoteFs(std::move(conn)));
}

DirIter RemoteFs::readdir(const char* path) {
	uint16_t path_len = (uint16_t)strlen(path);
	std::vector<uint8_t> payload(2 + path_len);
	proto_write_u16(payload.data(), path_len);
	memcpy(payload.data() + 2, path, path_len);

	RemoteConn::Response resp =
		conn_->send_request(CMD_FS_READDIR, payload.data(), (uint32_t)payload.size());

	if (resp.status != STATUS_OK || resp.payload.size() < 4)
		return DirIter();

	uint32_t count = proto_read_u32(resp.payload.data());
	std::vector<std::unique_ptr<DirEntry>> entries;
	entries.reserve(count);

	// Wire format per entry: type:u8 + ctime:u32 + mtime:u32 + size:u64 +
	//                        name_len:u16 + name:N  (19 bytes fixed + name)
	const uint8_t* p   = resp.payload.data() + 4;
	const uint8_t* end = resp.payload.data() + resp.payload.size();

	for (uint32_t i = 0; i < count; ++i) {
		if (p + 19 > end) return DirIter();  // truncated response
		DirEntry::Type type  = (DirEntry::Type)p[0];
		uint32_t       ctime = proto_read_u32(p +  1);
		uint32_t       mtime = proto_read_u32(p +  5);
		uint64_t       size  = proto_read_u64(p +  9);
		uint16_t       nlen  = proto_read_u16(p + 17);
		p += 19;
		if (p + nlen > end) return DirIter();  // truncated name

		// DirEntry's private parameterised constructor is accessible because
		// RemoteFs is declared a friend of DirEntry in unifs.hpp.
		entries.push_back(std::unique_ptr<DirEntry>(
			new DirEntry(std::string((const char*)p, nlen), type, size, ctime, mtime)));
		p += nlen;
	}

	return DirIter(std::unique_ptr<DirIterImpl>(
		new RemoteDirIterImpl(std::move(entries))));
}

std::unique_ptr<UniFile> RemoteFs::openfile(const char* path, const char* mode) {
	uint8_t flags;
	if      (strcmp(mode, "r")   == 0 || strcmp(mode, "rb")  == 0) flags = FILE_FLAG_READ;
	else if (strcmp(mode, "w")   == 0 || strcmp(mode, "wb")  == 0) flags = FILE_FLAG_WRITE;
	else if (strcmp(mode, "r+")  == 0 || strcmp(mode, "r+b") == 0) flags = FILE_FLAG_RDWR;
	else if (strcmp(mode, "a")   == 0 || strcmp(mode, "ab")  == 0) flags = FILE_FLAG_APPEND;
	else return std::unique_ptr<UniFile>();

	uint16_t path_len = (uint16_t)strlen(path);
	std::vector<uint8_t> payload(3 + path_len);
	payload[0] = flags;
	proto_write_u16(payload.data() + 1, path_len);
	memcpy(payload.data() + 3, path, path_len);

	RemoteConn::Response resp =
		conn_->send_request(CMD_FILE_OPEN, payload.data(), (uint32_t)payload.size());
	if (resp.status != STATUS_OK || resp.payload.size() < 4)
		return std::unique_ptr<UniFile>();

	uint32_t handle = proto_read_u32(resp.payload.data());
	return std::unique_ptr<UniFile>(new RemoteFile(conn_, handle));
}

int RemoteFs::removefile(const char* path) {
	uint16_t path_len = (uint16_t)strlen(path);
	std::vector<uint8_t> payload(2 + path_len);
	proto_write_u16(payload.data(), path_len);
	memcpy(payload.data() + 2, path, path_len);

	RemoteConn::Response resp =
		conn_->send_request(CMD_FS_REMOVE, payload.data(), (uint32_t)payload.size());
	return (resp.status == STATUS_OK) ? 0 : -1;
}
