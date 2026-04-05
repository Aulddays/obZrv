// obZrv
// https://github.com/Aulddays/obZrv
// 
// Copyright (c) 2020, 2021 Aulddays (https://dev.aulddays.com/). All rights reserved.
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

// ZView.cpp : implementation of the ObZrvView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "obZrv.h"
#endif

#include <algorithm>
#include <functional>
#include "ZDoc.h"
#include "ZView.h"
#include "Frame.h"
#include "../AulddaysDpiHelper/AulddaysDpiHelper.h"

#undef max
#undef min

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// ObZrvView

IMPLEMENT_DYNCREATE(ObZrvView, CView)

BEGIN_MESSAGE_MAP(ObZrvView, CView)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	ON_COMMAND(ID_VIEW_ZOOMIN, &ObZrvView::OnZoomIn)
	ON_COMMAND(ID_VIEW_ZOOMOUT, &ObZrvView::OnZoomOut)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMIN, &ObZrvView::OnUpdateZoomIn)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMOUT, &ObZrvView::OnUpdateZoomOut)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMTO, &ObZrvView::OnUpdateZoomTo)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMMODE, &ObZrvView::OnUpdateZoomTo)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_CREATE()
	ON_MESSAGE(WM_GESTURE, &ObZrvView::OnGesture)
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

// ObZrvView construction/destruction

ObZrvView::ObZrvView()
{
	// TODO: add construction code here

}

ObZrvView::~ObZrvView()
{
	releaseBitmap();
}

BOOL ObZrvView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.style &= ~(WS_BORDER);
	return CView::PreCreateWindow(cs);
}

// ObZrvView drawing

