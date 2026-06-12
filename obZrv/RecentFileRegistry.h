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
#include <stdint.h>
#include <string>
#include <vector>

class RecentFileRegistry
{
public:
	struct Entry {
		enum Type { LOCAL, REMOTE } type;
		std::string path;
		std::string host;
		uint16_t    port;
	};

	enum { MAX_ITEMS = 20, MAX_DISPLAY_CHARS = 50 };

	void Load();
	void Save() const;
	void AddLocal(const std::string &path);
	void AddRemote(const std::string &host, uint16_t port, const std::string &path);

	int count() const { return (int)m_entries.size(); }
	const Entry &entry(int i) const { return m_entries[i]; }
	std::wstring DisplayText(int i) const;

private:
	std::vector<Entry> m_entries;

	void Add(const Entry &entry);
	static std::wstring FormatEntry(const Entry &entry);
	static std::wstring ClipMiddle(const std::wstring &text);
};
