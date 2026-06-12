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
#include "../unifs/local_fs.hpp"
#include "resource.h"
#include "dpi.h"
#include "config.h"
#include <shellapi.h>

#undef max
#undef min

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

static void LoadCommandHint(HINSTANCE hInst, UINT id, TCHAR *buf, int len, bool shortText)
{
	if (!buf || len <= 0)
		return;
	buf[0] = TEXT('\0');
	LoadString(hInst, id, buf, len);

	TCHAR *sep = wcschr(buf, TEXT('\n'));
	if (!sep)
		return;

	if (shortText && *(sep + 1)) {
		TCHAR *shortPart = sep + 1;
		int i = 0;
		while (i < len - 1 && shortPart[i]) {
			buf[i] = shortPart[i];
			i++;
		}
		buf[i] = TEXT('\0');
	} else {
		*sep = TEXT('\0');
	}
}

static void ClearCommandMenuBitmaps(HMENU hMenu)
{
	if (!hMenu)
		return;

	MENUITEMINFOW mii = {};
	mii.cbSize   = sizeof(mii);
	mii.fMask    = MIIM_BITMAP;
	mii.hbmpItem = NULL;

	int n = GetMenuItemCount(hMenu);
	for (int i = 0; i < n; i++) {
		SetMenuItemInfoW(hMenu, (UINT)i, TRUE, &mii);

		HMENU hSub = GetSubMenu(hMenu, i);
		if (hSub)
			ClearCommandMenuBitmaps(hSub);
	}
}

static void UpdateCommandMenuBitmaps(HMENU hMenu, const Toolbar &toolbar,
									 int sz, std::vector<HBITMAP> &bitmaps)
{
	if (!hMenu)
		return;

	MENUITEMINFOW mii = {};
	mii.cbSize = sizeof(mii);
	mii.fMask  = MIIM_BITMAP;

	int n = GetMenuItemCount(hMenu);
	for (int i = 0; i < n; i++) {
		HMENU hSub = GetSubMenu(hMenu, i);
		if (hSub) {
			UpdateCommandMenuBitmaps(hSub, toolbar, sz, bitmaps);
			continue;
		}

		UINT cmd = GetMenuItemID(hMenu, i);
		if (cmd == (UINT)-1) continue;

		HBITMAP hbmp = toolbar.GetMenuIcon(cmd, sz);
		if (!hbmp) continue;
		mii.hbmpItem = hbmp;
		SetMenuItemInfoW(hMenu, (UINT)i, TRUE, &mii);
		bitmaps.push_back(hbmp);
	}
}

static bool SetSubMenuBitmap(HMENU hMenu, HMENU hSubMenu, HBITMAP hbmp)
{
	if (!hMenu || !hSubMenu || !hbmp)
		return false;

	MENUITEMINFOW mii = {};
	mii.cbSize   = sizeof(mii);
	mii.fMask    = MIIM_BITMAP;
	mii.hbmpItem = hbmp;

	int n = GetMenuItemCount(hMenu);
	for (int i = 0; i < n; i++) {
		HMENU hChild = GetSubMenu(hMenu, i);
		if (hChild == hSubMenu)
			return SetMenuItemInfoW(hMenu, (UINT)i, TRUE, &mii) != FALSE;
		if (hChild && SetSubMenuBitmap(hChild, hSubMenu, hbmp))
			return true;
	}
	return false;
}

bool MainWnd::Create(HINSTANCE hInst, int nShow)
{
	m_hInst           = hInst;
	m_recentMenu      = NULL;
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

	RECT rc = {};
	bool maximized = false;
	bool hasPlacement = LoadWindowPlacement(&rc, &maximized);

	HWND hwnd = CreateWindowEx(
		0, MAINWND_CLASS, TEXT("obZrv"),
		WS_OVERLAPPEDWINDOW,
		hasPlacement ? rc.left : CW_USEDEFAULT,
		hasPlacement ? rc.top : CW_USEDEFAULT,
		hasPlacement ? rc.right - rc.left : 800,
		hasPlacement ? rc.bottom - rc.top : 600,
		NULL, NULL, hInst, this);

	if (!hwnd)
		return false;

	ShowWindow(hwnd, maximized ? SW_SHOWMAXIMIZED : nShow);
	UpdateWindow(hwnd);
	return true;
}

