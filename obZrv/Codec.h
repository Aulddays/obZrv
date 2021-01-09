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

#pragma once

#include "BasicBitmap\BasicBitmap.h"

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

	// impage properties
	virtual SIZE getDimension() const = 0;
	virtual bool isAnim() const { return getFrameCount() > 1; }

	// Get transformed bitmap of current frame
	virtual BasicBitmap *getBBitmap(RECT srcRect, SIZE outSize) = 0;

	// animation properties
	virtual int getLoopNum() const = 0;
	virtual int getFrameCount() const = 0;
	// decode the next frame
	virtual int nextFrame(bool rewind=false) = 0;
	// get time delay of current frame, in ms
	virtual long getFrameDelay() const = 0;

protected:
	// image file buffer
	unsigned char *_filebuf = NULL;
	size_t _filesize = 0;

	int readFile(const wchar_t *fn)
	{
		assert(_filebuf == NULL);
		FILE *fp = _wfopen(fn, L"rb");
		if (!fp)
			return IM_READFILE_ERR;
		fseek(fp, 0, SEEK_END);
		_filesize = ftell(fp);
		_filebuf = new unsigned char[_filesize];
		fseek(fp, 0, SEEK_SET);
		size_t readlen = fread(_filebuf, 1, _filesize, fp);
		fclose(fp);
		if (readlen != _filesize)
			return IM_READFILE_ERR;
		return IM_OK;
	}



};

class Codec
{
public:
	Codec() { };
	virtual ~Codec() { };

public:
	virtual int init() { return 0; };
	//virtual int release() = 0;
	virtual int open(const wchar_t *filename, Image ** image) = 0;

protected:

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