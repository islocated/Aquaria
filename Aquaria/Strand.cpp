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
#include "Segmented.h"

Strand::Strand(const Vector &position, int segs, int dist) : RenderObject(), Segmented(dist, dist)
{
	cull = false;
	segments.resize(segs);
	for (int i = 0; i < segments.size(); i++)
	{
		segments[i] = new RenderObject;
	}
	initSegments(position);
}

void Strand::destroy()
{
	RenderObject::destroy();
	for (int i = 0; i < segments.size(); i++)
	{
		segments[i]->destroy();
		delete segments[i];
	}
	segments.clear();
}

void Strand::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);
	updateSegments(position);
}

void Strand::onRender()
{
#ifdef BBGE_BUILD_OPENGL
	if (segments.empty()) return;
	glEnable(GL_BLEND);
	glTranslatef(-position.x, -position.y, 0);
	glLineWidth(1);

	glBegin(GL_LINES);
	//glColor4f(0.25,0.25,0.5,1);
	glColor4f(color.x, color.y, color.z, 1);
	glVertex2f(position.x, position.y);
	glVertex2f(segments[0]->position.x, segments[0]->position.y);
	float x,y;
	float bit = 1.0/segments.size();
	for (int i =0; i < segments.size()-1; i++)
	{
		float d=i*0.02;
		float d2=i*bit;
		float used = 1 - d;
		if (used < 0) used = 0;
		glColor4f(used*color.x,used*color.y,used*color.z, 1-d2);
		x = segments[i]->position.x;
		y = segments[i]->position.y;		
		glVertex2f(x, y);
		glColor4f(used*color.x,used*color.y,used*color.z, 1-d2-bit);
		x = segments[i+1]->position.x;
		y = segments[i+1]->position.y;
		glVertex2f(x, y);
	}
	glEnd();
#endif
}