/* Create child controls after the window handle is available (WM_CREATE). */
bool MainWnd::OnCreate()
{
	Dpi::Update(m_hwnd);

	if (!m_menu.Load(m_hInst, IDR_MAINMENU))
		return false;
	m_recentFiles.Load();
	UpdateRecentFilesMenu();
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
	std::string uniPath = to_unipath(path);
	if (m_doc.open(std::shared_ptr<UniFs>(LocalFs::open()), uniPath.c_str(), -1, true, INT_MIN) == IM_OK) {
		m_recentFiles.AddLocal(uniPath);
		UpdateRecentFilesMenu();
	}
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
	if (id >= ID_FILE_RECENT_FIRST && id <= ID_FILE_RECENT_LAST) {
		OpenRecentFile(id);
		return;
	}

	switch (id) {
	case ID_FILE_OPEN:
	{
		std::wstring filter = L"Image Files";
		filter.push_back(L'\0');
		bool first = true;
		for (const std::string &type : Doc::getSupportedTypes())
		{
			if (!first)
				filter += L";";
			filter += L"*.";
			filter += utf8_to_wstr(type.c_str());
			first = false;
		}
		filter.push_back(L'\0');
		filter += L"All Files";
		filter.push_back(L'\0');
		filter += L"*.*";
		filter.push_back(L'\0');

		wchar_t path[MAX_PATH] = {};
		OPENFILENAMEW ofn = {};
		ofn.lStructSize  = sizeof(ofn);
		ofn.hwndOwner    = m_hwnd;
		ofn.lpstrFilter  = filter.c_str();
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
				if (m_doc.open(std::shared_ptr<UniFs>(client.release()),
						   remotePath.c_str(), -1, true, INT_MIN) == IM_OK) {
					m_recentFiles.AddRemote(host, port, remotePath);
					UpdateRecentFilesMenu();
				}
				SetFocus(m_mainView.imageView().hwnd());
			}
		}
		break;
	}

	case ID_FILE_SAVE_AS:
		SaveAs();
		break;

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

	case ID_FILE_REFRESH:
		m_doc.refreshDir();
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
	LoadCommandHint(m_hInst, id, buf, 256, false);
	m_statusBar.SetHint(buf);
}

/* Supply tooltip text for toolbar buttons and show status bar hint. */
void MainWnd::OnNotify(LPARAM lp)
{
	NMHDR *hdr = reinterpret_cast<NMHDR *>(lp);

	if (hdr->code == TTN_GETDISPINFO) {
		NMTTDISPINFO *tt = reinterpret_cast<NMTTDISPINFO *>(lp);
		static TCHAR tipBuf[128];
		LoadCommandHint(m_hInst, (UINT)tt->hdr.idFrom, tipBuf, 128, true);
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
			LoadCommandHint(m_hInst, (UINT)hi->idNew, hintBuf, 256, false);
			m_statusBar.SetHint(hintBuf);
		}
	}
}

void MainWnd::OnDestroy()
{
	SaveWindowPlacement();
	PostQuitMessage(0);
}

bool MainWnd::LoadWindowPlacement(RECT *rc, bool *maximized)
{
	std::wstring placement = Config::instance().getStr(L"Window", L"Placement");
	if (placement.empty())
		return false;

	RECT saved = {};
	int maxFlag = 0;
	if (swscanf(placement.c_str(), L"%ld,%ld,%ld,%ld,%d",
			   &saved.left, &saved.top, &saved.right, &saved.bottom,
			   &maxFlag) != 5)
		return false;
	if (saved.right <= saved.left || saved.bottom <= saved.top)
		return false;

	*rc = ClampWindowRectToNearestWorkArea(saved);
	*maximized = maxFlag != 0;
	return true;
}

void MainWnd::SaveWindowPlacement()
{
	WINDOWPLACEMENT wp = {};
	wp.length = sizeof(wp);
	if (!GetWindowPlacement(m_hwnd, &wp))
		return;

	wchar_t placement[128];
	_snwprintf(placement, 128, L"%ld,%ld,%ld,%ld,%d",
			  wp.rcNormalPosition.left,
			  wp.rcNormalPosition.top,
			  wp.rcNormalPosition.right,
			  wp.rcNormalPosition.bottom,
			  wp.showCmd == SW_SHOWMAXIMIZED ? 1 : 0);
	Config::instance().setStr(L"Window", L"Placement", placement);
}

