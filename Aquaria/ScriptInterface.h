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

#include "../BBGE/Base.h"

struct lua_State;

class Entity;
class CollideEntity;
class ScriptedParticleEffect;
class ParticleData;
class ScriptedEntity;

struct ParticleEffectScript
{
	lua_State* lua;
	std::string name;
	int idx;
};


class ScriptInterface
{
public:
	void init();
	void loadParticleEffectScripts();
	void shutdown();
	void setCurrentParticleEffect(ScriptedParticleEffect *e);
	bool setCurrentEntity (Entity *e);
	void setCurrentParticleData(ParticleData *p);
	ScriptedParticleEffect *getCurrentParticleEffect() { return currentParticleEffect; }


	ParticleData *getCurrentParticleData() { return currentParticleData; }
	lua_State *L;
	void initLuaVM(lua_State **L);
	bool runScript(const std::string &script, const std::string &func);
	bool runScriptNum(const std::string &script, const std::string &func, int num);
	typedef std::map<std::string, ParticleEffectScript> ParticleEffectScripts;
	ParticleEffectScripts particleEffectScripts;
	ParticleEffectScript *getParticleEffectScriptByIdx(int idx);
	//bool simpleConversation(const std::string &file);
	bool noMoreConversationsThisRun;

	//ScriptedEntity *se;
	//CollideEntity *collideEntity;
	//int currentEntityTarget;
	//Entity *getCurrentEntity() { return currentEntity; }
protected:

	ParticleData* currentParticleData;
	ScriptedParticleEffect* currentParticleEffect;
	Entity *currentEntity;
};
extern ScriptInterface *si;

void luaPushPointer(lua_State *L, void *ptr);


