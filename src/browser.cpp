/***************************************************************************
 *   Copyright (C) 2008-2012 by Andrzej Rybczak                            *
 *   electricityispower@gmail.com                                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include <dirent.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>
#include <algorithm>

#include "browser.h"
#include "charset.h"
#include "display.h"
#include "global.h"
#include "helpers.h"
#include "playlist.h"
#include "regex_filter.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "tag_editor.h"
#include "utility/comparators.h"
#include "title.h"
#include "tags.h"
#include "screen_switcher.h"

using namespace std::placeholders;

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

using MPD::itDirectory;
using MPD::itSong;
using MPD::itPlaylist;

Browser *myBrowser;

namespace {//

std::set<std::string> SupportedExtensions;
bool hasSupportedExtension(const std::string &file);

std::string ItemToString(const MPD::Item &item);
bool BrowserEntryMatcher(const Regex &rx, const MPD::Item &item, bool filter);

}

Browser::Browser() : itsBrowseLocally(0), itsScrollBeginning(0), itsBrowsedDir("/")
{
	w = NC::Menu<MPD::Item>(0, MainStartY, COLS, MainHeight, Config.columns_in_browser && Config.titles_visibility ? Display::Columns(COLS) : "", Config.main_color, NC::brNone);
	w.setHighlightColor(Config.main_highlight_color);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setSelectedPrefix(Config.selected_item_prefix);
	w.setSelectedSuffix(Config.selected_item_suffix);
	w.setItemDisplayer(std::bind(Display::Items, _1, proxySongList()));

	if (SupportedExtensions.empty())
		Mpd.GetSupportedExtensions(SupportedExtensions);
}

void Browser::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	w.setTitle(Config.columns_in_browser && Config.titles_visibility ? Display::Columns(w.getWidth()) : "");
	hasToBeResized = 0;
}

void Browser::switchTo()
{
	SwitchTo::execute(this);

	// local browser doesn't support sorting by mtime
	if (isLocal() && Config.browser_sort_mode == smMTime)
		Config.browser_sort_mode = smName;

	if (w.empty())
		GetDirectory(itsBrowsedDir);
	else
		markSongsInPlaylist(proxySongList());

	drawHeader();
}

std::wstring Browser::title()
{
	std::wstring result = L"Browse: ";
	result += Scroller(ToWString(itsBrowsedDir), itsScrollBeginning, COLS-result.length()-(Config.new_design ? 2 : Global::VolumeState.length()));
	return result;
}

void Browser::enterPressed()
{
	if (w.empty())
		return;

	const MPD::Item &item = w.current().value();
	switch (item.type)
	{
		case itDirectory:
		{
			if (isParentDirectory(item))
				GetDirectory(getParentDirectory(itsBrowsedDir), itsBrowsedDir);
			else
				GetDirectory(item.name, itsBrowsedDir);
			drawHeader();
			break;
		}
		case itSong:
		{
			addSongToPlaylist(*item.song, true, -1);
			break;
		}
		case itPlaylist:
		{
			if (Mpd.LoadPlaylist(item.name))
			{
				Statusbar::msg("Playlist \"%s\" loaded", item.name.c_str());
				myPlaylist->PlayNewlyAddedSongs();
			}
		}
	}
}

void Browser::spacePressed()
{
	if (w.empty())
		return;

	size_t i = itsBrowsedDir != "/" ? 1 : 0;
	if (Config.space_selects && w.choice() >= i)
	{
		i = w.choice();
		w.at(i).setSelected(!w.at(i).isSelected());
		w.scroll(NC::wDown);
		return;
	}

	const MPD::Item &item = w.current().value();

	if (isParentDirectory(item))
		return;

	switch (item.type)
	{
		case itDirectory:
		{
			bool result;
#			ifndef WIN32
			if (isLocal())
			{
				MPD::SongList list;
				MPD::ItemList items;
				Statusbar::msg("Scanning directory \"%s\"...", item.name.c_str());
				myBrowser->GetLocalDirectory(items, item.name, 1);
				list.reserve(items.size());
				for (MPD::ItemList::const_iterator it = items.begin(); it != items.end(); ++it)
					list.push_back(*it->song);
				result = addSongsToPlaylist(list, false, -1);
			}
			else
#			endif // !WIN32
				result = Mpd.Add(item.name);
			if (result)
				Statusbar::msg("Directory \"%s\" added", item.name.c_str());
			break;
		}
		case itSong:
		{
			addSongToPlaylist(*item.song, false);
			break;
		}
		case itPlaylist:
		{
			if (Mpd.LoadPlaylist(item.name))
				Statusbar::msg("Playlist \"%s\" loaded", item.name.c_str());
			break;
		}
	}
	w.scroll(NC::wDown);
}

void Browser::mouseButtonPressed(MEVENT me)
{
	if (w.empty() || !w.hasCoords(me.x, me.y) || size_t(me.y) >= w.size())
		return;
	if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
	{
		w.Goto(me.y);
		switch (w.current().value().type)
		{
			case itDirectory:
				if (me.bstate & BUTTON1_PRESSED)
				{
					GetDirectory(w.current().value().name);
					drawHeader();
				}
				else
				{
					size_t pos = w.choice();
					spacePressed();
					if (pos < w.size()-1)
						w.scroll(NC::wUp);
				}
				break;
			case itPlaylist:
			case itSong:
				if (me.bstate & BUTTON1_PRESSED)
				{
					size_t pos = w.choice();
					spacePressed();
					if (pos < w.size()-1)
						w.scroll(NC::wUp);
				}
				else
					enterPressed();
				break;
		}
	}
	else
		Screen<WindowType>::mouseButtonPressed(me);
}

/***********************************************************************/

