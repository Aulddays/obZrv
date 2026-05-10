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
#include "WndBase.h"

WndBase::WndBase()
    : m_hwnd(NULL)
{
}

WndBase::~WndBase()
{
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

/* Default message handler - falls through to DefWindowProc */
LRESULT WndBase::HandleMessage(UINT msg, WPARAM wp, LPARAM lp)
{
    return DefWindowProc(m_hwnd, msg, wp, lp);
}

/*
 * Static WndProc registered with the window class.
 *
 * On WM_NCCREATE the 'this' pointer (passed via lpCreateParams) is stored in
 * GWLP_USERDATA so that subsequent messages can be routed to the object.
 */
LRESULT CALLBACK WndBase::WndProcStatic(HWND hwnd, UINT msg,
                                        WPARAM wp, LPARAM lp)
{
    WndBase *self = NULL;

    if (msg == WM_NCCREATE) {
        CREATESTRUCT *cs = reinterpret_cast<CREATESTRUCT *>(lp);
        self = reinterpret_cast<WndBase *>(cs->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA,
                         reinterpret_cast<LONG_PTR>(self));
        self->m_hwnd = hwnd;
    } else {
        self = reinterpret_cast<WndBase *>(
            GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (self)
        return self->HandleMessage(msg, wp, lp);

    return DefWindowProc(hwnd, msg, wp, lp);
}
