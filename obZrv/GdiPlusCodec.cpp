// obZrv
// https://github.com/Aulddays/ZaViewer
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
#include "GdiPlusCodec.h"
#include <stdint.h>
#include <memory>

class GdiPlusImage : public Image
{
	friend GdiPlusCodec;
protected:
	//uint8_t *fileBuf = NULL;
	//size_t fileLen = 0;

	// image properties
	SIZE _size{};
	bool _animated = false;

	// bitmap object of current frame
	//Gdiplus::Bitmap *_bitmap = NULL;
	BasicBitmap *_bbitmap = NULL;

	// (resized) output bitmap of current frame
	Gdiplus::Color _bgcolor{ 240, 240, 240 };
	//HBITMAP _hOutBitmap = NULL;
	BasicBitmap *_outBitmap = NULL;
	RECT _outRect;
	SIZE _outSize;

	int open(const wchar_t *filename);

public:
	virtual ~GdiPlusImage()
	{
		//if (_hOutBitmap)
		//	DeleteObject(_hOutBitmap);
		//delete _bitmap;
		delete _bbitmap;
		delete _outBitmap;
		//delete []fileBuf;
	}

	virtual SIZE getSize() const
	{
		return _size;
	}

	virtual int getFrame()
	{
		//if (!_bitmap)
		//	return IM_FAIL;
		if (_animated)
			return IM_NOT_SUPPORTED;
		return IM_OK;
	}

	BasicBitmap *getBBitmap(RECT srcRect, SIZE outSize)
	{
		// if same as the one buffered, use it directly
		if (_outBitmap && srcRect == _outRect && outSize == _outSize)
			return _outBitmap;
		if (!_bbitmap)	// no opened image
			return NULL;
		if (!(srcRect.left >= 0 && srcRect.right > srcRect.left && srcRect.top >= 0 && srcRect.bottom > srcRect.top &&
			srcRect.right <= _size.cx && srcRect.bottom <= _size.cy && outSize.cx > 0 && outSize.cy > 0))
			return NULL;	// invalid input paramerters

		// if the whole image without scaling is required, just return the original bitmap
		if (srcRect.top == 0 && srcRect.left == 0 && srcRect.bottom == _size.cy && srcRect.right == _size.cx && outSize == _size)
			return _bbitmap;

		delete _outBitmap;	// delete the buffered one
		_outBitmap = NULL;
		_outRect = srcRect;
		_outSize = outSize;

		// crop & scale
		_outBitmap = new BasicBitmap(outSize.cx, outSize.cy, _bbitmap->Format());
		_outBitmap->Resample(0, 0, outSize.cx, outSize.cy, _bbitmap,
			srcRect.left, srcRect.top, srcRect.right, srcRect.bottom, BasicBitmap::BILINEAR);
		return _outBitmap;
	}
};

GdiPlusCodec::GdiPlusCodec()
{
}

GdiPlusCodec::~GdiPlusCodec()
{
}

int GdiPlusCodec::open(const wchar_t *filename, Image ** image)
{
	*image = NULL;
	std::unique_ptr<GdiPlusImage> pimg = std::make_unique<GdiPlusImage>();
	int res = pimg->open(filename);
	if (res != IM_OK)
		return res;
	*image = pimg.release();
	return res;
}

int GdiPlusImage::open(const wchar_t *filename)
{
	_bbitmap = BasicBitmap::GdiPlusLoadImage(filename);
	if (!_bbitmap)
		return IM_FAIL;

	//// create the bitmap obj
	//std::unique_ptr<Gdiplus::Bitmap> newbitmap = std::make_unique<Gdiplus::Bitmap>(filename);
	//// open failed
	//if (newbitmap->GetLastStatus() != Gdiplus::Ok)
	//	return IM_FAIL;
	//// check if too large
	//if (newbitmap->GetWidth() > MAX_IMAGE_DIMENSION || newbitmap->GetHeight() > MAX_IMAGE_DIMENSION ||
	//	newbitmap->GetWidth() * newbitmap->GetHeight() > MAX_IMAGE_PIXELS)
	//{
	//	return IM_TOO_LARGE;
	//}
	//_bitmap = newbitmap.release();

	_size.cx = _bbitmap->Width();
	_size.cy = _bbitmap->Height();

	return IM_OK;
}