bool Browser::allowsFiltering()
{
	return true;
}

std::string Browser::currentFilter()
{
	return RegexFilter<MPD::Item>::currentFilter(w);
}

void Browser::applyFilter(const std::string &filter)
{
	w.showAll();
	auto fun = std::bind(BrowserEntryMatcher, _1, _2, true);
	auto rx = RegexFilter<MPD::Item>(filter, Config.regex_type, fun);
	w.filter(w.begin(), w.end(), rx);
}

/***********************************************************************/

bool Browser::allowsSearching()
{
	return true;
}

bool Browser::search(const std::string &constraint)
{
	auto fun = std::bind(BrowserEntryMatcher, _1, _2, false);
	auto rx = RegexFilter<MPD::Item>(constraint, Config.regex_type, fun);
	return w.search(w.begin(), w.end(), rx);
}

void Browser::nextFound(bool wrap)
{
	w.nextFound(wrap);
}

void Browser::prevFound(bool wrap)
{
	w.prevFound(wrap);
}

/***********************************************************************/

ProxySongList Browser::proxySongList()
{
	return ProxySongList(w, [](NC::Menu<MPD::Item>::Item &item) -> MPD::Song * {
		MPD::Song *ptr = 0;
		if (item.value().type == itSong)
			ptr = item.value().song.get();
		return ptr;
	});
}

bool Browser::allowsSelection()
{
	return true;
}

void Browser::reverseSelection()
{
	reverseSelectionHelper(w.begin()+(itsBrowsedDir == "/" ? 0 : 1), w.end());
}

MPD::SongList Browser::getSelectedSongs()
{
	MPD::SongList result;
	auto item_handler = [this, &result](const MPD::Item &item) {
		if (item.type == itDirectory)
		{
#			ifndef WIN32
			if (isLocal())
			{
				MPD::ItemList list;
				GetLocalDirectory(list, item.name, true);
				for (auto it = list.begin(); it != list.end(); ++it)
					result.push_back(*it->song);
			}
			else
#			endif // !WIN32
			{
				auto list = Mpd.GetDirectoryRecursive(item.name);
				result.insert(result.end(), list.begin(), list.end());
			}
		}
		else if (item.type == itSong)
			result.push_back(*item.song);
		else if (item.type == itPlaylist)
		{
			auto list = Mpd.GetPlaylistContent(item.name);
			result.insert(result.end(), list.begin(), list.end());
		}
	};
	for (auto it = w.begin(); it != w.end(); ++it)
		if (it->isSelected())
			item_handler(it->value());
	// if no item is selected, add current one
	if (result.empty() && !w.empty())
		item_handler(w.current().value());
	return result;
}