RECT MainWnd::ClampWindowRectToNearestWorkArea(const RECT &rc)
{
	RECT work = {};
	SystemParametersInfo(SPI_GETWORKAREA, 0, &work, 0);

	MONITORINFO minfo = {};
	minfo.cbSize = sizeof(minfo);
	HMONITOR hmon = MonitorFromRect(&rc, MONITOR_DEFAULTTONEAREST);
	if (hmon && GetMonitorInfo(hmon, &minfo))
		work = minfo.rcWork;

	if (work.right <= work.left || work.bottom <= work.top) {
		work.left = 0;
		work.top = 0;
		work.right = GetSystemMetrics(SM_CXSCREEN);
		work.bottom = GetSystemMetrics(SM_CYSCREEN);
	}

	int workW = work.right - work.left;
	int workH = work.bottom - work.top;
	int width = rc.right - rc.left;
	int height = rc.bottom - rc.top;

	if (width < Dpi::Scale(320)) width = Dpi::Scale(320);
	if (height < Dpi::Scale(240)) height = Dpi::Scale(240);
	if (width > workW) width = workW;
	if (height > workH) height = workH;

	RECT out = rc;
	out.right = out.left + width;
	out.bottom = out.top + height;

	if (out.right > work.right) {
		out.left = work.right - width;
		out.right = work.right;
	}
	if (out.left < work.left) {
		out.left = work.left;
		out.right = work.left + width;
	}
	if (out.bottom > work.bottom) {
		out.top = work.bottom - height;
		out.bottom = work.bottom;
	}
	if (out.top < work.top) {
		out.top = work.top;
		out.bottom = work.top + height;
	}

	return out;
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

void MainWnd::UpdateRecentFilesMenu()
{
	HMENU hMain = m_menu.handle();
	if (!hMain)
		return;

	if (!m_recentMenu)
		m_recentMenu = FindSubMenuWith(hMain, ID_FILE_RECENT_EMPTY);
	if (!m_recentMenu)
		return;

	while (GetMenuItemCount(m_recentMenu) > 0)
		DeleteMenu(m_recentMenu, 0, MF_BYPOSITION);

	if (m_recentFiles.count() == 0) {
		AppendMenuW(m_recentMenu, MF_GRAYED, ID_FILE_RECENT_EMPTY, L"(Empty)");
	} else {
		int n = std::min(m_recentFiles.count(), (int)RecentFileRegistry::MAX_ITEMS);
		for (int i = 0; i < n; i++) {
			std::wstring text = m_recentFiles.DisplayText(i);
			AppendMenuW(m_recentMenu, MF_STRING, ID_FILE_RECENT_FIRST + i, text.c_str());
		}
	}
	DrawMenuBar(m_hwnd);
}

void MainWnd::SaveAs()
{
	if (m_doc.getPath().empty())
		return;

	wchar_t path[MAX_PATH] = {};
	int idx = m_doc.getDirIdx();
	if (idx >= 0) {
		wcsncpy(path, m_doc.getDirFile(idx).c_str(), MAX_PATH - 1);
		path[MAX_PATH - 1] = L'\0';
	}

	std::wstring filter = L"Original File";
	filter.push_back(L'\0');
	const wchar_t *dot = wcsrchr(path, L'.');
	if (dot && *(dot + 1)) {
		filter += L"*";
		filter += dot;
	} else {
		filter += L"*.*";
	}
	filter.push_back(L'\0');
	filter += L"All Files";
	filter.push_back(L'\0');
	filter += L"*.*";
	filter.push_back(L'\0');

	OPENFILENAMEW ofn = {};
	ofn.lStructSize  = sizeof(ofn);
	ofn.hwndOwner    = m_hwnd;
	ofn.lpstrFilter  = filter.c_str();
	ofn.lpstrFile    = path;
	ofn.nMaxFile     = MAX_PATH;
	ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
	if (!GetSaveFileNameW(&ofn))
		return;

	if (m_doc.saveAsLocal(path) != 0)
		MessageBoxW(m_hwnd, L"Could not save the file.",
					L"Save As Failed", MB_OK | MB_ICONERROR);
	SetFocus(m_mainView.imageView().hwnd());
}

void MainWnd::OpenRecentFile(UINT id)
{
	int idx = (int)(id - ID_FILE_RECENT_FIRST);
	if (idx < 0 || idx >= m_recentFiles.count())
		return;

	const RecentFileRegistry::Entry &entry = m_recentFiles.entry(idx);
	int res = IM_FAIL;
	if (entry.type == RecentFileRegistry::Entry::LOCAL) {
		res = m_doc.open(std::shared_ptr<UniFs>(LocalFs::open()),
						 entry.path.c_str(), -1, true, INT_MIN);
	} else {
		auto client = RemoteFs::open(entry.host.c_str(), entry.port);
		if (client) {
			res = m_doc.open(std::shared_ptr<UniFs>(client.release()),
						 entry.path.c_str(), -1, true, INT_MIN);
		} else {
			MessageBoxW(m_hwnd, L"Could not connect to the remote server.",
					   L"Connection Failed", MB_OK | MB_ICONERROR);
		}
	}

	if (res == IM_OK) {
		if (entry.type == RecentFileRegistry::Entry::LOCAL)
			m_recentFiles.AddLocal(entry.path);
		else
			m_recentFiles.AddRemote(entry.host, entry.port, entry.path);
		UpdateRecentFilesMenu();
		SetFocus(m_mainView.imageView().hwnd());
	}
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
	ClearCommandMenuBitmaps(m_menu.handle());

	for (HBITMAP hb : m_menuBitmaps) DeleteObject(hb);
	m_menuBitmaps.clear();
}

void MainWnd::UpdateMenuIcons()
{
	ClearMenuBitmaps();

	int sz = GetSystemMetrics(SM_CYSMICON);

	HMENU hMain = m_menu.handle();
	UpdateCommandMenuBitmaps(hMain, m_toolbar, sz, m_menuBitmaps);

	HMENU hZoomSub = FindSubMenuWith(hMain, ID_ZOOMMODE_W2I_ZOOMOUT);
	HBITMAP hbmpZoom = m_toolbar.GetMenuIcon(ID_VIEW_ZOOMMODE, sz);
	if (hbmpZoom && SetSubMenuBitmap(hMain, hZoomSub, hbmpZoom))
		m_menuBitmaps.push_back(hbmpZoom);
	else if (hbmpZoom)
		DeleteObject(hbmpZoom);

	HBITMAP hbmpRecent = Toolbar::GetShellMenuIcon(86, sz);
	if (hbmpRecent && SetSubMenuBitmap(hMain, m_recentMenu, hbmpRecent))
		m_menuBitmaps.push_back(hbmpRecent);
	else if (hbmpRecent)
		DeleteObject(hbmpRecent);
}


void MainWnd::UpdateButtonStates()
{
	int idx = m_doc.getDirIdx();
	int cnt = m_doc.getDirCount();
	bool canPrev   = idx > 0;
	bool canNext   = idx >= 0 && idx < cnt - 1;
	bool canSaveAs = !m_doc.getPath().empty();
	bool canZoomIn  = m_mainView.imageView().zoom( 1, true) == 0;
	bool canZoomOut = m_mainView.imageView().zoom(-1, true) == 0;

	m_toolbar.EnableButton(ID_FILE_SAVE_AS,  canSaveAs);
	m_toolbar.EnableButton(ID_FILE_PREV,     canPrev);
	m_toolbar.EnableButton(ID_FILE_NEXT,     canNext);
	m_toolbar.EnableButton(ID_VIEW_ZOOMIN,   canZoomIn);
	m_toolbar.EnableButton(ID_VIEW_ZOOMOUT,  canZoomOut);
	m_toolbar.EnableButton(ID_VIEW_ZOOMMODE, true);

	m_menu.EnableItem(ID_FILE_SAVE_AS, canSaveAs);
	m_menu.EnableItem(ID_FILE_PREV,    canPrev);
	m_menu.EnableItem(ID_FILE_NEXT,    canNext);
	m_menu.EnableItem(ID_VIEW_ZOOMIN,  canZoomIn);
	m_menu.EnableItem(ID_VIEW_ZOOMOUT, canZoomOut);
}

