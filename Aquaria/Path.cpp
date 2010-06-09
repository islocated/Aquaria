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
#include "Game.h"
extern "C"
{
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}

#include "Avatar.h"

Path::Path()
{
	localWarpType = LOCALWARP_NONE;
	effectOn = true;
	time = 0;
	naijaIn = false;
	amount = 0;
	catchActions = false;
	pathShape = PATHSHAPE_RECT;
	toFlip = -1;
	replayVox = 0;
	naijaHome = false;
	addEmitter = false;
	emitter = 0;
	active = true;
	currentMod = 1.0;
	songFunc = songNoteFunc = songNoteDoneFunc = true;
	pathType = PATH_NONE;
	neverSpawned = true;
	spawnedEntity = 0;
	L = 0;
	updateFunction = activateFunction = false;
	cursorActivation = false;
	rect.setWidth(64);
	rect.setHeight(64);

	animOffset = 0;

	spawnEnemyNumber = 0;
	spawnEnemyDistance = 0;
	warpType = 0;
	/*
	rect.x1 = -10;
	rect.x2 = 20;
	rect.y1 = -256;
	rect.y2 = 256;
	*/
}

void Path::clampPosition(Vector *pos, int radius)
{
	if (pathShape == PATHSHAPE_CIRCLE)
	{
		Vector diff = (*pos) - nodes[0].position;
		float rad = rect.getWidth()*0.5;
		diff.capLength2D(rad-2-radius);
		*pos = nodes[0].position + diff;
	}
	else if (pathShape == PATHSHAPE_RECT)
	{
		Vector diff = (*pos) - nodes[0].position;
		if (diff.x < rect.x1)
			diff.x = rect.x1;

		if (diff.x > rect.x2)
			diff.x = rect.x2;

		if (diff.y < rect.y1)
			diff.y = rect.y1;

		if (diff.y > rect.y2)
			diff.y = rect.y2;

		*pos = nodes[0].position + diff;
	}
}

PathNode *Path::getPathNode(int idx)
{
	if (idx < 0 || idx >= nodes.size()) return 0;
	return &nodes[idx];
}

void Path::reverseNodes()
{
	std::vector<PathNode> copy = nodes;
	nodes.clear();
	for (int i = copy.size()-1; i >= 0; i--)
	{
		nodes.push_back(copy[i]);
	}
}

void Path::setActive(bool v)
{
	active = v;
}

bool Path::isCoordinateInside(const Vector &pos, int radius)
{
	if (nodes.empty()) return false;
	if (pathShape == PATHSHAPE_CIRCLE)
	{
		Vector diff = pos - nodes[0].position;
		return diff.isLength2DIn(this->rect.getWidth()*0.5 - radius);
	}
	else
	{
		Vector rel = pos - nodes[0].position;
		return rect.isCoordinateInside(rel);
	}
	return false;
}

Vector Path::getEnterNormal()
{
	switch(warpType)
	{
	case CHAR_RIGHT:
		return Vector(-1, 0);
	break;
	case CHAR_LEFT:
		return Vector(1, 0);
	break;
	case CHAR_UP:
		return Vector(0, 1);
	break;
	case CHAR_DOWN:
		return Vector(0, -1);
	break;
	}
	return Vector(1,1);
}

Vector Path::getEnterPosition(int outAmount)
{
	if (nodes.empty()) return Vector(0,0,0);
	Vector enterPos = nodes[0].position;
	switch(warpType)
	{
	case CHAR_RIGHT:
		enterPos.x = getLeft()-outAmount;
	break;
	case CHAR_LEFT:
		enterPos.x = getRight()+outAmount;
	break;
	case CHAR_UP:
		enterPos.y = getDown()+outAmount;
	break;
	case CHAR_DOWN:
		enterPos.y = getUp()-outAmount;
	break;
	}
	return enterPos;
}

Vector Path::getBackPos(const Vector &position)
{
	Vector ret = position;
	switch(warpType)
	{
	case CHAR_RIGHT:
		ret.x = getLeft()-1;
	break;
	case CHAR_LEFT:
		ret.x = getRight()+1;
	break;
	case CHAR_UP:
		ret.y = getDown()+1;
	break;
	case CHAR_DOWN:
		ret.y = getUp()-1;
	break;
	}
	return ret;
}

int Path::getLeft()
{
	return nodes[0].position.x + rect.x1;
}

int Path::getRight()
{
	return nodes[0].position.x + rect.x2;
}

int Path::getUp()
{
	return nodes[0].position.y + rect.y1;
}

int Path::getDown()
{
	return nodes[0].position.y + rect.y2;
}

void Path::destroy()
{
	if (emitter)
	{
		emitter->safeKill();
		emitter = 0;
	}
	if (L)
	{
		lua_close(L);
		L = 0;
	}
}

Path::~Path()
{
}

