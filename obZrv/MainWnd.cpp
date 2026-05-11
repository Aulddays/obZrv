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
#include "MainWnd.h"
#include "../unifs/remote_fs.hpp"
#include "resource.h"
#include "dpi.h"
#include "config.h"
#include <shellapi.h>

/* Find the submenu that DIRECTLY contains cmdId as a non-popup item. */
static HMENU FindSubMenuWith(HMENU hMenu, UINT cmdId)
{
	int n = GetMenuItemCount(hMenu);
	for (int i = 0; i < n; i++) {
		HMENU sub = GetSubMenu(hMenu, i);
		if (!sub) continue;
		/* Check direct children of sub */
		int m = GetMenuItemCount(sub);
		for (int j = 0; j < m; j++) {
			if (GetMenuItemID(sub, j) == cmdId)
				return sub;
		}
		/* Recurse */
		HMENU found = FindSubMenuWith(sub, cmdId);
		if (found) return found;
	}
	return NULL;
}

bool MainWnd::Create(HINSTANCE hInst, int nShow)
{
	m_hInst           = hInst;
	m_fileListVisible = true;
	m_zoomMode        = ID_ZOOMMODE_W2I_ZOOMOUT;

	WNDCLASSEX wc   = {};
	wc.cbSize        = sizeof(wc);
	wc.lpfnWndProc   = WndBase::WndProcStatic;
	wc.hInstance     = hInst;
	wc.hIcon         = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPICON));
	wc.hIconSm       = LoadIcon(hInst, MAKEINTRESOURCE(IDI_APPICON));
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = MAINWND_CLASS;

	if (!RegisterClassEx(&wc))
		return false;

	HWND hwnd = CreateWindowEx(
		0, MAINWND_CLASS, TEXT("obZrv"),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
		NULL, NULL, hInst, this);

	if (!hwnd)
		return false;

	ShowWindow(hwnd, nShow);
	UpdateWindow(hwnd);
	return true;
}

/* Create child controls after the window handle is available (WM_CREATE). */
bool MainWnd::OnCreate()
{
	Dpi::Update(m_hwnd);

	if (!m_menu.Load(m_hInst, IDR_MAINMENU))
		return false;
	m_menu.Attach(m_hwnd);
	m_menu.SetChecked(ID_VIEW_FILELIST, m_fileListVisible);

	if (!m_toolbar.Create(m_hwnd, m_hInst))
		return false;
	m_toolbar.SetPressed(ID_VIEW_FILELIST, m_fileListVisible);

	if (!m_statusBar.Create(m_hwnd, m_hInst))
		return false;

	if (!m_mainView.Create(m_hwnd, m_hInst))
		return false;

	// Wire Doc <-> ImageView
	Doc::initCodec();
	m_doc.setView(&m_mainView.imageView());
	m_mainView.imageView().setDoc(&m_doc);
	m_mainView.imageView().setToolbar(m_toolbar.toolbarHwnd());
	m_doc.setFileList(&m_mainView.fileList());
	m_mainView.fileList().setDoc(&m_doc);

	// Accept drag-and-drop files
	DragAcceptFiles(m_hwnd, TRUE);

	// Load persisted zoom mode
	m_zoomMode = Config::instance().getInt(L"Settings", L"ZoomMode",
										   ID_ZOOMMODE_W2I_ZOOMOUT);
	if (m_zoomMode < ID_ZOOMMODE_W2I_ZOOMOUT || m_zoomMode > ID_ZOOMMODE_NOFIT)
		m_zoomMode = ID_ZOOMMODE_W2I_ZOOMOUT;
	m_mainView.imageView().setZoomMode(m_zoomMode);

	// Give ImageView initial keyboard focus
	SetFocus(m_mainView.imageView().hwnd());

	// Initialise button enable states (all disabled until a file is opened)
	UpdateButtonStates();
	UpdateMenuIcons();

	return true;
}

void MainWnd::OpenFile(const wchar_t *path)
{
	// Normalise to Unix-style path (/c/dir/file) so all downstream code
	// (Doc, UniFs, UniFile) works with a single path convention.
	std::wstring upath = win_path_to_unix(path);
	m_doc.open(upath.c_str());
	// Ensure ImageView gets focus for keyboard navigation
	SetFocus(m_mainView.imageView().hwnd());
}

