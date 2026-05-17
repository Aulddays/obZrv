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

#include "pch.h"
#include <winsock2.h>  // must precede windows.h when using asio
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdint.h>
#include <algorithm>
#include <set>
#include <string.h>
#include "Doc.h"
#include "../unifs/local_fs.hpp"
#include "GdiPlusCodec.h"
#include "WebpCodec.h"
#include "ImageView.h"
#include "FileList.h"
#include "strutil.h"
#include "../unifs/local_fs.hpp"

#undef max
#undef min

static const wchar_t DIRSEP = L'/';

static GdiPlusCodec gdiplusCodec;
static WebpCodec    webpCodec;

uint32_t Doc::_bgColor = RGB(250, 250, 250);

int Doc::initCodec()
{
	if (gdiplusCodec.init() != 0)
		return -1;
	return 0;
}

Doc::Doc()
{
}

Doc::~Doc()
{
	close();
}

void Doc::setClient(std::shared_ptr<UniFs> c,
					const std::string &host, uint16_t port)
{
	_client     = c;
	_remoteHost = host;
	_remotePort = port;
	// Reset any currently open local file/dir.
	// Do not notify _fileList here; the caller will call open() next, which will
	// detect the directory change and call rebuild() itself.
	close();
	_dir.clear();
	_dirfiles.clear();
	_diridx = -1;
}

void Doc::close()
{
	if (_animated && _view)
		KillTimer(_view->hwnd(), (UINT_PTR)this);
	_animated = false;
	delete _image;
	_image = NULL;
	_diridx = -1;
}

int Doc::openDirFile(int idx)
{
	if (idx < 0 || idx >= (int)_dirfiles.size())
		return IM_FAIL;

	// Open UniFile directly from the known directory + filename,
	// without constructing a path string.
	std::unique_ptr<UniFile> uf;
	std::string filename = wstr_to_utf8(_dirfiles[idx].c_str());
	if (_client) {
		std::string dir_utf8 = wstr_to_utf8(_dir.c_str());
		size_t slash = dir_utf8.find('/', 9); // skip "remote://"
		std::string remoteDir = (slash != std::string::npos)
								? dir_utf8.substr(slash) : std::string("/");
		uf = _client->openfile((remoteDir + "/" + filename).c_str(), "rb");
	} else {
		std::string dir_utf8 = wstr_to_utf8(_dir.c_str());
		std::unique_ptr<UniFs> localfs = LocalFs::open();
		uf = localfs->openfile((dir_utf8 + "/" + filename).c_str(), "rb");
	}
	if (!uf)
		return IM_READFILE_ERR;

	if (_animated && _view)
		KillTimer(_view->hwnd(), (UINT_PTR)this);
	_animated = false;
	// Record previous file before overwriting state
	if (_diridx >= 0 && _diridx < (int)_dirfiles.size())
	{
		_prevDir  = _dir;
		_prevFile = _dirfiles[_diridx];
	}
	close();
	_diridx = idx; // restore after close() reset it

	int res = IM_FAIL;
	const wchar_t *ext = wcsrchr(_dirfiles[idx].c_str(), L'.');
	if (ext && _wcsicmp(ext, L".webp") == 0)
		res = webpCodec.open(uf.get(), &_image, _bgColor);
	if (res != IM_OK)
		res = gdiplusCodec.open(uf.get(), &_image, _bgColor);
	if (res != IM_OK)
		return res;

	_path = _dir + L'/' + _dirfiles[idx];

	if (_view)    _view->onFileOpened(-1);
	_curframe  = 0;
	_curloop   = 0;
	_animated  = _image->isAnim();
	if (_animated) {
		_tmstart    = GetTickCount();
		_totaldelay = _image->getFrameDelay();
		if (_view)
			SetTimer(_view->hwnd(), (UINT_PTR)this,
					 (UINT)_image->getFrameDelay(), onAnimate);
	}
	if (_view)     _view->updateStatus();
	return IM_OK;
}