void Browser::LocateSong(const MPD::Song &s)
{
	if (s.getDirectory().empty())
		return;

	itsBrowseLocally = !s.isFromDatabase();

	if (myScreen != this)
		switchTo();

	if (itsBrowsedDir != s.getDirectory())
		GetDirectory(s.getDirectory());
	for (size_t i = 0; i < w.size(); ++i)
	{
		if (w[i].value().type == itSong && s.getHash() == w[i].value().song->getHash())
		{
			w.highlight(i);
			break;
		}
	}
	drawHeader();
}

void Browser::GetDirectory(std::string dir, std::string subdir)
{
	if (dir.empty())
		dir = "/";

	int highlightme = -1;
	itsScrollBeginning = 0;
	if (itsBrowsedDir != dir)
		w.reset();
	itsBrowsedDir = dir;

	w.clear();

	if (dir != "/")
	{
		MPD::Item parent;
		parent.name = "..";
		parent.type = itDirectory;
		w.addItem(parent);
	}

	MPD::ItemList list;
#	ifndef WIN32
	if (isLocal())
		GetLocalDirectory(list);
	else
		list = Mpd.GetDirectory(dir);
#	else
	list = Mpd.GetDirectory(dir);
#	endif // !WIN32
	if (!isLocal()) // local directory is already sorted
		std::sort(list.begin(), list.end(),
			LocaleBasedItemSorting(std::locale(), Config.ignore_leading_the, Config.browser_sort_mode));

	for (MPD::ItemList::iterator it = list.begin(); it != list.end(); ++it)
	{
		switch (it->type)
		{
			case itPlaylist:
			{
				w.addItem(*it);
				break;
			}
			case itDirectory:
			{
				if (it->name == subdir)
					highlightme = w.size();
				w.addItem(*it);
				break;
			}
			case itSong:
			{
				bool bold = 0;
				for (size_t i = 0; i < myPlaylist->main().size(); ++i)
				{
					if (myPlaylist->main().at(i).value().getHash() == it->song->getHash())
					{
						bold = 1;
						break;
					}
				}
				w.addItem(*it, bold);
				break;
			}
		}
	}
	if (highlightme >= 0)
		w.highlight(highlightme);
}

#ifndef WIN32
void Browser::GetLocalDirectory(MPD::ItemList &v, const std::string &directory, bool recursively) const
{
	DIR *dir = opendir((directory.empty() ? itsBrowsedDir : directory).c_str());

	if (!dir)
		return;

	dirent *file;

	struct stat file_stat;
	std::string full_path;

	size_t old_size = v.size();
	while ((file = readdir(dir)))
	{
		// omit . and ..
		if (file->d_name[0] == '.' && (file->d_name[1] == '\0' || (file->d_name[1] == '.' && file->d_name[2] == '\0')))
			continue;

		if (!Config.local_browser_show_hidden_files && file->d_name[0] == '.')
			continue;
		MPD::Item new_item;
		full_path = directory.empty() ? itsBrowsedDir : directory;
		if (itsBrowsedDir != "/")
			full_path += "/";
		full_path += file->d_name;
		stat(full_path.c_str(), &file_stat);
		if (S_ISDIR(file_stat.st_mode))
		{
			if (recursively)
			{
				GetLocalDirectory(v, full_path, 1);
				old_size = v.size();
			}
			else
			{
				new_item.type = itDirectory;
				new_item.name = full_path;
				v.push_back(new_item);
			}
		}
		else if (hasSupportedExtension(file->d_name))
		{
			new_item.type = itSong;
			mpd_pair file_pair = { "file", full_path.c_str() };
			MPD::MutableSong *s = new MPD::MutableSong(mpd_song_begin(&file_pair));
			new_item.song = std::shared_ptr<MPD::Song>(s);
#			ifdef HAVE_TAGLIB_H
			if (!recursively)
				Tags::read(*s);
#			endif // HAVE_TAGLIB_H
			v.push_back(new_item);
		}
	}
	closedir(dir);
	std::sort(v.begin()+old_size, v.end(),
		LocaleBasedItemSorting(std::locale(), Config.ignore_leading_the, Config.browser_sort_mode));
}

