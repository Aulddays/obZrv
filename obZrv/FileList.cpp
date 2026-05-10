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
#include "FileList.h"
#include "Doc.h"

bool FileList::Create(HWND hParent, HINSTANCE hInst)
{
	WNDCLASS wc      = {};
	wc.lpfnWndProc   = WndBase::WndProcStatic;
	wc.hInstance     = hInst;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = FILELIST_CLASS;
	RegisterClass(&wc);

	if (!CreateWindowEx(
			0, FILELIST_CLASS, NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
			0, 0, 0, 0,
			hParent, NULL, hInst, this))
		return false;

	/* ListView child -- fills the whole panel */
	m_hList = CreateWindowEx(
		0, WC_LISTVIEW, NULL,
		WS_CHILD | WS_VISIBLE |
			LVS_REPORT | LVS_NOCOLUMNHEADER |
			LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, 0, 0, 0,
		m_hwnd, NULL, hInst, NULL);
	if (!m_hList)
		return false;

	/* Full-row highlight */
	ListView_SetExtendedListViewStyle(m_hList, LVS_EX_FULLROWSELECT);

	/* Single column (no header text needed) */
	LVCOLUMN col = {};
	col.mask = LVCF_WIDTH;
	col.cx   = 150;
	ListView_InsertColumn(m_hList, 0, &col);

	return true;
}

void FileList::setDoc(Doc *doc)
{
	_doc = doc;
}

void FileList::refresh()
{
	if (!m_hList) return;

	/* Temporarily suppress LVN_ITEMCHANGED while we rebuild */
	ListView_DeleteAllItems(m_hList);

	if (!_doc || _doc->getDirCount() == 0)
		return;

	int count = _doc->getDirCount();
	for (int i = 0; i < count; i++)
	{
		LVITEM item  = {};
		item.mask    = LVIF_TEXT;
		item.iItem   = i;
		item.pszText = const_cast<wchar_t *>(_doc->getDirFile(i).c_str());
		ListView_InsertItem(m_hList, &item);
	}

	int idx = _doc->getDirIdx();
	if (idx >= 0 && idx < count)
	{
		ListView_SetItemState(m_hList, idx,
			LVIS_SELECTED | LVIS_FOCUSED,
			LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(m_hList, idx, FALSE);
	}

	UpdateListSize();
}

void FileList::UpdateListSize()
{
	if (!m_hwnd || !m_hList) return;

	RECT rc;
	GetClientRect(m_hwnd, &rc);
	SetWindowPos(m_hList, NULL, 0, 0, rc.right, rc.bottom,
				 SWP_NOZORDER | SWP_NOACTIVATE);

	/* Stretch the single column to fill the ListView width */
	RECT lrc;
	GetClientRect(m_hList, &lrc);
	if (lrc.right > 0)
		ListView_SetColumnWidth(m_hList, 0, lrc.right);
}

void FileList::OnActivateItem(int idx)
{
	if (!_doc || idx < 0 || idx >= _doc->getDirCount())
		return;

	std::wstring path = _doc->getDir() + L'/' + _doc->getDirFile(idx);
	if (path == _doc->getPath())
		return;
	_doc->open(path.c_str());
}

LRESULT FileList::HandleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg)
	{
	case WM_SIZE:
		UpdateListSize();
		return 0;

	case WM_NOTIFY:
	{
		NMHDR *hdr = reinterpret_cast<NMHDR *>(lp);
		if (hdr->hwndFrom == m_hList)
		{
			if (hdr->code == NM_CLICK)
			{
				NMITEMACTIVATE *nm = reinterpret_cast<NMITEMACTIVATE *>(lp);
				OnActivateItem(nm->iItem);
				return 0;
			}
			if (hdr->code == LVN_ITEMCHANGED)
			{
				/* Open when selection changes via keyboard navigation */
				NMLISTVIEW *nmlv = reinterpret_cast<NMLISTVIEW *>(lp);
				if ((nmlv->uChanged & LVIF_STATE) &&
					(nmlv->uNewState & LVIS_SELECTED) &&
					!(nmlv->uOldState & LVIS_SELECTED))
				{
					OnActivateItem(nmlv->iItem);
					return 0;
				}
			}
		}
		break;
	}

	case WM_SETFOCUS:
		/* Forward focus to the ListView so keyboard navigation works */
		if (m_hList)
			SetFocus(m_hList);
		return 0;
	}

	return WndBase::HandleMessage(msg, wp, lp);
}
