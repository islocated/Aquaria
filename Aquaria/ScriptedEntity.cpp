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
#include "ScriptedEntity.h"
#include "DSQ.h"
#include "Game.h"
#include "Avatar.h"

extern "C"
{
	#include "lua.h"
	#include "lauxlib.h"
	#include "lualib.h"
}
#include "Shot.h"

bool ScriptedEntity::runningActivation = false;

ScriptedEntity::ScriptedEntity(const std::string &script, Vector position, EntityType et, BehaviorType bt) : CollideEntity(), Segmented(2, 26)
{	
	crushDelay = 0;
	autoSkeletalSpriteUpdate = true;
	L = 0;
	songNoteFunction = songNoteDoneFunction = true;
	addChild(&pullEmitter, PM_STATIC);

	hair = 0;
	becomeSolidDelay = false;
	strandSpacing = 10;
	animKeyFunc = true;
	preUpdateFunc = true;
	//runningActivation = false;

	expType = -1;
	eggSpawnRate = 10;
	motherDelay = 0;

	setEntityType(et);
	setBehaviorType(bt);
	eggDataIdx = -1;
	//behavior = BT_NORMAL;
	myTimer = 0;
	layer = LR_ENTITIES;
	surfaceMoveDir = 1;
	this->position = position;
	numSegments = 0;
	reverseSegments = false;
	manaBallAmount = moneyAmount = 1;
	dsq->scriptInterface.initLuaVM(&L);
	std::string file;
	this->name = script;

	if (!script.empty())
	{
		if (script[0]=='@' && dsq->mod.isActive())
		{
			file = dsq->mod.getPath() + "scripts/" + script.substr(1, script.size()) + ".lua";
			this->name = script.substr(1, script.size());
		}
		else if (dsq->mod.isActive())
		{
			file = dsq->mod.getPath() + "scripts/" + script + ".lua";

			if (!exists(file))
			{
				file = "scripts/entities/" + script + ".lua";
			}
		}
		else
		{
			file = "scripts/entities/" + script + ".lua";
		}

		if (!exists(file))
		{
			errorLog("Could not load script [" + file + "]");
		}
	}
	file = core->adjustFilenameCase(file);
	int fail = (luaL_loadfile(L, file.c_str()));
	if (fail)	luaDebugMsg("luaL_loadfile", lua_tostring(L, -1));

	

	fail = lua_pcall(L, 0, 0, 0);
	if (fail)	luaDebugMsg("firstrun", lua_tostring(L, -1));
}

void ScriptedEntity::setAutoSkeletalUpdate(bool v)
{
	autoSkeletalSpriteUpdate = v;
}

void ScriptedEntity::message(const std::string &msg, int v)
{
	if (L)
	{
		bool fail=false;
		lua_getfield(L, LUA_GLOBALSINDEX, "msg");
		luaPushPointer(L, this);
		lua_pushstring(L, msg.c_str());
		lua_pushnumber(L, v);
		fail = lua_pcall(L, 3, 0, 0);
		if (fail)	debugLog(name + " : msg : " + lua_tostring(L, -1));
	}
	Entity::message(msg, v);
}

void ScriptedEntity::warpSegments()
{
	Segmented::warpSegments(position);
}

void ScriptedEntity::init()
{	
	if (L)
	{
		bool fail=false;
		lua_getfield(L, LUA_GLOBALSINDEX, "init");
		luaPushPointer(L, this);
		fail = lua_pcall(L, 1, 0, 0);
		if (fail)	luaDebugMsg("init", lua_tostring(L, -1));
	}
	//update(0);

	Entity::init();
	/*
	if (L)
	{
		bool fail=false;
		//update(0);
	}
	*/
}

void ScriptedEntity::postInit()
{	
	if (L)
	{
		bool fail=false;
		lua_getfield(L, LUA_GLOBALSINDEX, "postInit");
		luaPushPointer(L, this);
		fail = lua_pcall(L, 1, 0, 0);
		if (fail)	luaDebugMsg("postInit", lua_tostring(L, -1));
	}

	Entity::postInit();
}

void ScriptedEntity::initEmitter(int emit, const std::string &file)
{
	if (emitters.size() <= emit)
	{
		emitters.resize(emit+1);
	}
	if (emitters[emit] != 0)
	{
		errorLog("Trying to init emitter being used");
		return;
	}
	emitters[emit] = new ParticleEffect;
	addChild(emitters[emit], PM_POINTER);
	emitters[emit]->load(file);
}

