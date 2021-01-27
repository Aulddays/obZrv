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

// ZDoc.h : interface of the ObZrvDoc class
//


#pragma once

#include "Codec.h"
#include <vector>
#include <string>
#include <stdint.h>

class ObZrvView;

class ObZrvDoc : public CDocument
{
	friend ObZrvView;

protected: // create from serialization only
	ObZrvDoc();
	DECLARE_DYNCREATE(ObZrvDoc)

	// Attributes
public:
	BasicBitmap *getBBitmap(SIZE &size);
	Image *getImage() { return _image; }

protected:
	ObZrvView *_view = NULL;

	Image * _image = NULL;
	
	// for animated images
	bool _animated = false;
	int _curframe = 0;
	int _curloop = 0;
	int64_t _tmstart = 0;	// GetTickCount() of animation start. may be adjusted to < 0 if GetTickCount() has wrapped
	int64_t _totaldelay = 0;	// sum of frame delays desired
	static void CALLBACK onAnimate(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime);

	// dir management
	std::wstring _dir;
	std::vector<std::wstring> _dirfiles;
	int _diridx = -1;
	// update dirfiles, if preservlast is false and filename is in old _dir, do not update _dir
	int updateDir(const wchar_t *filename, bool preservelast = false);
	int _cmdid = -1;

	// zooming
	enum ZoomType
	{
		ZT_FITIMAGE,
	} _zoomtype;
	int _zoomlevel = 0;
	int _fitlevel = 0;	// actual zoom level if in fit mode

	// Operations
public:
	static int initCodec();

	// navigate
	enum NavCmd
	{
		NAV_FIRST,
		NAV_LAST,
		NAV_PREV = ID_FILE_PREV,
		NAV_NEXT = ID_FILE_NEXT,
	};
	int navigate(NavCmd cmd);
	afx_msg void OnNavPrev() { _cmdid = NAV_PREV; navigate(NAV_PREV); _cmdid = -1; }
	afx_msg void OnNavNext() { _cmdid = NAV_NEXT; navigate(NAV_NEXT); _cmdid = -1; }
	afx_msg void OnUpdateNavPrevNext(CCmdUI *pCmdUI);
	//afx_msg void OnUpdateNavNext(CCmdUI *pCmdUI);

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~ObZrvDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
public:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual void DeleteContents();
	afx_msg void OnFileSave();
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);

protected:

	afx_msg void OnZoomIn();
	afx_msg void OnZoomOut();
	afx_msg void OnUpdateZoomIn(CCmdUI *pCmdUI);
	afx_msg void OnUpdateZoomOut(CCmdUI *pCmdUI);
	int zoom(int inout, bool test = false);
public:
};
