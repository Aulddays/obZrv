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
#include <vector>
#include <string>
#include "WndBase.h"

#define FILELIST_CLASS TEXT("obzFileList")

class Doc;

/* FileList - left-side panel containing a ListView of image files.
 *
 * After a Doc opens a file, call refresh() to repopulate the list and
 * highlight the current entry.  Double-clicking or pressing Enter on an
 * item opens that image via the Doc. */
class FileList : public WndBase
{
public:
	bool Create(HWND hParent, HINSTANCE hInst);

	/* Bind to a Doc; must be called before refresh(). */
	void setDoc(Doc *doc);

	/* Full rebuild: repopulate from doc's current directory/index.
	 * Use when the directory itself changes (new folder, new remote connection). */
	void rebuild();

	/* Selection-only update: swap the highlighted row without rebuilding.
	 * Use when navigating within the same directory. */
	void moveSelection(int oldIdx, int newIdx);

	/* Remove one row and update the highlight.
	 * Use after a single file has been deleted from the current directory. */
	void removeItem(int delIdx, int newSelIdx);

	/* Smooth merge update: apply minimal inserts/deletes to transition from
	 * oldFiles to the current doc directory.  Preserves scroll position.
	 * Use after a directory rescan while the panel is still showing the old list. */
	void smoothRebuild(const std::vector<std::wstring> &oldFiles);

	/* Alias for rebuild(); kept for compatibility. */
	void refresh() { rebuild(); }

protected:
	LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
	Doc  *_doc       = NULL;
	HWND  m_hList    = NULL;   /* child ListView control */
	bool  _rebuilding = false; /* suppress LVN_ITEMCHANGED during batch updates */

	void UpdateListSize();
	void OnActivateItem(int idx);
};
