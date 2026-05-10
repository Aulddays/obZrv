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

#include <windows.h>
#include "WndBase.h"

#define IMAGEVIEW_CLASS TEXT("obzImageView")

class Doc;
class Image;
class BasicBitmap;

class ImageView : public WndBase
{
public:
	bool Create(HWND hParent, HINSTANCE hInst, HWND hMainWnd);
	void Layout(int x, int y, int w, int h);
	void setDoc(Doc *doc);
	void setToolbar(HWND hToolbar) { m_hToolbar = hToolbar; }

	// Callbacks invoked by Doc
	void onFileOpened(int cmdid);
	void onFrameUpdate();
	void updateStatus();

	// Zoom: inout>0 = zoom in, inout<0 = zoom out. test=true: only check feasibility.
	int zoom(int inout, bool test = false);

	// Set zoom mode from one of the ID_ZOOMMODE_* command IDs
	void setZoomMode(int cmdId);

protected:
	LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
	Doc       *_doc       = NULL;
	HINSTANCE  m_hInst    = NULL;
	HWND       m_hMainWnd = NULL;
	HWND       m_hToolbar = NULL;  // toolbar HWND for preserveMouse hit-test

	// Zoom state (from ZView)
	enum ZoomType {
		ZT_W2I_ZOOMOUT = 0, // Window Fit Image, ZoomOut only (default)
		ZT_W2I,             // Window Fit Image, 100% natural size
		ZT_I2W_ZOOMOUT,     // Image Fit Window, ZoomOut only
		ZT_I2W,             // Image Fit Window, both directions
		ZT_NOFIT,           // No Fit, always 100%
	} _zoomtype = ZT_W2I_ZOOMOUT;
	int _zoomlevel = 0;   // 0 = auto mode; nonzero = fixed % zoom
	int _fitlevel  = 0;   // effective zoom% when in auto mode
	int _timecost  = 0;   // last decode+scale time in ms

	// View bitmap (from ZView)
	BasicBitmap *_viewBitmap     = NULL;
	bool         _internalBitmap = false;
	void releaseBitmap();

	// View layout state (from ZView)
	SIZE _viewWndDim = { -1, -1 }; // last known client size
	SIZE _scaleSize  = { -1, -1 }; // full scaled image size at current zoom
	SIZE _viewOffset = {  0,  0 }; // pan offset on scaled image
	RECT _viewCrop   = { -1, -1, -1, -1 }; // visible area on scaled image

	// Dragging (from ZView)
	enum DragType {
		DT_PAN,      // pan with left button
		DT_ZOOMDRAG  // zoom with right button or Ctrl+left button
	};
	DragType _dragType    = DT_PAN;
	bool  _isdragging     = false;
	POINT _draggingOrigin = { -1, -1 };
	POINT _draggingCur    = { -1, -1 };
	POINT _zoomDragOrigin = { -1, -1 };  // origin for zoom drag
	int   _zoomDragStartLevel = 0;       // zoom level (%) at drag start
	SIZE  _clickImageCoord = { 0, 0 };   // original image coord at click position

	// Helpers
	POINT preserveMouse(int id);  // returns frame-relative cursor pos if cursor is on button 'id'
	void fitWindow2Image(Image *image, POINT mousepos);
	void OnPaint(HDC hdc);
	void fillBg(HDC hdc);
	void applyZoomDrag(int targetZoomLevel);
	void adjustViewOffsetForZoom(int oldZoomLevel, SIZE oldScaleSize);
};
