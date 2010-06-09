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
#include "Core.h"

RenderObjectLayer::RenderObjectLayer()
{	
	followCamera = NO_FOLLOW_CAMERA;
	visible = true;
	renderObjects.clear();
	rlType = RLT_DYNAMIC;
	setSize(64);
	freeIdx = 0;
	startPass = endPass = 0;
	currentPass = 0;
	followCameraLock = FCL_NONE;
	currentSize = 0;
	cull = true;
	update = true;

	mode = Core::MODE_2D;

	rlType = RLT_DYNAMIC;
	color = Vector(1,1,1);
	
	quickQuad = false;
	fastCull = false;
	fastCullDist = 1024;
}

void RenderObjectLayer::setCull(bool v)
{
	this->cull = v;
}

RenderObject *RenderObjectLayer::getFirst()
{
	switch(rlType)
	{
	case RLT_DYNAMIC:
		if (renderObjectList.empty()) return 0;
		dynamic_iter = renderObjectList.begin();
		return *dynamic_iter;
	break;
	case RLT_MAP:
		if (renderObjectMap.empty()) return 0;
		map_iter = renderObjectMap.begin();
		return (*map_iter).second;
	break;
	case RLT_FIXED:
	{
		/*
		if (renderObjects.empty()) return 0;
		fixed_iter = renderObjects.begin();
		while ((*fixed_iter)==0 && fixed_iter != renderObjects.end())
		{
			fixed_iter++;
		}
		if (fixed_iter != renderObjects.end())
			return *fixed_iter;
		*/
		int sz = renderObjects.size();
		fixed_iter = 0;
		for (; fixed_iter < currentSize && fixed_iter < sz; fixed_iter++)
		{
			if (renderObjects[fixed_iter] != 0)
			{
				return renderObjects[fixed_iter];
			}
		}
		/*
		while (renderObjects[fixed_iter]==0 && fixed_iter < sz)
		{
			fixed_iter++;
		}
		if (fixed_iter >= sz)						return 0;
		return renderObjects[fixed_iter];
		*/
		return 0;
	}
	break;
	}
	return 0;
}

RenderObject *RenderObjectLayer::getNext()
{
	switch(rlType)
	{
	case RLT_DYNAMIC:
		if (dynamic_iter == renderObjectList.end()) return 0;
		dynamic_iter++;
		if (dynamic_iter == renderObjectList.end()) return 0;
		return *dynamic_iter;
	break;
	case RLT_MAP:
		if (map_iter == renderObjectMap.end()) return 0;
		map_iter++;
		if (map_iter == renderObjectMap.end()) return 0;
		return (*map_iter).second;
	break;
	case RLT_FIXED:
	{
		int sz = renderObjects.size();
		fixed_iter++;
		if (fixed_iter < currentSize && fixed_iter < sz)
		{
			if (renderObjects[fixed_iter]==0)
			{
				for (; fixed_iter < currentSize && fixed_iter < sz; fixed_iter++)
				{
					if (renderObjects[fixed_iter] != 0)
					{
						return renderObjects[fixed_iter];
					}
				}
			}
			else
			{
				return renderObjects[fixed_iter];
			}
		}
		return 0;
		/*
		while (renderObjects[fixed_iter]==0 && fixed_iter < sz && )
		{
			fixed_iter++;
		}
		if (fixed_iter >= sz)						return 0;
		return renderObjects[fixed_iter];
		*/
		
		/*
		if (fixed_iter == renderObjects.end())		return 0;
		fixed_iter++;
		if (fixed_iter == renderObjects.end())		return 0;
		return *fixed_iter;
		*/
	}
	break;
	}
	return 0;
}

void RenderObjectLayer::setType(RLType type)
{
	renderObjects.clear();	
	rlType = type;
}

void RenderObjectLayer::setSize(int sz)
{
	switch(rlType)
	{
	case RLT_FIXED:
	{
		debugLog("setting fixed size");
		int oldSz = renderObjects.size();
		renderObjects.resize(sz);
		for (int i = oldSz; i < sz; i++)
		{
			renderObjects[i] = 0;
		}
	}
	break;
	case RLT_MAP:
	break;
	case RLT_DYNAMIC:
		//debugLog("Cannot set RenderObjectLayer size when RLT_DYNAMIC");
	break;
	}
}

bool sortRenderObjectsByDepth(RenderObject *r1, RenderObject *r2)
{
  return r1->getSortDepth() < r2->getSortDepth();
}

