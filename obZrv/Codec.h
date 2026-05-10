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
#include <stdio.h>
#include <stdint.h>
#include "BasicBitmap/BasicBitmap.h"
#include "../unifs/unifile.hpp"

enum IM_ErrorCodes
{
	IM_OK = 0,
	IM_FAIL,
	IM_TOO_LARGE,
	IM_NOT_SUPPORTED,
	IM_READFILE_ERR,
	IM_NO_MORE_FRAMES,
};

class Image
{
public:
	virtual ~Image() { delete[] _filebuf; }

	// image properties
	virtual SIZE getDimension() const = 0;
	virtual bool isAnim() const { return getFrameCount() > 1; }
	virtual const wchar_t *getFormat() const = 0;

	// Get transformed bitmap of current frame
	virtual BasicBitmap *getBBitmap(RECT srcRect, SIZE outSize) = 0;
	virtual BasicBitmap* getBBitmap(SIZE scaleSize, RECT cropRect) = 0;

	// animation properties
	virtual int getLoopNum() const = 0;
	virtual int getFrameCount() const = 0;
	virtual int getCurFrame() const { return 0; }
	virtual const wchar_t *getColorInfo() const { return L""; }
	virtual int nextFrame(bool rewind=false) = 0;
	virtual long getFrameDelay() const = 0;

protected:
	// image file buffer
	unsigned char *_filebuf = NULL;
	size_t _filesize = 0;

	// Read all data from a UniFile into _filebuf/_filesize.
	int readFromUniFile(UniFile* f)
	{
		assert(_filebuf == NULL);
		if (f->seek(0, SEEK_END) != 0) return IM_READFILE_ERR;
		int64_t sz = f->tell();
		if (sz <= 0) return IM_READFILE_ERR;
		_filesize = (size_t)sz;
		_filebuf = new unsigned char[_filesize];
		if (f->seek(0, SEEK_SET) != 0) { delete[] _filebuf; _filebuf = NULL; return IM_READFILE_ERR; }
		size_t got = f->read(_filebuf, _filesize);
		if (got != _filesize) { delete[] _filebuf; _filebuf = NULL; return IM_READFILE_ERR; }
		return IM_OK;
	}
};

class Codec
{
public:
	Codec() { };
	virtual ~Codec() { };

	virtual int init() { return 0; };

	// Open from an already-open UniFile.
	virtual int open(UniFile *f, Image **image, uint32_t bgcolor) = 0;
};

#define MAX_IMAGE_DIMENSION 65535
#define MAX_IMAGE_PIXELS ((unsigned int)1024 * 1024 * 100)

inline bool operator ==(const RECT &l, const RECT &r)
{
	return l.left == r.left && l.right == r.right && l.top == r.top && l.bottom == r.bottom;
}

inline bool operator ==(const SIZE &l, const SIZE &r)
{
	return l.cx == r.cx && l.cy == r.cy;
}
