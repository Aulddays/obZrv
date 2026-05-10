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
#include <windowsx.h>
#include "WndBase.h"
#include "FileList.h"
#include "ImageView.h"
#include "dpi.h"

#define MAINVIEW_CLASS TEXT("obzMainView")

/* MainView - occupies the area between toolbar and status bar.
 *
 * Layout (when file list is visible):
 *   [ FileList | splitter | content area ]
 *
 * The splitter is a SPLIT_W-pixel-wide strip drawn by MainView itself.
 * Dragging the splitter resizes the file list panel. */
class MainView : public WndBase
{
public:
	MainView();
	bool Create(HWND hParent, HINSTANCE hInst);

	/* Reposition and resize MainView within the parent client area. */
	void Layout(int x, int y, int w, int h);

	/* Show or hide the file list panel and splitter. */
	void ShowFileList(bool show);

	/* Access the image view for Doc/command wiring. */
	ImageView &imageView() { return m_imageView; }

	/* Access the file list panel for Doc wiring. */
	FileList &fileList() { return m_fileList; }

protected:
	LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
	HINSTANCE m_hInst;
	FileList  m_fileList;
	ImageView m_imageView;
	bool      m_showFileList;
	int       m_splitX;      /* current file list width */
	bool      m_dragging;
	int       m_dragStartX;  /* cursor x when drag began */
	int       m_dragStartSplitX; /* m_splitX when drag began */
	bool      m_splitHover;  /* mouse is over the splitter */
	bool      m_trackingLeave; /* TrackMouseEvent(TME_LEAVE) is active */

	static const int SPLIT_W_BASE   = 6;   /* splitter strip width at 96 dpi  */
	static const int SPLIT_MIN_BASE = 80;  /* minimum file list width at 96 dpi */
	static const int SPLIT_MAX_BASE = 600; /* maximum file list width at 96 dpi */
	int splitW()   const { return Dpi::Scale(SPLIT_W_BASE); }
	int splitMin() const { return Dpi::Scale(SPLIT_MIN_BASE); }
	int splitMax() const { return Dpi::Scale(SPLIT_MAX_BASE); }

	void UpdateLayout();
	void InvalidateSplitter();
	bool IsOnSplitter(int x) const;

	BOOL OnEraseBkgnd(HDC hdc);
	void OnLButtonDown(int x, int y);
	void OnMouseMove(int x, int y);
	void OnLButtonUp();
	void OnMouseLeave();
	bool OnSetCursor(HWND hwndHit);  /* returns true if cursor was set */
};
