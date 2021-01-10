// obZrv
// https://github.com/Aulddays/ZaViewer
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
#include "GdiPlusCodec.h"
#include <stdint.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <gdiplus.h>

#undef max

static BasicBitmap *gdiplusConvert(Gdiplus::Bitmap *gbitmap, BasicBitmap::PixelFmt outfmt = BasicBitmap::UNKNOW);

class GdiPlusImage : public Image
{
	friend GdiPlusCodec;
protected:
	//uint8_t *fileBuf = NULL;
	//size_t fileLen = 0;

	// image properties
	GUID _imgfmt;
	SIZE _dimension{};
	int _framecnt = 0;
	std::vector<long> _framedelay;
	int _loopnum = 0;
	int _curfid = 0;	// current frame id

	// bitmap object of current frame
	BasicBitmap *_fbitmap = NULL;

	// gdiplus bitmap object. only used for animated images, otherwise _fbitmap is sufficient
	Gdiplus::Bitmap *_gbitmap = NULL;
	// buffer for _gbitmap
	IStream *_gstream = NULL;

	// (resized) output bitmap of current frame
	Gdiplus::Color _bgcolor{ 240, 240, 240 };
	//HBITMAP _hOutBitmap = NULL;
	BasicBitmap *_outBitmap = NULL;
	RECT _outRect;
	SIZE _outDim;

	int open(const wchar_t *filename)
	{
		// read file into buffer
		int res = IM_OK;
		if ((res = readFile(filename)) != IM_OK)
			return res;

		// make a stream out of the buffer for gdi
		// https://docs.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-shcreatememstream
		// Prior to Windows Vista, SHCreateMemStream() must be called directly from Shlwapi.dll as ordinal 12
		static IStream *(WINAPI *pfSHCreateMemStream)(const BYTE *pInit, UINT cbInit) = NULL;
		if (!pfSHCreateMemStream)
		{
			static HINSTANCE hShlwapi = NULL;
			if (!hShlwapi)
			{
				hShlwapi = LoadLibraryA("Shlwapi.dll");
				if (!hShlwapi)
					return IM_NOT_SUPPORTED;
			}
			else
				return IM_NOT_SUPPORTED;
			pfSHCreateMemStream = (IStream *(WINAPI *)(const BYTE *pInit, UINT cbInit))GetProcAddress(hShlwapi, (LPCSTR)12);
			if (!pfSHCreateMemStream)
				return IM_NOT_SUPPORTED;
		}
		_gstream = pfSHCreateMemStream(_filebuf, _filesize);
		if (!_gstream)
			return IM_FAIL;

		// load the image using gdi
		_gbitmap = new Gdiplus::Bitmap(_gstream);
		if (_gbitmap->GetLastStatus() != Gdiplus::Ok)
			return IM_FAIL;

		_dimension.cx = _gbitmap->GetWidth();
		_dimension.cy = _gbitmap->GetHeight();

		// get image format
		if (_gbitmap->GetRawFormat(&_imgfmt) != Gdiplus::Ok)
			return IM_NOT_SUPPORTED;

		// test animated
		UINT dimcnt = _gbitmap->GetFrameDimensionsCount();
		TRACE("Frame dim count %u\n", dimcnt);
		std::vector<GUID> dimids(dimcnt);
		if (_gbitmap->GetFrameDimensionsList(dimids.data(), dimcnt) != Gdiplus::Ok)
			return IM_FAIL;
		UINT framecnt = dimcnt != 0 ? _gbitmap->GetFrameCount(&dimids[0]) : 1;
		TRACE("Frame count %u\n", framecnt);
		_framecnt = framecnt;
		// verify image format
		if (_framecnt > 1 && _imgfmt != Gdiplus::ImageFormatGIF && _imgfmt != Gdiplus::ImageFormatTIFF)
			_framecnt = 1;	// only animated gifs & tiffs are supported

		// if animated, get more info
		if (framecnt > 1)
		{
			// frame delay
			UINT propsize = _gbitmap->GetPropertyItemSize(PropertyTagFrameDelay);
			std::vector<char> propitembuf(propsize);
			Gdiplus::PropertyItem *propitem = (Gdiplus::PropertyItem *)propitembuf.data();
			if (_gbitmap->GetPropertyItem(PropertyTagFrameDelay, propsize, propitem) != Gdiplus::Ok)
				return IM_FAIL;
			_framedelay.resize(framecnt);
			long totaldelay = 0;
			for (UINT i = 0; i < framecnt; ++i)
			{
				_framedelay[i] = std::max(((long *)propitem->value)[i] * 10, 0l);
				totaldelay += _framedelay[i];
			}

			// loop count
			propsize = _gbitmap->GetPropertyItemSize(PropertyTagLoopCount);
			if (propsize > propitembuf.size())
				propitembuf.resize(propsize);
			propitem = (Gdiplus::PropertyItem *)propitembuf.data();
			if (_gbitmap->GetPropertyItem(PropertyTagLoopCount, propsize, propitem) != Gdiplus::Ok)
				return IM_FAIL;
			_loopnum = *((SHORT*)propitem->value);
			if (totaldelay == 0 && _loopnum <= 0)
				_loopnum = 1;	// if totaldelay is 0, force loop only once
			TRACE("Loop count %d\n", _loopnum);
		}

		// convert ot BasicBitmap
		_fbitmap = gdiplusConvert(_gbitmap);
		if (!_fbitmap)
			return IM_FAIL;

		if (_framecnt <= 1)	// if not animated, need not keep gdiplus stuffs
		{
			delete _gbitmap;
			_gbitmap = NULL;
			if (_gstream)
			{
				_gstream->Release();
				_gstream = NULL;
			}
			delete []_filebuf;
			_filebuf = NULL;
		}

		return IM_OK;
	}

public:
	virtual ~GdiPlusImage()
	{
		//if (_hOutBitmap)
		//	DeleteObject(_hOutBitmap);
		//delete _bitmap;
		delete _fbitmap;
		delete _outBitmap;
		delete _gbitmap;
		if (_gstream)
			_gstream->Release();
	}

