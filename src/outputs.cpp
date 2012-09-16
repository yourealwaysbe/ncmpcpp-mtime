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

#include "outputs.h"

#ifdef ENABLE_OUTPUTS

#include "display.h"
#include "global.h"
#include "settings.h"
#include "status.h"
#include "statusbar.h"
#include "title.h"
#include "screen_switcher.h"

using Global::MainHeight;
using Global::MainStartY;
using Global::myScreen;

Outputs *myOutputs;

Outputs::Outputs()
: Screen(NC::Menu<MPD::Output>(0, MainStartY, COLS, MainHeight, "", Config.main_color, NC::brNone))
{
	w.cyclicScrolling(Config.use_cyclic_scrolling);
	w.centeredCursor(Config.centered_cursor);
	w.setHighlightColor(Config.main_highlight_color);
	w.setItemDisplayer(Display::Outputs);
	FetchList();
}

void Outputs::switchTo()
{
	SwitchTo::execute(this);
	drawHeader();
}

void Outputs::resize()
{
	size_t x_offset, width;
	getWindowResizeParams(x_offset, width);
	w.resize(width, MainHeight);
	w.moveTo(x_offset, MainStartY);
	hasToBeResized = 0;
}

std::wstring Outputs::title()
{
	return L"Outputs";
}

void Outputs::enterPressed()
{
	if (w.current().value().isEnabled())
	{
		if (Mpd.DisableOutput(w.choice()))
			Statusbar::msg("Output \"%s\" disabled", w.current().value().name().c_str());
	}
	else
	{
		if (Mpd.EnableOutput(w.choice()))
			Statusbar::msg("Output \"%s\" enabled", w.current().value().name().c_str());
	}
	if (!Mpd.SupportsIdle())
		FetchList();
}

void Outputs::mouseButtonPressed(MEVENT me)
{
	if (w.empty() || !w.hasCoords(me.x, me.y) || size_t(me.y) >= w.size())
		return;
	if (me.bstate & BUTTON1_PRESSED || me.bstate & BUTTON3_PRESSED)
	{
		w.Goto(me.y);
		if (me.bstate & BUTTON3_PRESSED)
			enterPressed();
	}
	else
		Screen<WindowType>::mouseButtonPressed(me);
}

void Outputs::FetchList()
{
	w.clear();
	auto outputs = Mpd.GetOutputs();
	for (auto o = outputs.begin(); o != outputs.end(); ++o)
		w.addItem(*o, o->isEnabled());
	if (myScreen == this)
		w.refresh();
}

#endif // ENABLE_OUTPUTS

