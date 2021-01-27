// obZrv
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

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "obZrv.h"
#endif
#include "ZDoc.h"

#include <stdint.h>
#include <algorithm>
#include <functional>
#include <set>
#include <string.h>
#include <stdio.h>
#include <propkey.h>
#include "GdiPlusCodec.h"
#include "WebpCodec.h"
#include "ZView.h"
#include "Frame.h"
#include "../AulddaysDpiHelper/AulddaysDpiHelper.h"

#undef max
#undef min

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// ObZrvDoc

IMPLEMENT_DYNCREATE(ObZrvDoc, CDocument)

BEGIN_MESSAGE_MAP(ObZrvDoc, CDocument)
	ON_COMMAND(ID_FILE_SAVE, &ObZrvDoc::OnFileSave)
	ON_COMMAND(ID_VIEW_ZOOMIN, &ObZrvDoc::OnZoomIn)
	ON_COMMAND(ID_VIEW_ZOOMOUT, &ObZrvDoc::OnZoomOut)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMIN, &ObZrvDoc::OnUpdateZoomIn)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOMOUT, &ObZrvDoc::OnUpdateZoomOut)
	ON_COMMAND(ID_FILE_PREV, &ObZrvDoc::OnNavPrev)
	ON_COMMAND(ID_FILE_NEXT, &ObZrvDoc::OnNavNext)
	ON_UPDATE_COMMAND_UI(ID_FILE_PREV, &ObZrvDoc::OnUpdateNavPrevNext)
	ON_UPDATE_COMMAND_UI(ID_FILE_NEXT, &ObZrvDoc::OnUpdateNavPrevNext)
END_MESSAGE_MAP()

const static wchar_t DIRSEP = L'\\';

GdiPlusCodec gdiplusCodec;
WebpCodec webpCodec;

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

	_view = NULL;
	POSITION posView = GetFirstViewPosition();
	while (posView)
	{
		_view = dynamic_cast<ObZrvView *>(GetNextView(posView));
		if (_view)
			break;
	}
	if (!_view)
		return FALSE;
	if (_animated)
		_view->KillTimer((UINT_PTR)this);

	updateDir(lpszPathName, true);

	DeleteContents();
	SetModifiedFlag();  // dirty during de-serialize

	Codec *codec = &gdiplusCodec;
	// check ext
	const wchar_t *pos = wcsrchr(lpszPathName, L'.');
	if (!pos)
		pos = lpszPathName + wcslen(lpszPathName);
	else
		++pos;
	if (_wcsicmp(pos, L"webp") == 0)
		codec = &webpCodec;

	int res = codec->open(lpszPathName, &_image);

	SetModifiedFlag(FALSE);     // start off with unmodified

	if (res)
		return FALSE;

	updateDir(lpszPathName, false);

	_view->onFileOpened(_cmdid);

	_curframe = 0;
	_curloop = 0;
	_animated = _image->isAnim();
	if (_animated)
	{
		_tmstart = GetTickCount();
		_totaldelay = _image->getFrameDelay();
		_view->SetTimer((UINT_PTR)this, _image->getFrameDelay(), onAnimate);
	}

	_view->updateStatus();

	return TRUE;
}

void ObZrvDoc::DeleteContents()
{
	delete _image;
	_image = NULL;
	_diridx = -1;
}

void ObZrvDoc::OnFileSave()
{
	DoSave(NULL);
}


