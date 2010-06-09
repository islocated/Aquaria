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
#include "Base.h"

namespace MathFunctions
{
	static void calculateAngleBetweenVectorsInDegrees(const Vector &vector1, const Vector &vector2, float &solutionAngle)
	{
		Vector dist = vector1 - vector2;

		// 0 is down
		// 90 is left
		// 180 is up
		// 270 is right
		// 360 is down 
		solutionAngle = atan(dist.y/fabs(dist.x));
		solutionAngle  = (solutionAngle /PI)*180;
		if (dist.x < 0)
			solutionAngle = 180-solutionAngle;
		solutionAngle += 90;
	}
	static float toRadians(float rot)
	{
		return PI-(rot*PI)/180.0f;
	}

	static float getAngleToVector(const Vector &addVec, float offsetAngle = 0)
	{
		float angle=0;
		MathFunctions::calculateAngleBetweenVectorsInDegrees(Vector(0,0,0), addVec, angle);
		angle = 180-(360-angle);
		angle += offsetAngle;
		return angle;
	}

	static void calculateAngleBetweenVectorsInRadians(Vector vector1, Vector vector2, float &solutionAngle)
	{
		Vector dist = vector1 - vector2;		
		
		solutionAngle = atanf(dist.y/fabs(dist.x));
		
		if (dist.x < 0)
			solutionAngle = PI - solutionAngle;
		/*
		solutionAngle = -solutionAngle;
		solutionAngle += PI/2;
		*/
		
		/*
		if (dist.x<0)
			solutionAngle = PI - solutionAngle;
		*/
	
	
		/*
		vector1.normalize2D();
		vector2.normalize2D();

		solutionAngle = cos((vector1.x*vector2.x + vector2.y*vector2.y));
		*/
		

		 //solutionAngle = acos(vector1.dot(vector2) / (sqrt(vector1.dot(vector1) * vector2.dot(vector2))));
		//solutionAngle = acos(vector1.dot(vector2) / (vector1.getLength2D() * vector2.getLength2D()));
		/*
		solutionAngle  = (solutionAngle /PI)*180;
		if (dist.x < 0)
			solutionAngle = 180-solutionAngle;
		*/
	}

};