bool Path::hasScript()
{
	return L != 0;
}

void Path::song(SongType songType)
{
	if (hasScript() && songFunc)
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "song");
		luaPushPointer(L, this);
		lua_pushnumber(L, int(songType));
		int fail = lua_pcall(L, 2, 0, 0);
		if (fail)
		{
			songFunc = false;
			debugLog("Path [" + name + "] " + lua_tostring(L, -1));
		}
	}
}

void Path::songNote(int note)
{
	if (hasScript() && songNoteFunc)
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "songNote");
		luaPushPointer(L, this);
		lua_pushnumber(L, note);
		int fail = lua_pcall(L, 2, 0, 0);
		if (fail)
		{
			songNoteFunc = false;
			debugLog("Path [" + name + "] " + lua_tostring(L, -1) + " songNote");
		}
	}
}

void Path::songNoteDone(int note, float len)
{
	if (hasScript() && songNoteDoneFunc)
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "songNoteDone");
		luaPushPointer(L, this);
		lua_pushnumber(L, note);
		lua_pushnumber(L, len);
		int fail = lua_pcall(L, 3, 0, 0);
		if (fail)
		{
			songNoteDoneFunc = false;
			debugLog("Path [" + name + "] " + lua_tostring(L, -1) + " songNoteDone");
		}
	}
}

void Path::parseWarpNodeData(const std::string &dataString)
{
	std::string label;
	std::istringstream is(dataString);
	std::string flip;
	is >> label >> warpMap >> warpNode >> flip;
	pathType = PATH_WARP;
	toFlip = -1;

	stringToLower(flip);

	if (flip == "l")
		toFlip = 0;
	if (flip == "r")
		toFlip = 1;
}

void Path::refreshScript()
{

	amount = 0;

	// HACK: clean up
	/*+ dsq->game->sceneName + "_"*/
	L = 0;
	warpMap = warpNode = "";
	toFlip = -1;

	stringToLower(name);

	std::string scr;
	if (dsq->mod.isActive())
		scr = dsq->mod.getPath() + "scripts/node_" + name + ".lua";
	else
		scr = "scripts/maps/node_" + name + ".lua";
	if (exists(scr))
	{
		dsq->scriptInterface.initLuaVM(&L);
		std::string fname(core->adjustFilenameCase(scr));
		int fail = (luaL_loadfile(L, fname.c_str()));
		if (fail) debugLog(lua_tostring(L, -1));
		fail = lua_pcall(L, 0, 0, 0);
		if (fail)	debugLog(lua_tostring(L, -1));
		updateFunction = activateFunction = true;
	}
	std::string topLabel;
	std::istringstream tis;
	tis >> topLabel;
	if (name.find("seting ") != std::string::npos)
	{
		pathType = PATH_SETING;
		std::istringstream is(name);
		std::string label;
		is >> label >> content >> amount;
	}
	else if (name.find("setent ") != std::string::npos)
	{
		pathType = PATH_SETENT;
		std::istringstream is(name);
		std::string label;
		is >> label >> content >> amount;
	}
	else if (name.find("current") != std::string::npos)
	{
		pathType = PATH_CURRENT;
		std::istringstream is(name);
		std::string label;
		is >> label;
		is >> currentMod;
		is >> amount;
		if (amount == 0)
		{
			amount = 0.5;
		}
		if (amount > 1) amount = 1;
		if (amount < 0) amount = 0;
	}
	else if (name.find("gem ") != std::string::npos)
	{
		pathType = PATH_GEM;
		std::string label;
		std::istringstream is(name);
		is >> label >> gem;
	}
	else if (name == "waterbubble")
	{
		pathType = PATH_WATERBUBBLE;
	}
	else if (name == "cook")
	{
		pathType = PATH_COOK;
	}
	else if (name.find("zoom ") != std::string::npos)
	{
		pathType = PATH_ZOOM;
		std::istringstream is(name);
		std::string label;
		is >> label >> amount >> time;
		if (time == 0)
		{
			time = 1;
		}
	}
	else if (name.find("radarhide") != std::string::npos)
	{
		pathType = PATH_RADARHIDE;
	}
	else if (name.find("bgsfxloop") != std::string::npos)
	{
		pathType = PATH_BGSFXLOOP;
		std::istringstream is(name);
		std::string label;
		is >> label >> content;
		core->sound->loadLocalSound(content);
	}

	else if (name.find("savepoint") != std::string::npos)
	{
		pathType = PATH_SAVEPOINT;
	}
	else if (topLabel == "li")
	{
		pathType = PATH_LI;

		/*
		pathType = PATH_LI;
		std::istringstream is(name);
		std::string label;
		is >> label;
		*/
		//is >> expression;
	}
	else if (name.find("steam") != std::string::npos)
	{
		pathType = PATH_STEAM;
		std::istringstream is(name);
		std::string label;
		is >> label;

		float v = 0; 
		is >> v;
		if (v != 0)
			currentMod = v;
	}
	else if (name.find("warpnode ")!=std::string::npos)
	{
		parseWarpNodeData(name);
	}
	else if (name.find("spiritportal") != std::string::npos)
	{
		std::string label;
		std::istringstream is(name);
		is >> label >> warpMap >> warpNode;
		pathType = PATH_SPIRITPORTAL;
	}
	else if (name.find("warplocalnode ")!=std::string::npos)
	{
		std::string label;
		std::string type;
		std::istringstream is(name);
		is >> label >> warpNode >> type;
		if (type == "in")
			localWarpType = LOCALWARP_IN;
		else if (type == "out")
			localWarpType = LOCALWARP_OUT;
		pathType = PATH_WARP;
	}
	else if (name.find("vox ")!=std::string::npos || name.find("voice ")!=std::string::npos)
	{
		std::string label, re;
		std::istringstream is(name);
		is >> label >> vox >> re;
		if (!re.empty())
			replayVox = true;
	}
	else if (name.find("warp ")!=std::string::npos)
	{
		std::string label;
		std::istringstream is(name);
		is >> label >> warpMap >> warpType;

		if (warpMap.find("vedha")!=std::string::npos)
		{
			naijaHome = true;
		}
		//debugLog(label + " " + warpMap + " " + warpType);
		/*
		std::string parse = name;
		int pos = 0;

		pos = parse.find('_');
		parse = parse.substr(pos+1, parse.getLength2D());

		pos = parse.find('_');
		warpMap = parse.substr(0, pos-1);
		parse = parse.substr(pos+1, parse.getLength2D());

		pos = parse.find('_');
		std::string
		*/
		pathType = PATH_WARP;

	}
	else if (name.find("se ")!=std::string::npos)
	{
		std::string label;
		std::istringstream is(name);
		spawnEnemyNumber = 0;
		spawnEnemyDistance = 0;
		is >> label >> spawnEnemyName >> spawnEnemyDistance >> spawnEnemyNumber;
		neverSpawned = true;
		/*
		if (!spawnedEntity && !nodes.empty())
		{
			spawnedEntity = dsq->game->createEntity(spawnEnemyName, 0, nodes[0].position, 0, false, "");
		}
		*/
	}
	else if (name.find("pe ")!=std::string::npos)
	{
		std::string label, particleEffect;
		std::istringstream is(name);
		is >> label >> particleEffect;
		//core->removeRenderObject(&emitter, Core::DO_NOT_DESTROY_RENDER_OBJECT);
		//core->getTopStateData()->addRenderObject(&emitter, LR_PARTICLES);

		if (emitter)
		{
			emitter->safeKill();
			emitter = 0;
		}
		emitter = new ParticleEffect;
		emitter->load(particleEffect);
		emitter->start();
		if (!nodes.empty())
		{
			emitter->position = nodes[0].position;
		}
		addEmitter = true;
	}
}

