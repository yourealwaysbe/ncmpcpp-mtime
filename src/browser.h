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

#ifndef _BROWSER_H
#define _BROWSER_H

#include "interfaces.h"
#include "mpdpp.h"
#include "screen.h"

class Browser : public Screen< NC::Menu<MPD::Item> >, public Filterable, public HasSongs, public Searchable
{
	public:
		Browser() : itsBrowseLocally(0), itsScrollBeginning(0), itsBrowsedDir("/") { }
		
		// Screen< NC::Menu<MPD::Item> > implementation
		virtual void Resize() OVERRIDE;
		virtual void SwitchTo() OVERRIDE;
		
		virtual std::basic_string<my_char_t> Title() OVERRIDE;
		
		virtual void Update() OVERRIDE { }
		
		virtual void EnterPressed() OVERRIDE;
		virtual void SpacePressed() OVERRIDE;
		virtual void MouseButtonPressed(MEVENT me) OVERRIDE;
		
		virtual bool isTabbable() OVERRIDE { return true; }
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
		virtual std::shared_ptr<ProxySongList> getProxySongList() OVERRIDE;
		
		virtual bool allowsSelection() OVERRIDE;
		virtual void reverseSelection() OVERRIDE;
		virtual MPD::SongList getSelectedSongs() OVERRIDE;
		
		// private members
		const std::string &CurrentDir() { return itsBrowsedDir; }
		
		bool isLocal() { return itsBrowseLocally; }
		void LocateSong(const MPD::Song &);
		void GetDirectory(std::string, std::string = "/");
#		ifndef WIN32
		void GetLocalDirectory(MPD::ItemList &, const std::string & = "", bool = 0) const;
		void ClearDirectory(const std::string &) const;
		void ChangeBrowseMode();
		bool deleteItem(const MPD::Item &);
#		endif // !WIN32
		
		static bool isParentDirectory(const MPD::Item &item) {
			return item.type == MPD::itDirectory && item.name == "..";
		}
		
	protected:
		virtual void Init() OVERRIDE;
		virtual bool isLockable() OVERRIDE { return true; }
		
	private:
		bool itsBrowseLocally;
		size_t itsScrollBeginning;
		std::string itsBrowsedDir;
};

extern Browser *myBrowser;

#endif

