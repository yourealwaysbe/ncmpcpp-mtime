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

#include "lastfm.h"

#ifdef HAVE_CURL_CURL_H

#ifdef WIN32
# include <io.h>
#else
# include <sys/stat.h>
#endif // WIN32

#include <cassert>
#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>

#include "helpers.h"
#include "charset.h"
#include "global.h"
#include "statusbar.h"
#include "title.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

Lastfm *myLastfm;

Lastfm::Lastfm()
: Screen(NC::Scrollpad(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::brNone))
, isReadyToTake(0), isDownloadInProgress(0)
{ }

void Lastfm::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

std::wstring Lastfm::title()
{
	return itsTitle;
}

void Lastfm::update()
{
	if (isReadyToTake)
		Take();
}

void Lastfm::Take()
{
	assert(isReadyToTake);
	pthread_join(itsDownloader, 0);
	w.flush();
	w.refresh();
	isDownloadInProgress = 0;
	isReadyToTake = 0;
}

void Lastfm::switchTo()
{
	using Global::myScreen;
	if (myScreen != this)
	{
		SwitchTo::execute(this);
		// get an old info if it waits
		if (isReadyToTake)
			Take();
		Load();
		drawHeader();
	}
	else
		switchToPreviousScreen();
}

void Lastfm::Load()
{
	if (isDownloadInProgress)
		return;

	assert(itsService.get());
	assert(itsService->checkArgs(itsArgs));

	SetTitleAndFolder();

	w.clear();
	w.reset();

	std::string artist = itsArgs.find("artist")->second;
	std::string file = lowercase(artist + ".txt");
	removeInvalidCharsFromFilename(file);

	itsFilename = itsFolder + "/" + file;

	mkdir(itsFolder.c_str()
#	ifndef WIN32
	, 0755
#	endif // !WIN32
	     );

	std::ifstream input(itsFilename.c_str());
	if (input.is_open())
	{
		bool first = 1;
		std::string line;
		while (getline(input, line))
		{
			if (!first)
				w << '\n';
			IConv::utf8ToLocale_(line);
			w << line;
			first = 0;
		}
		input.close();
		itsService->colorizeOutput(w);
	}
	else
	{
		w << L"Fetching informations... ";
		pthread_create(&itsDownloader, 0, DownloadWrapper, this);
		isDownloadInProgress = 1;
	}
	w.flush();
}

void Lastfm::SetTitleAndFolder()
{
	if (dynamic_cast<ArtistInfo *>(itsService.get()))
	{
		itsTitle = L"Artist info - ";
		itsTitle += ToWString(itsArgs.find("artist")->second);
		itsFolder = Config.ncmpcpp_directory + "artists";
	}
}

void *Lastfm::DownloadWrapper(void *this_ptr)
{
	static_cast<Lastfm *>(this_ptr)->Download();
	pthread_exit(0);
}

void Lastfm::Download()
{
	LastfmService::Result result = itsService->fetch(itsArgs);

	if (result.first)
	{
		Save(result.second);
		w.clear();
		IConv::utf8ToLocale_(result.second);
		w << result.second;
		itsService->colorizeOutput(w);
	}
	else
		w << NC::clRed << result.second << NC::clEnd;

	isReadyToTake = 1;
}

void Lastfm::Save(const std::string &data)
{
	std::ofstream output(itsFilename.c_str());
	if (output.is_open())
	{
		output << data;
		output.close();
	}
	else
		Error("couldn't save file \"" << itsFilename << "\"");
}

void Lastfm::Refetch()
{
	if (remove(itsFilename.c_str()) && errno != ENOENT)
	{
		const char msg[] = "Couldn't remove \"%ls\": %s";
		Statusbar::msg(msg, wideShorten(ToWString(itsFilename), COLS-const_strlen(msg)-25).c_str(), strerror(errno));
		return;
	}
	Load();
}

bool Lastfm::SetArtistInfoArgs(const std::string &artist, const std::string &lang)
{
	if (isDownloading())
		return false;

	itsService.reset(new ArtistInfo);
	itsArgs.clear();
	itsArgs["artist"] = artist;
	if (!lang.empty())
		itsArgs["lang"] = lang;

	return true;
}

#endif // HVAE_CURL_CURL_H