void ScriptedEntity::startEmitter(int emit)
{
	if (emitters[emit])
	{
		emitters[emit]->start();
	}
}

void ScriptedEntity::stopEmitter(int emit)
{
	if (emitters[emit])
	{
		emitters[emit]->stop();
	}
}

void ScriptedEntity::registerNewPart(RenderObject *r, const std::string &name)
{
	partMap[name] = r;
}

void ScriptedEntity::initHair(int numSegments, int segmentLength, int width, const std::string &tex)
{
	if (hair)
	{
		errorLog("Trying to init hair when hair is already present");
	}
	hair = new Hair(numSegments, segmentLength, width);
	hair->setTexture(tex);
	dsq->game->addRenderObject(hair, layer);
}

void ScriptedEntity::setHairHeadPosition(const Vector &pos)
{
	if (hair)
	{
		hair->setHeadPosition(pos);
	}
}

void ScriptedEntity::updateHair(float dt)
{
	if (hair)
	{
		hair->updatePositions();
	}
}

void ScriptedEntity::exertHairForce(const Vector &force, float dt)
{
	if (hair)
	{
		hair->exertForce(force, dt);
	}
}

void ScriptedEntity::initSegments(int numSegments, int minDist, int maxDist, std::string bodyTex, std::string tailTex, int w, int h, float taper, bool reverseSegments)
{
	this->reverseSegments = reverseSegments;
	this->numSegments = numSegments;
	this->minDist = minDist;
	this->maxDist = maxDist;
	segments.resize(numSegments);
	for (int i = segments.size()-1; i >= 0 ; i--)
	{
		Quad *q = new Quad;
		if (i == segments.size()-1)
			q->setTexture(tailTex);
		else
			q->setTexture(bodyTex);
		q->setWidthHeight(w, h);
		
		if (i > 0 && i < segments.size()-1 && taper !=0)
			q->scale = Vector(1.0-(i*taper), 1-(i*taper));
		dsq->game->addRenderObject(q, LR_ENTITIES);
		segments[i] = q;
	}
	Segmented::initSegments(position);
}

void ScriptedEntity::setupEntity(const std::string &tex, int lcode)
{
	setEntityType(ET_NEUTRAL);
	if (!tex.empty())
		setTexture(tex);

	updateCull = -1;
	manaBallAmount = moneyAmount = 0;
	expType = -1;
	setState(STATE_IDLE);

	this->layer = dsq->getEntityLayerToLayer(lcode);
}

void ScriptedEntity::setupConversationEntity(std::string name, std::string texture)
{
	this->collideWithAvatar = true;
	this->name = name;
	setEntityType(ET_NEUTRAL);
	if (texture.empty())
	{
		renderQuad = false;
		width = 128;
		height = 128;
	}
	else
		setTexture(texture);
	
	this->canBeTargetedByAvatar = 1;
	this->canStickInStream = 0;
	this->canTalkWhileMoving = 1;
	this->updateCull = -1;
	manaBallAmount = moneyAmount = 0;
	expType = -1;
	perform(STATE_IDLE);
	collideRadius = 40;
}

void ScriptedEntity::setupBasicEntity(std::string texture, int health, int manaBall, int exp, int money, int collideRadius, int state, int w, int h, int expType, bool hitEntity, int updateCull, int layer)
{
	//this->updateCull = updateCull;
	updateCull = -1;
	this->collideWithEntity = hitEntity;

	if (texture.empty())
		renderQuad = false;
	else
		setTexture(texture);
	this->health = maxHealth = health;
	this->collideRadius = collideRadius;
	setState(state);
	this->manaBallAmount = manaBall;
	this->exp = exp;
	this->moneyAmount = money;
	width = w;
	height = h;
	this->expType = expType;

	setEntityLayer(layer);
}

void ScriptedEntity::setEntityLayer(int lcode)
{
	this->layer = dsq->getEntityLayerToLayer(lcode);
}

void ScriptedEntity::initStrands(int num, int segs, int dist, int strandSpacing, Vector color)
{
	this->strandSpacing = strandSpacing;
	strands.resize(num);
	for (int i = 0; i < strands.size(); i++)
	{
		/*
		int sz = 5;
		if (i == 0 || i == strands.size()-1)
			sz = 4;
		*/
		strands[i] = new Strand(position, segs, dist);
		strands[i]->color = color;
		dsq->game->addRenderObject(strands[i], this->layer);
	}
	updateStrands(0);
}

