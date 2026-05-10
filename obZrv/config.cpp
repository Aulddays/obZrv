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
#include "config.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4091)
#endif
#include <shlobj.h>
#ifdef _MSC_VER
#pragma warning(pop)
#endif

/* ---------- path helpers ------------------------------------------ */

static std::wstring ExeDir()
{
	wchar_t buf[MAX_PATH];
	GetModuleFileNameW(NULL, buf, MAX_PATH);
	wchar_t *slash = wcsrchr(buf, L'\\');
	if (slash) *(slash + 1) = L'\0';
	return buf;
}

static std::wstring UserDir()
{
	wchar_t buf[MAX_PATH] = {};
	if (SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, buf) != S_OK)
		GetTempPathW(MAX_PATH, buf);   /* fallback: %TEMP% */
	return std::wstring(buf) + L"\\obZrv\\";
}

/* Test whether we can open (or create) the given file for writing. */
static bool CanWrite(const std::wstring &path)
{
	HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE,
						   FILE_SHARE_READ | FILE_SHARE_WRITE,
						   NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return false;
	/* If we just created an empty file, delete it so we don't leave litter. */
	if (GetLastError() == 0 && GetFileSize(h, NULL) == 0) {
		CloseHandle(h);
		DeleteFileW(path.c_str());
	} else {
		CloseHandle(h);
	}
	return true;
}

/* ---------- Config ------------------------------------------------- */

Config::Config()
{
	m_exeIni  = ExeDir()  + L"obZrv.ini";
	m_userIni = UserDir() + L"obZrv.ini";

	if (CanWrite(m_exeIni)) {
		m_writeIni = m_exeIni;
	} else {
		/* Ensure %APPDATA%\obZrv\ exists before writing there. */
		CreateDirectoryW(UserDir().c_str(), NULL);
		m_writeIni = m_userIni;
	}
}

/* static */
Config &Config::instance()
{
	static Config c;
	return c;
}

/* Read from <exe_dir> first; if the key is absent, fall back to user dir. */
int Config::getInt(const wchar_t *sect, const wchar_t *key, int def) const
{
	/* Use a sentinel string to distinguish "key absent" from "value == def". */
	const wchar_t sentinel[] = L"\x01\x02NOTFOUND";
	wchar_t buf[64];

	if (!m_exeIni.empty()) {
		DWORD n = GetPrivateProfileStringW(sect, key, sentinel,
										   buf, 64, m_exeIni.c_str());
		if (n > 0 && wcscmp(buf, sentinel) != 0)
			return (int)wcstol(buf, NULL, 10);
	}

	return (int)GetPrivateProfileIntW(sect, key, (INT)def, m_userIni.c_str());
}

void Config::setInt(const wchar_t *sect, const wchar_t *key, int val)
{
	wchar_t buf[32];
	wsprintfW(buf, L"%d", val);
	WritePrivateProfileStringW(sect, key, buf, m_writeIni.c_str());
}

std::wstring Config::getStr(const wchar_t *sect, const wchar_t *key,
							 const wchar_t *def) const
{
	const wchar_t sentinel[] = L"\x01\x02NOTFOUND";
	wchar_t buf[512];

	if (!m_exeIni.empty()) {
		DWORD n = GetPrivateProfileStringW(sect, key, sentinel,
										   buf, 512, m_exeIni.c_str());
		if (n > 0 && wcscmp(buf, sentinel) != 0)
			return buf;
	}

	DWORD n = GetPrivateProfileStringW(sect, key, sentinel,
									   buf, 512, m_userIni.c_str());
	if (n > 0 && wcscmp(buf, sentinel) != 0)
		return buf;

	return def;
}

void Config::setStr(const wchar_t *sect, const wchar_t *key, const wchar_t *val)
{
	WritePrivateProfileStringW(sect, key, val, m_writeIni.c_str());
}
