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

#include "pch.h"
#include <windows.h>
#include <gdiplus.h>
#include <ole2.h>
#include <stdint.h>
#include <memory>
#include <vector>
#include <algorithm>
#include "GdiPlusCodec.h"

#undef max

static BasicBitmap *gdiplusConvert(Gdiplus::Bitmap *gbitmap, BasicBitmap::PixelFmt outfmt = BasicBitmap::UNKNOW);

class GdiPlusImage : public Image
{
	friend GdiPlusCodec;
protected:
	// image properties
	GUID _imgfmt;
	SIZE _dimension{};
	int _framecnt = 0;
	std::vector<long> _framedelay;
	int _loopnum = 0;
	int _curfid = 0;	// current frame id
	Gdiplus::PixelFormat _oripixfmt = PixelFormat32bppARGB;  // original pixel format

	// bitmap object of current frame
	BasicBitmap *_fbitmap = NULL;

	// gdiplus bitmap object. only used for animated images, otherwise _fbitmap is sufficient
	Gdiplus::Bitmap *_gbitmap = NULL;

	uint32_t _bgcolor = 0;

	// Decode from _filebuf (must already be filled before calling).
	int openFromBuffer(uint32_t bgcolor)
	{
		_bgcolor = bgcolor;

		// Create an IStream over the in-memory buffer, then load via GDI+.
		// Use CreateStreamOnHGlobal (ole32) to wrap the buffer.
		HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, _filesize);
		if (!hg) return IM_FAIL;
		memcpy(GlobalLock(hg), _filebuf, _filesize);
		GlobalUnlock(hg);
		IStream *stream = NULL;
		if (CreateStreamOnHGlobal(hg, TRUE, &stream) != S_OK || !stream)
		{
			GlobalFree(hg);
			return IM_FAIL;
		}

		// load the image using gdi
		_gbitmap = Gdiplus::Bitmap::FromStream(stream);
		stream->Release();
		if (!_gbitmap || _gbitmap->GetLastStatus() != Gdiplus::Ok)
			return IM_FAIL;

		_dimension.cx = _gbitmap->GetWidth();
		_dimension.cy = _gbitmap->GetHeight();

		// get image format
		if (_gbitmap->GetRawFormat(&_imgfmt) != Gdiplus::Ok)
			return IM_NOT_SUPPORTED;