/*
// write this if/when needed, set all strands to color (with lerp)
void ScriptedEntity::setStrandsColor(const Vector &color, float time)
{

}
*/

void ScriptedEntity::onAlwaysUpdate(float dt)
{
	Entity::onAlwaysUpdate(dt);
//	debugLog("calling updateStrands");
	updateStrands(dt);

	//HACK: this would be better in base Entity

	/*
	if (frozenTimer)
	{
	}
	*/

	if (!isEntityDead() && getState() != STATE_DEAD && getState() != STATE_DEATHSCENE && isPresent())
	{
		const bool useEV=false;

		if (useEV)
		{
			int mov = getv(EV_MOVEMENT);
			if (mov && frozenTimer)
			{
				doFriction(dt, 50);
			}
			else
			{
				// don't update friction if we're in a bubble.
				int fric = getv(EV_FRICTION);
				if (fric)
				{
					doFriction(dt, fric);
				}
			}

			switch (mov)
			{
			case 1:
				updateMovement(dt);
			break;
			case 2:
				updateCurrents(dt);
				updateMovement(dt);
			break;
			}

			if (mov)
			{
				if (hair)
				{
					setHairHeadPosition(position);
					updateHair(dt);
				}
			}


			switch (getv(EV_COLLIDE))
			{		
			case 1:
				if (skeletalSprite.isLoaded())
					dsq->game->handleShotCollisionsSkeletal(this);
				else
					dsq->game->handleShotCollisions(this);
			break;
			case 2:
				if (skeletalSprite.isLoaded())
					dsq->game->handleShotCollisionsSkeletal(this);
				else
					dsq->game->handleShotCollisions(this);

				int dmg = getv(EV_TOUCHDMG);
				if (frozenTimer > 0)
					dmg = 0;
				touchAvatarDamage(collideRadius, dmg);
			break;
			}
		}

		if (frozenTimer > 0)
		{
			pullEmitter.update(dt);

			if (!useEV)
			{
				doFriction(dt, 50);
				updateCurrents(dt);
				updateMovement(dt);

				if (hair)
				{
					setHairHeadPosition(position);
					updateHair(dt);
				}

				if (skeletalSprite.isLoaded())
					dsq->game->handleShotCollisionsSkeletal(this);
				else
					dsq->game->handleShotCollisions(this);
			}
		}

		if (isPullable() && !fillGridFromQuad)
		{
			bool doCrush = false;
			crushDelay -= dt;
			if (crushDelay < 0)
			{
				crushDelay = 0.2;
				doCrush = true;
			}
			//if ((dsq->game->avatar->position - this->position).getSquaredLength2D() < sqr(collideRadius + dsq->game->avatar->collideRadius))
			FOR_ENTITIES(i)
			{
				Entity *e = *i;
				if (e && e != this && e->life == 1 && e->ridingOnEntity != this)
				{
					if ((e->position - this->position).isLength2DIn(collideRadius + e->collideRadius))
					{
						if (this->isEntityProperty(EP_BLOCKER) && doCrush)
						{
							//bool doit = !vel.isLength2DIn(200) || (e->position.y > position.y && vel.y > 0);
							/*dsq->game->avatar->pullTarget != this ||*/ 
							/*&& */
							bool doit = !vel.isLength2DIn(64) || (e->position.y > position.y && vel.y > 0);
							if (doit)
							{
								if (e->getEntityType() == ET_ENEMY && e->isDamageTarget(DT_CRUSH))
								{
									DamageData d;
									d.damageType = DT_CRUSH;
									d.attacker = this;
									d.damage = 1;
									if (e->damage(d))
									{
										e->sound("RockHit");
										dsq->spawnParticleEffect("rockhit", e->position, 0, 0);
									}
									//e->push(vel, 0.2, 500, 0);
									Vector add = vel;
									add.setLength2D(5000*dt);
									e->vel += add;
								}
							}
						}
						Vector add = e->position - this->position;
						add.capLength2D(10000 * dt);
						e->vel += add;
						e->doCollisionAvoidance(dt, 3, 1);
					}
				}
			}
		}

		if (isPullable())
		{		
			//debugLog("movable!");
			Entity *followEntity = dsq->game->avatar;
			if (followEntity && dsq->game->avatar->pullTarget == this)
			{
				collideWithAvatar = false;
				//debugLog("followentity!");
				Vector dist = followEntity->position - this->position;
				if (dist.isLength2DIn(followEntity->collideRadius + collideRadius + 16))
				{
					vel = 0;
				}
				else if (!dist.isLength2DIn(800))
				{
					// break;
					vel.setZero();
					dsq->game->avatar->pullTarget->stopPull();
					dsq->game->avatar->pullTarget = 0;
				}
				else if (!dist.isLength2DIn(128))
				{				
					Vector v = dist;
					int moveSpeed = 1000;
					moveSpeed = 4000;
					v.setLength2D(moveSpeed);
					vel += v*dt;
					setMaxSpeed(dsq->game->avatar->getMaxSpeed());
				}
				else
				{
					if (!vel.isZero())
					{
						Vector sub = vel;
						sub.setLength2D(getMaxSpeed()*maxSpeedLerp.x*dt);
						vel -= sub;
						if (vel.isLength2DIn(100))
							vel = 0;
					}
					//vel = 0;
				}
				doCollisionAvoidance(dt, 2, 0.5);
			}
		}
	}
}

