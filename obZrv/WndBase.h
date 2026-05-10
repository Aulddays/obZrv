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
 * WndBase - base class for all window objects.
 *
 * Subclasses override HandleMessage() to process messages.
 * Unhandled messages fall through to DefWindowProc via the base implementation.
 *
 * Usage:
 *   1. Call RegisterClass() with WndProcStatic as lpfnWndProc
 *   2. Pass 'this' as lpCreateParams in CreateWindow/CreateWindowEx
 *   3. Override HandleMessage() and use a switch-case for desired messages
 */
class WndBase
{
public:
	WndBase();
	virtual ~WndBase();

	HWND hwnd() const { return m_hwnd; }
	bool isValid() const { return m_hwnd != NULL; }

	/* Static WndProc - routes messages to the object's HandleMessage() */
	static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

protected:
	HWND m_hwnd;

	/* Override in subclasses to handle messages.
	 * Always call the base class for unhandled messages. */
	virtual LRESULT HandleMessage(UINT msg, WPARAM wp, LPARAM lp);
};
