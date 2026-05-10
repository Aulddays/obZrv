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

/* Thin wrapper around HMENU for the main menu.
 * After Attach(), the window owns the menu and destroys it on close. */
class Menu
{
public:
	Menu();
	~Menu();

	bool Load(HINSTANCE hInst, UINT resId);

	/* Attach to a window via SetMenu(); window takes ownership. */
	void Attach(HWND hwnd);

	void SetChecked(UINT cmdId, bool checked);
	void EnableItem(UINT cmdId, bool enable);

	HMENU handle() const { return m_hMenu; }

private:
	HMENU m_hMenu;
	bool  m_attached;   /* true once SetMenu() has been called */
};
