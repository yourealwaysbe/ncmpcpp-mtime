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

#include <sys/time.h>
#include <iomanip>

#include "global.h"
#include "helpers.h"
#include "server_info.h"
#include "statusbar.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;

ServerInfo *myServerInfo;

ServerInfo::ServerInfo()
{
	SetDimensions();
	w = NC::Scrollpad((COLS-itsWidth)/2, (MainHeight-itsHeight)/2+MainStartY, itsWidth, itsHeight, "MPD server info", Config.main_color, Config.window_border);
	itsURLHandlers = Mpd.GetURLHandlers();
	itsTagTypes = Mpd.GetTagTypes();
}

void ServerInfo::switchTo()
{
	using Global::myScreen;
	if (myScreen != this)
		SwitchTo::execute(this);
	else
		switchToPreviousScreen();
}

void ServerInfo::resize()
{
	SetDimensions();
	w.resize(itsWidth, itsHeight);
	w.moveTo((COLS-itsWidth)/2, (MainHeight-itsHeight)/2+MainStartY);
	if (previousScreen() && previousScreen()->hasToBeResized) // resize background window
	{
		previousScreen()->resize();
		previousScreen()->refresh();
	}
	hasToBeResized = 0;
}

std::wstring ServerInfo::title()
{
	return previousScreen()->title();
}

void ServerInfo::update()
{
	static timeval past = { 0, 0 };
	if (Global::Timer.tv_sec <= past.tv_sec)
		return;
	past = Global::Timer;

	MPD::Statistics stats = Mpd.getStatistics();
	if (stats.empty())
		return;

	w.clear();

	w << NC::fmtBold << L"Version: " << NC::fmtBoldEnd << L"0." << Mpd.Version() << L".*\n";
	w << NC::fmtBold << L"Uptime: " << NC::fmtBoldEnd;
	ShowTime(w, stats.uptime(), 1);
	w << '\n';
	w << NC::fmtBold << L"Time playing: " << NC::fmtBoldEnd << MPD::Song::ShowTime(stats.playTime()) << '\n';
	w << '\n';
	w << NC::fmtBold << L"Total playtime: " << NC::fmtBoldEnd;
	ShowTime(w, stats.dbPlayTime(), 1);
	w << '\n';
	w << NC::fmtBold << L"Artist names: " << NC::fmtBoldEnd << stats.artists() << '\n';
	w << NC::fmtBold << L"Album names: " << NC::fmtBoldEnd << stats.albums() << '\n';
	w << NC::fmtBold << L"Songs in database: " << NC::fmtBoldEnd << stats.songs() << '\n';
	w << '\n';
	w << NC::fmtBold << L"Last DB update: " << NC::fmtBoldEnd << Timestamp(stats.dbUpdateTime()) << '\n';
	w << '\n';
	w << NC::fmtBold << L"URL Handlers:" << NC::fmtBoldEnd;
	for (auto it = itsURLHandlers.begin(); it != itsURLHandlers.end(); ++it)
		w << (it != itsURLHandlers.begin() ? L", " : L" ") << *it;
	w << L"\n\n";
	w << NC::fmtBold << L"Tag Types:" << NC::fmtBoldEnd;
	for (auto it = itsTagTypes.begin(); it != itsTagTypes.end(); ++it)
		w << (it != itsTagTypes.begin() ? L", " : L" ") << *it;

	w.flush();
	w.refresh();
}

void ServerInfo::SetDimensions()
{
	itsWidth = COLS*0.6;
	itsHeight = std::min(size_t(LINES*0.7), MainHeight);
}

