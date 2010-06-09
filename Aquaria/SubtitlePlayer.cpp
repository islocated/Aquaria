/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include "DSQ.h"

#include "../BBGE/DebugFont.h"
#include "../BBGE/BitmapFont.h"

//#define DISABLE_SUBS

SubtitlePlayer::SubtitlePlayer()
{
	curLine =0;
	vis = false;
}

bool SubtitlePlayer::isVisible()
{
	return vis;
}

void SubtitlePlayer::go(const std::string &subs)
{
	subLines.clear();

	std::string f;
	bool checkAfter = true;
	if (dsq->mod.isActive())
	{
		f = dsq->mod.getPath() + "audio/" + subs + ".txt";
		stringToLower(f);
		if (exists(f))
			checkAfter = false;
	}

	if (checkAfter)
	{
		f = "scripts/vox/" + subs + ".txt";
		stringToLower(f);
		if (!exists(f))
		{
			debugLog("Could not find subs file [" + subs + "]");
		}
	}

	std::ifstream in(f.c_str());
	std::string line;
	while (std::getline(in, line))
	{
		std::istringstream is(line);

		SubLine sline;

		std::string timeStamp;
		is >> timeStamp;
		int loc = timeStamp.find(':');
		if (loc != std::string::npos)
		{
			int minutes=0, seconds=0;
			std::istringstream is(timeStamp.substr(0, loc));
			is >> minutes;
			sline.timeStamp = minutes*60;
			std::istringstream is2(timeStamp.substr(loc+1, timeStamp.size()));
			is2 >> seconds;
			sline.timeStamp += seconds;
			sline.line = line.substr(loc+4, line.size());
		}
		subLines.push_back(sline);
	}
	curLine = 0;
}

void SubtitlePlayer::end()
{
	dsq->subtext->alpha.interpolateTo(0, 1);
	dsq->subbox->alpha.interpolateTo(0, 1.2);

	vis = false;
}

void SubtitlePlayer::forceOff()
{
	dsq->subtext->alpha = 0;
	dsq->subbox->alpha = 0;
	vis = false;
}

void SubtitlePlayer::update(float dt)
{
	dt = core->get_old_dt();

	float time = dsq->sound->getVoiceTime();

	if (dsq->subtext && dsq->subbox)
	{
		dsq->subtext->useOldDT = true;
		dsq->subbox->useOldDT = true;
	}

#ifndef DISABLE_SUBS
	if (curLine < subLines.size())
	{
		if (subLines[curLine].timeStamp <= time)
		{
			// present line
			// set text
			debugLog(subLines[curLine].line);
			dsq->subtext->scrollText(subLines[curLine].line, 0.02);
			//dsq->subtext->scrollText(subLines[curLine].line, 0.1);
			dsq->subtext->alpha.interpolateTo(1, 1);
			dsq->subbox->alpha.interpolateTo(1, 0.1);

			vis = true;

			// advance
			curLine++;
		}
	}
#endif

	if (vis && !sound->isPlayingVoice())
	{
		end();
	}
}

