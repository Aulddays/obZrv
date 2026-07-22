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
#include <cctype>
#include <set>
#include <string.h>
#include "Doc.h"
#include "filetype.h"
#include "../unifs/local_fs.hpp"
#include "../unifs/remote_fs.hpp"
#include "../unifs/protocol.hpp"
#include "GdiPlusCodec.h"
#include "WebpCodec.h"
#include "ImageView.h"
#include "FileList.h"
#include "strutil.h"
#include "ZDataFile.hpp"

#undef max
#undef min

static const wchar_t DIRSEP = L'/';

static GdiPlusCodec gdiplusCodec;
static WebpCodec    webpCodec;

static Codec *codecs[] = { &webpCodec, &gdiplusCodec };
static std::set<std::string> supportedTypes;

uint32_t Doc::_bgColor = RGB(250, 250, 250);

static RemoteFs* asRemoteFs(UniFs *fs)
{
	return dynamic_cast<RemoteFs *>(fs);
}

static bool shouldReconnectRemote(UniFs *fs)
{
	RemoteFs *remote = asRemoteFs(fs);
	return remote && remote->lastStatus() != STATUS_ERR_NOT_FOUND;
}

static bool reconnectRemoteOnce(UniFs *fs)
{
	RemoteFs *remote = asRemoteFs(fs);
	return remote && remote->reconnect();
}

int Doc::initCodec()
{
	supportedTypes.clear();
	for (Codec *codec : codecs)
	{
		if (codec->init() != 0)
			return -1;
		const std::set<std::string> &types = codec->getTypes();
		supportedTypes.insert(types.begin(), types.end());
	}
	for (char c = '0'; c <= '9'; ++c)
	{
		char ext[4] = { 'z', c, '?', '\0' };
		supportedTypes.insert(ext);
	}
	return 0;
}

Doc::Doc()
{
}

Doc::~Doc()
{
	close();
}

void Doc::close()
{
	if (_animated && _view)
		KillTimer(_view->hwnd(), (UINT_PTR)this);
	_animated = false;
	delete _image;
	_image = NULL;
	_diridx = -1;
	_unifs.reset();
}

void Doc::clearOpenState()
{
	close();
	_path.clear();
	_dir.clear();
	_dirfiles.clear();
	_prevFile.clear();
	_curFile.clear();
	if (_fileList) _fileList->rebuild();
	if (_view)     _view->updateStatus();
}


