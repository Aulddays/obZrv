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
#include "StatusBar.h"

/* Width reserved for the hint part (part 1). Clamped to reasonable bounds. */
static const int HINT_PART_WIDTH = 200;

StatusBar::StatusBar()
	: m_hwnd(NULL)
{
}

bool StatusBar::Create(HWND hParent, HINSTANCE hInst)
{
	m_hwnd = CreateWindowEx(
		0, STATUSCLASSNAME, NULL,
		WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
		0, 0, 0, 0,
		hParent, NULL, hInst, NULL);
	if (!m_hwnd) return false;

	UpdateParts();
	return true;
}

void StatusBar::UpdateParts()
{
	if (!m_hwnd) return;
	RECT rc = {};
	GetClientRect(m_hwnd, &rc);
	int total = rc.right;
	int hintW = HINT_PART_WIDTH;
	if (hintW > total / 2) hintW = total / 2;  /* never take more than half */
	int parts[2] = { total - hintW, -1 };
	SendMessage(m_hwnd, SB_SETPARTS, 2, (LPARAM)parts);
}

void StatusBar::OnParentSize()
{
	if (!m_hwnd) return;
	SendMessage(m_hwnd, WM_SIZE, 0, 0);
	UpdateParts();
}

void StatusBar::SetFileInfo(LPCTSTR text)
{
	if (m_hwnd)
		SendMessage(m_hwnd, SB_SETTEXT, 0, (LPARAM)text);
}

void StatusBar::SetHint(LPCTSTR text)
{
	if (m_hwnd)
		SendMessage(m_hwnd, SB_SETTEXT, 1, (LPARAM)text);
}

int StatusBar::height() const
{
	if (!m_hwnd) return 0;
	RECT rc = {};
	GetClientRect(m_hwnd, &rc);
	return rc.bottom;
}
