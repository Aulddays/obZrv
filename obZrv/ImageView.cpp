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
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <algorithm>
#include <vector>
#include <functional>
#include <cmath>
#include "ImageView.h"
#include "Doc.h"
#include "dpi.h"
#include "resource.h"

#undef max
#undef min

// ---- helpers ---------------------------------------------------------------

static inline int rectW(const RECT &r) { return r.right  - r.left; }
static inline int rectH(const RECT &r) { return r.bottom - r.top;  }

static inline bool sizeEq(const SIZE &a, const SIZE &b)
	{ return a.cx == b.cx && a.cy == b.cy; }
static inline bool rectEq(const RECT &a, const RECT &b)
	{ return a.left == b.left && a.top == b.top &&
			 a.right == b.right && a.bottom == b.bottom; }

// Fill a rectangle with a solid colour using an existing brush
static inline void fillRect(HDC hdc, int x, int y, int w, int h, HBRUSH br)
{
	RECT r = {x, y, x + w, y + h};
	FillRect(hdc, &r, br);
}

// ---- ImageView -------------------------------------------------------------

bool ImageView::Create(HWND hParent, HINSTANCE hInst, HWND hMainWnd)
{
	m_hInst    = hInst;
	m_hMainWnd = hMainWnd;

	WNDCLASS wc     = {};
	wc.lpfnWndProc  = WndBase::WndProcStatic;
	wc.hInstance    = hInst;
	wc.hbrBackground = NULL;   // WM_ERASEBKGND handled manually
	wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = IMAGEVIEW_CLASS;
	RegisterClass(&wc);

	if (!CreateWindowEx(
			0, IMAGEVIEW_CLASS, NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP,
			0, 0, 0, 0,
			hParent, NULL, hInst, this))
		return false;

	return true;
}