int Doc::open(const wchar_t *path, int cmdid)
{
	if (_animated && _view)
		KillTimer(_view->hwnd(), (UINT_PTR)this);
	_animated = false;

	// Drop remote connection when opening a local file.
	if (_client && wcsncmp(path, L"remote://", 9) != 0) {
		_client.reset();
		_remoteHost.clear();
		_remotePort = 0;
	}

	// Save state before close()/updateDir() so we can pick the right filelist update.
	std::wstring prevDir  = _dir;
	int          prevIdx  = _diridx;
	int          prevCount = (int)_dirfiles.size();

	// Record previous file before close() wipes _diridx
	if (_diridx >= 0 && _diridx < (int)_dirfiles.size())
	{
		_prevDir  = _dir;
		_prevFile = _dirfiles[_diridx];
	}
	close();

	// Open a UniFile: remote path via client, local path via open_file().
	std::unique_ptr<UniFile> uf;
	const std::wstring wpath(path);
	if (_client && wpath.compare(0, 9, L"remote://") == 0) {
		size_t slash = wpath.find(L'/', 9);
		if (slash != std::wstring::npos) {
			std::string filePath = wstr_to_utf8(wpath.c_str() + slash);
			uf = _client->openfile(filePath.c_str(), "rb");
		}
	} else {
		std::string utf8 = wstr_to_utf8(win_path_to_unix(path).c_str());
		std::unique_ptr<UniFs> localfs = LocalFs::open();
		uf = localfs->openfile(utf8.c_str(), "rb");
	}
	if (!uf)
		return IM_READFILE_ERR;

	int res = IM_FAIL;
	/* Try WebP first (extension-based), fall back to GDI+ */
	const wchar_t *ext = wcsrchr(path, L'.');
	if (ext && _wcsicmp(ext, L".webp") == 0)
		res = webpCodec.open(uf.get(), &_image, _bgColor);
	if (res != IM_OK)
		res = gdiplusCodec.open(uf.get(), &_image, _bgColor);
	if (res != IM_OK)
		return res;

	_path = path;
	updateDir(path, false);

	if (_view)
		_view->onFileOpened(cmdid);

	_curframe  = 0;
	_curloop   = 0;
	_animated  = _image->isAnim();
	if (_animated)
	{
		_tmstart    = GetTickCount();
		_totaldelay = _image->getFrameDelay();
		if (_view)
			SetTimer(_view->hwnd(), (UINT_PTR)this, (UINT)_image->getFrameDelay(), onAnimate);
	}

	if (_view)
		_view->updateStatus();

	if (_fileList)
	{
		// Rebuild only when the directory or its file count changed; otherwise
		// just move the highlight so scroll position is preserved.
		if (_dir != prevDir || (int)_dirfiles.size() != prevCount)
			_fileList->rebuild();
		else
			_fileList->moveSelection(prevIdx, _diridx);
	}

	return IM_OK;
}

