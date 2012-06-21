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

#include "charset.h"
#include "display.h"
#include "helpers.h"
#include "global.h"
#include "media_library.h"
#include "mpdpp.h"
#include "playlist.h"
#include "status.h"





////////////////////////////////////////////////////////////////////
// Simple ones


bool SearchConstraintsSorting::operator()(const SearchConstraints &a, const SearchConstraints &b) const
{
	int result;
	CaseInsensitiveStringComparison cmp;
	result = cmp(a.PrimaryTag, b.PrimaryTag);
	if (result != 0)
		return result < 0;
	result = cmp(a.Year, b.Year);
	return (result == 0 ? cmp(a.Album, b.Album) : result) < 0;
}


bool SortSongsByTrack(MPD::Song *a, MPD::Song *b)
{
	if (a->GetDisc() == b->GetDisc())
		return StrToInt(a->GetTrack()) < StrToInt(b->GetTrack());
	else
		return StrToInt(a->GetDisc()) < StrToInt(b->GetDisc());
}

bool SortAllTracks(MPD::Song *a, MPD::Song *b)
{
	static MPD::Song::GetFunction gets[] = { &MPD::Song::GetDate, &MPD::Song::GetAlbum, &MPD::Song::GetDisc, 0 };
	CaseInsensitiveStringComparison cmp;
	for (MPD::Song::GetFunction *get = gets; *get; ++get)
		if (int ret = cmp(a->GetTags(*get), b->GetTags(*get)))
			return ret < 0;
	return a->GetTrack() < b->GetTrack();
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


time_t MTimeArtistSorting::getAddArtistMTime(const mpd_tag_type primary_tag,
                                             const std::string &a) {
    forceInitedArtistMTimeMap(primary_tag);

    artist_mtime_map::iterator it;
    artist_mtime_key key = std::make_pair(primary_tag, a);
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
        MPD::SongList list;
        Mpd.GetDirectoryRecursive("/", list);
        for (MPD::SongList::const_iterator it = list.begin(); 
             it != list.end(); 
            ++it) {
            updateArtistMTimeMap(primary_tag,
                                 (*it)->GetTag(primary_tag),
                                 (*it)->GetMTime());
        }

        initedArtistMTimeMaps.insert(primary_tag);
    }
}

void MTimeArtistSorting::updateArtistMTimeMap(const mpd_tag_type primary_tag,
                                              const std::string &a, 
                                              const time_t time) {
    artist_mtime_map::iterator it;
    artist_mtime_key key = std::make_pair(primary_tag, a);
    it = artistMTimeMap.find(key);
    if (it == artistMTimeMap.end()) {
        artistMTimeMap.insert(std::make_pair(key, time));
    } else {
        it->second = std::max(it->second, time);
    }
}


time_t MTimeArtistSorting::getArtistMTime(const mpd_tag_type primary_tag,
                                          const std::string &a) {
    MPD::SongList list;
		
    Mpd.StartSearch(1);
	Mpd.AddSearch(primary_tag, locale_to_utf_cpy(a));
    Mpd.CommitSearch(list);
		
    time_t time = 0;
    for (MPD::SongList::const_iterator it = list.begin(); 
         it != list.end(); 
         ++it) {
        time = std::max(time, (*it)->GetMTime());
	}
    return time;
}



///////////////////////////////////////////////////////////////////////////
// Album Sorting


std::set<MTimeAlbumSorting::album_mtime_flags> 
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


bool MTimeAlbumSorting::AlbumMapSorting::operator()(const album_mtime_key &a, 
                                                    const album_mtime_key &b) const 
{
    if (a.first == b.first) {
        return scs(a.second, b.second);
    } else {
        return a.first < b.first;
    }
}


time_t MTimeAlbumSorting::getAddAlbumMTime(const mpd_tag_type primary_tag,
                                           const bool display_date,
                                           const SearchConstraints &a) {
    forceInitedAlbumMTimeMap(primary_tag, display_date);

    album_mtime_flags f = std::make_pair(primary_tag, display_date);
    album_mtime_key key = std::make_pair(f, a);
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
    MPD::SongList list;
		
    Mpd.StartSearch(1);
    Mpd.AddSearch(MPD_TAG_ALBUM, locale_to_utf_cpy(a.Album));
    Mpd.AddSearch(MPD_TAG_DATE, locale_to_utf_cpy(a.Year));

    if (a.PrimaryTag.length() > 0) {
	    Mpd.AddSearch(primary_tag,
                      locale_to_utf_cpy(a.PrimaryTag));
    }
    if (display_date) {
	    Mpd.AddSearch(MPD_TAG_DATE, locale_to_utf_cpy(a.Year));
    }
    Mpd.CommitSearch(list);
		
    time_t time = 0;
    for (MPD::SongList::const_iterator it = list.begin(); 
         it != list.end(); 
         ++it) {
        time = std::max(time, (*it)->GetMTime());
	}
    return time;
}

void MTimeAlbumSorting::forceInitedAlbumMTimeMap(const mpd_tag_type primary_tag,
                                                 const bool display_date) {
    album_mtime_flags f = std::make_pair(primary_tag, display_date);
    if (initedAlbumMTimeMaps.count(f) == 0) {
        MPD::SongList list;
        Mpd.GetDirectoryRecursive("/", list);
        for (MPD::SongList::const_iterator it = list.begin(); 
             it != list.end(); 
            ++it) {
            std::string date = display_date ? (*it)->GetDate() : "";
            updateAlbumMTimeMap(primary_tag,
                                display_date,
                                SearchConstraints((*it)->GetTag(primary_tag),
                                                  (*it)->GetAlbum(),
                                                  date),
                                (*it)->GetMTime());
	    }

        initedAlbumMTimeMaps.insert(f);
    }
}

void MTimeAlbumSorting::updateAlbumMTimeMap(const mpd_tag_type primary_tag,
                                            const bool display_date,
                                            const SearchConstraints &a, 
                                            const time_t time) {
    album_mtime_map::iterator it;
    album_mtime_flags f = std::make_pair(primary_tag, display_date);
    album_mtime_key key = std::make_pair(f, a);
    it = albumMTimeMap.find(key);
    if (it == albumMTimeMap.end()) {
        albumMTimeMap.insert(std::make_pair(key, time));
    } else {
        it->second = std::max(it->second, time);
    }
}