	virtual SIZE getDimension() const
	{
		return _dimension;
	}

	virtual int nextFrame(bool rewind=false)
	{
		if (_framecnt <= 1)
			return IM_OK;

		if (rewind)
			_curfid = 0;
		else
			++_curfid;
		if (_curfid >= _framecnt)
			return IM_NO_MORE_FRAMES;
		if (!_gbitmap)	// if animated, must have kept _gbitmap open
			return IM_FAIL;

		delete _fbitmap;
		_fbitmap = NULL;
		delete _outBitmap;
		_outBitmap = NULL;

		assert(_imgfmt == Gdiplus::ImageFormatGIF || _imgfmt == Gdiplus::ImageFormatTIFF);
		GUID pageid = _imgfmt == Gdiplus::ImageFormatGIF ? Gdiplus::FrameDimensionTime : Gdiplus::FrameDimensionPage;
		_gbitmap->SelectActiveFrame(&pageid, _curfid);

		_fbitmap = gdiplusConvert(_gbitmap);
		if (!_fbitmap)
			return IM_FAIL;

		return IM_OK;
	}

	virtual int getFrameCount() const
	{
		return _framecnt;
	}

	virtual long getFrameDelay() const
	{
		return _framedelay[_curfid];
	}

	virtual int getLoopNum() const
	{
		return _loopnum;
	}

