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

#include <map>
#include <set>

#include "ncmpcpp.h"
#include "screen.h"

class MediaLibrary : public Screen<Window>
{
	struct SearchConstraints
	{
		SearchConstraints(const std::string &tag, const std::string &album, const std::string &year) : PrimaryTag(tag), Album(album), Year(year) { }
		SearchConstraints(const std::string &album, const std::string &year) : Album(album), Year(year) { }
		
		std::string PrimaryTag;
		std::string Album;
		std::string Year;
	};
	
	struct SearchConstraintsSorting
	{
		bool operator()(const SearchConstraints &a, const SearchConstraints &b) const;
	};


    // typedefs map keys
    typedef std::pair<mpd_tag_type, bool> album_mtime_flags;
    typedef std::pair<album_mtime_flags,
                      SearchConstraints> album_mtime_key;
    typedef std::pair<mpd_tag_type, std::string> artist_mtime_key;

    // sorting for maps
	struct AlbumMapSorting
	{
        SearchConstraintsSorting scs;

		bool operator()(const album_mtime_key &a, const album_mtime_key &b) const;
	};

    // typedefs for maps
    typedef std::map<album_mtime_key, 
                    time_t, 
                    AlbumMapSorting> album_mtime_map;
    typedef std::map<artist_mtime_key,
                     time_t> artist_mtime_map; 

    // timestamp to make sure maps up to date
    static bool mtimeMapsOutdated;
    // erases maps if out of date
    static void ensureMTimeMapsUpToDate();

    // contains set of primary tags for which map has been initialised
    static std::set<album_mtime_flags> initedAlbumMTimeMaps;
    static album_mtime_map albumMTimeMap;

    static void forceInitedAlbumMTimeMap(const mpd_tag_type primary_tag,
                                         const bool display_date);
    static void updateAlbumMTimeMap(const mpd_tag_type primary_tag,
                                    const bool display_date,
                                    const SearchConstraints &a, 
                                    const time_t time);
    static time_t getAddAlbumMTime(const mpd_tag_type primary_tag,
                                   const bool display_date,
                                   const SearchConstraints &a);
    static time_t getAlbumMTime(const mpd_tag_type primary_tag,
                                const bool display_date,
                                const SearchConstraints &a);
    struct MTimeAlbumSorting 
    {
        bool operator()(const SearchConstraints &a, const SearchConstraints &b);
    };

    static std::set<mpd_tag_type> initedArtistMTimeMaps;
    static artist_mtime_map artistMTimeMap;

    static void forceInitedArtistMTimeMap(const mpd_tag_type primary_tag);
    static time_t getAddArtistMTime(const mpd_tag_type primary_tag,
                                    const std::string &a);
    static time_t getArtistMTime(const mpd_tag_type primary_tag,
                                 const std::string &a);
    static void updateArtistMTimeMap(const mpd_tag_type primary_tag,
                                     const std::string &a, 
                                     const time_t time);
    struct MTimeArtistSorting 
    {
        bool operator()(const std::string &a, const std::string &b);
    };

	
	public:
		virtual void SwitchTo();
		virtual void Resize();
		
		virtual std::basic_string<my_char_t> Title();
		
		virtual void Refresh();
		virtual void Update();
		
		virtual void EnterPressed() { AddToPlaylist(1); }
		virtual void SpacePressed();
		virtual void MouseButtonPressed(MEVENT);
		virtual bool isTabbable() { return true; }
		
		virtual MPD::Song *CurrentSong();
		virtual MPD::Song *GetSong(size_t pos) { return w == Songs ? &Songs->at(pos) : 0; }
		
		virtual bool allowsSelection() { return true; }
		virtual void ReverseSelection();
		virtual void GetSelectedSongs(MPD::SongList &);
		
		virtual void ApplyFilter(const std::string &);
		
		virtual List *GetList();
		
		virtual bool isMergable() { return true; }
		
		int Columns() { return hasTwoColumns ? 2 : 3; }
		bool NextColumn();
		bool PrevColumn();
		
		void LocateSong(const MPD::Song &);
		
		Menu<std::string> *Artists;
		Menu<SearchConstraints> *Albums;
		Menu<MPD::Song> *Songs;

        void DatabaseUpdated() { mtimeMapsOutdated = true; }
		
	protected:
		virtual void Init();
		virtual bool isLockable() { return true; }
		
	private:
		void AddToPlaylist(bool);
		
		static std::string SongToString(const MPD::Song &s, void *);
		
		static std::string AlbumToString(const SearchConstraints &, void *);
		static void DisplayAlbums(const SearchConstraints &, void *, Menu<SearchConstraints> *);
		static void DisplayPrimaryTags(const std::string &artist, void *, Menu<std::string> *menu);
		
		static bool SortSongsByTrack(MPD::Song *, MPD::Song *);
		static bool SortAllTracks(MPD::Song *, MPD::Song *);
		
		static bool hasTwoColumns;
		static size_t itsLeftColStartX;
		static size_t itsLeftColWidth;
		static size_t itsMiddleColWidth;
		static size_t itsMiddleColStartX;
		static size_t itsRightColWidth;
		static size_t itsRightColStartX;
		
		static const char AllTracksMarker[];
};

extern MediaLibrary *myLibrary;

#endif

