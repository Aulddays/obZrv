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
#include <string>

/* Application configuration backed by an INI file.
 *
 * Write path: <exe_dir>\obz.ini if that directory is writable,
 *             otherwise %APPDATA%\obz\obz.ini.
 *
 * Read path:  <exe_dir>\obz.ini first; if the key is absent, fall back
 *             to %APPDATA%\obz\obz.ini.
 *
 * Usage: Config::instance().getInt(...) / setInt(...) */
class Config
{
public:
    static Config &instance();

    int  getInt(const wchar_t *sect, const wchar_t *key, int def = 0) const;
    void setInt(const wchar_t *sect, const wchar_t *key, int val);

    std::wstring getStr(const wchar_t *sect, const wchar_t *key,
                        const wchar_t *def = L"") const;
    void         setStr(const wchar_t *sect, const wchar_t *key,
                        const wchar_t *val);

private:
    Config();

    std::wstring m_exeIni;    /* <exe_dir>\obz.ini              */
    std::wstring m_userIni;   /* %APPDATA%\obz\obz.ini          */
    std::wstring m_writeIni;  /* effective write path           */
};