void Path::init()
{
	if (L)
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "init");
		luaPushPointer(L, this);
		int fail = lua_pcall(L, 1, 0, 0);
		if (fail)
		{
			debugLog(name + " : " + lua_tostring(L, -1) + " init");
		}
	}
}

void Path::update(float dt)
{
	if (!dsq->game->isPaused() && dsq->continuity.getWorldType() == WT_NORMAL)
	{
		if (addEmitter && emitter)
		{
			addEmitter = false;
			dsq->game->addRenderObject(emitter, LR_PARTICLES);
		}

		if (emitter && !nodes.empty())
		{
			emitter->position = nodes[0].position;
		}
		if (L && updateFunction)
		{
			lua_getfield(L, LUA_GLOBALSINDEX, "update");
			luaPushPointer(L, this);
			lua_pushnumber(L, dt);
			int fail = lua_pcall(L, 2, 0, 0);
			if (fail)
			{
				debugLog(name + " : " + lua_tostring(L, -1) + " update");
				updateFunction = false;
			}
		}
		if (emitter)
		{
			if (!nodes.empty())
				emitter->position = nodes[0].position;
			emitter->update(dt);
		}

		if (!nodes.empty() && !spawnedEntity && !spawnEnemyName.empty())
		{
			if (neverSpawned || !(nodes[0].position - dsq->game->avatar->position).isLength2DIn(spawnEnemyDistance))
			{
				neverSpawned = false;
				spawnedEntity = dsq->game->createEntity(spawnEnemyName, 0, nodes[0].position, 0, false, "");
			}
		}
		if (spawnedEntity && spawnedEntity->life < 1.0)
		{
			spawnedEntity = 0;
		}
		if (pathType == PATH_CURRENT && dsq->continuity.getWorldType() == WT_NORMAL)
		{
			animOffset -= currentMod*(dt/830);
			/*
			while (animOffset < -1.0)
				animOffset += 1.0;
			*/
		}
		if (pathType == PATH_GEM && dsq->game->avatar)
		{
			if (isCoordinateInside(dsq->game->avatar->position))
			{
				if (!dsq->continuity.getPathFlag(this))
				{
					dsq->continuity.setPathFlag(this, 1);
					dsq->continuity.pickupGem(gem);
				}
			}
		}
		if (pathType == PATH_ZOOM && dsq->game->avatar)
		{
			if (isCoordinateInside(dsq->game->avatar->position))
			{
				naijaIn = true;
				dsq->game->overrideZoom(amount, time);
			}
			else
			{
				if (naijaIn)
				{
					naijaIn = false;
					dsq->game->overrideZoom(0);
				}
			}
		}

		if (pathType == PATH_STEAM && dsq->continuity.getWorldType() == WT_NORMAL && effectOn)
		{
			animOffset -= 1000*0.00002;


			if (nodes.size() >= 2)
			{
				Vector start = nodes[0].position;
				Vector end = nodes[1].position;
				Vector v = end - start;
				Vector left = v.getPerpendicularLeft();
				Vector right = v.getPerpendicularRight();
				Vector mid = (end-start) + start;
				FOR_ENTITIES(i)
				{
					Entity *e = *i;
					if (e)
					{
						/*
						if (e->getEntityType() == ET_AVATAR && dsq->continuity.form == FORM_SPIRIT)
							continue;
						*/
						if (dsq->game->collideCircleVsLine(e, start, end, rect.getWidth()*0.5))
						{
							if (e->getEntityType() == ET_AVATAR)
							{
								Avatar *a = (Avatar*)e;
								a->stopBurst();
								a->fallOffWall();
								a->vel.capLength2D(200);
								a->vel2.capLength2D(200);

								DamageData d;
								d.damage = 0.1;
								d.damageType = DT_STEAM;
								e->damage(d);
								//a->position = a->lastPosition;
							}
							Vector push;

							push = e->position - dsq->game->lastCollidePosition;

							// old method:
							/*
							int d1 = ((mid + left)-e->position).getSquaredLength2D();
							if (((mid + right)-e->position).getSquaredLength2D() < d1)
							{
								push = right;
							}
							else
							{
								push = left;
							}
							*/

							push.setLength2D(1000*dt);
							if (e->vel2.isLength2DIn(1000) && !e->isNearObstruction(3))
							{
								v.setLength2D(2000*dt);
								e->vel2 += v;
							}
							e->vel2 += push;
							if (dsq->game->collideCircleVsLine(e, start, end, rect.getWidth()*0.25))
							{
								push.setLength2D(100);
								/*
								Vector oldVel = e->vel;
								Vector nvel = v;
								nvel.setLength2D(e->vel);
								*/
								e->vel = 0;
								e->vel += push;
							}
						}
					}
				}
			}
		}
	}
}