LRESULT MainWnd::HandleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_CREATE:
		return OnCreate() ? 0 : -1;

	case WM_SIZE:
		OnSize(LOWORD(lp), HIWORD(lp));
		return 0;

	case WM_COMMAND:
		OnCommand(LOWORD(wp));
		return 0;

	case WM_MENUSELECT:
		OnMenuSelect(LOWORD(wp), HIWORD(wp));
		return 0;

	case WM_INITMENUPOPUP:
	{
		/* Radio-check the active zoom mode when the Zoom Mode submenu opens.
		 * Use GetMenuItemID to check direct membership (MF_BYCOMMAND is recursive). */
		HMENU hPop = (HMENU)wp;
		int n = GetMenuItemCount(hPop);
		for (int i = 0; i < n; i++) {
			if (GetMenuItemID(hPop, i) == ID_ZOOMMODE_W2I_ZOOMOUT) {
				CheckMenuRadioItem(hPop,
								   ID_ZOOMMODE_W2I_ZOOMOUT, ID_ZOOMMODE_NOFIT,
								   m_zoomMode, MF_BYCOMMAND);
				break;
			}
		}
		return 0;
	}

	case WM_NOTIFY:
		OnNotify(lp);
		return 0;

	case WM_DESTROY:
		OnDestroy();
		return 0;

	case WM_DPICHANGED:
		OnDpiChanged(reinterpret_cast<const RECT *>(lp));
		return 0;

	case WM_APP_SETINFO:
		m_statusBar.SetFileInfo(reinterpret_cast<const TCHAR *>(lp));
		UpdateButtonStates();
		{
			int idx = m_doc.getDirIdx();
			if (idx >= 0) {
				wchar_t title[MAX_PATH + 8];
				_snwprintf(title, MAX_PATH + 8, L"%s - obZrv",
						   m_doc.getDirFile(idx).c_str());
				SetWindowTextW(m_hwnd, title);
			}
		}
		return 0;

	case WM_DROPFILES:
	{
		HDROP hDrop = (HDROP)wp;
		wchar_t path[MAX_PATH];
		if (DragQueryFileW(hDrop, 0, path, MAX_PATH))
			OpenFile(path);
		DragFinish(hDrop);
		return 0;
	}
	}
	return WndBase::HandleMessage(msg, wp, lp);
}

void MainWnd::OnSize(int cx, int cy)
{
	m_toolbar.AutoSize();
	m_statusBar.OnParentSize();

	int tbH  = m_toolbar.bandHeight();
	int sbH  = m_statusBar.height();
	int viewH = cy - tbH - sbH;
	if (viewH < 0) viewH = 0;

	m_mainView.Layout(0, tbH, cx, viewH);
}

void MainWnd::OnCommand(UINT id)
{
	switch (id) {
	case ID_FILE_OPEN:
	{
		static const wchar_t filter[] =
			L"Image Files\0*.bmp;*.jpg;*.jpeg;*.gif;*.png;*.tiff;*.tif;*.ico;*.webp\0"
			L"All Files\0*.*\0";
		wchar_t path[MAX_PATH] = {};
		OPENFILENAMEW ofn = {};
		ofn.lStructSize  = sizeof(ofn);
		ofn.hwndOwner    = m_hwnd;
		ofn.lpstrFilter  = filter;
		ofn.lpstrFile    = path;
		ofn.nMaxFile     = MAX_PATH;
		ofn.Flags        = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
		if (GetOpenFileNameW(&ofn))
			OpenFile(path);
		break;
	}

	case ID_FILE_OPEN_REMOTE:
	{
		ConnectDlg dlg;
		std::string host;
		uint16_t    port = 0;
		auto client = dlg.DoModal(m_hwnd, host, port);
		if (client) {
			RemoteBrowserDlg browser;
			std::string remotePath = browser.DoModal(m_hwnd, client.get());
			if (!remotePath.empty()) {
				m_doc.setClient(std::shared_ptr<UniFs>(client.release()),
								host, port);
				// Build remote:// URL for Doc::open
				std::string url = "remote://" + host + ":" +
								  std::to_string(port) + remotePath;
				std::wstring wurl = utf8_to_wstr(url.c_str());
				m_doc.open(wurl.c_str());
				SetFocus(m_mainView.imageView().hwnd());
			}
		}
		break;
	}

	case ID_FILE_EXIT:
		DestroyWindow(m_hwnd);
		break;

	case ID_FILE_PREV:
		m_doc.navigate(Doc::NAV_PREV);
		break;

	case ID_FILE_NEXT:
		m_doc.navigate(Doc::NAV_NEXT);
		break;

	case ID_FILE_DELETE:
		m_doc.removeCurrentFile(false);
		break;

	case ID_FILE_DELETE_PERM:
		m_doc.removeCurrentFile(true);
		break;

	case ID_VIEW_ZOOMIN:
		m_mainView.imageView().zoom(1);
		break;

	case ID_VIEW_ZOOMOUT:
		m_mainView.imageView().zoom(-1);
		break;

	case ID_VIEW_ZOOMMODE:
		ShowZoomModeMenu();
		break;

	case ID_VIEW_FILELIST:
		m_fileListVisible = !m_fileListVisible;
		m_menu.SetChecked(ID_VIEW_FILELIST, m_fileListVisible);
		m_toolbar.SetPressed(ID_VIEW_FILELIST, m_fileListVisible);
		m_mainView.ShowFileList(m_fileListVisible);
		break;

	case ID_HELP_ABOUT:
		MessageBox(m_hwnd,
				   TEXT("obZrv\nVersion 0.1"),
				   TEXT("About obZrv"),
				   MB_OK | MB_ICONINFORMATION);
		break;
	}

	/* Zoom mode sub-commands (range check outside switch) */
	if (id >= ID_ZOOMMODE_W2I_ZOOMOUT && id <= ID_ZOOMMODE_NOFIT) {
		m_zoomMode = (int)id;
		Config::instance().setInt(L"Settings", L"ZoomMode", m_zoomMode);
		m_mainView.imageView().setZoomMode(m_zoomMode);
	}
}