void ScriptedEntity::updateStrands(float dt)
{
	if (strands.empty()) return;
	float angle = rotation.z;
	angle = (3.14*(360-(angle-90)))/180.0;
	//angle = (180*angle)/3.14;
	float sz = (strands.size()/2);
	for (int i = 0; i < strands.size(); i++)
	{
		float diff = (i-sz)*strandSpacing;
		if (diff < 0)
			strands[i]->position = position - Vector(sin(angle)*fabs(diff), cos(angle)*fabs(diff));
		else
			strands[i]->position = position + Vector(sin(angle)*diff, cos(angle)*diff);
		if (dt > 0)
			strands[i]->update(dt);
	}
}

void ScriptedEntity::destroy()
{
	//debugLog("calling target died");	

	CollideEntity::destroy();
	/*
	// spring plant might already be destroyed at this point (end of state)
	// could add as child?
	if (springPlant)
	{
		//springPlant->life = 0.1;
		springPlant->alpha = 0;
	}
	*/
	/*
	if (hair)
	{
		//dsq->removeRenderObject(hair, DESTROY_RENDER_OBJECT);
		dsq->game->removeRenderObject(hair);
		hair->destroy();
		delete hair;
		hair = 0;
	}
	*/
	if (L)
	{
		lua_close(L);
		L = 0;
	}
}

void ScriptedEntity::song(SongType songType)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "song");
	luaPushPointer(L, this);
	lua_pushnumber(L, int(songType));
	int fail = lua_pcall(L, 2, 0, 0);
	if (fail)	debugLog(name + " : " + lua_tostring(L, -1));
}

void ScriptedEntity::shiftWorlds(WorldType lastWorld, WorldType worldType)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "shiftWorlds");
	luaPushPointer(L, this);
	lua_pushnumber(L, int(lastWorld));
	lua_pushnumber(L, int(worldType));
	int fail = lua_pcall(L, 3, 0, 0);
	if (fail) debugLog(name + " : " + lua_tostring(L, -1) + " shiftWorlds");
}

void ScriptedEntity::startPull()
{	
	Entity::startPull();
	beforePullMaxSpeed = getMaxSpeed();
	becomeSolidDelay = false;
	debugLog("HERE!");
	if (isEntityProperty(EP_BLOCKER))
	{
		//debugLog("property set!");
		fillGridFromQuad = false;
		dsq->game->reconstructEntityGrid();
	}
	else
	{
		//debugLog("property not set!");
	}
	pullEmitter.load("Pulled");
	pullEmitter.start();

	// HACK: move this to the lower level at some point

	if (isEntityProperty(EP_BLOCKER))
	{
		FOR_ENTITIES(i)
		{
			Entity *e = *i;
			if (e != this && e->getEntityType() != ET_AVATAR && e->isv(EV_CRAWLING, 1))
			{
				if ((e->position - position).isLength2DIn(collideRadius+e->collideRadius+32))
				{
					debugLog(e->name + ": is now riding on : " + name);
					e->ridingOnEntity = this;
					e->ridingOnEntityOffset = e->position - position;
					e->ridingOnEntityOffset.setLength2D(collideRadius);
				}
			}
		}
	}
}

