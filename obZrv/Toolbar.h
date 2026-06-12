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
#include <commctrl.h>

/* Toolbar control wrapper.
 *
 * The toolbar lives inside a "band" container window (obzToolBand) that
 * provides a COLOR_BTNFACE background.  The band forwards WM_COMMAND and
 * WM_NOTIFY to its parent (MainWnd) so button clicks and tooltips work
 * as if the toolbar were a direct child of MainWnd.
 *
 * hwnd()       returns the band HWND (use this for layout in MainWnd).
 * bandHeight() returns the band height as determined in UpdateDpi().
 *
 * Icons are loaded from RCDATA PNG strips (4 DPI tiers: 96/120/144/192).
 * The strip has 9 buttons (open, prev, next, zoomin, -, zoomout, ...).
 * A folder icon extracted from shell32.dll is appended for FileList toggle.
 */
class Toolbar
{
public:
	Toolbar();
	~Toolbar();

	bool Create(HWND hParent, HINSTANCE hInst);

	/* Call after DPI changes to rebuild icons at the new scale. */
	void UpdateDpi();

	/* Call after the parent receives WM_SIZE. */
	void AutoSize();

	/* Toggle the pressed/checked state of a BTNS_CHECK button. */
	void SetPressed(UINT cmdId, bool pressed);

	/* Enable or disable a button. */
	void EnableButton(UINT cmdId, bool enable);

	/* Returns the band container HWND. */
	HWND hwnd()       const { return m_hBand; }

	/* Returns the actual HWND of the toolbar control (for hit-testing). */
	HWND toolbarHwnd() const { return m_hwnd; }

	/* Height of the band, set once per UpdateDpi() call. */
	int  bandHeight() const { return m_bandH; }

	/* Return a sz x sz HBITMAP for use as a menu icon (caller owns it). */
	HBITMAP GetMenuIcon(UINT cmdId, int sz) const;

	/* Return a sz x sz shell32 icon bitmap for use as a menu icon (caller owns it). */
	static HBITMAP GetShellMenuIcon(int idx, int sz);

private:
	HWND       m_hBand;
	HWND       m_hwnd;
	HINSTANCE  m_hInst;
	HIMAGELIST m_himl;
	HIMAGELIST m_himlDis;
	int        m_bandH;

	static void RegisterBandClass(HINSTANCE hInst);
	static LRESULT CALLBACK BandProc(HWND, UINT, WPARAM, LPARAM);

	/* Load a PNG strip from RCDATA and build a normal + disabled ILC_COLOR32
	 * image list pair.  Disabled = grayscale + 50 % alpha.
	 * Returns the normal list (NULL on failure); *pHimlDis receives disabled list. */
	HIMAGELIST LoadPngStrip(int resId, int iconSz, HIMAGELIST *pHimlDis) const;

	/* Extract an HICON from shell32.dll at the given 0-based index. */
	static HICON LoadShell32Icon(int idx, int size);
};
