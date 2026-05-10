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
#include "Codec.h"

class ImageView;    // forward declaration
class FileList;     // forward declaration
class UniFsClient;  // forward declaration (defined in unifs/client.hpp)

class Doc
{
	friend class ImageView;

public:
	Doc();
	~Doc();

	static int initCodec();

	// Open an image file; returns IM_OK (0) on success
	int open(const wchar_t *path, int cmdid = -1);

	// Delete the currently displayed file and open the next one.
	// confirm=true: show confirmation dialog for remote files.
	// shift=true: permanent delete (no recycle bin) for local files.
	// Returns 0 on success (file deleted), -1 on failure or user cancel.
	int removeCurrentFile(bool shift);

	// Release current image and reset state
	void close();

	Image *getImage() const { return _image; }
	const std::wstring &getPath() const { return _path; }

	// Open a file by index in the current directory listing
	int openDirFile(int idx);

	// Directory navigation
	enum NavCmd { NAV_FIRST, NAV_LAST, NAV_PREV, NAV_NEXT };
	int navigate(NavCmd cmd);

	int getDirIdx()   const { return _diridx; }
	int getDirCount() const { return (int)_dirfiles.size(); }
	const std::wstring &getDirFile(int i) const { return _dirfiles[i]; }
	const std::wstring &getDir()   const { return _dir; }

	// Background colour (global, stored as 0x00RRGGBB)
	static uint32_t getBgColor() { return _bgColor; }
	static void setBgColor(uint32_t c) { _bgColor = c; }

	// Bind the view that will receive animation callbacks
	void setView(ImageView *v) { _view = v; }

	// Bind the file list panel (optional; refreshed on every open)
	void setFileList(FileList *fl) { _fileList = fl; }

	// Remote connection management
	void setClient(std::shared_ptr<UniFsClient> c,
				   const std::string &host, uint16_t port);
	bool isRemote() const { return _client != nullptr; }
	UniFsClient *client() const { return _client.get(); }
	const std::string &remoteHost() const { return _remoteHost; }
	uint16_t remotePort() const { return _remotePort; }

	// Timer callback for animated images
	static void CALLBACK onAnimate(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime);

private:
	ImageView      *_view      = NULL;
	FileList       *_fileList  = NULL;
	Image          *_image     = NULL;
	std::wstring    _path;
	static uint32_t _bgColor;

	// Remote connection (nullptr = local mode)
	std::shared_ptr<UniFsClient> _client;
	std::string                  _remoteHost;
	uint16_t                     _remotePort = 0;

	// Animation state (mirrored from ZDoc)
	bool    _animated    = false;
	int     _curframe    = 0;
	int     _curloop     = 0;
	int64_t _tmstart     = 0;   // GetTickCount() at animation start
	int64_t _totaldelay  = 0;   // cumulative delay of frames shown so far

	// Directory state
	std::wstring              _dir;
	std::vector<std::wstring> _dirfiles;
	int                       _diridx = -1;

	// Previously shown file (for post-delete navigation direction)
	std::wstring              _prevDir;
	std::wstring              _prevFile;

	// update _dir/_dirfiles; if preservelast==true and file is already in
	// current _dir, skip the re-scan
	int updateDir(const wchar_t *filepath, bool preservelast = false);
};
