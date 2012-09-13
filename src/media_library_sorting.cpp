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



#include <algorithm>
#include <utility>
#include <array>

#include "charset.h"
#include "display.h"
#include "helpers.h"
#include "global.h"
#include "media_library.h"
#include "mpdpp.h"
#include "playlist.h"
#include "status.h"
#include "utility/comparators.h"
#include "utility/type_conversions.h"


///////////////////////////////////////////////////////////
// Lexico


template <typename T1, typename T2> 
bool lexico(const T1 &a1, const T1 &a2, 
            const T2 &b1, const T2 &b2) {
    if (a1 < a2)
        return true;
    else if (a2 < a1)
        return false;
    else
        return (b1 < b2);
}

template <typename T1, typename T2, typename T3> 
bool lexico(const T1 &a1, const T1 &a2, 
            const T2 &b1, const T2 &b2,
            const T3 &c1, const T3 &c2) {
    if (a1 < a2)
        return true;
    else if (a2 < a1)
        return false;
    else
        return lexico(b1, b2,
                      c1, c2);
}





////////////////////////////////////////////////////////////////////
// Simple ones


bool SearchConstraints::operator<(const SearchConstraints &a) const {
    return lexico(PrimaryTag, a.PrimaryTag,
                  Album, a.Album,
                  Date, a.Date);
}


bool SortSearchConstraints::operator()(const SearchConstraints &a,  
                                       const SearchConstraints &b) const {
    int result;
    result = m_cmp(a.PrimaryTag, b.PrimaryTag);
    if (result != 0)
        return result < 0;
    result = m_cmp(a.Date, b.Date);
    if (result != 0)
        return result < 0;
    return m_cmp(a.Album, b.Album) < 0;
}

 

bool SortSongsByTrack(const MPD::Song &a, const MPD::Song &b)
{
	int cmp = a.getDisc().compare(a.getDisc());
	if (cmp != 0)
		return cmp;
	return a.getTrack() < b.getTrack();
}


SortAllTracks::SortAllTracks() : m_gets({{
		&MPD::Song::getDate,
		&MPD::Song::getAlbum,
		&MPD::Song::getDisc
	}}), m_cmp(std::locale(), Config.ignore_leading_the) { 
    // all done above    
}

bool SortAllTracks::operator()(const MPD::Song &a, const MPD::Song &b) {
	for (auto get = m_gets.begin(); get != m_gets.end(); ++get) {
		int ret = m_cmp(a.getTags(*get, Config.tags_separator),
                        b.getTags(*get, Config.tags_separator));
		if (ret != 0)
			return ret < 0;
	}
	return a.getTrack() < b.getTrack();
}




///////////////////////////////////////////////////////////////////////
// Artist Sorting

std::set<mpd_tag_type> MTimeArtistSorting::initedArtistMTimeMaps;
MTimeArtistSorting::artist_mtime_map MTimeArtistSorting::artistMTimeMap;



bool MTimeArtistSorting::operator()(const std::string &a, 
                                    const std::string &b)
{
    mpd_tag_type tt = Config.media_lib_primary_tag;
    time_t ta = getAddArtistMTime(tt, a);
    time_t tb = getAddArtistMTime(tt, b);
    return ta > tb;
}

void MTimeArtistSorting::DatabaseUpdated() {
    artistMTimeMap.clear();
    initedArtistMTimeMaps.clear();
}


bool MTimeArtistSorting::ArtistMTimeKey::operator<(const ArtistMTimeKey &a) const {
    return lexico(TagType, a.TagType,
                  Artist, a.Artist);
}


time_t MTimeArtistSorting::getAddArtistMTime(const mpd_tag_type primary_tag,
                                             const std::string &a) {
    forceInitedArtistMTimeMap(primary_tag);

    artist_mtime_map::iterator it;
    ArtistMTimeKey key = ArtistMTimeKey(primary_tag, a);
    it = artistMTimeMap.find(key);
    time_t time = 0;
    if (it == artistMTimeMap.end()) {
        time = getArtistMTime(primary_tag, a);
        artistMTimeMap.insert(std::make_pair(key, time));
    } else {
        time = it->second;
    }
    return time;
}

void MTimeArtistSorting::forceInitedArtistMTimeMap(const mpd_tag_type primary_tag) {
    if (initedArtistMTimeMaps.count(primary_tag) == 0) {
        auto list = Mpd.GetDirectoryRecursive("/");
        for (MPD::SongList::const_iterator it = list.begin(); 
             it != list.end(); 
            ++it) {
            updateArtistMTimeMap(primary_tag,
                                 (*it).getTag(primary_tag),
                                 (*it).getMTime());
        }

        initedArtistMTimeMaps.insert(primary_tag);

        // finally, because we need album sorting eventually
        MTimeAlbumSorting::InitMapsWith(primary_tag,
                                        // best guess at whether we need to
                                        // display date
                                        Config.media_library_display_date,
                                        list);
    }
}

void MTimeArtistSorting::updateArtistMTimeMap(const mpd_tag_type primary_tag,
                                              const std::string &a, 
                                              const time_t time) {
    artist_mtime_map::iterator it;
    ArtistMTimeKey key = ArtistMTimeKey(primary_tag, a);
    it = artistMTimeMap.find(key);
    if (it == artistMTimeMap.end()) {
        artistMTimeMap.insert(std::make_pair(key, time));
    } else {
        it->second = std::max(it->second, time);
    }
}