void ObZrvView::OnDraw(CDC* pDc)
{
	ObZrvDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	Image *image = pDoc ? pDoc->getImage() : NULL;
	if (!pDoc || !image)
	{
		fillBg(pDc);
		return;
	}

	CRect rect, crect;
	GetClientRect(rect);
	crect = rect;
	CSize size = { rect.Width(), rect.Height() };

	if (!_viewBitmap || size != _viewWndDim)		// need update viewBitmap
	{
		CRect oldViewCrop = _viewCrop;
		if (_zoomtype == ZT_FITIMAGE && _zoomlevel == 0)	// Fit image to view
		{
			if (size.cx >= image->getDimension().cx && size.cy >= image->getDimension().cy)	// View is larger than image
			{
				_fitlevel = 100;
				_scaleSize = CSize{ image->getDimension().cx, image->getDimension().cy };
			}
			else if ((uint64_t)size.cx * image->getDimension().cy > (uint64_t)size.cy * image->getDimension().cx)	// Fit on height
			{
				_fitlevel = size.cy * 100 / image->getDimension().cy;
				_scaleSize.cx = (LONG)((double)size.cy * image->getDimension().cx / image->getDimension().cy + 0.5);
				_scaleSize.cx = std::max(_scaleSize.cx, 1l);
				_scaleSize.cy = size.cy;
			}
			else	// Fit on width
			{
				assert((uint64_t)size.cx * image->getDimension().cy <= (uint64_t)size.cy * image->getDimension().cx);
				_fitlevel = size.cx * 100 / image->getDimension().cx;
				_scaleSize.cy = (LONG)((double)size.cx * image->getDimension().cy / image->getDimension().cx + 0.5);
				_scaleSize.cy = std::max(_scaleSize.cy, 1l);
				_scaleSize.cx = size.cx;
			}
			_viewCrop = CRect{ 0, 0, _scaleSize.cx, _scaleSize.cy };
		}
		else
		{
			assert(true); // TODO: Makesure: must have been a window size change or zooming
			_scaleSize.cx = std::max(image->getDimension().cx * _zoomlevel / 100, 1l);
			_scaleSize.cy = std::max(image->getDimension().cy * _zoomlevel / 100, 1l);
			if (rect.Width() >= _scaleSize.cx)
			{
				_viewCrop.left = 0;
				_viewCrop.right = _scaleSize.cx;
			}
			else
			{
				_viewCrop.left = (_scaleSize.cx - rect.Width()) / 2;
				_viewCrop.right = _viewCrop.left + rect.Width();
			}
			if (rect.Height() >= _scaleSize.cy)
			{
				_viewCrop.top = 0;
				_viewCrop.bottom = _scaleSize.cy;
			}
			else
			{
				_viewCrop.top = (_scaleSize.cy - rect.Height()) / 2;
				_viewCrop.bottom = _viewCrop.top + rect.Height();
			}
			// Adjust _viewCrop based on offsets
			if (_viewOffset.cx != 0 || _viewOffset.cy != 0 || _isdragging && (_draggingOrigin.x != _draggingCur.x || _draggingOrigin.y != _draggingCur.y))
			{
				CSize offset = { 0, 0 };
				if (_viewCrop.Width() < _scaleSize.cx)
				{
					offset.cx = _viewOffset.cx;	// base offset
					if (_isdragging)
						offset.cx -= _draggingCur.x - _draggingOrigin.x;	// dragging offset
					// limit offset to be within _viewScale
					if (_viewCrop.left + offset.cx < 0)
						offset.cx = -_viewCrop.left;
					if (_viewCrop.right + offset.cx > _scaleSize.cx)
						offset.cx = _scaleSize.cx - _viewCrop.right;
					_viewCrop.left += offset.cx;
					_viewCrop.right += offset.cx;
				}
				if (_viewCrop.Height() < _scaleSize.cy)
				{
					offset.cy = _viewOffset.cy;	// base offset
					if (_isdragging)
						offset.cy -= _draggingCur.y - _draggingOrigin.y;	// dragging offset
					// limit offset to be within _viewScale
					if (_viewCrop.top + offset.cy < 0)
						offset.cy = -_viewCrop.top;
					if (_viewCrop.bottom + offset.cy > _scaleSize.cy)
						offset.cy = _scaleSize.cy - _viewCrop.bottom;
					_viewCrop.top += offset.cy;
					_viewCrop.bottom += offset.cy;
				}
				if (!_isdragging && offset != _viewOffset)
					_viewOffset = offset;	// write back adjusted offset
			}
		}

		_viewWndDim = size;
		if (_viewCrop != oldViewCrop)
			releaseBitmap();
		if (!_viewBitmap)
		{
			DWORD tmstart = GetTickCount();
			//_viewBitmap = image->getBBitmap(_viewRect, _viewDim);
			_viewBitmap = image->getBBitmap(_scaleSize, _viewCrop);
			_timecost = GetTickCount() - tmstart;
		}
		updateStatus();
	}

	if (!_viewBitmap)
	{
		fillBg(pDc);
		return;
	}
	// calculate the output rect
	if (rect.Width() > _viewCrop.Width())
	{
		rect.left = (rect.Width() - _viewCrop.Width()) / 2;
		rect.right = rect.left + _viewCrop.Width();
	}
	if (rect.Height() > _viewCrop.Height())
	{
		rect.top = (rect.Height() - _viewCrop.Height()) / 2;
		rect.bottom = rect.top + _viewCrop.Height();
	}
	assert(_viewBitmap->Width() >= _viewCrop.Width() && _viewBitmap->Height() >= _viewCrop.Height());
	_viewBitmap->SetDIBitsToDevice(pDc->GetSafeHdc(), rect.left, rect.top, 0, 0, _viewCrop.Width(), _viewCrop.Height());
	if (rect.left > 0)
		pDc->FillSolidRect(0, 0, rect.left, crect.bottom, pDoc->getBgColor());
	if (rect.right != crect.right)
		pDc->FillSolidRect(rect.right, 0, crect.right - rect.right, crect.bottom, pDoc->getBgColor());
	if (rect.top != 0)
		pDc->FillSolidRect(rect.left, 0, rect.Width(), rect.top, pDoc->getBgColor());
	if (rect.bottom != crect.bottom)
		pDc->FillSolidRect(rect.left, rect.bottom, rect.Width(), crect.bottom - rect.bottom, pDoc->getBgColor());
	return;
}

void ObZrvView::fillBg(CDC *pDC)
{
	CRect rect;
	GetClientRect(rect);
	pDC->FillSolidRect(&rect, GetDocument()->getBgColor());
}

void ObZrvView::OnInitialUpdate()
{
	CView::OnInitialUpdate();
}

void ObZrvView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void ObZrvView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// ObZrvView diagnostics

#ifdef _DEBUG
void ObZrvView::AssertValid() const
{
	CView::AssertValid();
}

void ObZrvView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

