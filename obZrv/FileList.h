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

	/* Repopulate the ListView from doc's current directory/index. */
	void refresh();

protected:
	LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
	Doc  *_doc    = NULL;
	HWND  m_hList = NULL;   /* child ListView control */

	void UpdateListSize();
	void OnActivateItem(int idx);
};