void Browser::ClearDirectory(const std::string &path) const
{
	DIR *dir = opendir(path.c_str());
	if (!dir)
		return;

	dirent *file;
	struct stat file_stat;
	std::string full_path;

	while ((file = readdir(dir)))
	{
		// omit . and ..
		if (file->d_name[0] == '.' && (file->d_name[1] == '\0' || (file->d_name[1] == '.' && file->d_name[2] == '\0')))
			continue;

		full_path = path;
		if (*full_path.rbegin() != '/')
			full_path += '/';
		full_path += file->d_name;
		lstat(full_path.c_str(), &file_stat);
		if (S_ISDIR(file_stat.st_mode))
			ClearDirectory(full_path);
		if (remove(full_path.c_str()) == 0)
		{
			const char msg[] = "Deleting \"%ls\"...";
			Statusbar::msg(msg, wideShorten(ToWString(full_path), COLS-const_strlen(msg)).c_str());
		}
		else
		{
			const char msg[] = "Couldn't remove \"%ls\": %s";
			Statusbar::msg(msg, wideShorten(ToWString(full_path), COLS-const_strlen(msg)-25).c_str(), strerror(errno));
		}
	}
	closedir(dir);
}

void Browser::ChangeBrowseMode()
{
	if (Mpd.GetHostname()[0] != '/')
	{
		Statusbar::msg("For browsing local filesystem connection to MPD via UNIX Socket is required");
		return;
	}

	itsBrowseLocally = !itsBrowseLocally;
	Statusbar::msg("Browse mode: %s", itsBrowseLocally ? "Local filesystem" : "MPD database");
	itsBrowsedDir = itsBrowseLocally ? Config.GetHomeDirectory() : "/";
	if (itsBrowseLocally && *itsBrowsedDir.rbegin() == '/')
		itsBrowsedDir.resize(itsBrowsedDir.length()-1);
	w.reset();
	GetDirectory(itsBrowsedDir);
	drawHeader();
}

bool Browser::deleteItem(const MPD::Item &item)
{
	// parent dir
	if (item.type == itDirectory && item.name == "..")
		return false;

	// playlist created by mpd
	if (!isLocal() && item.type == itPlaylist && CurrentDir() == "/")
		return Mpd.DeletePlaylist(item.name);

	std::string path;
	if (!isLocal())
		path = Config.mpd_music_dir;
	path += item.type == itSong ? item.song->getURI() : item.name;

	if (item.type == itDirectory)
		ClearDirectory(path);

	return remove(path.c_str()) == 0;
}
#endif // !WIN32

namespace {//

bool hasSupportedExtension(const std::string &file)
{
	size_t last_dot = file.rfind(".");
	if (last_dot > file.length())
		return false;

	std::string ext = lowercase(file.substr(last_dot+1));
	return SupportedExtensions.find(ext) != SupportedExtensions.end();
}

std::string ItemToString(const MPD::Item &item)
{
	std::string result;
	switch (item.type)
	{
		case MPD::itDirectory:
			result = "[" + getBasename(item.name) + "]";
			break;
		case MPD::itSong:
			if (Config.columns_in_browser)
				result = item.song->toString(Config.song_in_columns_to_string_format, Config.tags_separator);
			else
				result = item.song->toString(Config.song_list_format_dollar_free, Config.tags_separator);
			break;
		case MPD::itPlaylist:
			result = Config.browser_playlist_prefix.str() + getBasename(item.name);
			break;
	}
	return result;
}

bool BrowserEntryMatcher(const Regex &rx, const MPD::Item &item, bool filter)
{
	if (Browser::isParentDirectory(item))
		return filter;
	return rx.match(ItemToString(item));
}

}

