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
#include "App.h"
#include "MainWnd.h"
#include "resource.h"
#include "dpi.h"
#include <commctrl.h>
#include <gdiplus.h>

App::App(HINSTANCE hInst)
	: m_hInst(hInst), m_gdipToken(0)
{
	/* Enable PerMonitorV2 DPI awareness at runtime (belt-and-suspenders:
	 * the manifest already declares it, but this covers stripped manifests
	 * and older Windows fallbacks). Must run before any window is created. */
	Dpi::EnableAwareness();

	/* Enable visual styles (comctl32 v6) */
	INITCOMMONCONTROLSEX icc = {};
	icc.dwSize = sizeof(icc);
	icc.dwICC  = ICC_WIN95_CLASSES | ICC_BAR_CLASSES;
	InitCommonControlsEx(&icc);

	/* Seed DPI from the primary screen before any window is created.
	 * MainWnd::OnCreate will refine this with the actual window's monitor. */
	Dpi::Update(NULL);

	/* Initialize GDI+ for image loading */
	Gdiplus::GdiplusStartupInput gdipInput;
	Gdiplus::GdiplusStartup(&m_gdipToken, &gdipInput, NULL);
}

App::~App()
{
	if (m_gdipToken)
		Gdiplus::GdiplusShutdown(m_gdipToken);
}

int App::Run(int nShow, const wchar_t *initPath)
{
	MainWnd mainWnd;
	if (!mainWnd.Create(m_hInst, nShow))
		return 1;

	HACCEL hAccel = LoadAccelerators(m_hInst, MAKEINTRESOURCE(IDR_MAINMENU));

	if (initPath && *initPath)
		mainWnd.OpenFile(initPath);

	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0) > 0) {
		if (!hAccel || !TranslateAccelerator(mainWnd.hwnd(), hAccel, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	if (hAccel) DestroyAcceleratorTable(hAccel);
	return (int)msg.wParam;
}
