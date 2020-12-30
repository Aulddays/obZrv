// obZrv
// https://github.com/Aulddays/obZrv
// 
// Copyright (c) 2020 Aulddays (https://dev.aulddays.com/). All rights reserved.
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

// obZrvView.h : interface of the ObZrvView class
//

#pragma once


class ObZrvView : public CView
{
protected: // create from serialization only
	ObZrvView();
	DECLARE_DYNCREATE(ObZrvView)

// Attributes
public:
	ObZrvDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void OnInitialUpdate(); // called first time after construct

// Implementation
public:
	virtual ~ObZrvView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	COLORREF _bgColor{ RGB(66, 66, 66) };

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	void fillBg(CDC *pDC);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
};

#ifndef _DEBUG  // debug version in ObZrvView.cpp
inline ObZrvDoc* ObZrvView::GetDocument() const
   { return reinterpret_cast<ObZrvDoc*>(m_pDocument); }
#endif