		// test animated
		UINT dimcnt = _gbitmap->GetFrameDimensionsCount();
		std::vector<GUID> dimids(dimcnt);
		if (_gbitmap->GetFrameDimensionsList(dimids.data(), dimcnt) != Gdiplus::Ok)
			return IM_FAIL;
		UINT framecnt = dimcnt != 0 ? _gbitmap->GetFrameCount(&dimids[0]) : 1;
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
		}

		// convert ot BasicBitmap
		_oripixfmt = _gbitmap->GetPixelFormat();
		_fbitmap = gdiplusConvert(_gbitmap);
		if (!_fbitmap)
			return IM_FAIL;
		_fbitmap->BlendColor(_bgcolor);

		if (_framecnt <= 1)	// if not animated, need not keep gdiplus stuffs
		{
			delete _gbitmap;
			_gbitmap = NULL;
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
		delete _gbitmap;
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

		assert(_imgfmt == Gdiplus::ImageFormatGIF || _imgfmt == Gdiplus::ImageFormatTIFF);
		GUID pageid = _imgfmt == Gdiplus::ImageFormatGIF ? Gdiplus::FrameDimensionTime : Gdiplus::FrameDimensionPage;
		_gbitmap->SelectActiveFrame(&pageid, _curfid);

		_fbitmap = gdiplusConvert(_gbitmap);
		if (!_fbitmap)
			return IM_FAIL;
		_fbitmap->BlendColor(_bgcolor);

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

	virtual BasicBitmap *getBBitmap(RECT srcRect, SIZE outDim)
	{
		// crop & scale
		BasicBitmap *outBitmap = new BasicBitmap(outDim.cx, outDim.cy, _fbitmap->Format());
		outBitmap->Resample(0, 0, outDim.cx, outDim.cy, _fbitmap,
			srcRect.left, srcRect.top, srcRect.right - srcRect.left, srcRect.bottom - srcRect.top, BasicBitmap::BILINEAR);
		return outBitmap;
	}

	virtual BasicBitmap* getBBitmap(SIZE scaleSize, RECT cropRect)
	{
		// scale and crop
		BasicBitmap* outBitmap = new BasicBitmap(cropRect.right - cropRect.left, cropRect.bottom - cropRect.top, _fbitmap->Format());
		//outBitmap->Fill(0, 0, cropRect.right - cropRect.left, cropRect.bottom - cropRect.top,
		//	_pixel_asm_8888(255, GetRValue(bg), GetGValue(bg), GetBValue(bg)));
		outBitmap->ScaleCropAda(0, 0, _fbitmap, scaleSize.cx, scaleSize.cy,
			cropRect.left, cropRect.top, cropRect.right - cropRect.left, cropRect.bottom - cropRect.top);
		//outBitmap->ScaleCrop(0, 0, _fbitmap, scaleSize.cx, scaleSize.cy,
		//	cropRect.left, cropRect.top, cropRect.right - cropRect.left, cropRect.bottom - cropRect.top,
		//	PIXEL_FLAG_BILINEAR, 0xffffffff, 0);
		return outBitmap;
	}

	virtual const wchar_t *getFormat() const
	{
		if (_imgfmt == Gdiplus::ImageFormatGIF)
			return L"gif";
		else if (_imgfmt == Gdiplus::ImageFormatTIFF)
			return L"tiff";
		else if (_imgfmt == Gdiplus::ImageFormatBMP)
			return L"bmp";
		else if (_imgfmt == Gdiplus::ImageFormatEMF)
			return L"emf";
		else if (_imgfmt == Gdiplus::ImageFormatWMF)
			return L"wmf";
		else if (_imgfmt == Gdiplus::ImageFormatJPEG)
			return L"jpeg";
		else if (_imgfmt == Gdiplus::ImageFormatPNG)
			return L"png";
		else if (_imgfmt == Gdiplus::ImageFormatIcon)
			return L"icon";
		return L"UNKNOWN";
	}

	virtual int getCurFrame() const { return _curfid; }

	virtual const wchar_t *getColorInfo() const
	{
		switch (_oripixfmt)
		{
		case PixelFormat32bppARGB:   return L"ARGB32";
		case PixelFormat32bppPARGB:  return L"ARGB32";
		case PixelFormat64bppARGB:   return L"ARGB64";
		case PixelFormat64bppPARGB:  return L"ARGB64";
		case PixelFormat32bppRGB:    return L"RGB32";
		case PixelFormat24bppRGB:    return L"RGB24";
		case PixelFormat48bppRGB:    return L"RGB48";
		case PixelFormat16bppRGB565: return L"RGB16";
		case PixelFormat16bppRGB555: return L"RGB16";
		case PixelFormat16bppARGB1555: return L"ARGB16";
		case PixelFormat16bppGrayScale: return L"Gray16";
		case PixelFormat8bppIndexed: return L"Index8";
		case PixelFormat4bppIndexed: return L"Index4";
		case PixelFormat1bppIndexed: return L"Index1";
		default: return L"";
		}
	}
};

GdiPlusCodec::GdiPlusCodec()
{
}

GdiPlusCodec::~GdiPlusCodec()
{
}

int GdiPlusCodec::open(UniFile *f, Image **image, uint32_t bgcolor)
{
	*image = NULL;
	std::unique_ptr<GdiPlusImage> pimg(new GdiPlusImage());
	// Load from UniFile directly: readFromUniFile fills _filebuf, then open() uses it.
	int res = pimg->readFromUniFile(f);
	if (res != IM_OK)
		return res;
	res = pimg->openFromBuffer(bgcolor);
	if (res != IM_OK)
		return res;
	*image = pimg.release();
	return IM_OK;
}

void *internal_memcpy(void *dst, const void *src, size_t size);

