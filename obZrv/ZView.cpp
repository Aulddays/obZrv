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

#include "ZDoc.h"
#include "ZView.h"

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
END_MESSAGE_MAP()

// ObZrvView construction/destruction

ObZrvView::ObZrvView()
{
	// TODO: add construction code here

}

ObZrvView::~ObZrvView()
{
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
	if (!pDoc)
	{
		fillBg(pDc);
		return;
	}

	CRect rect, crect;
	GetClientRect(rect);
	crect = rect;
	SIZE size = { rect.Width(), rect.Height() };

	BasicBitmap *pBitmap = pDoc->getBBitmap(size);
	if (!pBitmap)
	{
		fillBg(pDc);
		return;
	}
	// calculate the output rect
	if (rect.Width() > size.cx)
	{
		rect.left = (rect.Width() - size.cx) / 2;
		rect.right = rect.left + size.cx;
	}
	if (rect.Height() > size.cy)
	{
		rect.top = (rect.Height() - size.cy) / 2;
		rect.bottom = rect.top + size.cy;
	}
	pBitmap->SetDIBitsToDevice(pDc->GetSafeHdc(), rect.left, rect.top, 0, 0, pBitmap->Width(), pBitmap->Height());
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
