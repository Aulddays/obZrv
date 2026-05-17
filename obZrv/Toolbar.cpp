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
#include "Toolbar.h"
#include <gdiplus.h>
#include <objbase.h>
#include "dpi.h"
#include "resource.h"

/* Shell32 icon indices */
//#define SHELL32_FOLDER   3    /* folder -- used for FileList toggle */

/* Icon images in the PNG strip; Icons from shell dll are appended after */
#define IMG_OPEN        0
#define IMG_PREV        1
#define IMG_NEXT        2
#define IMG_ZOOMIN      3
#define IMG_ZOOMTO      4   /* zoom to ratio */
#define IMG_ZOOMOUT     5
#define IMG_ZOOMMODE    6   /* zoom mode selector */
#define IMG_ZOOMREM     7   /* remember zoom */
#define IMG_ZOOM        8   /* zoom placeholder */
#define IMG_FILELIST    9   /* Toggle file list */
#define IMG_OPENREMOTE  10
#define IMG_REFRESH     11

Toolbar::Toolbar()
	: m_hBand(NULL), m_hwnd(NULL), m_hInst(NULL), m_himl(NULL), m_himlDis(NULL), m_bandH(0)
{
}

Toolbar::~Toolbar()
{
	if (m_himl)    ImageList_Destroy(m_himl);
	if (m_himlDis) ImageList_Destroy(m_himlDis);
}

/* static */
LRESULT CALLBACK Toolbar::BandProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_COMMAND || msg == WM_NOTIFY)
		return SendMessage(GetParent(hwnd), msg, wp, lp);
	return DefWindowProc(hwnd, msg, wp, lp);
}

/* static */
void Toolbar::RegisterBandClass(HINSTANCE hInst)
{
	static bool s_done = false;
	if (s_done) return;
	s_done = true;

	WNDCLASS wc      = {};
	wc.lpfnWndProc   = BandProc;
	wc.hInstance     = hInst;
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wc.lpszClassName = TEXT("obzToolBand");
	RegisterClass(&wc);
}

