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

class UniFs;

class ConnectDlg
{
public:
	std::shared_ptr<UniFs> DoModal(HWND hParent,
	                                std::string &host, uint16_t &port,
	                                std::string &remotePath,
	                                std::shared_ptr<UniFs> existing = std::shared_ptr<UniFs>(),
	                                const char *initialPath = nullptr);

private:
	struct Entry {
		std::string name;
		bool        is_dir;
	};

	HWND m_hwnd = NULL;
	HFONT m_listFont = NULL;

	static INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
	INT_PTR HandleMessage(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp);

	void InitBrowserFont(HWND hDlg);
	void SetBrowserEnabled(bool enabled);
	void Navigate(const std::string &dir);
	void PopulateList();
	void TryOpenSelected();
	bool ConnectFromInput(HWND hDlg);

	std::string  m_host;
	uint16_t     m_port = 0;
	std::shared_ptr<UniFs> m_client;
	std::string  m_cwd;
	std::string  m_result;
	std::vector<Entry> m_entries;
};
