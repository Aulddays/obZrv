#ifdef _WIN32

#include "win_fs.hpp"
#include "local_file.hpp"
#include "win_path.hpp"
#include <string.h>

// Converts a FILETIME (100-ns intervals since 1601-01-01) to Unix seconds.
// Returns 0 for timestamps before the Unix epoch.
// Uses uint64_t arithmetic throughout to avoid overflow on 32-bit builds.
static uint32_t filetime_to_unix_sec(const FILETIME& ft) {
	uint64_t t = ((uint64_t)ft.dwHighDateTime << 32) | (uint64_t)ft.dwLowDateTime;
	// 100-ns intervals between 1601-01-01 and 1970-01-01
	static const uint64_t EPOCH_OFFSET = 116444736000000000ULL;
	if (t < EPOCH_OFFSET) return 0;
	return (uint32_t)((t - EPOCH_OFFSET) / 10000000ULL);
}

// ---------------------------------------------------------------------------
// WinDirIterImpl
// ---------------------------------------------------------------------------

// Converts UTF-8 path to a wide-character search pattern ("path\*").
int WinDirIterImpl::to_search_path(const char* path, wchar_t* buf, int buf_size) {
	int n = MultiByteToWideChar(CP_UTF8, 0, path, -1, buf, buf_size - 3);
	if (n <= 0) return -1;
	// n includes the null terminator; append "\*"
	int len = n - 1;
	if (len > 0 && buf[len - 1] != L'\\' && buf[len - 1] != L'/')
		buf[len++] = L'\\';
	buf[len++] = L'*';
	buf[len]   = L'\0';
	return len;
}

WinDirIterImpl::WinDirIterImpl(const char* path)
	: handle_(INVALID_HANDLE_VALUE)
	, has_first_(false)
	, valid_(false)
	, root_mode_(false)
	, drives_mask_(0)
	, drive_idx_(0)
{
	// Convert Linux-style path (/c/foo) to Windows native path (C:\foo).
	// linux_path_to_win returns "" for "/" (root) and for empty input.
	std::string win_path = linux_path_to_win(path);

	if (win_path.empty()) {
		// path was "/" -- root mode: enumerate logical drives on next()
		root_mode_   = true;
		drives_mask_ = GetLogicalDrives();
		valid_       = drives_mask_ != 0;
		return;
	}

	base_utf8_ = win_path;
	search_path_[0] = L'\0';

	// Build search_path_ now so it is ready for FindFirstFileW.
	if (to_search_path(win_path.c_str(), search_path_,
	                   (int)(sizeof(search_path_) / sizeof(wchar_t))) < 0)
		return;

	handle_ = FindFirstFileW(search_path_, &find_data_);
	if (handle_ == INVALID_HANDLE_VALUE) return;

	has_first_ = true;
	valid_     = true;
}

WinDirIterImpl::~WinDirIterImpl() {
	if (handle_ != INVALID_HANDLE_VALUE) {
		FindClose(handle_);
		handle_ = INVALID_HANDLE_VALUE;
	}
	// root_mode_ has no resources to release.
}

static bool is_dot(const wchar_t* name) {
	return (name[0] == L'.' &&
		(name[1] == L'\0' || (name[1] == L'.' && name[2] == L'\0')));
}

const DirEntry* WinDirIterImpl::next() {
	if (!valid_) return nullptr;

	// Root mode: return one entry per logical drive (c, d, ...).
	if (root_mode_) {
		while (drive_idx_ < 26) {
			int idx = drive_idx_++;
			if (!(drives_mask_ & (1u << idx))) continue;
			char drive_name[3] = { (char)('a' + idx), '\0', '\0' };
			entry_.name_  = drive_name;  // single lowercase letter, e.g. "c"
			entry_.type_  = DirEntry::DIR;
			entry_.size_  = 0;
			entry_.ctime_ = 0;
			entry_.mtime_ = 0;
			return &entry_;
		}
		return nullptr;
	}

	if (handle_ == INVALID_HANDLE_VALUE) return nullptr;

	for (;;) {
		if (!has_first_) {
			if (!FindNextFileW(handle_, &find_data_))
				return nullptr;
		}
		has_first_ = false;

		if (is_dot(find_data_.cFileName)) continue;

		// Convert wide filename back to UTF-8.
		char name_utf8[MAX_PATH * 3];
		int n = WideCharToMultiByte(CP_UTF8, 0,
		            find_data_.cFileName, -1,
		            name_utf8, sizeof(name_utf8), nullptr, nullptr);
		if (n <= 0) continue;

		entry_.name_  = name_utf8;
		entry_.ctime_ = filetime_to_unix_sec(find_data_.ftCreationTime);
		entry_.mtime_ = filetime_to_unix_sec(find_data_.ftLastWriteTime);

		DWORD attr = find_data_.dwFileAttributes;
		if (attr & FILE_ATTRIBUTE_REPARSE_POINT) {
			entry_.type_ = DirEntry::SYMLINK;
			entry_.size_ = 0;
		} else if (attr & FILE_ATTRIBUTE_DIRECTORY) {
			entry_.type_ = DirEntry::DIR;
			entry_.size_ = 0;
		} else {
			entry_.type_ = DirEntry::FILE;
			entry_.size_ = ((uint64_t)find_data_.nFileSizeHigh << 32)
			             | (uint64_t)find_data_.nFileSizeLow;
		}
		return &entry_;
	}
}

// ---------------------------------------------------------------------------
// WinFs
// ---------------------------------------------------------------------------

std::unique_ptr<UniFs> WinFs::open() {
	return std::unique_ptr<UniFs>(new WinFs());
}

DirIter WinFs::readdir(const char* path) {
	WinDirIterImpl* impl = new WinDirIterImpl(path);
	if (!impl->valid()) {
		delete impl;
		return DirIter();
	}
	return DirIter(std::unique_ptr<DirIterImpl>(impl));
}

std::unique_ptr<UniFile> WinFs::openfile(const char* path, const char* mode) {
	std::string win_path = linux_path_to_win(path);
	wchar_t wpath[MAX_PATH], wmode[16];
	if (MultiByteToWideChar(CP_UTF8, 0, win_path.c_str(), -1, wpath, MAX_PATH) <= 0)
		return std::unique_ptr<UniFile>();
	if (MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, 16) <= 0)
		return std::unique_ptr<UniFile>();
	FILE* fp = _wfopen(wpath, wmode);
	if (!fp) return std::unique_ptr<UniFile>();
	return std::unique_ptr<UniFile>(new LocalFile(fp));
}

int WinFs::removefile(const char* path) {
	// Build full Windows path then convert to wide for DeleteFileW.
	std::string win_path = linux_path_to_win(path);
	const char* p = win_path.empty() ? path : win_path.c_str();
	wchar_t wpath[MAX_PATH];
	if (MultiByteToWideChar(CP_UTF8, 0, p, -1, wpath, MAX_PATH) <= 0) return -1;
	// DeleteFileW rejects directories, which is the desired behaviour.
	return DeleteFileW(wpath) ? 0 : -1;
}

#endif // _WIN32
