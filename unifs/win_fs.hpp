#pragma once
#ifdef _WIN32

#include "unifs.hpp"
#include <windows.h>

// Internal per-readdir iteration state for Windows; created by WinFs::readdir().
class WinDirIterImpl : public DirIterImpl {
public:
	explicit WinDirIterImpl(const char* path);
	~WinDirIterImpl();

	bool            valid() const { return valid_; }
	const DirEntry* next()  override;

private:
	static int to_search_path(const char* path, wchar_t* buf, int buf_size);

	std::string      base_utf8_;
	HANDLE           handle_;
	WIN32_FIND_DATAW find_data_;
	bool             has_first_;
	bool             valid_;
	wchar_t          search_path_[MAX_PATH + 4];
	DirEntry         entry_;

	// Root-mode: path was "/" -- enumerate logical drives instead.
	bool             root_mode_;
	DWORD            drives_mask_;
	int              drive_idx_;
};

class WinFs : public UniFs {
public:
	static std::unique_ptr<UniFs> open();

	DirIter                  readdir(const char* path)             override;
	std::unique_ptr<UniFile> openfile(const char* path,
	                                  const char* mode)            override;
	int                      removefile(const char* path)          override;

private:
	WinFs() {}
};

#endif // _WIN32
