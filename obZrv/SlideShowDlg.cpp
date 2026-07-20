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
// along with obZrv.If not, see <http://www.gnu.org/licenses/>.

#include "pch.h"
#include "SlideShowDlg.h"
#include "resource.h"
#include "config.h"
#include "Doc.h"
#include "strutil.h"

#include <commdlg.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdlib.h>
#include <wctype.h>

namespace {

const wchar_t *CFG_SECTION = L"SlideShow";

struct ZoomEntry
{
	int id;
	const wchar_t *label;
};

const ZoomEntry ZOOMS[] = {
	{ ID_ZOOMMODE_W2I_ZOOMOUT, L"Window Fit Image, ZoomOut if Large" },
	{ ID_ZOOMMODE_W2I,         L"Window Fit Image, No Zoom" },
	{ ID_ZOOMMODE_I2W_ZOOMOUT, L"Image Fit Window, ZoomOut Only" },
	{ ID_ZOOMMODE_I2W,         L"Image Fit Window" },
	{ ID_ZOOMMODE_NOFIT,       L"No Fit" },
};

struct DlgData
{
	HWND hwnd = NULL;
	bool hasCurrentFolder = false;
	SlideShowDlg::Options *options = NULL;
	std::vector<SlideShowDlg::Item> items;
};

int ClampInt(int v, int lo, int hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

std::string LowerExtNoDot(const std::wstring &path)
{
	const wchar_t *ext = PathFindExtensionW(path.c_str());
	if (!ext || !*ext) return std::string();
	if (*ext == L'.') ext++;
	std::wstring wext(ext);
	std::transform(wext.begin(), wext.end(), wext.begin(), towlower);
	return wstr_to_utf8(wext.c_str());
}

bool IsSupportedImage(const std::wstring &path)
{
	std::string ext = LowerExtNoDot(path);
	return !ext.empty() && Doc::isSupportedExt(ext);
}

std::wstring BuildImageFilter()
{
	std::wstring filter = L"Image Files";
	filter.push_back(L'\0');
	bool first = true;
	for (const std::string &type : Doc::getSupportedTypes()) {
		if (!first) filter += L";";
		filter += L"*.";
		filter += utf8_to_wstr(type.c_str());
		first = false;
	}
	filter.push_back(L'\0');
	filter += L"All Files";
	filter.push_back(L'\0');
	filter += L"*.*";
	filter.push_back(L'\0');
	filter.push_back(L'\0');
	return filter;
}

void AppendItem(DlgData *data, const std::wstring &path)
{
	SlideShowDlg::Item item;
	item.display = path;
	item.path = to_unipath(path.c_str());
	data->items.push_back(item);
}

void RebuildList(DlgData *data)
{
	HWND hList = GetDlgItem(data->hwnd, IDC_SS_FILELIST);
	SendMessageW(hList, LB_RESETCONTENT, 0, 0);
	for (const auto &item : data->items)
		SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)item.display.c_str());
}

std::vector<int> GetSelection(HWND hList)
{
	int count = (int)SendMessageW(hList, LB_GETSELCOUNT, 0, 0);
	std::vector<int> sel;
	if (count <= 0) return sel;
	sel.resize(count);
	SendMessageW(hList, LB_GETSELITEMS, (WPARAM)count, (LPARAM)sel.data());
	std::sort(sel.begin(), sel.end());
	return sel;
}

void SelectRange(HWND hList, int first, int count)
{
	SendMessageW(hList, LB_SETSEL, FALSE, -1);
	for (int i = 0; i < count; i++)
		SendMessageW(hList, LB_SETSEL, TRUE, first + i);
	SendMessageW(hList, LB_SETTOPINDEX, first, 0);
}

void MoveSelection(DlgData *data, bool down)
{
	HWND hList = GetDlgItem(data->hwnd, IDC_SS_FILELIST);
	std::vector<int> sel = GetSelection(hList);
	if (sel.empty()) return;
	int n = (int)data->items.size();
	if ((!down && sel.front() == 0) || (down && sel.back() == n - 1))
		return;

	std::vector<SlideShowDlg::Item> block;
	std::vector<SlideShowDlg::Item> rest;
	block.reserve(sel.size());
	rest.reserve(data->items.size() - sel.size());

	size_t nextSel = 0;
	for (int i = 0; i < n; i++) {
		if (nextSel < sel.size() && sel[nextSel] == i) {
			block.push_back(data->items[i]);
			nextSel++;
		} else {
			rest.push_back(data->items[i]);
		}
	}

	int insertAt = down ? sel.front() + 1 : sel.front() - 1;
	insertAt = ClampInt(insertAt, 0, (int)rest.size());
	rest.insert(rest.begin() + insertAt, block.begin(), block.end());
	data->items.swap(rest);
	RebuildList(data);
	SelectRange(hList, insertAt, (int)block.size());
}

