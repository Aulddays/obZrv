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
#include <string>
#include <vector>
#include "WndBase.h"
#include "Menu.h"
#include "Toolbar.h"
#include "StatusBar.h"
#include "MainView.h"
#include "Doc.h"
#include "dpi.h"
#include "strutil.h"
#include "ConnectDlg.h"
#include "RemoteBrowserDlg.h"
#include "RecentFileRegistry.h"
#include "SlideShowDlg.h"

/* Name of the main window class registered with Windows */
#define MAINWND_CLASS TEXT("obZrvMain")

/* MainWnd - the application's top-level window.
 * Owns the menu, toolbar, status bar and main view (added in M4).
 * Handles layout on WM_SIZE and exits the message loop on WM_DESTROY. */
class MainWnd : public WndBase
{
public:
	bool Create(HINSTANCE hInst, int nShow);

	/* Open an image file (called from App::Run for command-line / drag-drop). */
	void OpenFile(const wchar_t *path);

protected:
	LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
	HINSTANCE m_hInst;
	Menu      m_menu;
	Toolbar   m_toolbar;
	StatusBar m_statusBar;
	MainView  m_mainView;
	Doc       m_doc;
	RecentFileRegistry m_recentFiles;
	HMENU     m_recentMenu;
	bool      m_fileListVisible;
	int       m_zoomMode;   /* active ID_ZOOMMODE_* value */
	std::vector<HBITMAP> m_menuBitmaps;  /* owned menu icon bitmaps */

	bool      m_slideShowActive;
	bool      m_slideShowFullscreen;
	bool      m_slideShowOpening;
	bool      m_savedToolbarVisible;
	bool      m_savedStatusBarVisible;
	bool      m_savedFileListVisible;
	UINT_PTR  m_slideShowTimer;
	SlideShowDlg::Options m_slideShowOptions;
	std::vector<std::string> m_slideShowFiles;
	int       m_slideShowIndex;
	int       m_slideShowRound;
	DWORD     m_savedStyle;
	DWORD     m_savedExStyle;
	WINDOWPLACEMENT m_savedPlacement;

	bool OnCreate();
	void OnDestroy();
	void OnSize(int cx, int cy);
	void OnCommand(UINT id);
	void OnMenuSelect(UINT id, UINT flags);
	void OnNotify(LPARAM lp);
	void OnDpiChanged(const RECT *pRect);
	bool LoadWindowPlacement(RECT *rc, bool *maximized);
	void SaveWindowPlacement();
	RECT ClampWindowRectToNearestWorkArea(const RECT &rc);
	void UpdateButtonStates();
	void UpdateRecentFilesMenu();
	void OpenRecentFile(UINT id);
	void SaveAs();
	void ShowZoomModeMenu();
	void ShowSlideShowDialog();
	void StartSlideShow(const SlideShowDlg::Options &options);
	void StopSlideShow();
	void ResetSlideShowTimer();
	void SyncSlideShowIndexToCurrent();
	bool OpenSlideShowCurrent();
	bool StepSlideShowIndex();
	void AdvanceSlideShow();
	void BuildSlideShowFiles();
	void ShuffleSlideShowFiles();
	void EnterSlideShowFullscreen();
	void LeaveSlideShowFullscreen();
	void UpdateMenuIcons();    /* set/refresh menu item bitmaps from toolbar image list */
	void ClearMenuBitmaps();   /* remove bitmaps from menu items and free them */
};