ObZrvDoc* ObZrvView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(ObZrvDoc)));
	return (ObZrvDoc*)m_pDocument;
}
#endif //_DEBUG




BOOL ObZrvView::OnEraseBkgnd(CDC* pDC)
{
	// Do nothing here. We'll deal with the background in WM_PAINT
	return TRUE;
}


void ObZrvView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// get virtual key state
	enum { KEYHOLD = 0x8000 };
	bool bshift = (GetKeyState(VK_SHIFT) & KEYHOLD) != 0;
	bool bctrl = (GetKeyState(VK_CONTROL) & KEYHOLD) != 0;
	bool balt = (GetKeyState(VK_MENU) & KEYHOLD) != 0;

	// navigation
	if ((nChar == VK_LEFT || nChar == VK_UP || nChar == VK_PRIOR) && !bshift && !bctrl && !balt)
	{
		this->GetDocument()->navigate(ObZrvDoc::NAV_PREV);
		return;
	}
	if ((nChar == VK_RIGHT || nChar == VK_DOWN || nChar == VK_NEXT) && !bshift && !bctrl && !balt)
	{
		this->GetDocument()->navigate(ObZrvDoc::NAV_NEXT);
		return;
	}
	if (nChar == VK_HOME)
	{
		this->GetDocument()->navigate(ObZrvDoc::NAV_FIRST);
		return;
	}
	if (nChar == VK_END)
	{
		this->GetDocument()->navigate(ObZrvDoc::NAV_LAST);
		return;
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void ObZrvView::onFileOpened(int cmdid)
{
	Image *image = GetDocument()->getImage();
	if (!image)
		return;

	// ZT_FITIMAGE: adjust window size to fit the image
	CPoint mousepos = preserveMouse(cmdid);
	if (_zoomtype == ZT_FITIMAGE)
	{
		_zoomlevel = _fitlevel = 0;
		fitWindow2Image(image, mousepos);
	}
	_scaleSize = CSize{ -1, -1 };
	_viewCrop = { -1, -1, -1, -1 };
	_viewOffset = { 0, 0 };
}

// adjust window size to fit the image
void ObZrvView::fitWindow2Image(Image *image, CPoint mousepos)
{
	// get main window size & pos
	CWnd *pMainfrm = AfxGetApp()->GetMainWnd();
	WINDOWPLACEMENT winpos;
	winpos.length = sizeof(winpos);
	pMainfrm->GetWindowPlacement(&winpos);
	MONITORINFO minfo;
	CRect crect;
	bool mok = false;	// if we've got enough information about the monitor
	if (winpos.showCmd == SW_SHOWNORMAL)
	{
		// Get screen size
		HMONITOR hmon = MonitorFromWindow(pMainfrm->GetSafeHwnd(), MONITOR_DEFAULTTONULL);
		minfo.cbSize = sizeof(minfo);
		if (hmon && GetMonitorInfo(hmon, &minfo))
			mok = true;
		// Get current view size
		GetClientRect(&crect);
	}
	if (mok)
	{
		// Determine the best desired image display (view window) size => imw and imh
		// Step1: init them with the original image size
		int imw = image->getDimension().cx;
		int imh = image->getDimension().cy;
		// Step2: perform zooming
		if (_zoomlevel != 0)	// 
		{
			imw = imw * _zoomlevel / 100;
			imh = imh * _zoomlevel / 100;
		}
		// Step3: Determine the max possible view size and adjust imw/imh with it
		// current main frame size
		int fw = ((CRect)winpos.rcNormalPosition).Width();
		int fh = ((CRect)winpos.rcNormalPosition).Height();
		// screen size
		int sw = ((CRect)minfo.rcWork).Width();
		int sh = ((CRect)minfo.rcWork).Height();
		// max possible view window size
		int mw = sw - (fw - crect.Width());
		int mh = sh - (fh - crect.Height());
		// Adjust imw/imh with mw/mh
		if (_zoomlevel != 0)
		{
			imw = std::min(imw, mw);
			imh = std::min(imh, mh);
		}
		else if (imw > mw && mw * imh <= mh * imw)
		{
			imh = std::min(mw * imh / imw + 1, mh);
			imw = mw;
		}
		else if (imh > mh && mw * imh >= mh * imw)
		{
			imw = std::min(mh * imw / imh + 1, mw);
			imh = mh;
		}
		// Step4: If mw/mh allows, add a small outer margin to imw/imh, and also guarrantee a minimal imw/imh
		int margin = 0;// (int)(10 * AulddaysDpiHelper::getScale(pMainfrm->GetSafeHwnd()));	// allow some margin
		imw = std::min(std::max(imw + margin, (int)(400 * AulddaysDpiHelper::getScale(pMainfrm->GetSafeHwnd()))), mw);
		imh = std::min(std::max(imh + margin, 20), mh);

		// Now the best view window size in imw/imh is determined, reposition the main window based on it
		// diff size
		int dw = imw - crect.Width();
		int dh = imh - crect.Height();
		// adjust winpos
		CRect npos = winpos.rcNormalPosition;
		if (dw != 0)
		{
			npos.left -= dw / 2;
			npos.right += dw - dw / 2;
			// keep the window within monitor area
			if (npos.right > minfo.rcWork.right)
			{
				npos.left = minfo.rcWork.right - npos.Width();
				npos.right = minfo.rcWork.right;
			}
			if (npos.left < minfo.rcWork.left)
			{
				npos.right = minfo.rcWork.left + npos.Width();
				npos.left = minfo.rcWork.left;
			}
		}
		if (dh != 0)
		{
			npos.top -= dh / 2;
			npos.bottom += dh - dh / 2;
			// keep the window within monitor area. do bottom first, to force top in right place
			if (npos.bottom > minfo.rcWork.bottom)
			{
				npos.top = minfo.rcWork.bottom - npos.Height();
				npos.bottom = minfo.rcWork.bottom;
			}
			if (npos.top < minfo.rcWork.top)
			{
				npos.bottom = minfo.rcWork.top + npos.Height();
				npos.top = minfo.rcWork.top;
			}
		}
		winpos.rcNormalPosition = npos;
		if (dw != 0 || dh != 0)
		{
			pMainfrm->SetWindowPlacement(&winpos);
			if (mousepos.x >= 0)
			{
				mousepos.Offset(npos.TopLeft());
				SetCursorPos(mousepos.x, mousepos.y);
			}
		}
	}
	releaseBitmap();
	Invalidate(FALSE);
}

void ObZrvView::onFrameUpdate()
{
	releaseBitmap();
	Image *image = GetDocument()->getImage();
	if (image)
		_viewBitmap = image->getBBitmap(_scaleSize, _viewCrop);
	Invalidate(FALSE);
}

// Update status text
void ObZrvView::updateStatus()
{
	Image *image = GetDocument()->getImage();
	if (!image)
		return;

	enum { INFO_LEN = 1024 };
	static wchar_t infobuf[INFO_LEN];
	static char framebuf[20];
	_snwprintf(infobuf, INFO_LEN, L"%d/%d | %s | %dx%d%S%S %s | %d%% | %dms",
		GetDocument()->_diridx + 1, (int)GetDocument()->_dirfiles.size(),
		GetDocument()->_dirfiles[GetDocument()->_diridx].c_str(),
		image->getDimension().cx, image->getDimension().cy,
		image->isAnim() ? "x" : "",
		image->isAnim() ? _itoa(image->getFrameCount(), framebuf, 10) : "",
		image->getFormat(), _zoomlevel != 0 ? _zoomlevel : _fitlevel, _timecost);
	((ObZrvFrm *)AfxGetMainWnd())->SetInfoText(infobuf);
}

CPoint ObZrvView::preserveMouse(int id)
{
	if (id < 0)
		return CPoint{ -1, -1 };
	// get cursor pos
	CPoint curpos;
	GetCursorPos(&curpos);
	// enumerate toolbars
	const CObList &toolbars = CMFCToolBar::GetAllToolbars();
	for (POSITION postoolbar = toolbars.GetHeadPosition(); postoolbar != NULL; )
	{
		const CMFCToolBar *toolbar = (const CMFCToolBar *)toolbars.GetNext(postoolbar);
		// check whether cursor inside the toolbar
		CPoint tbpos = curpos;
		toolbar->ScreenToClient(&tbpos);
		CRect rectToolbar;
		toolbar->GetClientRect(&rectToolbar);
		if (!rectToolbar.PtInRect(tbpos))
			continue;
		// enumerate toolbar buttons
		const CObList &buttons = toolbar->GetAllButtons();
		for (POSITION posbutton = buttons.GetHeadPosition(); posbutton != NULL;)
		{
			const CMFCToolBarButton *button = (const CMFCToolBarButton *)buttons.GetNext(posbutton);
			// whether cursor inside the button
			if (button->Rect().PtInRect(tbpos) && !button->IsHidden())
			{
				if (button->m_nID == id)	// right the button requested
				{
					// convert curpos to be relative to the frame window
					CRect rectFrame;
					AfxGetMainWnd()->GetWindowRect(&rectFrame);
					curpos.Offset(-rectFrame.TopLeft());
					TRACE("Preserve toolbar ID %d, pos (%d:%d)\n", id, curpos.x, curpos.y);
					return curpos;
				}
				else
					return CPoint{ -1, -1 };
			}
		}
	}
	// not inside any toolbar button
	return CPoint{ -1, -1 };
}

void ObZrvView::OnZoomIn()
{
	zoom(1);
}
void ObZrvView::OnZoomOut()
{
	zoom(-1);
}
void ObZrvView::OnUpdateZoomIn(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(zoom(1, true) == 0);
}
void ObZrvView::OnUpdateZoomOut(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(zoom(-1, true) == 0);
}
void ObZrvView::OnUpdateZoomTo(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(GetDocument()->getImage() != NULL);
}

int ObZrvView::zoom(int inout, bool test)
{
	static const std::vector<int> levels = {
		1, 2, 3, 5, 7, 10, 15, 20, 30, 50, 70, 100, 150, 200, 300, 500, 700, 1000, 2000, 3000, 5000, 7000, 10000 };
	Image *image = GetDocument()->getImage();
	if (!image)
		return -1;
	int orilevel = _zoomlevel != 0 ? _zoomlevel : 100;
	if (inout == 0)
		return 0;
	else if (inout > 0)
	{
		if (_zoomlevel != 0 && _zoomlevel >= levels.back())
			return -1;
		if (test)
			return 0;
		_zoomlevel = *std::upper_bound(levels.begin(), levels.end(), _zoomlevel != 0 ? _zoomlevel : _fitlevel);
	}
	else
	{
		if (_zoomlevel != 0 && _zoomlevel <= levels.front())
			return -1;
		int newlevel = *std::upper_bound(levels.rbegin(), levels.rend(), _zoomlevel != 0 ? _zoomlevel : _fitlevel, std::greater<int>());
		if (std::max(image->getDimension().cx, image->getDimension().cy) * newlevel / 100 < 1)
			return -1;	// do not zoom out if already very small
		if (test)
			return 0;
		_zoomlevel = newlevel;
	}
	_viewOffset = { _viewOffset.cx * _zoomlevel / orilevel, _viewOffset.cy * _zoomlevel / orilevel };	// keep image view center
	CPoint mousepos = preserveMouse(inout > 0 ? ID_VIEW_ZOOMIN : ID_VIEW_ZOOMOUT);
	fitWindow2Image(image, mousepos);
	updateStatus();
	return 0;
}


/////////////////////// For dragging /////////////////////////
void ObZrvView::OnLButtonDown(UINT nFlags, CPoint point)
{
	_isdragging = true;
	_draggingOrigin = _draggingCur = point;
	SetCapture();
	CView::OnLButtonDown(nFlags, point);
}


void ObZrvView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (_isdragging)
	{
		_isdragging = false;
		ReleaseCapture();
		_viewOffset.cx -= _draggingCur.x - _draggingOrigin.x;
		_viewOffset.cy -= _draggingCur.y - _draggingOrigin.y;
		releaseBitmap();
		Invalidate(FALSE);
	}
	CView::OnLButtonUp(nFlags, point);
}


void ObZrvView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (_isdragging)
	{
		_draggingCur = point;
		releaseBitmap();
		Invalidate(FALSE);
	}
	CView::OnMouseMove(nFlags, point);
}


int ObZrvView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	//CGestureConfig config;
	//GetGestureConfig(&config);
	//config.EnableZoom();
	//SetGestureConfig(&config);

	return 0;
}

LRESULT ObZrvView::OnGesture(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

void ObZrvView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CView::OnVScroll(nSBCode, nPos, pScrollBar);
}
