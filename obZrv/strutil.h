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
#include <algorithm>
#include <cctype>
#include <string>
#include <string.h>

// Convert wchar_t (UTF-16) string to UTF-8 std::string.
inline std::string wstr_to_utf8(const wchar_t* s)
{
	if (!s || !*s) return std::string();
	int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 0) return std::string();
	std::string out(len - 1, '\0');
	WideCharToMultiByte(CP_UTF8, 0, s, -1, &out[0], len, nullptr, nullptr);
	return out;
}

// Convert UTF-8 string to wchar_t (UTF-16) std::wstring.
inline std::wstring utf8_to_wstr(const char* s)
{
	if (!s || !*s) return std::wstring();
	int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
	if (len <= 0) return std::wstring();
	std::wstring out(len - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s, -1, &out[0], len);
	return out;
}

inline std::string getExt(const char *path)
{
	if (!path)
		return std::string();
	const char *sep = strrchr(path, '/');
	const char *name = sep ? sep + 1 : path;
	const char *dot = strrchr(name, '.');
	if (!dot || !*(dot + 1))
		return std::string();
	std::string ext(dot + 1);
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return (char)tolower(c); });
	return ext;
}

// Convert Windows absolute path (C:\dir\file) to Unix-style UTF-8 (/c/dir/file).
// Paths already in Unix style or remote:// URLs are passed through unchanged.
inline std::string to_unipath(const wchar_t* s)
{
	if (!s || !*s) return std::string();
	// Already Unix style or remote path: just convert encoding
	if (s[0] == L'/' || s[0] == L'r') return wstr_to_utf8(s);
	// Detect drive letter: "X:\" or "X:/"
	if (((s[0] >= L'a' && s[0] <= L'z') || (s[0] >= L'A' && s[0] <= L'Z'))
		&& s[1] == L':')
	{
		std::wstring out;
		out.reserve(wcslen(s) + 1);
		out += L'/';
		out += (wchar_t)towlower(s[0]);  // drive letter lowercase
		// skip "X:" prefix, copy rest converting '\' to '/'
		for (const wchar_t* p = s + 2; *p; ++p)
			out += (*p == L'\\') ? L'/' : *p;
		return wstr_to_utf8(out.c_str());
	}
	// Fallback: return as-is
	return wstr_to_utf8(s);
}
