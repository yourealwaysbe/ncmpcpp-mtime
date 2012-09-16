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

#include "display.h"
#include "global.h"
#include "playlist.h"
#include "settings.h"
#include "sort_playlist.h"
#include "statusbar.h"
#include "utility/comparators.h"
#include "screen_switcher.h"

SortPlaylistDialog *mySortPlaylistDialog;

SortPlaylistDialog::SortPlaylistDialog()
{
	typedef SortPlaylistDialog Self;
	typedef WindowType::Item::Type Entry;
	
	using Global::MainHeight;
	using Global::MainStartY;
	
	setDimensions();
	w = WindowType((COLS-m_width)/2, (MainHeight-m_height)/2+MainStartY, m_width, m_height, "Sort songs by...", Config.main_color, Config.window_border);
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setItemDisplayer([](Self::WindowType &menu) {
		menu << menu.drawn()->value().item().first;
	});
	
	w.addItem(Entry(std::make_pair("Artist", &MPD::Song::getArtist),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Album", &MPD::Song::getAlbum),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Disc", &MPD::Song::getDisc),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Track", &MPD::Song::getTrack),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Genre", &MPD::Song::getGenre),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Date", &MPD::Song::getDate),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Composer", &MPD::Song::getComposer),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Performer", &MPD::Song::getPerformer),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Title", &MPD::Song::getTitle),
		std::bind(&Self::moveSortOrderHint, this)
	));
	w.addItem(Entry(std::make_pair("Filename", &MPD::Song::getURI),
		std::bind(&Self::moveSortOrderHint, this)
	));
	m_sort_options = w.size();
	w.addSeparator();
	w.addItem(Entry(std::make_pair("Sort", static_cast<MPD::Song::GetFunction>(0)),
		std::bind(&Self::sort, this)
	));
	w.addItem(Entry(std::make_pair("Cancel", static_cast<MPD::Song::GetFunction>(0)),
		std::bind(&Self::cancel, this)
	));
}

void SortPlaylistDialog::switchTo()
{
	SwitchTo::execute(this);
	w.reset();
}

void SortPlaylistDialog::resize()
{
	using Global::MainHeight;
	using Global::MainStartY;
	setDimensions();
	w.resize(m_width, m_height);
	w.moveTo((COLS-m_width)/2, (MainHeight-m_height)/2+MainStartY);
	hasToBeResized = false;
}

std::wstring SortPlaylistDialog::title()
{
	return previousScreen()->title();
}

void SortPlaylistDialog::enterPressed()
{
	w.current().value().exec()();
}

void SortPlaylistDialog::mouseButtonPressed(MEVENT me)
{
	if (w.hasCoords(me.x, me.y))
	{
		if (me.bstate & (BUTTON1_PRESSED | BUTTON3_PRESSED))
		{
			w.Goto(me.y);
			if (me.bstate & BUTTON3_PRESSED)
				enterPressed();
		}
		else
			Screen<WindowType>::mouseButtonPressed(me);
	}
}

void SortPlaylistDialog::moveSortOrderDown()
{
	size_t pos = w.choice();
	if (pos < m_sort_options-1)
	{
		w.Swap(pos, pos+1);
		w.scroll(NC::wDown);
	}
}

void SortPlaylistDialog::moveSortOrderUp()
{
	size_t pos = w.choice();
	if (pos > 0 && pos < m_sort_options)
	{
		w.Swap(pos, pos-1);
		w.scroll(NC::wUp);
	}
}

void SortPlaylistDialog::moveSortOrderHint() const
{
	Statusbar::msg("Move tag types up and down to adjust sort order");
}

void SortPlaylistDialog::sort() const
{
	auto &pl = myPlaylist->main();
	auto begin = pl.begin(), end = pl.end();
	// if songs are selected, sort range from first selected to last selected
	if (pl.hasSelected())
	{
		while (!begin->isSelected())
			++begin;
		while (!(end-1)->isSelected())
			--end;
	}
	
	size_t start_pos = begin - pl.begin();
	MPD::SongList playlist;
	playlist.reserve(end - begin);
	for (; begin != end; ++begin)
		playlist.push_back(begin->value());
	
	typedef MPD::SongList::iterator Iterator;
	LocaleStringComparison cmp(std::locale(), Config.ignore_leading_the);
	std::function<void(Iterator, Iterator)> iter_swap, quick_sort;
	auto song_cmp = [this, &cmp](const MPD::Song &a, const MPD::Song &b) -> bool {
		for (size_t i = 0; i < m_sort_options; ++i)
		{
			int res = cmp(a.getTags(w[i].value().item().second, Config.tags_separator),
			              b.getTags(w[i].value().item().second, Config.tags_separator));
			if (res != 0)
				return res < 0;
		}
		return a.getPosition() < b.getPosition();
	};
	iter_swap = [&playlist, &start_pos](Iterator a, Iterator b) {
		std::iter_swap(a, b);
		Mpd.Swap(start_pos+a-playlist.begin(), start_pos+b-playlist.begin());
	};
	quick_sort = [this, &song_cmp, &quick_sort, &iter_swap](Iterator first, Iterator last) {
		if (last-first > 1)
		{
			Iterator pivot = first+rand()%(last-first);
			iter_swap(pivot, last-1);
			pivot = last-1;
			
			Iterator tmp = first;
			for (Iterator i = first; i != pivot; ++i)
				if (song_cmp(*i, *pivot))
					iter_swap(i, tmp++);
			iter_swap(tmp, pivot);
			
			quick_sort(first, tmp);
			quick_sort(tmp+1, last);
		}
	};
	
	Statusbar::msg("Sorting...");
	Mpd.StartCommandsList();
	quick_sort(playlist.begin(), playlist.end());
	if (Mpd.CommitCommandsList())
	{
		Statusbar::msg("Playlist sorted");	
		switchToPreviousScreen();
	}
}

void SortPlaylistDialog::cancel() const
{
	switchToPreviousScreen();
}

void SortPlaylistDialog::setDimensions()
{
	m_height = std::min(size_t(17), Global::MainHeight);
	m_width = 30;
}
