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

class WebpImage : public Image
{
	friend WebpCodec;

protected:
	int open(const wchar_t *filename)
	{
		return IM_NOT_SUPPORTED;
	}

public:
	virtual ~WebpImage()
	{
	}

	virtual SIZE getDimension() const
	{
		return {-1, -1};
	}

	virtual int getFrame(int idx)
	{
		return IM_NOT_SUPPORTED;
	}

	virtual BasicBitmap *getBBitmap(RECT srcRect, SIZE outSize)
	{
		return NULL;
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
