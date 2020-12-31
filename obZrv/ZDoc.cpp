// obZrv
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

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "obZrv.h"
#endif
#include "ZDoc.h"

#include <stdint.h>
#include <algorithm>
#include <set>
#include <string.h>
#include <propkey.h>
#include "GdiPlusCodec.h"

#undef max
#undef min

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ObZrvDoc

IMPLEMENT_DYNCREATE(ObZrvDoc, CDocument)

BEGIN_MESSAGE_MAP(ObZrvDoc, CDocument)
	ON_COMMAND(ID_FILE_SAVE, &ObZrvDoc::OnFileSave)
END_MESSAGE_MAP()

const static wchar_t DIRSEP = L'\\';

GdiPlusCodec gdiplusCodec;

int ObZrvDoc::initCodec()
{
	if (gdiplusCodec.init() != 0)
		return -1;
	return 0;
}


// ObZrvDoc construction/destruction

ObZrvDoc::ObZrvDoc()
{
	// TODO: add one-time construction code here
	TRACE("ObZrvDoc::ObZrvDoc()\n");
}

ObZrvDoc::~ObZrvDoc()
{
}

BOOL ObZrvDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// ObZrvDoc serialization

void ObZrvDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void ObZrvDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void ObZrvDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void ObZrvDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// ObZrvDoc diagnostics

#ifdef _DEBUG
void ObZrvDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void ObZrvDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// ObZrvDoc commands


BOOL ObZrvDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
#ifdef _DEBUG
	if (IsModified())
		TRACE(traceAppMsg, 0, "Warning: OnOpenDocument replaces an unsaved document.\n");
#endif
	ENSURE(lpszPathName);

	updateDir(lpszPathName, true);

	DeleteContents();
	SetModifiedFlag();  // dirty during de-serialize

	int res = gdiplusCodec.open(lpszPathName, &_image);

	SetModifiedFlag(FALSE);     // start off with unmodified

	if (res)
		return FALSE;

	return TRUE;
}

void ObZrvDoc::DeleteContents()
{
	// TODO: Add your specialized code here and/or call the base class
	delete _image;
	_image = NULL;
}

BasicBitmap *ObZrvDoc::getBBitmap(SIZE &size)
{
	if (!_image || size.cx <= 0 || size.cy <= 0)
		return NULL;
	_image->getFrame();
	RECT srcRect = { 0, 0, _image->getSize().cx, _image->getSize().cy };
	if (size.cx >= _image->getSize().cx && size.cy >= _image->getSize().cy)
	{
		size.cx = _image->getSize().cx;
		size.cy = _image->getSize().cy;
	}
	else if ((uint64_t)size.cx * _image->getSize().cy > (uint64_t)size.cy * _image->getSize().cx)
	{
		size.cx = (LONG)((double)size.cy * _image->getSize().cx / _image->getSize().cy + 0.5);
		size.cx = std::max(size.cx, 1l);
	}
	else if ((uint64_t)size.cx * _image->getSize().cy < (uint64_t)size.cy * _image->getSize().cx)
	{
		size.cy = (LONG)((double)size.cx * _image->getSize().cy / _image->getSize().cx + 0.5);
		size.cy = std::max(size.cy, 1l);
	}
	return _image->getBBitmap(srcRect, size);
}


void ObZrvDoc::OnFileSave()
{
	DoSave(NULL);
}


BOOL ObZrvDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
	// TODO: Add your specialized code here and/or call the base class

	//return CDocument::OnSaveDocument(lpszPathName);
	return TRUE;
}

// update dirfiles, if preservlast is false and filename is in old _dir, do not update _dir
int ObZrvDoc::updateDir(const wchar_t *filepath, bool preservelast/* = false*/)
{
	// parse filepath
	std::wstring path;
	std::wstring filename;
	const wchar_t *pos = wcsrchr(filepath, DIRSEP);
	if (pos)
	{
		path.assign(filepath, pos);
		filename.assign(pos + 1);
	}
	else
	{
		path = L".";
		filename = filepath;
	}
	if (_dir == path && preservelast)
		return 0;
	std::transform(filename.begin(), filename.end(), filename.begin(), towlower);

	// clear
	_dir = L"";
	_dirfiles.clear();
	_diridx = -1;
	if (filename.empty())
		return 0;
	
	// find file list
	WIN32_FIND_DATAW wfd;
	HANDLE hFind = FindFirstFileW((path + DIRSEP + L"*").c_str(), &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
		return -1;
	do
	{
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		// check ext
		pos = wcsrchr(wfd.cFileName, L'.');
		if (!pos)
			continue;
		std::wstring ext(pos + 1);
		std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
		static const std::set<std::wstring> acceptext = {
			L"bmp", L"jpg", L"jpeg", L"gif", L"png", L"webp"
		};
		if (acceptext.count(ext) == 0)
			continue;
		_dirfiles.push_back(wfd.cFileName);
	} while (FindNextFileW(hFind, &wfd));
	FindClose(hFind);
	// sort
	std::sort(_dirfiles.begin(), _dirfiles.end(),
		[](const std::wstring &l, const std::wstring &r) -> bool { return StrCmpLogicalW(l.c_str(), r.c_str()) < 0; });

	_dir = path;
	// search for current file
	for (int i = 0; i < (int)_dirfiles.size(); ++i)
	{
		if (_wcsicmp(filename.c_str(), _dirfiles[i].c_str()) == 0)
		{
			_diridx = i;
			break;
		}
	}

	return 0;
}

int ObZrvDoc::navigate(NavCmd cmd)
{
	updateDir(GetPathName());
	if (_diridx == -1)	// no file open
		return -1;
	if (cmd == NAV_PREV && _diridx > 0)
		return AfxGetApp()->OpenDocumentFile((_dir + DIRSEP + _dirfiles[_diridx - 1]).c_str(), FALSE) ? 0 : -1;
	if (cmd == NAV_NEXT && _diridx < (int)_dirfiles.size() - 1)
		return AfxGetApp()->OpenDocumentFile((_dir + DIRSEP + _dirfiles[_diridx + 1]).c_str(), FALSE) ? 0 : -1;
	if (cmd == NAV_FIRST && _diridx > 0)
		return AfxGetApp()->OpenDocumentFile((_dir + DIRSEP + _dirfiles[0]).c_str(), FALSE) ? 0 : -1;
	if (cmd == NAV_LAST && _diridx < (int)_dirfiles.size() - 1)
		return AfxGetApp()->OpenDocumentFile((_dir + DIRSEP + _dirfiles[_dirfiles.size() - 1]).c_str(), FALSE) ? 0 : -1;
	return -1;
}