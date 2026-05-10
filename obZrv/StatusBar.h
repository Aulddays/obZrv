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

/* Status bar wrapper -- two parts:
 *   Part 0 (left, wider):  file info set by SetFileInfo()
 *   Part 1 (right, fixed): menu/toolbar hover hint set by SetHint() */
class StatusBar
{
public:
	StatusBar();

	bool Create(HWND hParent, HINSTANCE hInst);

	/* Forward the parent's WM_SIZE so the bar redraws and recalculates parts. */
	void OnParentSize();

	void SetFileInfo(LPCTSTR text);   /* band 0: image/file info */
	void SetHint(LPCTSTR text);       /* band 1: menu/toolbar hover hint */

	HWND hwnd()   const { return m_hwnd; }
	int  height() const;

private:
	HWND m_hwnd;

	void UpdateParts();  /* recalculate part widths to match current bar width */
};