int Doc::open(std::shared_ptr<UniFs> unifs, const char *path, int cmdid, bool forceScanDir, int dirHint)
{
	if (!unifs)	// reuse the same ori fs
		unifs = _unifs;
	if (!unifs)
		return IM_READFILE_ERR;
	bool scanned = false;
	bool retriedRemote = false;
	std::string prevDir = _dir;
	int prevIdx = _diridx;
	std::vector<std::wstring> oldFiles;

	// Extract filename from path.
	const char *sep = strrchr(path, '/');
	std::string targetFile = sep ? std::string(sep + 1) : std::string(path);
	std::wstring targetFileW = utf8_to_wstr(targetFile.c_str());

	// --- Step 1: obtain a readable UniFile ---
	auto uf = unifs->openfile(path, "rb");
	if (!uf && shouldReconnectRemote(unifs.get()))
	{
		retriedRemote = true;
		if (!reconnectRemoteOnce(unifs.get()))
		{
			clearOpenState();
			MessageBoxW(_view ? _view->hwnd() : NULL,
						L"Could not reconnect to the remote server.",
						L"Connection Failed", MB_OK | MB_ICONERROR);
			return IM_FAIL;
		}
		uf = unifs->openfile(path, "rb");
	}
	if (!uf)	// open failed, try fallback
	{
		if (dirHint == INT_MIN)	// fallback disabled
			return IM_READFILE_ERR;

		// Fallback: rescan and find nearest readable file.
		oldFiles = _dirfiles;
		int dirRes = updateDir(unifs.get(), path, false);
		if (dirRes != 0 && !retriedRemote && shouldReconnectRemote(unifs.get()))
		{
			retriedRemote = true;
			if (!reconnectRemoteOnce(unifs.get()))
			{
				clearOpenState();
				MessageBoxW(_view ? _view->hwnd() : NULL,
							L"Could not reconnect to the remote server.",
							L"Connection Failed", MB_OK | MB_ICONERROR);
				return IM_FAIL;
			}
			dirRes = updateDir(unifs.get(), path, false);
		}
		scanned = true;

		if (dirRes != 0)
		{
			clearOpenState();
			MessageBoxW(_view ? _view->hwnd() : NULL,
						L"Cannot open directory.",
						L"Error", MB_OK | MB_ICONERROR);
			return IM_FAIL;
		}

		if (_dirfiles.empty())
		{
			clearOpenState();
			MessageBoxW(_view ? _view->hwnd() : NULL,
						L"No image files found in this folder.",
						L"Info", MB_OK | MB_ICONINFORMATION);
			return IM_FAIL;
		}

		// Find insertion point of targetFile in the new sorted list.
		int pos = (int)_dirfiles.size();
		for (int i = 0; i < (int)_dirfiles.size(); i++)
		{
			if (StrCmpLogicalW(_dirfiles[i].c_str(), targetFileW.c_str()) >= 0)
			{
				pos = i;
				break;
			}
		}
		int target = (dirHint >= 0)
			? std::min(pos, (int)_dirfiles.size() - 1)
			: std::max(0, pos - 1);
		targetFileW = _dirfiles[target];
		targetFile  = wstr_to_utf8(targetFileW.c_str());
		_diridx = target;

		std::string newPath = _dir + "/" + targetFile;
		uf = unifs->openfile(newPath.c_str(), "rb");
		if (!uf && !retriedRemote && shouldReconnectRemote(unifs.get()))
		{
			retriedRemote = true;
			if (!reconnectRemoteOnce(unifs.get()))
			{
				clearOpenState();
				MessageBoxW(_view ? _view->hwnd() : NULL,
							L"Could not reconnect to the remote server.",
							L"Connection Failed", MB_OK | MB_ICONERROR);
				return IM_FAIL;
			}
			uf = unifs->openfile(newPath.c_str(), "rb");
		}
		if (!uf)	// still failed, give up
		{
			if (_fileList)
				_fileList->smoothRebuild(oldFiles);
			return IM_FAIL;
		}
	}

	// --- Step 2: close current file, decode & show ---
	close();  // resets _unifs
	_unifs = unifs;

	std::string fullPath = scanned ? (_dir + "/" + targetFile) : std::string(path);
	{
		std::string ext = getExt(fullPath.c_str());
		if (ZDataFile::isZDataExt(ext))
			uf.reset(new ZDataFile(std::move(uf)));

		int res = IM_NOT_SUPPORTED;
		for (Codec *codec : codecs)
		{
			if (codec->getTypes().count(ext) != 0)
			{
				uf->seek(0, SEEK_SET);
				res = codec->open(uf.get(), &_image, _bgColor);
				if (res == IM_OK)
					break;
				delete _image;
				_image = NULL;
			}
		}

		if (res != IM_OK)
		{
			std::string detectedExt = filetype(uf.get());
			if (detectedExt != "UNK" && detectedExt != ext)
			{
				for (Codec *codec : codecs)
				{
					if (codec->getTypes().count(detectedExt) != 0)
					{
						uf->seek(0, SEEK_SET);
						res = codec->open(uf.get(), &_image, _bgColor);
						if (res == IM_OK)
							break;
						delete _image;
						_image = NULL;
					}
				}
			}
		}

		if (res != IM_OK)
		{
			delete _image;
			_image = NULL;
			return res;
		}

		_path = fullPath;
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
	}

	// --- Step 3: rescan directory / update _diridx ---
	if (forceScanDir && !scanned)	// rescan
	{
		oldFiles = _dirfiles;
		int dirRes = updateDir(_unifs.get(), path, false);
		if (dirRes != 0 && !retriedRemote && shouldReconnectRemote(_unifs.get()))
		{
			retriedRemote = true;
			if (!reconnectRemoteOnce(_unifs.get()))
			{
				clearOpenState();
				MessageBoxW(_view ? _view->hwnd() : NULL,
							L"Could not reconnect to the remote server.",
							L"Connection Failed", MB_OK | MB_ICONERROR);
				return IM_FAIL;
			}
			dirRes = updateDir(_unifs.get(), path, false);
		}
		if (dirRes != 0)
		{
			clearOpenState();
			MessageBoxW(_view ? _view->hwnd() : NULL,
						L"Cannot open directory.",
						L"Error", MB_OK | MB_ICONERROR);
			return IM_FAIL;
		}
		scanned = true;
	}
	else	// just update idx
	{
		for (int i = 0; i < (int)_dirfiles.size(); i++)
		{
			if (_dirfiles[i] == targetFileW)
			{
				_diridx = i;
				break;
			}
		}
	}

	// --- Step 4: update filelist ---
	if (_fileList)
	{
		if (!scanned)
			_fileList->moveSelection(prevIdx, _diridx);
		else if (_dir == prevDir)
			_fileList->smoothRebuild(oldFiles);
		else
			_fileList->rebuild();
	}

	// Rotate _curFile -> _prevFile (clear _prevFile on directory change).
	_prevFile = (_dir == prevDir) ? _curFile : std::wstring();
	_curFile  = targetFileW;

	return IM_OK;
}
// update dirfiles; if preservelast==true and the file is already in current
// _dir, skip the expensive directory re-scan
int Doc::updateDir(UniFs *fs, const char *filepath, bool preservelast)
{
	// split filepath into directory and filename
	const char *pos = strrchr(filepath, '/');
	std::string dir      = pos ? std::string(filepath, pos) : ".";
	std::string filename = pos ? std::string(pos + 1) : std::string(filepath);

	if (_dir == dir && preservelast)
		return 0;

	std::wstring filenameW = utf8_to_wstr(filename.c_str());
	std::transform(filenameW.begin(), filenameW.end(), filenameW.begin(), towlower);

	_dir = "";
	_dirfiles.clear();
	_diridx = -1;

	if (filename.empty())
		return 0;

	// Enumerate files in directory via the provided UniFs.
	DirIter iter;
	if (fs)
		iter = fs->readdir(dir.c_str());
	if (!iter)
		return -1;

	const DirEntry* ent;
	while ((ent = iter.next()) != nullptr)
	{
		if (ent->type() != DirEntry::FILE)
			continue;
		std::string ext = getExt(ent->name());
		if (ext.empty() || !Doc::isSupportedExt(ext))
			continue;
		std::wstring wname = utf8_to_wstr(ent->name());
		_dirfiles.push_back(wname);
	}

	// natural sort (matches Explorer ordering)
	std::sort(_dirfiles.begin(), _dirfiles.end(),
		[](const std::wstring &l, const std::wstring &r) -> bool {
			return StrCmpLogicalW(l.c_str(), r.c_str()) < 0;
		});

	_dir = dir;

	// find the current file's index
	for (int i = 0; i < (int)_dirfiles.size(); ++i)
	{
		if (_wcsicmp(filenameW.c_str(), _dirfiles[i].c_str()) == 0)
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

	int dirHint = (cmd == NAV_PREV) ? -1 : +1;
	std::string path = _dir + "/" + wstr_to_utf8(_dirfiles[target].c_str());
	return open(_unifs, path.c_str(), cmd, false, dirHint);
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
	if (_path.empty())
		return -1;
	return open(_unifs, _path.c_str(), -1, true, +1);
}

int Doc::saveAsLocal(const wchar_t *path)
{
	if (_path.empty() || !_unifs || !path || !*path)
		return -1;

	auto src = _unifs->openfile(_path.c_str(), "rb");
	if (!src)
		return -1;

	std::string dstPath = to_unipath(path);
	auto dst = LocalFs::open()->openfile(dstPath.c_str(), "wb");
	if (!dst)
		return -1;

	char buf[64 * 1024];
	for (;;) {
		size_t n = src->read(buf, sizeof(buf));
		if (n == 0)
			break;
		if (dst->write(buf, n) != n)
			return -1;
	}

	return dst->close();
}

int Doc::removeCurrentFile(bool shift){
	if (_diridx < 0 || _diridx >= (int)_dirfiles.size())
		return -1;

	const std::wstring delFile = _dirfiles[_diridx];
	const int          delIdx  = _diridx;

	if (dynamic_cast<LocalFs *>(_unifs.get()) == nullptr)	// Not LocalFs => Remote
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
		std::string name_utf8 = wstr_to_utf8(delFile.c_str());
		if (_unifs->removefile((_dir + "/" + name_utf8).c_str()) != 0)
		{
			refreshDir();	// failed, try refresh
			return -1;
		}
	}
	else
	{
		// Local delete via SHFileOperationW (handles recycle bin / permanent)
		std::wstring dir_w = utf8_to_wstr(_dir.c_str());
		// Convert Unix-style separators back to backslashes for the Shell API
		for (wchar_t &c : dir_w)
			if (c == L'/')
				c = L'\\';
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

		if (SHFileOperationW(&op) != 0)
		{
			refreshDir();	// failed, try refresh
			return -1;
		}
		else if (op.fAnyOperationsAborted)
			return -1;	// canceled, just return
	}

	// File deleted successfully -- update directory listing
	_dirfiles.erase(_dirfiles.begin() + delIdx);

	if (_dirfiles.empty())
	{
		// Directory is now empty: reset to initial state
		close();
		_dir.clear();
		_prevFile.clear();
		_curFile.clear();
		if (_fileList)
			_fileList->rebuild();
		if (_view)
			_view->updateStatus();
		return 0;
	}

	// Determine which file to open next:
	// If _prevFile is set, use its sort-order relative to the deleted file
	// to pick the opposite-direction neighbour.
	int nextIdx = -1;
	if (!_prevFile.empty() &&
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

	int dirHint = (nextIdx >= delIdx) ? +1 : -1;
	std::string nextPath = _dir + "/" + wstr_to_utf8(_dirfiles[nextIdx].c_str());
	_diridx = -1;
	if (_fileList) _fileList->removeItem(delIdx, -1);
	open(_unifs, nextPath.c_str(), -1, false, dirHint);
	return 0;
}

const std::set<std::string> &Doc::getSupportedTypes()
{
	return supportedTypes;
}

bool Doc::isSupportedExt(const std::string &ext)
{
	return supportedTypes.count(ext) != 0 || ZDataFile::isZDataExt(ext);
}
