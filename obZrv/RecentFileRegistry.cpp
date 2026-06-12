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
#include "RecentFileRegistry.h"
#include "config.h"
#include "strutil.h"
#include <algorithm>

static const wchar_t RECENT_SECTION[] = L"RecentFiles";

static std::wstring LocalDisplayPath(const std::string &path)
{
	std::wstring wpath = utf8_to_wstr(path.c_str());
	if (wpath.size() >= 3 && wpath[0] == L'/' && wpath[2] == L'/' &&
		((wpath[1] >= L'a' && wpath[1] <= L'z') || (wpath[1] >= L'A' && wpath[1] <= L'Z'))) {
		std::wstring out;
		out.reserve(wpath.size());
		out += (wchar_t)towupper(wpath[1]);
		out += L':';
		for (size_t i = 2; i < wpath.size(); i++)
			out += (wpath[i] == L'/') ? L'\\' : wpath[i];
		return out;
	}
	for (wchar_t &ch : wpath)
		if (ch == L'/')
			ch = L'\\';
	return wpath;
}

void RecentFileRegistry::Load()
{
	m_entries.clear();

	wchar_t key[32];
	for (int i = 0; i < MAX_ITEMS; i++) {
		wsprintfW(key, L"Type%d", i);
		std::wstring type = Config::instance().getStr(RECENT_SECTION, key);
		if (type.empty())
			continue;

		wsprintfW(key, L"Path%d", i);
		std::wstring path = Config::instance().getStr(RECENT_SECTION, key);
		if (path.empty())
			continue;

		Entry entry = {};
		entry.type = (type == L"remote") ? Entry::REMOTE : Entry::LOCAL;
		entry.path = wstr_to_utf8(path.c_str());

		if (entry.type == Entry::REMOTE) {
			wsprintfW(key, L"Host%d", i);
			entry.host = wstr_to_utf8(Config::instance().getStr(RECENT_SECTION, key).c_str());
			wsprintfW(key, L"Port%d", i);
			entry.port = (uint16_t)Config::instance().getInt(RECENT_SECTION, key, 0);
			if (entry.host.empty() || entry.port == 0)
				continue;
		}

		m_entries.push_back(entry);
	}
}

void RecentFileRegistry::Save() const
{
	wchar_t key[32];
	for (int i = 0; i < MAX_ITEMS; i++) {
		const Entry *entry = (i < (int)m_entries.size()) ? &m_entries[i] : nullptr;

		wsprintfW(key, L"Type%d", i);
		Config::instance().setStr(RECENT_SECTION, key,
			entry ? (entry->type == Entry::REMOTE ? L"remote" : L"local") : L"");

		wsprintfW(key, L"Path%d", i);
		std::wstring path = entry ? utf8_to_wstr(entry->path.c_str()) : std::wstring();
		Config::instance().setStr(RECENT_SECTION, key, path.c_str());

		wsprintfW(key, L"Host%d", i);
		std::wstring host = entry ? utf8_to_wstr(entry->host.c_str()) : std::wstring();
		Config::instance().setStr(RECENT_SECTION, key, host.c_str());

		wsprintfW(key, L"Port%d", i);
		Config::instance().setInt(RECENT_SECTION, key, entry ? entry->port : 0);
	}
}

void RecentFileRegistry::AddLocal(const std::string &path)
{
	Entry entry = {};
	entry.type = Entry::LOCAL;
	entry.path = path;
	Add(entry);
}

void RecentFileRegistry::AddRemote(const std::string &host, uint16_t port, const std::string &path)
{
	Entry entry = {};
	entry.type = Entry::REMOTE;
	entry.path = path;
	entry.host = host;
	entry.port = port;
	Add(entry);
}

std::wstring RecentFileRegistry::DisplayText(int i) const
{
	if (i < 0 || i >= (int)m_entries.size())
		return std::wstring();
	return ClipMiddle(FormatEntry(m_entries[i]));
}

void RecentFileRegistry::Add(const Entry &entry)
{
	if (entry.path.empty())
		return;

	m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
		[&](const Entry &item) {
			return item.type == entry.type &&
				item.path == entry.path &&
				item.host == entry.host &&
				item.port == entry.port;
		}), m_entries.end());

	m_entries.insert(m_entries.begin(), entry);
	if (m_entries.size() > MAX_ITEMS)
		m_entries.resize(MAX_ITEMS);
	Save();
}

std::wstring RecentFileRegistry::FormatEntry(const Entry &entry)
{
	std::wstring path = utf8_to_wstr(entry.path.c_str());
	if (entry.type == Entry::LOCAL)
		return LocalDisplayPath(entry.path);

	wchar_t prefix[320];
	_snwprintf(prefix, 320, L"remote:%s:%u", utf8_to_wstr(entry.host.c_str()).c_str(), entry.port);
	return std::wstring(prefix) + path;
}

std::wstring RecentFileRegistry::ClipMiddle(const std::wstring &text)
{
	if ((int)text.size() <= MAX_DISPLAY_CHARS)
		return text;

	const std::wstring ellipsis = L"...";
	int keep = MAX_DISPLAY_CHARS - (int)ellipsis.size();
	int left = keep / 2;
	int right = keep - left;
	return text.substr(0, left) + ellipsis + text.substr(text.size() - right);
}
