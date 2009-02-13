/***************************************************************************
 *   Copyright (C) 2008-2009 by Andrzej Rybczak                            *
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

#ifndef _DISPLAY_H
#define _DISPLAY_H

#include "ncmpcpp.h"
#include "menu.h"
#include "mpdpp.h"

namespace Display
{
	std::string Columns(std::string);
	
	template <class T> void Generic(const T &t, void *, Menu<T> *menu)
	{
		*menu << t;
	}
	
	void TotalPlaylistLength(Window &);
	
	void StringPairs(const string_pair &, void *, Menu<string_pair> *);
	
	void SongsInColumns(const MPD::Song &, void *, Menu<MPD::Song> *);
	
	void Songs(const MPD::Song &, void *, Menu<MPD::Song> *);
	
	void Tags(const MPD::Song &, void *, Menu<MPD::Song> *);
	
	void SearchEngine(const std::pair<Buffer *, MPD::Song *> &, void *, Menu< std::pair<Buffer *, MPD::Song *> > *);
	
	void Items(const MPD::Item &, void *, Menu<MPD::Item> *);
	
	void Clock(Window &, const tm *);
}

namespace Refresh
{
	void MediaLibrary();
	
	void PlaylistEditor();
	
#	ifdef HAVE_TAGLIB_H
	void TagEditor();
#	endif
}

#endif