void ScriptedEntity::sporesDropped(const Vector &pos, int type)
{
	lua_getfield(L, LUA_GLOBALSINDEX, "sporesDropped");
	luaPushPointer(L, this);
	lua_pushnumber(L, pos.x);
	lua_pushnumber(L, pos.y);
	lua_pushnumber(L, type);
	int fail = lua_pcall(L, 4, 0, 0);
	//if (fail) debugLog(name + " : " + lua_tostring(L, -1) + " sporesDropped");
}

void ScriptedEntity::stopPull()
{
	Entity::stopPull();
	pullEmitter.stop();
	setMaxSpeed(beforePullMaxSpeed);
}

void ScriptedEntity::onUpdate(float dt)
{
	BBGE_PROF(ScriptedEntity_onUpdate);

	/*
	if (preUpdateFunc)
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "preUpdate");
		luaPushPointer(L, this);
		lua_pushnumber(L, dt);
		int fail = lua_pcall(L, 2, 0, 0);
		if (fail)
		{
			debugLog(name + " : preUpdate : " + lua_tostring(L, -1));
			preUpdateFunc = false;
		}
	}
	*/

	if (!autoSkeletalSpriteUpdate)
		skeletalSprite.ignoreUpdate = true;

	CollideEntity::onUpdate(dt);

	if (!autoSkeletalSpriteUpdate)
		skeletalSprite.ignoreUpdate = false;
	//updateStrands(dt);

	if (becomeSolidDelay)
	{
		if (vel.isLength2DIn(5))
		{
			if (!isEntityInside())
			{
				becomeSolid();
				becomeSolidDelay = false;
			}
		}
	}


	if (life != 1 || isEntityDead()) return;
	//if (!dsq->scriptInterface.setCurrentEntity(this))	return;


	if (myTimer > 0)
	{
		myTimer -= dt;
		if (myTimer <= 0)
		{
			myTimer = 0;
			onExitTimer();
		}
	}

	if (this->isEntityDead() || this->getState() == STATE_DEATHSCENE || this->getState() == STATE_DEAD)
	{
		return;
	}
	lua_getfield(L, LUA_GLOBALSINDEX, "update");
	luaPushPointer(L, this);
	lua_pushnumber(L, dt);
	int fail = lua_pcall(L, 2, 0, 0);
	if (fail)
	{
		debugLog(name + " : update : " + lua_tostring(L, -1));
	}

	if (numSegments > 0)
	{
		updateSegments(position, reverseSegments);
		updateAlpha(alpha.x);
	}

	/*
	//HACK: if this is wanted (to support moving placed entities), then 
	// springPlant has to notify ScriptedEntity when it is deleted / pulled out
	if (springPlant)
	{
		springPlant->position = this->position;
	}
	*/
}

void ScriptedEntity::resetTimer(float t)
{
	myTimer = t;
}

void ScriptedEntity::stopTimer()
{
	myTimer = 0;
}

void ScriptedEntity::onExitTimer()
{
	lua_getfield(L, LUA_GLOBALSINDEX, "exitTimer");
	luaPushPointer(L, this);
	int fail = lua_pcall(L, 1, 0, 0);
	if (fail)	debugLog(this->name + " : " + std::string(lua_tostring(L, -1)) + " exitTimer");
}

void ScriptedEntity::onAnimationKeyPassed(int key)
{
	if (animKeyFunc)
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "animationKey");
		luaPushPointer(L, this);
		lua_pushnumber(L, key);
		int fail = lua_pcall(L, 2, 0, 0);
		if (fail)
		{
			debugLog(this->name + " : " + std::string(lua_tostring(L, -1)) + " animationKey");
			animKeyFunc = false;
		}
	}

	Entity::onAnimationKeyPassed(key);
}

void ScriptedEntity::lightFlare()
{
	if (!isEntityDead())
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "lightFlare");
		luaPushPointer(L, this);
		//int fail = 
		lua_pcall(L, 1, 1, 0);
	}
}

