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
#include "GridRender.h"
#include "Game.h"

GridRender::GridRender(ObsType obsType) : RenderObject()
{
	color = Vector(1, 0, 0);
	//color = Vector(0.2,0.2,1);
	position.z = 5;
	cull = false;
	alpha = 0.5f;
	this->obsType = obsType;
	blendEnabled = false;
	//setTexture("grid");
}

void GridRender::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);	
	if (obsType != OT_BLACK) { blendEnabled = true; }
}

void GridRender::onRender()
{	
	float width2 = float(TILE_SIZE)/2.0;
	float height2 = float(TILE_SIZE)/2.0;
	int skip = 1;
	
	if (width2 < 5)
		skip = 2;
	
	/*
	int obsType2 = obsType;	
	if (obsType == OT_INVISIBLEIN)
		obsType2 = OT_HURT;
	*/
	float drawx1, drawx2, drawy;
	int startRow = -1;
	int endRow;
	Vector camPos = core->cameraPos;
	camPos.x -= core->getVirtualOffX() * (core->invGlobalScale);
	TileVector ct(camPos);

	int width = (core->getVirtualWidth() * (core->invGlobalScale))/TILE_SIZE + 1;
	int height = (600 * (core->invGlobalScale))/TILE_SIZE + 1;

	for (int y = ct.y; y < ct.y+height+1; y+=skip)
	{			
		if (y<0) continue;
		startRow = -1;
		int endX = ct.x+width+3; // +1
		for (int x = ct.x-3; x <= endX; x++)
		{
			if (x < 0) continue;
			int v = dsq->game->getGrid(TileVector(x, y));
			if (v && v == int(obsType) && startRow == -1)
			{
				startRow = x;
			}
			else if ((!v || v != int(obsType) || x == endX) && startRow != -1)
			{				
				float oldHeight2 = height2;
				if (skip > 1)
					height2 *= skip;
				endRow = x-1;
				switch(obsType)
				{
				case OT_INVISIBLE:
					core->setColor(1,0,0,alpha.getValue());
				break;
				case OT_INVISIBLEIN:
					core->setColor(1, 0.5, 0, alpha.getValue());
				break;
				case OT_BLACK:
					core->setColor(0, 0, 0, 1); 
					startRow++;
					endRow--;
				break;
				case OT_HURT:
					core->setColor(1,1,0,alpha.getValue());
				break;
				default:
				break;
				}

				drawx1 = (startRow*TILE_SIZE+width2);
				drawx2 = (endRow*TILE_SIZE+width2);
				drawy = (y*TILE_SIZE+height2);
				/*
				if (drawx1 > ((dsq->cameraPos.x - width/2)+40))
					drawx1 += 20;
				if (drawx2 < ((dsq->cameraPos.x + width/2)-20))
					drawx2 -= 20;
				*/

#ifdef BBGE_BUILD_OPENGL
				glBegin(GL_QUADS);
					glVertex3f(drawx1-width2, drawy+height2,  0.0f);
					glVertex3f(drawx2+width2, drawy+height2,  0.0f);
					glVertex3f(drawx2+width2, drawy-height2,  0.0f);
					glVertex3f(drawx1-width2, drawy-height2,  0.0f);
				glEnd();
#endif

#ifdef BBGE_BUILD_DIRECTX
				core->blitD3DVerts(0,
					drawx1-width2, drawy-height2,
					drawx2+width2, drawy-height2,
					drawx2+width2, drawy+height2,
					drawx1-width2, drawy+height2);
#endif
				height2 = oldHeight2;
				startRow = -1;
			}
		}
	}
}


SongLineRender::SongLineRender()
{
	followCamera = 1;
	cull = false;
}

void SongLineRender::newPoint(const Vector &pt, const Vector &color)
{
	int maxx = 40;
	bool inRange = true;
	if (pts.size() > 1)
		inRange = (pt - pts[pts.size()-2].pt).isLength2DIn(4);
	if (pts.size()<2 || !inRange)
	{
		SongLinePoint s;
		s.pt = pt;
		s.color = color;
		pts.push_back(s);
		if (pts.size() > maxx)
		{
			std::vector<SongLinePoint> copy;
			copy = pts;
			pts.clear();
			for (int i = 1; i < copy.size(); i++)
			{
				pts.push_back(copy[i]);
			}
		}
			
	}
	else if (!pts.empty() && inRange)
	{
		pts[pts.size()-1].color = color;
		pts[pts.size()-1].pt = pt;
	}
}

void SongLineRender::clear()
{
	pts.clear();
}

void SongLineRender::onRender()
{
	int w=core->getWindowWidth();
	//core->getWindowWidth(&w);
	int ls = (4*w)/1024.0;
	if (ls < 0)
		ls = 1;
#ifdef BBGE_BUILD_OPENGL
	glLineWidth(ls);
	const int alphaLine = pts.size()*(0.9);
	float a = 1;
	glBegin(GL_LINE_STRIP);
	for (int i = 0; i < pts.size(); i++)
	{
		if (i < alphaLine)
			a = float(i)/float(alphaLine);
		else
			a = 1;		
		glColor4f(pts[i].color.x, pts[i].color.y, pts[i].color.z, a);
		glVertex2f(pts[i].pt.x, pts[i].pt.y);
	}
	glEnd();
#endif
}