/* Show status bar hint when hovering over a menu item. */
void MainWnd::OnMenuSelect(UINT id, UINT flags)
{
	if (flags == 0xFFFF) {
		m_statusBar.SetHint(TEXT(""));
		return;
	}
	if (flags & (MF_POPUP | MF_SEPARATOR))
		return;

	TCHAR buf[256] = {};
	LoadString(m_hInst, id, buf, 256);
	m_statusBar.SetHint(buf);
}

/* Supply tooltip text for toolbar buttons and show status bar hint. */
void MainWnd::OnNotify(LPARAM lp)
{
	NMHDR *hdr = reinterpret_cast<NMHDR *>(lp);

	if (hdr->code == TTN_GETDISPINFO) {
		NMTTDISPINFO *tt = reinterpret_cast<NMTTDISPINFO *>(lp);
		static TCHAR tipBuf[128];
		LoadString(m_hInst, (UINT)tt->hdr.idFrom + STRING_TIP_OFFSET, tipBuf, 128);
		tt->lpszText = tipBuf;

	} else if (hdr->code == TBN_DROPDOWN) {
		NMTOOLBAR *tb = reinterpret_cast<NMTOOLBAR *>(lp);
		if (tb->iItem == ID_VIEW_ZOOMMODE)
			ShowZoomModeMenu();
	} else if (hdr->code == TBN_HOTITEMCHANGE) {
		NMTBHOTITEM *hi = reinterpret_cast<NMTBHOTITEM *>(lp);
		if (hi->dwFlags & HICF_LEAVING) {
			m_statusBar.SetHint(TEXT(""));
		} else {
			TCHAR hintBuf[256] = {};
			LoadString(m_hInst, (UINT)hi->idNew, hintBuf, 256);
			m_statusBar.SetHint(hintBuf);
		}
	}
}

void MainWnd::OnDestroy()
{
	PostQuitMessage(0);
}

/* Respond to monitor DPI changes (Windows 8.1+, PerMonitorV2 on Win10+).
 * pRect is the suggested window position/size Windows calculated for us. */
void MainWnd::OnDpiChanged(const RECT *pRect)
{
	Dpi::Update(m_hwnd);
	m_toolbar.UpdateDpi();
	SetWindowPos(m_hwnd, NULL,
				 pRect->left, pRect->top,
				 pRect->right  - pRect->left,
				 pRect->bottom - pRect->top,
				 SWP_NOZORDER | SWP_NOACTIVATE);
	UpdateMenuIcons();
	UpdateButtonStates();
}

void MainWnd::ShowZoomModeMenu()
{
	/* Find the Zoom Mode submenu from the main menu */
	HMENU hSub = FindSubMenuWith(m_menu.handle(), ID_ZOOMMODE_W2I_ZOOMOUT);
	if (!hSub) return;

	/* Radio-check the active mode */
	CheckMenuRadioItem(hSub,
					   ID_ZOOMMODE_W2I_ZOOMOUT, ID_ZOOMMODE_NOFIT,
					   m_zoomMode, MF_BYCOMMAND);

	/* Position menu below the toolbar button */
	HWND hTb  = m_toolbar.toolbarHwnd();
	int  bidx = (int)SendMessage(hTb, TB_COMMANDTOINDEX, ID_VIEW_ZOOMMODE, 0);
	RECT btnRc = {};
	SendMessage(hTb, TB_GETITEMRECT, bidx, (LPARAM)&btnRc);
	POINT pt = {btnRc.left, btnRc.bottom};
	ClientToScreen(hTb, &pt);

	TrackPopupMenu(hSub, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
				   pt.x, pt.y, 0, m_hwnd, NULL);
}


