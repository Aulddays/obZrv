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
#include "MainView.h"

MainView::MainView()
	: m_hInst(NULL), m_showFileList(true),
	  m_splitX(150), m_dragging(false),
	  m_dragStartX(0), m_dragStartSplitX(0),
	  m_splitHover(false), m_trackingLeave(false)
{
}

bool MainView::Create(HWND hParent, HINSTANCE hInst)
{
	m_hInst        = hInst;
	m_showFileList = true;
	m_splitX       = Dpi::Scale(150);
	m_dragging     = false;

	WNDCLASS wc      = {};
	wc.lpfnWndProc   = WndBase::WndProcStatic;
	wc.hInstance     = hInst;
	wc.hbrBackground = NULL; /* we handle WM_ERASEBKGND ourselves */
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = MAINVIEW_CLASS;
	RegisterClass(&wc);

	if (!CreateWindowEx(
			0, MAINVIEW_CLASS, NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			0, 0, 0, 0,
			hParent, NULL, hInst, this))
		return false;

	return m_fileList.Create(m_hwnd, hInst) &&
		   m_imageView.Create(m_hwnd, hInst, hParent);
}

void MainView::Layout(int x, int y, int w, int h)
{
	if (!m_hwnd) return;
	SetWindowPos(m_hwnd, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
	UpdateLayout();
}

void MainView::ShowFileList(bool show)
{
	m_showFileList = show;
	ShowWindow(m_fileList.hwnd(), show ? SW_SHOW : SW_HIDE);
	UpdateLayout();
	InvalidateRect(m_hwnd, NULL, TRUE);
}

/* Reposition child windows to match the current split position. */
void MainView::UpdateLayout()
{
	if (!m_hwnd) return;

	RECT rc;
	GetClientRect(m_hwnd, &rc);
	int w = rc.right;
	int h = rc.bottom;

	int contentX = 0;
	if (m_showFileList && m_fileList.hwnd()) {
		SetWindowPos(m_fileList.hwnd(), NULL,
					 0, 0, m_splitX, h,
					 SWP_NOZORDER | SWP_NOACTIVATE);
		contentX = m_splitX + splitW();
	}

	if (m_imageView.hwnd()) {
		int contentW = w - contentX;
		if (contentW < 0) contentW = 0;
		m_imageView.Layout(contentX, 0, contentW, h);
	}

	/* Invalidate the splitter strip so it repaints */
	if (m_showFileList) {
		RECT dirty = {m_splitX, 0, contentX, h};
		InvalidateRect(m_hwnd, &dirty, TRUE);
	}
}

LRESULT MainView::HandleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
	case WM_SIZE:
		UpdateLayout();
		return 0;

	case WM_ERASEBKGND:
		return OnEraseBkgnd((HDC)wp);

	case WM_LBUTTONDOWN:
		OnLButtonDown(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lp), GET_Y_LPARAM(lp));
		return 0;

	case WM_MOUSELEAVE:
		OnMouseLeave();
		return 0;

	case WM_LBUTTONUP:
		OnLButtonUp();
		return 0;

	case WM_MOUSEWHEEL:
		// Forward to ImageView if it exists; otherwise ignore
		if (m_imageView.hwnd())
			SendMessage(m_imageView.hwnd(), WM_MOUSEWHEEL, wp, lp);
		return 0;

	case WM_SETCURSOR:
		if (OnSetCursor((HWND)wp))
			return TRUE;   /* prevent DefWindowProc from resetting the cursor */
		break;
	}
	return WndBase::HandleMessage(msg, wp, lp);
}