bool Path::action(int id, int state)
{
	if (L)
	{
		bool dontRemove = true;
		lua_getfield(L, LUA_GLOBALSINDEX, "action");
		luaPushPointer(L, this);
		lua_pushnumber(L, id);
		lua_pushnumber(L, int(state));
		int fail = lua_pcall(L, 3, 1, 0);
		if (fail)
			debugLog(name + " : " + lua_tostring(L, -1) + " action");
		else
		{
			dontRemove = lua_toboolean(L, -1);
		}
		return dontRemove;
	}
	return true;
}

void Path::activate(Entity *e)
{
	if (L && activateFunction)
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "activate");
		luaPushPointer(L, this);
		luaPushPointer(L, e);
		int fail = lua_pcall(L, 2, 0, 0);
		if (fail)
		{
			debugLog(name + " : " + lua_tostring(L, -1) + " activate");
			activateFunction = false;
		}
	}
}

void Path::removeNode(int idx)
{
	std::vector<PathNode> copy = nodes;
	nodes.clear();
	for (int i = 0; i < copy.size(); i++)
	{
		if (idx != i)
			nodes.push_back(copy[i]);
	}
}

void Path::addNode(int idx)
{
	std::vector<PathNode> copy = nodes;
	nodes.clear();
	bool added = false;
	for (int i = 0; i < copy.size(); i++)
	{
		nodes.push_back(copy[i]);
		if (idx == i)
		{
			added = true;
			PathNode p;
			int j = i + 1;
			if (j < copy.size())
			{
				Vector add = copy[j].position - copy[i].position;
				add/=2;
				p.position = copy[i].position + add;
			}
			else
			{
				p.position = dsq->getGameCursorPosition();
			}
			nodes.push_back(p);
		}
	}
	if (!added)
	{
		PathNode p;
		p.position = dsq->getGameCursorPosition();
		nodes.push_back(p);
	}
}

