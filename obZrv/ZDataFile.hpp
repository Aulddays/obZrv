#pragma once

#include <stdint.h>
#include <stddef.h>
#include <memory>
#include <string>
#include <utility>
#include "../unifs/unifile.hpp"

class ZDataFile : public UniFile
{
public:
	explicit ZDataFile(std::unique_ptr<UniFile> inner)
		: _inner(std::move(inner))
	{
		int64_t pos = _inner ? _inner->tell() : -1;
		if (pos > 0)
		{
			_pos = (uint64_t)pos;
			_seed = seedAfter(_pos);
		}
	}

	int seek(int64_t offset, int whence) override
	{
		if (!_inner || _inner->seek(offset, whence) != 0)
			return -1;

		int64_t pos = _inner->tell();
		if (pos < 0)
			return -1;

		_pos = (uint64_t)pos;
		_seed = seedAfter(_pos);
		return 0;
	}

	int64_t tell() override
	{
		if (!_inner)
			return -1;
		return _inner->tell();
	}

	size_t read(void* buf, size_t size) override
	{
		if (!_inner || !buf || size == 0)
			return 0;

		size_t n = _inner->read(buf, size);
		uint8_t* data = (uint8_t*)buf;
		for (size_t i = 0; i < n; ++i)
		{
			_seed = _seed * 22695477u + 1u;
			data[i] ^= (uint8_t)(_seed >> 16);
		}
		_pos += n;
		return n;
	}

	size_t write(const void* /*buf*/, size_t /*size*/) override
	{
		return 0;
	}

	int close() override
	{
		return _inner ? _inner->close() : 0;
	}

	static bool isZDataExt(const std::string& ext)
	{
		return ext.size() == 3 && ext[0] == 'z' &&
			ext[1] >= '0' && ext[1] <= '9' &&
			ext[2] >= '0' && ext[2] <= '9';
	}

private:
	static uint32_t seedAfter(uint64_t count)
	{
		uint32_t mul = 22695477u;
		uint32_t add = 1u;
		uint32_t accMul = 1u;
		uint32_t accAdd = 0u;

		while (count != 0)
		{
			if ((count & 1) != 0)
			{
				accMul = accMul * mul;
				accAdd = accAdd * mul + add;
			}
			add = add * (mul + 1u);
			mul = mul * mul;
			count >>= 1;
		}

		return accMul * 32093u + accAdd;
	}

	std::unique_ptr<UniFile> _inner;
	uint64_t _pos = 0;
	uint32_t _seed = 32093u;
};
