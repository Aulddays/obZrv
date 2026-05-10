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
#include <winsock2.h>  // must precede windows.h when using asio
#include "ConnectDlg.h"
#include "../unifs/client.hpp"
#include "resource.h"
#include "strutil.h"
#include "config.h"
#include <commctrl.h>
#include <algorithm>

/* Load remote history from config (most-recent first, max 5). */
static std::vector<std::wstring> LoadHistory()
{
	std::vector<std::wstring> hist;
	wchar_t key[16];
	for (int i = 0; i < 5; i++) {
		wsprintfW(key, L"Host%d", i);
		std::wstring v = Config::instance().getStr(L"RemoteHistory", key);
		if (!v.empty())
			hist.push_back(v);
	}
	return hist;
}

/* Prepend entry, dedupe, trim to 5, write back. */
static void SaveHistory(const std::wstring &entry)
{
	std::vector<std::wstring> hist = LoadHistory();
	hist.erase(std::remove(hist.begin(), hist.end(), entry), hist.end());
	hist.insert(hist.begin(), entry);
	if (hist.size() > 5) hist.resize(5);

	wchar_t key[16];
	for (int i = 0; i < 5; i++) {
		wsprintfW(key, L"Host%d", i);
		Config::instance().setStr(L"RemoteHistory", key,
								  i < (int)hist.size() ? hist[i].c_str() : L"");
	}
}

std::unique_ptr<UniFsClient> ConnectDlg::DoModal(HWND hParent,
												   std::string &host,
												   uint16_t    &port)
{
	m_host.clear();
	m_port = 0;
	m_client.reset();

	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONNECT),
				   hParent, DlgProc, (LPARAM)this);

	if (!m_client)
		return std::unique_ptr<UniFsClient>();

	host = m_host;
	port = m_port;
	return std::move(m_client);
}

/* static */
INT_PTR CALLBACK ConnectDlg::DlgProc(HWND hDlg, UINT msg,
									   WPARAM wp, LPARAM lp)
{
	ConnectDlg *pThis = NULL;
	if (msg == WM_INITDIALOG) {
		pThis = reinterpret_cast<ConnectDlg *>(lp);
		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pThis);
		pThis->m_hwnd = hDlg;
	} else {
		pThis = reinterpret_cast<ConnectDlg *>(
					GetWindowLongPtr(hDlg, DWLP_USER));
	}
	if (!pThis) return FALSE;
	return pThis->HandleMessage(hDlg, msg, wp, lp);
}

INT_PTR ConnectDlg::HandleMessage(HWND hDlg, UINT msg, WPARAM wp, LPARAM /*lp*/)
{
	switch (msg) {
	case WM_INITDIALOG:
	{
		// Load history into combobox; select the most recent entry if any
		HWND hCb = GetDlgItem(hDlg, IDC_HOST);
		std::vector<std::wstring> hist = LoadHistory();
		for (const std::wstring &h : hist)
			SendMessageW(hCb, CB_ADDSTRING, 0, (LPARAM)h.c_str());
		if (!hist.empty())
			SendMessageW(hCb, CB_SETCURSEL, 0, 0);
		SendMessageW(hCb, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
		SetFocus(hCb);
		return FALSE;
	}

	case WM_COMMAND:
		switch (LOWORD(wp)) {
		case IDOK: {
			wchar_t wbuf[300] = {};
			GetDlgItemTextW(hDlg, IDC_HOST, wbuf, 300);
			std::string s = wstr_to_utf8(wbuf);
			// Parse "host:port"
			size_t colon = s.rfind(':');
			if (colon == std::string::npos || colon == 0) {
				MessageBoxW(hDlg, L"Please enter address as  host:port",
							L"Input Required", MB_OK | MB_ICONWARNING);
				SetFocus(GetDlgItem(hDlg, IDC_HOST));
				return TRUE;
			}
			m_host = s.substr(0, colon);
			int p  = 0;
			try { p = std::stoi(s.substr(colon + 1)); } catch (...) {}
			if (p <= 0 || p > 65535) {
				MessageBoxW(hDlg, L"Port must be between 1 and 65535.",
							L"Input Required", MB_OK | MB_ICONWARNING);
				SetFocus(GetDlgItem(hDlg, IDC_HOST));
				return TRUE;
			}
			m_port = (uint16_t)p;
			// Attempt connection while dialog is still open
			m_client = UniFsClient::connect(m_host.c_str(), m_port);
			if (!m_client) {
				MessageBoxW(hDlg,
							L"Could not connect to the remote server.\n"
							L"Please check the address and try again.",
							L"Connection Failed", MB_OK | MB_ICONERROR);
				SetFocus(GetDlgItem(hDlg, IDC_HOST));
				return TRUE;
			}
			SaveHistory(utf8_to_wstr(s.c_str()));
			EndDialog(hDlg, IDOK);
			return TRUE;
		}
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}