bool Toolbar::Create(HWND hParent, HINSTANCE hInst)
{
	m_hInst = hInst;
	RegisterBandClass(hInst);

	m_hBand = CreateWindowEx(
		0, TEXT("obzToolBand"), NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		0, 0, 0, 0,
		hParent, NULL, hInst, NULL);
	if (!m_hBand)
		return false;

	m_hwnd = CreateWindowEx(
		0, TOOLBARCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | CCS_NODIVIDER,
		0, 0, 0, 0,
		m_hBand, NULL, hInst, NULL);
	if (!m_hwnd)
		return false;

	SendMessage(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
	SendMessage(m_hwnd, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_DRAWDDARROWS);

	/* Button layout:
	 *  [Open][Prev][Next] | [ZoomIn][ZoomTo][ZoomMode][ZoomOut][ZoomRem][Zoom] | [FileList(toggle)][OpenRemote][Refresh]
	 * Stub buttons (ZoomTo, ZoomRem, Zoom) start disabled; image indices match IMG_* above. */
	TBBUTTON buttons[] = {
		{IMG_OPEN,        ID_FILE_OPEN,        TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{IMG_OPENREMOTE,  ID_FILE_OPEN_REMOTE, TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{IMG_REFRESH,     ID_FILE_REFRESH,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{IMG_FILELIST,    ID_VIEW_FILELIST,    TBSTATE_ENABLED, BTNS_CHECK,  {0}, 0, 0},
		{IMG_PREV,        ID_FILE_PREV,        TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{IMG_NEXT,        ID_FILE_NEXT,        TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{0,               0,                   TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
		{IMG_ZOOMIN,      ID_VIEW_ZOOMIN,      TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{IMG_ZOOMTO,      ID_VIEW_ZOOMTO,      0,               BTNS_BUTTON, {0}, 0, 0},
		{IMG_ZOOMMODE,    ID_VIEW_ZOOMMODE,    TBSTATE_ENABLED, BTNS_WHOLEDROPDOWN, {0}, 0, 0},
		{IMG_ZOOMOUT,     ID_VIEW_ZOOMOUT,     TBSTATE_ENABLED, BTNS_BUTTON, {0}, 0, 0},
		{IMG_ZOOMREM,     ID_VIEW_ZOOMREM,     0,               BTNS_BUTTON, {0}, 0, 0},
		{IMG_ZOOM,        ID_VIEW_ZOOM,        0,               BTNS_BUTTON, {0}, 0, 0},
		{0,               0,                   TBSTATE_ENABLED, BTNS_SEP,    {0}, 0, 0},
	};
	SendMessage(m_hwnd, TB_ADDBUTTONS,
				sizeof(buttons) / sizeof(buttons[0]), (LPARAM)buttons);
	UpdateDpi();
	return true;
}

/* Select RCDATA resource ID and icon size for current DPI tier. */
static void SelectDpiTier(int *resId, int *iconSz)
{
	int dpi = Dpi::Get();
	if      (dpi < 108) { *resId = IDR_TOOLBAR_96;  *iconSz = 20; }
	else if (dpi < 132) { *resId = IDR_TOOLBAR_120; *iconSz = 25; }
	else if (dpi < 168) { *resId = IDR_TOOLBAR_144; *iconSz = 30; }
	else                { *resId = IDR_TOOLBAR_192; *iconSz = 40; }
}

/* Build a 32-bit premultiplied-alpha DIB from a GDI+ Bitmap for use in ILC_COLOR32.
 * gray=true converts RGB to luminance; alphaPercent scales the alpha channel.
 * ILC_COLOR32 expects premultiplied BGRA -- GetHBITMAP composites onto background
 * instead, so we build the DIB manually here. */
static HBITMAP MakePremultipliedDIB(Gdiplus::Bitmap *bmp, bool gray, int alphaPercent)
{
	int w = (int)bmp->GetWidth(), h = (int)bmp->GetHeight();

	BITMAPINFO bi = {};
	bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth       = w;
	bi.bmiHeader.biHeight      = -h;   /* top-down */
	bi.bmiHeader.biPlanes      = 1;
	bi.bmiHeader.biBitCount    = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	void *bits = NULL;
	HBITMAP hbmp = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
	if (!hbmp || !bits) return NULL;

	Gdiplus::BitmapData bd = {};
	Gdiplus::Rect rect(0, 0, w, h);
	if (bmp->LockBits(&rect, Gdiplus::ImageLockModeRead,
					  PixelFormat32bppARGB, &bd) != Gdiplus::Ok) {
		DeleteObject(hbmp);
		return NULL;
	}

	/* GDI+ PixelFormat32bppARGB byte order: B G R A */
	const BYTE *src = (const BYTE *)bd.Scan0;
	BYTE       *dst = (BYTE *)bits;
	for (int i = 0; i < w * h; i++, src += 4, dst += 4) {
		BYTE b = src[0], g = src[1], r = src[2], a = src[3];
		if (gray) {
			BYTE lum = (BYTE)((r * 77u + g * 150u + b * 29u) >> 8);
			r = g = b = lum;
		}
		UINT a2 = (UINT)a * (UINT)alphaPercent / 100;
		/* premultiply: each channel * a2 / 255 */
		dst[0] = (BYTE)((UINT)b * a2 / 255);
		dst[1] = (BYTE)((UINT)g * a2 / 255);
		dst[2] = (BYTE)((UINT)r * a2 / 255);
		dst[3] = (BYTE)a2;
	}
	bmp->UnlockBits(&bd);
	return hbmp;
}

/* Build disabled icon: grayscale blended onto COLOR_BTNFACE at iconOpacity%.
 * Output is fully opaque -- no alpha dependency on the toolbar's drawing path. */
static HBITMAP MakeDisabledDIB(Gdiplus::Bitmap *bmp, int iconOpacity)
{
	int w = (int)bmp->GetWidth(), h = (int)bmp->GetHeight();

	BITMAPINFO bi = {};
	bi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth       = w;
	bi.bmiHeader.biHeight      = -h;
	bi.bmiHeader.biPlanes      = 1;
	bi.bmiHeader.biBitCount    = 32;
	bi.bmiHeader.biCompression = BI_RGB;

	void *bits = NULL;
	HBITMAP hbmp = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
	if (!hbmp || !bits) return NULL;

	COLORREF bgcr = GetSysColor(COLOR_BTNFACE);
	UINT bgR = GetRValue(bgcr), bgG = GetGValue(bgcr), bgB = GetBValue(bgcr);

	Gdiplus::BitmapData bd = {};
	Gdiplus::Rect rect(0, 0, w, h);
	if (bmp->LockBits(&rect, Gdiplus::ImageLockModeRead,
					  PixelFormat32bppARGB, &bd) != Gdiplus::Ok) {
		DeleteObject(hbmp);
		return NULL;
	}

	/* For each pixel:  out = lum * (a/255 * opacity) + bg * (1 - a/255 * opacity)
	 * Use denominator 25500 = 255 * 100 to keep integer arithmetic.             */
	const BYTE *src = (const BYTE *)bd.Scan0;
	BYTE       *dst = (BYTE *)bits;
	for (int i = 0; i < w * h; i++, src += 4, dst += 4) {
		BYTE b = src[0], g = src[1], r = src[2], a = src[3];
		BYTE lum  = (BYTE)((r * 77u + g * 150u + b * 29u) >> 8);
		UINT iconW = (UINT)a * (UINT)iconOpacity;   /* 0 .. 255*100 = 25500 */
		UINT bgW   = 25500u - iconW;
		dst[0] = (BYTE)((lum * iconW + bgB * bgW) / 25500u);
		dst[1] = (BYTE)((lum * iconW + bgG * bgW) / 25500u);
		dst[2] = (BYTE)((lum * iconW + bgR * bgW) / 25500u);
		dst[3] = 255;   /* fully opaque -- no alpha channel needed */
	}
	bmp->UnlockBits(&bd);
	return hbmp;
}

/* Load a PNG strip from RCDATA and build normal + disabled ILC_COLOR32 image lists.
 * Disabled variant: grayscale + reduced alpha.
 * Returns the normal list (NULL on failure); *pHimlDis receives the disabled list. */
HIMAGELIST Toolbar::LoadPngStrip(int resId, int iconSz, HIMAGELIST *pHimlDis) const
{
	if (pHimlDis) *pHimlDis = NULL;

	HRSRC hrsrc = FindResource(m_hInst, MAKEINTRESOURCE(resId), RT_RCDATA);
	if (!hrsrc) return NULL;
	HGLOBAL hg = LoadResource(m_hInst, hrsrc);
	if (!hg) return NULL;
	void *data = LockResource(hg);
	DWORD size = SizeofResource(m_hInst, hrsrc);
	if (!data || !size) return NULL;

	IStream *stm = NULL;
	HGLOBAL hgMem = GlobalAlloc(GMEM_MOVEABLE, size);
	if (!hgMem) return NULL;
	void *buf = GlobalLock(hgMem);
	memcpy(buf, data, size);
	GlobalUnlock(hgMem);
	if (FAILED(CreateStreamOnHGlobal(hgMem, TRUE /*auto-free*/, &stm))) {
		GlobalFree(hgMem);
		return NULL;
	}

	Gdiplus::Bitmap *bmp = Gdiplus::Bitmap::FromStream(stm);
	stm->Release();
	if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) {
		delete bmp;
		return NULL;
	}

	HBITMAP hbmpNormal = MakePremultipliedDIB(bmp, false, 100);

	if (pHimlDis) {
		HBITMAP hbmpDis = MakeDisabledDIB(bmp, 40);  /* 40% icon on bg */
		if (hbmpDis) {
			HIMAGELIST himlDis = ImageList_Create(iconSz, iconSz, ILC_COLOR32, 10, 0);
			if (himlDis) {
				ImageList_Add(himlDis, hbmpDis, NULL);
				*pHimlDis = himlDis;
			}
			DeleteObject(hbmpDis);
		}
	}

	delete bmp;

	if (!hbmpNormal) return NULL;
	HIMAGELIST himl = ImageList_Create(iconSz, iconSz, ILC_COLOR32, 10, 0);
	if (himl) ImageList_Add(himl, hbmpNormal, NULL);
	DeleteObject(hbmpNormal);
	return himl;
}

/* Rebuild image list and resize buttons for current DPI.
 * Safe to call multiple times (e.g. on WM_DPICHANGED). */
void Toolbar::UpdateDpi()
{
	if (!m_hwnd) return;

	int resId, iconSz;
	SelectDpiTier(&resId, &iconSz);

	/* Build normal + disabled image lists from PNG strip */
	HIMAGELIST himlNewDis = NULL;
	HIMAGELIST himlNew = LoadPngStrip(resId, iconSz, &himlNewDis);

	/* Append the FileList (folder) shell32 icon */
	//if (himlNew) {
	//    HICON hRefresh = LoadShell32Icon(SHELL32_REFRESH, iconSz);
	//    if (hRefresh) {
	//        ImageList_AddIcon(himlNew, hRefresh);
	//        if (himlNewDis) ImageList_AddIcon(himlNewDis, hRefresh);
	//        DestroyIcon(hRefresh);
	//    }
	//}

	/* Swap in the new image lists */
	if (m_himl) {
		SendMessage(m_hwnd, TB_SETIMAGELIST,    0, 0);
		SendMessage(m_hwnd, TB_SETDISABLEDIMAGELIST, 0, 0);
		ImageList_Destroy(m_himl);
	}
	if (m_himlDis) ImageList_Destroy(m_himlDis);
	m_himl    = himlNew;
	m_himlDis = himlNewDis;

	if (m_himl)
		SendMessage(m_hwnd, TB_SETIMAGELIST, 0, (LPARAM)m_himl);
	if (m_himlDis)
		SendMessage(m_hwnd, TB_SETDISABLEDIMAGELIST, 0, (LPARAM)m_himlDis);
	SendMessage(m_hwnd, TB_SETBITMAPSIZE, 0, MAKELONG(iconSz, iconSz));
	SendMessage(m_hwnd, TB_AUTOSIZE, 0, 0);

	m_bandH = HIWORD(SendMessage(m_hwnd, TB_GETBUTTONSIZE, 0, 0)) + 4;
}

void Toolbar::AutoSize()
{
	if (!m_hBand || !m_hwnd) return;

	RECT rc = {};
	GetClientRect(GetParent(m_hBand), &rc);

	SetWindowPos(m_hBand, NULL, 0, 0, rc.right, m_bandH,
				 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
	SetWindowPos(m_hwnd,  NULL, 0, 0, rc.right, m_bandH,
				 SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
}

void Toolbar::SetPressed(UINT cmdId, bool pressed)
{
	if (m_hwnd)
		SendMessage(m_hwnd, TB_CHECKBUTTON, cmdId,
					MAKELONG(pressed ? TRUE : FALSE, 0));
}

void Toolbar::EnableButton(UINT cmdId, bool enable)
{
	if (m_hwnd)
		SendMessage(m_hwnd, TB_ENABLEBUTTON, cmdId,
					MAKELONG(enable ? TRUE : FALSE, 0));
}

/* static */
HICON Toolbar::LoadShell32Icon(int idx, int size)
{
	HICON hLarge = NULL;
	ExtractIconEx(TEXT("shell32.dll"), idx, &hLarge, NULL, 1);
	if (!hLarge) return NULL;
	HICON hSized = (HICON)CopyImage(hLarge, IMAGE_ICON, size, size, 0);
	DestroyIcon(hLarge);
	return hSized;
}

HBITMAP Toolbar::GetMenuIcon(UINT cmdId, int sz) const
{
	if (!m_himl) return NULL;

	int idx;
	switch (cmdId) {
	case ID_FILE_OPEN:     idx = IMG_OPEN;     break;
	case ID_FILE_PREV:     idx = IMG_PREV;     break;
	case ID_FILE_NEXT:     idx = IMG_NEXT;     break;
	case ID_VIEW_ZOOMIN:   idx = IMG_ZOOMIN;   break;
	case ID_VIEW_ZOOMTO:   idx = IMG_ZOOMTO;   break;
	case ID_VIEW_ZOOMOUT:  idx = IMG_ZOOMOUT;  break;
	case ID_VIEW_ZOOMMODE: idx = IMG_ZOOMMODE; break;
	case ID_VIEW_ZOOMREM:  idx = IMG_ZOOMREM;  break;
	case ID_VIEW_ZOOM:     idx = IMG_ZOOM;     break;
	case ID_VIEW_FILELIST:    idx = IMG_FILELIST;    break;
	case ID_FILE_OPEN_REMOTE: idx = IMG_OPENREMOTE;  break;
	case ID_FILE_REFRESH:     idx = IMG_REFRESH;     break;
	default: return NULL;
	}

	/* Extract icon from the image list's internal strip via GetDIBits.
	 * StretchBlt(SRCCOPY) zeroes the alpha byte when passing through GDI,
	 * so we read raw pixels directly with GetDIBits to preserve premultiplied
	 * BGRA data, then manually scale the sub-region into the output DIB. */
	IMAGEINFO ii = {};
	if (!ImageList_GetImageInfo(m_himl, idx, &ii)) return NULL;

	BITMAP bm = {};
	GetObject(ii.hbmImage, sizeof(bm), &bm);
	int stripW = bm.bmWidth;
	int stripH = bm.bmHeight;
	if (stripW <= 0 || stripH <= 0) return NULL;

	/* Read full strip pixels bottom-up (positive biHeight) */
	BITMAPINFOHEADER bih = {};
	bih.biSize        = sizeof(bih);
	bih.biWidth       = stripW;
	bih.biHeight      = stripH;
	bih.biPlanes      = 1;
	bih.biBitCount    = 32;
	bih.biCompression = BI_RGB;

	BYTE *strip = (BYTE *)HeapAlloc(GetProcessHeap(), 0, stripW * stripH * 4);
	if (!strip) return NULL;

	HDC hdcTmp = CreateCompatibleDC(NULL);
	int rows = GetDIBits(hdcTmp, ii.hbmImage, 0, (UINT)stripH,
						  strip, (BITMAPINFO *)&bih, DIB_RGB_COLORS);
	DeleteDC(hdcTmp);
	if (!rows) { HeapFree(GetProcessHeap(), 0, strip); return NULL; }

	/* Create output top-down 32-bit DIB section */
	BITMAPINFOHEADER outBih = {};
	outBih.biSize        = sizeof(outBih);
	outBih.biWidth       = sz;
	outBih.biHeight      = -sz;
	outBih.biPlanes      = 1;
	outBih.biBitCount    = 32;
	outBih.biCompression = BI_RGB;

	void *outBits = NULL;
	HBITMAP hbmp = CreateDIBSection(NULL, (BITMAPINFO *)&outBih,
									 DIB_RGB_COLORS, &outBits, NULL, 0);
	if (!hbmp) { HeapFree(GetProcessHeap(), 0, strip); return NULL; }

	/* Scale icon sub-region into output with nearest-neighbour.
	 * ii.rcImage is top-down; strip is stored bottom-up by GetDIBits. */
	int srcX = ii.rcImage.left;
	int srcY = ii.rcImage.top;
	int srcW = ii.rcImage.right  - srcX;
	int srcH = ii.rcImage.bottom - srcY;

	BYTE *dst = (BYTE *)outBits;
	for (int dy = 0; dy < sz; dy++) {
		int sytd = srcY + dy * srcH / sz;
		int sybu = (stripH - 1) - sytd;   /* flip top-down -> bottom-up */
		for (int dx = 0; dx < sz; dx++) {
			int sx = srcX + dx * srcW / sz;
			const BYTE *s = strip + (sybu * stripW + sx) * 4;
			BYTE       *d = dst   + (dy   * sz     + dx) * 4;
			d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = s[3];
		}
	}

	HeapFree(GetProcessHeap(), 0, strip);
	return hbmp;
}