void ImageView::Layout(int x, int y, int w, int h)
{
	if (!m_hwnd) return;
	SetWindowPos(m_hwnd, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
}

void ImageView::setDoc(Doc *doc)
{
	_doc = doc;
}

void ImageView::releaseBitmap()
{
	if (!_internalBitmap)
		delete _viewBitmap;
	_viewBitmap      = NULL;
	_internalBitmap  = false;
}

// ---- Callbacks from Doc ----------------------------------------------------

void ImageView::onFileOpened(int cmdid)
{
	Image *image = _doc ? _doc->getImage() : NULL;
	if (!image)
		return;

	// cmdid may be a NavCmd enum value; map to the actual toolbar command ID
	int tbid = -1;
	if      (cmdid == Doc::NAV_PREV)  tbid = ID_FILE_PREV;
	else if (cmdid == Doc::NAV_NEXT)  tbid = ID_FILE_NEXT;
	else if (cmdid == ID_FILE_PREV || cmdid == ID_FILE_NEXT)
		tbid = cmdid;  // already a command ID (future-proof)

	POINT mousepos = preserveMouse(tbid);

	_zoomlevel = 0;
	_fitlevel  = 0;
	_scaleSize.cx = _scaleSize.cy = -1;
	_viewCrop   = RECT{-1, -1, -1, -1};
	_viewOffset.cx = _viewOffset.cy = 0;

	// W2I modes resize the window to fit the image; I2W/NOFIT keep the window fixed
	if (_zoomtype == ZT_W2I_ZOOMOUT || _zoomtype == ZT_W2I)
		fitWindow2Image(image, mousepos);
	else
	{
		releaseBitmap();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}

void ImageView::onFrameUpdate()
{
	releaseBitmap();
	Image *image = _doc ? _doc->getImage() : NULL;
	if (image)
		_viewBitmap = image->getBBitmap(_scaleSize, _viewCrop);
	InvalidateRect(m_hwnd, NULL, FALSE);
	updateStatus();
}

void ImageView::updateStatus()
{
	if (!_doc || !_doc->getImage())
		return;

	Image *image = _doc->getImage();
	enum { INFO_LEN = 512 };
	static wchar_t infobuf[INFO_LEN];

	const wchar_t *color = image->getColorInfo();
	int zoom = _zoomlevel != 0 ? _zoomlevel : _fitlevel;

	if (image->isAnim()) {
		_snwprintf(infobuf, INFO_LEN, L"%d/%d | %dx%d | %s | %s | frame %d/%d | %d%% | %dms",
			_doc->getDirIdx() + 1, _doc->getDirCount(),
			image->getDimension().cx, image->getDimension().cy,
			color, image->getFormat(),
			image->getCurFrame() + 1, image->getFrameCount(),
			zoom, _timecost);
	} else {
		_snwprintf(infobuf, INFO_LEN, L"%d/%d | %dx%d | %s | %s | %d%% | %dms",
			_doc->getDirIdx() + 1, _doc->getDirCount(),
			image->getDimension().cx, image->getDimension().cy,
			color, image->getFormat(),
			zoom, _timecost);
	}
	SendMessageW(m_hMainWnd, WM_APP_SETINFO, 0, (LPARAM)infobuf);
}

// ---- fitWindow2Image (from ZView::fitWindow2Image) -------------------------

void ImageView::fitWindow2Image(Image *image, POINT mousepos)
{
	WINDOWPLACEMENT winpos;
	winpos.length = sizeof(winpos);
	if (!m_hMainWnd || !GetWindowPlacement(m_hMainWnd, &winpos))
	{
		releaseBitmap();
		InvalidateRect(m_hwnd, NULL, FALSE);
		return;
	}

	RECT crect;
	GetClientRect(m_hwnd, &crect);
	int viewW = rectW(crect);
	int viewH = rectH(crect);

	bool mok = false;
	MONITORINFO minfo;
	minfo.cbSize = sizeof(minfo);
	if (winpos.showCmd == SW_SHOWNORMAL)
	{
		HMONITOR hmon = MonitorFromWindow(m_hMainWnd, MONITOR_DEFAULTTONULL);
		if (hmon && GetMonitorInfo(hmon, &minfo))
			mok = true;
	}

	if (mok)
	{
		int imw = image->getDimension().cx;
		int imh = image->getDimension().cy;

		int fw = winpos.rcNormalPosition.right  - winpos.rcNormalPosition.left;
		int fh = winpos.rcNormalPosition.bottom - winpos.rcNormalPosition.top;
		int sw = minfo.rcWork.right  - minfo.rcWork.left;
		int sh = minfo.rcWork.bottom - minfo.rcWork.top;
		int mw = sw - (fw - viewW);
		int mh = sh - (fh - viewH);

		if (_zoomlevel != 0 || _zoomtype == ZT_W2I)
		{
			// Fixed zoom or W2I (no auto-zoom): resize window to requested size,
			// clamp each axis independently to the available monitor area
			if (_zoomlevel != 0)
			{
				imw = imw * _zoomlevel / 100;
				imh = imh * _zoomlevel / 100;
			}
			imw = std::min(imw, mw);
			imh = std::min(imh, mh);
		}
		else
		{
			// W2I_ZOOMOUT (_zoomlevel == 0): proportionally scale the image
			// down to fit the monitor, but never upscale
			if (imw > mw && mw * imh <= mh * imw)
			{
				imh = std::min(mw * imh / imw + 1, mh);
				imw = mw;
			}
			else if (imh > mh && mw * imh >= mh * imw)
			{
				imw = std::min(mh * imw / imh + 1, mw);
				imh = mh;
			}
		}

		// guarantee a minimum width (DPI-scaled 400px); allow a small extra margin
		double dpiScale = (double)Dpi::Scale(1000) / 1000.0;
		imw = std::min(std::max(imw, (int)(400 * dpiScale)), mw);
		imh = std::min(std::max(imh, 20), mh);

		int dw = imw - viewW;
		int dh = imh - viewH;
		RECT npos = winpos.rcNormalPosition;

		if (dw != 0)
		{
			npos.left  -= dw / 2;
			npos.right += dw - dw / 2;
			if (npos.right > minfo.rcWork.right)
			{
				npos.left  = minfo.rcWork.right - (npos.right - npos.left);
				npos.right = minfo.rcWork.right;
			}
			if (npos.left < minfo.rcWork.left)
			{
				npos.right = minfo.rcWork.left + (npos.right - npos.left);
				npos.left  = minfo.rcWork.left;
			}
		}
		if (dh != 0)
		{
			npos.top    -= dh / 2;
			npos.bottom += dh - dh / 2;
			if (npos.bottom > minfo.rcWork.bottom)
			{
				npos.top    = minfo.rcWork.bottom - (npos.bottom - npos.top);
				npos.bottom = minfo.rcWork.bottom;
			}
			if (npos.top < minfo.rcWork.top)
			{
				npos.bottom = minfo.rcWork.top + (npos.bottom - npos.top);
				npos.top    = minfo.rcWork.top;
			}
		}

		if (dw != 0 || dh != 0)
		{
			winpos.rcNormalPosition = npos;
			SetWindowPlacement(m_hMainWnd, &winpos);
			if (mousepos.x >= 0)
			{
				mousepos.x += npos.left;
				mousepos.y += npos.top;
				SetCursorPos(mousepos.x, mousepos.y);
			}
		}
	}

	releaseBitmap();
	InvalidateRect(m_hwnd, NULL, FALSE);
}

// ---- Zoom (from ZView::zoom) -----------------------------------------------

int ImageView::zoom(int inout, bool test)
{
	static const std::vector<int> levels = {
		1, 2, 3, 5, 7, 10, 15, 20, 30, 50, 70,
		100, 150, 200, 300, 500, 700, 1000, 2000, 3000, 5000, 7000, 10000
	};

	Image *image = _doc ? _doc->getImage() : NULL;
	if (!image)
		return -1;
	if (inout == 0)
		return 0;

	int orilevel = _zoomlevel != 0 ? _zoomlevel : 100;

	if (inout > 0)
	{
		int current = _zoomlevel != 0 ? _zoomlevel : _fitlevel;
		if (current >= levels.back())
			return -1;
		if (test)
			return 0;
		_zoomlevel = *std::upper_bound(levels.begin(), levels.end(), current);
	}
	else
	{
		int current = _zoomlevel != 0 ? _zoomlevel : _fitlevel;
		if (current <= levels.front())
			return -1;
		auto it = std::lower_bound(levels.begin(), levels.end(), current);
		int newlevel = *std::prev(it);  // last level strictly below current
		if (std::max(image->getDimension().cx, image->getDimension().cy) * newlevel / 100 < 1)
			return -1;
		if (test)
			return 0;
		_zoomlevel = newlevel;
	}

	_viewOffset.cx = _viewOffset.cx * _zoomlevel / orilevel;
	_viewOffset.cy = _viewOffset.cy * _zoomlevel / orilevel;

	// W2I modes resize the window on zoom; I2W/NOFIT keep the window fixed
	if (_zoomtype == ZT_W2I_ZOOMOUT || _zoomtype == ZT_W2I)
	{
		POINT mousepos = preserveMouse(inout > 0 ? ID_VIEW_ZOOMIN : ID_VIEW_ZOOMOUT);
		fitWindow2Image(image, mousepos);
	}
	else
	{
		_viewWndDim.cx = _viewWndDim.cy = -1;  // force full recalc on next paint
		releaseBitmap();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
	updateStatus();
	return 0;
}

POINT ImageView::preserveMouse(int id)
{
	if (id < 0 || !m_hToolbar || !m_hMainWnd)
		return POINT{-1, -1};

	// Get current cursor position in screen coords
	POINT curpos;
	GetCursorPos(&curpos);

	// Check whether cursor is inside the toolbar client area
	POINT tbpos = curpos;
	ScreenToClient(m_hToolbar, &tbpos);
	RECT tbrc;
	GetClientRect(m_hToolbar, &tbrc);
	if (!PtInRect(&tbrc, tbpos))
		return POINT{-1, -1};

	// Hit-test each visible toolbar button
	int count = (int)SendMessage(m_hToolbar, TB_BUTTONCOUNT, 0, 0);
	for (int i = 0; i < count; ++i)
	{
		TBBUTTON tbb = {};
		if (!SendMessage(m_hToolbar, TB_GETBUTTON, i, (LPARAM)&tbb))
			continue;
		if (tbb.fsStyle & BTNS_SEP)
			continue;
		if (tbb.fsState & TBSTATE_HIDDEN)
			continue;
		RECT btnrc;
		if (!SendMessage(m_hToolbar, TB_GETITEMRECT, i, (LPARAM)&btnrc))
			continue;
		if (!PtInRect(&btnrc, tbpos))
			continue;
		// cursor is on this button
		if ((int)tbb.idCommand != id)
			return POINT{-1, -1};
		// correct button -- return position relative to the main frame
		RECT framerc;
		GetWindowRect(m_hMainWnd, &framerc);
		return POINT{curpos.x - framerc.left, curpos.y - framerc.top};
	}

	return POINT{-1, -1};
}

// ---- setZoomMode -----------------------------------------------------------

void ImageView::setZoomMode(int cmdId)
{
	static const ZoomType map[] = {
		ZT_W2I_ZOOMOUT,  // ID_ZOOMMODE_W2I_ZOOMOUT
		ZT_W2I,          // ID_ZOOMMODE_W2I
		ZT_I2W_ZOOMOUT,  // ID_ZOOMMODE_I2W_ZOOMOUT
		ZT_I2W,          // ID_ZOOMMODE_I2W
		ZT_NOFIT,        // ID_ZOOMMODE_NOFIT
	};
	int idx = cmdId - ID_ZOOMMODE_W2I_ZOOMOUT;
	if (idx < 0 || idx >= (int)(sizeof(map) / sizeof(map[0])))
		return;

	_zoomtype  = map[idx];
	_zoomlevel = 0;
	_fitlevel  = 0;
	_viewOffset.cx = _viewOffset.cy = 0;
	_scaleSize  = {-1, -1};
	_viewCrop   = {-1, -1, -1, -1};
	_viewWndDim = {-1, -1};  // force full recalc on next paint

	Image *image = _doc ? _doc->getImage() : NULL;
	if (!image)
		return;

	if (_zoomtype == ZT_W2I_ZOOMOUT || _zoomtype == ZT_W2I)
		fitWindow2Image(image, POINT{-1, -1});
	else
	{
		releaseBitmap();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}

void ImageView::fillBg(HDC hdc)
{
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	uint32_t raw = Doc::getBgColor();
	HBRUSH br = CreateSolidBrush(RGB((raw >> 16) & 0xff, (raw >> 8) & 0xff, raw & 0xff));
	FillRect(hdc, &rc, br);
	DeleteObject(br);
}

void ImageView::OnPaint(HDC hdc)
{
	Image *image = _doc ? _doc->getImage() : NULL;
	if (!image)
	{
		fillBg(hdc);
		return;
	}

	RECT rc;
	GetClientRect(m_hwnd, &rc);
	RECT crect = rc;
	SIZE size  = { rectW(rc), rectH(rc) };

	if (!_viewBitmap || !sizeEq(size, _viewWndDim))
	{
		RECT oldViewCrop = _viewCrop;
		SIZE imgdim = image->getDimension();

		if (_zoomlevel == 0)
		{
			// Auto mode: scaling strategy depends on zoom type
			switch (_zoomtype)
			{
			case ZT_W2I:
			case ZT_NOFIT:
				// Always display at 100%; image may be larger than the view (pan available)
				_fitlevel  = 100;
				_scaleSize = SIZE{imgdim.cx, imgdim.cy};
				break;

			case ZT_I2W:
				// Scale to fit completely within view, zoom in or out as needed.
				// Use the same aspect-ratio comparison as ZT_I2W_ZOOMOUT so that
				// the image is never cropped: the shorter scaled side stays <= view.
				if (imgdim.cx == 0 || imgdim.cy == 0)
				{
					_fitlevel  = 100;
					_scaleSize = SIZE{1, 1};
				}
				else if ((UINT64)size.cx * imgdim.cy > (UINT64)size.cy * imgdim.cx)
				{
					// view is wider than image -> fit on height (width will be < viewW)
					_fitlevel     = size.cy * 100 / imgdim.cy;
					_scaleSize.cx = std::max((LONG)((double)size.cy * imgdim.cx / imgdim.cy + 0.5), 1L);
					_scaleSize.cy = size.cy;
				}
				else
				{
					// image is wider than view -> fit on width (height will be < viewH)
					_fitlevel     = size.cx * 100 / imgdim.cx;
					_scaleSize.cy = std::max((LONG)((double)size.cx * imgdim.cy / imgdim.cx + 0.5), 1L);
					_scaleSize.cx = size.cx;
				}
				// Scaled image always fits inside the view; no panning needed
				_viewCrop   = RECT{0, 0, _scaleSize.cx, _scaleSize.cy};
				_viewOffset = {0, 0};
				break;

			default: // ZT_W2I_ZOOMOUT, ZT_I2W_ZOOMOUT
				// Scale down if the image is larger than the view; never upscale
				if (size.cx >= imgdim.cx && size.cy >= imgdim.cy)
				{
					_fitlevel  = 100;
					_scaleSize = SIZE{imgdim.cx, imgdim.cy};
				}
				else if ((UINT64)size.cx * imgdim.cy > (UINT64)size.cy * imgdim.cx)
				{
					// fit on height
					_fitlevel     = size.cy * 100 / imgdim.cy;
					_scaleSize.cx = std::max((LONG)((double)size.cy * imgdim.cx / imgdim.cy + 0.5), 1L);
					_scaleSize.cy = size.cy;
				}
				else
				{
					// fit on width
					_fitlevel     = size.cx * 100 / imgdim.cx;
					_scaleSize.cy = std::max((LONG)((double)size.cx * imgdim.cy / imgdim.cx + 0.5), 1L);
					_scaleSize.cx = size.cx;
				}
				// Image always fits inside the view; no panning needed
				_viewCrop   = RECT{0, 0, _scaleSize.cx, _scaleSize.cy};
				_viewOffset = {0, 0};
				break;
			}
		}

		// Fixed zoom OR auto-100% modes (W2I / NOFIT): compute viewCrop with pan support
		if (_zoomlevel != 0 || _zoomtype == ZT_W2I || _zoomtype == ZT_NOFIT)
		{
			if (_zoomlevel != 0)
			{
				_scaleSize.cx = std::max((LONG)(imgdim.cx) * _zoomlevel / 100, 1L);
				_scaleSize.cy = std::max((LONG)(imgdim.cy) * _zoomlevel / 100, 1L);
			}

			if (rc.right >= _scaleSize.cx)
			{ _viewCrop.left = 0; _viewCrop.right = _scaleSize.cx; }
			else
			{ _viewCrop.left = (_scaleSize.cx - rc.right) / 2;
			  _viewCrop.right = _viewCrop.left + rc.right; }

			if (rc.bottom >= _scaleSize.cy)
			{ _viewCrop.top = 0; _viewCrop.bottom = _scaleSize.cy; }
			else
			{ _viewCrop.top = (_scaleSize.cy - rc.bottom) / 2;
			  _viewCrop.bottom = _viewCrop.top + rc.bottom; }

			// apply pan offset and dragging
			if (_viewOffset.cx != 0 || _viewOffset.cy != 0 ||
				(_isdragging && (_draggingOrigin.x != _draggingCur.x ||
								 _draggingOrigin.y != _draggingCur.y)))
			{
				SIZE offset = {0, 0};
				if (rectW(_viewCrop) < _scaleSize.cx)
				{
					offset.cx = _viewOffset.cx;
					// Only apply dragging offset for pan dragging, not for zoom dragging
					if (_isdragging && _dragType == DT_PAN)
						offset.cx -= _draggingCur.x - _draggingOrigin.x;
					if (_viewCrop.left + offset.cx < 0)
						offset.cx = -_viewCrop.left;
					if (_viewCrop.right + offset.cx > _scaleSize.cx)
						offset.cx = _scaleSize.cx - _viewCrop.right;
					_viewCrop.left  += offset.cx;
					_viewCrop.right += offset.cx;
				}
				if (rectH(_viewCrop) < _scaleSize.cy)
				{
					offset.cy = _viewOffset.cy;
					// Only apply dragging offset for pan dragging, not for zoom dragging
					if (_isdragging && _dragType == DT_PAN)
						offset.cy -= _draggingCur.y - _draggingOrigin.y;
					if (_viewCrop.top + offset.cy < 0)
						offset.cy = -_viewCrop.top;
					if (_viewCrop.bottom + offset.cy > _scaleSize.cy)
						offset.cy = _scaleSize.cy - _viewCrop.bottom;
					_viewCrop.top    += offset.cy;
					_viewCrop.bottom += offset.cy;
				}
				if (!_isdragging && !sizeEq(offset, _viewOffset))
					_viewOffset = offset;
			}
		}

		_viewWndDim = size;
		if (!rectEq(_viewCrop, oldViewCrop))
			releaseBitmap();
		if (!_viewBitmap)
		{
			DWORD tmstart = GetTickCount();
			_viewBitmap = image->getBBitmap(_scaleSize, _viewCrop);
			_timecost   = (int)(GetTickCount() - tmstart);
		}
		updateStatus();
	}

	if (!_viewBitmap)
	{
		fillBg(hdc);
		return;
	}

	// calculate the draw rect (centered if image is smaller than the view)
	RECT drawrect = crect;
	int imgw = rectW(_viewCrop);
	int imgh = rectH(_viewCrop);
	if (rectW(crect) > imgw)
	{
		drawrect.left  = (rectW(crect) - imgw) / 2;
		drawrect.right = drawrect.left + imgw;
	}
	if (rectH(crect) > imgh)
	{
		drawrect.top    = (rectH(crect) - imgh) / 2;
		drawrect.bottom = drawrect.top + imgh;
	}

	_viewBitmap->SetDIBitsToDevice(hdc, drawrect.left, drawrect.top, 0, 0, imgw, imgh);

	// fill background margins
	uint32_t raw = Doc::getBgColor();
	COLORREF bgcol = RGB((raw >> 16) & 0xff, (raw >> 8) & 0xff, raw & 0xff);
	HBRUSH bgbr = CreateSolidBrush(bgcol);
	if (drawrect.left > 0)
		fillRect(hdc, 0, 0, drawrect.left, crect.bottom, bgbr);
	if (drawrect.right < crect.right)
		fillRect(hdc, drawrect.right, 0, crect.right - drawrect.right, crect.bottom, bgbr);
	if (drawrect.top > 0)
		fillRect(hdc, drawrect.left, 0, imgw, drawrect.top, bgbr);
	if (drawrect.bottom < crect.bottom)
		fillRect(hdc, drawrect.left, drawrect.bottom, imgw, crect.bottom - drawrect.bottom, bgbr);
	DeleteObject(bgbr);
}

// ---- Message handler -------------------------------------------------------

LRESULT ImageView::HandleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(m_hwnd, &ps);
		OnPaint(hdc);
		EndPaint(m_hwnd, &ps);
		return 0;
	}

	case WM_ERASEBKGND:
		return TRUE;   // suppress default erase; OnPaint handles everything

	case WM_SIZE:
		// force bitmap recalculation on next paint
		_viewWndDim.cx = _viewWndDim.cy = -1;
		InvalidateRect(m_hwnd, NULL, FALSE);
		return 0;

	case WM_KEYDOWN:
	{
		UINT nChar = (UINT)wp;
		bool bshift = (GetKeyState(VK_SHIFT)   & 0x8000) != 0;
		bool bctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		bool balt   = (GetKeyState(VK_MENU)    & 0x8000) != 0;

		if (!_doc) break;

		if ((nChar == VK_LEFT || nChar == VK_UP || nChar == VK_PRIOR) && !bshift && !bctrl && !balt)
		{ _doc->navigate(Doc::NAV_PREV); return 0; }
		if ((nChar == VK_RIGHT || nChar == VK_DOWN || nChar == VK_NEXT) && !bshift && !bctrl && !balt)
		{ _doc->navigate(Doc::NAV_NEXT); return 0; }
		if (nChar == VK_HOME)
		{ _doc->navigate(Doc::NAV_FIRST); return 0; }
		if (nChar == VK_END)
		{ _doc->navigate(Doc::NAV_LAST);  return 0; }
		if ((nChar == VK_OEM_PLUS  || nChar == VK_ADD) && !balt)
		{ zoom(1);  return 0; }
		if ((nChar == VK_OEM_MINUS || nChar == VK_SUBTRACT) && !balt)
		{ zoom(-1); return 0; }
		break;
	}

	case WM_MOUSEWHEEL:
	{
		int delta = GET_WHEEL_DELTA_WPARAM(wp);
		if (_doc)
			_doc->navigate(delta > 0 ? Doc::NAV_PREV : Doc::NAV_NEXT);
		return 0;
	}

	case WM_LBUTTONDOWN:
	{
		SetFocus(m_hwnd);
		_isdragging     = true;
		_draggingOrigin = _draggingCur = POINT{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

		// Check if Ctrl is pressed for zoom drag
		bool bctrl  = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		if (bctrl)
		{
			_dragType = DT_ZOOMDRAG;
			_zoomDragOrigin = _draggingOrigin;
			_zoomDragStartLevel = (_zoomlevel != 0 ? _zoomlevel : _fitlevel);

			// Calculate the original image coordinate at the click position
			RECT crect;
			GetClientRect(m_hwnd, &crect);
			int centerOffsetX = (_scaleSize.cx - rectW(crect)) / 2;
			int centerOffsetY = (_scaleSize.cy - rectH(crect)) / 2;
			if (centerOffsetX < 0) centerOffsetX = 0;
			if (centerOffsetY < 0) centerOffsetY = 0;

			int scaledImageX = centerOffsetX + _viewOffset.cx + _zoomDragOrigin.x;
			int scaledImageY = centerOffsetY + _viewOffset.cy + _zoomDragOrigin.y;

			_clickImageCoord.cx = scaledImageX * 100 / _zoomDragStartLevel;
			_clickImageCoord.cy = scaledImageY * 100 / _zoomDragStartLevel;
		}
		else
		{
			_dragType = DT_PAN;
		}

		SetCapture(m_hwnd);
		return 0;
	}

	case WM_LBUTTONUP:
		if (_isdragging)
		{
			_isdragging  = false;
			ReleaseCapture();
			if (_dragType == DT_PAN)
			{
				// Apply pan offset
				_viewOffset.cx -= _draggingCur.x - _draggingOrigin.x;
				_viewOffset.cy -= _draggingCur.y - _draggingOrigin.y;
			}
			releaseBitmap();
			InvalidateRect(m_hwnd, NULL, FALSE);
		}
		return 0;

	case WM_RBUTTONDOWN:
	{
		SetFocus(m_hwnd);
		_isdragging     = true;
		_draggingOrigin = _draggingCur = POINT{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
		_dragType = DT_ZOOMDRAG;
		_zoomDragOrigin = _draggingOrigin;
		_zoomDragStartLevel = (_zoomlevel != 0 ? _zoomlevel : _fitlevel);

		// Calculate the original image coordinate at the click position
		RECT crect;
		GetClientRect(m_hwnd, &crect);
		int centerOffsetX = (_scaleSize.cx - rectW(crect)) / 2;
		int centerOffsetY = (_scaleSize.cy - rectH(crect)) / 2;
		if (centerOffsetX < 0) centerOffsetX = 0;
		if (centerOffsetY < 0) centerOffsetY = 0;

		int scaledImageX = centerOffsetX + _viewOffset.cx + _zoomDragOrigin.x;
		int scaledImageY = centerOffsetY + _viewOffset.cy + _zoomDragOrigin.y;

		_clickImageCoord.cx = scaledImageX * 100 / _zoomDragStartLevel;
		_clickImageCoord.cy = scaledImageY * 100 / _zoomDragStartLevel;

		SetCapture(m_hwnd);
		return 0;
	}

	case WM_RBUTTONUP:
		if (_isdragging)
		{
			_isdragging  = false;
			ReleaseCapture();
			releaseBitmap();
			InvalidateRect(m_hwnd, NULL, FALSE);
		}
		return 0;

	case WM_MOUSEMOVE:
		if (_isdragging)
		{
			_draggingCur = POINT{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};

			if (_dragType == DT_ZOOMDRAG)
			{
				// Calculate zoom: exponential scaling
				// Up drag (Y decrease) = zoom in with factor 1.01^(-dy)
				// Down drag (Y increase) = zoom out with factor 0.99^dy
				int dy = _draggingCur.y - _draggingOrigin.y;
				double factor = std::pow(1.01, -dy);  // negative dy for up = zoom in
				int targetZoomLevel = (int)(_zoomDragStartLevel * factor + 0.5);
				applyZoomDrag(targetZoomLevel);
			}

			releaseBitmap();
			InvalidateRect(m_hwnd, NULL, FALSE);
		}
		return 0;

	case WM_SETFOCUS:
		// ensure we can receive keyboard input
		return 0;
	}

	return WndBase::HandleMessage(msg, wp, lp);
}

// ---- Mouse drag zoom --------------------------------------------------------

void ImageView::applyZoomDrag(int targetZoomLevel)
{
	Image *image = _doc ? _doc->getImage() : NULL;
	if (!image) return;

	// Clamp to valid zoom range [1, 10000]
	static const int MIN_ZOOM = 1;
	static const int MAX_ZOOM = 10000;
	if (targetZoomLevel < MIN_ZOOM)
		targetZoomLevel = MIN_ZOOM;
	if (targetZoomLevel > MAX_ZOOM)
		targetZoomLevel = MAX_ZOOM;

	// Get current zoom level
	int curLevel = (_zoomlevel != 0 ? _zoomlevel : _fitlevel);
	if (targetZoomLevel == curLevel)
		return;

	// Save old state
	int oldZoomLevel = curLevel;
	SIZE oldScaleSize = _scaleSize;

	// Apply new zoom level
	_zoomlevel = targetZoomLevel;
	_scaleSize.cx = std::max((LONG)(image->getDimension().cx) * targetZoomLevel / 100, 1L);
	_scaleSize.cy = std::max((LONG)(image->getDimension().cy) * targetZoomLevel / 100, 1L);

	// Adjust _viewOffset to keep the clicked point at the same screen position
	adjustViewOffsetForZoom(oldZoomLevel, oldScaleSize);
}

void ImageView::adjustViewOffsetForZoom(int oldZoomLevel, SIZE oldScaleSize)
{
	RECT crect;
	GetClientRect(m_hwnd, &crect);

	// Calculate center offset for the new zoom level
	int newCenterOffsetX = (_scaleSize.cx - rectW(crect)) / 2;
	int newCenterOffsetY = (_scaleSize.cy - rectH(crect)) / 2;
	if (newCenterOffsetX < 0) newCenterOffsetX = 0;
	if (newCenterOffsetY < 0) newCenterOffsetY = 0;

	// Calculate the scaled image coordinate of _clickImageCoord in the new zoom level
	int scaledImageX = _clickImageCoord.cx * _zoomlevel / 100;
	int scaledImageY = _clickImageCoord.cy * _zoomlevel / 100;

	// Adjust _viewOffset so that scaledImageX/Y is at _zoomDragOrigin in view coordinates
	// Formula: viewCrop.left = centerOffset + _viewOffset.cx
	// And: viewCrop.left + _zoomDragOrigin.x = scaledImageX
	// So: _viewOffset.cx = scaledImageX - _zoomDragOrigin.x - centerOffsetX
	_viewOffset.cx = scaledImageX - _zoomDragOrigin.x - newCenterOffsetX;
	_viewOffset.cy = scaledImageY - _zoomDragOrigin.y - newCenterOffsetY;

	// Force recalculation of viewCrop on next paint
	_viewWndDim = {-1, -1};
}