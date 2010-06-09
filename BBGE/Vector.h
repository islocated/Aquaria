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
#pragma once

#include <math.h>
#include <vector> 
#include "Event.h"

#ifdef BBGE_BUILD_DIRECTX
	#include <d3dx9.h>
#endif
typedef float scalar_t;

class Vector
{
public:
     scalar_t x;
     scalar_t y;
     scalar_t z;    // x,y,z coordinates

     Vector(scalar_t a = 0, scalar_t b = 0, scalar_t c = 0) : x(a), y(b), z(c) {}
     Vector(const Vector &vec) : x(vec.x), y(vec.y), z(vec.z) {}


     float *getv()
	 {
		 v[0] = x; v[1] = y; v[2] = z;
		 return v;
	 }

	 float *getv4(float param)
	 {
		 v4[0] = x; v4[1] = y; v4[2] = z; v4[3] = param;
		 return v4;
	 }

	 float v[3];
	 float v4[4];
		 
	 // vector assignment
     const Vector &operator=(const Vector &vec)
     {
          x = vec.x;
          y = vec.y;
          z = vec.z;

          return *this;
     }

     // vecector equality
     const bool operator==(const Vector &vec) const
     {
          return ((x == vec.x) && (y == vec.y) && (z == vec.z));
     }

     // vecector inequality
     const bool operator!=(const Vector &vec) const
     {
          return !(*this == vec);
     }

     // vector add
     const Vector operator+(const Vector &vec) const
     {
          return Vector(x + vec.x, y + vec.y, z + vec.z);
     }

     // vector add (opposite of negation)
     const Vector operator+() const
     {    
          return Vector(*this);
     }

     // vector increment
     const Vector& operator+=(const Vector& vec)
     {    x += vec.x;
          y += vec.y;
          z += vec.z;
          return *this;
     }

     // vector subtraction
     const Vector operator-(const Vector& vec) const
     {    
          return Vector(x - vec.x, y - vec.y, z - vec.z);
     }
     
     // vector negation
     const Vector operator-() const
     {    
          return Vector(-x, -y, -z);
     }
	 bool isZero()
	 {
		 return x == 0 && y == 0 && z == 0;
	 }

     // vector decrement
     const Vector &operator-=(const Vector& vec)
     {
          x -= vec.x;
          y -= vec.y;
          z -= vec.z;

          return *this;
     }

     // scalar self-multiply
     const Vector &operator*=(const scalar_t &s)
     {
          x *= s;
          y *= s;
          z *= s;
          
          return *this;
     }

     // scalar self-divecide
     const Vector &operator/=(const scalar_t &s)
     {
          const float recip = 1/s; // for speed, one divecision

          x *= recip;
          y *= recip;
          z *= recip;

          return *this;
     }

	 // vector self-divide
     const Vector &operator/=(const Vector &v)
	 {
          x /= v.x;
          y /= v.y;
          z /= v.z;

          return *this;
     }

     const Vector &operator*=(const Vector &v)
	 {
          x *= v.x;
          y *= v.y;
          z *= v.z;

          return *this;
     }


     // post multiply by scalar
     const Vector operator*(const scalar_t &s) const
     {
          return Vector(x*s, y*s, z*s);
     }

	 // post multiply by Vector
     const Vector operator*(const Vector &v) const
     {
          return Vector(x*v.x, y*v.y, z*v.z);
     }

     // pre multiply by scalar
     friend inline const Vector operator*(const scalar_t &s, const Vector &vec)
     {
          return vec*s;
     }

/*   friend inline const Vector operator*(const Vector &vec, const scalar_t &s)
     {
          return Vector(vec.x*s, vec.y*s, vec.z*s);
     }

*/   // divecide by scalar
     const Vector operator/(scalar_t s) const
     {
          s = 1/s;

          return Vector(s*x, s*y, s*z);
     }


     // cross product
     const Vector CrossProduct(const Vector &vec) const
     {
          return Vector(y*vec.z - z*vec.y, z*vec.x - x*vec.z, x*vec.y - y*vec.x);
     }

	 Vector getPerpendicularLeft()
	 {
		 return Vector(-y, x);
	 }

 	 Vector getPerpendicularRight()
	 {
		 return Vector(y, -x);
	 }

     // cross product
     const Vector operator^(const Vector &vec) const
     {
          return Vector(y*vec.z - z*vec.y, z*vec.x - x*vec.z, x*vec.y - y*vec.x);
     }

     // dot product
     const scalar_t dot(const Vector &vec) const
     {
          return x*vec.x + y*vec.y + z*vec.z;
     }

	 const scalar_t dot2D(const Vector &vec) const
	 {
		 return x*vec.x + y*vec.y;
	 }

     // dot product
     const scalar_t operator%(const Vector &vec) const
     {
          return x*vec.x + y*vec.x + z*vec.z;
     }


     // length of vector
     const scalar_t getLength3D() const;
     const scalar_t getLength2D() const;

     // return the unit vector
	 const Vector unitVector3D() const;

     // normalize this vector
     void normalize3D();
	 void normalize2D();

     const scalar_t operator!() const
     {
          return sqrtf(x*x + y*y + z*z);
     }

	 /*
     // return vector with specified length
     const Vector operator | (const scalar_t length) const
     {
          return *this * (length / !(*this));
     }

     // set length of vector equal to length
     const Vector& operator |= (const float length)
     {
          (*this).setLength2D(length);
		  return *this;
     }
	 */

