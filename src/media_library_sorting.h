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

#ifndef _H_MEDIA_LIBRARY_SORTING
#define _H_MEDIA_LIBRARY_SORTING

#include <map>
#include <set>

#include "ncmpcpp.h"



bool SortSongsByTrack(MPD::Song *, MPD::Song *);
bool SortAllTracks(MPD::Song *, MPD::Song *);

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


class MTimeArtistSorting 
{
    public:
        
        bool operator()(const std::string &a, const std::string &b);

        static void DatabaseUpdated();

    private:        

        typedef std::pair<mpd_tag_type, std::string> artist_mtime_key;


        typedef std::map<artist_mtime_key,
                         time_t> artist_mtime_map; 

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
        
};




class MTimeAlbumSorting 
{
    public:

        bool operator()(const SearchConstraints &a, const SearchConstraints &b);

        static void DatabaseUpdated();


    private:
        typedef std::pair<mpd_tag_type, bool> album_mtime_flags;
        typedef std::pair<album_mtime_flags,
                          SearchConstraints> album_mtime_key;

        struct AlbumMapSorting
        {
            SearchConstraintsSorting scs;

            bool operator()(const album_mtime_key &a, 
                            const album_mtime_key &b) const;
        };

        typedef std::map<album_mtime_key, 
                        time_t, 
                        AlbumMapSorting> album_mtime_map;

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

};

#endif // _H_MEDIA_LIBRARY_SORTING

