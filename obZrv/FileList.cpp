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
#include <shlwapi.h>
#include "FileList.h"
#include "Doc.h"

/* Subclass proc for the ListView: grab focus on mouse-down before any
 * notification (LVN_ITEMCHANGED) fires, so a W2I-mode window resize
 * triggered by the resulting file open cannot steal focus back. */
static LRESULT CALLBACK ListViewProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp,
									 UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/)
{
	if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN)
		SetFocus(hWnd);
	if (msg == WM_NCDESTROY)
		RemoveWindowSubclass(hWnd, ListViewProc, 0);
	return DefSubclassProc(hWnd, msg, wp, lp);
}

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

	SetWindowSubclass(m_hList, ListViewProc, 0, 0);

	return true;
}

void FileList::setDoc(Doc *doc)
{
	_doc = doc;
}

void FileList::rebuild()
{
	if (!m_hList) return;

	_rebuilding = true;
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
	_rebuilding = false;
}

void FileList::moveSelection(int oldIdx, int newIdx)
{
	if (!m_hList || !_doc) return;

	_rebuilding = true;
	if (oldIdx >= 0)
		ListView_SetItemState(m_hList, oldIdx, 0, LVIS_SELECTED | LVIS_FOCUSED);

	if (newIdx >= 0 && newIdx < _doc->getDirCount())
	{
		ListView_SetItemState(m_hList, newIdx,
			LVIS_SELECTED | LVIS_FOCUSED,
			LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(m_hList, newIdx, FALSE);
	}
	_rebuilding = false;
}

void FileList::removeItem(int delIdx, int newSelIdx)
{
	if (!m_hList || !_doc) return;

	_rebuilding = true;
	ListView_DeleteItem(m_hList, delIdx);

	if (newSelIdx >= 0 && newSelIdx < _doc->getDirCount())
	{
		ListView_SetItemState(m_hList, newSelIdx,
			LVIS_SELECTED | LVIS_FOCUSED,
			LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(m_hList, newSelIdx, FALSE);
	}
	_rebuilding = false;
}

void FileList::smoothRebuild(const std::vector<std::wstring> &oldFiles)
{
	if (!m_hList || !_doc)
		return;

	_rebuilding = true;

	int newCount = _doc->getDirCount();
	int oldCount = (int)oldFiles.size();

	/* Two-pointer merge over two sorted lists (StrCmpLogicalW order).
	 * lv_idx tracks the live row position in the ListView as we apply changes. */
	int lv_idx = 0;
	int oi = 0, ni = 0;
	while (oi < oldCount || ni < newCount)
	{
		int cmp;
		if      (oi >= oldCount) cmp =  1;   /* old exhausted: insert new */
		else if (ni >= newCount) cmp = -1;   /* new exhausted: delete old */
		else
			cmp = StrCmpLogicalW(oldFiles[oi].c_str(), _doc->getDirFile(ni).c_str());

		if (cmp == 0)
		{
			/* Same name in both lists: row stays, advance both pointers */
			lv_idx++; oi++; ni++;
		}
		else if (cmp < 0)
		{
			/* Old item not present in new list: remove the row */
			ListView_DeleteItem(m_hList, lv_idx);
			oi++;   /* lv_idx stays -- next old item shifted up */
		}
		else
		{
			/* New item not present in old list: insert before current row */
			LVITEM item  = {};
			item.mask    = LVIF_TEXT;
			item.iItem   = lv_idx;
			item.pszText = const_cast<wchar_t *>(_doc->getDirFile(ni).c_str());
			ListView_InsertItem(m_hList, &item);
			lv_idx++; ni++;
		}
	}

	/* Update selection to current file */
	int idx = _doc->getDirIdx();
	if (idx >= 0 && idx < newCount)
	{
		ListView_SetItemState(m_hList, idx,
			LVIS_SELECTED | LVIS_FOCUSED,
			LVIS_SELECTED | LVIS_FOCUSED);
		ListView_EnsureVisible(m_hList, idx, FALSE);
	}

	_rebuilding = false;
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

void FileList::OnActivateItem(int idx, bool notify)
{
	if (!_doc || idx < 0 || idx >= _doc->getDirCount())
		return;
	std::string path = _doc->getDir() + "/" + wstr_to_utf8(_doc->getDirFile(idx).c_str());
	if (path == _doc->getPath())
		return;
	std::string targetPath = path;
	_doc->open(nullptr, path.c_str(), -1, false, +1);
	if (notify && _doc->getPath() != targetPath)
		MessageBoxW(m_hwnd, (L"Cannot open: " + utf8_to_wstr(targetPath.c_str())).c_str(),
					L"Error", MB_OK | MB_ICONWARNING);
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
			if (hdr->code == LVN_ITEMCHANGED)
			{
				/* Open when selection changes via keyboard or mouse navigation */
				NMLISTVIEW *nmlv = reinterpret_cast<NMLISTVIEW *>(lp);
				if (!_rebuilding &&
					(nmlv->uChanged & LVIF_STATE) &&
					(nmlv->uNewState & LVIS_SELECTED) &&
					!(nmlv->uOldState & LVIS_SELECTED))
				{
					bool byMouse = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
					OnActivateItem(nmlv->iItem, byMouse);
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
