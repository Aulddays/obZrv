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
#include "dpi.h"

int Dpi::s_dpi = 96;

void Dpi::Update(HWND hwnd)
{
	/* GetDpiForWindow - Windows 10+ */
	typedef UINT (WINAPI *PFN_GetDpiForWindow)(HWND);
	static PFN_GetDpiForWindow pfnW = (PFN_GetDpiForWindow)
		GetProcAddress(GetModuleHandle(TEXT("user32.dll")), "GetDpiForWindow");

	if (pfnW && hwnd) {
		s_dpi = (int)pfnW(hwnd);
		return;
	}

	/* GetDpiForMonitor - Windows 8.1+ (shcore.dll) */
	typedef HRESULT (WINAPI *PFN_GetDpiForMonitor)(HMONITOR, int, UINT *, UINT *);
	static PFN_GetDpiForMonitor pfnM = NULL;
	static bool s_triedShcore = false;
	if (!s_triedShcore) {
		s_triedShcore = true;
		HMODULE hShcore = LoadLibrary(TEXT("shcore.dll"));
		if (hShcore)
			pfnM = (PFN_GetDpiForMonitor)GetProcAddress(hShcore, "GetDpiForMonitor");
	}

	if (pfnM) {
		HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		UINT x = 96, y = 96;
		if (SUCCEEDED(pfnM(hMon, 0 /* MDT_EFFECTIVE_DPI */, &x, &y))) {
			s_dpi = (int)x;
			return;
		}
	}

	/* Fallback: GetDeviceCaps - always available, including WinXP */
	HDC hdc = GetDC(hwnd);
	s_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
	ReleaseDC(hwnd, hdc);
}

void Dpi::EnableAwareness()
{
	/* 1. SetProcessDpiAwarenessContext - Windows 10 1607+ (user32.dll)
	 *    DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = (HANDLE)-4 */
	typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(HANDLE);
	PFN_SetProcessDpiAwarenessContext pfnCtx =
		(PFN_SetProcessDpiAwarenessContext)GetProcAddress(
			GetModuleHandle(TEXT("user32.dll")),
			"SetProcessDpiAwarenessContext");
	if (pfnCtx) {
		pfnCtx((HANDLE)(INT_PTR)-4); /* DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 */
		return;
	}

	/* 2. SetProcessDpiAwareness - Windows 8.1+ (shcore.dll)
	 *    PROCESS_PER_MONITOR_DPI_AWARE = 2 */
	typedef HRESULT (WINAPI *PFN_SetProcessDpiAwareness)(int);
	HMODULE hShcore = GetModuleHandle(TEXT("shcore.dll"));
	if (!hShcore)
		hShcore = LoadLibrary(TEXT("shcore.dll"));
	if (hShcore) {
		PFN_SetProcessDpiAwareness pfnAware =
			(PFN_SetProcessDpiAwareness)GetProcAddress(hShcore,
													   "SetProcessDpiAwareness");
		if (pfnAware) {
			pfnAware(2); /* PROCESS_PER_MONITOR_DPI_AWARE */
			return;
		}
	}

	/* 3. SetProcessDPIAware - Vista/Win7 (user32.dll)
	 *    Marks the process as system-DPI-aware; no WM_DPICHANGED on these OSes. */
	typedef BOOL (WINAPI *PFN_SetProcessDPIAware)(void);
	PFN_SetProcessDPIAware pfnOld =
		(PFN_SetProcessDPIAware)GetProcAddress(
			GetModuleHandle(TEXT("user32.dll")), "SetProcessDPIAware");
	if (pfnOld)
		pfnOld();
	/* WinXP: no DPI awareness API; system always reports 96 DPI. */
}
