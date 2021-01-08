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

#include "stdafx.h"
#include "WebpCodec.h"
#include <memory>
#include "webp/decode.h"

// determine the libwebp version to link against
#if !defined(_M_X64)	// x86
#	if !defined(_DLL)	// static crt
#		if !defined(_DEBUG)	// release
#			pragma comment(lib, "webp/libwebpdecoder-MT.lib")
#		else	// debug
#			pragma comment(lib, "webp/libwebpdecoder-MTd.lib")
#		endif	// _DEBUG
#	endif	// _DLL
#endif	// _M_X64

class WebpImage : public Image
{
	friend WebpCodec;
protected:
	SIZE _dimension{};

	// bitmap object of current frame
	BasicBitmap *_fbitmap = NULL;

	// (resized) output bitmap of current frame
	BasicBitmap *_outBitmap = NULL;
	RECT _outRect;
	SIZE _outDim;

protected:
	int open(const wchar_t *filename)
	{
		int res = readFile(filename);
		if (res != IM_OK)
			return res;
		WebPBitstreamFeatures imgfea;
		if (WebPGetFeatures(_filebuf, _filesize, &imgfea) != VP8_STATUS_OK)
			return IM_FAIL;
		std::unique_ptr<BasicBitmap> bmp = std::make_unique<BasicBitmap>(imgfea.width, imgfea.height,
			imgfea.has_alpha ? BasicBitmap::A8R8G8B8 : BasicBitmap::R8G8B8);
		if (imgfea.has_alpha &&
			!WebPDecodeARGBInto(_filebuf, _filesize, bmp->Bits(), bmp->Pitch() * bmp->Height(), bmp->Pitch()) ||
			!imgfea.has_alpha &&
			!WebPDecodeRGBInto(_filebuf, _filesize, bmp->Bits(), bmp->Pitch() * bmp->Height(), bmp->Pitch()))
			return IM_FAIL;

		_fbitmap = bmp.release();
		_dimension = SIZE{ imgfea.width, imgfea.height };

		return IM_OK;
	}

public:
	virtual ~WebpImage()
	{
		delete _fbitmap;
		delete _outBitmap;
	}

	virtual SIZE getDimension() const
	{
		return _dimension;
	}

	virtual int getFrame(int idx)
	{
		return IM_OK;
	}

	virtual BasicBitmap *getBBitmap(RECT srcRect, SIZE outDim)
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

	virtual int getFrameCount() const
	{
		return 0;
	}

	virtual long getFrameDelay(int fid) const
	{
		return 0;
	}

	virtual int getLoopNum() const
	{
		return 0;
	}
};


WebpCodec::WebpCodec()
{
}

WebpCodec::~WebpCodec()
{
}

int WebpCodec::open(const wchar_t *filename, Image ** image)
{
	*image = NULL;
	std::unique_ptr<WebpImage> pimg = std::make_unique<WebpImage>();
	int res = pimg->open(filename);
	if (res != IM_OK)
		return res;
	*image = pimg.release();
	return res;
}