bool ScriptedEntity::damage(const DamageData &d)
{
	if (d.damageType == DT_NONE)	return false;
	bool doDefault = true;
	lua_getfield(L, LUA_GLOBALSINDEX, "damage");
	//(me, attacker, bone, spellType, dmg)
	luaPushPointer(L, this);
	luaPushPointer(L, d.attacker);
	luaPushPointer(L, d.bone);
	lua_pushnumber(L, int(d.damageType));
	lua_pushnumber(L, d.damage);
	lua_pushnumber(L, d.hitPos.x);
	lua_pushnumber(L, d.hitPos.y);
	luaPushPointer(L, d.shot);
	//float mult = 0;
	int fail = lua_pcall(L, 8, 1, 0);
	if (fail)
	{
		debugLog(name + ": damage function failed");
		//debugLog(this->name + " : " + std::string(lua_tostring(L, -1)) + " hit");
	}
	else
	{
		//mult = lua_tonumber(L, -1);
		doDefault = lua_toboolean(L, -1);
		

		/*
		std::ostringstream os;
		os << "doDefault: " << doDefault << " mult: " << mult;
		debugLog(os.str());
		*/
	}
	/*
	DamageData pass = d;
	pass.mult = mult;
	*/

	if (doDefault)
	{
		//debugLog("doing default damage");
		return Entity::damage(d);
	}

	//debugLog("not doing default damage");
	return false;
}

void ScriptedEntity::onHitEntity(const CollideData &c)
{
	CollideEntity::onHitEntity(c);

	lua_getfield(L, LUA_GLOBALSINDEX, "hitEntity");
	//(me, attacker, bone, spellType, dmg)
	luaPushPointer(L, this);
	luaPushPointer(L, c.entity);
	luaPushPointer(L, c.bone);
	int fail = lua_pcall(L, 3, 1, 0);
	if (fail)
	{
	}
	else
	{
	//	doDefault = lua_toboolean(L, -1);
	}
}

void ScriptedEntity::songNote(int note)
{
	Entity::songNote(note);

	if (songNoteFunction)
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "songNote");
		luaPushPointer(L, this);
		lua_pushnumber(L, note);
		int fail = lua_pcall(L, 2, 0, 0);
		if (fail)
		{
			songNoteFunction=false;
			debugLog(this->name + " : " + std::string(lua_tostring(L, -1)) + " songNote");
		}
	}
}

void ScriptedEntity::songNoteDone(int note, float len)
{
	Entity::songNoteDone(note, len);
	if (songNoteDoneFunction)
	{
		lua_getfield(L, LUA_GLOBALSINDEX, "songNoteDone");
		luaPushPointer(L, this);
		lua_pushnumber(L, note);
		lua_pushnumber(L, len);
		int fail = lua_pcall(L, 3, 0, 0);
		if (fail)
		{
			songNoteDoneFunction=false;
			debugLog(this->name + " : " + std::string(lua_tostring(L, -1)) + " songNoteDone");
		}
	}
}

bool ScriptedEntity::isEntityInside()
{
	bool v = false;
	int avatars = 0;
	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e->getEntityType() == ET_AVATAR)
			avatars ++;
		if (e && e->life == 1 && e != this && e->ridingOnEntity != this)
		{			
			if (isCoordinateInside(e->position))
			{				
				/*
				Vector diff = (e->position - position);
				diff.setLength2D(100);
				e->vel += diff;
				*/
				v = true;
			}
		}
		
	}
	return v;
}

void ScriptedEntity::becomeSolid()
{
	//vel = 0;
	float oldRot = 0;
	bool doRot=false;
	Vector n = dsq->game->getWallNormal(position);
	if (!n.isZero())
	{
		oldRot = rotation.z;
		rotateToVec(n, 0);
		doRot = true;
	}
	fillGridFromQuad = true;
	dsq->game->reconstructEntityGrid();

	FOR_ENTITIES(i)
	{
		Entity *e = *i;
		if (e->ridingOnEntity == this)
		{
			e->ridingOnEntity = 0;
			e->moveOutOfWall();
			// if can't get the rider out of the wall, kill it
			if (dsq->game->isObstructed(TileVector(e->position)))
			{
				e->setState(STATE_DEAD);
			}
		}
	}

	if (doRot)
	{
		rotation.z = oldRot;
		rotateToVec(n, 0.01);
	}
}

void ScriptedEntity::onHitWall()
{
	if (isEntityProperty(EP_BLOCKER) && !fillGridFromQuad && dsq->game->avatar->pullTarget != this)
	{
		becomeSolidDelay = true;
	}
	
	if (isEntityProperty(EP_BLOCKER) && !fillGridFromQuad)
	{
		Vector n = dsq->game->getWallNormal(position);
		if (!n.isZero())
		{
			rotateToVec(n, 0.2);
		}
	}

	CollideEntity::onHitWall();


	lua_getfield(L, LUA_GLOBALSINDEX, "hitSurface");
	luaPushPointer(L, this);
	int fail = lua_pcall(L, 1, 0, 0);
	if (fail)	debugLog(this->name + " : " + std::string(lua_tostring(L, -1)) + " hitSurface");
}