static BasicBitmap *gdiplusConvert(Gdiplus::Bitmap *gbitmap, BasicBitmap::PixelFmt outfmt)
{
	// determine src & dst format
	if (outfmt == BasicBitmap::UNKNOW)
	{
		Gdiplus::PixelFormat orifmt = gbitmap->GetPixelFormat();
		switch (orifmt)
		{
		case PixelFormat32bppARGB:
			outfmt = BasicBitmap::A8R8G8B8;
			break;
		case PixelFormat24bppRGB:
			outfmt = BasicBitmap::R8G8B8;
			break;
		case PixelFormat16bppRGB565:
			outfmt = BasicBitmap::R5G6B5;
			break;
		case PixelFormat16bppARGB1555:
			outfmt = BasicBitmap::A1R5G5B5;
			break;
		case PixelFormat16bppRGB555:
			outfmt = BasicBitmap::X1R5G5B5;
			break;
		case PixelFormat32bppPARGB:
		case PixelFormat64bppARGB:
		case PixelFormat64bppPARGB:
			outfmt = BasicBitmap::A8R8G8B8;
			break;
		default:
			outfmt = BasicBitmap::R8G8B8;
			break;
		}
	}

	Gdiplus::PixelFormat getfmt = PixelFormat24bppRGB;
	int fmt = 0;
	int nbytes = 0;
	switch (outfmt)
	{
	case BasicBitmap::A8R8G8B8:
		getfmt = PixelFormat32bppARGB;
		fmt = 8888;
		nbytes = 4;
		break;
	case BasicBitmap::R8G8B8:
		getfmt = PixelFormat24bppRGB;
		fmt = 888;
		nbytes = 3;
		break;
	case BasicBitmap::R5G6B5:
		getfmt = PixelFormat16bppRGB565;
		fmt = 565;
		nbytes = 2;
		break;
	case BasicBitmap::A1R5G5B5:
		getfmt = PixelFormat16bppARGB1555;
		fmt = 1555;
		nbytes = 2;
		break;
	case BasicBitmap::X1R5G5B5:
		getfmt = PixelFormat16bppRGB555;
		fmt = 555;
		nbytes = 2;
		break;
	case BasicBitmap::A8B8G8R8:
	case BasicBitmap::X8R8G8B8:
	case BasicBitmap::A4R4G4B4:
	default:	// not supported by GdiPlus
		return NULL;
	}

	// determine parameters
	int bpp = nbytes * 8;
	UINT width = gbitmap->GetWidth();
	UINT height = gbitmap->GetHeight();
	long stride = (nbytes * width + 3) & ~3;

	std::unique_ptr<BasicBitmap> bmp(new BasicBitmap((int)width, (int)height, outfmt));
	if (!bmp)
		return NULL;

	// get bits
	int linestep = std::max(1, (int)((1024 * 1024 * 10) / stride));	// approx. 10MB per block to limit intermediate mem usage
	//linestep = height;
	Gdiplus::BitmapData bmData = { 0 };
	for (int i = 0; i < (int)height; i += linestep)
	{
		int blockh = std::min<>(linestep, (int)height - i);
		Gdiplus::Rect rect(0, i, (int)width, blockh);
		bmData.Height = blockh;
		bmData.Scan0 = NULL;
		if (gbitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, getfmt, &bmData) != Gdiplus::Ok || bmData.PixelFormat != getfmt)
		{
			if (bmData.Scan0)
				gbitmap->UnlockBits(&bmData);
			return NULL;
		}
		for (int j = 0; j < blockh; j++)
		{
			void* dst = bmp->Line(i + j);
			char* src = (char*)bmData.Scan0 + bmData.Stride * j;
			internal_memcpy(dst, src, stride);
		}
		gbitmap->UnlockBits(&bmData);
	}

	//// convert to outfmt if necessary
	//if (outfmt == BasicBitmap::UNKNOW || outfmt == bmp->Format())
	//	return bmp.release();
	//BasicBitmap *cvt = new BasicBitmap(bmp->Width(), bmp->Height(), outfmt);
	//if (cvt) {
	//	cvt->Convert(0, 0, bmp.get(), 0, 0, bmp->Width(), bmp->Height());
	//}
	//return cvt;
	return bmp.release();
}
