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

#ifndef _H_MEDIA_LIBRARY
#define _H_MEDIA_LIBRARY

#include "interfaces.h"
#include "screen.h"

struct MediaLibrary: Screen<NC::Window *>, Filterable, HasColumns, HasSongs, Searchable, Tabbable
{
	MediaLibrary();

	virtual void switchTo() OVERRIDE;
	virtual void resize() OVERRIDE;

	virtual std::wstring title() OVERRIDE;
	virtual ScreenType type() OVERRIDE { return ScreenType::MediaLibrary; }

	virtual void refresh() OVERRIDE;
	virtual void update() OVERRIDE;

	virtual void enterPressed() OVERRIDE;
	virtual void spacePressed() OVERRIDE;
	virtual void mouseButtonPressed(MEVENT me) OVERRIDE;

	virtual bool isMergable() OVERRIDE { return true; }

	// Filterable implementation
	virtual bool allowsFiltering() OVERRIDE;
	virtual std::string currentFilter() OVERRIDE;
	virtual void applyFilter(const std::string &filter) OVERRIDE;

	// Searchable implementation
	virtual bool allowsSearching() OVERRIDE;
	virtual bool search(const std::string &constraint) OVERRIDE;
	virtual void nextFound(bool wrap) OVERRIDE;
	virtual void prevFound(bool wrap) OVERRIDE;

	// HasSongs implementation
	virtual ProxySongList proxySongList() OVERRIDE;

	virtual bool allowsSelection() OVERRIDE;
	virtual void reverseSelection() OVERRIDE;
	virtual MPD::SongList getSelectedSongs() OVERRIDE;

	// HasColumns implementation
	virtual bool previousColumnAvailable() OVERRIDE;
	virtual void previousColumn() OVERRIDE;

	virtual bool nextColumnAvailable() OVERRIDE;
	virtual void nextColumn() OVERRIDE;

	// private members
	void toggleColumnsMode();
	int Columns();
	void LocateSong(const MPD::Song &);
	ProxySongList songsProxyList();

	// mtimes
	bool hasMTimes();
	void toggleMTimeSort();

	struct SearchConstraints
	{
		SearchConstraints() { }
		SearchConstraints(const std::string &tag, const std::string &album, const std::string &date) : PrimaryTag(tag), Album(album), Date(date), MTime(0) { }
		SearchConstraints(const std::string &album, const std::string &date) : Album(album), Date(date), MTime(0) { }
		SearchConstraints(const std::string &tag, const std::string &album, const std::string &date, time_t mtime) : PrimaryTag(tag), Album(album), Date(date), MTime(mtime) { }
		SearchConstraints(const std::string &album, const std::string &date, time_t mtime) : Album(album), Date(date), MTime(mtime) { }

		std::string PrimaryTag;
		std::string Album;
		std::string Date;
		time_t MTime;

		bool operator<(const SearchConstraints &a) const;

<<<<<<< HEAD
		bool hasMTime() { return MTime != 0; }
<<<<<<< HEAD
=======

>>>>>>> a9796565f86abdb5c81424cac29324ecff90cb38
=======
		bool hasMTime() { return MTime != 0; } 
>>>>>>> parent of a6778ea... formatted correctly
	};

	NC::Menu<MPD::TagMTime> Tags;
	NC::Menu<SearchConstraints> Albums;
	NC::Menu<MPD::Song> Songs;

protected:
	virtual bool isLockable() OVERRIDE { return true; }

private:
	void AddToPlaylist(bool);
};

extern MediaLibrary *myLibrary;

#endif