void ScriptedEntity::activate()
{	
	if (dsq->conversationDelay > 0) return;
	if (runningActivation) return;
	Entity::activate();
	/*
	if (dsq->game->avatar)
	{
		Avatar *a = dsq->game->avatar;
		if (a->position.x < this->position.x)
		{
			if (!a->isFlippedHorizontal())
				a->flipHorizontal();
		}
		else
		{
			if (a->isFlippedHorizontal())
				a->flipHorizontal();
		}
		if (getEntityType() == ET_NEUTRAL)
			flipToTarget(dsq->game->avatar->position);
	}
	*/
	
	runningActivation = true;
	dsq->conversationDelay = 1.5;
	if (!dsq->scriptInterface.setCurrentEntity(this)) return;
	lua_getfield(L, LUA_GLOBALSINDEX, "activate");
	luaPushPointer(L, this);
	int fail = lua_pcall(L, 1, 0, 0);
	if (fail)	luaDebugMsg("activate", lua_tostring(L, -1));
	runningActivation = false;
}

void ScriptedEntity::shotHitEntity(Entity *hit, Shot *shot, Bone *bone)
{
	Entity::shotHitEntity(hit, shot, bone);

	lua_getfield(L, LUA_GLOBALSINDEX, "shotHitEntity");
	luaPushPointer(L, this);
	luaPushPointer(L, hit);
	luaPushPointer(L, shot);
	luaPushPointer(L, bone);
	lua_pcall(L, 4, 0, 0);
}

void ScriptedEntity::entityDied(Entity *e)
{
	CollideEntity::entityDied(e);

	lua_getfield(L, LUA_GLOBALSINDEX, "entityDied");
	luaPushPointer(L, this);
	luaPushPointer(L, e);
	//int fail = 
	lua_pcall(L, 2, 0, 0);
	//if (fail)	luaDebugMsg("entityDied", lua_tostring(L, -1));
}

void ScriptedEntity::luaDebugMsg(const std::string &func, const std::string &msg)
{
	debugLog("luaScriptError: " + name + " : " + func + " : " + msg);
}

void ScriptedEntity::onDieNormal()
{
	Entity::onDieNormal();
	lua_getfield(L, LUA_GLOBALSINDEX, "dieNormal");
	luaPushPointer(L, this);
	int fail = lua_pcall(L, 1, 0, 0);
}

void ScriptedEntity::onDieEaten()
{
	Entity::onDieEaten();
	lua_getfield(L, LUA_GLOBALSINDEX, "dieEaten");
	luaPushPointer(L, this);
	int fail = lua_pcall(L, 1, 0, 0);
}

void ScriptedEntity::onEnterState(int action)
{
	CollideEntity::onEnterState(action);
	
	//if (!dsq->scriptInterface.setCurrentEntity(this)) return;
	lua_getfield(L, LUA_GLOBALSINDEX, "enterState");
	luaPushPointer(L, this);
	int fail = lua_pcall(L, 1, 0, 0);
	if (fail) luaDebugMsg("enterState", lua_tostring(L, -1));
	switch(action)
	{
	case STATE_DEAD:
		if (!isGoingToBeEaten())
		{			
			doDeathEffects(manaBallAmount, moneyAmount);
			dsq->spawnParticleEffect(deathParticleEffect, position);
			onDieNormal();
		}
		else
		{
			// eaten
			doDeathEffects(0, 0);
			onDieEaten();
		}
		destroySegments(1);


		for (int i = 0; i < strands.size(); i++)
		{
			strands[i]->safeKill();
		}
		strands.clear();

		// BASE ENTITY CLASS WILL HANDLE CLEANING UP HAIR
		/*
		if (hair)
		{
			hair->setLife(1.0);
			hair->setDecayRate(10);
			hair->fadeAlphaWithLife = true;
			hair = 0;
		}
		*/
	break;
	}
}

void ScriptedEntity::onExitState(int action)
{
	
	if (!dsq->scriptInterface.setCurrentEntity(this)) return;
	lua_getfield(L, LUA_GLOBALSINDEX, "exitState");
	luaPushPointer(L, this);
	int fail = lua_pcall(L, 1, 0, 0);
	if (fail)	luaDebugMsg("exitState", lua_tostring(L, -1));

	CollideEntity::onExitState(action);
}