BOOL ObZrvDoc::OnSaveDocument(LPCTSTR lpszPathName)
{
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

void ObZrvDoc::OnUpdateNavPrevNext(CCmdUI *pCmdUI)
{
	if (_diridx == -1)
		pCmdUI->Enable(FALSE);
	else if (pCmdUI->m_nID == ID_FILE_PREV)
		pCmdUI->Enable(_diridx > 0);
	else
		pCmdUI->Enable(_diridx < (int)_dirfiles.size() - 1);
}

void CALLBACK ObZrvDoc::onAnimate(HWND hWnd, UINT nMsg, UINT_PTR nIDEvent, DWORD dwTime)
{
	ObZrvDoc *pthis = (ObZrvDoc *)nIDEvent;
	pthis->_view->KillTimer((UINT_PTR)nIDEvent);
	if (!pthis->_image)
		return;

	if (pthis->_image->getLoopNum() > 0 && pthis->_curloop >= pthis->_image->getLoopNum())	// loop end
	{
		pthis->_curframe = pthis->_image->getFrameCount() - 1;
		pthis->UpdateAllViews(NULL);
		return;
	}

	// adjust time
	int64_t tmcur = GetTickCount();
	while (pthis->_tmstart + pthis->_totaldelay - pthis->_image->getFrameDelay() > tmcur)
		pthis->_tmstart -= (int64_t)0x100000000ll;	// fix GetTickCount() wrap
	long allowdiff = std::max(pthis->_image->getFrameDelay() * 10, 1000l);
	bool reset = false;
	if (pthis->_tmstart + pthis->_totaldelay > tmcur + allowdiff || pthis->_tmstart + pthis->_totaldelay < tmcur - allowdiff)
	{
		// shifted too much, just start over
		reset = true;
		pthis->_curframe = pthis->_image->getFrameCount() - 1;
		pthis->_tmstart = tmcur;
	}
	else if (pthis->_tmstart + pthis->_totaldelay > tmcur + 20)
	{
		// current frame not due yet, just wait more
		pthis->_view->SetTimer((UINT_PTR)nIDEvent, (UINT)(pthis->_tmstart + pthis->_totaldelay - tmcur), onAnimate);
		return;
	}

	do
	{
		int res = pthis->_image->nextFrame(reset);
		if (reset)
			pthis->_curframe = 0;
		else
			pthis->_curframe++;
		if (res == IM_NO_MORE_FRAMES)
		{
			pthis->_curloop++;
			if (pthis->_image->getLoopNum() > 0 && pthis->_curloop >= pthis->_image->getLoopNum())	// loop end
			{
				pthis->_curframe = pthis->_image->getFrameCount() - 1;
				break;
			}
			if (pthis->_curframe <= 1)	// only one frame
			{
				pthis->_curframe = 0;
				pthis->_curloop = -1;
				break;
			}
			res = pthis->_image->nextFrame(true);
			pthis->_curframe = 0;
		}
		if (pthis->_image->getFrameDelay() == 0)	// 0-delay frame
		{
			if (pthis->_curloop > 0 && pthis->_totaldelay == 0)	// all frames were 0-delay, just stick to frame 0
			{
				res = pthis->_image->nextFrame(true);
				pthis->_curframe = 0;
				pthis->_curloop = -1;
				break;
			}
			else
				continue;	// skip 0 delay frames
		}
		pthis->_totaldelay += pthis->_image->getFrameDelay();
	} while (pthis->_tmstart + pthis->_totaldelay <= tmcur + 10);	// skip a frame if its time has already passed

	//pthis->UpdateAllViews(NULL);
	pthis->_view->onFrameUpdate();
	if (pthis->_image->getLoopNum() <= 0 && pthis->_curloop >=0 ||	// setup timer only if has not loop end
		pthis->_curloop < pthis->_image->getLoopNum())
	{
		int64_t tmcurnew = GetTickCount();
		if (tmcurnew < tmcur)
			pthis->_tmstart -= (int64_t)0x100000000ll;
		// calculate precise delay
		UINT delay = pthis->_tmstart + pthis->_totaldelay > tmcur ? (UINT)(pthis->_tmstart + pthis->_totaldelay - tmcur) : 1;
		delay = std::min(delay, (UINT)(pthis->_image->getFrameDelay()) * 5);
		pthis->_view->SetTimer(nIDEvent, delay, onAnimate);
	}
}

void ObZrvDoc::OnZoomIn()
{
	zoom(1);
}
void ObZrvDoc::OnZoomOut()
{
	zoom(-1);
}
void ObZrvDoc::OnUpdateZoomIn(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(zoom(1, true) == 0);
}
void ObZrvDoc::OnUpdateZoomOut(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(zoom(-1, true) == 0);
}

int ObZrvDoc::zoom(int inout, bool test)
{
	static const std::vector<int> levels = {
		1, 2, 3, 5, 7, 10, 15, 20, 30, 50, 70, 100, 150, 200, 300, 500, 700, 1000, 2000, 3000, 5000, 7000, 10000 };
	if (!_image)
		return -1;
	if (inout == 0)
		return 0;
	else if (inout > 0)
	{
		if (_zoomlevel != 0 && _zoomlevel >= levels.back())
			return -1;
		if (test)
			return 0;
		_zoomlevel = *std::upper_bound(levels.begin(), levels.end(), _zoomlevel != 0 ? _zoomlevel : 100);
	}
	else
	{
		if (_zoomlevel != 0 && _zoomlevel <= levels.front())
			return -1;
		int newlevel = *std::upper_bound(levels.rbegin(), levels.rend(), _zoomlevel != 0 ? _zoomlevel : 100, std::greater<int>());
		if (std::max(_image->getDimension().cx, _image->getDimension().cy) * newlevel / 100 < 1)
			return -1;	// do not zoom out if already very small
		if (test)
			return 0;
		_zoomlevel = newlevel;
	}
	_view->updateStatus();
	return 0;
}
