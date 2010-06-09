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
#ifndef __render_object__
#define __render_object__

#include "Base.h"
#include "Texture.h"
#include "Flags.h"

class Core;
class StateData;

enum RenderObjectFlags
{
	RO_CLEAR			= 0x00,
	RO_RENDERBORDERS	= 0x01,
	RO_NEXT				= 0x02,
	RO_MOTIONBLUR		= 0x04
};

enum AutoSize
{
	AUTO_VIRTUALWIDTH		= -101,
	AUTO_VIRTUALHEIGHT		= -102
};

enum ParentManaged
{
	PM_NONE					= 0,
	PM_POINTER				= 1,
	PM_STATIC				= 2
};

enum ChildOrder
{
	CHILD_BACK				= 0,
	CHILD_FRONT				= 1
};

enum RenderBeforeParent
{
	RBP_NONE				= -1,
	RBP_OFF					= 0,
	RBP_ON					= 1
};

struct MotionBlurFrame
{
	Vector position;
	float rotz;
};

typedef std::vector<RectShape> CollideRects;

class RenderObjectLayer;

class RenderObject
{
public:
	friend class Core;
	RenderObject();
	virtual ~RenderObject();
	virtual void render();

	static RenderObjectLayer *rlayer;

	enum AddRefChoice { NO_ADD_REF = 0, ADD_REF = 1};

	void setTexturePointer(Texture *t, AddRefChoice addRefChoice)
	{
		this->texture = t;
		if (addRefChoice == ADD_REF)
			texture->addRef();
		onSetTexture();
	}

	void setStateDataObject(StateData *state);
	void setTexture(const std::string &name);

	void toggleAlpha(float t = 0.2);
	void matrixChain();

	virtual void update(float dt);
	bool isDead() {return _dead;}

	InterpolatedVector position, scale, color, alpha, rotation;
	InterpolatedVector offset, rotationOffset, internalOffset, beforeScaleOffset;
	InterpolatedVector velocity, gravity;

	Texture *texture;

	int mode;

	bool fadeAlphaWithLife;
	bool blendEnabled;

	void setLife(float life)
	{
		maxLife = this->life = life;
	}
	void setDecayRate(float decayRate)
	{
		this->decayRate = decayRate;
	}
	void setBlendType (int bt)
	{
		blendType = bt;
	}

	//enum DestroyType { RANDOM=0, REMOVE_STATE };
	virtual void destroy();

	int blendType;
	float life;
	float lifeAlphaFadeMultiplier;
	enum BlendTypes { BLEND_DEFAULT = 0, BLEND_ADD, BLEND_SUB };
	float followCamera;

	void addChild(RenderObject *r, ParentManaged pm, RenderBeforeParent rbp = RBP_NONE, ChildOrder order = CHILD_BACK);
	void removeChild(RenderObject *r);
	void removeAllChildren();
	void recursivelyRemoveEveryChild();

	bool useColor;
	bool renderBeforeParent;
	bool updateAfterParent;

	bool followXOnly;
	bool renderOrigin;

	Vector getRealPosition();
	Vector getRealScale();

	virtual double getSortDepth();

	StateData *getStateData();

	float updateMultiplier;
	EventPtr deathEvent;

	void setPositionSnapTo(InterpolatedVector *positionSnapTo);
	InterpolatedVector *positionSnapTo;

	static bool renderCollisionShape;
	static bool integerizePositionForRender;

	virtual bool isOnScreen();

	bool shareAlphaWithChildren;
	bool shareColorWithChildren;

	bool isCoordinateInRadius(const Vector &pos, float r);

	void copyProperties(RenderObject *target);

	const RenderObject &operator=(const RenderObject &r);

	
	void enableProjectCollision();
	void disableProjectCollision();
	//DestroyType destroyType;
	typedef std::list<RenderObject*> Children;
	Children children, childGarbage;

	void toggleCull(bool value);
	
