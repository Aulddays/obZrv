// obZrv
// https://github.com/Aulddays/obZrv
// 
// Copyright (c) 2020-2026 Aulddays (https://dev.aulddays.com/). All rights reserved.
//
// This file is part of obZrv.
// 
// obZrv is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// obZrv is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with obZrv. If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <windows.h>
#include <stdint.h>
#include <vector>
#include <string>
#include <memory>
#include <set>
#include "Codec.h"
#include "strutil.h"

class ImageView;    // forward declaration
class FileList;     // forward declaration
class UniFs;  // forward declaration (defined in unifs/unifs.hpp)

class Doc
{
	friend class ImageView;

public:
	Doc();
	~Doc();

	static int initCodec();
	static const std::set<std::string> &getSupportedTypes();
	static bool isSupportedExt(const std::string &ext);

	// Open an image file.
	// unifs: filesystem to use; if null, reuse the current _unifs.
	// forceScanDir=true: always rescan the directory after opening.
	// forceScanDir=false: rescan only on failure.
	// dirHint: INT_MIN = no fallback on failure (return error immediately);
	//   +1 = prefer file after target; -1 = prefer file before target.
	// Returns IM_OK on success.
	int open(std::shared_ptr<UniFs> unifs, const char *path, int cmdid = -1,
			 bool forceScanDir = true, int dirHint = INT_MIN);

	// Delete the currently displayed file and open the next one.
	// confirm=true: show confirmation dialog for remote files.
	// shift=true: permanent delete (no recycle bin) for local files.
	// Returns 0 on success (file deleted), -1 on failure or user cancel.
	int removeCurrentFile(bool shift);

	// Rescan the current directory.  If the current file is still accessible,
	// only the file list is updated (smooth merge, scroll preserved).
	// If the current file has disappeared, the nearest remaining file is opened.
	int refreshDir();

	// Release current image and reset state
	void close();

	// Save the currently opened file bytes to a local filesystem path.
	int saveAsLocal(const wchar_t *path);

	Image *getImage() const { return _image; }
	const std::string &getPath() const { return _path; }
	std::shared_ptr<UniFs> getUniFs() const { return _unifs; }

	// Directory navigation
	enum NavCmd { NAV_FIRST, NAV_LAST, NAV_PREV, NAV_NEXT };
	int navigate(NavCmd cmd);

	int getDirIdx()   const { return _diridx; }
	int getDirCount() const { return (int)_dirfiles.size(); }
	const std::wstring &getDirFile(int i) const { return _dirfiles[i]; }
	const std::string  &getDir()   const { return _dir; }

	// Background colour (global, stored as 0x00RRGGBB)
	static uint32_t getBgColor() { return _bgColor; }
	static void setBgColor(uint32_t c) { _bgColor = c; }

	// Bind the view that will receive animation callbacks
	void setView(ImageView *v) { _view = v; }

	// Bind the file list panel (optional; refreshed on every open)
	void setFileList(FileList *fl) { _fileList = fl; }

	// Timer callback for animated images
	static void CALLBACK onAnimate(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime);

private:
	ImageView      *_view      = NULL;
	FileList       *_fileList  = NULL;
	Image          *_image     = NULL;
	std::string     _path;
	static uint32_t _bgColor;

	// Active filesystem (local or remote)
	std::shared_ptr<UniFs>    _unifs;

	// Animation state (mirrored from ZDoc)
	bool    _animated    = false;
	int     _curframe    = 0;
	int     _curloop     = 0;
	int64_t _tmstart     = 0;   // GetTickCount() at animation start
	int64_t _totaldelay  = 0;   // cumulative delay of frames shown so far

	// Directory state
	std::string               _dir;
	std::vector<std::wstring> _dirfiles;
	int                       _diridx = -1;

	// Previously shown file (for post-delete navigation direction)
	std::wstring              _curFile;
	std::wstring              _prevFile;

	// update _dir/_dirfiles; if preservelast==true and file is already in
	// current _dir, skip the re-scan
	int updateDir(UniFs *fs, const char *filepath, bool preservelast = false);
	void clearOpenState();
};
