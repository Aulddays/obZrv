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
#include "Menu.h"

Menu::Menu()
	: m_hMenu(NULL), m_attached(false)
{
}

Menu::~Menu()
{
	/* If never attached to a window, we own the menu and must destroy it. */
	if (m_hMenu && !m_attached)
		DestroyMenu(m_hMenu);
}

bool Menu::Load(HINSTANCE hInst, UINT resId)
{
	m_hMenu = LoadMenu(hInst, MAKEINTRESOURCE(resId));
	return m_hMenu != NULL;
}

void Menu::Attach(HWND hwnd)
{
	if (m_hMenu) {
		SetMenu(hwnd, m_hMenu);
		m_attached = true;
	}
}

void Menu::SetChecked(UINT cmdId, bool checked)
{
	if (!m_hMenu)
		return;
	CheckMenuItem(m_hMenu, cmdId,
				  MF_BYCOMMAND | (checked ? MF_CHECKED : MF_UNCHECKED));
}

void Menu::EnableItem(UINT cmdId, bool enable)
{
	if (!m_hMenu)
		return;
	EnableMenuItem(m_hMenu, cmdId,
				   MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
}
