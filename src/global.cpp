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

#include "global.h"

BasicScreen *Global::myScreen;
BasicScreen *Global::myOldScreen;
BasicScreen *Global::myPrevScreen;
BasicScreen *Global::myLockedScreen;
BasicScreen *Global::myInactiveScreen;

NC::Window *Global::wHeader;
NC::Window *Global::wFooter;

size_t Global::MainStartY;
size_t Global::MainHeight;

bool Global::ShowMessages = false;
bool Global::SeekingInProgress = false;

bool Global::RedrawHeader = true;
bool Global::RedrawStatusbar = true;

std::string Global::VolumeState;
timeval Global::Timer;