	int layer;
	int updateCull;
	bool cull;
	void safeKill();

	//Flags flags;

	void enqueueChildDeletion(RenderObject *r);

#ifdef BBGE_BUILD_DIRECTX
	bool useDXTransform;
	//D3DXMATRIX matrix;
#endif

	bool renderBorders;

	virtual void flipHorizontal();
	virtual void flipVertical();

	bool isfh() { return _fh; }
	bool isfv() { return _fv; }

	// recursive
	bool isfhr();																	
	bool isfvr();

	int getIdx() { return idx; }
	void setIdx(int idx) { this->idx = idx; }
	void moveToFront();
	void moveToBack();

	virtual int getCullRadius();

	int getTopLayer();

	void shareColor(const Vector &color);
	void multiplyColor(const Vector &color, bool inv=false);
	void multiplyAlpha(const Vector &alpha, bool inv=false);

	void enableMotionBlur(int sz=10, int off=5);
	void disableMotionBlur();
	static bool renderPaths;
	Vector getWorldPosition();
	int collideRadius;
	Vector collidePosition;
	Vector getWorldCollidePosition(const Vector &vec=Vector(0,0,0));
	Vector getInvRotPosition(const Vector &vec);
	bool useCollisionMask;
	Vector collisionMaskHalfVector;
	std::vector<Vector> collisionMask;
	std::vector<Vector> transformedCollisionMask;

	CollideRects collisionRects;
	bool isPieceFlippedHorizontal();

	RenderObject *getTopParent();
	int collisionMaskRadius;
	int touchDamage;

	virtual void onAnimationKeyPassed(int key){}

	Vector getAbsoluteRotation();
	float getWorldRotation();
	Vector getNormal();
	Vector getFollowCameraPosition();
	float alphaMod;
	Vector getForward();
	void setOverrideCullRadius(int ovr);
	void setRenderPass(int pass) { renderPass = pass; }
	int getRenderPass() { return renderPass; }
	void setOverrideRenderPass(int pass) { overrideRenderPass = pass; }
	int getOverrideRenderPass() { return overrideRenderPass; }
	enum { RENDER_ALL=314, OVERRIDE_NONE=315 };

	void lookAt(const Vector &pos, float t, float minAngle, float maxAngle, float offset=0);
	bool ignoreUpdate;
	RenderObject *getParent();
	void applyBlendType();
	void fhTo(bool fh);
	void addDeathNotify(RenderObject *r);
	virtual void unloadDevice();
	virtual void reloadDevice();

	Vector getCollisionMaskNormal(int index);

	bool useOldDT;
	
	static int lastTextureApplied;
	static bool lastTextureRepeat;

protected:
	virtual void onFH(){}
	virtual void onFV(){}
	virtual void onDestroy(){}
	virtual void onSetTexture(){}
	virtual void onRender(){}
	virtual void onUpdate(float dt);
	virtual void deathNotify(RenderObject *r);
	virtual void onEndOfLife() {}

	void addDeathNotifyInternal(RenderObject *r);
	// spread parentManagedStatic flag to the entire child tree
	void propogateParentManagedStatic();
	void propogateAlpha();
	void updateLife(float dt);

	inline void renderCall();

	ParentManaged pm;
	typedef std::list<RenderObject*> RenderObjectList;
	RenderObjectList deathNotifications;
	int overrideRenderPass;
	int renderPass;
	int overrideCullRadius;
	bool repeatTexture;
	float motionBlurTransitionTimer;
	int motionBlurFrameOffsetCounter, motionBlurFrameOffset;
	std::vector<MotionBlurFrame>motionBlurPositions;
	bool motionBlur, motionBlurTransition;

	int idx;
	bool _fv, _fh;
	bool rotateFirst;
	RenderObject *parent;
	StateData *stateData;
	float decayRate;
	float maxLife;
	bool _dead;
	static InterpolatedVector savePosition;
};

#endif