time_t MTimeArtistSorting::getArtistMTime(const mpd_tag_type primary_tag,
                                          const std::string &a) {
    Mpd.StartSearch(1);
	Mpd.AddSearch(primary_tag, a);
    auto list = Mpd.CommitSearchSongs();
		
    time_t time = 0;
    for (MPD::SongList::const_iterator it = list.begin(); 
         it != list.end(); 
         ++it) {
        time = std::max(time, (*it).getMTime());
	}
    return time;
}



///////////////////////////////////////////////////////////////////////////
// Album Sorting


std::set<MTimeAlbumSorting::AlbumMTimeFlags> 
    MTimeAlbumSorting::initedAlbumMTimeMaps;

MTimeAlbumSorting::album_mtime_map MTimeAlbumSorting::albumMTimeMap;


bool MTimeAlbumSorting::operator()(const SearchConstraints &a, 
                                   const SearchConstraints &b)
{
    mpd_tag_type tt = Config.media_lib_primary_tag;
    bool dd = Config.media_library_display_date;
    time_t ta = getAddAlbumMTime(tt, dd, a);
    time_t tb = getAddAlbumMTime(tt, dd, b);
    return ta > tb;
}



void MTimeAlbumSorting::DatabaseUpdated() {
   albumMTimeMap.clear();
   initedAlbumMTimeMaps.clear();
}

void MTimeAlbumSorting::InitMapsWith(mpd_tag_type primary_tag,
                                     bool display_date,
                                     const MPD::SongList &list) {
    AlbumMTimeFlags f = AlbumMTimeFlags(primary_tag, display_date);
    for (MPD::SongList::const_iterator it = list.begin(); 
         it != list.end(); 
        ++it) {
        std::string date = display_date ? (*it).getDate() : "";
        updateAlbumMTimeMap(primary_tag,
                            display_date,
                            SearchConstraints((*it).getTag(primary_tag),
                                              (*it).getAlbum(),
                                              date),
                            (*it).getMTime());
	}

    initedAlbumMTimeMaps.insert(f);
}


bool MTimeAlbumSorting::AlbumMTimeFlags::operator<(const AlbumMTimeFlags &a) const {
    return lexico(TagType, a.TagType,
                  DisplayDate, a.DisplayDate);
}


bool MTimeAlbumSorting::AlbumMTimeKey::operator<(const AlbumMTimeKey &a) const {
    return lexico(Flags, a.Flags,
                  Constraints, a.Constraints);
}


time_t MTimeAlbumSorting::getAddAlbumMTime(const mpd_tag_type primary_tag,
                                           const bool display_date,
                                           const SearchConstraints &a) {
    forceInitedAlbumMTimeMap(primary_tag, display_date);

    AlbumMTimeFlags f = AlbumMTimeFlags(primary_tag, display_date);
    AlbumMTimeKey key = AlbumMTimeKey(f, a);
    album_mtime_map::iterator it;
    it = albumMTimeMap.find(key);
    time_t time = 0;
    if (it == albumMTimeMap.end()) {
        time = getAlbumMTime(primary_tag, display_date, a);
        albumMTimeMap.insert(std::make_pair(key, time));
    } else {
        time = it->second;
    }
    return time;
}

time_t MTimeAlbumSorting::getAlbumMTime(const mpd_tag_type primary_tag, 
                                        const bool display_date,
                                        const SearchConstraints &a) {
    // make this the newest song with same album tag
    Mpd.StartSearch(1);
    Mpd.AddSearch(MPD_TAG_ALBUM, a.Album);
    Mpd.AddSearch(MPD_TAG_DATE, a.Date);

    if (a.PrimaryTag.length() > 0) {
	    Mpd.AddSearch(primary_tag,
                      a.PrimaryTag);
    }
    if (display_date) {
	    Mpd.AddSearch(MPD_TAG_DATE, a.Date);
    }
    auto list = Mpd.CommitSearchSongs();
		
    time_t time = 0;
    for (MPD::SongList::const_iterator it = list.begin(); 
         it != list.end(); 
         ++it) {
        time = std::max(time, (*it).getMTime());
	}
    return time;
}

void MTimeAlbumSorting::forceInitedAlbumMTimeMap(const mpd_tag_type primary_tag,
                                                 const bool display_date) {
    AlbumMTimeFlags f = AlbumMTimeFlags(primary_tag, display_date);
    if (initedAlbumMTimeMaps.count(f) == 0) {
        auto list = Mpd.GetDirectoryRecursive("/");
        InitMapsWith(primary_tag, display_date, list);
    }
}

void MTimeAlbumSorting::updateAlbumMTimeMap(const mpd_tag_type primary_tag,
                                            const bool display_date,
                                            const SearchConstraints &a, 
                                            const time_t time) {
    album_mtime_map::iterator it;
    AlbumMTimeFlags f = AlbumMTimeFlags(primary_tag, display_date);
    AlbumMTimeKey key = AlbumMTimeKey(f, a);
    it = albumMTimeMap.find(key);
    if (it == albumMTimeMap.end()) {
        albumMTimeMap.insert(std::make_pair(key, time));
    } else {
        it->second = std::max(it->second, time);
    }
}



