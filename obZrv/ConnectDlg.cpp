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
#include "Doc.h"
#include "resource.h"
#include "strutil.h"
#include "config.h"
#include "../unifs/remote_fs.hpp"
#include "../unifs/unifs.hpp"
#include <commctrl.h>
#include <shlwapi.h>
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

static std::string DirFromPath(const char *path)
{
	if (!path || !*path)
		return "/";
	const char *pos = strrchr(path, '/');
	if (!pos)
		return "/";
	if (pos == path)
		return "/";
	return std::string(path, pos);
}

std::shared_ptr<UniFs> ConnectDlg::DoModal(HWND hParent,
									   std::string &host,
									   uint16_t    &port,
									   std::string &remotePath,
									   std::shared_ptr<UniFs> existing,
									   const char *initialPath)
{
	m_host.clear();
	m_port = 0;
	m_client.reset();
	m_cwd = "/";
	m_result.clear();
	m_entries.clear();

	RemoteFs *remote = dynamic_cast<RemoteFs *>(existing.get());
	if (remote && initialPath && *initialPath) {
		m_client = existing;
		m_host = remote->host();
		m_port = remote->port();
		m_cwd = DirFromPath(initialPath);
	}

	DialogBoxParam(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONNECT),
				   hParent, DlgProc, (LPARAM)this);

	if (!m_client || m_result.empty())
		return std::shared_ptr<UniFs>();

	host = m_host;
	port = m_port;
	remotePath = m_result;
	return m_client;
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
		HWND hCb = GetDlgItem(hDlg, IDC_HOST);
		std::vector<std::wstring> hist = LoadHistory();
		for (const std::wstring &h : hist)
			SendMessageW(hCb, CB_ADDSTRING, 0, (LPARAM)h.c_str());

		if (!m_host.empty()) {
			std::wstring addr = utf8_to_wstr((m_host + ":" + std::to_string(m_port)).c_str());
			SetWindowTextW(hCb, addr.c_str());
		} else if (!hist.empty()) {
			SendMessageW(hCb, CB_SETCURSEL, 0, 0);
		}
		SendMessageW(hCb, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));

		InitBrowserFont(hDlg);
		if (m_client)
			Navigate(m_cwd);
		else
			SetBrowserEnabled(false);
		SetFocus(hCb);
		return FALSE;
	}

	case WM_COMMAND:
		switch (LOWORD(wp)) {
		case IDC_CONNECT:
			ConnectFromInput(hDlg);
			return TRUE;
		case IDC_BTN_UP:
		{
			if (m_cwd == "/") break;
			size_t pos = m_cwd.rfind('/');
			Navigate(pos == 0 ? std::string("/") : m_cwd.substr(0, pos));
			return TRUE;
		}
		case IDC_FILELIST:
			if (HIWORD(wp) == LBN_DBLCLK) {
				TryOpenSelected();
			} else if (HIWORD(wp) == LBN_SELCHANGE) {
				int sel = (int)SendDlgItemMessage(hDlg, IDC_FILELIST, LB_GETCURSEL, 0, 0);
				bool ok = sel >= 0 && sel < (int)m_entries.size() && !m_entries[sel].is_dir;
				EnableWindow(GetDlgItem(hDlg, IDOK), ok ? TRUE : FALSE);
			}
			return TRUE;
		case IDOK:
			TryOpenSelected();
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;

	case WM_DESTROY:
		if (m_listFont) {
			DeleteObject(m_listFont);
			m_listFont = NULL;
		}
		break;
	}
	return FALSE;
}

void ConnectDlg::InitBrowserFont(HWND hDlg)
{
	NONCLIENTMETRICSW ncm = {};
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
	m_listFont = CreateFontIndirectW(&ncm.lfMessageFont);
	SendDlgItemMessage(hDlg, IDC_FILELIST, WM_SETFONT,
					   (WPARAM)m_listFont, FALSE);

	HWND hLB = GetDlgItem(hDlg, IDC_FILELIST);
	HDC hdc  = GetDC(hLB);
	HFONT hOld = (HFONT)SelectObject(hdc, m_listFont);
	TEXTMETRICW tm = {};
	GetTextMetricsW(hdc, &tm);
	SelectObject(hdc, hOld);
	ReleaseDC(hLB, hdc);
	SendMessage(hLB, LB_SETITEMHEIGHT, 0,
				tm.tmHeight + tm.tmExternalLeading + 4);
}

