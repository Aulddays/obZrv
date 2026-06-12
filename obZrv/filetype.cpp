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
#include "filetype.h"
#include <stdio.h>
#include <string.h>

static bool match(const unsigned char *buf, size_t size, size_t off, const char *magic, size_t len)
{
	return size >= off + len && memcmp(buf + off, magic, len) == 0;
}

static bool isFtypBrand(const unsigned char *buf, size_t size, const char *brand)
{
	if (!match(buf, size, 4, "ftyp", 4))
		return false;
	for (size_t off = 8; off + 4 <= size && off < 64; off += 4)
	{
		if (memcmp(buf + off, brand, 4) == 0)
			return true;
	}
	return false;
}

std::string filetype(UniFile *f)
{
	if (!f)
		return "UNK";

	int64_t pos = f->tell();
	if (f->seek(0, SEEK_SET) != 0)
		return "UNK";

	unsigned char buf[512] = {};
	size_t size = f->read(buf, sizeof(buf));
	if (pos >= 0)
		f->seek(pos, SEEK_SET);

	if (size == 0)
		return "UNK";

	if (size >= 3 && buf[0] == 0xff && buf[1] == 0xd8 && buf[2] == 0xff)
		return "jpg";
	if (match(buf, size, 0, "\x89PNG\r\n\x1a\n", 8))
		return "png";
	if (match(buf, size, 0, "GIF87a", 6) || match(buf, size, 0, "GIF89a", 6))
		return "gif";
	if (match(buf, size, 0, "BM", 2))
		return "bmp";
	if ((match(buf, size, 0, "II*\0", 4)) || (match(buf, size, 0, "MM\0*", 4)))
		return "tiff";
	if (match(buf, size, 0, "\0\0\1\0", 4))
		return "ico";
	if (match(buf, size, 0, "RIFF", 4) && match(buf, size, 8, "WEBP", 4))
		return "webp";

	if (match(buf, size, 0, "\x00\x00\x00\x0cjP  \r\n\x87\n", 12) || isFtypBrand(buf, size, "jp2 "))
		return "jp2";
	if (match(buf, size, 0, "\xff\x0a", 2) || match(buf, size, 0, "\0\0\0\x0cJXL \r\n\x87\n", 12))
		return "jxl";
	if (match(buf, size, 0, "8BPS", 4))
		return "psd";
	if (match(buf, size, 0, "P1", 2) || match(buf, size, 0, "P2", 2) ||
		match(buf, size, 0, "P3", 2) || match(buf, size, 0, "P4", 2) ||
		match(buf, size, 0, "P5", 2) || match(buf, size, 0, "P6", 2) ||
		match(buf, size, 0, "P7", 2))
		return "pnm";
	if (isFtypBrand(buf, size, "avif") || isFtypBrand(buf, size, "avis"))
		return "avif";
	if (isFtypBrand(buf, size, "heic") || isFtypBrand(buf, size, "heix") ||
		isFtypBrand(buf, size, "hevc") || isFtypBrand(buf, size, "hevx") ||
		isFtypBrand(buf, size, "mif1") || isFtypBrand(buf, size, "msf1"))
		return "heic";
	if (match(buf, size, 0, "DDS ", 4))
		return "dds";
	if (match(buf, size, 0, "\x76\x2f\x31\x01", 4))
		return "exr";
	if (match(buf, size, 0, "#?RADIANCE", 10) || match(buf, size, 0, "#?RGBE", 6))
		return "hdr";
	if (match(buf, size, 0, "qoif", 4))
		return "qoi";
	if (match(buf, size, 0, "farbfeld", 8))
		return "ff";
	if (match(buf, size, 0, "icns", 4))
		return "icns";
	if (match(buf, size, 0, "\0\0\2\0", 4))
		return "cur";
	if (match(buf, size, 0, "\xd7\xcd\xc6\x9a", 4))
		return "wmf";
	if (match(buf, size, 40, " EMF", 4))
		return "emf";

	return "UNK";
}
