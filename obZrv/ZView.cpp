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

	if (!_viewBitmap || _viewRect.left < 0 || _viewDim.cx < 0 || size != _viewWndDim)		// need update viewBitmap
	{
		CRect oldViewRect = _viewRect;
		CSize oldViewDim = _viewDim;
		if (_zoomtype == ZT_FITIMAGE && _zoomlevel == 0)
		{
			_viewRect = CRect{ 0, 0, image->getDimension().cx, image->getDimension().cy };
			if (size.cx >= image->getDimension().cx && size.cy >= image->getDimension().cy)
			{
				_viewDim.cx = image->getDimension().cx;
				_viewDim.cy = image->getDimension().cy;
				_fitlevel = 100;
			}
			else if ((uint64_t)size.cx * image->getDimension().cy > (uint64_t)size.cy * image->getDimension().cx)
			{
				_viewDim.cx = (LONG)((double)size.cy * image->getDimension().cx / image->getDimension().cy + 0.5);
				_viewDim.cx = std::max(_viewDim.cx, 1l);
				_viewDim.cy = size.cy;
				_fitlevel = size.cy * 100 / image->getDimension().cy;
			}
			else
			{
				assert((uint64_t)size.cx * image->getDimension().cy <= (uint64_t)size.cy * image->getDimension().cx);
				_viewDim.cy = (LONG)((double)size.cx * image->getDimension().cy / image->getDimension().cx + 0.5);
				_viewDim.cy = std::max(_viewDim.cy, 1l);
				_viewDim.cx = size.cx;
				_fitlevel = size.cx * 100 / image->getDimension().cx;
			}
		}
		else
		{
			// must have been a window size change or zooming
			assert(_viewRect.left >= 0 && _viewDim.cx >= 0);
			// keep the center point in current view still center in the new view
			_viewRect.left = (_viewRect.left + _viewRect.right) / 2 - rect.Width() * 100 / _zoomlevel / 2;
			_viewRect.right = _viewRect.left + rect.Width() * 100 / _zoomlevel;
			if (_viewRect.left < 0)
			{
				_viewRect.right -= _viewRect.left;
				_viewRect.left = 0;
			}
			if (_viewRect.right > image->getDimension().cx)
			{
				_viewRect.right = image->getDimension().cx;
				_viewRect.left = _viewRect.right - rect.Width() * 100 / _zoomlevel;
				if (_viewRect.left < 0)
				{
					_viewRect.left = 0;
					_viewDim.cx = std::max(image->getDimension().cx * _zoomlevel / 100, 1l);
				}
				else
					_viewDim.cx = size.cx;
			}
			else
				_viewDim.cx = size.cx;
			_viewRect.top = (_viewRect.top + _viewRect.bottom) / 2 - rect.Height() * 100 / _zoomlevel / 2;
			_viewRect.bottom = _viewRect.top + rect.Height() * 100 / _zoomlevel;
			if (_viewRect.top < 0)
			{
				_viewRect.bottom -= _viewRect.top;
				_viewRect.top = 0;
			}
			if (_viewRect.bottom > image->getDimension().cy)
			{
				_viewRect.bottom = image->getDimension().cy;
				_viewRect.top = _viewRect.bottom - rect.Height() * 100 / _zoomlevel;
				if (_viewRect.top < 0)
				{
					_viewRect.top = 0;
					_viewDim.cy = std::max(image->getDimension().cy * _zoomlevel / 100, 1l);
				}
				else
					_viewDim.cy = size.cy;
			}
			else
				_viewDim.cy = size.cy;
		}

		_viewWndDim = size;
		if (oldViewRect != _viewRect || oldViewDim != _viewDim)
			releaseBitmap();
		if (!_viewBitmap)
			_viewBitmap = image->getBBitmap(_viewRect, _viewDim);
		updateStatus();
	}

	if (!_viewBitmap)
	{
		fillBg(pDc);
		return;
	}
	// calculate the output rect
	if (rect.Width() > _viewDim.cx)
	{
		rect.left = (rect.Width() - _viewDim.cx) / 2;
		rect.right = rect.left + _viewDim.cx;
	}
	if (rect.Height() > _viewDim.cy)
	{
		rect.top = (rect.Height() - _viewDim.cy) / 2;
		rect.bottom = rect.top + _viewDim.cy;
	}
	assert(_viewBitmap->Width() == _viewDim.cx && _viewBitmap->Height() == _viewDim.cy);
	_viewBitmap->SetDIBitsToDevice(pDc->GetSafeHdc(), rect.left, rect.top, 0, 0, _viewDim.cx, _viewDim.cy);
	if (rect.left > 0)
		pDc->FillSolidRect(0, 0, rect.left, crect.bottom, _bgColor);
	if (rect.right != crect.right)
		pDc->FillSolidRect(rect.right, 0, crect.right - rect.right, crect.bottom, _bgColor);
	if (rect.top != 0)
		pDc->FillSolidRect(rect.left, 0, rect.Width(), rect.top, _bgColor);
	if (rect.bottom != crect.bottom)
		pDc->FillSolidRect(rect.left, rect.bottom, rect.Width(), crect.bottom - rect.bottom, _bgColor);
	return;
}