void ConnectDlg::SetBrowserEnabled(bool enabled)
{
	EnableWindow(GetDlgItem(m_hwnd, IDC_PATHBAR), enabled ? TRUE : FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_BTN_UP), enabled && m_cwd != "/" ? TRUE : FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDC_FILELIST), enabled ? TRUE : FALSE);
	EnableWindow(GetDlgItem(m_hwnd, IDOK), FALSE);
	if (!enabled) {
		SetDlgItemTextW(m_hwnd, IDC_PATHBAR, L"");
		SendDlgItemMessage(m_hwnd, IDC_FILELIST, LB_RESETCONTENT, 0, 0);
		m_entries.clear();
	}
}

bool ConnectDlg::ConnectFromInput(HWND hDlg)
{
	wchar_t wbuf[300] = {};
	GetDlgItemTextW(hDlg, IDC_HOST, wbuf, 300);
	std::string s = wstr_to_utf8(wbuf);

	size_t colon = s.rfind(':');
	if (colon == std::string::npos || colon == 0) {
		MessageBoxW(hDlg, L"Please enter address as  host:port",
					L"Input Required", MB_OK | MB_ICONWARNING);
		SetFocus(GetDlgItem(hDlg, IDC_HOST));
		return false;
	}

	std::string host = s.substr(0, colon);
	int p = 0;
	try { p = std::stoi(s.substr(colon + 1)); } catch (...) {}
	if (p <= 0 || p > 65535) {
		MessageBoxW(hDlg, L"Port must be between 1 and 65535.",
					L"Input Required", MB_OK | MB_ICONWARNING);
		SetFocus(GetDlgItem(hDlg, IDC_HOST));
		return false;
	}

	auto client = RemoteFs::open(host.c_str(), (uint16_t)p);
	if (!client) {
		MessageBoxW(hDlg,
					L"Could not connect to the remote server.\n"
					L"Please check the address and try again.",
					L"Connection Failed", MB_OK | MB_ICONERROR);
		SetFocus(GetDlgItem(hDlg, IDC_HOST));
		return false;
	}

	m_host = host;
	m_port = (uint16_t)p;
	m_client.reset(client.release());
	m_result.clear();
	SaveHistory(utf8_to_wstr(s.c_str()));
	SetBrowserEnabled(false);
	Navigate("/");
	return true;
}

void ConnectDlg::Navigate(const std::string &dir)
{
	if (!m_client) {
		SetBrowserEnabled(false);
		return;
	}

	DirIter iter = m_client->readdir(dir.c_str());
	if (!iter) {
		MessageBoxW(m_hwnd, L"Cannot open directory.", L"Error",
					MB_OK | MB_ICONERROR);
		return;
	}

	m_cwd = dir;
	m_entries.clear();

	const DirEntry* ent;
	while ((ent = iter.next()) != nullptr) {
		std::string n = ent->name();
		if (n == "." || n == "..") continue;
		Entry e;
		e.name   = n;
		e.is_dir = (ent->type() == DirEntry::DIR);
		if (!e.is_dir) {
			std::string ext = getExt(n.c_str());
			if (ext.empty() || !Doc::isSupportedExt(ext)) continue;
		}
		m_entries.push_back(e);
	}

	std::sort(m_entries.begin(), m_entries.end(),
		[](const Entry& a, const Entry& b) {
			if (a.is_dir != b.is_dir) return a.is_dir > b.is_dir;
			std::wstring wa = utf8_to_wstr(a.name.c_str());
			std::wstring wb = utf8_to_wstr(b.name.c_str());
			return StrCmpLogicalW(wa.c_str(), wb.c_str()) < 0;
		});

	PopulateList();
	std::wstring wdir = utf8_to_wstr(m_cwd.c_str());
	SetDlgItemTextW(m_hwnd, IDC_PATHBAR, wdir.c_str());
	SetBrowserEnabled(true);
}

void ConnectDlg::PopulateList()
{
	HWND hList = GetDlgItem(m_hwnd, IDC_FILELIST);
	SendMessage(hList, LB_RESETCONTENT, 0, 0);

	for (const Entry& e : m_entries) {
		std::string display = e.is_dir ? ("[" + e.name + "]") : e.name;
		std::wstring wdisp = utf8_to_wstr(display.c_str());
		SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)wdisp.c_str());
	}
}

void ConnectDlg::TryOpenSelected()
{
	HWND hList = GetDlgItem(m_hwnd, IDC_FILELIST);
	int sel = (int)SendMessage(hList, LB_GETCURSEL, 0, 0);
	if (sel < 0 || sel >= (int)m_entries.size()) return;

	const Entry& e = m_entries[sel];
	if (e.is_dir) {
		std::string newdir = (m_cwd == "/") ? ("/" + e.name)
									 : (m_cwd + "/" + e.name);
		Navigate(newdir);
	} else {
		m_result = (m_cwd == "/") ? ("/" + e.name)
								 : (m_cwd + "/" + e.name);
		EndDialog(m_hwnd, IDOK);
	}
}
