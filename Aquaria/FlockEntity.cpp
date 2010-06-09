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
#include "FlockEntity.h"

FlockEntity::Flock FlockEntity::flocks;

FlockEntity::FlockEntity() : CollideEntity()
{
	flockType = FLOCK_FISH;
	flockID = 0;

	angle = 0;
	flocks.push_back(this);

	collideRadius = 8;
}

void FlockEntity::destroy()
{
	/*
	Flock copy = flocks;
	flocks.clear();
	for (Flock::iterator i = copy.begin(); i != flock.copy
	{
		if (copy[i] != this)
			flocks.push_back(copy[i]);
	}
	copy.clear();
	*/
	flocks.remove(this);
	CollideEntity::destroy();
}

Vector FlockEntity::getFlockCenter()
{
	Vector position;
	int sz = 0;
	for (Flock::iterator i = flocks.begin(); i != flocks.end(); i++)
	{
		if ((*i)->flockID == this->flockID)
		{
			position += (*i)->position;
			sz++;
		}
	}
	position/=sz;
	return position;
}

Vector FlockEntity::getFlockHeading()
{
	Vector v;
	int sz = 0;
	for (Flock::iterator i = flocks.begin(); i != flocks.end(); i++)
	{
		if ((*i)->flockID == this->flockID)
		{
			v += (*i)->vel;
			sz++;
		}
	}
	v/=sz;
	return v;
}

FlockEntity *FlockEntity::getNearestFlockEntity()
{
	FlockEntity *e = 0;
	float smallDist = -1;
	float dist = 0;
	Vector distVec;
	for (Flock::iterator i = flocks.begin(); i != flocks.end(); i++)
	{
		if ((*i) != this)
		{
			distVec = ((*i)->position - position);
			dist = distVec.getSquaredLength2D();
			if (dist < smallDist || smallDist == -1)
			{
				smallDist = dist;
				e = (*i);
			}
		}
	}
	return e;
}

Vector FlockEntity::averageVectors(const VectorSet &vectors, int maxNum)
{
	Vector avg;
	int c= 0;
	for (VectorSet::const_iterator i = vectors.begin(); i != vectors.end(); i++)
	{
		if (maxNum != 0 && c >= maxNum)
			break;
		avg.x += (*i).x;
		avg.y += (*i).y;
		c++;
		//avg.z += vectors[i].z;
	}
	//int sz = vectors.size();

	if (c != 0)
	{
		avg.x /= float(c);
		avg.y /= float(c);
	}
	//avg.z /= vectors.size();
	return avg;
}

void FlockEntity::getFlockInRange(int radius, FlockPiece *f)
{
	//f->clear();
	FlockEntity *ff=0;
	Vector dist;
	int fp = 0;
	for (Flock::iterator i = flocks.begin(); i != flocks.end(); i++)
	{
		if (fp >= MAX_FLOCKPIECE) break;

		ff = (*i);
		if (ff->flockType == this->flockType && ff->flockID == this->flockID)
		{			
			dist = this->position - ff->position;
			if (dist.isLength2DIn(radius))
			{
				f->e[fp] = ff;
				//f->push_front(ff);
				fp++;
			}
		}
	}
}