void EnableCustomControls(DlgData *data)
{
	bool custom = IsDlgButtonChecked(data->hwnd, IDC_SS_SOURCE_CUSTOM) == BST_CHECKED;
	int ids[] = {
		IDC_SS_CUSTOM_GROUP, IDC_SS_FILELIST, IDC_SS_ADD_FILES, IDC_SS_ADD_FOLDER,
		IDC_SS_MOVE_UP, IDC_SS_MOVE_DOWN
	};
	for (int id : ids)
		EnableWindow(GetDlgItem(data->hwnd, id), custom);
}

int SelectedZoom(HWND hDlg)
{
	HWND hZoom = GetDlgItem(hDlg, IDC_SS_ZOOM);
	int idx = (int)SendMessageW(hZoom, CB_GETCURSEL, 0, 0);
	if (idx < 0 || idx >= (int)(sizeof(ZOOMS) / sizeof(ZOOMS[0])))
		return ID_ZOOMMODE_W2I_ZOOMOUT;
	return ZOOMS[idx].id;
}

void SetIntText(HWND hDlg, int id, int val)
{
	wchar_t buf[32];
	_itow(val, buf, 10);
	SetDlgItemTextW(hDlg, id, buf);
}

int GetIntText(HWND hDlg, int id, int def)
{
	wchar_t buf[32] = {};
	GetDlgItemTextW(hDlg, id, buf, 32);
	int val = _wtoi(buf);
	return val > 0 ? val : def;
}

void AddFiles(DlgData *data)
{
	std::vector<wchar_t> buffer(65536, 0);
	std::wstring filter = BuildImageFilter();

	OPENFILENAMEW ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = data->hwnd;
	ofn.lpstrFilter = filter.c_str();
	ofn.lpstrFile = buffer.data();
	ofn.nMaxFile = (DWORD)buffer.size();
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT;

	if (!GetOpenFileNameW(&ofn))
		return;

	wchar_t *p = buffer.data();
	std::wstring first = p;
	p += first.size() + 1;
	if (*p == L'\0') {
		AppendItem(data, first);
	} else {
		std::wstring dir = first;
		while (*p) {
			std::wstring path = dir + L"\\" + p;
			AppendItem(data, path);
			p += wcslen(p) + 1;
		}
	}
	RebuildList(data);
}

void AddFolder(DlgData *data)
{
	BROWSEINFOW bi = {};
	bi.hwndOwner = data->hwnd;
	bi.lpszTitle = L"Select image folder";
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
	if (!pidl) return;

	wchar_t folder[MAX_PATH] = {};
	bool ok = SHGetPathFromIDListW(pidl, folder) != FALSE;
	CoTaskMemFree(pidl);
	if (!ok) return;

	std::wstring mask = folder;
	mask += L"\\*";
	WIN32_FIND_DATAW fd = {};
	HANDLE hFind = FindFirstFileW(mask.c_str(), &fd);
	if (hFind == INVALID_HANDLE_VALUE) return;

	std::vector<std::wstring> paths;
	do {
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		std::wstring path = std::wstring(folder) + L"\\" + fd.cFileName;
		if (IsSupportedImage(path))
			paths.push_back(path);
	} while (FindNextFileW(hFind, &fd));
	FindClose(hFind);

	std::sort(paths.begin(), paths.end(), [](const std::wstring &a, const std::wstring &b) {
		return StrCmpLogicalW(a.c_str(), b.c_str()) < 0;
	});
	for (const auto &path : paths)
		AppendItem(data, path);
	RebuildList(data);
}

