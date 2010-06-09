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
#include "ScriptInterface.h"
extern "C"
{
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"
#include "ScriptedEntity.h"
#include "Shot.h"
#include "Entity.h"
#include "Web.h"
#include "GridRender.h"

#include "../BBGE/MathFunctions.h"

#define FRAME_TIME  0.04

ScriptInterface *si = 0;
bool conversationStarted = false;
bool throwLuaErrors = false;

//============================================================================================
// S C R I P T  C O M M A N D S
//============================================================================================

void luaPushPointer(lua_State *L, void *ptr)
{
	// All the scripts do this:
	//    x = getFirstEntity()
	//    while x =~ 0 do x = getNextEntity() end
	// The problem is this is now a pointer ("light user data"), so in
	//  Lua, it's never equal to 0 (or nil!), even if it's NULL.
	// So we push an actual zero when we get a NULL to keep the existing
	//  scripts happy.  --ryan.
	if (ptr != NULL)
		lua_pushlightuserdata(L, ptr);
	else
		lua_pushnumber(L, 0);
}

#define luaf(func)		int l_##func(lua_State *L) {
#define luap(ptr)		luaPushPointer(L, ptr); return 1; }
#define luab(bool)		lua_pushboolean(L, bool); return 1; }

#define luar(func)		lua_register(*L, #func, l_##func);

void luaErrorMsg(lua_State *L, const std::string &msg)
{
	debugLog(msg);

	if (throwLuaErrors)
	{
		lua_pushstring(L, msg.c_str());
		lua_error(L);
	}
}

int l_randRange(lua_State *L)
{
	int n1 = lua_tointeger(L, 1);
	int n2 = lua_tointeger(L, 2);
	int spread = n2-n1;

	int r = rand()%spread;
	r += n1;

	lua_pushnumber(L, r);
	return 1;
}

int l_upgradeHealth(lua_State *L)
{
	dsq->continuity.upgradeHealth();
	lua_pushnumber(L, 0);
	return 1;
}

int l_shakeCamera(lua_State *L)
{
	dsq->shakeCamera(lua_tonumber(L,1), lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_changeForm(lua_State *L)
{
	dsq->game->avatar->changeForm((FormType)lua_tointeger(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_getWaterLevel(lua_State *L)
{
	lua_pushnumber(L, dsq->game->getWaterLevel());
	return 1;
}

int l_setPoison(lua_State *L)
{
	dsq->continuity.setPoison(lua_tonumber(L, 1), lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_cureAllStatus(lua_State *L)
{
	dsq->continuity.cureAllStatus();
	lua_pushnumber(L, 0);
	return 1;
}

int l_setMusicToPlay(lua_State *L)
{
	if (lua_isstring(L, 1))
		dsq->game->setMusicToPlay(lua_tostring(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_setActivePet(lua_State *L)
{
	Entity *e = dsq->game->setActivePet(lua_tonumber(L, 1));

	luaPushPointer(L, e);
	return 1;
}

int l_setWaterLevel(lua_State *L)
{
	dsq->game->waterLevel.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	lua_pushnumber(L, dsq->game->waterLevel.x);
	return 1;
}

int l_getForm(lua_State *L)
{
	lua_pushnumber(L, dsq->continuity.form);
	return 1;
}

int l_isForm(lua_State *L)
{
	FormType form = FormType(lua_tointeger(L, 1));
	bool v = (form == dsq->continuity.form);
	lua_pushboolean(L, v);
	return 1;
}

int l_learnFormUpgrade(lua_State *L)
{
	dsq->continuity.learnFormUpgrade((FormUpgradeType)lua_tointeger(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_hasLi(lua_State *L)
{
	lua_pushboolean(L, dsq->continuity.hasLi());
	return 1;
}

int l_hasFormUpgrade(lua_State *L)
{
	lua_pushboolean(L, dsq->continuity.hasFormUpgrade((FormUpgradeType)lua_tointeger(L, 1)));
	return 1;
}

int l_castSong(lua_State *L)
{
	dsq->continuity.castSong(lua_tonumber(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_isStory(lua_State *L)
{
	lua_pushboolean(L, dsq->continuity.isStory(lua_tonumber(L, 1)));
	return 1;
}

int l_getNoteVector(lua_State *L)
{
	int note = lua_tointeger(L, 1);
	float mag = lua_tonumber(L, 2);
	Vector v = dsq->getNoteVector(note, mag);
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	return 2;
}

int l_getNoteColor(lua_State *L)
{
	int note = lua_tointeger(L, 1);
	Vector v = dsq->getNoteColor(note);

	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	lua_pushnumber(L, v.z);
	return 3;
}

int l_getRandNote(lua_State *L)
{
	int note = lua_tointeger(L, 1);

	lua_pushnumber(L, dsq->getRandNote());
	return 1;
}

int l_getStory(lua_State *L)
{
	lua_pushnumber(L, dsq->continuity.getStory());
	return 1;
}

int l_foundLostMemory(lua_State *L)
{
	int num = 0;
	if (dsq->continuity.getFlag(FLAG_SECRET01)) num++;
	if (dsq->continuity.getFlag(FLAG_SECRET02)) num++;
	if (dsq->continuity.getFlag(FLAG_SECRET03)) num++;

	int sbank = 800+(num-1);
	dsq->game->setControlHint(dsq->continuity.stringBank.get(sbank), 0, 0, 0, 4, "13/face");

	dsq->sound->playSfx("memory-found");

	lua_pushinteger(L, 0);
	return 1;
}

int l_setGameOver(lua_State *L)
{
	bool v = false;
	if (lua_isnumber(L, 0))
		v = lua_tointeger(L, 0);
	else if (lua_isboolean(L, 0))
		v = lua_toboolean(L, 0);

	dsq->game->runGameOverScript = !v;

	lua_pushnumber(L, 0);
	return 1;
}

int l_reloadTextures(lua_State *L)
{
	dsq->precacher.clean();
	dsq->precacher.precacheList("data/precache.txt");
	dsq->reloadResources();

	lua_pushnumber(L, 0);
	return 1;
}

int l_setStory(lua_State *L)
{
	dsq->continuity.setStory(lua_tonumber(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

inline
ScriptedEntity *scriptedEntity(lua_State *L, int s = 1)
{
	ScriptedEntity *se = (ScriptedEntity*)lua_touserdata(L, s);
	if (!se)
		debugLog("ScriptedEntity invalid pointer.");
	return se;
}

inline
CollideEntity *collideEntity(lua_State *L, int s = 1)
{
	CollideEntity *ce = (CollideEntity*)lua_touserdata(L, s);
	if (!ce)
		debugLog("CollideEntity invalid pointer.");
	return ce ;
}

inline
RenderObject *object(lua_State *L, int s=1)
{
	//RenderObject *obj = dynamic_cast<RenderObject*>((RenderObject*)(int(lua_tonumber(L, s))));
	RenderObject *obj = static_cast<RenderObject*>(lua_touserdata(L, s));
	if (!obj)
		debugLog("RenderObject invalid pointer");
	return obj;
}

inline
Beam *beam(lua_State *L, int s = 1)
{
	Beam *b = (Beam*)lua_touserdata(L, s);
	if (!b)
		debugLog("Beam invalid pointer.");
	return b;
}

inline
std::string getString(lua_State *L, int s = 1)
{
	std::string sr;
	if (lua_isstring(L, s))
	{
		sr = lua_tostring(L, s);
	}
	return sr;
}

inline
Shot *getShot(lua_State *L, int s = 1)
{
	Shot *shot = (Shot*)lua_touserdata(L, s);
	return shot;
}

inline
Web *getWeb(lua_State *L, int s = 1)
{
	Web *web = (Web*)lua_touserdata(L, s);
	return web;
}

int l_confirm(lua_State *L)
{
	std::string s1 = getString(L, 1);
	std::string s2 = getString(L, 2);
	bool b = dsq->confirm(s1, s2);
	lua_pushboolean(L, b);
	return 1;
}

int l_createWeb(lua_State *L)
{
	Web *web = new Web();
	dsq->game->addRenderObject(web, LR_PARTICLES);
	luaPushPointer(L, web);
	return 1;
}

// spore has base entity
int l_createSpore(lua_State *L)
{
	Vector pos(lua_tonumber(L, 1), lua_tonumber(L, 2));
	if (Spore::isPositionClear(pos))
	{
		Spore *spore = new Spore(pos);
		dsq->game->addRenderObject(spore, LR_ENTITIES);
		luaPushPointer(L, spore);
	}
	else
		lua_pushnumber(L, 0);
	return 1;
}

int l_web_addPoint(lua_State *L)
{
	Web *w = getWeb(L);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	int r = 0;
	if (w)
	{
		r = w->addPoint(Vector(x,y));
	}
	lua_pushnumber(L, r);
	return 1;
}

int l_web_setPoint(lua_State *L)
{
	Web *w = getWeb(L);
	int pt = lua_tonumber(L, 2);
	float x = lua_tonumber(L, 3);
	float y = lua_tonumber(L, 4);
	if (w)
	{
		w->setPoint(pt, Vector(x, y));
	}
	lua_pushnumber(L, pt);
	return 1;
}

int l_web_getNumPoints(lua_State *L)
{
	Web *web = getWeb(L);
	int num = 0;
	if (web)
	{
		num = web->getNumPoints();
	}
	lua_pushnumber(L, num);
	return 1;
}

int l_web_delete(lua_State *L)
{
	Web *e = getWeb(L);
	if (e)
	{
		float time = lua_tonumber(L, 2);
		if (time == 0)
		{
			e->alpha = 0;
			e->setLife(0);
			e->setDecayRate(1);
		}
		else
		{
			e->fadeAlphaWithLife = true;
			e->setLife(1);
			e->setDecayRate(1.0/time);
		}
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_shot_getPosition(lua_State *L)
{
	float x=0,y=0;
	Shot *shot = getShot(L);
	if (shot)
	{
		x = shot->position.x;
		y = shot->position.y;
	}
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}

int l_shot_setLifeTime(lua_State *L)
{
	Shot *shot = getShot(L);
	if (shot)
	{
		shot->setLifeTime(lua_tonumber(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_shot_setVel(lua_State *L)
{
	Shot *shot = getShot(L);
	float vx = lua_tonumber(L, 2);
	float vy = lua_tonumber(L, 3);
	if (shot)
	{
		shot->velocity = Vector(vx, vy);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_shot_setOut(lua_State *L)
{
	Shot *shot = getShot(L);
	float outness = lua_tonumber(L, 2);
	if (shot && shot->firer)
	{
		Vector adjust = shot->velocity;
		adjust.setLength2D(outness);
		shot->position += adjust;
		/*
		std::ostringstream os;
		os << "out(" << adjust.x << ", " << adjust.y << ")";
		debugLog(os.str());
		*/
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_shot_setAimVector(lua_State *L)
{
	Shot *shot = getShot(L);
	float ax = lua_tonumber(L, 2);
	float ay = lua_tonumber(L, 3);
	if (shot)
	{
		shot->setAimVector(Vector(ax, ay));
	}
	lua_pushnumber(L, 0);
	return 1;
}

// shot, texture, particles
// (kills segs)
int l_shot_setNice(lua_State *L)
{
	/*
	Shot *shot = getShot(L);
	if (shot)
	{
		shot->setTexture(getString(L,2));
		shot->setParticleEffect(getString(L, 3));
		shot->hitParticleEffect = getString(L, 4);
		shot->hitSound = getString(L, 5);
		shot->noSegs();
		int blend = lua_tonumber(L, 6);
		shot->setBlendType(blend);
		shot->scale = Vector(1,1);
	}
	*/
	debugLog("shot_setNice is deprecated");

	lua_pushnumber(L, 0);
	return 1;
}

inline
Ingredient *getIng(lua_State *L, int s = 1)
{
	return (Ingredient*)lua_touserdata(L, 1);
}

inline
bool getBool(lua_State *L, int s = 1)
{
	if (lua_isnumber(L, s))
	{
		return bool(lua_tonumber(L, s));
	}
	else if (lua_islightuserdata(L, s))
	{
		return (lua_touserdata(L, s) != NULL);
	}
	else if (lua_isboolean(L, s))
	{
		return lua_toboolean(L, s);
	}
	return false;
}

inline
Entity *entity(lua_State *L, int s = 1)
{
	Entity *ent = (Entity*)lua_touserdata(L, s);
	if (!ent)
	{
		luaErrorMsg(L, "Entity Invalid Pointer");
	}
	return ent;
}

inline
Vector getVector(lua_State *L, int s = 1)
{
	Vector v(lua_tonumber(L, s), lua_tonumber(L, s+1));
	return v;
}


inline
Bone *bone(lua_State *L, int s = 1)
{
	Bone *b = (Bone*)lua_touserdata(L, s);
	if (!b)
	{
		luaErrorMsg(L, "Bone Invalid Pointer");
	}
	return b;
}

inline
Path *pathFromName(lua_State *L, int slot = 1)
{
	std::string s = lua_tostring(L, slot);
	stringToLowerUserData(s);
	Path *p = dsq->game->getPathByName(s);
	if (!p)
	{
		debugLog("Could not find path [" + s + "]");
	}
	return p;
}

inline
Path *path(lua_State *L, int slot = 1)
{
	Path *p = (Path*)lua_touserdata(L, slot);
	return p;
}

RenderObject *entityToRenderObject(lua_State *L, int i = 1)
{
	Entity *e = entity(L, i);
	return dynamic_cast<RenderObject*>(e);
}

RenderObject *boneToRenderObject(lua_State *L, int i = 1)
{
	Bone *b = bone(L, i);
	return dynamic_cast<RenderObject*>(b);
}

int l_entity_addIgnoreShotDamageType(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->addIgnoreShotDamageType((DamageType)lua_tointeger(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_warpLastPosition(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->warpLastPosition();
	}
	lua_pushnumber(L, 0);
	return 1;
}


/*
int l_mod_setActive(lua_State *L)
{
	bool b = getBool(L);
	dsq->mod.setActive(b);
	lua_pushboolean(L, b);
	return 1;
}
*/

int l_entity_velTowards(lua_State *L)
{
	Entity *e = entity(L);
	int x = lua_tonumber(L, 2);
	int y = lua_tonumber(L, 3);
	int velLen = lua_tonumber(L, 4);
	int range = lua_tonumber(L, 5);
	if (e)
	{
		Vector pos(x,y);
		if (range==0 || ((pos - e->position).getLength2D() < range))
		{
			Vector add = pos - e->position;
			add.setLength2D(velLen);
			e->vel2 += add;
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getBoneLockEntity(lua_State *L)
{
	Entity *e = entity(L);
	Entity *ent = NULL;
	if (e)
	{
		BoneLock *b = e->getBoneLock();
		ent = b->entity;
		//ent = e->boneLock.entity;
	}
	luaPushPointer(L, ent);
	return 1;
}

int l_entity_ensureLimit(lua_State *L)
{
	Entity *e = entity(L);
	dsq->game->ensureLimit(e, lua_tonumber(L, 2), lua_tonumber(L, 3));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setRidingPosition(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->setRidingPosition(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setRidingData(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->setRidingData(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), getBool(L, 5));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setBoneLock(lua_State *L)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	Bone *b = bone(L, 3);
	bool ret = false;
	if (e)
	{
		BoneLock bl;
		bl.entity = e2;
		bl.bone = b;
		bl.on = true;
		bl.collisionMaskIndex = dsq->game->lastCollideMaskIndex;
		ret = e->setBoneLock(bl);
	}
	lua_pushboolean(L, ret);
	return 1;
}

int l_entity_setIngredient(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setIngredientData(getString(L,2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setSegsMaxDist(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->setMaxDist(lua_tonumber(L, 2));

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setBounceType(lua_State *L)
{
	Entity *e = entity(L);
	int v = lua_tointeger(L, 2);
	if (e)
	{
		e->setBounceType((BounceType)v);
	}
	lua_pushnumber(L, v);
	return 1;
}

int l_shot_setBounceType(lua_State *L)
{
	Shot *s = getShot(L);
	int v = lua_tointeger(L, 2);
	if (s)
	{
		s->setBounceType((BounceType)v);
	}
	lua_pushnumber(L, v);
	return 1;
}


int l_user_set_demo_intro(lua_State *L)
{
#ifndef AQUARIA_DEMO
	dsq->user.demo.intro = lua_tonumber(L, 1);
#endif
	lua_pushinteger(L, 0);
	return 1;
}

int l_user_save(lua_State *L)
{
	dsq->user.save();
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_setAutoSkeletalUpdate(lua_State *L)
{
	ScriptedEntity *e = scriptedEntity(L);
	bool v = getBool(L, 2);
	if (e)
		e->setAutoSkeletalUpdate(v);
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_getBounceType(lua_State *L)
{
	Entity *e = entity(L);
	BounceType bt=BOUNCE_SIMPLE;
	if (e)
	{
		bt = (BounceType)e->getBounceType();
	}
	lua_pushinteger(L, (int)bt);
	return 1;
}

int l_entity_setDieTimer(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDieTimer(lua_tonumber(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setLookAtPoint(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->lookAtPoint = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getLookAtPoint(lua_State *L)
{
	Entity *e = entity(L);
	Vector pos;
	if (e)
	{
		pos = e->getLookAtPoint();
	}
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	return 2;
}


int l_entity_setLife(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setLife(lua_tonumber(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;

}
int l_entity_setRiding(lua_State *L)
{
	Entity *e = entity(L);
	Entity *e2 = 0;
	if (lua_touserdata(L, 2) != NULL)
		e2 = entity(L, 2);
	if (e)
	{
		e->setRiding(e2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getHealthPerc(lua_State *L)
{
	Entity *e = entity(L);
	float p = 0;
	if (e)
	{
		p = e->getHealthPerc();
	}
	lua_pushnumber(L, p);
	return 1;
}

int l_entity_getRiding(lua_State *L)
{
	Entity *e = entity(L);
	Entity *ret = 0;
	if (e)
		ret = e->getRiding();
	luaPushPointer(L, ret);
	return 1;
}

int l_entity_setTargetPriority(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->targetPriority = lua_tonumber(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setNodeGroupActive(lua_State *L)
{
	errorLog("setNodeGroup unsupported!");
	Entity *e = entity(L);
	int group = lua_tonumber(L, 2);
	bool v = getBool(L, 3);
	if (e)
	{
		e->setNodeGroupActive(group, v);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_isQuitFlag(lua_State *L)
{
	lua_pushboolean(L, dsq->isQuitFlag());
	return 1;
}

int l_isDeveloperKeys(lua_State *L)
{
	lua_pushboolean(L, dsq->isDeveloperKeys());

	return 1;
}

int l_isDemo(lua_State *L)
{
#ifdef AQUARIA_DEMO
	lua_pushboolean(L, true);
#else
	lua_pushboolean(L, false);
#endif
	return 1;
}

int l_isWithin(lua_State *L)
{
	Vector v1 = getVector(L, 1);
	Vector v2 = getVector(L, 3);
	int dist = lua_tonumber(L, 5);
	/*
	std::ostringstream os;
	os << "v1(" << v1.x << ", " << v1.y << ") v2(" << v2.x << ", " << v2.y << ")";
	debugLog(os.str());
	*/
	Vector d = v2-v1;
	bool v = false;
	if (d.isLength2DIn(dist))
	{
		v = true;
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_stopCursorGlow(lua_State *L)
{
	//dsq->stopCursorGlow();
	lua_pushnumber(L, 0);
	return 1;
}

int l_toggleDamageSprite(lua_State *L)
{
	dsq->game->toggleDamageSprite(getBool(L));
	lua_pushnumber(L, 0);
	return 1;
}

int l_toggleCursor(lua_State *L)
{
	dsq->toggleCursor(getBool(L, 1), lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_toggleBlackBars(lua_State *L)
{
	dsq->toggleBlackBars(getBool(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_setBlackBarsColor(lua_State *L)
{
	Vector c(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3));
	dsq->setBlackBarsColor(c);
	lua_pushnumber(L, 0);
	return 1;
}

int l_toggleLiCombat(lua_State *L)
{
	dsq->continuity.toggleLiCombat(getBool(L));
	lua_pushnumber(L, 0);
	return 1;
}

int l_toggleConversationWindow(lua_State *L)
{
	//dsq->toggleConversationWindow((bool)lua_tointeger(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_toggleConversationWindowSoft(lua_State *L)
{
	//dsq->toggleConversationWindow((bool)lua_tointeger(L, 1), true);
	lua_pushnumber(L, 0);
	return 1;
}

int l_getNoteName(lua_State *L)
{
	lua_pushstring(L, dsq->game->getNoteName(lua_tonumber(L, 1), getString(L, 2)).c_str());
	return 1;
}

int l_getWorldType(lua_State *L)
{
	lua_pushnumber(L, (int)dsq->continuity.getWorldType());
	return 1;
}
	
int l_getNearestNodeByType(lua_State *L)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	int type = lua_tonumber(L, 3);

	luaPushPointer(L, dsq->game->getNearestPath(Vector(x,y), (PathType)type));
	return 1;
}

int l_getNearestNode(lua_State *L)
{
	//Entity *e = entity(L);
	std::string s;
	if (lua_isstring(L, 2))
	{
		s = lua_tostring(L, 2);
	}
	Path *p = path(L);
	luaPushPointer(L, dsq->game->getNearestPath(p, s));
	return 1;
}

int l_fadeOutMusic(lua_State *L)
{
	dsq->sound->fadeMusic(SFT_OUT, lua_tonumber(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_getNode(lua_State *L)
{
	luaPushPointer(L, pathFromName(L));
	return 1;
}

int l_getNodeToActivate(lua_State *L)
{
	luaPushPointer(L, dsq->game->avatar->pathToActivate);
	return 1;
}

int l_setNodeToActivate(lua_State *L)
{
	dsq->game->avatar->pathToActivate = path(L, 1);
	lua_pushinteger(L, 0);
	return 1;
}

int l_setActivation(lua_State *L)
{
	dsq->game->activation = getBool(L, 1);
	lua_pushinteger(L, 0);
	return 1;
}

int l_setNaijaModel(lua_State *L)
{
	std::string s = lua_tostring(L, 1);
	dsq->continuity.setNaijaModel(s);
	lua_pushnumber(L, 0);
	return 1;
}

int l_debugLog(lua_State *L)
{
	std::string s = lua_tostring(L, 1);
	debugLog(s);
	lua_pushstring(L, s.c_str());
	return 1;
}

int l_reconstructGrid(lua_State *L)
{
	dsq->game->reconstructGrid(true);
	lua_pushnumber(L, 0);
	return 1;
}

int l_reconstructEntityGrid(lua_State *L)
{
	dsq->game->reconstructEntityGrid();
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setCanLeaveWater(lua_State *L)
{
	Entity *e = entity(L);
	bool v = getBool(L, 2);
	if (e)
	{
		e->setCanLeaveWater(v);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setSegmentTexture(lua_State *L)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		RenderObject *ro = e->getSegment(lua_tonumber(L, 2));
		if (ro)
		{
			ro->setTexture(lua_tostring(L, 3));
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_findNearestEntityOfType(lua_State *L)
{
	Entity *e = entity(L);
	Entity *nearest = 0;
	if (e)
	{
		int et = (EntityType)lua_tointeger(L, 2);
		int maxRange = lua_tointeger(L, 3);
		int smallestDist = -1;
		Entity *closest = 0;
		FOR_ENTITIES(i)
		{
			Entity *ee = *i;
			if (ee != e)
			{
				int dist = (ee->position - e->position).getSquaredLength2D();
				if (ee->health > 0 && !ee->isEntityDead() && ee->getEntityType() == et && (smallestDist == -1 || dist < smallestDist))
				{
					smallestDist = dist;
					closest = ee;
				}
			}
		}
		if (maxRange == 0 || smallestDist <= sqr(maxRange))
		{
			nearest = closest;
		}
	}
	luaPushPointer(L, nearest);
	return 1;
}

int l_createShot(lua_State *L)
{
	std::string shotData = lua_tostring(L, 1);
	Entity *e = entity(L,2);
	Entity *t = 0;
	if (lua_touserdata(L, 3) != NULL)
		t = entity(L,3);
	Shot *s = 0;
	Vector pos, aim;
	pos.x = lua_tonumber(L, 4);
	pos.y = lua_tonumber(L, 5);
	aim.x = lua_tonumber(L, 6);
	aim.y = lua_tonumber(L, 7);


	s = dsq->game->fireShot(shotData, e, t, pos, aim);

	luaPushPointer(L, s);
	return 1;
}


int l_entity_fireShot(lua_State *L)
{
	Entity *e = entity(L);
	Shot *s = 0;
	if (e)
	{
		int homing = lua_tonumber(L, 6);
		int maxSpeed = lua_tonumber(L, 7);
		Entity *e2 = 0;
		if (lua_touserdata(L, 2) != NULL)
		{
			e2 = entity(L,2);
		}
		std::string particle;
		if (lua_isstring(L, 8))
		{
			particle = lua_tostring(L, 8);
		}
		s = dsq->game->fireShot(e, particle, e->position, lua_tointeger(L, 3), Vector(lua_tonumber(L, 4),lua_tonumber(L, 5),0), e2, homing, maxSpeed);
	}
	luaPushPointer(L, s);
	return 1;
}

int l_entity_sound(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->sound(lua_tostring(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	}
	lua_pushnumber(L, 0);
	return 1;
}


int l_entity_soundFreq(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->soundFreq(lua_tostring(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setSpiritFreeze(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setSpiritFreeze(getBool(L,2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setFillGrid(lua_State *L)
{
	Entity *e = entity(L);
	bool b = getBool(L,2);
	if (e)
	{
		e->fillGridFromQuad = b;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setTouchDamage(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->touchDamage = lua_tonumber(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setTouchPush(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->pushAvatar = lua_tonumber(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setCollideRadius(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->collideRadius = lua_tonumber(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getNormal(lua_State *L)
{
	float nx=0, ny=1;
	Entity *e = entity(L);
	if (e)
	{
		Vector v = e->getForward();
		nx = v.x;
		ny = v.y;
	}
	lua_pushnumber(L, nx);
	lua_pushnumber(L, ny);
	return 2;
}

int l_entity_getAimVector(lua_State *L)
{
	Entity *e = entity(L);
	Vector aim;
	float adjust = lua_tonumber(L, 2);
	float len = lua_tonumber(L, 3);
	bool flip = getBool(L, 4);
	if (e)
	{
		float a = e->rotation.z;
		if (!flip)
			a += adjust;
		else
		{
			if (e->isfh())
			{
				a -= adjust;
			}
			else
			{
				a += adjust;
			}
		}
		a = MathFunctions::toRadians(a);
		aim = Vector(sinf(a)*len, cosf(a)*len);
	}
	lua_pushnumber(L, aim.x);
	lua_pushnumber(L, aim.y);
	return 2;
}

int l_entity_getVectorToEntity(lua_State *L)
{
	Entity *e1 = entity(L);
	Entity *e2 = entity(L, 2);
	if (e1 && e2)
	{
		Vector diff = e2->position -	e1->position;
		lua_pushnumber(L, diff.x);
		lua_pushnumber(L, diff.y);
	}
	else
	{
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);
	}
	return 2;
}

int l_entity_getCollideRadius(lua_State *L)
{
	Entity *e = entity(L);
	int ret = 0;
	if (e)
	{
		ret = e->collideRadius;
	}
	lua_pushnumber(L, ret);
	return 1;
}

int l_entity_setRegisterEntityDied(lua_State *L)
{
	debugLog("entity_setRegisterEntityDied is deceased!");
	lua_pushnumber(L, 0);
	/*
	Entity *e = entity(L);
	bool b = getBool(L, 2);
	if (e)
		e->registerEntityDied = b;
		lua_pushboolean(L, b);
	*/


	return 1;
}

int l_entity_setDropChance(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->dropChance = lua_tonumber(L, 2);
		int amount = lua_tonumber(L, 3);
		ScriptedEntity *se = dynamic_cast<ScriptedEntity*>(e);
		if (se && amount)
		{
			se->manaBallAmount = amount;
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setAffectedBySpell(lua_State *L)
{

	/*
	Entity *e = entity(L);
	SpellType st = (SpellType)int(lua_tonumber(L, 2));
	int v = lua_tonumber(L, 3);
	if (e)
	{
		e->setAffectedBySpell(st, v);
	}
	*/
	debugLog("entity_setAffectedBySpell is deprecated");
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setAffectedBySpells(lua_State *L)
{
	/*
	Entity *e = entity(L);
	if (e)
	{
		e->setAffectedBySpells(lua_tointeger(L, 2));
	}
	*/
	debugLog("entity_setAffectedBySpells is deprecated");
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_warpToNode(lua_State *L)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	if (e && p)
	{
		e->position.stopPath();
		e->position = p->nodes[0].position;
		e->rotateToVec(Vector(0,-1), 0.1);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_stopPull(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->stopPull();
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_stopInterpolating(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->position.stop();
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_moveToNode(lua_State *L)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	if (e && p)
	{
		e->moveToNode(p, lua_tointeger(L, 3), lua_tointeger(L, 4), 0);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_swimToNode(lua_State *L)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	if (e && p)
	{
		e->moveToNode(p, lua_tointeger(L, 3), lua_tointeger(L, 4), 1);
		/*
		ScriptedEntity *se = dynamic_cast<ScriptedEntity*>(e);
		se->swimPath = true;
		*/
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_swimToPosition(lua_State *L)
{
	Entity *e = entity(L);
	//Path *p = path(L, 2);
	Path p;
	PathNode n;
	n.position = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	p.nodes.push_back(n);
	if (e)
	{
		e->moveToNode(&p, lua_tointeger(L, 4), lua_tointeger(L, 5), 1);
		/*
		ScriptedEntity *se = dynamic_cast<ScriptedEntity*>(e);
		se->swimPath = true;
		*/
	}
	lua_pushnumber(L, 0);
	return 1;
}


/*
int l_options(lua_State *L)
{

	dsq->game->avatar->disableInput();
	std::string file = dsq->getDialogueFilename(dsq->dialogueFile);
	std::ifstream inFile(file.c_str());
	std::string s;
	std::vector<DialogOption> options;

	dsq->continuity.cm.figureCase();

	// less than big number
	// get pairs of either string + bool or string + string
	for (int i = 1; i < 19; i+=2)
	{
		DialogOption option;
		int idx = i;
		// first part of the pair is a dialogue file section
		if (lua_isstring(L, idx))
			option.section = lua_tostring(L, idx);
		// second part of pair is either boolean/integer or string
		bool addOption = true;
		if (lua_isboolean(L, idx+1) || lua_isnumber(L, idx+1))
		{
			if (lua_isboolean(L, idx+1))
				option.used = lua_toboolean(L, idx+1);
			else if (lua_isnumber(L, idx+1))
				option.used= bool(lua_tointeger(L, idx+1));
		}
		else if (lua_isstring(L, idx+1))
		{
			// if the second part of the pair is a string, it means we used a min function
			// here we parse the min function
			// we expect a string of length 4 ([type] [ID] [EGO] [SEGO])
			option.cmString = lua_tostring(L, idx+1);
			if (option.cmString.empty())
			{
				debugLog("[option] error parsing cmString");
			}
			else
			{
				std::istringstream is(option.cmString);
				int v;
				is >> v >> option.id >> option.ego >> option.sego;
				option.cmType = (CMType)v;

				// figure out if it should be used
				switch(option.cmType)
				{
				case CMT_TRUE:
					option.used = true;
				break;
				case CMT_FALSE:
					option.used = false;
				break;
				default:
				{
					int sz = dsq->continuity.cm.allowedOptionTypes.size();
					int i;
					for (i = 0; i < sz; i++)
					{
						CMType c = dsq->continuity.cm.allowedOptionTypes[i];
						if (option.cmType == CMT_MIN_EGOSEGO && (c == CMT_MIN_EGO || c == CMT_MIN_SEGO))
						{
							option.used = true;
							break;
						}
						else if (option.cmType == CMT_MAJ_EGOSEGO && (c == CMT_MAJ_EGO || c == CMT_MAJ_SEGO))
						{
							option.used = true;
							break;
						}
						else if (c == option.cmType)
						{
							option.used = true;
							break;
						}
					}
					if (i == sz)
					{
						std::ostringstream os;
						os << dsq->dialogueFile << " - option dropped, choice (" << int((idx-1)/2) << ") function type :"
							<< dsq->continuity.cm.getVerbose(option.cmType);
						debugLog(os.str());
						option.used = false;
					}
				}
				break;
				}

			}
		}
		else
		{
			addOption = false;
		}
		if (option.used)
		{
			if (option.section.find(':') == std::string::npos)
				option.text = option.section;
			else
			{
				// read the text
				std::ifstream inFile;
				dsq->jumpToSection(inFile, option.section);
				std::getline(inFile, option.text);
				if (option.text.empty())
				{
					option.text = "NOTFOUND";
				}
				else
				{
					int pos = option.text.find(':');
					if (pos!=std::string::npos)
					{
						option.text = option.text.substr(pos+2, option.text.size());
					}
				}
			}
		}
		if (addOption)
			options.push_back(option);
	}

	// display the options


	dsq->game->avatar->disableInput();

	dsq->toggleCursor(true);
	while (core->getNestedMains() > 1)
		core->quitNestedMain();

	std::vector<AquariaMenuItem*> menu;
	if (!options.empty())
	{
		int c = 0;
		for (int i = 0; i < options.size(); i++)
		{
			if (options[i].used)
			{
				AquariaMenuItem* a = new AquariaMenuItem;
				a->setLabel(options[i].text);
				a->choice = i;
				//150
				a->position = Vector(400,100 + c*40);
				dsq->game->addRenderObject(a, LR_MENU);
				menu.push_back(a);
				c++;
			}
		}
		core->main();
		for (int i = 0; i < menu.size(); i++)
		{
			menu[i]->alpha = 0;
			menu[i]->safeKill();
		}
		menu.clear();
	}

	DialogOption *d = &(options[dsq->lastChoice]);
	// ADD IN THE STATS HERE
	switch (d->cmType)
	{
	case CMT_TRUE:
	case CMT_FALSE:
	break;
	default:
		dsq->continuity.cm.addID(d->id);
		dsq->continuity.cm.addEGO(d->ego);
		dsq->continuity.cm.addSEGO(d->sego);
		// add stats
		//d->id
		//d->ego
		//d->sego
	break;
	}



	if (d->text != d->section)
		dsq->simpleConversation(d->section);


	dsq->toggleCursor(false);
	lua_pushnumber(L, 0);
	return 1;
}
*/

int l_avatar_setCanDie(lua_State *L)
{
	dsq->game->avatar->canDie = getBool(L, 1);

	lua_pushnumber(L, 0);
	return 1;
}

int l_setGLNearest(lua_State *L)
{
	Texture::filter = GL_NEAREST;

	lua_pushnumber(L, 0);
	return 1;
}

int l_avatar_toggleCape(lua_State *L)
{
	dsq->game->avatar->toggleCape(getBool(L,1));

	lua_pushnumber(L, 0);
	return 1;
}

int l_avatar_setBlockSinging(lua_State *L)
{
	bool b = getBool(L);
	dsq->game->avatar->setBlockSinging(b);
	lua_pushnumber(L, 0);
	return 1;
}

int l_avatar_fallOffWall(lua_State *L)
{
	dsq->game->avatar->fallOffWall();
	lua_pushnumber(L, 0);
	return 1;
}

int l_avatar_isBursting(lua_State *L)
{
	lua_pushboolean(L, dsq->game->avatar->bursting);
	return 1;
}

int l_avatar_isLockable(lua_State *L)
{
	lua_pushboolean(L, dsq->game->avatar->isLockable());
	return 1;
}

int l_avatar_isRolling(lua_State *L)
{
	lua_pushboolean(L, dsq->game->avatar->isRolling());
	return 1;
}

int l_avatar_isOnWall(lua_State *L)
{
	bool v = dsq->game->avatar->state.lockedToWall;
	lua_pushboolean(L, v);
	return 1;
}

int l_avatar_isShieldActive(lua_State *L)
{
	bool v = (dsq->game->avatar->activeAura == AURA_SHIELD);
	lua_pushboolean(L, v);
	return 1;

}

int l_avatar_getStillTimer(lua_State *L)
{
	lua_pushnumber(L, dsq->game->avatar->stillTimer.getValue());
	return 1;
}

int l_avatar_getRollDirection(lua_State *L)
{
	int v = 0;
	if (dsq->game->avatar->isRolling())
		v = dsq->game->avatar->rollDir;
	lua_pushnumber(L, v);
	return 1;
}

int l_avatar_getSpellCharge(lua_State *L)
{
	lua_pushnumber(L, dsq->game->avatar->state.spellCharge);
	return 1;
}

int l_jumpState(lua_State *L)
{
	dsq->enqueueJumpState(lua_tostring(L, 1), getBool(L, 2));
	lua_pushinteger(L, 0);
	return 1;
}

int l_goToTitle(lua_State *L)
{
	dsq->title();
	lua_pushnumber(L, 0);
	return 1;
}

int l_getEnqueuedState(lua_State *L)
{
	lua_pushstring(L, dsq->getEnqueuedJumpState().c_str());
	return 1;
}

int l_learnSpell(lua_State* L)
{
	//dsq->continuity.learnSpell((SpellType)lua_tointeger(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_learnSong(lua_State* L)
{
	dsq->continuity.learnSong(lua_tointeger(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_unlearnSong(lua_State *L)
{
	dsq->continuity.unlearnSong(lua_tointeger(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_showInGameMenu(lua_State *L)
{
	dsq->game->showInGameMenu(getBool(L, 1), getBool(L, 2), (MenuPage)lua_tointeger(L, 3));
	lua_pushnumber(L, 0);
	return 1;
}

int l_hideInGameMenu(lua_State *L)
{
	dsq->game->hideInGameMenu();
	lua_pushnumber(L, 0);
	return 1;
}

Quad *image=0;

int l_showImage(lua_State *L)
{
	dsq->game->showImage(getString(L));
	lua_pushnumber(L, 0);
	return 1;
}

int l_hideImage(lua_State *L)
{
	dsq->game->hideImage();
	lua_pushnumber(L, 0);
	return 1;
}

int l_hasSong(lua_State* L)
{
	bool b = dsq->continuity.hasSong(lua_tointeger(L, 1));
	lua_pushboolean(L, b);
	return 1;
}

int l_isInConversation(lua_State *L)
{
	lua_pushboolean(L, 0);
	return 1;
}

int l_loadSound(lua_State *L)
{
	void *handle = core->sound->loadLocalSound(getString(L, 1));
	luaPushPointer(L, handle);
	return 1;
}

int l_loadMap(lua_State *L)
{
	std::string s = getString(L, 1);
	std::string n = getString(L, 2);

	if (!s.empty())
	{
		if (!n.empty())
		{
			if (dsq->game->avatar)
				dsq->game->avatar->disableInput();
			dsq->game->warpToSceneNode(s, n);
		}
		else
		{
			if (dsq->game->avatar)
				dsq->game->avatar->disableInput();
			dsq->game->transitionToScene(s);
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_followPath(lua_State *L)
{
	/*
	std::ostringstream os2;
	os2 << lua_tointeger(L, 1);
	errorLog(os2.str());
	std::ostringstream os;
	os << "Entity: " << scriptedEntity(L)->name << " moving on Path: " << lua_tostring(L, 2);
	debugLog(os.str());
	*/
	Entity *e = entity(L);
	if (e)
	{
		Path *p = path(L, 2);
		int speedType = lua_tonumber(L, 3);
		int dir = lua_tonumber(L, 4);

		e->followPath(p, speedType, dir);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_enableMotionBlur(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->enableMotionBlur(10, 2);
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_disableMotionBlur(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->disableMotionBlur();
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_warpToPathStart(lua_State *L)
{
	ScriptedEntity *e = scriptedEntity(L);
	std::string s;
	if (lua_isstring(L, 2))
		s = lua_tostring(L, 2);
	if (s.empty())
		e->warpToPathStart();
	else
	{
		//e->followPath(s, 0, 0);
		e->warpToPathStart();
		e->stopFollowingPath();
	}
	lua_pushnumber(L, 0);
	return 1;
}

/*
int l_entity_spawnParticleEffect(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		dsq->spawnParticleEffect(lua_tointeger(L, 2), e->position + Vector(lua_tointeger(L, 3), lua_tointeger(L, 4)), 0, lua_tonumber(L, 5));
	lua_pushnumber(L, 0);
	return 1;
}
*/

int l_getIngredientGfx(lua_State *L)
{
	lua_pushstring(L, dsq->continuity.getIngredientGfx(getString(L, 1)).c_str());
	return 1;
}

int l_spawnIngredient(lua_State *L)
{
	int times = lua_tonumber(L, 4);
	if (times == 0) times = 1;
	bool out = getBool(L, 5);
	Entity *e = dsq->game->spawnIngredient(getString(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), times, out);
	
	luaPushPointer(L, e);
	return 1;
}

int l_getNearestIngredient(lua_State *L)
{
	Ingredient *i = dsq->game->getNearestIngredient(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)), lua_tonumber(L, 3));
	luaPushPointer(L, i);
	return 1;
}

int l_dropIngredients(lua_State *L)
{
	//dsq->continuity.dropIngredients(lua_tonumber(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_spawnAllIngredients(lua_State *L)
{
	dsq->game->spawnAllIngredients(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)));
	lua_pushnumber(L, 0);
	return 1;
}

int l_spawnParticleEffect(lua_State *L)
{
	dsq->spawnParticleEffect(getString(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 5), lua_tonumber(L, 4));
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_showFrame(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
		b->showFrame(lua_tointeger(L, 2));
	lua_pushnumber(L, 1);
	return 1;
}

int l_bone_setRenderPass(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setRenderPass(lua_tonumber(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_setSegmentOffset(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
		b->segmentOffset = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_setSegmentProps(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setSegmentProps(lua_tonumber(L, 2), lua_tonumber(L, 3), getBool(L, 4));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_setSegmentChainHead(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
	{
		if (getBool(L, 2))
			b->segmentChain = 1;
		else
			b->segmentChain = 0;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_addSegment(lua_State *L)
{
	Bone *b = bone(L);
	Bone *b2 = bone(L, 2);
	if (b && b2)
		b->addSegment(b2);
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_setAnimated(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setAnimated(lua_tointeger(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_lookAtEntity(lua_State *L)
{
	Bone *b = bone(L);
	Entity *e = entity(L, 2);
	if (b && e)
	{
		Vector pos = e->position;
		if (e->getEntityType() == ET_AVATAR)
		{
			pos = e->skeletalSprite.getBoneByIdx(1)->getWorldPosition();
		}
		b->lookAt(pos, lua_tonumber(L, 3), lua_tonumber(L, 4),lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_setSegs(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
		b->setSegs(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8), lua_tointeger(L, 9));
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_setSegs(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->setSegs(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8), lua_tointeger(L, 9));
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_resetTimer(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->resetTimer(lua_tonumber(L, 2));
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_stopFollowingPath(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		if (e->isFollowingPath())
		{
			e->stopFollowingPath();
		}
	}
	lua_tointeger(L, 0);
	return 1;
}

int l_entity_slowToStopPath(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		if (e->isFollowingPath())
		{
			debugLog("calling slow to stop path");
			e->slowToStopPath(lua_tonumber(L, 2));
		}
		else
		{
			debugLog("wasn't following path");
		}
	}
	lua_tointeger(L, 0);
	return 1;
}

int l_entity_stopTimer(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->stopTimer();
	lua_pushinteger(L, 0);
	return 1;
}

Vector createEntityOffset(0, 0);
int l_entity_createEntity(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		dsq->game->createEntity(dsq->getEntityTypeIndexByName(lua_tostring(L, 2)), 0, e->position+createEntityOffset, 0, false, "", ET_ENEMY, BT_NORMAL, 0, 0, true);
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_checkSplash(lua_State *L)
{
	Entity *e = entity(L);
	bool ret = false;
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	if (e)
		ret = e->checkSplash(Vector(x,y));
	lua_pushboolean(L, ret);
	return 1;
}


int l_entity_isUnderWater(lua_State *L)
{
	Entity *e = entity(L);
	bool b = false;
	if (e)
	{
		b = e->isUnderWater();
	}
	lua_pushboolean(L, b);
	return 1;
}

int l_entity_isBeingPulled(lua_State *L)
{
	Entity *e = entity(L);
	bool v= false;
	if (e)
		v = (dsq->game->avatar->pullTarget == e);
	lua_pushboolean(L, v);
	return 1;
}

int l_avatar_setPullTarget(lua_State *L)
{
	Entity *e = 0;
	if (lua_tonumber(L, 1) != 0)
		e = entity(L);

	if (dsq->game->avatar->pullTarget != 0)
		dsq->game->avatar->pullTarget->stopPull();

	dsq->game->avatar->pullTarget = e;

	if (e)
		e->startPull();

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_isDead(lua_State *L)
{
	Entity *e = entity(L);
	bool v= false;
	if (e)
	{
		v = e->isEntityDead();
	}
	lua_pushboolean(L, v);
	return 1;
}


int l_getLastCollidePosition(lua_State *L)
{
	lua_pushnumber(L, dsq->game->lastCollidePosition.x);
	lua_pushnumber(L, dsq->game->lastCollidePosition.y);
	return 2;
}

int l_entity_isNearGround(lua_State *L)
{
	Entity *e = entity(L);
	int sampleArea = 0;
	bool value = false;
	if (e)
	{
		if (lua_isnumber(L, 2))
			sampleArea = int(lua_tonumber(L, 2));
		Vector v = dsq->game->getWallNormal(e->position, sampleArea);
		if (!v.isZero())
		{
			//if (v.y < -0.5 && fabs(v.x) < 0.4)
			if (v.y < 0 && fabs(v.x) < 0.6)
			{
				value = true;
			}
		}
		/*
		Vector v = e->position + Vector(0,e->collideRadius + TILE_SIZE/2);
		std::ostringstream os;
		os << "checking (" << v.x << ", " << v.y << ")";
		debugLog(os.str());
		TileVector t(v);
		TileVector c;
		for (int i = -5; i < 5; i++)
		{
			c.x = t.x+i;
			c.y = t.y;
			if (dsq->game->isObstructed(t))
			{
				value = true;
			}
		}
		*/
	}
	lua_pushboolean(L, value);
	return 1;
}

int l_entity_isHit(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
		v = e->isHit();
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_waitForPath(lua_State *L)
{
	Entity *e = entity(L);
	while (e && e->isFollowingPath())
	{
		core->main(FRAME_TIME);
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_quitNestedMain(lua_State *L)
{
	core->quitNestedMain();
	lua_pushinteger(L, 0);
	return 1;
}

int l_isNestedMain(lua_State *L)
{
	lua_pushboolean(L, core->isNested());
	return 1;
}

int l_entity_watchForPath(lua_State *L)
{
	dsq->game->avatar->disableInput();

	Entity *e = entity(L);
	while (e && e->isFollowingPath())
	{
		core->main(FRAME_TIME);
	}

	dsq->game->avatar->enableInput();
	lua_pushinteger(L, 0);
	return 1;
}


int l_watchForVoice(lua_State *L)
{
	int quit = lua_tointeger(L, 1);
	while (dsq->sound->isPlayingVoice())
	{
		dsq->watch(FRAME_TIME, quit);
		if (quit && dsq->isQuitFlag())
		{
			dsq->sound->stopVoice();
			break;
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}


int l_entity_isSlowingToStopPath(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->isSlowingToStopPath();
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_resumePath(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->position.resumePath();
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_isAnimating(lua_State *L)
{
	Entity *e = entity(L);
	bool v= false;
	if (e)
	{
		v = e->skeletalSprite.isAnimating(lua_tonumber(L, 2));
	}
	lua_pushboolean(L, v);
	return 1;
}


int l_entity_getAnimationName(lua_State *L)
{
	Entity *e = entity(L);
	std::string ret;
	int layer = lua_tonumber(L, 2);
	if (e)
	{
		if (Animation *anim = e->skeletalSprite.getCurrentAnimation(layer))
		{
			ret = anim->name;
		}
	}
	lua_pushstring(L, ret.c_str());
	return 1;
}

int l_entity_getAnimationLength(lua_State *L)
{
	Entity *e = entity(L);
	float ret=0;
	int layer = lua_tonumber(L, 2);
	if (e)
	{
		if (Animation *anim = e->skeletalSprite.getCurrentAnimation(layer))
		{
			ret = anim->getAnimationLength();
		}
	}
	lua_pushnumber(L, ret);
	return 1;
}

int l_entity_isFollowingPath(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		lua_pushboolean(L, e->isFollowingPath());
	else
		lua_pushboolean(L, false);
	return 1;
}

int l_entity_setBehaviorType(lua_State* L)
{
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_toggleBone(lua_State *L)
{
	Entity *e = entity(L);
	Bone *b = bone(L, 2);
	if (e && b)
	{
		e->skeletalSprite.toggleBone(e->skeletalSprite.getBoneIdx(b), lua_tonumber(L, 3));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getBehaviorType(lua_State* L)
{
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_setColor(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		//e->color = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
		e->color.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4)), lua_tonumber(L, 5));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_bone_scale(lua_State *L)
{
	Bone *e = bone(L);
	if (e)
	{
		//e->color = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
		e->scale.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_bone_setBlendType(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setBlendType(lua_tonumber(L, 2));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_bone_update(lua_State *L)
{
	bone(L)->update(lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_setColor(lua_State *L)
{
	Bone *e = bone(L);
	if (e)
	{
		//e->color = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
		e->color.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4)), lua_tonumber(L, 5));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_bone_rotate(lua_State *L)
{
	Bone *e = bone(L);
	if (e)
		e->rotation.interpolateTo(Vector(0,0,lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6));
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_rotateOffset(lua_State *L)
{
	Bone *e = bone(L);
	if (e)
		e->rotationOffset.interpolateTo(Vector(0,0,lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6));
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_getRotation(lua_State *L)
{
	Bone *e = bone(L);
	if (e)
		lua_pushnumber(L, e->rotation.z);
	else
		lua_pushnumber(L, 0);
	return 1;
}


int l_bone_setPosition(lua_State* L)
{
	Bone *b = bone(L);
	if (b)
	{
		b->position.interpolateTo(Vector(lua_tointeger(L, 2), lua_tointeger(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_bone_getWorldRotation(lua_State *L)
{
	Bone *b = bone(L);
	float rot = 0;
	if (b)
	{
		//rot = b->getAbsoluteRotation().z;
		rot = b->getWorldRotation();
	}
	lua_pushnumber(L, rot);
	return 1;
}

int l_bone_getWorldPosition(lua_State *L)
{
	Bone *b = bone(L);
	float x=0,y=0;
	if (b)
	{
		Vector v = b->getWorldPosition();
		x = v.x;
		y = v.y;
	}
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}

int l_entity_setBlendType(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setBlendType(lua_tonumber(L, 2));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_setEntityType(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->setEntityType(EntityType(lua_tointeger(L, 2)));
	lua_pushinteger(L, 1);
	return 1;
}

int l_entity_getEntityType(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		lua_pushinteger(L, int(e->getEntityType()));
	else
		lua_pushinteger(L, 0);
	return 1;
}

int l_cam_snap(lua_State *L)
{
	dsq->game->snapCam();
	lua_pushnumber(L, 0);
	return 1;
}

int l_cam_toNode(lua_State *L)
{
	Path *p = path(L);
	if (p)
	{
		//dsq->game->cameraPanToNode(p, 500);
		dsq->game->setCameraFollow(&p->nodes[0].position);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_cam_toEntity(lua_State *L)
{
	if (lua_touserdata(L, 1) == NULL)
	{
		Vector *pos = 0;
		dsq->game->setCameraFollow(pos);
	}
	else
	{
		Entity *e = entity(L);
		if (e)
		{
			dsq->game->setCameraFollowEntity(e);
			//dsq->game->cameraPanToNode(p, 500);
			//dsq->game->setCameraFollow(&p->nodes[0].position);
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_cam_setPosition(lua_State *L)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float time = lua_tonumber(L, 3);
	int loopType = lua_tointeger(L, 4);
	int pingPong = lua_tointeger(L, 5);
	int ease = lua_tointeger(L, 6);

	/*

	*/

	Vector p = dsq->game->getCameraPositionFor(Vector(x,y));

	dsq->game->cameraInterp.stop();
	dsq->game->cameraInterp.interpolateTo(p, time, loopType, pingPong, ease);

	if (time == 0)
	{
		dsq->game->cameraInterp = p;
	}

	dsq->cameraPos = p;
	return 1;
}

/*
int l_cam_restore(lua_State *L)
{
	dsq->game->setCameraFollow(dsq->game->avatar);
	lua_pushnumber(L, 0);
	return 1;
}
*/


int l_entity_spawnParticlesFromCollisionMask(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		int intv = lua_tonumber(L, 3);
		if (intv <= 0)
			intv = 1;
		e->spawnParticlesFromCollisionMask(getString(L, 2), intv);
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_initEmitter(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	int e = lua_tointeger(L, 2);
	std::string pfile = getString(L, 3);
	if (se)
	{
		se->initEmitter(e, pfile);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_startEmitter(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	int e = lua_tointeger(L, 2);
	std::string pfile = getString(L, 3);
	if (se)
	{
		se->startEmitter(e);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_stopEmitter(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	int e = lua_tointeger(L, 2);
	std::string pfile = getString(L, 3);
	if (se)
	{
		se->stopEmitter(e);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_initStrands(lua_State *L)
{
	ScriptedEntity *e = scriptedEntity(L);
	if (e)
	{
		e->initStrands(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), Vector(lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8)));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_initSkeletal(lua_State* L)
{
	//ScriptedEntity *e = (ScriptedEntity*)(si->getCurrentEntity());
	ScriptedEntity *e = scriptedEntity(L);
	e->renderQuad = false;
	e->setWidthHeight(128, 128);
	e->skeletalSprite.loadSkeletal(lua_tostring(L, 2));
	if (lua_isstring(L, 3))
	{
		std::string s = lua_tostring(L, 3);
		if (!s.empty())
		{
			e->skeletalSprite.loadSkin(s);
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}


SkeletalSprite *getSkeletalSprite(Entity *e)
{
	Avatar *a;
	ScriptedEntity *se;
	SkeletalSprite *skel = 0;
	if (a=dynamic_cast<Avatar*>(e))
	{
		//a->skeletalSprite.transitionAnimate(lua_tostring(L, 2), 0.15, lua_tointeger(L, 3));
		skel = &a->skeletalSprite;
	}
	else if (se=dynamic_cast<ScriptedEntity*>(e))
	{
		skel = &se->skeletalSprite;
	}
	return skel;
}

int l_entity_idle(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->idle();
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_stopAllAnimations(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->skeletalSprite.stopAllAnimations();
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setAnimLayerTimeMult(lua_State *L)
{
	Entity *e = entity(L);
	int layer = 0;
	float t = 0;
	if (e)
	{
		layer = lua_tointeger(L, 2);
		t = lua_tonumber(L, 3);
		AnimationLayer *l = e->skeletalSprite.getAnimationLayer(layer);
		if (l)
		{
			l->timeMultiplier.interpolateTo(t, lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
		}
	}
	lua_tonumber(L, t);
	return 1;
}

int l_entity_animate(lua_State *L)
{
	SkeletalSprite *skel = getSkeletalSprite(entity(L));

	// 0.15
	// 0.2
	float transition = lua_tonumber(L, 5);
	if (transition == -1)
		transition = 0;
	else if (transition == 0)
		transition = 0.2;
	float ret = skel->transitionAnimate(lua_tostring(L, 2), transition, lua_tointeger(L, 3), lua_tointeger(L, 4));

	lua_pushnumber(L, ret);
	return 1;
}

int l_entity_moveToFront(lua_State *L)
{
	Entity *e = entity(L);
	e->moveToFront();

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_moveToBack(lua_State *L)
{
	Entity *e = entity(L);
	e->moveToBack();

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_move(lua_State *L)
{
	//Entity *e = si->getCurrentEntity();
	Entity *e = entity(L);
	bool ease = lua_tointeger(L, 5);
	Vector p(lua_tointeger(L, 2), lua_tointeger(L, 3));
	if (lua_tointeger(L, 6))
	{
		p = e->position + p;
	}
	if (!ease)
	{
		e->position.interpolateTo(p, lua_tonumber(L, 4));
	}
	else
	{
		e->position.interpolateTo(p, lua_tonumber(L, 4), 0, 0, 1);
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_spawnManaBall(lua_State *L)
{
	Vector p;
	p.x = lua_tonumber(L, 1);
	p.y = lua_tonumber(L, 2);
	int amount = lua_tonumber(L, 3);
	dsq->game->spawnManaBall(p, amount);
	lua_pushinteger(L, 0);
	return 1;
}

int l_spawnAroundEntity(lua_State *L)
{
	Entity *e = entity(L);
	int num = lua_tonumber(L, 2);
	int radius = lua_tonumber(L, 3);
	std::string entType = lua_tostring(L, 4);
	std::string name = lua_tostring(L, 5);
	int idx = dsq->game->getIdxForEntityType(entType);
	if (e)
	{
		Vector pos = e->position;
		for (int i = 0; i < num; i++)
		{
			float angle = i*((2*PI)/float(num));

			e = dsq->game->createEntity(idx, 0, pos + Vector(sin(angle)*radius, cos(angle)*radius), 0, false, name);
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_createBeam(lua_State *L)
{
	int x = lua_tointeger(L, 1);
	int y = lua_tointeger(L, 2);
	float a = lua_tonumber(L, 3);
	int l = lua_tointeger(L, 4);
	Beam *b = new Beam(Vector(x,y), a);
	if (l == 1)
		dsq->game->addRenderObject(b, LR_PARTICLES);
	else
		dsq->game->addRenderObject(b, LR_ENTITIES_MINUS2);
	luaPushPointer(L, b);
	return 1;
}

int l_beam_setPosition(lua_State *L)
{
	Beam *b = beam(L);
	if (b)
	{
		b->position = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
		b->trace();
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_beam_setDamage(lua_State *L)
{
	Beam *b = beam(L);
	if (b)
	{
		b->setDamage(lua_tonumber(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_beam_setBeamWidth(lua_State *L)
{
	Beam *b = beam(L);
	if (b)
	{
		b->setBeamWidth(lua_tonumber(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_beam_setTexture(lua_State *L)
{
	Beam *b = beam(L);
	if (b)
	{
		b->setTexture(getString(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_beam_setAngle(lua_State *L)
{
	Beam *b = beam(L);
	if (b)
	{
		b->angle = lua_tonumber(L, 2);
		b->trace();
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_beam_delete(lua_State *L)
{
	Beam *b = beam(L);
	if (b)
	{
		b->safeKill();
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_getStringBank(lua_State *L)
{
	lua_pushstring(L, dsq->continuity.stringBank.get(lua_tointeger(L, 1)).c_str());
	return 1;
}

int l_isPlat(lua_State *L)
{
	int plat = lua_tointeger(L, 1);
	bool v = false;
#ifdef BBGE_BUILD_WINDOWS
	v = (plat == 0);
#elif BBGE_BUILD_MACOSX
	v = (plat == 1);
#elif BBGE_BUILD_UNIX
	v = (plat == 2);
#endif
	lua_pushboolean(L, v);
	return 1;
}

int l_getAngleBetweenEntities(lua_State *L)
{
	Entity *e1 = entity(L, 1);
	Entity *e2 = entity(L, 2);
	float angle=0;
	if (e1 && e2)
	{
		MathFunctions::calculateAngleBetweenVectorsInRadians(e1->position, e2->position, angle);
	}
	lua_pushnumber(L, angle);
	return 1;
}

// in radians
int l_getAngleBetween(lua_State *L)
{
	Vector p1(lua_tonumber(L, 1), lua_tonumber(L, 2));
	Vector p2(lua_tonumber(L, 3), lua_tonumber(L, 4));
	float angle = 0;
	MathFunctions::calculateAngleBetweenVectorsInRadians(p1, p2, angle);
	angle = 2*PI - angle;
	angle -= PI/2;
	while (angle > 2*PI)
		angle -= 2*PI;
	while (angle < 0)
		angle += 2*PI;

	//angle = (angle/PI)*180;
	/*
	angle += PI/2;
	angle = PI-(2*PI-angle);
	*/
	lua_pushnumber(L, angle);
	return 1;
}

int l_createEntity(lua_State *L)
{
	std::string type = lua_tostring(L, 1);
	std::string name;
	if (lua_isstring(L, 2))
		name = lua_tostring(L, 2);
	int x = lua_tointeger(L, 3);
	int y = lua_tointeger(L, 4);

	Entity *e = 0;
	e = dsq->game->createEntity(type, 0, Vector(x, y), 0, false, name, ET_ENEMY, BT_NORMAL, 0, 0, true);

	/*
	int idx = dsq->game->getIdxForEntityType(type);
	Entity *e = 0;
	if (idx == -1)
	{
		errorLog("Unknown entity type [" + type + "]");
	}
	else
	{
		e = dsq->game->createEntity(idx, 0, Vector(x,y), 0, false, name, ET_ENEMY, BT_NORMAL, 0, 0, true);
	}
	*/
	luaPushPointer(L, e);
	return 1;
}
// moveEntity(name, x, y, time, ease)
int l_moveEntity(lua_State* L)
{
	errorLog ("moveEntity is deprecated");
	/*
	Entity *e = dsq->getEntityByName(lua_tostring(L, 1));
	bool ease = lua_tointeger(L, 5);
	Vector p(lua_tointeger(L, 2), lua_tointeger(L, 3));
	if (lua_tonumber(L, 5))
	{
		p = e->position + p;
	}
	if (ease)
	{
		e->position.interpolateTo(p, lua_tonumber(L, 4));
	}
	else
	{
		e->position.interpolateTo(p, lua_tonumber(L, 4), 0, 0, 1);
	}
	*/
	lua_pushnumber(L, 0);
	return 1;
}

int l_savePoint(lua_State* L)
{
	Path *p = path(L);
	Vector position;
	if (p)
	{
		//dsq->game->avatar->moveToNode(p, 0, 0, 1);
		position = p->nodes[0].position;
	}

	dsq->doSavePoint(position);
	/*
	Entity *e=0;
	if (e=si->getCurrentEntity())
	{
		dsq->doSavePoint(e);
	}
	*/
	lua_pushnumber(L, 0);
	return 1;
}

int l_pause(lua_State *L)
{
	dsq->game->togglePause(1);
	lua_pushnumber(L, 0);
	return 1;
}

int l_unpause(lua_State *L)
{
	dsq->game->togglePause(0);
	lua_pushnumber(L, 0);
	return 1;
}

int l_clearControlHint(lua_State *L)
{
	dsq->game->clearControlHint();
	lua_pushnumber(L, 0);
	return 1;
}

int l_setSceneColor(lua_State *L)
{
	dsq->game->sceneColor3.interpolateTo(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	lua_pushnumber(L, 0);
	return 1;
}

int l_setCameraLerpDelay(lua_State *L)
{
	dsq->game->cameraLerpDelay = lua_tonumber(L, 1);
	lua_pushnumber(L, 0);
	return 1;
}

int l_setControlHint(lua_State *L)
{
	std::string str = lua_tostring(L, 1);
	bool left = getBool(L, 2);
	bool right = getBool(L, 3);
	bool middle = getBool(L, 4);
	float t = lua_tonumber(L, 5);
	std::string s;
	if (lua_isstring(L, 6))
		s = lua_tostring(L, 6);
	int songType = lua_tointeger(L, 7);
	float scale = lua_tonumber(L, 8);
	if (scale == 0)
		scale = 1;

	dsq->game->setControlHint(str, left, right, middle, t, s, false, songType, scale);
	lua_pushnumber(L, 0);
	return 1;
}

int l_setCanChangeForm(lua_State *L)
{
	dsq->game->avatar->canChangeForm = getBool(L);
	lua_pushnumber(L, 0);
	return 1;
}

int l_setInvincibleOnNested(lua_State *L)
{
	dsq->game->invincibleOnNested = getBool(L);
	lua_pushnumber(L, 0);
	return 1;
}

int l_setCanWarp(lua_State* L)
{
	dsq->game->avatar->canWarp = getBool(L);
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_generateCollisionMask(lua_State *L)
{
	Entity *e = entity(L);
	float num = lua_tonumber(L, 2);
	if (e)
	{
		e->generateCollisionMask(num);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_damage(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		DamageData d;
		//d.attacker = e;
		d.attacker = entity(L, 2);
		d.damage = lua_tonumber(L, 3);
		d.damageType = (DamageType)lua_tointeger(L, 4);

		e->damage(d);
	}
	lua_pushnumber(L, 0);
	return 1;
}

// must be called in init
int l_entity_setEntityLayer(lua_State *L)
{
	ScriptedEntity *e = scriptedEntity(L);
	int l = lua_tonumber(L, 2);
	if (e)
	{
		e->setEntityLayer(l);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setRenderPass(lua_State *L)
{
	Entity *e = entity(L);
	int pass = lua_tonumber(L, 2);
	if (e)
		e->setOverrideRenderPass(pass);
	lua_pushnumber(L, 0);
	return 1;
}

// intended to be used for setting max health and refilling it all
int l_entity_setHealth(lua_State *L)
{
	Entity *e = entity(L, 1);
	int h = lua_tonumber(L, 2);
	if (e)
	{
		e->health = e->maxHealth = h;
	}
	lua_pushnumber(L, 0);
	return 1;
}

// intended to be used for setting max health and refilling it all
int l_entity_changeHealth(lua_State *L)
{
	Entity *e = entity(L, 1);
	int h = lua_tonumber(L, 2);
	if (e)
	{
		e->health += h;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_heal(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->heal(lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_revive(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->revive(lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_screenFadeCapture(lua_State *L)
{
	dsq->screenTransition->capture();
	lua_pushnumber(L, 0);
	return 1;
}


int l_screenFadeTransition(lua_State *L)
{
	dsq->screenTransition->transition(lua_tonumber(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_screenFadeGo(lua_State *L)
{
	dsq->screenTransition->go(lua_tonumber(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_isEscapeKey(lua_State *L)
{
	bool isDown = dsq->game->isActing(ACTION_ESC);
	lua_pushboolean(L, isDown);
	return 1;
}

int l_isLeftMouse(lua_State *L)
{
	bool isDown = core->mouse.buttons.left || (dsq->game->avatar && dsq->game->avatar->pollAction(ACTION_PRIMARY));
	lua_pushboolean(L, isDown);
	return 1;
}

int l_isRightMouse(lua_State *L)
{
	bool isDown = core->mouse.buttons.right || (dsq->game->avatar && dsq->game->avatar->pollAction(ACTION_SECONDARY));
	lua_pushboolean(L, isDown);
	return 1;
}

int l_setTimerTextAlpha(lua_State *L)
{
	dsq->game->setTimerTextAlpha(lua_tonumber(L, 1), lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_setTimerText(lua_State *L)
{
	dsq->game->setTimerText(lua_tonumber(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_getWallNormal(lua_State *L)
{
	float x,y;
	x = lua_tonumber(L, 1);
	y = lua_tonumber(L, 2);
	int range = lua_tonumber(L, 3);
	if (range == 0)
	{
		range = 5;
	}

	Vector n = dsq->game->getWallNormal(Vector(x, y), range);

	lua_pushnumber(L, n.x);
	lua_pushnumber(L, n.y);
	return 2;
}

int l_incrFlag(lua_State* L)
{
	std::string f = lua_tostring(L, 1);
	int v = 1;
	if (lua_isnumber(L, 2))
		v = lua_tointeger(L, 2);
	dsq->continuity.setFlag(f, dsq->continuity.getFlag(f)+v);
	lua_pushnumber(L, 0);
	return 1;
}

int l_decrFlag(lua_State* L)
{
	std::string f = lua_tostring(L, 1);
	int v = 1;
	if (lua_isnumber(L, 2))
		v = lua_tointeger(L, 2);
	dsq->continuity.setFlag(f, dsq->continuity.getFlag(f)-v);
	lua_pushnumber(L, 0);
	return 1;
}

int l_setFlag(lua_State* L)
{
	/*
	if (lua_isstring(L, 1))
		dsq->continuity.setFlag(lua_tostring(L, 1), lua_tonumber(L, 2));
	else
	*/
	dsq->continuity.setFlag(lua_tointeger(L, 1), lua_tointeger(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_getFlag(lua_State* L)
{
	int v = 0;
	/*
	if (lua_isstring(L, 1))
		v = dsq->continuity.getFlag(lua_tostring(L, 1));
	else if (lua_isnumber(L, 1))
	*/
	v = dsq->continuity.getFlag(lua_tointeger(L, 1));

	lua_pushnumber(L, v);
	return 1;
}

int l_getStringFlag(lua_State *L)
{
	lua_pushstring(L, dsq->continuity.getStringFlag(getString(L, 1)).c_str());
	return 1;
}

int l_entity_x(lua_State *L)
{
	Entity *e = entity(L);
	float v = 0;
	if (e)
	{
		v = e->position.x;
	}
	lua_pushnumber(L, v);
	return 1;
}

int l_entity_y(lua_State *L)
{
	Entity *e = entity(L);
	float v = 0;
	if (e)
	{
		v = e->position.y;
	}
	lua_pushnumber(L, v);
	return 1;
}

int l_node_setActive(lua_State *L)
{
	Path *p = path(L);
	bool v = getBool(L, 2);
	if (p)
	{
		p->active = v;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_node_setCursorActivation(lua_State *L)
{
	Path *p = path(L);
	bool v = getBool(L, 2);
	if (p)
	{
		p->cursorActivation = v;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_node_setCatchActions(lua_State *L)
{
	Path *p = path(L);
	bool v = getBool(L, 2);
	if (p)
	{
		p->catchActions = v;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_node_isEntityInRange(lua_State *L)
{
	Path *p = path(L);
	Entity *e = entity(L,2);
	int range = lua_tonumber(L, 3);
	bool v = false;
	if (p && e)
	{
		if ((p->nodes[0].position - e->position).isLength2DIn(range))
		{
			v = true;
		}
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_node_isEntityPast(lua_State *L)
{
	Path *p = path(L);
	bool past = false;
	if (p && !p->nodes.empty())
	{
		PathNode *n = &p->nodes[0];
		Entity *e = entity(L, 2);
		if (e)
		{
			bool checkY = lua_tonumber(L, 3);
			int dir = lua_tonumber(L, 4);
			int range = lua_tonumber(L, 5);
			if (!checkY)
			{
				if (e->position.x > n->position.x-range && e->position.x < n->position.x+range)
				{
					if (!dir)
					{
						if (e->position.y < n->position.y)
							past = true;
					}
					else
					{
						if (e->position.y > n->position.y)
							past = true;
					}
				}
			}
			else
			{
				if (e->position.y > n->position.y-range && e->position.y < n->position.y+range)
				{
					if (!dir)
					{
						if (e->position.x < n->position.x)
							past = true;
					}
					else
					{
						if (e->position.x > n->position.x)
							past = true;
					}
				}
			}
		}
	}
	lua_pushboolean(L, past);
	return 1;
}

int l_node_x(lua_State *L)
{
	Path *p = path(L);
	float v = 0;
	if (p)
	{
		v = p->nodes[0].position.x;
	}
	lua_pushnumber(L, v);
	return 1;
}

int l_node_y(lua_State *L)
{
	Path *p = path(L);
	float v = 0;
	if (p)
	{
		v = p->nodes[0].position.y;
	}
	lua_pushnumber(L, v);
	return 1;
}

int l_entity_isName(lua_State *L)
{
	Entity *e = entity(L);
	std::string s = getString(L, 2);
	bool ret = false;
	if (e)
	{
		ret = (nocasecmp(s,e->name)==0);
	}
	lua_pushboolean(L, ret);
	return 1;
}


int l_entity_getName(lua_State *L)
{
	Entity *e = entity(L);
	std::string s;
	if (e)
	{
		s = e->name;
	}
	lua_pushstring(L, s.c_str());
	return 1;
}

int l_node_getContent(lua_State *L)
{
	Path *p = path(L);
	std::string s;
	if (p)
	{
		s = p->content;
	}
	lua_pushstring(L, s.c_str());
	return 1;
}

int l_node_getAmount(lua_State *L)
{
	Path *p = path(L);
	float a = 0;
	if (p)
	{
		a = p->amount;
	}
	lua_pushnumber(L, a);
	return 1;
}

int l_node_getSize(lua_State *L)
{
	Path *p = path(L);
	int w=0,h=0;
	if (p)
	{
		w = p->rect.getWidth();
		h = p->rect.getHeight();
	}
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	return 2;
}

int l_node_getName(lua_State *L)
{
	Path *p = path(L);
	std::string s;
	if (p)
	{
		s = p->name;
	}
	lua_pushstring(L, s.c_str());
	return 1;
}

int l_node_getPathPosition(lua_State *L)
{
	Path *p = path(L);
	int idx = lua_tonumber(L, 2);
	float x=0,y=0;
	if (p)
	{
		PathNode *node = p->getPathNode(idx);
		if (node)
		{
			x = node->position.x;
			y = node->position.y;
		}
	}
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}

int l_node_getPosition(lua_State *L)
{
	Path *p = path(L);
	float x=0,y=0;
	if (p)
	{
		PathNode *node = &p->nodes[0];
		x = node->position.x;
		y = node->position.y;
	}
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}

int l_node_setPosition(lua_State *L)
{
	Path *p = path(L);
	float x=0,y=0;
	if (p)
	{
		x = lua_tonumber(L, 2);
		y = lua_tonumber(L, 3);
		PathNode *node = &p->nodes[0];
		node->position = Vector(x, y);
	}
	lua_pushnumber(L, 0);
	return 1;
}


int l_registerSporeDrop(lua_State *L)
{
	float x, y;
	int t=0;
	x = lua_tonumber(L, 1);
	y = lua_tonumber(L, 2);
	t = lua_tointeger(L, 3);

	dsq->game->registerSporeDrop(Vector(x,y), t);

	lua_pushnumber(L, 0);
	return 1;
}

int l_setStringFlag(lua_State *L)
{
	std::string n = getString(L, 1);
	std::string v = getString(L, 2);
	dsq->continuity.setStringFlag(n, v);
	lua_pushinteger(L, 0);
	return 1;
}

int l_centerText(lua_State *L)
{
	dsq->centerText(getString(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_msg(lua_State* L)
{
	std::string s(lua_tostring(L, 1));
	dsq->screenMessage(s);
	lua_pushnumber(L, 0);
	return 1;
}

int l_chance(lua_State *L)
{
	int r = rand()%100;
	int c = lua_tointeger(L, 1);
	if (c == 0)
		lua_pushboolean(L, false);
	else
	{
		if (r <= c || c==100)
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);
	}
	return 1;
}

int l_entity_handleShotCollisions(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		dsq->game->handleShotCollisions(e);
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_handleShotCollisionsSkeletal(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		dsq->game->handleShotCollisionsSkeletal(e);
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_handleShotCollisionsHair(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		dsq->game->handleShotCollisionsHair(e, lua_tonumber(L, 2));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_collideSkeletalVsCircle(lua_State *L)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L,2);
	Bone *b = 0;
	if (e && e2)
	{
		b = dsq->game->collideSkeletalVsCircle(e,e2);
	}
	luaPushPointer(L, b);
	return 1;
}

int l_entity_collideSkeletalVsLine(lua_State *L)
{
	Entity *e = entity(L);
	int x1, y1, x2, y2, sz;
	x1 = lua_tonumber(L, 2);
	y1 = lua_tonumber(L, 3);
	x2 = lua_tonumber(L, 4);
	y2 = lua_tonumber(L, 5);
	sz = lua_tonumber(L, 6);
	Bone *b = 0;
	if (e)
	{
		b = dsq->game->collideSkeletalVsLine(e, Vector(x1, y1), Vector(x2, y2), sz);
	}
	luaPushPointer(L, b);
	return 1;
}

int l_entity_collideCircleVsLine(lua_State *L)
{
	Entity *e = entity(L);
	int x1, y1, x2, y2, sz;
	x1 = lua_tonumber(L, 2);
	y1 = lua_tonumber(L, 3);
	x2 = lua_tonumber(L, 4);
	y2 = lua_tonumber(L, 5);
	sz = lua_tonumber(L, 6);
	bool v = false;
	if (e)
		v = dsq->game->collideCircleVsLine(e, Vector(x1, y1), Vector(x2, y2), sz);
	lua_pushboolean(L, v);
	return 1;
}


int l_entity_collideCircleVsLineAngle(lua_State *L)
{
	Entity *e = entity(L);
	float angle = lua_tonumber(L, 2);
	int start=lua_tonumber(L, 3), end=lua_tonumber(L, 4), radius=lua_tonumber(L, 5);
	int x=lua_tonumber(L, 6);
	int y=lua_tonumber(L, 7);
	bool v = false;
	if (e)
		v = dsq->game->collideCircleVsLineAngle(e, angle, start, end, radius, Vector(x,y));
	lua_pushboolean(L, v);
	return 1;
}


int l_entity_collideHairVsCircle(lua_State *L)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	bool col=false;
	if (e && e2)
	{
		int num = lua_tonumber(L, 3);
		// perc: percent of hairWidth to use as collide radius
		float perc = lua_tonumber(L, 4);
		col = dsq->game->collideHairVsCircle(e, num, e2->position, e2->collideRadius, perc);
	}
	lua_pushboolean(L, col);
	return 1;
}

int l_entity_collideSkeletalVsCircleForListByName(lua_State *L)
{
	Entity *e = entity(L);
	std::string name;
	if (lua_isstring(L, 2))
		name = lua_tostring(L, 2);
	if (e && !name.empty())
	{
		FOR_ENTITIES(i)
		{
			Entity *e2 = *i;
			if (e2->life == 1 && e2->name == name)
			{
				Bone *b = dsq->game->collideSkeletalVsCircle(e, e2);
				if (b)
				{
					DamageData d;
					d.attacker = e2;
					d.bone = b;
					e->damage(d);
				}
			}
		}
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_debugText(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		BitmapText *f = new BitmapText(&dsq->smallFont);
		f->setText(lua_tostring(L, 2));
		f->position = e->position;
		core->getTopStateData()->addRenderObject(f, LR_DEBUG_TEXT);
		f->setLife(5);
		f->setDecayRate(1);
		f->fadeAlphaWithLife=1;
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_getHealth(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		lua_pushnumber(L, e->health);
	else
		lua_pushnumber(L, 0);
	return 1;
}

int l_entity_initSegments(lua_State* L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->initSegments(lua_tointeger(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tostring(L, 5), lua_tostring(L, 6), lua_tointeger(L, 7), lua_tointeger(L, 8), lua_tonumber(L, 9), lua_tointeger(L, 10));

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_warpSegments(lua_State* L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
		se->warpSegments();

	lua_pushnumber(L, 0);
	return 1;
}

//entity_incrTargetLeaches
int l_entity_incrTargetLeaches(lua_State* L)
{
	Entity *e = entity(L);
	if (e && e->getTargetEntity())
		e->getTargetEntity()->leaches++;
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_decrTargetLeaches(lua_State* L)
{
	Entity *e = entity(L);
	if (e && e->getTargetEntity())
		e->getTargetEntity()->leaches--;
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_rotateToVel(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
	{
		if (!e->vel.isZero())
		{
			e->rotateToVec(e->vel, lua_tonumber(L, 2), lua_tointeger(L, 3));
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_rotateToEntity(lua_State* L)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);

	if (e && e2)
	{
		Vector vec = e2->position - e->position;
		if (!vec.isZero())
		{
			e->rotateToVec(vec, lua_tonumber(L, 3), lua_tointeger(L, 4));
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_rotateToVec(lua_State* L)
{
	Entity *e = entity(L);
	Vector vec(lua_tonumber(L, 2), lua_tonumber(L, 3));
	if (e)
	{
		if (!vec.isZero())
		{
			e->rotateToVec(vec, lua_tonumber(L, 4), lua_tointeger(L, 5));
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_update(lua_State *L)
{
	entity(L)->update(lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_updateSkeletal(lua_State *L)
{
	entity(L)->skeletalSprite.update(lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_msg(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->message(getString(L, 2), lua_tonumber(L, 3));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_updateCurrents(lua_State *L)
{
	lua_pushboolean(L, entity(L)->updateCurrents(lua_tonumber(L, 2)));
	return 1;
}

int l_entity_updateLocalWarpAreas(lua_State *L)
{
	lua_pushboolean(L, entity(L)->updateLocalWarpAreas(getBool(L, 2)));
	return 1;
}

int l_entity_updateMovement(lua_State* L)
{
	scriptedEntity(L)->updateMovement(lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_applySurfaceNormalForce(lua_State *L)
{
	//Entity *e = si->getCurrentEntity();
	Entity *e = entity(L);
	if (e)
	{
		Vector v;
		if (!e->ridingOnEntity)
		{
			v = dsq->game->getWallNormal(e->position, 8);
		}
		else
		{
			v = e->position - e->ridingOnEntity->position;
			e->ridingOnEntity = 0;
		}
		v.setLength2D(lua_tointeger(L, 2));
		e->vel += v;
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_applyRandomForce(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
	{
		Vector f;
		f.x = ((rand()%1000)-500)/500.0f;
		f.y = ((rand()%1000)-500)/500.0f;
		f.setLength2D(lua_tonumber(L, 1));
		e->vel += f;
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_getRotation(lua_State *L)
{
	Entity *e = entity(L);
	float v = 0;
	if (e)
	{
		v = e->rotation.z;
	}
	lua_pushnumber(L, v);
	return 1;
}

int l_flingMonkey(lua_State *L)
{
	Entity *e = entity(L);

	dsq->continuity.flingMonkey(e);

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getDistanceToTarget(lua_State *L)
{
	Entity *e = entity(L);
	float dist = 0;
	if (e)
	{
		Entity *t = e->getTargetEntity();
		if (t)
		{
			dist = (t->position - e->position).getLength2D();
		}
	}
	lua_pushnumber(L, dist);
	return 1;
}

int l_entity_watchEntity(lua_State *L)
{
	Entity *e = entity(L);
	Entity *e2 = 0;
	if (lua_touserdata(L, 2) != NULL)
		e2 = entity(L, 2);

	if (e)
	{
		e->watchEntity(e2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_setNaijaHeadTexture(lua_State *L)
{
	Avatar *a = dsq->game->avatar;
	if (a)
	{
		a->setHeadTexture(lua_tostring(L, 1));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_flipToSame(lua_State *L)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	if (e && e2)
	{
		if ((e->isfh() && !e2->isfh())
			|| (!e->isfh() && e2->isfh()))
		{
			e->flipHorizontal();
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_flipToEntity(lua_State *L)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	if (e && e2)
	{
		e->flipToTarget(e2->position);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_flipToNode(lua_State *L)
{
	Entity *e = entity(L);
	Path *p = path(L, 2);
	PathNode *n = &p->nodes[0];
	if (e && n)
	{
		e->flipToTarget(n->position);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_flipToVel(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->flipToVel();
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_node_isEntityIn(lua_State *L)
{
	Path *p = path(L,1);
	Entity *e = entity(L,2);

	bool v = false;
	if (e && p)
	{
		if (!p->nodes.empty())
		{
			v = p->isCoordinateInside(e->position);
			//(e->position - p->nodes[0].position);
		}
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_node_isPositionIn(lua_State *L)
{
	Path *p = path(L,1);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);

	bool v = false;
	if (p)
	{
		if (!p->nodes.empty())
		{
			v = p->rect.isCoordinateInside(Vector(x,y) - p->nodes[0].position);
		}
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_isInDarkness(lua_State *L)
{
	Entity *e = entity(L);
	bool d = false;
	if (e)
	{
		d = e->isInDarkness();
	}
	lua_pushboolean(L, d);
	return 1;
}

int l_entity_isInRect(lua_State *L)
{
	Entity *e = entity(L);
	bool v= false;
	int x1, y1, x2, y2;
	x1 = lua_tonumber(L, 2);
	y1 = lua_tonumber(L, 3);
	x2 = lua_tonumber(L, 4);
	y2 = lua_tonumber(L, 5);
	if (e)
	{
		if (e->position.x > x1 && e->position.x < x2)
		{
			if (e->position.y > y1 && e->position.y < y2)
			{
				v = true;
			}
		}
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_isFlippedHorizontal(lua_State *L)
{
	Entity *e = entity(L);
	bool v=false;
	if (e)
	{
		v = e->isfh();
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_isFlippedVertical(lua_State *L)
{
	Entity *e = entity(L);
	bool v=false;
	if (e)
	{
		v = e->isfv();
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_flipHorizontal(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->flipHorizontal();
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_fhTo(lua_State* L)
{
	Entity *e = entity(L);
	bool b = getBool(L);
	if (e)
		e->fhTo(b);
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_flipVertical(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->flipVertical();
	lua_pushnumber(L, 0);
	return 1;
}

PauseQuad *getPauseQuad(lua_State *L, int n=1)
{
	PauseQuad *q = (PauseQuad*)lua_touserdata(L, n);
	if (q)
		return q;
	else
		errorLog("Invalid PauseQuad/Particle");
	return 0;
}

int l_createQuad(lua_State *L)
{
	PauseQuad *q = new PauseQuad();
	q->setTexture(getString(L, 1));
	int layer = lua_tonumber(L, 2);
	if (layer == 13)
		layer = 13;
	else
		layer = (LR_PARTICLES+1) - LR_ELEMENTS1;
	dsq->game->addRenderObject(q, LR_ELEMENTS1+(layer-1));

	luaPushPointer(L, q);
	return 1;
}

int l_quad_scale(lua_State *L)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
		q->scale.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	lua_pushinteger(L, 0);
	return 1;
}

int l_quad_rotate(lua_State *L)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
		q->rotation.interpolateTo(Vector(0,0,lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tointeger(L, 4));
	lua_pushnumber(L, 0);
	return 1;
}

int l_quad_color(lua_State *L)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
	{
		//e->color = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
		q->color.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4)), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_quad_alpha(lua_State *L)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
		q->alpha.interpolateTo(Vector(lua_tonumber(L, 2),0,0), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	lua_pushinteger(L, 0);
	return 1;
}

int l_quad_alphaMod(lua_State *L)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
		q->alphaMod = lua_tonumber(L, 2);
	lua_pushinteger(L, 0);
	return 1;
}

int l_quad_getAlpha(lua_State *L)
{
	PauseQuad *q = getPauseQuad(L);
	float v = 0;
	if (q)
		v = q->alpha.x;
	lua_pushnumber(L, v);
	return 1;
}


int l_quad_delete(lua_State *L)
{
	PauseQuad *q = getPauseQuad(L);
	float t = lua_tonumber(L, 2);
	if (q)
	{
		if (t == 0)
			q->safeKill();
		else
		{
			q->setLife(1);
			q->setDecayRate(1.0/t);
			q->fadeAlphaWithLife = 1;
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}


int l_quad_setBlendType(lua_State *L)
{
	PauseQuad *q = getPauseQuad(L);
	if (q)
	{
		q->setBlendType(lua_tonumber(L, 2));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_quad_setPosition(lua_State *L)
{
	PauseQuad *q = getPauseQuad(L);
	float x = lua_tonumber(L, 2);
	float y = lua_tonumber(L, 3);
	if (q)
		q->position.interpolateTo(Vector(x,y), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	lua_pushnumber(L, 0);
	return 1;
}

int l_setupConversationEntity(lua_State* L)
{
	std::string name, gfx;
	if (lua_isstring(L, 2))
		name = lua_tostring(L, 2);
	if (lua_isstring(L, 3))
		gfx = lua_tostring(L, 3);
	scriptedEntity(L)->setupConversationEntity(name, gfx);
	lua_pushnumber(L, 0);
	return 1;
}

int l_setupEntity(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		std::string tex;
		if (lua_isstring(L, 2))
		{
			tex = lua_tostring(L, 2);
		}
		se->setupEntity(tex, lua_tonumber(L, 3));
	}
	return 1;
}

int l_setupBasicEntity(lua_State* L)
{
	//ScriptedEntity *se = dynamic_cast<ScriptedEntity*>(si->getCurrentEntity());
	ScriptedEntity *se = scriptedEntity(L);
	//-- texture, health, manaballamount, exp, money, collideRadius, initState
	if (se)
		se->setupBasicEntity(lua_tostring(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6), lua_tointeger(L, 7), lua_tointeger(L, 8), lua_tointeger(L, 9), lua_tointeger(L, 10), lua_tointeger(L, 11), lua_tointeger(L, 12), lua_tointeger(L, 13), lua_tointeger(L, 14));

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setBeautyFlip(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->beautyFlip = getBool(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_setInvincible(lua_State *L)
{
	dsq->game->invinciblity = getBool(L, 1);

	lua_pushboolean(L, dsq->game->invinciblity);
	return 1;
}

int l_entity_setInvincible(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setInvincible(getBool(L, 2));

	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setDeathSound(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->deathSound = lua_tostring(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setDeathParticleEffect(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->deathParticleEffect = getString(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

/*
int l_entity(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->deathParticleEffect = lua_tostring(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}
*/

int l_entity_setNaijaReaction(lua_State *L)
{
	Entity *e = entity(L);
	std::string s;
	if (lua_isstring(L, 2))
		s = lua_tostring(L, 2);
	if (e)
	{
		e->naijaReaction = s;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setName(lua_State *L)
{
	Entity *e = entity(L);
	std::string s;
	if (lua_isstring(L, 2))
		s = lua_tostring(L, 2);
	if (e)
	{
		debugLog("setting entity name to: " + s);
		e->setName(s);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_pathBurst(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
		v = e->pathBurst(lua_tointeger(L, 2));
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_moveTowardsAngle(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveTowardsAngle(lua_tointeger(L, 2), lua_tonumber(L, 3), lua_tointeger(L, 4));
	}
	lua_tonumber(L, 0);
	return 1;
}

int l_entity_moveAroundAngle(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveTowardsAngle(lua_tointeger(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	}
	lua_tonumber(L, 0);
	return 1;
}

int l_entity_moveTowards(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveTowards(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5));
	}
	lua_tonumber(L, 0);
	return 1;
}

int l_entity_moveAround(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveAround(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	lua_tonumber(L, 0);
	return 1;
}

int l_entity_addVel(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->vel += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_addVel2(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->vel2 += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_addRandomVel(lua_State *L)
{
	Entity *e = entity(L);
	int len = lua_tonumber(L, 2);
	if (e && len)
	{
		int angle = int(rand()%360);
		float a = MathFunctions::toRadians(angle);
		Vector add(sinf(a), cosf(a));
		add.setLength2D(len);
		e->vel += add;
		//e->vel += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_addGroupVel(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		FOR_ENTITIES(i)
		{
			Entity *e2 = *i;
			if (e2->getGroupID() == e->getGroupID())
				e2->vel += Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
		}
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_isValidTarget(lua_State *L)
{
	Entity *e = entity(L);
	Entity *e2 = 0;
	if (lua_tonumber(L, 2)!=0)
		e2 = entity(L);
	bool b = false;
	if (e)
		b = dsq->game->isValidTarget(e, e2);
	lua_pushboolean(L, b);
	return 1;
}

int l_entity_isVelIn(lua_State *L)
{
	Entity *e = entity(L);
	bool b = false;
	if (e)
	{
		b = e->vel.isLength2DIn(lua_tonumber(L, 2));
	}
	lua_pushboolean(L, b);
	return 1;
}

int l_entity_getVelLen(lua_State *L)
{
	Entity *e = entity(L);
	int l = 0;
	if (e)
		l = e->vel.getLength2D();
	lua_pushnumber(L, l);
	return 1;
}

int l_entity_velx(lua_State *L)
{
	Entity *e = entity(L);
	float velx = 0;
	if (e)
	{
		velx = e->vel.x;
	}
	lua_pushnumber(L, velx);
	return 1;
}

int l_entity_vely(lua_State *L)
{
	Entity *e = entity(L);
	float vely = 0;
	if (e)
	{
		vely = e->vel.y;
	}
	lua_pushnumber(L, vely);
	return 1;
}

int l_entity_clearVel(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->vel = Vector(0,0,0);
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_clearVel2(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->vel2 = Vector(0,0,0);
	lua_pushnumber(L, 0);
	return 1;
}

int l_getScreenCenter(lua_State *L)
{
	lua_pushnumber(L, core->screenCenter.x);
	lua_pushnumber(L, core->screenCenter.y);
	return 2;
}

int l_getNodeFromEntity(lua_State *L)
{
	Path *nodePtr = 0;
	Entity *e = entity(L);
	if (e)
	{
		nodePtr = e->getNode();
	}
	luaPushPointer(L, nodePtr);
	return 1;
}

int l_entity_rotate(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->rotation.interpolateTo(Vector(0,0,lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tointeger(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_rotateOffset(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->rotationOffset.interpolateTo(Vector(0,0,lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tointeger(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_isState(lua_State *L)
{
	Entity *e = entity(L);
	bool v=false;
	if (e)
	{
		v = (e->getState() == lua_tointeger(L, 2));
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_getState(lua_State *L)
{
	Entity *e = entity(L);
	int state = 0;
	if (e)
		state = e->getState();
	lua_pushnumber(L, state);
	return 1;
}

int l_entity_getEnqueuedState(lua_State *L)
{
	Entity *e = entity(L);
	int state = 0;
	if (e)
		state = e->getEnqueuedState();
	lua_pushnumber(L, state);
	return 1;
}

int l_entity_getPrevState(lua_State *L)
{
	Entity *e = entity(L);
	int state = 0;
	if (e)
		state = e->getPrevState();
	lua_pushnumber(L, state);
	return 1;
}

int l_entity_setTarget(lua_State *L)
{
	Entity *e = entity(L);
	Entity *t = 0;
	int tv=0;
	if (lua_touserdata(L, 2) != NULL)
	{
		t = entity(L, 2);
	}
	if (e)
	{
		e->setTargetEntity(t);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setBounce(lua_State* L)
{
	CollideEntity *e = collideEntity(L);
	if (e)
		e->bounceAmount = e->bounceEntityAmount = lua_tonumber(L, 2);
	lua_pushinteger(L, 0);
	return 1;
}

int l_avatar_isSinging(lua_State *L)
{
	bool b = dsq->game->avatar->isSinging();
	lua_pushboolean(L, b);
	return 1;
}

int l_avatar_isTouchHit(lua_State *L)
{
	//avatar_canBeTouchHit
	bool b = !(dsq->game->avatar->bursting && dsq->continuity.form == FORM_BEAST);
	lua_pushboolean(L, b);
	return 1;
}

int l_avatar_clampPosition(lua_State *L)
{
	dsq->game->avatar->clampPosition();
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_setPosition(lua_State* L)
{
	Entity *e = entity(L);
	float t = 0;
	if (e)
	{
		t = e->position.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	lua_pushnumber(L, t);
	return 1;
}

int l_entity_setInternalOffset(lua_State* L)
{
	Entity *e = entity(L);
	float t = 0;
	if (e)
	{
		t = e->internalOffset.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	}
	lua_pushnumber(L, t);
	return 1;
}

int l_entity_setTexture(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		std::string s = lua_tostring(L, 2);
		e->setTexture(s);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setMaxSpeed(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->setMaxSpeed(lua_tointeger(L, 2));

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getMaxSpeed(lua_State* L)
{
	Entity *e = entity(L);
	int v = 0;
	if (e)
		v = e->getMaxSpeed();

	lua_pushnumber(L, v);
	return 1;
}

int l_entity_setMaxSpeedLerp(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->maxSpeedLerp.interpolateTo(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));

	lua_pushnumber(L, 0);
	return 1;
}

// note: this is a weaker setState than perform
// this is so that things can override it
// for example getting PUSH-ed (Force) or FROZEN (bubbled)
int l_entity_setState(lua_State *L)
{
	Entity *me = entity(L);
	if (me)
	{
		/*
		if (!me->isDead())
		{
		*/
		int state = lua_tointeger(L, 2);
		float time = lua_tonumber(L, 3);
		if (time == 0)
			time = -1;
		bool force = getBool(L, 4);
		//if (me->getEnqueuedState() == StateMachine::STATE_NONE)
		me->setState(state, time, force);
		//}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_fireAtTarget(lua_State* L)
{
	/*
	Entity *e = entity(L);
	Shot *shot = 0;
	if (e)
	{
		if (!e->getTargetEntity())
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		std::string pe = lua_tostring(L, 2);
		float damage = lua_tonumber(L, 3);
		int maxSpeed = lua_tointeger(L, 4);
		int homingness = lua_tointeger(L, 5);
		int numSegs = lua_tointeger(L, 6);
		int out = lua_tointeger(L, 7);
		int target = e->currentEntityTarget;
		float offx = lua_tonumber(L, 8);
		float offy = lua_tonumber(L, 9);
		float velx = lua_tonumber(L, 10);
		float vely = lua_tonumber(L, 11);
		float x = lua_tonumber(L, 12);
		float y = lua_tonumber(L, 13);
		Vector off(offx, offy);
		//x' = cos(theta)*x - sin(theta)*y
		//y' = sin(theta)*x + cos(theta)*y

		if (!off.isZero())
			off.rotate2D(e->getAbsoluteRotation().z);


		Vector pos;
		if (x == 0 && y == 0)
			pos = e->position + off;
		else
			pos = Vector(x,y) + off;
		std::string tex;
		if (e->getEntityType() == ET_PET)
			tex="shot-green";
		//new Shot(
		//new Shot(pos, 0, texture, homingness, maxSpeed, segments, segmin, segmax, damage);
		DamageType dt;
		if (e->getEntityType() == ET_ENEMY)
			dt = DT_ENEMY_ENERGYBLAST;
		else if (e->getEntityType() == ET_AVATAR)
			dt = DT_AVATAR_ENERGYBLAST;
		else
			errorLog("undefined shot type in entity_fireAtTarget");
		shot = new Shot(dt, e, pos, e->getTargetEntity(target),
			tex, homingness, maxSpeed, numSegs, 0.1, 5, damage);
		shot->setParticleEffect(pe);
		if (velx != 0 || vely != 0)
		{
			shot->velocity = Vector(velx, vely);
		}
		else
		{
			shot->velocity = e->getTargetEntity()->position - pos;
		}
		shot->velocity.setLength2D(maxSpeed);
		if (out > 0)
		{
			Vector mov = shot->velocity;
			mov.setLength2D(out);
			shot->position += mov;
		}
		dsq->game->addRenderObject(shot, LR_PROJECTILES);
	}
		//addRenderObject(shot);
		luaPushPointer(L, shot);
	*/

	{
		debugLog("entire_fireAtTarget is deprecated");
		lua_pushnumber(L, 0);
	}


	return 1;
}

int l_entity_getBoneByIdx(lua_State *L)
{
	Entity *e = entity(L);
	Bone *b = 0;
	if (e)
	{
		int n = 0;
		if (lua_isnumber(L, 2))
		{
			n = lua_tonumber(L, 2);
			b = e->skeletalSprite.getBoneByIdx(n);
		}
	}
	luaPushPointer(L, b);
	return 1;
}

int l_entity_getBoneByName(lua_State *L)
{
	Entity *e = entity(L);
	Bone *b = 0;
	if (e)
	{
		b = e->skeletalSprite.getBoneByName(lua_tostring(L, 2));
	}
	/*
	if (e)
	{
		int n = 0;
		if (lua_isnumber(L, 2))
		{
			n = lua_tonumber(L, 2);
			b = e->skeletalSprite.getBoneByIdx(n);
		}
	}
	*/
	luaPushPointer(L, b);
	return 1;
}

// ditch entity::sound and use this code instead...
// replace entity sound with this code

int l_entity_playSfx(lua_State *L)
{
	Entity *e= entity(L);
	if (e)
	{
		std::string sfx = lua_tostring(L, 2);


		/*
		int f = rand()%200-100;
		f += 1000;
		*/
		dsq->playPositionalSfx(sfx, e->position);
		/*
		Vector diff = e->position - dsq->game->avatar->position;
		if (diff.isLength2DIn(800))
		{
			int dist = diff.getLength2D();
			int vol = 255 - int((dist*255.0) / 1500.0);
			int pan = (diff.x*100)/800.0;
			if (pan < -100)
				pan = -100;
			if (pan > 100)
				pan = 100;

			std::ostringstream os;
			os << "vol: " << vol << " pan: " << pan;
			debugLog(os.str());


			sound->playSfx(sfx, vol, pan, 1000+f);
		}
		*/

		//sound->playSfx(sfx);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_getPosition(lua_State *L)
{
	Bone *b = bone(L);
	int x=0,y=0;
	if (b)
	{
		Vector pos = b->getWorldPosition();
		x = pos.x;
		y = pos.y;
	}
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}

int l_bone_getScale(lua_State *L)
{
	Bone *b = bone(L);
	float x=0,y=0;
	if (b)
	{
		Vector sc = b->scale;
		x = sc.x;
		y = sc.y;
	}
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}


int l_bone_getNormal(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
	{
		Vector n = b->getForward();
		lua_pushnumber(L, n.x);
		lua_pushnumber(L, n.y);
	}
	else
	{
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);
	}
	return 2;
}

int l_bone_damageFlash(lua_State *L)
{
	Bone *b = bone(L);
	int type = lua_tonumber(L, 2);
	if (b)
	{
		Vector toColor = Vector(1, 0.1, 0.1);
		if (type == 1)
		{
			toColor = Vector(1, 1, 0.1);
		}
		b->color = Vector(1,1,1);
		b->color.interpolateTo(toColor, 0.1, 5, 1);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_isVisible(lua_State *L)
{
	bool ret = false;
	Bone *b = bone(L);
	if (b)
		ret = b->renderQuad;
	lua_pushboolean(L, ret);
	return 1;
}

int l_bone_setVisible(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
		b->renderQuad = getBool(L, 2);
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_setTexture(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
	{
		b->setTexture(lua_tostring(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_setTouchDamage(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
	{
		b->touchDamage = lua_tonumber(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_getidx(lua_State *L)
{
	Bone *b = bone(L);
	int idx = -1;
	if (b)
		idx = b->boneIdx;
	lua_pushnumber(L, idx);
	return 1;
}

int l_bone_getName(lua_State *L)
{
	std::string n;
	Bone *b = bone(L);
	if (b)
	{
		n = b->name;
	}
	lua_pushstring(L, n.c_str());
	return 1;
}

int l_bone_isName(lua_State *L)
{
	Bone *b = bone(L);
	bool v = false;
	if (b)
	{
		v = b->name == lua_tostring(L, 2);
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_overrideZoom(lua_State *L)
{
	dsq->game->overrideZoom(lua_tonumber(L, 1), lua_tonumber(L, 2));

	lua_pushnumber(L, 0);
	return 1;
}

int l_disableOverrideZoom(lua_State *L)
{
	dsq->game->toggleOverrideZoom(false);
	lua_pushnumber(L, 0);
	return 1;
}

// dt, range, mod
int l_entity_doSpellAvoidance(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->doSpellAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4));
	lua_pushnumber(L, 0);
	return 1;
}

// dt, range, mod, ignore
int l_entity_doEntityAvoidance(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->doEntityAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), e->getTargetEntity());
	lua_pushnumber(L, 0);
	return 1;
}

// doCollisionAvoidance(me, dt, search, mod)
int l_entity_doCollisionAvoidance(lua_State* L)
{
	Entity *e = entity(L);
	bool ret = false;

	int useVel2 = lua_tonumber(L, 6);
	bool onlyVP = getBool(L, 7);

	if (e)
	{
		if (useVel2)
			ret = e->doCollisionAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), &e->vel2, lua_tonumber(L, 5), onlyVP);
		else
			ret = e->doCollisionAvoidance(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tonumber(L, 4), 0, lua_tonumber(L, 5));
	}
	lua_pushboolean(L, ret);
	return 1;
}

int l_setOverrideMusic(lua_State *L)
{
	dsq->game->overrideMusic = getString(L, 1);
	lua_pushnumber(L, 0);
	return 1;
}

int l_setOverrideVoiceFader(lua_State *L)
{
	dsq->sound->setOverrideVoiceFader(lua_tonumber(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_setGameSpeed(lua_State *L)
{
	dsq->gameSpeed.stop();
	dsq->gameSpeed.stopPath();
	dsq->gameSpeed.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	lua_pushnumber(L, 0);
	return 1;
}

int l_sendEntityMessage(lua_State* L)
{
	Entity *e = dsq->getEntityByName(lua_tostring(L, 1));
	if (e)
		e->onMessage(lua_tostring(L, 2));

	lua_pushnumber(L, 0);
	return 1;
}

int l_bedEffects(lua_State* L)
{
	dsq->overlay->alpha.interpolateTo(1, 2);
	dsq->sound->fadeMusic(SFT_OUT, 1);
	core->main(1);
	// music goes here
	dsq->sound->fadeMusic(SFT_CROSS, 1);
	dsq->sound->playMusic("Sleep");
	core->main(6);
	Vector bedPosition(lua_tointeger(L, 1), lua_tointeger(L, 2));
	if (bedPosition.x == 0 && bedPosition.y == 0)
	{
		bedPosition = dsq->game->avatar->position;
	}
	dsq->game->positionToAvatar = bedPosition;
	dsq->game->transitionToScene(dsq->game->sceneName);

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setDeathScene(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDeathScene(getBool(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setPauseInConversation(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setPauseInConversation(lua_toboolean(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setCollideWithAvatar(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->collideWithAvatar = getBool(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setCurrentTarget(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->currentEntityTarget = lua_tointeger(L, 2);
	lua_pushinteger(L, 0);
	return 1;
}

int l_setMiniMapHint(lua_State *L)
{
	std::istringstream is(lua_tostring(L, 1));
	is >> dsq->game->miniMapHint.scene >> dsq->game->miniMapHint.warpAreaType;
	dsq->game->updateMiniMapHintPosition();

	lua_pushnumber(L, 0);
	return 1;
}

int l_entityFollowEntity(lua_State* L)
{
	Entity *e = dsq->getEntityByName(lua_tostring(L, 1));
	if (e)
	{
		e->followEntity = dsq->getEntityByName(lua_tostring(L, 2));
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_isFollowingEntity(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
		v = e->followEntity != 0;
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_followEntity(lua_State* L)
{
	Entity *e1 = entity(L);
	Entity *e2 = 0;
	if (lua_touserdata(L, 2) != NULL)
	{
		e2 = entity(L, 2);
	}
	if (e1)
	{
		e1->followEntity = e2;
		e1->followPos = lua_tointeger(L, 3);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_setEntityScript(lua_State* L)
{
	errorLog ("setentityScript is deprecated");
	if (false)
	{
		/*
		Entity *e = dsq->getEntityByName(lua_tostring(L, 1));
		if (e)
		{
			e->convo = lua_tostring(L, 2);
		}
		*/
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_toggleInput(lua_State *L)
{
	int v = lua_tointeger(L, 1);
	if (v)
		dsq->game->avatar->enableInput();
	else
		dsq->game->avatar->disableInput();
	lua_pushnumber(L, 0);
	return 1;
}

int l_toggleTransitFishRide(lua_State* L)
{
	Entity *t = entity(L);
	if (t)
	{
		if (!dsq->game->avatar->attachedTo)
			t->attachEntity(dsq->game->avatar, Vector(0,0));
		else
		{
			//TransitFish *t = dynamic_cast<TransitFish*>(dsq->game->avatar->attachedTo);
			Entity *t = dsq->game->avatar->attachedTo;
			if (t)
				t->detachEntity(dsq->game->avatar);
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_offset(lua_State *L)
{
	Bone *b = bone(L);
	if (b)
		b->offset.interpolateTo(Vector(lua_tointeger(L, 2), lua_tointeger(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_offset(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->offset.interpolateTo(Vector(lua_tointeger(L, 2), lua_tointeger(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	lua_pushnumber(L, 0);
	return 1;
}

int l_warpAvatar(lua_State* L)
{
	dsq->game->positionToAvatar = Vector(lua_tointeger(L, 2),lua_tointeger(L, 3));
	dsq->game->transitionToScene(lua_tostring(L, 1));

	lua_pushnumber(L, 0);
	return 1;
}

int l_warpNaijaToSceneNode(lua_State* L)
{
	std::string scene = getString(L, 1);
	std::string node = getString(L, 2);
	std::string flip = getString(L, 3);
	if (!scene.empty() && !node.empty())
	{
		dsq->game->toNode = node;
		stringToLower(flip);
		if (flip == "l")
			dsq->game->toFlip = 0;
		if (flip == "r")
			dsq->game->toFlip = 1;
		dsq->game->transitionToScene(scene);
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_registerSporeChildData(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		dsq->continuity.registerSporeChildData(e);
	}
	lua_pushnumber(L, 0);
	return 1;
 }

int l_streamSfx(lua_State *L)
{
	//core->sound->streamSfx(lua_tostring(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setDamageTarget(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setDamageTarget((DamageType)lua_tointeger(L, 2), getBool(L, 3));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setAllDamageTargets(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setAllDamageTargets(getBool(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_isDamageTarget(lua_State *L)
{
	Entity *e = entity(L);
	bool v=false;
	if (e)
	{
		v = e->isDamageTarget((DamageType)lua_tointeger(L, 2));
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_setEnergyShotTarget(lua_State *L)
{
	/*
	Entity *e = entity(L);
	if (e)
	{
		e->energyShotTarget = lua_toboolean(L, 2);
	}
	*/
	debugLog("setEnergyShotTarget antiquated");
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setTargetRange(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->targetRange = lua_tonumber(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_clearTargetPoints(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->clearTargetPoints();
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_addTargetPoint(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->addTargetPoint(Vector(lua_tonumber(L,2), lua_tonumber(L, 3)));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getTargetPoint(lua_State *L)
{
	Entity *e = entity(L);
	int idx = lua_tointeger(L, 2);
	Vector v;
	if (e)
	{
		v = e->getTargetPoint(idx);
	}
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	return 2;
}

int l_entity_getRandomTargetPoint(lua_State *L)
{
	Entity *e = entity(L);
	Vector v;
	int idx = 0;
	if (e)
	{
		idx = e->getRandomTargetPoint();
	}
	lua_pushnumber(L, idx);
	return 2;
}

int l_entity_setEnergyShotTargetPosition(lua_State *L)
{
	/*
	Entity *e = entity(L);
	if (e)
	{
		e->energyShotTargetPosition = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	*/
	errorLog("entity_setEnergyShotTargetPosition is obsolete!");
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setEnergyChargeTarget(lua_State *L)
{
	/*
	Entity *e = entity(L);
	if (e)
	{
		e->energyChargeTarget = lua_toboolean(L, 2);
	}
	*/
	debugLog("setEnergyChargeTarget antiquated");
	lua_pushnumber(L, 0);
	return 1;
}

int l_playVisualEffect(lua_State *L)
{
	dsq->playVisualEffect(lua_tonumber(L, 1), Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	lua_pushnumber(L, 0);
	return 1;
}

int l_playNoEffect(lua_State *L)
{
	dsq->playNoEffect();
	lua_pushnumber(L, 0);
	return 1;
}

int l_emote(lua_State *L)
{
	int emote = lua_tonumber(L, 1);
	dsq->emote.playSfx(emote);
	lua_pushnumber(L, 0);
	return 1;
}

int l_playSfx(lua_State *L)
{
	int freq = lua_tonumber(L, 2);
	float vol = lua_tonumber(L, 3);
	int loops = lua_tointeger(L, 4);
	if (vol == 0)
		vol = 1;

	PlaySfx sfx;
	sfx.name = getString(L, 1);
	sfx.vol = vol;
	sfx.freq = freq;
	sfx.loops = loops;

	void *handle = NULL;
	
	if (!dsq->isSkippingCutscene())
		handle = core->sound->playSfx(sfx);
	
	luaPushPointer(L, handle);
	return 1;
}

int l_fadeSfx(lua_State *L)
{
	void *header = lua_touserdata(L, 1);
	float ft = lua_tonumber(L, 2);

	core->sound->fadeSfx(header, SFT_OUT, ft);

	luaPushPointer(L, header);
	return 1;
}

int l_resetTimer(lua_State *L)
{
	dsq->resetTimer();
	lua_pushnumber(L, 0);
	return 1;
}

int l_stopMusic(lua_State *L)
{
	dsq->sound->stopMusic();
	lua_pushnumber(L, 0);
	return 1;
}

int l_playMusic(lua_State* L)
{
	float crossfadeTime = 0.8;
	dsq->sound->playMusic(std::string(lua_tostring(L, 1)), SLT_LOOP, SFT_CROSS, crossfadeTime);
	lua_pushnumber(L, 0);
	return 1;
}

int l_playMusicStraight(lua_State *L)
{
	dsq->sound->setMusicFader(1,0);
	dsq->sound->playMusic(getString(L, 1), SLT_LOOP, SFT_IN, 0.5); //SFT_IN, 0.1);//, SFT_IN, 0.2);
	lua_pushnumber(L, 0);
	return 1;
}

int l_playMusicOnce(lua_State* L)
{
	float crossfadeTime = 0.8;
	dsq->sound->playMusic(std::string(lua_tostring(L, 1)), SLT_NONE, SFT_CROSS, crossfadeTime);
	lua_pushnumber(L, 0);
	return 1;
}

int l_addInfluence(lua_State *L)
{
	ParticleInfluence pinf;
	pinf.pos.x = lua_tonumber(L, 1);
	pinf.pos.y = lua_tonumber(L, 2);
	pinf.size = lua_tonumber(L, 3);
	pinf.spd = lua_tonumber(L, 4);
	dsq->particleManager->addInfluence(pinf);
	lua_pushnumber(L, 0);
	return 1;
}

int l_updateMusic(lua_State *L)
{
	dsq->game->updateMusic();
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_grabTarget(lua_State* L)
{
	Entity *e = entity(L);
	e->attachEntity(e->getTargetEntity(), Vector(lua_tointeger(L, 2), lua_tointeger(L, 3)));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_clampToHit(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->clampToHit();
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_clampToSurface(lua_State* L)
{
	Entity *e = entity(L);
	bool ret = e->clampToSurface(lua_tonumber(L, 2));

	lua_pushboolean(L, ret);
	return 1;
}

int l_entity_checkSurface(lua_State *L)
{
	Entity *e = entity(L);
	bool c = false;

	if (e)
	{
		c = e->checkSurface(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4));
	}

	lua_pushboolean(L, c);
	return 1;
}

int l_entity_switchSurfaceDirection(lua_State* L)
{
	//ScriptedEntity *e = (ScriptedEntity*)si->getCurrentEntity();
	ScriptedEntity *e = scriptedEntity(L);
	int n = -1;
	if (lua_isnumber(L, 2))
	{
		n = lua_tonumber(L, 2);
	}

	if (e->isv(EV_SWITCHCLAMP, 1))
	{
		Vector oldPos = e->position;
		if (e->isNearObstruction(0))
		{
			Vector n = dsq->game->getWallNormal(e->position);
			if (!n.isZero())
			{
				do
				{
					e->position += n * 2;
				}
				while(e->isNearObstruction(0));
			}
		}
		Vector usePos = e->position;
		e->position = oldPos;
		e->clampToSurface(0, usePos);
	}

	if (n == -1)
	{
		if (e->surfaceMoveDir)
			e->surfaceMoveDir = 0;
		else
			e->surfaceMoveDir = 1;
	}
	else
	{
		e->surfaceMoveDir = n;
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_adjustPositionBySurfaceNormal(lua_State* L)
{
	ScriptedEntity *e = scriptedEntity(L);//(ScriptedEntity*)si->getCurrentEntity();
	if (!e->ridingOnEntity)
	{
		Vector v = dsq->game->getWallNormal(e->position);
		if (v.x != 0 || v.y != 0)
		{
			v.setLength2D(lua_tonumber(L, 2));
			e->position += v;
		}
		e->setv(EV_CRAWLING, 0);
		//e->setCrawling(false);
	}
	lua_pushinteger(L, 0);
	return 1;
}

// HACK: move functionality inside entity class
int l_entity_moveAlongSurface(lua_State* L)
{
	ScriptedEntity *e = scriptedEntity(L);//(ScriptedEntity*)si->getCurrentEntity();

	if (e->isv(EV_CLAMPING,0))
	{
		e->lastPosition = e->position;

		//if (!e->position.isInterpolating())
		{


			/*
			if (dsq->game->isObstructed(TileVector(e->position)))
			{
				e->moveOutOfWall();
			}
			*/

			Vector v;
			if (e->ridingOnEntity)
			{
				v = (e->position - e->ridingOnEntity->position);
				v.normalize2D();
			}
			else
				v = dsq->game->getWallNormal(e->position);
			//int outFromWall = lua_tonumber(L, 5);
			int outFromWall = e->getv(EV_WALLOUT);
			bool invisibleIn = e->isSittingOnInvisibleIn();

			/*
			if (invisibleIn)
				debugLog("Found invisibleIn");
			else
				debugLog("NOT FOUND");
			*/

			/*
			for (int x = -2; x < 2; x++)
			{
				for (int y = -2; y< 2; y++)
				{
					if (dsq->game->getGrid(TileVector(x,y))== OT_INVISIBLEIN)
					{
						debugLog("found invisible in");
						invisibleIn = true;
						break;
					}
				}
			}
			*/
			if (invisibleIn)
				outFromWall -= TILE_SIZE;
			float t = 0.1;
			e->offset.interpolateTo(v*outFromWall, t);
			/*
			if (outFromWall)
			{
				//e->lastWallOffset = dsq->game->getWallNormal(e->position)*outFromWall;
				//e->offset.interpolateTo(dsq->game->getWallNormal(e->position)*outFromWall, time*2);
				//e->offset = v*outFromWall;

				//float t = 0;
				e->offset.interpolateTo(v*outFromWall, t);
				//pos += e->lastWallOffset;
			}
			else
			{
				e->offset.interpolateTo(Vector(0,0), t);
				//e->offset.interpolateTo(Vector(0,0), time*2);
				//e->lastWallOffset = Vector(0,0);g
			}
			*/
			// HACK: make this an optional parameter?
			//e->rotateToVec(v, 0.1);
			float dt = lua_tonumber(L, 2);
			int speed = lua_tonumber(L, 3);
			//int climbHeight = lua_tonumber(L, 4);
			Vector mov;
			if (e->surfaceMoveDir==1)
				mov = Vector(v.y, -v.x);
			else
				mov = Vector(-v.y, v.x);
			e->position += mov * speed * dt;

			if (e->ridingOnEntity)
				e->ridingOnEntityOffset = e->position - e->ridingOnEntity->position;

			e->vel = 0;

			/*
			float adjustbit = float(speed)/float(TILE_SIZE);
			if (e->isNearObstruction(0))
			{
				Vector n = dsq->game->getWallNormal(e->position);
				if (!n.isZero())
				{
					Vector sp = e->position;
					e->position += n * adjustbit * dt;
				}
			}
			if (!e->isNearObstruction(1))
			{
				Vector n = dsq->game->getWallNormal(e->position);
				if (!n.isZero())
				{
					Vector sp = e->position;
					e->position -= n * adjustbit * dt;
				}
			}
			*/
				/*
				Vector sp = e->position;
				e->clampToSurface();
				*/
				/*
				e->position = sp;
				e->internalOffset.interpolateTo(e->position-sp, 0.2);
				*/
				/*
				e->position = e->lastPosition;
				e->position.interpolateTo(to*0.5 + e->position*0.5, 0.5);
				*/
					/*
					Vector to = e->position;
					e->position = e->lastPosition;
					e->position.interpolateTo(to, 0.5);
					*/
					/*
					e->position = sp;
					e->internalOffset.interpolateTo(e->position - sp, 0.2);
					*/
					//e->clampToSurface(0.1);
		}
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_flipHToAvatar(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->flipToTarget(dsq->game->avatar->position);

	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_rotateToSurfaceNormal(lua_State* L)
{
	//ScriptedEntity *e = scriptedEntity(L);//(ScriptedEntity*)si->getCurrentEntity();
	Entity *e = entity(L);
	float t = lua_tonumber(L, 2);
	int n = lua_tonumber(L, 3);
	int rot = lua_tonumber(L, 4);
	if (e)
	{
		e->rotateToSurfaceNormal(t, n, rot);
	}
	//Entity *e = entity(L);

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_releaseTarget(lua_State* L)
{
	Entity *e = entity(L);//si->getCurrentEntity();
	e->detachEntity(e->getTargetEntity());
	lua_pushnumber(L, 0);
	return 1;
}

int l_e_setv(lua_State *L)
{
	Entity *e = entity(L);
	EV ev = (EV)lua_tointeger(L, 2);
	int n = lua_tointeger(L, 3);
	if (e)
		e->setv(ev, n);
	lua_pushnumber(L, n);
	return 1;
}

int l_e_getv(lua_State *L)
{
	Entity *e = entity(L);
	EV ev = (EV)lua_tointeger(L, 2);
	lua_pushnumber(L, e->getv(ev));
	return 1;
}

int l_e_setvf(lua_State *L)
{
	Entity *e = entity(L);
	EV ev = (EV)lua_tointeger(L, 2);
	float n = lua_tonumber(L, 3);
	if (e)
		e->setvf(ev, n);
	lua_pushnumber(L, n);
	return 1;
}

int l_e_getvf(lua_State *L)
{
	Entity *e = entity(L);
	EV ev = (EV)lua_tointeger(L, 2);
	lua_pushnumber(L, e->getvf(ev));
	return 1;
}

int l_e_isv(lua_State *L)
{
	Entity *e = entity(L);
	EV ev = (EV)lua_tointeger(L, 2);
	int n = lua_tonumber(L, 3);
	bool b = 0;
	if (e)
		b = e->isv(ev, n);
	lua_pushboolean(L, b);
	return 1;
}

int l_entity_setClampOnSwitchDir(lua_State *L)
{
	debugLog("_setClampOnSwitchDir is old");
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setWidth(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->setWidth(lua_tonumber(L, 2));
		//e->width.interpolateTo(Vector(lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setHeight(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->setHeight(lua_tonumber(L, 2));
		//e->height.interpolateTo(Vector(lua_tonumber(L, 2)), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	lua_pushnumber(L, 0);
	return 1;
}

int l_vector_normalize(lua_State *L)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	v.normalize2D();
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	return 2;
}

int l_vector_getLength(lua_State *L)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	float len = v.getLength2D();
	lua_pushnumber(L, len);
	return 1;
}

int l_vector_setLength(lua_State *L)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	v.setLength2D(lua_tonumber(L, 3));
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	return 2;
}

int l_vector_dot(lua_State *L)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	float x2 = lua_tonumber(L, 3);
	float y2 = lua_tonumber(L, 4);
	Vector v(x,y);
	Vector v2(x2,y2);
	lua_pushnumber(L, v.dot2D(v2));
	return 1;
}

int l_vector_cap(lua_State *L)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	v.capLength2D(lua_tonumber(L, 3));
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	return 2;
}

int l_vector_isLength2DIn(lua_State *L)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	Vector v(x,y);
	bool ret = v.isLength2DIn(lua_tonumber(L, 3));
	lua_pushboolean(L, ret);
	return 1;
}

int l_entity_push(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->push(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_pushTarget(lua_State* L)
{
	//Entity *e = si->getCurrentEntity();
	Entity *e = entity(L);
	if (e)
	{
		Entity *target = e->getTargetEntity();
		if (target)
		{
			Vector diff = target->position - e->position;
			diff.setLength2D(lua_tointeger(L, 2));
			target->vel += diff;
		}
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_watch(lua_State* luaVM)
{
	float t = lua_tonumber(luaVM, 1);
	int quit = lua_tointeger(luaVM, 2);
	dsq->watch(t, quit);
	lua_pushnumber( luaVM, 0 );
	return 1;
}

int l_wait(lua_State* luaVM)
{
	core->main(lua_tonumber(luaVM, 1));
	lua_pushnumber( luaVM, 0 );
	return 1;
}

int l_healEntity(lua_State* L)
{
	Entity *e = dsq->getEntityByName(lua_tostring(L, 1));
	if (e)
	{
		e->heal(lua_tonumber(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_killEntity(lua_State* L)
{
	Entity *e = dsq->getEntityByName(lua_tostring(L, 1));
	if (e)
		e->safeKill();
	lua_pushnumber(L, 0);
	return 1;
}

int l_warpNaijaToEntity(lua_State *L)
{
	Entity *e = dsq->getEntityByName(lua_tostring(L, 1));
	if (e)
	{
		dsq->overlay->alpha.interpolateTo(1, 1);
		core->main(1);

		Vector offset(lua_tointeger(L, 2), lua_tointeger(L, 3));
		dsq->game->avatar->position = e->position + offset;

		dsq->overlay->alpha.interpolateTo(0, 1);
		core->main(1);

	}
	lua_pushnumber(L,0);
	return 1;
}

int l_getTimer(lua_State *L)
{
	float n = lua_tonumber(L, 1);
	if (n == 0)
		n = 1;
	lua_pushnumber(L, dsq->game->getTimer(n));
	return 1;
}

int l_getHalfTimer(lua_State *L)
{
	float n = lua_tonumber(L, 1);
	if (n == 0)
		n = 1;
	lua_pushnumber(L, dsq->game->getHalfTimer(n));
	return 1;
}

int l_isNested(lua_State *L)
{
	lua_pushboolean(L, core->isNested());
	return 1;
}

int l_getNumberOfEntitiesNamed(lua_State *L)
{
	std::string s = getString(L);
	int c = dsq->game->getNumberOfEntitiesNamed(s);
	lua_pushnumber(L, c);
	return 1;
}

int l_entity_pullEntities(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		Vector pos(lua_tonumber(L, 2), lua_tonumber(L, 3));
		int range = lua_tonumber(L, 4);
		float len = lua_tonumber(L, 5);
		float dt = lua_tonumber(L, 6);
		FOR_ENTITIES(i)
		{
			Entity *ent = *i;
			if (ent != e && (e->getEntityType() == ET_ENEMY || e->getEntityType() == ET_AVATAR) && e->isUnderWater())
			{
				Vector diff = ent->position - pos;
				if (diff.isLength2DIn(range))
				{
					Vector pull = pos - ent->position;
					pull.setLength2D(double(len) * dt);
					ent->vel2 += pull;
					/*
					std::ostringstream os;
					os << "ent: " << ent->name << " + (" << pull.x << ", " << pull.y << ")";
					debugLog(os.str());
					*/
				}
			}
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_delete(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		float time = lua_tonumber(L, 2);
		if (time == 0)
		{
			e->alpha = 0;
			e->setLife(0);
			e->setDecayRate(1);
		}
		else
		{
			e->fadeAlphaWithLife = true;
			e->setLife(1);
			e->setDecayRate(1.0/time);
		}
	}
	lua_pushinteger(L, 0);
	return 1;
}


int l_entity_setCull(lua_State *L)
{
	Entity *e = entity(L);
	bool v = getBool(L, 2);
	if (e)
	{
		e->cull = v;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_isRidingOnEntity(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		luaPushPointer(L, e->ridingOnEntity);
	else
		luaPushPointer(L, NULL);
	return 1;
}
//entity_setProperty(me, EP_SOLID, true)
int l_entity_isProperty(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->isEntityProperty((EntityProperty)int(lua_tonumber(L, 2)));
		//e->setEntityProperty((EntityProperty)lua_tointeger(L, 2), getBool(L, 3));
	}
	lua_pushboolean(L, v);
	return 1;
}

//entity_setProperty(me, EP_SOLID, true)
int l_entity_setProperty(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setEntityProperty((EntityProperty)lua_tointeger(L, 2), getBool(L, 3));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setActivation(lua_State* L)
{
	ScriptedEntity *e = scriptedEntity(L);
	//if (!e) return;
	int type = lua_tonumber(L, 2);
	// cursor radius
	int convoRadius = lua_tonumber(L, 3);
	int range = lua_tonumber(L, 4);
	e->activationType = (Entity::ActivationType)type;
	e->activationRange = range;
	e->convoRadius = convoRadius;

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_say(lua_State *L)
{
	Entity *e = entity(L);
	int n = lua_tonumber(L, 3);
	if (e)
	{
		e->say(getString(L, 2), (SayType)n);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_isSaying(lua_State *L)
{
	Entity *e = entity(L);
	bool b=false;
	if (e)
	{
		b = e->isSaying();
	}
	lua_pushboolean(L, b);
	return 1;
}

int l_entity_setSayPosition(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
		e->sayPosition = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setOverrideCullRadius(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->setOverrideCullRadius(lua_tonumber(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setActivationType(lua_State* L)
{
	Entity *e = entity(L);//si->getCurrentEntity();
	int type = lua_tonumber(L, 2);
	e->activationType = (Entity::ActivationType)type;

	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_hasTarget(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		lua_pushboolean(L, e->hasTarget(e->currentEntityTarget));
	else
		lua_pushboolean(L, false);
	return 1;
}

int l_entity_hurtTarget(lua_State* L)
{
	Entity *e = entity(L);
	if (e && e->getTargetEntity())
	{
		DamageData d;
		d.attacker = e;
		d.damage = lua_tointeger(L, 2);
		e->getTargetEntity(e->currentEntityTarget)->damage(d);
	}
	/*
	if (e && e->getTargetEntity())
		e->getTargetEntity(e->currentEntityTarget)->damage(lua_tointeger(L, 2), 0, e);
	*/
	lua_pushinteger(L, 0);
	return 1;
}

// radius dmg speed pushtime
int l_entity_touchAvatarDamage(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->touchAvatarDamage(lua_tonumber(L, 2), lua_tonumber(L, 3), Vector(-1,-1,-1), lua_tonumber(L, 4), lua_tonumber(L, 5), Vector(lua_tonumber(L, 6), lua_tonumber(L, 7)));
	}
	lua_pushboolean(L, v);
	return 1;
}

/*
int l_entity_touchAvatarDamageRect(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->touchAvatarDamageRect(lua_tonumber(L, 2), 
	}
	lua_pushboolean(L, v);
	return 1;
}
*/

int l_entity_getDistanceToEntity(lua_State *L)
{
	Entity *e = entity(L);
	Entity *e2 = entity(L, 2);
	float d = 0;
	if (e && e2)
	{
		Vector diff = e->position - e2->position;
		d = diff.getLength2D();
	}
	lua_pushnumber(L, d);
	return 1;
}

// entity_istargetInRange
int l_entity_isTargetInRange(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		lua_pushboolean(L, e->isTargetInRange(lua_tointeger(L, 2), e->currentEntityTarget));
	else
		lua_pushboolean(L, false);
	return 1;
}

int l_randAngle360(lua_State *L)
{
	lua_pushnumber(L, randAngle360());
	return 1;
}

int l_randVector(lua_State *L)
{
	float num = lua_tonumber(L, 1);
	if (num == 0)
		num = 1;
	Vector v = randVector(num);
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	return 2;
}

int l_getNaija(lua_State *L)
{
	luaPushPointer(L, dsq->game->avatar);
	return 1;
}

int l_getLi(lua_State *L)
{
	luaPushPointer(L, dsq->game->li);
	return 1;
}

int l_setLi(lua_State *L)
{
	dsq->game->li = entity(L);

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_isPositionInRange(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	int x, y;
	x = lua_tonumber(L, 2);
	y = lua_tonumber(L, 3);
	if (e)
	{
		if ((e->position - Vector(x,y)).isLength2DIn(lua_tonumber(L, 4)))
		{ 
			v = true;
		}
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_isEntityInRange(lua_State* L)
{
	Entity *e1 = entity(L);
	Entity *e2 = entity(L, 2);
	bool v= false;
	if (e1 && e2)
	{
		//v = ((e2->position - e1->position).getSquaredLength2D() < sqr(lua_tonumber(L, 3)));
		v = (e2->position - e1->position).isLength2DIn(lua_tonumber(L, 3));
	}
	lua_pushboolean(L, v);
	return 1;
}

//entity_moveTowardsTarget(spd, dt)
int l_entity_moveTowardsTarget(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveTowardsTarget(lua_tonumber(L, 2), lua_tonumber(L, 3), e->currentEntityTarget);
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_moveTowardsGroupCenter(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveTowardsGroupCenter(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_avgVel(lua_State *L)
{
	Entity *e = entity(L);
	float div = lua_tonumber(L, 2);
	if (e && div != 0)
	{
		e->vel /= div;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setVelLen(lua_State *L)
{
	Entity *e = entity(L);
	int len = lua_tonumber(L, 2);
	if (e)
	{
		e->vel.setLength2D(len);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_moveTowardsGroupHeading(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->moveTowardsGroupHeading(lua_tonumber(L, 2), lua_tonumber(L, 3));
	}
	lua_pushnumber(L, 0);
	return 1;
}

// entity dt speed dir
int l_entity_moveAroundTarget(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->moveAroundTarget(lua_tonumber(L, 2), lua_tointeger(L, 3), lua_tointeger(L, 4), e->currentEntityTarget);
	// do stuff
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_rotateToTarget(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
	{
		Entity *t = e->getTargetEntity(e->currentEntityTarget);
		if (t)
		{
			Vector v = t->position - e->position;
			e->rotateToVec(v, lua_tonumber(L, 2), lua_tointeger(L, 3));
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

/*
entity_initPart(partName, partTexture, partPosition, partFlipH, partFlipV,
partOffsetInterpolateTo, partOffsetInterpolateTime

*/

int l_entity_partWidthHeight(lua_State* L)
{
	ScriptedEntity *e = scriptedEntity(L);//(ScriptedEntity*)si->getCurrentEntity();
	Quad *r = (Quad*)e->partMap[lua_tostring(L, 2)];
	if (r)
	{
		int w = lua_tointeger(L, 3);
		int h = lua_tointeger(L, 4);
		r->setWidthHeight(w, h);
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_partSetSegs(lua_State *L)
{
	ScriptedEntity *e = scriptedEntity(L);
	Quad *r = (Quad*)e->partMap[lua_tostring(L, 2)];
	if (r)
	{
		r->setSegs(lua_tointeger(L, 3), lua_tointeger(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7), lua_tonumber(L, 8), lua_tonumber(L, 9), lua_tointeger(L, 10));
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_getEntityInGroup(lua_State *L)
{
	int gid = lua_tonumber(L, 1);
	int iter = lua_tonumber(L, 2);
	luaPushPointer(L, dsq->game->getEntityInGroup(gid, iter));
	return 1;
}

int l_entity_getGroupID(lua_State *L)
{
	Entity *e = entity(L);
	int id = 0;
	if(e)
	{
		id = e->getGroupID();
	}
	lua_pushnumber(L, id);
	return 1;
}

int l_entity_getID(lua_State *L)
{
	Entity *e = entity(L);
	int id = 0;
	if (e)
	{
		id = e->getID();
		std::ostringstream os;
		os << "id: " << id;
		debugLog(os.str());
	}
	lua_pushnumber(L, id);
	return 1;
}

int l_getEntityByID(lua_State *L)
{
	debugLog("Calling getEntityByID");
	int v = lua_tointeger(L, 1);
	Entity *found = 0;
	if (v)
	{
		std::ostringstream os;
		os << "searching for entity with id: " << v;
		debugLog(os.str());
		FOR_ENTITIES_EXTERN(i)
		{
			Entity *e = *i;
			if (e->getID() == v)
			{
				found = e;
				break;
			}
		}
		if (i == dsq->entities.end())
		{
			std::ostringstream os;
			os << "entity with id: " << v << " not found!";
			debugLog(os.str());
		}
		else
		{
			std::ostringstream os;
			os << "Found: " << found->name;
			debugLog(os.str());
		}
	}
	else
	{
		debugLog("entity ID was 0");
	}
	luaPushPointer(L, found);
	return 1;
}

/*
int l_node_toggleElements(lua_State *L)
{
	// disable the elements found within
	errorLog("node_disableElements not yet written");
	lua_pushnumber(L, 0);
	return 1;
}
*/

int l_node_setEffectOn(lua_State *L)
{
	Path *p = path(L, 1);
	p->setEffectOn(getBool(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_node_activate(lua_State *L)
{
	Path *p = path(L);
	Entity *e = 0;
	if (lua_touserdata(L, 2) != NULL)
		e = entity(L, 2);
	if (p)
	{
		p->activate(e);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_node_setElementsInLayerActive(lua_State *L)
{
	Path *p = path(L);
	int l = lua_tonumber(L, 2);
	bool v = getBool(L, 3);
	for (int i = 0; i < dsq->elements.size(); i++)
	{
		Element *e = dsq->elements[i];
		if (e && p->isCoordinateInside(e->position) && e->bgLayer == l)
		{
			debugLog("setting an element to the value");
			e->setElementActive(v);
		}
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_node_getNumEntitiesIn(lua_State *L)
{
	Path *p = path(L);
	std::string name;
	if (lua_isstring(L, 2))
	{
		name = lua_tostring(L, 2);
	}
	int c = 0;
	if (p && !p->nodes.empty())
	{
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (name.empty() || (nocasecmp(e->name, name)==0))
			{
				if (p->isCoordinateInside(e->position))
				{
					c++;
				}
			}
		}
	}

	/*
	std::ostringstream os;
	os << "counted: " << c << " entities with name: " << name;
	if (p)
		os << " in node area: " << p->name;
	debugLog(os.str());
	*/

	lua_pushnumber(L, c);
	return 1;
}

int l_node_getNearestEntity(lua_State *L)
{
	//Entity *me = entity(L);
	Path *p = path(L);
	Entity *closest=0;

	if (p && !p->nodes.empty())
	{

		Vector pos = p->nodes[0].position;
		std::string name;
		if (lua_isstring(L, 2))
			name = lua_tostring(L, 2);

		int smallestDist = -1;
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e->isPresent() && e->isNormalLayer())
			{
				if (name.empty() || (nocasecmp(e->name, name)==0))
				{
					int dist = (pos - e->position).getSquaredLength2D();
					if (smallestDist == -1 || dist < smallestDist)
					{
						smallestDist = dist;
						closest = e;
					}
				}
			}
		}
	}
	luaPushPointer(L, closest);
	return 1;
}

int l_node_getNearestNode(lua_State *L)
{
	//Entity *me = entity(L);
	Path *p = path(L);
	Path *closest=0;
	if (p && !p->nodes.empty())
	{
		Vector pos = p->nodes[0].position;
		std::string name;
		if (lua_isstring(L, 2))
			name = lua_tostring(L, 2);

		int smallestDist = -1;
		for (int i = 0; i < dsq->game->paths.size(); i++)
		{
			Path *p = dsq->game->paths[i];
			if ((name.empty() || p->name == name) && !p->nodes.empty())
			{
				int dist = (pos - p->nodes[0].position).getSquaredLength2D();
				if (smallestDist == -1 || dist < smallestDist)
				{
					smallestDist = dist;
					closest = p;
				}
			}
		}
	}
	luaPushPointer(L, closest);
	return 1;
}

int l_entity_getNearestBoneToPosition(lua_State *L)
{
	Entity *me = entity(L);
	Vector p(lua_tonumber(L, 2), lua_tonumber(L, 3));
	int smallestDist = -1;
	Bone *closest = 0;
	if (me)
	{
		for (int i = 0; i < me->skeletalSprite.bones.size(); i++)
		{
			Bone *b = me->skeletalSprite.bones[i];
			int dist = (b->getWorldPosition() - p).getSquaredLength2D();
			if (smallestDist == -1 || dist < smallestDist)
			{
				smallestDist = dist;
				closest = b;
			}
		}
	}
	luaPushPointer(L, closest);
	return 1;
}

int l_entity_getNearestNode(lua_State *L)
{
	Entity *me = entity(L);
	std::string name;
	if (lua_isstring(L, 2))
	{
		name = lua_tostring(L, 2);
		stringToLower(name);
	}

	Path *ignore = path(L, 3);
	Path *closest=0;
	int smallestDist = -1;
	for (int i = 0; i < dsq->game->paths.size(); i++)
	{
		Path *p = dsq->game->paths[i];
		if (p && p != ignore)
		{
			if (name.empty() || p->name == name)
			{
				int dist = (me->position - p->nodes[0].position).getSquaredLength2D();
				if (smallestDist == -1 || dist < smallestDist)
				{
					smallestDist = dist;
					closest = p;
				}
			}
		}
	}
	luaPushPointer(L, closest);
	return 1;
}

int l_ing_hasIET(lua_State *L)
{
	Ingredient *i = getIng(L, 1);
	bool has = i->hasIET((IngredientEffectType)lua_tointeger(L, 2));
	lua_pushboolean(L, has);
	return 1;
}

int l_entity_getNearestEntity(lua_State *L)
{
	Entity *me = entity(L);
	std::string name;
	if (lua_isstring(L, 2))
	{
		name = lua_tostring(L, 2);
		stringToLower(name);
	}

	bool nameCheck = true;
	if (!name.empty() && (name[0] == '!' || name[0] == '~'))
	{
		name = name.substr(1, name.size());
		nameCheck = false;
	}

	int range = lua_tointeger(L, 3);
	int type = lua_tointeger(L, 4);
	int damageTarget = lua_tointeger(L, 5);
	range = sqr(range);
	Entity *closest=0;
	int smallestDist = -1;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e != me && e->isPresent())
		{
			if (e->isNormalLayer())
			{
				if (name.empty() || ((nocasecmp(e->name, name)==0) == nameCheck))
				{
					if (type == 0 || e->getEntityType() == type)
					{
						if (damageTarget == 0 || e->isDamageTarget((DamageType)damageTarget))
						{
							int dist = (me->position - e->position).getSquaredLength2D();
							if ((range == 0 || dist < range) && smallestDist == -1 || dist < smallestDist)
							{
								smallestDist = dist;
								closest = e;
							}
						}
					}
				}
			}
		}
	}
	luaPushPointer(L, closest);
	return 1;
}

int l_findWall(lua_State *L)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	int dirx = lua_tonumber(L, 3);
	int diry = lua_tonumber(L, 4);
	if (dirx == 0 && diry == 0){ debugLog("dirx && diry are zero!"); lua_pushnumber(L, 0); return 1; }

	TileVector t(Vector(x, y));
	while (!dsq->game->isObstructed(t))
	{
		t.x += dirx;
		t.y += diry;
	}
	Vector v = t.worldVector();
	int wall = 0;
	if (diry != 0)		wall = v.y;
	if (dirx != 0)		wall = v.x;
	lua_pushnumber(L, wall);
	return 1;
}

int l_toggleVersionLabel(lua_State *L)
{
	bool on = getBool(L, 1);

	dsq->toggleVersionLabel(on);

	lua_pushboolean(L, on);
	return 1;
}

luaf(setVersionLabelText)
	dsq->setVersionLabelText();
luap(NULL)

luaf(setCutscene)
	dsq->setCutscene(getBool(L, 1), getBool(L, 2));
luap(NULL)

luaf(isInCutscene)
luab(dsq->isInCutscene())

luaf(toggleSteam)
	bool on = getBool(L, 1);
	for (int i = 0; i < dsq->game->paths.size(); i++)
	{
		Path *p = dsq->game->paths[i];
		if (p->pathType == PATH_STEAM)
		{
			p->setEffectOn(on);
		}
	}
luab(on)

luaf(getFirstEntity)
luap(dsq->getFirstEntity())

luaf(getNextEntity)
luap(dsq->getNextEntity())

luaf(getEntity)
	Entity *ent =0;
	// THIS WAS IMPORTANT: this was for getting entity by NUMBER IN LIST used for looping through all entities in script
	if (lua_isnumber(L, 1))
	{
		//HACK: FIX:
		// this has been disabled due to switching to list based entities
	}
	else if (lua_isstring(L, 1))
	{
		std::string s = lua_tostring(L, 1);
		ent = dsq->getEntityByName(s);
	}
luap(ent)

int _alpha(lua_State *L, RenderObject *r)
{
	if (r)
	{
		r->alpha.stop();
		r->alpha.interpolateTo(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tonumber(L, 6));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_bone_alpha(lua_State *L)
{
	return _alpha(L, boneToRenderObject(L));
}

int l_entity_alpha(lua_State *L)
{
	return _alpha(L, entityToRenderObject(L));
}

int l_entity_partAlpha(lua_State* L)
{
	ScriptedEntity *e = scriptedEntity(L);//(ScriptedEntity*)si->getCurrentEntity();
	RenderObject *r = e->partMap[lua_tostring(L, 2)];
	if (r)
	{

		float start = lua_tonumber(L, 3);
		if (start != -1)
			r->alpha = start;
		r->alpha.interpolateTo(lua_tonumber(L, 4), lua_tonumber(L, 5), lua_tointeger(L, 6), lua_tointeger(L, 7), lua_tointeger(L, 8));
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_partBlendType(lua_State *L)
{
	ScriptedEntity *e = scriptedEntity(L);//(ScriptedEntity*)si->getCurrentEntity();
	e->partMap[lua_tostring(L, 2)]->setBlendType(lua_tointeger(L, 3));
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_partRotate(lua_State* L)
{
	ScriptedEntity *e = scriptedEntity(L);//(ScriptedEntity*)si->getCurrentEntity();
	RenderObject *r = e->partMap[lua_tostring(L, 2)];
	if (r)
	{
		r->rotation.interpolateTo(Vector(0,0,lua_tointeger(L, 3)), lua_tonumber(L, 4), lua_tointeger(L, 5), lua_tointeger(L, 6), lua_tointeger(L, 7));
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getStateTime(lua_State* L)
{
	Entity *e = entity(L);//si->getCurrentEntity();
	float t = 0;
	if (e)
		t = e->getStateTime();
	lua_pushnumber(L, t);
	return 1;
}

int l_entity_setStateTime(lua_State* L)
{
	Entity *e = entity(L);//si->getCurrentEntity();
	float t = lua_tonumber(L, 2);
	if (e)
		e->setStateTime(t);
	lua_pushnumber(L, e->getStateTime());
	return 1;
}

int l_entity_offsetUpdate(lua_State* L)
{
	Entity *e = entity(L);//si->getCurrentEntity();
	if (e)
	{
		int uc = e->updateCull;
		e->updateCull = -1;
		float t = float(rand()%10000)/1000.0;
		e->update(t);
		e->updateCull = uc;
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_scale(lua_State* L)
{
	Entity *e = entity(L);
	float time = lua_tonumber(L, 4);
	//e->scale = Vector(lua_tonumber(L, 2), lua_tonumber(L, 3));
	e->scale.interpolateTo(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3), 0), time, lua_tonumber(L, 5), lua_tonumber(L, 6), lua_tonumber(L, 7));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_switchLayer(lua_State *L)
{
	Entity *e = entity(L);
	int lcode = lua_tonumber(L, 2);
	int toLayer = LR_ENTITIES;

	toLayer = dsq->getEntityLayerToLayer(lcode);

	if (e->getEntityType() == ET_AVATAR)
		toLayer = LR_ENTITIES;

	core->switchRenderObjectLayer(e, toLayer);
	return 1;
}

int l_entity_isScaling(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->scale.isInterpolating();
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_getScale(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		lua_pushnumber(L, e->scale.x);
		lua_pushnumber(L, e->scale.y);
	}
	else
	{
		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);
	}
	return 2;
}

// entity numSegments segmentLength width texture
int l_entity_initHair(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->initHair(lua_tonumber(L, 2), lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tostring(L, 5));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getHairPosition(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	float x=0;
	float y=0;
	int idx = lua_tonumber(L, 2);
	if (se && se->hair)
	{
		HairNode *h = se->hair->getHairNode(idx);
		if (h)
		{
			x = h->position.x;
			y = h->position.y;
		}
	}
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}

int l_entity_setUpdateCull(lua_State *L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->updateCull = lua_tonumber(L, 2);
	}
	lua_pushnumber(L, 0);
	return 1;
}

// entity x y z
int l_entity_setHairHeadPosition(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->setHairHeadPosition(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_updateHair(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		se->updateHair(lua_tonumber(L, 2));
	}
	lua_pushnumber(L, 0);
	return 1;
}

// entity x y dt
int l_entity_exertHairForce(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		if (se->hair)
			se->hair->exertForce(Vector(lua_tonumber(L, 2), lua_tonumber(L, 3)), lua_tonumber(L, 4), lua_tonumber(L, 5));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_initPart(lua_State* L)
{
	std::string partName(lua_tostring(L, 2));
	std::string partTex(lua_tostring(L, 3));
	Vector partPosition(lua_tointeger(L, 4), lua_tointeger(L, 5));
	int renderAfter = lua_tointeger(L, 6);
	bool partFlipH = lua_tointeger(L, 7);
	bool partFlipV = lua_tointeger(L,8);
	Vector offsetInterpolateTo(lua_tointeger(L, 9), lua_tointeger(L, 10));
	float offsetInterpolateTime = lua_tonumber(L, 11);


	ScriptedEntity *e = scriptedEntity(L);//(ScriptedEntity*)si->getCurrentEntity();

	Quad *q = new Quad;
	q->setTexture(partTex);
	q->renderBeforeParent = !renderAfter;


	q->position = partPosition;
	if (offsetInterpolateTo.x != 0 || offsetInterpolateTo.y != 0)
		q->offset.interpolateTo(offsetInterpolateTo, offsetInterpolateTime, -1, 1, 1);
	if (partFlipH)
		q->flipHorizontal();
	if (partFlipV)
		q->flipVertical();

	e->addChild(q, PM_POINTER);
	e->registerNewPart(q, partName);

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_findTarget(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->findTarget(lua_tointeger(L, 2), lua_tointeger(L, 3), e->currentEntityTarget);

	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_doFriction(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
	{
		e->doFriction(lua_tonumber(L, 2), lua_tointeger(L, 3));
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_doGlint(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->doGlint(e->position, Vector(2,2), getString(L,2), (RenderObject::BlendTypes)lua_tointeger(L, 3));
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_getPosition(lua_State* L)
{
	Entity *e = entity(L);
	float x=0,y=0;
	if (e)
	{
		x = e->position.x;
		y = e->position.y;
	}
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}

int l_entity_getOffset(lua_State* L)
{
	Entity *e = entity(L);
	float x=0,y=0;
	if (e)
	{
		x = e->offset.x;
		y = e->offset.y;
	}
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}



int l_entity_getPositionX(lua_State* L)
{
	Entity *e = entity(L);
	int x = 0;
	if (e)
		x = int(e->position.x);
	lua_pushinteger(L, x);
	return 1;
}

int l_entity_getPositionY(lua_State* L)
{
	Entity *e = entity(L);
	int y = 0;
	if (e)
		y = int(e->position.y);
	lua_pushinteger(L, y);
	return 1;
}

int l_entity_getTarget(lua_State *L)
{
	Entity *e = entity(L);
	Entity *retEnt = NULL;
	if (e)
	{
		retEnt = e->getTargetEntity(lua_tonumber(L, 2));
		//e->activate();
	}
	luaPushPointer(L, retEnt);
	return 1;
}

int l_entity_getTargetPositionX(lua_State* L)
{
	lua_pushinteger(L, int(entity(L)->getTargetEntity()->position.x));
	return 1;
}

int l_entity_getTargetPositionY(lua_State* L)
{
	lua_pushinteger(L, int(entity(L)->getTargetEntity()->position.y));
	return 1;
}

int l_entity_isNearObstruction(lua_State *L)
{
	Entity *e = entity(L);
	int sz = lua_tonumber(L, 2);
	int type = lua_tointeger(L, 3);
	bool v = false;
	if (e)
	{
		v = e->isNearObstruction(sz, type);
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_isInvincible(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->isInvincible();
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_isInterpolating(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->position.isInterpolating();
	}
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_isRotating(lua_State *L)
{
	Entity *e = entity(L);
	bool v = false;
	if (e)
	{
		v = e->rotation.isInterpolating();
	}
	lua_pushboolean(L, v);
	return 1;
}


int l_entity_interpolateTo(lua_State *L)
{
	Entity *e = entity(L);
	int x = lua_tonumber(L, 2);
	int y = lua_tonumber(L, 3);
	float t = lua_tonumber(L, 4);
	if (e)
	{
		e->position.interpolateTo(Vector(x, y), t);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_entity_setEatType(lua_State* L)
{
	Entity *e = entity(L);
	int et = lua_tointeger(L, 2);
	if (e)
		e->setEatType((EatType)et, getString(L, 3));
	lua_pushinteger(L, et);
	return 1;
}

int l_entity_setPositionX(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->position.x = lua_tointeger(L, 2);
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_setPositionY(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->position.y = lua_tointeger(L, 2);
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_rotateTo(lua_State* L)
{
	Entity *e = entity(L);
	if (e)
		e->rotation.interpolateTo(Vector(0,0,lua_tointeger(L, 2)), lua_tonumber(L, 3));
	lua_pushinteger(L, 0);
	return 1;
}

int l_getMapName(lua_State *L)
{
	lua_pushstring(L, dsq->game->sceneName.c_str());
	return 1;
}

int l_isMapName(lua_State *L)
{
	std::string s1 = dsq->game->sceneName;
	std::string s2 = getString(L, 1);
	stringToUpper(s1);
	stringToUpper(s2);
	bool ret = (s1 == s2);

	lua_pushboolean(L, ret);
	return 1;
}

int l_mapNameContains(lua_State *L)
{
	std::string s = dsq->game->sceneName;
	stringToLower(s);
	bool b = (s.find(getString(L, 1)) != std::string::npos);
	lua_pushboolean(L, b);
	return 1;
}

int l_entity_fireGas(lua_State* L)
{
	Entity *e = entity(L);//si->getCurrentEntity();
	if (e)
	{
		int radius = lua_tointeger(L, 2);
		float life = lua_tonumber(L, 3);
		float damage = lua_tonumber(L, 4);
		std::string gfx = lua_tostring(L, 5);
		float colorx = lua_tonumber(L, 6);
		float colory = lua_tonumber(L, 7);
		float colorz = lua_tonumber(L, 8);
		float offx = lua_tonumber(L, 9);
		float offy = lua_tonumber(L, 10);
		float poisonTime = lua_tonumber(L, 11);

		GasCloud *c = new GasCloud(e, e->position + Vector(offx, offy), gfx, Vector(colorx, colory, colorz), radius, life, damage, false, poisonTime);
		core->getTopStateData()->addRenderObject(c, LR_PARTICLES);
	}
	lua_pushinteger(L, 0);
	return 1;
}

int l_isInputEnabled(lua_State *L)
{
	lua_pushboolean(L, dsq->game->avatar->isInputEnabled());
	return 1;
}

int l_enableInput(lua_State* L)
{
	dsq->game->avatar->enableInput();
	lua_pushinteger(L, 0);
	return 1;
}

int l_disableInput(lua_State* L)
{
	dsq->game->avatar->disableInput();
	lua_pushinteger(L, 0);
	return 1;
}

int l_quit(lua_State *L)
{
#ifdef AQUARIA_DEMO
	dsq->nag(NAG_QUIT);
#else
	dsq->quit();
#endif

	lua_pushinteger(L, 0);
	return 1;
}

int l_doModSelect(lua_State *L)
{
	dsq->doModSelect();
	lua_pushinteger(L, 0);
	return 1;
}

int l_doLoadMenu(lua_State *L)
{
	dsq->doLoadMenu();
	lua_pushinteger(L, 0);
	return 1;
}

int l_resetContinuity(lua_State *L)
{
	dsq->continuity.reset();
	lua_pushinteger(L, 0);
	return 1;
}

int l_toWindowFromWorld(lua_State *L)
{
	float x = lua_tonumber(L, 1);
	float y = lua_tonumber(L, 2);
	x = x - core->screenCenter.x;
	y = y - core->screenCenter.y;
	x *= core->globalScale.x;
	y *= core->globalScale.x;
	x = 400+x;
	y = 300+y;
	lua_pushnumber(L, x);
	lua_pushnumber(L, y);
	return 2;
}

int l_setMousePos(lua_State *L)
{
	core->setMousePosition(Vector(lua_tonumber(L, 1), lua_tonumber(L, 2)));
	lua_pushnumber(L, 0);
	return 1;
}

int l_getMousePos(lua_State *L)
{
	lua_pushnumber(L, core->mouse.position.x);
	lua_pushnumber(L, core->mouse.position.y);
	return 2;
}

int l_getMouseWorldPos(lua_State *L)
{
	Vector v = dsq->getGameCursorPosition();
	lua_pushnumber(L, v.x);
	lua_pushnumber(L, v.y);
	return 2;
}

int l_fade(lua_State* L)
{
	dsq->overlay->color = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	dsq->overlay->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	lua_pushinteger(L, 0);
	return 1;
}

int l_fade2(lua_State* L)
{
	dsq->overlay2->color = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	dsq->overlay2->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	lua_pushinteger(L, 0);
	return 1;
}

int l_fade3(lua_State* L)
{
	dsq->overlay3->color = Vector(lua_tonumber(L, 3), lua_tonumber(L, 4), lua_tonumber(L, 5));
	dsq->overlay3->alpha.interpolateTo(lua_tonumber(L, 1), lua_tonumber(L, 2));
	lua_pushinteger(L, 0);
	return 1;
}

int l_vision(lua_State *L)
{
	dsq->vision(lua_tostring(L, 1), lua_tonumber(L, 2), getBool(L, 3));
	lua_pushnumber(L, 0);
	return 1;
}

int l_musicVolume(lua_State *L)
{
	dsq->sound->setMusicFader(lua_tonumber(L, 1), lua_tonumber(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_voice(lua_State *L)
{
	float vmod = lua_tonumber(L, 2);
	if (vmod == 0)
		vmod = -1;
	else if (vmod == -1)
		vmod = 0;
	dsq->voice(lua_tostring(L, 1), vmod);
	lua_pushinteger(L, 0);
	return 1;
}

int l_voiceOnce(lua_State *L)
{
	dsq->voiceOnce(lua_tostring(L, 1));
	lua_pushinteger(L, 0);
	return 1;
}

int l_voiceInterupt(lua_State *L)
{
	dsq->voiceInterupt(lua_tostring(L, 1));
	lua_pushinteger(L, 0);
	return 1;
}

int l_stopVoice(lua_State *L)
{
	dsq->stopVoice();
	lua_pushnumber(L, 0);
	return 1;
}

int l_stopAllSfx(lua_State *L)
{
	dsq->sound->stopAllSfx();
	lua_pushnumber(L, 0);
	return 1;
}

int l_stopAllVoice(lua_State *L)
{
	dsq->sound->stopAllVoice();
	lua_pushnumber(L, 0);
	return 1;
}

int l_fadeIn(lua_State *L)
{
	dsq->overlay->alpha.interpolateTo(0, lua_tonumber(L, 1));
	lua_pushinteger(L, 0);
	return 1;
}

int l_fadeOut(lua_State *L)
{
	dsq->overlay->color = 0;
	dsq->overlay->alpha.interpolateTo(1, lua_tonumber(L, 1));
	lua_pushinteger(L, 0);
	return 1;
}

int l_entity_setWeight(lua_State *L)
{
	CollideEntity *e = collideEntity(L);
	if (e)
		e->weight = lua_tointeger(L, 2);
	lua_pushinteger(L, 0);
	return 1;
}

int l_pickupGem(lua_State *L)
{
	dsq->continuity.pickupGem(getString(L), !getBool(L, 2));
	lua_pushnumber(L, 0);
	return 1;
}

int l_beaconEffect(lua_State *L)
{
	int index = lua_tointeger(L, 1);

	BeaconData *b = dsq->continuity.getBeaconByIndex(index);

	float p1 = 0.7;
	float p2 = 1.0 - p1;

	dsq->clickRingEffect(dsq->game->miniMapRender->getWorldPosition(), 0, (b->color*p1) + Vector(p2, p2, p2), 1);
	dsq->clickRingEffect(dsq->game->miniMapRender->getWorldPosition(), 1, (b->color*p1) + Vector(p2, p2, p2), 1);

	dsq->sound->playSfx("ping");

	lua_pushnumber(L, 0);
	return 1;
}

int l_setBeacon(lua_State *L)
{
	int index = lua_tointeger(L, 1);

	bool v = getBool(L, 2);

	Vector pos;
	pos.x = lua_tonumber(L, 3);
	pos.y = lua_tonumber(L, 4);

	Vector color;
	color.x = lua_tonumber(L, 5);
	color.y = lua_tonumber(L, 6);
	color.z = lua_tonumber(L, 7);

	dsq->continuity.setBeacon(index, v, pos, color);

	lua_pushnumber(L, 0);
	return 1;
}

int l_getBeacon(lua_State *L)
{
	int index = lua_tointeger(L, 1);
	bool v = false;
	BeaconData *b = 0;

	if (b = dsq->continuity.getBeaconByIndex(index))
	{
		v = true;
	}

	lua_pushboolean(L, v);
	return 1;
}

// particle effect

int l_getCostume(lua_State *L)
{
	lua_pushstring(L, dsq->continuity.costume.c_str());
	return 1;
}

int l_setCostume(lua_State *L)
{
	dsq->continuity.setCostume(getString(L));
	lua_pushnumber(L, 0);
	return 1;
}

int l_setElementLayerVisible(lua_State *L)
{
	int l = lua_tonumber(L, 1);
	bool v = getBool(L, 2);
	dsq->game->setElementLayerVisible(l, v);
	lua_pushnumber(L, 0);
	return 1;
}

int l_isElementLayerVisible(lua_State *L)
{
	lua_pushboolean(L, dsq->game->isElementLayerVisible(lua_tonumber(L, 1)));
	return 1;
}

int l_isStreamingVoice(lua_State *L)
{
	bool v = dsq->sound->isPlayingVoice();
	lua_pushboolean(L, v);
	return 1;
}

int l_entity_getAlpha(lua_State *L)
{
	Entity *e = entity(L);
	float a = 0;
	if (e)
	{
		a = e->alpha.x;
	}
	lua_pushnumber(L, a);
	return 1;
}

int l_isObstructed(lua_State *L)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	lua_pushboolean(L, dsq->game->isObstructed(TileVector(Vector(x,y))));
	return 1;
}

int l_isObstructedBlock(lua_State *L)
{
	int x = lua_tonumber(L, 1);
	int y = lua_tonumber(L, 2);
	int span = lua_tonumber(L, 3);
	TileVector t(Vector(x,y));

	bool obs = false;
	for (int xx = t.x-span; xx < t.x+span; xx++)
	{
		for (int yy = t.y-span; yy < t.y+span; yy++)
		{
			if (dsq->game->isObstructed(TileVector(xx, yy)))
			{
				obs = true;
				break;
			}
		}
	}
	lua_pushboolean(L, obs);
	return 1;
}

int l_node_getFlag(lua_State *L)
{
	Path *p = path(L);
	int v = 0;
	if (p)
	{
		v = dsq->continuity.getPathFlag(p);
	}
	lua_pushnumber(L, v);
	return 1;
}

int l_node_isFlag(lua_State *L)
{
	Path *p = path(L);
	int c = lua_tonumber(L, 2);
	bool ret = false;
	if (p)
	{
		ret = (c == dsq->continuity.getPathFlag(p));
	}
	lua_pushboolean(L, ret);
	return 1;
}

int l_node_setFlag(lua_State *L)
{
	Path *p = path(L);
	int v = lua_tonumber(L, 2);
	if (p)
	{
		dsq->continuity.setPathFlag(p, v);
	}
	lua_pushnumber(L, v);
	return 1;
}

int l_entity_isFlag(lua_State *L)
{
	Entity *e = entity(L);
	int v = lua_tonumber(L, 2);
	bool b = false;
	if (e)
	{
		b = (dsq->continuity.getEntityFlag(dsq->game->sceneName, e->getID())==v);
	}
	lua_pushboolean(L, b);
	return 1;
}

int l_entity_setFlag(lua_State *L)
{
	Entity *e = entity(L);
	int v = lua_tonumber(L, 2);
	if (e)
	{
		dsq->continuity.setEntityFlag(dsq->game->sceneName, e->getID(), v);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_isFlag(lua_State *L)
{
	int v = 0;
	/*
	if (lua_isstring(L, 1))
		v = dsq->continuity.getFlag(lua_tostring(L, 1));
	else if (lua_isnumber(L, 1))
	*/
	bool f = false;
	if (lua_isnumber(L, 1))
	{
		v = dsq->continuity.getFlag(lua_tointeger(L, 1));
		f = (v == lua_tointeger(L, 2));
	}
	else
	{
		v = dsq->continuity.getFlag(getString(L, 1));
		f = (v == lua_tointeger(L, 2));
	}
	/*
	int f = 0;
	dsq->continuity.getFlag(lua_tostring(L, 1));

	*/
	lua_pushboolean(L, f);
	return 1;
}

int l_avatar_updatePosition(lua_State *L)
{
	dsq->game->avatar->updatePosition();
	lua_pushnumber(L, 0);
	return 1;
}

int l_avatar_toggleMovement(lua_State *L)
{
	dsq->game->avatar->toggleMovement((bool)lua_tointeger(L, 1));
	lua_pushnumber(L, 0);
	return 1;
}

int l_clearShots(lua_State *L)
{
	Shot::killAllShots();
	lua_pushnumber(L, 0);
	return 1;
}

int l_clearHelp(lua_State *L)
{
	float t = 0.4;

	/*
	RenderObjects *r = &core->renderObjectLayers[LR_HELP].renderObjects;


	for (RenderObjects::iterator i = r->begin(); i != r->end(); i++)
	{
		RenderObject *ro = (*i);
	*/

	RenderObjectLayer *rl = &core->renderObjectLayers[LR_HELP];
	RenderObject *ro = rl->getFirst();
	while (ro)
	{
		ro->setLife(t);
		ro->setDecayRate(1);
		ro->alpha.stopPath();
		ro->alpha.interpolateTo(0,t-0.01);

		ro = rl->getNext();
	}

	lua_pushnumber(L, 0);
	return 1;
}

int l_setLiPower(lua_State *L)
{
	float m = lua_tonumber(L, 1);
	float t = lua_tonumber(L, 2);
	dsq->continuity.setLiPower(m, t); 
	return 1;
}

int l_getLiPower(lua_State *L)
{
	lua_pushnumber(L, dsq->continuity.liPower);
	return 1;
}

int l_getPetPower(lua_State *L)
{
	lua_pushnumber(L, dsq->continuity.petPower);
	return 1;
}

int l_getPlantGrabNode(lua_State *L)
{
	ScriptedEntity *se = scriptedEntity(L);
	if (se)
	{
		//se->springPlant.
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_showControls(lua_State *L)
{
	std::string keygfx = lua_tostring(L, 1);
	std::string mousegfx = lua_tostring(L, 2);
	if (!keygfx.empty())
	{
		Quad *keyboard = new Quad;
		keyboard->setBlendType(RenderObject::BLEND_ADD);
		keyboard->followCamera = 1;
		keyboard->setTexture("controls/" + keygfx);
		keyboard->alpha=0;
		keyboard->alpha.interpolateTo(0.5, 4, 1, 1);
		keyboard->scale.interpolateTo(Vector(0.9, 0.9), 4);
		keyboard->position = Vector(300, 500);
		core->getTopStateData()->addRenderObject(keyboard, LR_HELP);
	}
	if (!mousegfx.empty())
	{
		float t = 30;
		Quad *keyboard = new Quad;
		keyboard->setBlendType(RenderObject::BLEND_ADD);
		keyboard->followCamera = 1;
		keyboard->setTexture("controls/" + mousegfx);
		keyboard->alpha = 0;
		keyboard->alpha.path.addPathNode(0, 0);
		keyboard->alpha.path.addPathNode(0.5, .1);
		keyboard->alpha.path.addPathNode(0.5, .9);
		keyboard->alpha.path.addPathNode(0, 1);
		keyboard->alpha.startPath(t);
		//keyboard->alpha.interpolateTo(0.5, 4, 1, 1);
		keyboard->scale.interpolateTo(Vector(0.9, 0.9), t+0.5);
		keyboard->position = Vector(600, 400);
		core->getTopStateData()->addRenderObject(keyboard, LR_HELP);
	}
	lua_pushnumber(L, 0);
	return 1;
}

int l_appendUserDataPath(lua_State *L)
{
	std::string path = getString(L, 1);
	
	if (!dsq->getUserDataFolder().empty())
		path = dsq->getUserDataFolder() + "/" + path;
	
	lua_pushstring(L, path.c_str());
	return 1;
}


//============================================================================================
// F U N C T I O N S
//============================================================================================
void ScriptInterface::init()
{
//	choice = -1;
	si = this;
	currentEntity = 0;
	currentParticleEffect = 0;
	//collideEntity = 0;

//	particleEffectScripts
	//loadParticleEffectScripts();

	initLuaVM(&L);
}

ParticleEffectScript *ScriptInterface::getParticleEffectScriptByIdx(int idx)
{
	for (ParticleEffectScripts::iterator i = particleEffectScripts.begin();
		i != particleEffectScripts.end(); i++)
	{
		ParticleEffectScript *p = &((*i).second);
		if (p->idx == idx)
			return p;
	}
	return 0;
}

void ScriptInterface::loadParticleEffectScripts()
{
	//particleEffectScripts
	std::ifstream in("scripts/particleEffects/ParticleEffects.txt");
	std::string line;
	while (std::getline(in, line))
	{
		int v;
		std::string n;
		std::istringstream is(line);
		is >> v >> n;
		//ggggerrorLog (n);

		lua_State *L;
		dsq->scriptInterface.initLuaVM(&L);
		std::string file = "scripts/particleEffects/" + n + ".lua";
		file = core->adjustFilenameCase(file);
		int fail = (luaL_loadfile(L, file.c_str()));
		if (fail)	errorLog(lua_tostring(L, -1));
		//this->name = script;
		fail = lua_pcall(L, 0, 0, 0);
		if (fail)	errorLog(lua_tostring(L, -1));

		particleEffectScripts[n].lua = L;
		particleEffectScripts[n].name = n;
		particleEffectScripts[n].idx = v;
	}
}

bool ScriptInterface::setCurrentEntity(Entity *e)
{
	//if (se && se->runningActivation) return false;
	//currentEntityTarget = 0;
	noMoreConversationsThisRun = false;
	currentEntity = e;
	//collideEntity = dynamic_cast<CollideEntity*>(e);
	//se = dynamic_cast<ScriptedEntity*>(e);
	return true;
}

static int l_dofile_caseinsensitive(lua_State *L)
{
	// This is Lua's dofile(), with some tweaks.  --ryan.
	std::string fname(core->adjustFilenameCase(luaL_checkstring(L, 1)));
	int n = lua_gettop(L);
	if (luaL_loadfile(L, fname.c_str()) != 0) lua_error(L);
	lua_call(L, 0, LUA_MULTRET);
	return lua_gettop(L) - n;
}

void ScriptInterface::initLuaVM(lua_State **L)
{
	*L = lua_open();				/* opens Lua */
	luaopen_base(*L);				/* opens the basic library */
	luaopen_table(*L);				/* opens the table library */
	luaopen_string(*L);				/* opens the string lib. */
	luaopen_math(*L);				/* opens the math lib. */
	//luaopen_os(*L);				/* opens the os lib */

	// override Lua's standard dofile(), so we can handle filename case issues.
	lua_register( *L, "dofile", l_dofile_caseinsensitive);

	//luaopen_io(*L);				/* opens the I/O library */

	lua_register( *L, "shakeCamera",									l_shakeCamera);
	lua_register( *L, "upgradeHealth",									l_upgradeHealth);
	
	lua_register( *L, "cureAllStatus",									l_cureAllStatus);
	lua_register( *L, "setPoison",										l_setPoison);
	lua_register( *L, "setMusicToPlay",									l_setMusicToPlay);
	lua_register( *L, "confirm",										l_confirm);
	
	lua_register( *L, "randRange",										l_randRange);

	lua_register( *L, "flingMonkey",									l_flingMonkey);
	

	lua_register( *L, "setLiPower",										l_setLiPower);
	lua_register( *L, "getLiPower",										l_getLiPower);
	lua_register( *L, "getPetPower",									l_getPetPower);
	lua_register( *L, "getTimer",										l_getTimer);
	lua_register( *L, "getHalfTimer",									l_getHalfTimer);
	lua_register( *L, "setCostume",										l_setCostume);
	lua_register( *L, "getCostume",										l_getCostume);
	lua_register( *L, "getNoteName",									l_getNoteName);


	lua_register( *L, "getWorldType",									l_getWorldType);


	lua_register( *L, "getWaterLevel",									l_getWaterLevel);
	lua_register( *L, "setWaterLevel",									l_setWaterLevel);


	lua_register( *L, "getEntityInGroup",								l_getEntityInGroup);

	lua_register( *L, "createQuad",										l_createQuad);
	lua_register( *L, "quad_delete",									l_quad_delete);
	lua_register( *L, "quad_scale",										l_quad_scale);
	lua_register( *L, "quad_rotate",									l_quad_rotate);

	lua_register( *L, "quad_color",										l_quad_color);
	lua_register( *L, "quad_alpha",										l_quad_alpha);
	lua_register( *L, "quad_alphaMod",									l_quad_alphaMod);
	lua_register( *L, "quad_getAlpha",									l_quad_getAlpha);

	lua_register( *L, "quad_setPosition",								l_quad_setPosition);
	lua_register( *L, "quad_setBlendType",								l_quad_setBlendType);


	lua_register( *L, "setupEntity",									l_setupEntity);
	lua_register( *L, "setActivePet",									l_setActivePet);


	lua_register( *L, "reconstructGrid",								l_reconstructGrid);
	lua_register( *L, "reconstructEntityGrid",							l_reconstructEntityGrid);



	

	lua_register( *L, "ing_hasIET",										l_ing_hasIET);


	lua_register( *L, "esetv",											l_e_setv);
	lua_register( *L, "esetvf",											l_e_setvf);
	lua_register( *L, "egetv",											l_e_getv);
	lua_register( *L, "egetvf",											l_e_getvf);
	lua_register( *L, "eisv",											l_e_isv);

	lua_register( *L, "entity_addIgnoreShotDamageType",					l_entity_addIgnoreShotDamageType);
	lua_register( *L, "entity_ensureLimit",								l_entity_ensureLimit);
	lua_register( *L, "entity_getBoneLockEntity",						l_entity_getBoneLockEntity);


	lua_register( *L, "entity_setRidingPosition",						l_entity_setRidingPosition);
	lua_register( *L, "entity_setRidingData",							l_entity_setRidingData);
	lua_register( *L, "entity_setBoneLock",								l_entity_setBoneLock);
	lua_register( *L, "entity_setIngredient",							l_entity_setIngredient);
	lua_register( *L, "entity_setDeathScene",							l_entity_setDeathScene);
	lua_register( *L, "entity_say",										l_entity_say);
	lua_register( *L, "entity_isSaying",								l_entity_isSaying);
	lua_register( *L, "entity_setSayPosition",							l_entity_setSayPosition);


	lua_register( *L, "entity_setClampOnSwitchDir",						l_entity_setClampOnSwitchDir);

	lua_register( *L, "entity_setRegisterEntityDied",					l_entity_setRegisterEntityDied);

	lua_register( *L, "entity_setBeautyFlip",							l_entity_setBeautyFlip);
	lua_register( *L, "entity_setInvincible",							l_entity_setInvincible);

	lua_register( *L, "setInvincible",									l_setInvincible);

	



	lua_register( *L, "entity_setLife",									l_entity_setLife);
	lua_register( *L, "entity_setLookAtPoint",							l_entity_setLookAtPoint);
	lua_register( *L, "entity_getLookAtPoint",							l_entity_getLookAtPoint);


	lua_register( *L, "entity_setDieTimer",								l_entity_setDieTimer);
	lua_register( *L, "entity_setAutoSkeletalUpdate",					l_entity_setAutoSkeletalUpdate);
	lua_register( *L, "entity_updateSkeletal",							l_entity_updateSkeletal);
	lua_register( *L, "entity_setBounceType",							l_entity_setBounceType);

	lua_register( *L, "entity_getHealthPerc",							l_entity_getHealthPerc);
	lua_register( *L, "entity_getBounceType",							l_entity_getBounceType);
	lua_register( *L, "entity_setRiding",								l_entity_setRiding);
	lua_register( *L, "entity_getRiding",								l_entity_getRiding);

	lua_register( *L, "entity_setNodeGroupActive",						l_entity_setNodeGroupActive);

	lua_register( *L, "entity_setNaijaReaction",						l_entity_setNaijaReaction);

	lua_register( *L, "entity_setEatType",								l_entity_setEatType);

	lua_register( *L, "entity_setSpiritFreeze",							l_entity_setSpiritFreeze);

	lua_register( *L, "entity_setCanLeaveWater",						l_entity_setCanLeaveWater);

	lua_register( *L, "entity_pullEntities",							l_entity_pullEntities);

	lua_register( *L, "entity_setEntityLayer",							l_entity_setEntityLayer);
	lua_register( *L, "entity_setRenderPass",							l_entity_setRenderPass);

	lua_register( *L, "entity_clearTargetPoints",						l_entity_clearTargetPoints);
	lua_register( *L, "entity_addTargetPoint",							l_entity_addTargetPoint);


	lua_register( *L, "entity_setOverrideCullRadius",					l_entity_setOverrideCullRadius);
	lua_register( *L, "entity_setCullRadius",							l_entity_setOverrideCullRadius);

	lua_register( *L, "entity_setUpdateCull",							l_entity_setUpdateCull);
	lua_register( *L, "entity_flipHToAvatar",							l_entity_flipHToAvatar);
	//lua_register( *L, "entity_fhTo",									l_entity_fhTo);

	lua_register( *L, "entity_switchLayer",								l_entity_switchLayer);

	lua_register( *L, "entity_debugText",								l_entity_debugText);

	
	lua_register( *L, "avatar_setCanDie",								l_avatar_setCanDie);
	lua_register( *L, "avatar_toggleCape",								l_avatar_toggleCape);
	lua_register( *L, "avatar_setPullTarget",							l_avatar_setPullTarget);


	lua_register( *L, "setGLNearest",									l_setGLNearest);
	

	lua_register( *L, "avatar_clampPosition",							l_avatar_clampPosition);
	lua_register( *L, "avatar_updatePosition",							l_avatar_updatePosition);

	lua_register( *L, "pause",											l_pause);
	lua_register( *L, "unpause",										l_unpause);


	lua_register( *L, "vector_normalize",								l_vector_normalize);
	lua_register( *L, "vector_setLength",								l_vector_setLength);
	lua_register( *L, "vector_getLength",								l_vector_getLength);

	lua_register( *L, "vector_dot",										l_vector_dot);

	lua_register( *L, "vector_isLength2DIn",							l_vector_isLength2DIn);
	lua_register( *L, "vector_cap",										l_vector_cap);


	lua_register( *L, "entity_setDeathParticleEffect",					l_entity_setDeathParticleEffect);
	lua_register( *L, "entity_setDeathSound",							l_entity_setDeathSound);

	lua_register( *L, "entity_setDamageTarget",							l_entity_setDamageTarget);
	lua_register( *L, "entity_setAllDamageTargets",						l_entity_setAllDamageTargets);

	lua_register( *L, "entity_isDamageTarget",							l_entity_isDamageTarget);
	lua_register( *L, "entity_isVelIn",									l_entity_isVelIn);
	lua_register( *L, "entity_isValidTarget",							l_entity_isValidTarget);


	lua_register( *L, "entity_isUnderWater",							l_entity_isUnderWater);
	lua_register( *L, "entity_checkSplash",								l_entity_checkSplash);




	lua_register( *L, "entity_setEnergyShotTarget",						l_entity_setEnergyShotTarget);
	lua_register( *L, "entity_setEnergyShotTargetPosition",				l_entity_setEnergyShotTargetPosition);
	//lua_register( *L, "entity_getEnergyShotTargetPosition",				l_entity_getEnergyShotTargetPosition);
	lua_register( *L, "entity_getRandomTargetPoint",					l_entity_getRandomTargetPoint);
	lua_register( *L, "entity_getTargetPoint",							l_entity_getTargetPoint);


	lua_register( *L, "entity_setTargetRange",							l_entity_setTargetRange);


	lua_register( *L, "entity_setEnergyChargeTarget",					l_entity_setEnergyChargeTarget);

	lua_register( *L, "entity_setCollideWithAvatar",					l_entity_setCollideWithAvatar);
	lua_register( *L, "entity_setPauseInConversation",					l_entity_setPauseInConversation);


	lua_register( *L, "bone_setRenderPass",								l_bone_setRenderPass);
	lua_register( *L, "bone_setVisible",								l_bone_setVisible);
	lua_register( *L, "bone_isVisible",									l_bone_isVisible);

	lua_register( *L, "bone_addSegment",								l_bone_addSegment);
	lua_register( *L, "entity_setSegs",									l_entity_setSegs);
	lua_register( *L, "bone_setSegs",									l_bone_setSegs);
	lua_register( *L, "bone_update",									l_bone_update);


	lua_register( *L, "bone_setSegmentOffset",							l_bone_setSegmentOffset);
	lua_register( *L, "bone_setSegmentProps",							l_bone_setSegmentProps);
	lua_register( *L, "bone_setSegmentChainHead",						l_bone_setSegmentChainHead);
	lua_register( *L, "bone_setAnimated",								l_bone_setAnimated);
	lua_register( *L, "bone_showFrame",									l_bone_showFrame);

	lua_register( *L, "bone_lookAtEntity",								l_bone_lookAtEntity);

	lua_register( *L, "bone_setTexture",								l_bone_setTexture);

	lua_register( *L, "bone_scale",										l_bone_scale);
	lua_register( *L, "bone_setBlendType",								l_bone_setBlendType);

	//lua_register( *L, "bone_setRotation",								l_bone_setRotation);


	lua_register( *L, "entity_partSetSegs",								l_entity_partSetSegs);


	lua_register( *L, "entity_adjustPositionBySurfaceNormal",			l_entity_adjustPositionBySurfaceNormal);
	lua_register( *L, "entity_applySurfaceNormalForce",					l_entity_applySurfaceNormalForce);
	lua_register( *L, "entity_applyRandomForce",						l_entity_applyRandomForce);

	lua_register( *L, "createBeam",										l_createBeam);
	lua_register( *L, "beam_setAngle",									l_beam_setAngle);
	lua_register( *L, "beam_setPosition",								l_beam_setPosition);
	lua_register( *L, "beam_setTexture",								l_beam_setTexture);
	lua_register( *L, "beam_setDamage",									l_beam_setDamage);
	lua_register( *L, "beam_setBeamWidth",								l_beam_setBeamWidth);


	lua_register( *L, "beam_delete",									l_beam_delete);

	lua_register( *L, "getStringBank",									l_getStringBank);

	lua_register( *L, "isPlat",											l_isPlat);

	lua_register( *L, "getAngleBetweenEntities",						l_getAngleBetweenEntities);
	lua_register( *L, "getAngleBetween",								l_getAngleBetween);


	lua_register( *L, "createEntity",									l_createEntity);
	lua_register( *L, "entity_setWeight",								l_entity_setWeight);
	lua_register( *L, "entity_setBlendType",							l_entity_setBlendType);

	lua_register( *L, "entity_setActivationType",						l_entity_setActivationType);
	lua_register( *L, "entity_setColor",								l_entity_setColor);
	lua_register( *L, "entity_color",									l_entity_setColor);
	lua_register( *L, "entity_playSfx",									l_entity_playSfx);
	
	lua_register( *L, "isQuitFlag",										l_isQuitFlag);
	lua_register( *L, "isDeveloperKeys",								l_isDeveloperKeys);
	lua_register( *L, "isDemo",											l_isDemo);

	lua_register( *L, "isInputEnabled",									l_isInputEnabled);
	lua_register( *L, "disableInput",									l_disableInput);

	lua_register( *L, "setMousePos",									l_setMousePos);
	lua_register( *L, "getMousePos",									l_getMousePos);
	lua_register( *L, "getMouseWorldPos",								l_getMouseWorldPos);

	lua_register( *L, "resetContinuity",								l_resetContinuity);

	lua_register( *L, "quit",											l_quit);
	lua_register( *L, "doModSelect",									l_doModSelect);
	lua_register( *L, "doLoadMenu",										l_doLoadMenu);


	lua_register( *L, "enableInput",									l_enableInput);
	lua_register( *L, "fade",											l_fade);
	lua_register( *L, "fade2",											l_fade2);
	lua_register( *L, "fade3",											l_fade3);

	lua_register( *L, "setupConversationEntity",						l_setupConversationEntity);

	lua_register( *L, "getMapName",										l_getMapName);
	lua_register( *L, "isMapName",										l_isMapName);
	lua_register( *L, "mapNameContains",								l_mapNameContains);
	
	lua_register( *L, "entity_getNormal",								l_entity_getNormal);

	lua_register( *L, "entity_getAlpha",								l_entity_getAlpha);
	lua_register( *L, "entity_getAimVector",							l_entity_getAimVector);

	lua_register( *L, "entity_getVectorToEntity",						l_entity_getVectorToEntity);

	lua_register( *L, "entity_getVelLen",								l_entity_getVelLen);

	lua_register( *L, "entity_getDistanceToTarget",						l_entity_getDistanceToTarget);
	lua_register( *L, "entity_delete",									l_entity_delete);
	lua_register( *L, "entity_move",									l_entity_move);


	lua_register( *L, "entity_moveToFront",								l_entity_moveToFront);
	lua_register( *L, "entity_moveToBack",								l_entity_moveToBack);


	

	lua_register( *L, "entity_getID",									l_entity_getID);
	lua_register( *L, "entity_getGroupID",								l_entity_getGroupID);

	lua_register( *L, "getEntityByID",									l_getEntityByID);

	lua_register( *L, "entity_setBounce",								l_entity_setBounce);
	lua_register( *L, "entity_setPosition",								l_entity_setPosition);
	lua_register( *L, "entity_setInternalOffset",						l_entity_setInternalOffset);
	lua_register( *L, "entity_setActivation",							l_entity_setActivation);
	lua_register( *L, "entity_rotateToEntity",							l_entity_rotateToEntity);
	lua_register( *L, "entity_rotateTo",								l_entity_rotateTo);
	lua_register( *L, "entity_rotateOffset",							l_entity_rotateOffset);

	lua_register( *L, "entity_fireGas",									l_entity_fireGas);
	lua_register( *L, "entity_rotateToTarget",							l_entity_rotateToTarget);

	lua_register( *L, "entity_switchSurfaceDirection",					l_entity_switchSurfaceDirection);

	lua_register( *L, "entity_offset",									l_entity_offset);
	lua_register( *L, "entity_moveAlongSurface",						l_entity_moveAlongSurface);
	lua_register( *L, "entity_rotateToSurfaceNormal",					l_entity_rotateToSurfaceNormal);
	lua_register( *L, "entity_clampToSurface",							l_entity_clampToSurface);
	lua_register( *L, "entity_checkSurface",							l_entity_checkSurface);
	lua_register( *L, "entity_clampToHit",								l_entity_clampToHit);


	lua_register( *L, "entity_grabTarget",								l_entity_grabTarget);
	lua_register( *L, "entity_releaseTarget",							l_entity_releaseTarget);

	lua_register( *L, "entity_getStateTime",							l_entity_getStateTime);
	lua_register( *L, "entity_setStateTime",							l_entity_setStateTime);

	lua_register( *L, "entity_scale",									l_entity_scale);
	lua_register( *L, "entity_getScale",								l_entity_getScale);

	lua_register( *L, "entity_doFriction",								l_entity_doFriction);

	lua_register( *L, "entity_partWidthHeight",							l_entity_partWidthHeight);
	lua_register( *L, "entity_partBlendType",							l_entity_partBlendType);
	lua_register( *L, "entity_partRotate",								l_entity_partRotate);
	lua_register( *L, "entity_partAlpha",								l_entity_partAlpha);

	lua_register( *L, "entity_fireAtTarget",							l_entity_fireAtTarget);

	lua_register( *L, "entity_getHealth",								l_entity_getHealth);
	lua_register( *L, "entity_pushTarget",								l_entity_pushTarget);
	lua_register( *L, "entity_flipHorizontal",							l_entity_flipHorizontal);
	lua_register( *L, "entity_flipVertical",							l_entity_flipVertical);
	lua_register( *L, "entity_fh",										l_entity_flipHorizontal);
	lua_register( *L, "entity_fhTo",									l_entity_fhTo);
	lua_register( *L, "entity_fv",										l_entity_flipVertical);
	lua_register( *L, "entity_update",									l_entity_update);
	lua_register( *L, "entity_msg",										l_entity_msg);
	lua_register( *L, "entity_updateMovement",							l_entity_updateMovement);
	lua_register( *L, "entity_updateCurrents",							l_entity_updateCurrents);
	lua_register( *L, "entity_updateLocalWarpAreas",					l_entity_updateLocalWarpAreas);

	lua_register( *L, "entity_setPositionX",							l_entity_setPositionX);
	lua_register( *L, "entity_setPositionY",							l_entity_setPositionY);
	lua_register( *L, "entity_getPosition",								l_entity_getPosition);
	lua_register( *L, "entity_getOffset",								l_entity_getOffset);
	lua_register( *L, "entity_getPositionX",							l_entity_getPositionX);
	lua_register( *L, "entity_getPositionY",							l_entity_getPositionY);

	lua_register( *L, "entity_getTargetPositionX",						l_entity_getTargetPositionX);
	lua_register( *L, "entity_getTargetPositionY",						l_entity_getTargetPositionY);

	lua_register( *L, "entity_incrTargetLeaches",						l_entity_incrTargetLeaches);
	lua_register( *L, "entity_decrTargetLeaches",						l_entity_decrTargetLeaches);
	lua_register( *L, "entity_rotateToVel",								l_entity_rotateToVel);
	lua_register( *L, "entity_rotateToVec",								l_entity_rotateToVec);

	lua_register( *L, "entity_setSegsMaxDist",							l_entity_setSegsMaxDist);



	lua_register( *L, "entity_offsetUpdate",							l_entity_offsetUpdate);

	lua_register( *L, "entity_createEntity",							l_entity_createEntity);
	lua_register( *L, "entity_resetTimer",								l_entity_resetTimer);
	lua_register( *L, "entity_stopTimer",								l_entity_stopTimer);
	lua_register( *L, "entity_stopPull",								l_entity_stopPull);
	lua_register( *L, "entity_setTargetPriority",						l_entity_setTargetPriority);


	lua_register( *L, "entity_setBehaviorType",							l_entity_setBehaviorType);
	lua_register( *L, "entity_getBehaviorType",							l_entity_getBehaviorType);
	lua_register( *L, "entity_setEntityType",							l_entity_setEntityType);
	lua_register( *L, "entity_getEntityType",							l_entity_getEntityType);

	lua_register( *L, "entity_setSegmentTexture",						l_entity_setSegmentTexture);


	lua_register( *L, "entity_spawnParticlesFromCollisionMask",			l_entity_spawnParticlesFromCollisionMask);
	lua_register( *L, "entity_initEmitter",								l_entity_initEmitter);
	lua_register( *L, "entity_startEmitter",							l_entity_startEmitter);
	lua_register( *L, "entity_stopEmitter",								l_entity_stopEmitter);

	lua_register( *L, "entity_initPart",								l_entity_initPart);
	lua_register( *L, "entity_initSegments",							l_entity_initSegments);
	lua_register( *L, "entity_warpSegments",							l_entity_warpSegments);
	lua_register( *L, "entity_initSkeletal",							l_entity_initSkeletal);
	lua_register( *L, "entity_initStrands",								l_entity_initStrands);

	lua_register( *L, "entity_hurtTarget",								l_entity_hurtTarget);
	lua_register( *L, "entity_doSpellAvoidance",						l_entity_doSpellAvoidance);
	lua_register( *L, "entity_doEntityAvoidance",						l_entity_doEntityAvoidance);
	lua_register( *L, "entity_rotate",									l_entity_rotate);
	lua_register( *L, "entity_doGlint",									l_entity_doGlint);
	lua_register( *L, "entity_findTarget",								l_entity_findTarget);
	lua_register( *L, "entity_hasTarget",								l_entity_hasTarget);
	lua_register( *L, "entity_isInRect",								l_entity_isInRect);
	lua_register( *L, "entity_isInDarkness",							l_entity_isInDarkness);
	lua_register( *L, "entity_isScaling",								l_entity_isScaling);

	lua_register( *L, "entity_isRidingOnEntity",						l_entity_isRidingOnEntity);

	lua_register( *L, "entity_isBeingPulled",							l_entity_isBeingPulled);

	lua_register( *L, "entity_isNearObstruction",						l_entity_isNearObstruction);
	lua_register( *L, "entity_isDead",									l_entity_isDead);



	lua_register( *L, "entity_isTargetInRange",							l_entity_isTargetInRange);
	lua_register( *L, "entity_getDistanceToEntity",						l_entity_getDistanceToEntity);

	lua_register( *L, "entity_isInvincible",							l_entity_isInvincible);

	lua_register( *L, "entity_isNearGround",							l_entity_isNearGround);

	lua_register( *L, "entity_moveTowardsTarget",						l_entity_moveTowardsTarget);
	lua_register( *L, "entity_moveAroundTarget",						l_entity_moveAroundTarget);

	lua_register( *L, "entity_moveTowardsAngle",						l_entity_moveTowardsAngle);
	lua_register( *L, "entity_moveAroundAngle",							l_entity_moveAroundAngle);
	lua_register( *L, "entity_moveTowards",								l_entity_moveTowards);
	lua_register( *L, "entity_moveAround",								l_entity_moveAround);

	lua_register( *L, "entity_moveTowardsGroupCenter",					l_entity_moveTowardsGroupCenter);
	lua_register( *L, "entity_moveTowardsGroupHeading",					l_entity_moveTowardsGroupHeading);
	lua_register( *L, "entity_avgVel",									l_entity_avgVel);
	lua_register( *L, "entity_setVelLen",								l_entity_setVelLen);

	lua_register( *L, "entity_setMaxSpeed",								l_entity_setMaxSpeed);
	lua_register( *L, "entity_getMaxSpeed",								l_entity_getMaxSpeed);
	lua_register( *L, "entity_setMaxSpeedLerp",							l_entity_setMaxSpeedLerp);
	lua_register( *L, "entity_setState",								l_entity_setState);
	lua_register( *L, "entity_getState",								l_entity_getState);
	lua_register( *L, "entity_getEnqueuedState",						l_entity_getEnqueuedState);

	lua_register( *L, "entity_getPrevState",							l_entity_getPrevState);
	lua_register( *L, "entity_doCollisionAvoidance",					l_entity_doCollisionAvoidance);
	lua_register( *L, "entity_animate",									l_entity_animate);
	lua_register( *L, "entity_setAnimLayerTimeMult",					l_entity_setAnimLayerTimeMult);

	lua_register( *L, "entity_setCurrentTarget",						l_entity_setCurrentTarget);
	//lua_register( *L, "entity_spawnParticleEffect",					l_entity_spawnParticleEffect);
	lua_register( *L, "entity_warpToPathStart",							l_entity_warpToPathStart);
	lua_register( *L, "entity_stopInterpolating",						l_entity_stopInterpolating);

	lua_register( *L, "entity_followPath",								l_entity_followPath);
	lua_register( *L, "entity_isFollowingPath",							l_entity_isFollowingPath);
	lua_register( *L, "entity_followEntity",							l_entity_followEntity);
	lua_register( *L, "entity_sound",									l_entity_sound);
	lua_register( *L, "entity_soundFreq",								l_entity_soundFreq);


	lua_register( *L, "entity_enableMotionBlur",						l_entity_enableMotionBlur);
	lua_register( *L, "entity_disableMotionBlur",						l_entity_disableMotionBlur);


	lua_register( *L, "registerSporeChildData",							l_registerSporeChildData);
	lua_register( *L, "registerSporeDrop",								l_registerSporeDrop);

	
	lua_register( *L, "getIngredientGfx",								l_getIngredientGfx);

	lua_register( *L, "spawnIngredient",								l_spawnIngredient);
	lua_register( *L, "spawnAllIngredients",							l_spawnAllIngredients);
	lua_register( *L, "spawnParticleEffect",							l_spawnParticleEffect);
	lua_register( *L, "spawnManaBall",									l_spawnManaBall);


	lua_register( *L, "isEscapeKey",									l_isEscapeKey);
	

	lua_register( *L, "resetTimer",										l_resetTimer);

	lua_register( *L, "addInfluence",									l_addInfluence);
	lua_register( *L, "setupBasicEntity",								l_setupBasicEntity);
	lua_register( *L, "playMusic",										l_playMusic);
	lua_register( *L, "playMusicStraight",								l_playMusicStraight);
	lua_register( *L, "stopMusic",										l_stopMusic);

	lua_register( *L, "user_set_demo_intro",							l_user_set_demo_intro);
	lua_register( *L, "user_save",										l_user_save);

	lua_register( *L, "playMusicOnce",									l_playMusicOnce);
	
	lua_register( *L, "playSfx",										l_playSfx);
	lua_register( *L, "fadeSfx",										l_fadeSfx);
	
	lua_register( *L, "emote",											l_emote);

	lua_register( *L, "playVfx",										l_playVisualEffect);
	lua_register( *L, "playVisualEffect",								l_playVisualEffect);
	lua_register( *L, "playNoEffect",									l_playNoEffect);

	
	lua_register( *L, "setOverrideMusic",								l_setOverrideMusic);

	lua_register( *L, "setOverrideVoiceFader",							l_setOverrideVoiceFader);
	lua_register( *L, "setGameSpeed",									l_setGameSpeed);
	lua_register( *L, "sendEntityMessage",								l_sendEntityMessage);
	lua_register( *L, "healEntity",										l_healEntity);
	lua_register( *L, "warpAvatar",										l_warpAvatar);
	lua_register( *L, "warpNaijaToSceneNode",							l_warpNaijaToSceneNode);



	lua_register( *L, "toWindowFromWorld",								l_toWindowFromWorld);

	lua_register( *L, "toggleTransitFishRide",							l_toggleTransitFishRide);

	lua_register( *L, "toggleDamageSprite",								l_toggleDamageSprite);

	lua_register( *L, "toggleLiCombat",									l_toggleLiCombat);

	lua_register( *L, "toggleCursor",									l_toggleCursor);
	lua_register( *L, "toggleBlackBars",								l_toggleBlackBars);
	lua_register( *L, "setBlackBarsColor",								l_setBlackBarsColor);

	
	lua_register( *L, "stopCursorGlow",									l_stopCursorGlow);

	lua_register( *L, "entityFollowEntity",								l_entityFollowEntity);
	lua_register( *L, "setEntityScript",								l_setEntityScript);

	lua_register( *L, "setMiniMapHint",									l_setMiniMapHint);
	lua_register( *L, "bedEffects",										l_bedEffects);

	lua_register( *L, "killEntity",										l_killEntity);
	lua_register( *L, "warpNaijaToEntity",								l_warpNaijaToEntity);

	lua_register( *L, "setNaijaHeadTexture",							l_setNaijaHeadTexture);
	lua_register( *L, "avatar_setHeadTexture",							l_setNaijaHeadTexture);

	//lua_register( *L, "hurtEntity",									l_hurtEntity);

	lua_register( *L, "incrFlag",										l_incrFlag );
	lua_register( *L, "decrFlag",										l_decrFlag );
	lua_register( *L, "setFlag",										l_setFlag );
	lua_register( *L, "getFlag",										l_getFlag );
	lua_register( *L, "setStringFlag",									l_setStringFlag );
	lua_register( *L, "getStringFlag",									l_getStringFlag );
	lua_register( *L, "learnSpell",										l_learnSpell );
	lua_register( *L, "learnSong",										l_learnSong );
	lua_register( *L, "unlearnSong",									l_unlearnSong );
	lua_register( *L, "hasSong",										l_hasSong );
	lua_register( *L, "hasLi",											l_hasLi );

	lua_register( *L, "setCanWarp",										l_setCanWarp );
	lua_register( *L, "setCanChangeForm",								l_setCanChangeForm );
	lua_register( *L, "setInvincibleOnNested",							l_setInvincibleOnNested );
	
	lua_register( *L, "setControlHint",									l_setControlHint );
	lua_register( *L, "setCameraLerpDelay",								l_setCameraLerpDelay );
	lua_register( *L, "screenFadeGo",									l_screenFadeGo );
	lua_register( *L, "screenFadeTransition",							l_screenFadeTransition );
	lua_register( *L, "screenFadeCapture",								l_screenFadeCapture );

	lua_register( *L, "clearControlHint",								l_clearControlHint );


	lua_register( *L, "savePoint",										l_savePoint );
	lua_register( *L, "moveEntity",										l_moveEntity );
	lua_register( *L, "wait",											l_wait );
	lua_register( *L, "watch",											l_watch );

	lua_register( *L, "quitNestedMain",									l_quitNestedMain );
	lua_register( *L, "isNestedMain",									l_isNestedMain );


	lua_register( *L, "msg",											l_msg );
	lua_register( *L, "centerText",										l_centerText );
	lua_register( *L, "watchForVoice",									l_watchForVoice );

	lua_register( *L, "setElementLayerVisible",							l_setElementLayerVisible);
	lua_register( *L, "isElementLayerVisible",							l_isElementLayerVisible);

	lua_register( *L, "isWithin",										l_isWithin );



	lua_register( *L, "pickupGem",										l_pickupGem );
	lua_register( *L, "setBeacon",										l_setBeacon );
	lua_register( *L, "getBeacon",										l_getBeacon );
	lua_register( *L, "beaconEffect",									l_beaconEffect );
	
	lua_register( *L, "chance",											l_chance);

	lua_register( *L, "goToTitle",										l_goToTitle);
	lua_register( *L, "jumpState",										l_jumpState);
	lua_register( *L, "getEnqueuedState",								l_getEnqueuedState);


	lua_register( *L, "fadeIn",											l_fadeIn);
	lua_register( *L, "fadeOut",										l_fadeOut);

	lua_register( *L, "vision",											l_vision);

	lua_register( *L, "musicVolume",									l_musicVolume);

	lua_register( *L, "voice",											l_voice);
	lua_register( *L, "playVoice",										l_voice);
	lua_register( *L, "voiceOnce",										l_voiceOnce);
	lua_register( *L, "voiceInterupt",									l_voiceInterupt);


	lua_register( *L, "stopVoice",										l_stopVoice);
	lua_register( *L, "stopAllVoice",									l_stopAllVoice);
	lua_register( *L, "stopAllSfx",										l_stopAllSfx);



	lua_register( *L, "fadeOutMusic",									l_fadeOutMusic);


	/*
	lua_register( *L, "options",										l_options);
	lua_register( *L, "opt",											l_options);
	*/
	lua_register( *L, "isStreamingVoice",								l_isStreamingVoice);
	lua_register( *L, "isPlayingVoice",									l_isStreamingVoice);

	lua_register( *L, "changeForm",										l_changeForm);
	lua_register( *L, "getForm",										l_getForm);
	lua_register( *L, "isForm",											l_isForm);
	lua_register( *L, "learnFormUpgrade",								l_learnFormUpgrade);
	lua_register( *L, "hasFormUpgrade",									l_hasFormUpgrade);


	lua_register( *L, "castSong",										l_castSong);
	lua_register( *L, "isObstructed",									l_isObstructed);
	lua_register( *L, "isObstructedBlock",								l_isObstructedBlock);

	lua_register( *L, "isFlag",											l_isFlag);

	lua_register( *L, "entity_isFlag",				 					l_entity_isFlag);
	lua_register( *L, "entity_setFlag",									l_entity_setFlag);

	lua_register( *L, "node_isFlag",				 					l_node_isFlag);
	lua_register( *L, "node_setFlag",									l_node_setFlag);
	lua_register( *L, "node_getFlag",									l_node_getFlag);

	lua_register( *L, "avatar_getStillTimer",							l_avatar_getStillTimer);
	lua_register( *L, "avatar_getSpellCharge",							l_avatar_getSpellCharge);

	lua_register( *L, "avatar_isSinging",								l_avatar_isSinging);
	lua_register( *L, "avatar_isTouchHit",								l_avatar_isTouchHit);
	lua_register( *L, "avatar_isBursting",								l_avatar_isBursting);
	lua_register( *L, "avatar_isLockable",								l_avatar_isLockable);
	lua_register( *L, "avatar_isRolling",								l_avatar_isRolling);
	lua_register( *L, "avatar_isOnWall",								l_avatar_isOnWall);
	lua_register( *L, "avatar_isShieldActive",							l_avatar_isShieldActive);
	lua_register( *L, "avatar_getRollDirection",						l_avatar_getRollDirection);

	lua_register( *L, "avatar_fallOffWall",								l_avatar_fallOffWall);
	lua_register( *L, "avatar_setBlockSinging",							l_avatar_setBlockSinging);


	lua_register( *L, "avatar_toggleMovement",							l_avatar_toggleMovement);

	lua_register( *L, "toggleConversationWindow",						l_toggleConversationWindow);
	lua_register( *L, "toggleDialogWindow",								l_toggleConversationWindow);


	lua_register( *L, "showInGameMenu",									l_showInGameMenu);
	lua_register( *L, "hideInGameMenu",									l_hideInGameMenu);
	

	lua_register( *L, "showImage",										l_showImage);
	lua_register( *L, "hideImage",										l_hideImage);
	lua_register( *L, "showControls",									l_showControls);
	lua_register( *L, "clearHelp",										l_clearHelp);
	lua_register( *L, "clearShots",										l_clearShots);



	lua_register( *L, "getEntity",										l_getEntity);
	lua_register( *L, "getEntityByName",								l_getEntity);

	lua_register( *L, "getFirstEntity",									l_getFirstEntity);
	lua_register( *L, "getNextEntity",									l_getNextEntity);

	lua_register( *L, "setStory",										l_setStory);
	lua_register( *L, "getStory",										l_getStory);
	lua_register( *L, "getNoteColor",									l_getNoteColor);
	lua_register( *L, "getNoteVector",									l_getNoteVector);
	lua_register( *L, "getRandNote",									l_getRandNote);

	lua_register( *L, "foundLostMemory",								l_foundLostMemory);

	

	lua_register( *L, "isStory",										l_isStory);

	lua_register( *L, "isInDialog",										l_isInConversation);
	lua_register( *L, "entity_damage",									l_entity_damage);
	lua_register( *L, "entity_heal",									l_entity_heal);

	lua_register( *L, "getNearestIngredient",							l_getNearestIngredient);

	lua_register( *L, "getNearestNode",									l_getNearestNode);
	lua_register( *L, "getNearestNodeByType",							l_getNearestNodeByType);

	lua_register( *L, "getNode",										l_getNode);
	lua_register( *L, "getNodeByName",									l_getNode);
	lua_register( *L, "getNodeToActivate",								l_getNodeToActivate);
	lua_register( *L, "setNodeToActivate",								l_setNodeToActivate);
	lua_register( *L, "setActivation",									l_setActivation);
	
	lua_register( *L, "entity_warpToNode",								l_entity_warpToNode);
	lua_register( *L, "entity_moveToNode",								l_entity_moveToNode);

	lua_register( *L, "setNaijaModel",									l_setNaijaModel);

	lua_register( *L, "cam_toNode",										l_cam_toNode);
	lua_register( *L, "cam_snap",										l_cam_snap);
	lua_register( *L, "cam_toEntity",									l_cam_toEntity);
	lua_register( *L, "cam_setPosition",								l_cam_setPosition);


	lua_register( *L, "entity_flipTo",									l_entity_flipToEntity);
	lua_register( *L, "entity_flipToEntity",							l_entity_flipToEntity);
	lua_register( *L, "entity_flipToSame",								l_entity_flipToSame);

	lua_register( *L, "entity_flipToNode",								l_entity_flipToNode);
	lua_register( *L, "entity_flipToVel",								l_entity_flipToVel);

	lua_register( *L, "entity_swimToNode",								l_entity_swimToNode);
	lua_register( *L, "entity_swimToPosition",							l_entity_swimToPosition);


	lua_register( *L, "createShot",										l_createShot);
	lua_register( *L, "entity_fireShot",								l_entity_fireShot);

	lua_register( *L, "entity_setAffectedBySpells",						l_entity_setAffectedBySpells);
	lua_register( *L, "entity_isHit",									l_entity_isHit);



	lua_register( *L, "createWeb",										l_createWeb);
	lua_register( *L, "web_addPoint",									l_web_addPoint);
	lua_register( *L, "web_setPoint",									l_web_setPoint);
	lua_register( *L, "web_getNumPoints",								l_web_getNumPoints);
	lua_register( *L, "web_delete",										l_web_delete);

	lua_register( *L, "createSpore",									l_createSpore);



	lua_register( *L, "shot_getPosition",								l_shot_getPosition);
	lua_register( *L, "shot_setAimVector",								l_shot_setAimVector);
	lua_register( *L, "shot_setOut",									l_shot_setOut);
	lua_register( *L, "shot_setLifeTime",								l_shot_setLifeTime);
	lua_register( *L, "shot_setNice",									l_shot_setNice);
	lua_register( *L, "shot_setVel",									l_shot_setVel);
	lua_register( *L, "shot_setBounceType",								l_shot_setBounceType);
	lua_register( *L, "entity_pathBurst",								l_entity_pathBurst);
	lua_register( *L, "entity_handleShotCollisions",					l_entity_handleShotCollisions);
	lua_register( *L, "entity_handleShotCollisionsSkeletal",			l_entity_handleShotCollisionsSkeletal);
	lua_register( *L, "entity_handleShotCollisionsHair",				l_entity_handleShotCollisionsHair);
	lua_register( *L, "entity_collideSkeletalVsCircle",					l_entity_collideSkeletalVsCircle);
	lua_register( *L, "entity_collideSkeletalVsLine",					l_entity_collideSkeletalVsLine);
	lua_register( *L, "entity_collideSkeletalVsCircleForListByName",	l_entity_collideSkeletalVsCircleForListByName);
	lua_register( *L, "entity_collideCircleVsLine",						l_entity_collideCircleVsLine);
	lua_register( *L, "entity_collideCircleVsLineAngle",				l_entity_collideCircleVsLineAngle);


	lua_register( *L, "entity_collideHairVsCircle",						l_entity_collideHairVsCircle);

	lua_register( *L, "entity_setDropChance",							l_entity_setDropChance);

	lua_register( *L, "entity_waitForPath",								l_entity_waitForPath);
	lua_register( *L, "entity_watchForPath",							l_entity_watchForPath);

	lua_register( *L, "entity_addVel",									l_entity_addVel);
	lua_register( *L, "entity_addVel2",									l_entity_addVel2);
	lua_register( *L, "entity_addRandomVel",							l_entity_addRandomVel);

	lua_register( *L, "entity_addGroupVel",								l_entity_addGroupVel);
	lua_register( *L, "entity_clearVel",								l_entity_clearVel);
	lua_register( *L, "entity_clearVel2",								l_entity_clearVel2);


	lua_register( *L, "entity_revive",									l_entity_revive);

	lua_register( *L, "entity_getTarget",								l_entity_getTarget);
	lua_register( *L, "entity_isState",									l_entity_isState);

	lua_register( *L, "entity_setProperty",								l_entity_setProperty);
	lua_register( *L, "entity_isProperty",								l_entity_isProperty);


	lua_register( *L, "entity_initHair",								l_entity_initHair);
	lua_register( *L, "entity_getHairPosition",							l_entity_getHairPosition);

	lua_register( *L, "entity_setHairHeadPosition",						l_entity_setHairHeadPosition);
	lua_register( *L, "entity_updateHair",								l_entity_updateHair);
	lua_register( *L, "entity_exertHairForce",							l_entity_exertHairForce);

	lua_register( *L, "entity_setName",									l_entity_setName);

	lua_register( *L, "getNumberOfEntitiesNamed",						l_getNumberOfEntitiesNamed);

	lua_register( *L, "isNested",										l_isNested);

	lua_register( *L, "wnd",											l_toggleConversationWindow);
	lua_register( *L, "wnds",											l_toggleConversationWindowSoft);

	lua_register( *L, "entity_idle",									l_entity_idle);
	lua_register( *L, "entity_stopAllAnimations",						l_entity_stopAllAnimations);

	lua_register(*L, "entity_getBoneByIdx",								l_entity_getBoneByIdx);
	lua_register(*L, "entity_getBoneByName",							l_entity_getBoneByName);
	//lua_register(*L, "bone_getWorldPosition",							l_bone_getWorldPosition);



	lua_register( *L, "inp",											l_toggleInput);

	//lua_register( *L, "CM",											l_CM);
	//lua_register( *L, "isIDHighest",									l_isIDHighest);
	//lua_register( *L, "isEGOHighest",									l_isEGOHighest);
	//lua_register( *L, "isSUPHighest",									l_isSUPHighest);

	lua_register( *L, "entity_setTarget",								l_entity_setTarget);
	lua_register( *L, "getNodeFromEntity",								l_getNodeFromEntity);

	lua_register( *L, "getScreenCenter",								l_getScreenCenter);



	lua_register( *L, "debugLog",										l_debugLog);
	lua_register( *L, "loadMap",										l_loadMap);

	lua_register( *L, "reloadTextures",									l_reloadTextures);

	lua_register( *L, "loadSound",										l_loadSound);

	lua_register( *L, "node_activate",									l_node_activate);
	lua_register( *L, "node_getName",									l_node_getName);
	lua_register( *L, "node_getPathPosition",							l_node_getPathPosition);
	lua_register( *L, "node_getPosition",								l_node_getPosition);
	lua_register( *L, "node_setPosition",								l_node_setPosition);
	lua_register( *L, "node_getContent",								l_node_getContent);
	lua_register( *L, "node_getAmount",									l_node_getAmount);
	lua_register( *L, "node_getSize",									l_node_getSize);
	lua_register( *L, "node_setEffectOn",								l_node_setEffectOn);

	//luar(	node_setEffectOn				);
	luar(	toggleSteam						);
	luar(	toggleVersionLabel				);
	luar(	setVersionLabelText				);
	
	luar(	appendUserDataPath				);

	luar(	setCutscene						);
	luar(	isInCutscene					);

		

	lua_register( *L, "node_getNumEntitiesIn",							l_node_getNumEntitiesIn);


	lua_register( *L, "entity_getName",									l_entity_getName);
	lua_register( *L, "entity_isName",									l_entity_isName);
	

	lua_register( *L, "node_setCursorActivation",						l_node_setCursorActivation);
	lua_register( *L, "node_setCatchActions",							l_node_setCatchActions);

	lua_register( *L, "node_setElementsInLayerActive",					l_node_setElementsInLayerActive);


	lua_register( *L, "entity_setHealth",								l_entity_setHealth);
	lua_register( *L, "entity_changeHealth",							l_entity_changeHealth);

	lua_register( *L, "node_setActive",									l_node_setActive);


	lua_register( *L, "setGameOver",									l_setGameOver);
	lua_register( *L, "setSceneColor",									l_setSceneColor);


	lua_register( *L, "entity_watchEntity",								l_entity_watchEntity);

	lua_register( *L, "entity_setCollideRadius",						l_entity_setCollideRadius);
	lua_register( *L, "entity_getCollideRadius",						l_entity_getCollideRadius);
	lua_register( *L, "entity_setTouchPush",							l_entity_setTouchPush);
	lua_register( *L, "entity_setTouchDamage",							l_entity_setTouchDamage);

	lua_register( *L, "entity_isEntityInRange",							l_entity_isEntityInRange);
	lua_register( *L, "entity_isPositionInRange",						l_entity_isPositionInRange);

	lua_register( *L, "entity_stopFollowingPath",						l_entity_stopFollowingPath);
	lua_register( *L, "entity_slowToStopPath",							l_entity_slowToStopPath);
	lua_register( *L, "entity_isSlowingToStopPath",						l_entity_isSlowingToStopPath);

	lua_register( *L, "entity_findNearestEntityOfType",					l_entity_findNearestEntityOfType);
	lua_register( *L, "entity_isFollowingEntity",						l_entity_isFollowingEntity);
	lua_register( *L, "entity_resumePath",								l_entity_resumePath);

	lua_register( *L, "entity_generateCollisionMask",					l_entity_generateCollisionMask);

	lua_register( *L, "entity_isAnimating",								l_entity_isAnimating);
	lua_register( *L, "entity_getAnimationName",						l_entity_getAnimationName);
	lua_register( *L, "entity_getAnimName",								l_entity_getAnimationName);
	lua_register( *L, "entity_getAnimationLength",						l_entity_getAnimationLength);
	lua_register( *L, "entity_getAnimLen",								l_entity_getAnimationLength);

	lua_register( *L, "entity_setCull",									l_entity_setCull);

	lua_register( *L, "entity_setTexture",								l_entity_setTexture);
	lua_register( *L, "entity_setFillGrid",								l_entity_setFillGrid);

	lua_register( *L, "entity_interpolateTo",							l_entity_interpolateTo);
	lua_register( *L, "entity_isInterpolating",							l_entity_isInterpolating);
	lua_register( *L, "entity_isRotating",								l_entity_isRotating);


	lua_register( *L, "entity_isFlippedHorizontal",						l_entity_isFlippedHorizontal);
	lua_register( *L, "entity_isfh",									l_entity_isFlippedHorizontal);
	lua_register( *L, "entity_isfv",									l_entity_isFlippedVertical);

	lua_register( *L, "entity_setWidth",								l_entity_setWidth);
	lua_register( *L, "entity_setHeight",								l_entity_setHeight);
	lua_register( *L, "entity_push",									l_entity_push);

	lua_register( *L, "entity_alpha",									l_entity_alpha);

	lua_register( *L, "findWall",										l_findWall);


	lua_register( *L, "overrideZoom",									l_overrideZoom);
	lua_register( *L, "disableOverrideZoom",							l_disableOverrideZoom);



	lua_register( *L, "spawnAroundEntity",								l_spawnAroundEntity);

	lua_register( *L, "entity_setAffectedBySpell",						l_entity_setAffectedBySpell);

	lua_register( *L, "entity_toggleBone",								l_entity_toggleBone);

	lua_register( *L, "bone_damageFlash",								l_bone_damageFlash);
	lua_register( *L, "bone_setColor", 									l_bone_setColor);
	lua_register( *L, "bone_color", 									l_bone_setColor);
	lua_register( *L, "bone_setPosition",								l_bone_setPosition);
	lua_register( *L, "bone_rotate",									l_bone_rotate);
	lua_register( *L, "bone_rotateOffset",								l_bone_rotateOffset);
	lua_register( *L, "bone_getRotation",								l_bone_getRotation);
	lua_register( *L, "bone_offset",									l_bone_offset);

	lua_register( *L, "bone_alpha",										l_bone_alpha);

	lua_register( *L, "bone_setTouchDamage",							l_bone_setTouchDamage);
	lua_register( *L, "bone_getNormal",									l_bone_getNormal);
	lua_register( *L, "bone_getPosition",								l_bone_getPosition);
	lua_register( *L, "bone_getScale",									l_bone_getScale);
	lua_register( *L, "bone_getWorldPosition",							l_bone_getWorldPosition);
	lua_register( *L, "bone_getWorldRotation",							l_bone_getWorldRotation);



	lua_register( *L, "bone_getName",									l_bone_getName);
	lua_register( *L, "bone_isName",									l_bone_isName);
	lua_register( *L, "bone_getidx",									l_bone_getidx);
	lua_register( *L, "bone_getIndex",									l_bone_getidx);
	lua_register( *L, "node_x",											l_node_x);
	lua_register( *L, "node_y",											l_node_y);
	lua_register( *L, "node_isEntityPast",								l_node_isEntityPast);
	lua_register( *L, "node_isEntityInRange",							l_node_isEntityInRange);
	lua_register( *L, "node_isPositionIn",								l_node_isPositionIn);



	lua_register( *L, "entity_warpLastPosition",						l_entity_warpLastPosition);
	lua_register( *L, "entity_x",										l_entity_x);
	lua_register( *L, "entity_y",										l_entity_y);
	lua_register( *L, "entity_velx",									l_entity_velx);
	lua_register( *L, "entity_vely",									l_entity_vely);
	lua_register( *L, "entity_velTowards",								l_entity_velTowards);



	lua_register( *L, "updateMusic",									l_updateMusic);

	lua_register( *L, "entity_touchAvatarDamage",						l_entity_touchAvatarDamage);
	lua_register( *L, "getNaija",										l_getNaija);
	lua_register( *L, "getLi",											l_getLi);
	lua_register( *L, "setLi",											l_setLi);

	lua_register( *L, "randAngle360",									l_randAngle360);
	lua_register( *L, "randVector",										l_randVector);
	lua_register( *L, "getRandVector",									l_randVector);


	lua_register( *L, "getAvatar",										l_getNaija);

	lua_register( *L, "entity_getNearestEntity",						l_entity_getNearestEntity);
	lua_register( *L, "entity_getNearestBoneToPosition",				l_entity_getNearestBoneToPosition);

	lua_register( *L, "entity_getNearestNode",							l_entity_getNearestNode);

	lua_register( *L, "node_getNearestEntity",							l_node_getNearestEntity);
	lua_register( *L, "node_getNearestNode",							l_node_getNearestNode);


	lua_register( *L, "entity_getRotation",								l_entity_getRotation);

	lua_register( *L, "streamSfx",										l_streamSfx);

	lua_register( *L, "node_isEntityIn",								l_node_isEntityIn);



	lua_register( *L, "isLeftMouse",									l_isLeftMouse);
	lua_register( *L, "isRightMouse",									l_isRightMouse);


	lua_register( *L, "setTimerTextAlpha",								l_setTimerTextAlpha);
	lua_register( *L, "setTimerText",									l_setTimerText);


	lua_register( *L, "getWallNormal",									l_getWallNormal);
	lua_register( *L, "getLastCollidePosition",							l_getLastCollidePosition);




	// ============== deprecated
}

void ScriptInterface::shutdown()
{
	if (L)
	{
		lua_close(L);
		L = 0;
	}
	for (ParticleEffectScripts::iterator i = particleEffectScripts.begin(); i != particleEffectScripts.end(); i++)
	{
		ParticleEffectScript *p = &(*i).second;
		if (p->lua)
		{
			lua_close(p->lua);
			p->lua = 0;
		}
	}
}

void ScriptInterface::setCurrentParticleData(ParticleData *p)
{
	currentParticleData = p;
}

void ScriptInterface::setCurrentParticleEffect(ScriptedParticleEffect *p)
{
	currentParticleEffect = p;
}

bool ScriptInterface::runScriptNum(const std::string &script, const std::string &func, int num)
{
	noMoreConversationsThisRun = false;
	std::string file = script;
	if (script.find('/')==std::string::npos)
		file = "scripts/" + script + ".lua";
	file = core->adjustFilenameCase(file);
	int fail = (luaL_loadfile(L, file.c_str()));
	if (fail)
	{
		debugLog(lua_tostring(L, -1));
		debugLog("(error loading script: " + script + " from file [" + file + "])");
		//errorLog ("error in [" + file + "]");
		return false;
	}
	else
	{
		fail = lua_pcall(L, 0, 0, 0);
		if (fail)
		{
			errorLog(lua_tostring(L, -1));
			debugLog("(error doing initial run of script: " + script + ")");
		}

		lua_getfield(L, LUA_GLOBALSINDEX, func.c_str());
		lua_pushnumber(L, num);
		int fail = lua_pcall(L, 1, 0, 0);
		if (fail)
		{
			debugLog(lua_tostring(L, -1));
			debugLog("(error calling func: " + func + " in script: " + script + ")");
		}
	}
	return true;
}

bool ScriptInterface::runScript(const std::string &script, const std::string &func)
{
	noMoreConversationsThisRun = false;
	std::string file = script;
	if (script.find('/')==std::string::npos)
	{
		file = "scripts/" + script;
		if (file.find(".lua") == std::string::npos)
			file += ".lua";
	}
	file = core->adjustFilenameCase(file);
	int fail = (luaL_loadfile(L, file.c_str()));
	if (fail)
	{
		debugLog(lua_tostring(L, -1));
		debugLog("(error loading script: " + script + " from file [" + file + "])");
		//errorLog ("error in [" + file + "]");
		return false;
	}
	else
	{
		if (!func.empty())
		{
			fail = lua_pcall(L, 0, 0, 0);
			if (fail)
			{
				errorLog(lua_tostring(L, -1));
				debugLog("(error doing initial run of script: " + script + ")");
			}

			lua_getfield(L, LUA_GLOBALSINDEX, func.c_str());
			int fail = lua_pcall(L, 0, 0, 0);
			if (fail)
			{
				debugLog(lua_tostring(L, -1));
				debugLog("(error calling func: " + func + " in script: " + script + ")");
			}
		}
		else
		{
			fail = lua_pcall(L, 0, 0, 0);
			if (fail)
			{
				errorLog(lua_tostring(L, -1));
				debugLog("(error calling script: " + script + ")");
			}
		}
	}
	return true;
}

