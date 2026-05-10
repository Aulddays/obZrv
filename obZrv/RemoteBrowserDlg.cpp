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
#include <winsock2.h>
#include <windows.h>
#include <shlwapi.h>
#include "RemoteBrowserDlg.h"
#include "../unifs/client.hpp"
#include "resource.h"
#include "strutil.h"
#include <commctrl.h>
#include <vector>
#include <string>
#include <algorithm>

// Entry stored per list-box item: name + is_dir flag.
struct Entry {
    std::string name;
    bool        is_dir;
};

// Per-dialog heap data (avoids storing STL in the class across DlgProc static).
struct BrowserData {
    RemoteBrowserDlg*  dlg;
    std::vector<Entry> entries;  // current directory listing
    HFONT              hListFont = NULL;  // font for the listbox (released on destroy)
};

static BrowserData* GetData(HWND hDlg)
{
    return reinterpret_cast<BrowserData*>(GetWindowLongPtr(hDlg, DWLP_USER));
}

// ---- public -----------------------------------------------------------------

std::string RemoteBrowserDlg::DoModal(HWND hParent, UniFsClient* client)
{
    m_client = client;
    m_cwd    = "/";
    m_result.clear();

    DialogBoxParam(GetModuleHandle(NULL),
                   MAKEINTRESOURCE(IDD_REMOTE_BROWSER),
                   hParent, DlgProc, (LPARAM)this);
    return m_result;
}

// ---- static glue ------------------------------------------------------------

/* static */
INT_PTR CALLBACK RemoteBrowserDlg::DlgProc(HWND hDlg, UINT msg,
                                            WPARAM wp, LPARAM lp)
{
    RemoteBrowserDlg* pThis = nullptr;
    if (msg == WM_INITDIALOG) {
        pThis = reinterpret_cast<RemoteBrowserDlg*>(lp);
        BrowserData* bd = new BrowserData;
        bd->dlg = pThis;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)bd);
        pThis->m_hwnd = hDlg;
    } else {
        BrowserData* bd = GetData(hDlg);
        if (bd) pThis = bd->dlg;
    }
    if (!pThis) return FALSE;

    INT_PTR r = pThis->HandleMessage(hDlg, msg, wp, lp);

    if (msg == WM_DESTROY) {
        BrowserData* bd = GetData(hDlg);
        if (bd) {
            if (bd->hListFont) DeleteObject(bd->hListFont);
            delete bd;
        }
        SetWindowLongPtr(hDlg, DWLP_USER, 0);
    }
    return r;
}

// ---- instance message handler -----------------------------------------------

INT_PTR RemoteBrowserDlg::HandleMessage(HWND hDlg, UINT msg,
                                         WPARAM wp, LPARAM /*lp*/)
{
    switch (msg) {
    case WM_INITDIALOG:
    {
        // Use the system UI font (same as ListView controls) for the listbox.
        BrowserData* bd = GetData(hDlg);
        NONCLIENTMETRICSW ncm = {};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        bd->hListFont = CreateFontIndirectW(&ncm.lfMessageFont);
        SendDlgItemMessage(hDlg, IDC_FILELIST, WM_SETFONT,
                           (WPARAM)bd->hListFont, FALSE);
        // Set item height to match ListView row spacing:
        // ListView adds a few pixels of padding on top of the font height;
        // a plain LISTBOX does not, so we add it manually.
        {
            HWND hLB = GetDlgItem(hDlg, IDC_FILELIST);
            HDC hdc  = GetDC(hLB);
            HFONT hOld = (HFONT)SelectObject(hdc, bd->hListFont);
            TEXTMETRICW tm = {};
            GetTextMetricsW(hdc, &tm);
            SelectObject(hdc, hOld);
            ReleaseDC(hLB, hdc);
            SendMessage(hLB, LB_SETITEMHEIGHT, 0,
                        tm.tmHeight + tm.tmExternalLeading + 4);
        }
        Navigate(m_cwd);
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case IDC_BTN_UP:
        {
            if (m_cwd == "/") break;
            size_t pos = m_cwd.rfind('/');
            Navigate(pos == 0 ? std::string("/") : m_cwd.substr(0, pos));
            break;
        }
        case IDC_FILELIST:
            if (HIWORD(wp) == LBN_DBLCLK) {
                TryOpenSelected();
            } else if (HIWORD(wp) == LBN_SELCHANGE) {
                // Enable OK only when a file is selected
                BrowserData* bd = GetData(hDlg);
                int sel = (int)SendDlgItemMessage(hDlg, IDC_FILELIST,
                                                  LB_GETCURSEL, 0, 0);
                bool ok = false;
                if (sel >= 0 && bd && sel < (int)bd->entries.size())
                    ok = !bd->entries[sel].is_dir;
                EnableWindow(GetDlgItem(hDlg, IDOK), ok ? TRUE : FALSE);
            }
            break;
        case IDOK:
            TryOpenSelected();
            break;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            break;
        }
        break;
    }
    return FALSE;
}

