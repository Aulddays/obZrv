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

// ZDoc.h : interface of the ObZrvDoc class
//


#pragma once

#include "Codec.h"
#include <vector>
#include <string>


class ObZrvDoc : public CDocument
{
protected: // create from serialization only
	ObZrvDoc();
	DECLARE_DYNCREATE(ObZrvDoc)

	// Attributes
public:
	BasicBitmap *getBBitmap(SIZE &size);

protected:
	Image * _image = NULL;

	// dir management
	std::wstring _dir;
	std::vector<std::wstring> _dirfiles;
	int _diridx = -1;
	// update dirfiles, if preservlast is false and filename is in old _dir, do not update _dir
	int updateDir(const wchar_t *filename, bool preservelast = false);

	// Operations
public:
	static int initCodec();

	// navigate
	enum NavCmd
	{
		NAV_FIRST,
		NAV_LAST,
		NAV_PREV,
		NAV_NEXT,
	};
	int navigate(NavCmd cmd);

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
};
