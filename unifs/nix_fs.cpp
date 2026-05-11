#ifndef _WIN32

#include "nix_fs.hpp"
#include "local_file.hpp"
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <string>

// ---------------------------------------------------------------------------
// NixDirIterImpl
// ---------------------------------------------------------------------------

NixDirIterImpl::NixDirIterImpl(const char* path)
	: dir_(nullptr)
	, base_(path)
{
	if (!base_.empty() && base_.back() != '/')
		base_ += '/';
	dir_ = ::opendir(base_.c_str());
}

NixDirIterImpl::~NixDirIterImpl() {
	if (dir_) {
		closedir(dir_);
		dir_ = nullptr;
	}
}

const DirEntry* NixDirIterImpl::next() {
	if (!dir_) return nullptr;

	struct dirent* ent;
	while ((ent = ::readdir(dir_)) != nullptr) {
		// Skip "." and ".."
		if (ent->d_name[0] == '.' &&
			(ent->d_name[1] == '\0' ||
			 (ent->d_name[1] == '.' && ent->d_name[2] == '\0')))
			continue;

		entry_.name_  = ent->d_name;
		entry_.size_  = 0;
		entry_.ctime_ = 0;
		entry_.mtime_ = 0;

		unsigned char d_type = ent->d_type;
		std::string full_path = base_ + ent->d_name;

		// stat() provides timestamps and resolves DT_UNKNOWN type.
		// Cast via uint64_t before uint32_t to avoid sign-extension on 32-bit systems.
		struct stat st;
		if (stat(full_path.c_str(), &st) == 0) {
			entry_.mtime_ = (uint32_t)(uint64_t)st.st_mtime;
			entry_.ctime_ = entry_.mtime_;  // Linux stat() has no birth time
			if (d_type == DT_UNKNOWN) {
				if      (S_ISREG(st.st_mode)) d_type = DT_REG;
				else if (S_ISDIR(st.st_mode)) d_type = DT_DIR;
				else if (S_ISLNK(st.st_mode)) d_type = DT_LNK;
			}
			if (d_type == DT_REG)
				entry_.size_ = (uint64_t)st.st_size;
		}

		if      (d_type == DT_REG) entry_.type_ = DirEntry::FILE;
		else if (d_type == DT_DIR) entry_.type_ = DirEntry::DIR;
		else if (d_type == DT_LNK) entry_.type_ = DirEntry::SYMLINK;
		else                       entry_.type_ = DirEntry::OTHER;

		return &entry_;
	}
	return nullptr;
}

// ---------------------------------------------------------------------------
// NixFs
// ---------------------------------------------------------------------------

std::unique_ptr<UniFs> NixFs::open() {
	return std::unique_ptr<UniFs>(new NixFs());
}

DirIter NixFs::readdir(const char* path) {
	NixDirIterImpl* impl = new NixDirIterImpl(path);
	if (!impl->valid()) {
		delete impl;
		return DirIter();
	}
	return DirIter(std::unique_ptr<DirIterImpl>(impl));
}

std::unique_ptr<UniFile> NixFs::openfile(const char* path, const char* mode) {
	FILE* fp = fopen(path, mode);
	if (!fp) return std::unique_ptr<UniFile>();
	return std::unique_ptr<UniFile>(new LocalFile(fp));
}

int NixFs::removefile(const char* path) {
	// unlink() rejects directories, which is the desired behaviour.
	return ::unlink(path) == 0 ? 0 : -1;
}

#endif // !_WIN32
