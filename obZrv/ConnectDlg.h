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
#include <memory>
#include <string>
#include <vector>
#include <stdint.h>

class UniFsClient;  // forward declaration

/* ConnectDlg - modal dialog to connect to a remote UniFsServer.
 *
 * Usage:
 *   ConnectDlg dlg;
 *   auto client = dlg.DoModal(hParent, host_out, port_out);
 *   if (client) { ... }  // nullptr on cancel or failure
 */
class ConnectDlg
{
public:
    /* Show the dialog.  On success returns a connected UniFsClient and fills
     * host/port; on cancel or connection failure returns nullptr. */
    std::unique_ptr<UniFsClient> DoModal(HWND hParent,
                                         std::string &host, uint16_t &port);

private:
    HWND m_hwnd = NULL;

    static INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
    INT_PTR HandleMessage(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);

    /* Filled by IDOK handler before EndDialog */
    std::string  m_host;
    uint16_t     m_port   = 0;
    std::unique_ptr<UniFsClient> m_client;  // set on successful connect
};