void ObZrvView::fillBg(CDC *pDC)
{
	CRect rect;
	GetClientRect(rect);
	pDC->FillSolidRect(&rect, _bgColor);
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
		_viewRect = CRect{ 0, 0, image->getDimension().cx, image->getDimension().cy };
		_viewDim = CSize{ -1, -1 };
		fitWindow2Image(image, mousepos);
	}
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
		// image size
		int imw = image->getDimension().cx;
		int imh = image->getDimension().cy;
		if (_zoomlevel != 0)
		{
			imw = imw * _zoomlevel / 100;
			imh = imh * _zoomlevel / 100;
		}
		// main frame size
		int fw = ((CRect)winpos.rcNormalPosition).Width();
		int fh = ((CRect)winpos.rcNormalPosition).Height();
		// screen size
		int sw = ((CRect)minfo.rcWork).Width();
		int sh = ((CRect)minfo.rcWork).Height();
		// max possible view size
		int mw = sw - fw + crect.Width();
		int mh = sh - fh + crect.Height();
		// calculate the adapted view size
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
		int margin = (int)(10 * AulddaysDpiHelper::getScale(pMainfrm->GetSafeHwnd()));	// allow some margin
		imw = std::min(std::max(imw + margin, (int)(400 * AulddaysDpiHelper::getScale(pMainfrm->GetSafeHwnd()))), mw);
		imh = std::min(std::max(imh + margin, 20), mh);

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
		_viewBitmap = image->getBBitmap(_viewRect, _viewDim);
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
	_snwprintf(infobuf, INFO_LEN, L"%d/%d | %s | %dx%d%S%S %s | %d%%",
		GetDocument()->_diridx + 1, (int)GetDocument()->_dirfiles.size(),
		GetDocument()->_dirfiles[GetDocument()->_diridx].c_str(),
		image->getDimension().cx, image->getDimension().cy,
		image->isAnim() ? "x" : "",
		image->isAnim() ? _itoa(image->getFrameCount(), framebuf, 10) : "",
		image->getFormat(), _zoomlevel != 0 ? _zoomlevel : _fitlevel);
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

int ObZrvView::zoom(int inout, bool test)
{
	static const std::vector<int> levels = {
		1, 2, 3, 5, 7, 10, 15, 20, 30, 50, 70, 100, 150, 200, 300, 500, 700, 1000, 2000, 3000, 5000, 7000, 10000 };
	Image *image = GetDocument()->getImage();
	if (!image)
		return -1;
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
	CPoint mousepos = preserveMouse(inout > 0 ? ID_VIEW_ZOOMIN : ID_VIEW_ZOOMOUT);
	fitWindow2Image(image, mousepos);
	updateStatus();
	return 0;
}