bool ValidateAndSave(HWND hDlg, DlgData *data, SlideShowDlg::StartMode start)
{
	SlideShowDlg::Options &opt = *data->options;
	opt.source = IsDlgButtonChecked(hDlg, IDC_SS_SOURCE_CURRENT) == BST_CHECKED
		? SlideShowDlg::SOURCE_CURRENT_FOLDER : SlideShowDlg::SOURCE_CUSTOM;
	opt.order = IsDlgButtonChecked(hDlg, IDC_SS_ORDER_REVERSE) == BST_CHECKED
		? SlideShowDlg::ORDER_REVERSE
		: (IsDlgButtonChecked(hDlg, IDC_SS_ORDER_RANDOM) == BST_CHECKED
			? SlideShowDlg::ORDER_RANDOM : SlideShowDlg::ORDER_NORMAL);
	opt.repeat = IsDlgButtonChecked(hDlg, IDC_SS_REPEAT_COUNT) == BST_CHECKED
		? SlideShowDlg::REPEAT_COUNT : SlideShowDlg::REPEAT_FOREVER;
	opt.intervalSec = ClampInt(GetIntText(hDlg, IDC_SS_INTERVAL, 5), 1, 3600);
	opt.rounds = ClampInt(GetIntText(hDlg, IDC_SS_ROUNDS, 1), 1, 9999);
	opt.zoomMode = SelectedZoom(hDlg);
	opt.start = start;
	opt.customItems = data->items;

	if (opt.source == SlideShowDlg::SOURCE_CUSTOM && opt.customItems.empty()) {
		MessageBoxW(hDlg, L"Add at least one file to the custom list.", L"Slide Show", MB_ICONWARNING | MB_OK);
		return false;
	}

	Config &cfg = Config::instance();
	cfg.setInt(CFG_SECTION, L"IntervalSec", opt.intervalSec);
	cfg.setInt(CFG_SECTION, L"Order", (int)opt.order);
	cfg.setInt(CFG_SECTION, L"Repeat", (int)opt.repeat);
	cfg.setInt(CFG_SECTION, L"Rounds", opt.rounds);
	return true;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT msg, WPARAM wp, LPARAM lp)
{
	DlgData *data = reinterpret_cast<DlgData *>(GetWindowLongPtr(hDlg, DWLP_USER));

	switch (msg) {
	case WM_INITDIALOG:
	{
		data = reinterpret_cast<DlgData *>(lp);
		data->hwnd = hDlg;
		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)data);

		Config &cfg = Config::instance();
		int interval = ClampInt(cfg.getInt(CFG_SECTION, L"IntervalSec", 5), 1, 3600);
		int order = ClampInt(cfg.getInt(CFG_SECTION, L"Order", SlideShowDlg::ORDER_NORMAL),
			SlideShowDlg::ORDER_NORMAL, SlideShowDlg::ORDER_RANDOM);
		int repeat = ClampInt(cfg.getInt(CFG_SECTION, L"Repeat", SlideShowDlg::REPEAT_FOREVER),
			SlideShowDlg::REPEAT_FOREVER, SlideShowDlg::REPEAT_COUNT);
		int rounds = ClampInt(cfg.getInt(CFG_SECTION, L"Rounds", 1), 1, 9999);

		CheckRadioButton(hDlg, IDC_SS_SOURCE_CURRENT, IDC_SS_SOURCE_CUSTOM,
			data->hasCurrentFolder ? IDC_SS_SOURCE_CURRENT : IDC_SS_SOURCE_CUSTOM);
		EnableWindow(GetDlgItem(hDlg, IDC_SS_SOURCE_CURRENT), data->hasCurrentFolder);
		CheckRadioButton(hDlg, IDC_SS_ORDER_NORMAL, IDC_SS_ORDER_RANDOM,
			order == SlideShowDlg::ORDER_REVERSE ? IDC_SS_ORDER_REVERSE
			: (order == SlideShowDlg::ORDER_RANDOM ? IDC_SS_ORDER_RANDOM : IDC_SS_ORDER_NORMAL));
		CheckRadioButton(hDlg, IDC_SS_REPEAT_FOREVER, IDC_SS_REPEAT_COUNT,
			repeat == SlideShowDlg::REPEAT_COUNT ? IDC_SS_REPEAT_COUNT : IDC_SS_REPEAT_FOREVER);
		SetIntText(hDlg, IDC_SS_INTERVAL, interval);
		SetIntText(hDlg, IDC_SS_ROUNDS, rounds);

		HWND hZoom = GetDlgItem(hDlg, IDC_SS_ZOOM);
		int selZoom = 0;
		for (int i = 0; i < (int)(sizeof(ZOOMS) / sizeof(ZOOMS[0])); i++) {
			SendMessageW(hZoom, CB_ADDSTRING, 0, (LPARAM)ZOOMS[i].label);
			if (ZOOMS[i].id == data->options->zoomMode)
				selZoom = i;
		}
		SendMessageW(hZoom, CB_SETCURSEL, selZoom, 0);

		EnableCustomControls(data);
		return TRUE;
	}

	case WM_COMMAND:
		switch (LOWORD(wp)) {
		case IDC_SS_SOURCE_CURRENT:
		case IDC_SS_SOURCE_CUSTOM:
			EnableCustomControls(data);
			return TRUE;
		case IDC_SS_ADD_FILES:
			AddFiles(data);
			return TRUE;
		case IDC_SS_ADD_FOLDER:
			AddFolder(data);
			return TRUE;
		case IDC_SS_MOVE_UP:
			MoveSelection(data, false);
			return TRUE;
		case IDC_SS_MOVE_DOWN:
			MoveSelection(data, true);
			return TRUE;
		case IDC_SS_FULLSCREEN:
			if (ValidateAndSave(hDlg, data, SlideShowDlg::START_FULLSCREEN))
				EndDialog(hDlg, IDOK);
			return TRUE;
		case IDC_SS_WINDOWED:
			if (ValidateAndSave(hDlg, data, SlideShowDlg::START_WINDOWED))
				EndDialog(hDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			data->options->start = SlideShowDlg::START_CANCEL;
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

} // namespace

bool SlideShowDlg::Show(HWND owner, HINSTANCE hInst, bool hasCurrentFolder,
						int currentZoomMode, Options &options)
{
	options.zoomMode = currentZoomMode;
	DlgData data;
	data.hasCurrentFolder = hasCurrentFolder;
	data.options = &options;
	data.items = options.customItems;

	INT_PTR res = DialogBoxParamW(hInst, MAKEINTRESOURCE(IDD_SLIDESHOW), owner,
		DlgProc, (LPARAM)&data);
	return res == IDOK && options.start != START_CANCEL;
}