void RenderObjectLayer::sort()
{
	switch(rlType)
	{
	case RLT_FIXED:
	{
		for (int i = renderObjects.size()-1; i >= 0; i--)
		{
			bool flipped = false;
			if (!renderObjects[i]) continue;
			for (int j = 0; j < i; j++)
			{
				if (!renderObjects[j]) continue;
				if (!renderObjects[j+1]) continue;
				//position.z 
				//position.z
				//!renderObjects[j]->parent && !renderObjects[j+1]->parent && 
				if (renderObjects[j]->getSortDepth() > renderObjects[j+1]->getSortDepth())
				{
					RenderObject *temp;
					temp = renderObjects[j];
					int temp2 = renderObjects[j]->getIdx();
					renderObjects[j] = renderObjects[j+1];
					renderObjects[j+1] = temp;

					renderObjects[j]->setIdx(j);
					renderObjects[j+1]->setIdx(j+1);

					flipped = true;
				}
			}
			if (!flipped) break;
		}
	}
	break;
	case RLT_DYNAMIC:
		renderObjectList.sort(sortRenderObjectsByDepth);
	break;
	}
}

void RenderObjectLayer::findNextFreeIdx()
{
	int c = 0;
	int sz = renderObjects.size();
	//freeIdx++;
	while (renderObjects[freeIdx] != 0)
	{
		freeIdx ++;
		if (freeIdx >= sz)
			freeIdx = 0;
		c++;
		if (c > sz)
		{
			std::ostringstream os;
			os << "exceeded max renderobject count max[" << sz << "]";
			debugLog(os.str());
			return;
		}
	}
	if (freeIdx > currentSize)
	{
		currentSize = freeIdx+1;
		if (currentSize > sz)
		{
			currentSize = sz;
		}
		/*
		std::ostringstream os;
		os << "CurrentSize: " << currentSize;
		debugLog(os.str());
		*/
	}
	/*
	std::ostringstream os;
	os << "layer: " << index << " freeIdx: " << freeIdx;
	debugLog(os.str());
	*/
}

void RenderObjectLayer::add(RenderObject* r)
{
	switch(rlType)
	{
	case RLT_FIXED:
	{
		renderObjects[freeIdx] = r;
		r->setIdx(freeIdx);
		
		findNextFreeIdx();

		//renderObjects[freeIdx] = r;
	}
	break;
	case RLT_DYNAMIC:
		renderObjectList.push_back(r);
	break;
	case RLT_MAP:
		renderObjectMap[intptr_t(r)] = r;
	break;
	}
}

void RenderObjectLayer::remove(RenderObject* r)
{
	switch(rlType)
	{
	case RLT_FIXED:
	{
		if (r->getIdx() <= -1 || r->getIdx() >= renderObjects.size())
			errorLog("trying to remove renderobject with invalid idx");
		renderObjects[r->getIdx()] = 0;
		if (r->getIdx() < freeIdx)
			freeIdx = r->getIdx();

		int c = currentSize; 
		while (renderObjects[c] == 0 && c >= 0)
		{			
			c--;
		}
		currentSize = c+1;
		if (currentSize > renderObjects.size())
			currentSize = renderObjects.size();

		/*
		std::ostringstream os;
		os << "CurrentSize: " << currentSize;
		debugLog(os.str());
		*/
	}
	break;
	case RLT_DYNAMIC:
		renderObjectList.remove(r);
	break;
	case RLT_MAP:
		renderObjectMap[intptr_t(r)] = 0;
	break;
	}
}

void RenderObjectLayer::moveToFront(RenderObject *r)
{
	switch(rlType)
	{
	case RLT_FIXED:
	{
		int idx = r->getIdx();
		int last = renderObjects.size()-1;
		int i = 0;
		for (i = renderObjects.size()-1; i >=0; i--)
		{
			if (renderObjects[i] == 0)
				last = i;
			else
				break; 
		}
		if (idx == last) return;
		for (i = idx; i < last; i++)
		{
			renderObjects[i] = renderObjects[i+1];
			if (renderObjects[i])
				renderObjects[i]->setIdx(i);
		}
		renderObjects[last] = r;
		r->setIdx(last);

		findNextFreeIdx();
	}
	break;
	case RLT_DYNAMIC:
	{
		renderObjectList.remove(r);
		renderObjectList.push_back(r);
	}
	break;
	}
}

void RenderObjectLayer::moveToBack(RenderObject *r)
{
	switch(rlType)
	{
	case RLT_FIXED:
	{
		int idx = r->getIdx();
		if (idx == 0) return;
		for (int i = idx; i >= 1; i--)
		{
			renderObjects[i] = renderObjects[i-1];
			if (renderObjects[i])
				renderObjects[i]->setIdx(i);
		}
		renderObjects[0] = r;
		r->setIdx(0);

		findNextFreeIdx();
	}
	break;
	case RLT_DYNAMIC:
	{
        renderObjectList.remove(r);
		renderObjectList.push_front(r);
	}
	break;
	}
}

bool RenderObjectLayer::empty()
{
	switch(rlType)
	{
	case RLT_FIXED:
		return renderObjects.empty();
	case RLT_DYNAMIC:
		return renderObjectList.empty();
	}
	return false;
}
