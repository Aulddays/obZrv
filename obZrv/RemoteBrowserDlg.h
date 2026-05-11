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
#include <memory>

class UniFs;

// Modal dialog for browsing a remote UniFs directory tree.
// DoModal() returns the selected file path (UTF-8, Unix-style: /dir/file.jpg)
// on OK, or an empty string on Cancel/error.
class RemoteBrowserDlg {
public:
	// path_result receives the selected file path on success.
	std::string DoModal(HWND hParent, UniFs* client);

private:
	static INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
	INT_PTR HandleMessage(HWND, UINT, WPARAM, LPARAM);

	void Navigate(const std::string& dir);
	void PopulateList();
	void TryOpenSelected();

	HWND   m_hwnd   = NULL;
	UniFs* m_client = nullptr;
	std::string  m_cwd;          // current directory (Unix path, no trailing /)
	std::string  m_result;       // filled on IDOK
};
