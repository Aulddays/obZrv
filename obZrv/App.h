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

/*
 * App - application singleton.
 *
 * Holds the HINSTANCE, creates the main window and runs the message loop.
 */
class App
{
public:
	explicit App(HINSTANCE hInst);
	~App();

	/* Create window and run message loop. Returns process exit code. */
	int Run(int nShow, const wchar_t *initPath = NULL);

private:
	HINSTANCE  m_hInst;
	ULONG_PTR  m_gdipToken;
};
