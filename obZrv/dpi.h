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

/* WM_DPICHANGED was added in Windows 8.1 (0x0602).
 * Define it ourselves so the code compiles with WINVER=0x0501. */
#ifndef WM_DPICHANGED
#  define WM_DPICHANGED 0x02E0
#endif

/* DPI helper.
 *
 * Call Dpi::Update(NULL) once at startup (uses the screen DC as fallback),
 * then Dpi::Update(hwnd) after the main window is created, and again
 * inside the WM_DPICHANGED handler.
 *
 * All values passed to Scale() are assumed to be at the 96-DPI baseline. */
class Dpi
{
public:
	/* Call once at startup (before any window is created) to set the process
	 * DPI awareness level.  Tries PerMonitorV2 -> PerMonitor -> DPIAware in
	 * order, using runtime GetProcAddress so the binary still runs on WinXP. */
	static void EnableAwareness();

	/* Refresh the cached DPI from hwnd (or the screen DC if hwnd is NULL). */
	static void Update(HWND hwnd);

	/* Scale a baseline-96 pixel value to the current DPI, rounding to nearest. */
	static int Scale(int px) { return (px * s_dpi + 48) / 96; }

	/* Raw DPI value (96 = 100%). */
	static int Get() { return s_dpi; }

private:
	static int s_dpi;
};