	 void setLength3D(const float l);
	 void setLength2D(const float l);

     // return angle between two vectors
     const float inline Angle(const Vector& normal) const
     {
          return acosf(*this % normal);
     }

	 /*
	 const scalar_t inline getCheatLength3D() const;
	 const scalar_t inline cheatLen2D() const;
	 */
	 const bool isLength2DIn(float radius) const;

     // reflect this vector off surface with normal vector
	 /*
     const Vector inline Reflection(const Vector& normal) const
     {    
          const Vector vec(*this | 1);     // normalize this vector
          return (vec - normal * 2.0 * (vec % normal)) * !*this;
     }
	 */

	 /*
	 const scalar_t inline cheatLen() const
	 {
			return (x*x + y*y + z*z);
	 }
	 const scalar_t inline cheatLen2D() const
	 {
		 return (x*x + y*y);
	 }
	 */

	 const void setZero();
	 const float getSquaredLength2D() const;
	 const bool isZero() const;

	 const bool isNan() const;

	 void capLength2D(const float l);
	 void capRotZ360();

#ifdef BBGE_BUILD_DIRECTX
	 const D3DCOLOR getD3DColor(float alpha)
	 {
		 return D3DCOLOR_RGBA(int(x*255), int(y*255), int(z*255), int(alpha*255));
	 }
#endif
	 void rotate2DRad(float rad);
	 void rotate2D360(int angle);
};

inline
const float Vector::getSquaredLength2D() const
{
	return (x*x) + (y*y);
}

class VectorPathNode
{
public:
	VectorPathNode() { percent = 0; }

	Vector value;
	float percent;
};

class VectorPath
{
public:
	void flip();
	void clear();
	void addPathNode(Vector v, float p);
	Vector getValue(float percent);
	int getNumPathNodes() { return pathNodes.size(); }
	void resizePathNodes(int sz) { pathNodes.resize(sz); }
	VectorPathNode *getPathNode(int i) { if (i<getNumPathNodes() && i >= 0) return &pathNodes[i]; return 0; }
	void cut(int n);
	void splice(const VectorPath &path, int sz);
	void prepend(const VectorPath &path);
	void append(const VectorPath &path);
	void removeNode(int i);
	void calculatePercentages();
	float getLength();
	void realPercentageCalc();
	void removeNodes(int startInclusive, int endInclusive);
	float getSubSectionLength(int startIncl, int endIncl);
	void subdivide();
protected:
	std::vector <VectorPathNode> pathNodes;
};


class InterpolatedVector : public Vector
{
public:
    InterpolatedVector(scalar_t a = 0, scalar_t b = 0, scalar_t c = 0) : Vector(a,b,c), interpolating(false) 
	{
		pathTimeMultiplier = 1;
		timeSpeedEase = 0;
		timeSpeedMultiplier = 1;
		currentPathNode = 0;
		speedPath = false;
		pathSpeed = 1;
		pathTime =0;
		pathTimer = 0;
		pingPong = false;
		trigger = 0;
		triggerFlag = false;
		pendingInterpolation = false;
		followingPath = false;
		loopType = 0;
		fakeTimePassed = 0;
		ease = false;
		timePassed = timePeriod = 0;
	}
    InterpolatedVector(const Vector &vec) : Vector(vec), interpolating(false) 
	{
		pathTimeMultiplier = 1;
		timeSpeedEase = 0;
		timeSpeedMultiplier = 1;
		currentPathNode = 0;		
		speedPath = false;
		pathSpeed = 1;
		pathTimer = 0;
		pathTime = 0;
		pingPong = false;
		trigger = 0;
		triggerFlag = false;
		pendingInterpolation = false;
		followingPath = false;
		loopType = 0;
		fakeTimePassed = 0;
		ease = false;
		timePassed = timePeriod = 0;
	}

	void setInterpolationTrigger(InterpolatedVector *trigger, bool triggerFlag);
	enum InterpolateToFlag { NONE=0, IS_LOOPING };
	float interpolateTo (Vector vec, float timePeriod, int loopType = 0, bool pingPong = false, bool ease = false, InterpolateToFlag flag = NONE);
	void update(float dt);

	inline bool isInterpolating()
	{
		return interpolating;
	}

	void startPath(float time, float ease=0);
	void startSpeedPath(float speed);
	void stopPath();
	void resumePath();

	void updatePath(float dt);

	void stop();

	InterpolatedVector *trigger;
	bool triggerFlag;
	bool pendingInterpolation;


	EventPtr endOfInterpolationEvent;
	EventPtr startOfInterpolationEvent;
	EventPtr endOfPathEvent;

	VectorPath path;

	bool interpolating;
	int loopType;
	bool pingPong;

	float pathTimer, pathTime;
	float timePassed, timePeriod;
	Vector target;
	Vector from;

	float getPercentDone();

	bool isFollowingPath()
	{
		return followingPath;
	}

	// for faking a single value
	float getValue()
	{
		return x;
	}

	float pathSpeed;
	bool speedPath;
	int currentPathNode;
	float pathTimeMultiplier;
protected:
	
	float timeSpeedMultiplier, timeSpeedEase;
	bool ease;
	float fakeTimePassed;
	bool followingPath;
};

Vector getRotatedVector(const Vector &vec, float rot);

Vector lerp(const Vector &v1, const Vector &v2, float dt, int lerpType);
