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

#pragma once

#include <windows.h>
#include <vector>
#include <string>

class SlideShowDlg
{
public:
	enum SourceMode { SOURCE_CURRENT_FOLDER = 0, SOURCE_CUSTOM = 1 };
	enum OrderMode  { ORDER_NORMAL = 0, ORDER_REVERSE = 1, ORDER_RANDOM = 2 };
	enum RepeatMode { REPEAT_FOREVER = 0, REPEAT_COUNT = 1 };
	enum StartMode  { START_CANCEL = 0, START_FULLSCREEN = 1, START_WINDOWED = 2 };

	struct Item
	{
		std::wstring display;
		std::string  path;
	};

	struct Options
	{
		SourceMode source = SOURCE_CURRENT_FOLDER;
		OrderMode  order = ORDER_NORMAL;
		RepeatMode repeat = REPEAT_FOREVER;
		StartMode  start = START_CANCEL;
		int intervalSec = 5;
		int rounds = 1;
		int zoomMode = 0;
		std::vector<Item> customItems;
	};

	static bool Show(HWND owner, HINSTANCE hInst, bool hasCurrentFolder,
					 int currentZoomMode, Options &options);
};