// update dirfiles; if preservelast==true and the file is already in current
// _dir, skip the expensive directory re-scan
int Doc::updateDir(const wchar_t *filepath, bool preservelast)
{
	// split filepath into directory and filename
	std::wstring path;
	std::wstring filename;
	const wchar_t *pos = wcsrchr(filepath, DIRSEP);
	if (pos)
	{
		path.assign(filepath, pos);
		filename.assign(pos + 1);
	}
	else
	{
		path = L".";
		filename = filepath;
	}

	if (_dir == path && preservelast)
		return 0;

	std::transform(filename.begin(), filename.end(), filename.begin(), towlower);

	_dir = L"";
	_dirfiles.clear();
	_diridx = -1;

	if (filename.empty())
		return 0;

	// Enumerate files in directory via UniFs (accepts Unix-style path).
	// For remote paths use the client; for local paths use the local UniFs.
	std::string dir_utf8 = wstr_to_utf8(path.c_str());
	DirIter iter;
	if (_client && dir_utf8.compare(0, 9, "remote://") == 0) {
		// Extract /dir_path part from "remote://host:port/dir_path"
		size_t slash = dir_utf8.find('/', 9);
		std::string remoteDir = (slash != std::string::npos)
								? dir_utf8.substr(slash)
								: std::string("/");
		iter = _client->readdir(remoteDir.c_str());
	} else {
		std::unique_ptr<UniFs> localfs = LocalFs::open();
		iter = localfs->readdir(dir_utf8.c_str());
	}
	if (!iter)
		return -1;

	static const std::set<std::wstring> acceptext = {
		L"bmp", L"jpg", L"jpeg", L"gif", L"png", L"tiff", L"tif", L"ico", L"webp"
	};

	const DirEntry* ent;
	while ((ent = iter.next()) != nullptr)
	{
		if (ent->type() != DirEntry::FILE)
			continue;
		std::wstring wname = utf8_to_wstr(ent->name());
		const wchar_t *dot = wcsrchr(wname.c_str(), L'.');
		if (!dot)
			continue;
		std::wstring ext(dot + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
		if (acceptext.count(ext) == 0)
			continue;
		_dirfiles.push_back(wname);
	}

	// natural sort (matches Explorer ordering)
	std::sort(_dirfiles.begin(), _dirfiles.end(),
		[](const std::wstring &l, const std::wstring &r) -> bool {
			return StrCmpLogicalW(l.c_str(), r.c_str()) < 0;
		});

	_dir = path;

	// find the current file's index
	for (int i = 0; i < (int)_dirfiles.size(); ++i)
	{
		if (_wcsicmp(filename.c_str(), _dirfiles[i].c_str()) == 0)
		{
			_diridx = i;
			break;
		}
	}

	return 0;
}

int Doc::navigate(NavCmd cmd)
{
	if (_diridx == -1 || _dirfiles.empty())
		return -1;

	int target = -1;
	if (cmd == NAV_PREV && _diridx > 0)
		target = _diridx - 1;
	else if (cmd == NAV_NEXT && _diridx < (int)_dirfiles.size() - 1)
		target = _diridx + 1;
	else if (cmd == NAV_FIRST && _diridx > 0)
		target = 0;
	else if (cmd == NAV_LAST && _diridx < (int)_dirfiles.size() - 1)
		target = (int)_dirfiles.size() - 1;

	if (target == -1)
		return -1;

	std::wstring failedFile = _dirfiles[target];
	std::wstring newpath = _dir + DIRSEP + failedFile;
	if (open(newpath.c_str(), cmd) == IM_OK)
		return IM_OK;

	int dirHint = (cmd == NAV_PREV) ? -1 : +1;
	return reopenAfterFail(failedFile, dirHint);
}

int Doc::reopenAfterFail(const std::wstring& refFile, int dirHint)
{
	// Save list before rescan; open() returns early on failure so _dirfiles
	// still holds the pre-failure state at this point.
	std::vector<std::wstring> oldFiles = _dirfiles;

	// Rescan the directory (refFile may no longer exist).
	std::wstring scanPath = _dir + DIRSEP + refFile;
	updateDir(scanPath.c_str(), false);

	if (_dirfiles.empty()) {
		close();
		_path.clear();
		_dir.clear();
		_prevDir.clear();
		_prevFile.clear();
		if (_fileList) _fileList->rebuild();
		if (_view)     _view->updateStatus();
		MessageBoxW(_view ? _view->hwnd() : NULL,
					L"No image files found in this folder.",
					L"Info", MB_OK | MB_ICONINFORMATION);
		return -1;
	}

	// Find insertion point of refFile in the new sorted list.
	int pos = (int)_dirfiles.size();
	for (int i = 0; i < (int)_dirfiles.size(); i++) {
		if (StrCmpLogicalW(_dirfiles[i].c_str(), refFile.c_str()) >= 0) {
			pos = i;
			break;
		}
	}

	// dirHint > 0: want a file at/after refFile; < 0: want a file before refFile.
	int target;
	if (dirHint >= 0)
		target = std::min(pos, (int)_dirfiles.size() - 1);
	else
		target = std::max(0, pos - 1);

	int res = openDirFile(target);
	// Smooth merge: preserves scroll position and minimises flicker when
	// only a few files changed (e.g. a single inaccessible file was skipped).
	if (_fileList) _fileList->smoothRebuild(oldFiles);
	return res;
}

void CALLBACK Doc::onAnimate(HWND /*hWnd*/, UINT /*nMsg*/, UINT_PTR nIDEvent, DWORD /*dwTime*/)
{
	Doc *pthis = (Doc *)nIDEvent;
	if (!pthis->_view)
		return;
	KillTimer(pthis->_view->hwnd(), nIDEvent);
	if (!pthis->_image)
		return;

	// loop-end check
	if (pthis->_image->getLoopNum() > 0 && pthis->_curloop >= pthis->_image->getLoopNum())
	{
		pthis->_curframe = pthis->_image->getFrameCount() - 1;
		pthis->_view->onFrameUpdate();
		return;
	}

	// adjust _tmstart for GetTickCount() wrap
	int64_t tmcur = GetTickCount();
	while (pthis->_tmstart + pthis->_totaldelay - pthis->_image->getFrameDelay() > tmcur)
		pthis->_tmstart -= (int64_t)0x100000000ll;

	long allowdiff = std::max(pthis->_image->getFrameDelay() * 10, 1000l);
	bool reset = false;
	if (pthis->_tmstart + pthis->_totaldelay > tmcur + allowdiff ||
		pthis->_tmstart + pthis->_totaldelay < tmcur - allowdiff)
	{
		// shifted too much -- restart
		reset = true;
		pthis->_curframe = pthis->_image->getFrameCount() - 1;
		pthis->_tmstart  = tmcur;
	}
	else if (pthis->_tmstart + pthis->_totaldelay > tmcur + 20)
	{
		// current frame not due yet -- wait a bit more
		SetTimer(pthis->_view->hwnd(), nIDEvent,
			(UINT)(pthis->_tmstart + pthis->_totaldelay - tmcur), onAnimate);
		return;
	}

	do
	{
		int res = pthis->_image->nextFrame(reset);
		if (reset)
			pthis->_curframe = 0;
		else
			pthis->_curframe++;
		if (res == IM_NO_MORE_FRAMES)
		{
			pthis->_curloop++;
			if (pthis->_image->getLoopNum() > 0 && pthis->_curloop >= pthis->_image->getLoopNum())
			{
				pthis->_curframe = pthis->_image->getFrameCount() - 1;
				break;
			}
			if (pthis->_curframe <= 1)  // only one effective frame
			{
				pthis->_curframe = 0;
				pthis->_curloop  = -1;
				break;
			}
			res = pthis->_image->nextFrame(true);
			pthis->_curframe = 0;
		}
		if (pthis->_image->getFrameDelay() == 0)  // 0-delay frame
		{
			if (pthis->_curloop > 0 && pthis->_totaldelay == 0)
			{
				// all frames 0-delay -- stick on frame 0
				res = pthis->_image->nextFrame(true);
				pthis->_curframe = 0;
				pthis->_curloop  = -1;
				break;
			}
			else
				continue;   // skip 0-delay frames
		}
		pthis->_totaldelay += pthis->_image->getFrameDelay();
	} while (pthis->_tmstart + pthis->_totaldelay <= tmcur + 10);  // catch up if behind

	pthis->_view->onFrameUpdate();

	// set timer for next frame if animation is still running
	bool stillRunning = (pthis->_image->getLoopNum() <= 0 && pthis->_curloop >= 0) ||
						(pthis->_curloop < pthis->_image->getLoopNum());
	if (stillRunning)
	{
		int64_t tmcurnew = GetTickCount();
		if (tmcurnew < tmcur)
			pthis->_tmstart -= (int64_t)0x100000000ll;
		UINT delay = (pthis->_tmstart + pthis->_totaldelay > tmcur)
			? (UINT)(pthis->_tmstart + pthis->_totaldelay - tmcur)
			: 1;
		delay = std::min(delay, (UINT)pthis->_image->getFrameDelay() * 5);
		SetTimer(pthis->_view->hwnd(), nIDEvent, delay, onAnimate);
	}
}

int Doc::refreshDir()
{
	if (_dir.empty() || _dirfiles.empty())
		return -1;

	// Save directory state before the rescan.
	std::vector<std::wstring> oldFiles = _dirfiles;
	std::wstring curFile = (_diridx >= 0) ? _dirfiles[_diridx] : L"";

	// Rescan directory.
	std::wstring scanPath = _dir + DIRSEP + (!curFile.empty() ? curFile : oldFiles.front());
	updateDir(scanPath.c_str(), false);

	if (_dirfiles.empty())
	{
		// Every file in the directory is gone.
		close();
		_path.clear();
		_dir.clear();
		_prevDir.clear();
		_prevFile.clear();
		if (_fileList) _fileList->rebuild();
		if (_view)     _view->updateStatus();
		return 0;
	}

	if (_diridx >= 0)
	{
		// Current file still exists: image unchanged, just sync the list.
		if (_fileList) _fileList->smoothRebuild(oldFiles);
		return 0;
	}

	// Current file is gone: find the nearest remaining file and open it.
	int pos = (int)_dirfiles.size();
	for (int i = 0; i < (int)_dirfiles.size(); i++)
	{
		if (StrCmpLogicalW(_dirfiles[i].c_str(), curFile.c_str()) >= 0)
		{
			pos = i;
			break;
		}
	}
	int target = std::min(pos, (int)_dirfiles.size() - 1);

	_diridx = -1;
	if (openDirFile(target) != IM_OK)
	{
		reopenAfterFail(_dirfiles[target], +1);
		return 0;
	}

	// Smooth merge: the missing file is dropped, others are preserved in place.
	if (_fileList) _fileList->smoothRebuild(oldFiles);
	return 0;
}

int Doc::removeCurrentFile(bool shift){
	if (_diridx < 0 || _diridx >= (int)_dirfiles.size())
		return -1;

	const std::wstring delFile = _dirfiles[_diridx];
	const int          delIdx  = _diridx;

	if (_client)
	{
		// Remote delete
		bool CONFIRM_DELETE = false; // TODO: user option
		if (!shift && CONFIRM_DELETE)
		{
			// Del: confirm before deleting
			std::wstring msg = L"Delete \"" + delFile + L"\" from remote server?";
			if (MessageBoxW(_view ? _view->hwnd() : NULL,
							msg.c_str(), L"Confirm Delete",
							MB_YESNO | MB_ICONWARNING) != IDYES)
				return -1;
		}

		// Delete the remote file using removefile()
		std::string dir_utf8 = wstr_to_utf8(_dir.c_str());
		size_t slash = dir_utf8.find('/', 9); // skip "remote://"
		std::string remoteDir = (slash != std::string::npos)
								? dir_utf8.substr(slash) : std::string("/");
		std::string name_utf8 = wstr_to_utf8(delFile.c_str());
		if (_client->removefile((remoteDir + "/" + name_utf8).c_str()) != 0)
			return -1;
	}
	else
	{
		// Local delete via SHFileOperationW (handles recycle bin / permanent)
		std::wstring dir_w = _dir;
		// Convert Unix-style separators back to backslashes for the Shell API
		for (wchar_t &c : dir_w) if (c == L'/') c = L'\\';
		std::wstring fullPath = dir_w + L'\\' + delFile;

		// SHFileOperationW requires double-null-terminated string
		std::vector<wchar_t> buf(fullPath.size() + 2, L'\0');
		wmemcpy(buf.data(), fullPath.c_str(), fullPath.size());

		SHFILEOPSTRUCTW op = {};
		op.hwnd   = _view ? _view->hwnd() : NULL;
		op.wFunc  = FO_DELETE;
		op.pFrom  = buf.data();
		op.fFlags = FOF_NOERRORUI;
		if (shift)
			op.fFlags |= 0;          // no FOF_ALLOWUNDO -> permanent delete with Shell confirm
		else
			op.fFlags |= FOF_ALLOWUNDO; // move to recycle bin

		if (SHFileOperationW(&op) != 0 || op.fAnyOperationsAborted)
			return -1;
	}

	// File deleted successfully -- update directory listing
	_dirfiles.erase(_dirfiles.begin() + delIdx);

	if (_dirfiles.empty())
	{
		// Directory is now empty: reset to initial state
		close();
		_dir.clear();
		_prevDir.clear();
		_prevFile.clear();
		if (_fileList) _fileList->rebuild();
		if (_view)     _view->updateStatus();
		return 0;
	}

	// Determine which file to open next:
	// If _prevFile was in the same directory, use its sort-order relative to
	// the deleted file to pick the opposite-direction neighbour.
	int nextIdx = -1;
	if (!_prevDir.empty() && _prevDir == _dir && !_prevFile.empty() &&
		_wcsicmp(_prevFile.c_str(), delFile.c_str()) != 0)
	{
		int cmp = StrCmpLogicalW(_prevFile.c_str(), delFile.c_str());
		if (cmp > 0)
		{
			// prev was after deleted -> open the one before deleted
			if (delIdx - 1 >= 0)
				nextIdx = delIdx - 1;
		}
		else
		{
			// prev was before deleted -> open the one after deleted
			// (after erase, that file is now at delIdx)
			if (delIdx < (int)_dirfiles.size())
				nextIdx = delIdx;
		}
	}

	// Fallback: open the file that slid into the deleted slot
	if (nextIdx < 0)
		nextIdx = (delIdx < (int)_dirfiles.size()) ? delIdx : (int)_dirfiles.size() - 1;

	// Reset _diridx so openDirFile does not record the wrong prev
	std::wstring failedFile = _dirfiles[nextIdx];
	int dirHint = (nextIdx >= delIdx) ? +1 : -1;
	_diridx = -1;
	if (openDirFile(nextIdx) != IM_OK)
		reopenAfterFail(failedFile, dirHint);
	else if (_fileList)
		_fileList->removeItem(delIdx, _diridx);
	return 0;
}