// ---- helpers ----------------------------------------------------------------

void RemoteBrowserDlg::Navigate(const std::string& dir)
{
    BrowserData* bd = GetData(m_hwnd);
    if (!bd) return;

    // Try to list the directory
    std::unique_ptr<UniFs> fs = m_client->open_dir(dir.c_str());
    if (!fs || fs->readdir() != 0) {
        MessageBoxW(m_hwnd, L"Cannot open directory.", L"Error",
                    MB_OK | MB_ICONERROR);
        return;
    }

    m_cwd = dir;
    bd->entries.clear();

    DirEntry* ent;
    while ((ent = fs->next()) != nullptr) {
        std::string n = ent->name();
        if (n == "." || n == "..") continue;
        Entry e;
        e.name   = n;
        e.is_dir = (ent->type() == DirEntry::DIR);
        bd->entries.push_back(e);
    }
    fs->close();

    // Sort: dirs first, then files; within each group use natural sort
    // (StrCmpLogicalW) to match Explorer / FileList ordering.
    std::sort(bd->entries.begin(), bd->entries.end(),
        [](const Entry& a, const Entry& b) {
            if (a.is_dir != b.is_dir) return a.is_dir > b.is_dir;
            std::wstring wa = utf8_to_wstr(a.name.c_str());
            std::wstring wb = utf8_to_wstr(b.name.c_str());
            return StrCmpLogicalW(wa.c_str(), wb.c_str()) < 0;
        });

    PopulateList();

    // Update path bar
    std::wstring wdir = utf8_to_wstr(m_cwd.c_str());
    SetDlgItemTextW(m_hwnd, IDC_PATHBAR, wdir.c_str());

    // Disable OK until a file is selected
    EnableWindow(GetDlgItem(m_hwnd, IDOK), FALSE);
    EnableWindow(GetDlgItem(m_hwnd, IDC_BTN_UP),
                 m_cwd != "/" ? TRUE : FALSE);
}

void RemoteBrowserDlg::PopulateList()
{
    BrowserData* bd = GetData(m_hwnd);
    HWND hList = GetDlgItem(m_hwnd, IDC_FILELIST);
    SendMessage(hList, LB_RESETCONTENT, 0, 0);

    for (const Entry& e : bd->entries) {
        // Prefix dirs with "[" "]" so they stand out
        std::string display = e.is_dir ? ("[" + e.name + "]") : e.name;
        std::wstring wdisp = utf8_to_wstr(display.c_str());
        SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)wdisp.c_str());
    }
}

void RemoteBrowserDlg::TryOpenSelected()
{
    BrowserData* bd = GetData(m_hwnd);
    if (!bd) return;

    HWND hList = GetDlgItem(m_hwnd, IDC_FILELIST);
    int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
    if (sel < 0 || sel >= (int)bd->entries.size()) return;

    const Entry& e = bd->entries[sel];
    if (e.is_dir) {
        std::string newdir = (m_cwd == "/") ? ("/" + e.name)
                                             : (m_cwd + "/" + e.name);
        Navigate(newdir);
    } else {
        // Build full path
        m_result = (m_cwd == "/") ? ("/" + e.name)
                                  : (m_cwd + "/" + e.name);
        EndDialog(m_hwnd, IDOK);
    }
}
