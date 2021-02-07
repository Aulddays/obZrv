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
#include <algorithm>
#include "webp/decode.h"
#include "webp/demux.h"

// determine the libwebp version to link against
#if !defined(_M_X64)	// x86
#	if !defined(_DLL)	// static crt
#		if !defined(_DEBUG)	// release
#			pragma comment(lib, "webp/libwebpdecoder-MT.lib")
#			pragma comment(lib, "webp/libwebpdemux-MT.lib")
#		else	// debug
#			pragma comment(lib, "webp/libwebpdecoder-MTd.lib")
#			pragma comment(lib, "webp/libwebpdemux_MTd.lib")
#		endif	// _DEBUG
#	endif	// _DLL
#endif	// _M_X64

#undef max
void *internal_memcpy(void *dst, const void *src, size_t size);

class WebpImage : public Image
{
	friend WebpCodec;
protected:
	SIZE _dimension{};

	// bitmap object of current frame
	BasicBitmap *_fbitmap = NULL;

	// animation properties
	int _framecnt = 0;
	int _frametm = 0;
	int _lframetm = 0;
	int _loopnum = 0;
	int _curfid = 0;	// current frame id
	WebPAnimDecoder* _animdec = NULL;

protected:
	int open(const wchar_t *filename)
	{
		int res = readFile(filename);
		if (res != IM_OK)
			return res;
		WebPBitstreamFeatures imgfea;
		if (WebPGetFeatures(_filebuf, _filesize, &imgfea) != VP8_STATUS_OK)
			return IM_FAIL;

		if (!imgfea.has_animation)
		{
			// not animated, just decode and buffer it
			std::unique_ptr<BasicBitmap> bmp = std::make_unique<BasicBitmap>(imgfea.width, imgfea.height,
				imgfea.has_alpha ? BasicBitmap::A8R8G8B8 : BasicBitmap::R8G8B8);
			if (imgfea.has_alpha &&
				!WebPDecodeARGBInto(_filebuf, _filesize, bmp->Bits(), bmp->Pitch() * bmp->Height(), bmp->Pitch()) ||
				!imgfea.has_alpha &&
				!WebPDecodeRGBInto(_filebuf, _filesize, bmp->Bits(), bmp->Pitch() * bmp->Height(), bmp->Pitch()))
				return IM_FAIL;

			_fbitmap = bmp.release();
			_dimension = SIZE{ imgfea.width, imgfea.height };

			// we dont need the file buffer since we have saved the decoded image
			delete[] _filebuf;
			_filebuf = NULL;

			return IM_OK;
		}

		// process the animated image
		// create decoder
		WebPData wpdata = { _filebuf, _filesize };
		WebPAnimDecoderOptions opt = { MODE_BGRA, true };
		_animdec = WebPAnimDecoderNew(&wpdata, &opt);
		if (!_animdec)
			return IM_FAIL;
		// get info
		WebPAnimInfo animinfo;
		if (!WebPAnimDecoderGetInfo(_animdec, &animinfo))
			return IM_FAIL;
		_framecnt = animinfo.frame_count;
		_loopnum = animinfo.loop_count;
		_dimension = { animinfo.canvas_width, animinfo.canvas_height };
		// get first frame
		res = nextFrame(true);
		if (res != IM_OK)
			return res;
		if (_framecnt <= 1)
		{
			// if only one frame, we do not need the decoder or file buffer any more
			WebPAnimDecoderDelete(_animdec);
			_animdec = NULL;
			delete[] _filebuf;
			_filebuf = NULL;
		}
		return IM_OK;
	}

public:
	virtual ~WebpImage()
	{
		delete _fbitmap;
		WebPAnimDecoderDelete(_animdec);
	}

	virtual SIZE getDimension() const
	{
		return _dimension;
	}

	virtual BasicBitmap *getBBitmap(RECT srcRect, SIZE outDim)
	{
		// crop & scale
		BasicBitmap *outBitmap = new BasicBitmap(outDim.cx, outDim.cy, _fbitmap->Format());
		outBitmap->Resample(0, 0, outDim.cx, outDim.cy, _fbitmap,
			srcRect.left, srcRect.top, srcRect.right - srcRect.left, srcRect.bottom - srcRect.top, BasicBitmap::BILINEAR);
		return outBitmap;
	}

	virtual int getFrameCount() const
	{
		return _framecnt;
	}

	virtual long getFrameDelay() const
	{
		return std::max(0, _frametm - _lframetm);
	}

	virtual int getLoopNum() const
	{
		return _loopnum;
	}

	int nextFrame(bool rewind = false)
	{
		if (!isAnim())
			return IM_OK;

		if (rewind)
		{
			WebPAnimDecoderReset(_animdec);
			_frametm = _lframetm = 0;
		}
		if (!WebPAnimDecoderHasMoreFrames(_animdec))
			return IM_NO_MORE_FRAMES;
		uint8_t *decbits = NULL;
		_lframetm = _frametm;
		if (!WebPAnimDecoderGetNext(_animdec, &decbits, &_frametm))
			return IM_FAIL;

		// copy the image data into _fbitmap
		if (_fbitmap && (
			_fbitmap->Width() != _dimension.cx || _fbitmap->Height() != _dimension.cy ||
			_fbitmap->Format() != BasicBitmap::A8R8G8B8))
		{
			delete _fbitmap;
			_fbitmap = NULL;
		}
		if (!_fbitmap)
			_fbitmap = new BasicBitmap(_dimension.cx, _dimension.cy, BasicBitmap::A8R8G8B8);
		int linelen = _dimension.cx * 4;
		for (int i = 0; i < _fbitmap->Height(); ++i) {
			void *dst = _fbitmap->Line(i);
			void *src = decbits + linelen * i;
			internal_memcpy(dst, src, linelen);
		}
		return IM_OK;
	}
	virtual const wchar_t *getFormat() const
	{
		return L"webp";
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