void MainWnd::ClearMenuBitmaps()
{
	/* Remove bitmaps from menu items before deleting to avoid dangling handles */
	MENUITEMINFOW mii = {};
	mii.cbSize   = sizeof(mii);
	mii.fMask    = MIIM_BITMAP;
	mii.hbmpItem = NULL;

	HMENU hMain = m_menu.handle();
	static const UINT cmds[] = {
		ID_FILE_OPEN, ID_FILE_PREV, ID_FILE_NEXT,
		ID_VIEW_ZOOMIN, ID_VIEW_ZOOMTO, ID_VIEW_ZOOMOUT,
		ID_VIEW_ZOOMREM, ID_VIEW_ZOOM, ID_VIEW_FILELIST
	};
	for (int i = 0; i < (int)(sizeof(cmds)/sizeof(cmds[0])); i++)
		SetMenuItemInfoW(hMain, cmds[i], FALSE, &mii);

	/* Clear Zoom Mode popup bitmap */
	HMENU hView    = GetSubMenu(hMain, 2);  /* File=0, Edit=1, View=2 */
	HMENU hZoomSub = FindSubMenuWith(hMain, ID_ZOOMMODE_W2I_ZOOMOUT);
	if (hView && hZoomSub) {
		int n = GetMenuItemCount(hView);
		for (int i = 0; i < n; i++) {
			if (GetSubMenu(hView, i) == hZoomSub) {
				SetMenuItemInfoW(hView, (UINT)i, TRUE, &mii);
				break;
			}
		}
	}

	for (HBITMAP hb : m_menuBitmaps) DeleteObject(hb);
	m_menuBitmaps.clear();
}

void MainWnd::UpdateMenuIcons()
{
	ClearMenuBitmaps();

	int sz = GetSystemMetrics(SM_CYSMICON);

	/* Commands with both a menu item and a toolbar icon */
	static const struct { UINT cmd; } items[] = {
		{ ID_FILE_OPEN    },
		{ ID_FILE_PREV    },
		{ ID_FILE_NEXT    },
		{ ID_VIEW_ZOOMIN  },
		{ ID_VIEW_ZOOMTO  },
		{ ID_VIEW_ZOOMOUT },
		{ ID_VIEW_ZOOMREM },
		{ ID_VIEW_ZOOM    },
		{ ID_VIEW_FILELIST },
	};
	HMENU hMain = m_menu.handle();
	MENUITEMINFOW mii = {};
	mii.cbSize = sizeof(mii);
	mii.fMask  = MIIM_BITMAP;

	for (int i = 0; i < (int)(sizeof(items)/sizeof(items[0])); i++) {
		HBITMAP hbmp = m_toolbar.GetMenuIcon(items[i].cmd, sz);
		if (!hbmp) continue;
		mii.hbmpItem = hbmp;
		SetMenuItemInfoW(hMain, items[i].cmd, FALSE, &mii);
		m_menuBitmaps.push_back(hbmp);
	}

	/* Zoom Mode is a POPUP item -- find it by submenu handle, set by position */
	HMENU hView    = GetSubMenu(hMain, 2);  /* File=0, Edit=1, View=2 */
	HMENU hZoomSub = FindSubMenuWith(hMain, ID_ZOOMMODE_W2I_ZOOMOUT);
	if (hView && hZoomSub) {
		int n = GetMenuItemCount(hView);
		for (int i = 0; i < n; i++) {
			if (GetSubMenu(hView, i) == hZoomSub) {
				HBITMAP hbmp = m_toolbar.GetMenuIcon(ID_VIEW_ZOOMMODE, sz);
				if (hbmp) {
					mii.hbmpItem = hbmp;
					SetMenuItemInfoW(hView, (UINT)i, TRUE, &mii);
					m_menuBitmaps.push_back(hbmp);
				}
				break;
			}
		}
	}
}


void MainWnd::UpdateButtonStates()
{
	int idx = m_doc.getDirIdx();
	int cnt = m_doc.getDirCount();
	bool canPrev   = idx > 0;
	bool canNext   = idx >= 0 && idx < cnt - 1;
	bool canZoomIn  = m_mainView.imageView().zoom( 1, true) == 0;
	bool canZoomOut = m_mainView.imageView().zoom(-1, true) == 0;

	m_toolbar.EnableButton(ID_FILE_PREV,     canPrev);
	m_toolbar.EnableButton(ID_FILE_NEXT,     canNext);
	m_toolbar.EnableButton(ID_VIEW_ZOOMIN,   canZoomIn);
	m_toolbar.EnableButton(ID_VIEW_ZOOMOUT,  canZoomOut);
	m_toolbar.EnableButton(ID_VIEW_ZOOMMODE, true);

	m_menu.EnableItem(ID_FILE_PREV,    canPrev);
	m_menu.EnableItem(ID_FILE_NEXT,    canNext);
	m_menu.EnableItem(ID_VIEW_ZOOMIN,  canZoomIn);
	m_menu.EnableItem(ID_VIEW_ZOOMOUT, canZoomOut);
}