/* Paint the splitter strip and the content area (white). */
BOOL MainView::OnEraseBkgnd(HDC hdc)
{
	RECT rc;
	GetClientRect(m_hwnd, &rc);

	if (m_showFileList) {
		int color = m_splitHover ? COLOR_BTNSHADOW : COLOR_BTNFACE;
		RECT split = {m_splitX, 0, m_splitX + splitW(), rc.bottom};
		FillRect(hdc, &split, GetSysColorBrush(color));

		RECT right = {m_splitX + splitW(), 0, rc.right, rc.bottom};
		FillRect(hdc, &right, GetSysColorBrush(COLOR_WINDOW));
	} else {
		FillRect(hdc, &rc, GetSysColorBrush(COLOR_WINDOW));
	}
	return TRUE;
}

bool MainView::IsOnSplitter(int x) const
{
	return m_showFileList && x >= m_splitX && x < m_splitX + splitW();
}

void MainView::OnLButtonDown(int x, int y)
{
	(void)y;
	if (!IsOnSplitter(x)) return;

	m_dragging      = true;
	m_dragStartX    = x;
	m_dragStartSplitX = m_splitX;
	SetCapture(m_hwnd);
}

void MainView::OnMouseMove(int x, int y)
{
	(void)y;

	/* Update splitter hover state */
	bool onSplit = IsOnSplitter(x);
	if (onSplit != m_splitHover) {
		m_splitHover = onSplit;
		InvalidateSplitter();
	}

	/* Request WM_MOUSELEAVE so we can clear hover when cursor exits */
	if (!m_trackingLeave) {
		TRACKMOUSEEVENT tme = {};
		tme.cbSize    = sizeof(tme);
		tme.dwFlags   = TME_LEAVE;
		tme.hwndTrack = m_hwnd;
		TrackMouseEvent(&tme);
		m_trackingLeave = true;
	}

	if (!m_dragging) return;

	RECT rc;
	GetClientRect(m_hwnd, &rc);

	int newSplit = m_dragStartSplitX + (x - m_dragStartX);
	int maxSplit = m_showFileList ? (rc.right - splitW() - 50) : splitMax();
	if (maxSplit > splitMax()) maxSplit = splitMax();
	if (newSplit < splitMin()) newSplit = splitMin();
	if (newSplit > maxSplit)   newSplit = maxSplit;

	if (newSplit != m_splitX) {
		m_splitX = newSplit;
		UpdateLayout();
	}
}

void MainView::OnMouseLeave()
{
	m_trackingLeave = false;
	if (m_splitHover) {
		m_splitHover = false;
		InvalidateSplitter();
	}
}

void MainView::InvalidateSplitter()
{
	if (!m_hwnd || !m_showFileList) return;
	RECT rc;
	GetClientRect(m_hwnd, &rc);
	RECT split = {m_splitX, 0, m_splitX + splitW(), rc.bottom};
	InvalidateRect(m_hwnd, &split, TRUE);
}

void MainView::OnLButtonUp()
{
	if (m_dragging) {
		m_dragging = false;
		ReleaseCapture();

		/* After capture release, cursor may be over a child window.
		 * Re-evaluate hover state immediately to avoid it getting stuck. */
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(m_hwnd, &pt);

		bool onSplit = IsOnSplitter(pt.x);
		if (onSplit != m_splitHover) {
			m_splitHover = onSplit;
			InvalidateSplitter();
		}

		/* Re-register TME_LEAVE if cursor is still within our client area */
		if (!m_trackingLeave) {
			RECT rc;
			GetClientRect(m_hwnd, &rc);
			if (PtInRect(&rc, pt)) {
				TRACKMOUSEEVENT tme = {};
				tme.cbSize    = sizeof(tme);
				tme.dwFlags   = TME_LEAVE;
				tme.hwndTrack = m_hwnd;
				TrackMouseEvent(&tme);
				m_trackingLeave = true;
			}
		}
	}
}

bool MainView::OnSetCursor(HWND hwndHit)
{
	if (hwndHit != m_hwnd) return false;  /* over a child window */

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(m_hwnd, &pt);

	if (IsOnSplitter(pt.x)) {
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		return true;
	}
	return false;
}