	BasicBitmap *getBBitmap(RECT srcRect, SIZE outDim)
	{
		// if same as the one buffered, use it directly
		if (_outBitmap && srcRect == _outRect && outDim == _outDim)
			return _outBitmap;
		if (!_fbitmap)	// no opened image
			return NULL;
		if (!(srcRect.left >= 0 && srcRect.right > srcRect.left && srcRect.top >= 0 && srcRect.bottom > srcRect.top &&
			srcRect.right <= _dimension.cx && srcRect.bottom <= _dimension.cy && outDim.cx > 0 && outDim.cy > 0))
			return NULL;	// invalid input paramerters

		// if the whole image without scaling is required, just return the original bitmap
		if (srcRect.top == 0 && srcRect.left == 0 && srcRect.bottom == _dimension.cy && srcRect.right == _dimension.cx && outDim == _dimension)
			return _fbitmap;

		delete _outBitmap;	// delete the buffered one
		_outBitmap = NULL;
		_outRect = srcRect;
		_outDim = outDim;

		// crop & scale
		_outBitmap = new BasicBitmap(outDim.cx, outDim.cy, _fbitmap->Format());
		_outBitmap->Resample(0, 0, outDim.cx, outDim.cy, _fbitmap,
			srcRect.left, srcRect.top, srcRect.right, srcRect.bottom, BasicBitmap::BILINEAR);
		return _outBitmap;
	}

private:
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

void *internal_memcpy(void *dst, const void *src, size_t size);

static BasicBitmap *gdiplusConvert(Gdiplus::Bitmap *gbitmap, BasicBitmap::PixelFmt outfmt)
{
	// determine src format
	Gdiplus::PixelFormat gpixfmt = gbitmap->GetPixelFormat();
	int fmt = 0;
	int nbytes = 0;
	switch (gpixfmt)
	{
	case PixelFormat8bppIndexed:
		fmt = 8;
		nbytes = 1;
		break;
	case PixelFormat16bppRGB555:
		fmt = 555;
		nbytes = 2;
		break;
	case PixelFormat16bppRGB565:
		fmt = 565;
		nbytes = 2;
		break;
	case PixelFormat16bppARGB1555:
		fmt = 1555;
		nbytes = 2;
		break;
	case PixelFormat24bppRGB:
		fmt = 888;
		nbytes = 3;
		break;
	case PixelFormat32bppRGB:
		fmt = 888;
		nbytes = 4;
		break;
	case PixelFormat32bppARGB:
	case PixelFormat64bppARGB:
	case PixelFormat64bppPARGB:
	case PixelFormat32bppPARGB:
	default:
		gpixfmt = PixelFormat32bppARGB;
		fmt = 8888;
		nbytes = 4;
		break;
	}

	// determine parameters
	int bpp = nbytes * 8;
	UINT width = gbitmap->GetWidth();
	UINT height = gbitmap->GetHeight();
	long stride = (nbytes * width + 3) & ~3;
	void *bits = (char*)malloc(stride * height);
	if (!bits)
		return NULL;

	// get bits
	Gdiplus::Rect rect(0, 0, (int)width, (int)height);
	Gdiplus::BitmapData bmData;
	bmData.Scan0 = NULL;
	if (gbitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, gpixfmt, &bmData) != Gdiplus::Ok || bmData.PixelFormat != gpixfmt)
	{
		if (bmData.Scan0)
			gbitmap->UnlockBits(&bmData);
		free(bits);
		return NULL;
	}
	for (int i = 0; i < (int)height; i++) {
		char *src = (char*)bmData.Scan0 + bmData.Stride * i;
		char *dst = (char*)bits + stride * i;
		internal_memcpy(dst, src, stride);
	}
	gbitmap->UnlockBits(&bmData);

	// construct BasicBitmap obj
	switch (fmt) {
	case 8: fmt = BasicBitmap::G8; break;
	case 555: fmt = BasicBitmap::X1R5G5B5; break;
	case 565: fmt = BasicBitmap::R5G6B5; break;
	case 888: fmt = BasicBitmap::R8G8B8; break;
	case 8888: fmt = BasicBitmap::A8R8G8B8; break;
	default:
		fmt = BasicBitmap::UNKNOW;
		break;
	}
	if (fmt == BasicBitmap::UNKNOW) {
		free(bits);
		return NULL;
	}

	BasicBitmap *bmp = new BasicBitmap((int)width, (int)height, (BasicBitmap::PixelFmt)fmt);
	if (bmp == NULL) {
		free(bits);
		return NULL;
	}

	for (int j = 0; j < bmp->Height(); j++) {
		void *dst = bmp->Line(j);
		void *src = (char *)bits + stride * j;
		internal_memcpy(dst, src, (bmp->Bpp() + 1) / 8 * width);
	}

	free(bits);

	// convert to outfmt if necessary
	if (outfmt == BasicBitmap::UNKNOW || outfmt == bmp->Format())
		return bmp;
	BasicBitmap *cvt = new BasicBitmap(bmp->Width(), bmp->Height(), outfmt);
	if (cvt) {
		cvt->Convert(0, 0, bmp, 0, 0, bmp->Width(), bmp->Height());
	}
	delete bmp;
	return cvt;
}
