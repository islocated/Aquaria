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
#include "RenderObject.h"
#include "Core.h"
#include "MathFunctions.h"

#include <assert.h>

bool	RenderObject::renderCollisionShape			= false;
bool	RenderObject::integerizePositionForRender	= false;
int		RenderObject::lastTextureApplied			= 0;
bool	RenderObject::lastTextureRepeat				= false;
bool	RenderObject::renderPaths					= false;

const bool RENDEROBJECT_SHAREATTRIBUTES				= true;
const bool RENDEROBJECT_FASTTRANSFORM				= false;

RenderObjectLayer *RenderObject::rlayer				= 0;

InterpolatedVector RenderObject::savePosition;

void RenderObject::toggleAlpha(float t)
{
	if (alpha.x < 0.5)
		alpha.interpolateTo(1,t);
	else
		alpha.interpolateTo(0,t);
}

int RenderObject::getTopLayer()
{
	if (parent)
	{
		return parent->getTopLayer();
	}
	return layer;
}

RenderObject *RenderObject::getParent()
{
	return parent;
}

void RenderObject::applyBlendType()
{
#ifdef BBGE_BUILD_OPENGL
	if (blendEnabled)
	{
		glEnable(GL_BLEND);
		switch (blendType)
		{
		case BLEND_DEFAULT:
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
		case BLEND_ADD:
			glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		break;
		case BLEND_SUB:
			glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
		break;
		}
	}
	else
	{
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
	}
#endif
#ifdef BBGE_BUILD_DIRECTX
	if (blendEnabled)
	{
		core->getD3DDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); 
		switch (blendType)
		{
		case BLEND_DEFAULT:
			core->getD3DDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			core->getD3DDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
		case BLEND_ADD:
			core->getD3DDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
			core->getD3DDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
			//glBlendFunc(GL_SRC_ALPHA,GL_ONE);
		break;
		case BLEND_SUB:
			core->getD3DDevice()->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
			core->getD3DDevice()->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);
			//glBlendFunc(GL_ZERO, GL_SRC_ALPHA);
		break;
		}
		
		core->getD3DDevice()->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		core->getD3DDevice()->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	}
	else
	{
		core->getD3DDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		/*
		glDisable(GL_BLEND);
		glDisable(GL_ALPHA_TEST);
		*/
	}
#endif
}

void RenderObject::shareColor(const Vector &color)
{
	this->color = color;
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->shareColor(color);
	}
}

void RenderObject::multiplyColor(const Vector &color, bool inv)
{
	if (inv)
		this->color /= color;
	else
		this->color *= color;
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->multiplyColor(color, inv);
	}
}

void RenderObject::multiplyAlpha(const Vector &alpha, bool inv)
{
	if (inv)
		this->alpha.x /= alpha.x;
	else
		this->alpha.x *= alpha.x;
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->multiplyAlpha(alpha, inv);
	}
}

RenderObject::RenderObject()
{
	useOldDT = false;

	updateAfterParent = false;
	ignoreUpdate = false;
	overrideRenderPass = OVERRIDE_NONE;
	renderPass = 0;
	overrideCullRadius = 0;
	repeatTexture = false;
	alphaMod = 1;
	collisionMaskRadius = 0;
	collideRadius = 0;
	useCollisionMask = false;
	motionBlurTransition = false;
	motionBlurFrameOffsetCounter = 0;
	motionBlurFrameOffset = 0;
	motionBlur = false;
	idx = -1;
	renderBorders = false;
#ifdef BBGE_BUILD_DIRECTX
	useDXTransform = false;
#endif
	_fv = false;
	_fh = false;
	updateCull = -1;
	rotateFirst = true;
	layer = LR_NONE;
	cull = true;

	pm = PM_NONE;

	positionSnapTo = 0;

	updateMultiplier = 1;
	blendEnabled = true;
	texture = 0; scale = Vector(1,1,1); color=Vector(1,1,1); alpha.x=1; mode=0;
	life = maxLife = 1;
	decayRate = 0;
	_dead = false;
	fadeAlphaWithLife = false;
	blendType = 0;
	lifeAlphaFadeMultiplier = 1;
	followCamera = 0;
	stateData = 0;
	parent = 0;
	useColor = true;
	renderBeforeParent = false;
	followXOnly = false;
	renderOrigin = false;
	shareAlphaWithChildren = false;
	shareColorWithChildren = false;
	touchDamage = 0;	
	motionBlurTransitionTimer = 0;
}

RenderObject::~RenderObject()
{
}

Vector RenderObject::getWorldPosition()
{
	return getWorldCollidePosition();
}

RenderObject* RenderObject::getTopParent()
{
	RenderObject *p = parent;
	RenderObject *lastp=0;
	while (p)
	{
		lastp = p;
		p = p->parent;
	}
	return lastp;
}

bool RenderObject::isPieceFlippedHorizontal()
{
	RenderObject *p = getTopParent();
	if (p)
		return p->isfh();
	return isfh();
}


Vector RenderObject::getInvRotPosition(const Vector &vec)
{
#ifdef BBGE_BUILD_OPENGL
	glPushMatrix();
	glLoadIdentity();

	std::vector<RenderObject*>chain;
	RenderObject *p = this;
	while(p)
	{
		chain.push_back(p);
		p = p->parent;
	}
	
	for (int i = chain.size()-1; i >= 0; i--)
	{
		glRotatef(-(chain[i]->rotation.z+chain[i]->rotationOffset.z), 0, 0, 1);
	
		if (chain[i]->isfh())
		{
			//glDisable(GL_CULL_FACE);
			glRotatef(180, 0, 1, 0);
		}
	}

	if (vec.x != 0 || vec.y != 0)
	{
		//glRotatef(this->rotation.z, 0,0,1,this->rotation.z);
		glTranslatef(vec.x, vec.y, 0);
	}

	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float x = m[12];
	float y = m[13];
	float z = m[14];

	glPopMatrix();
	return Vector(x,y,z);
#elif BBGE_BUILD_DIRECTX
	return vec;
#endif
}

void RenderObject::matrixChain()
{	
	std::vector<RenderObject*>chain;
	RenderObject *p = this;
	while(p)
	{
		chain.push_back(p);
		p = p->parent;
	}
	
#ifdef BBGE_BUILD_OPENGL
	for (int i = chain.size()-1; i >= 0; i--)
	{		
		glTranslatef(chain[i]->position.x, chain[i]->position.y, 0);
		glTranslatef(chain[i]->offset.x, chain[i]->offset.y, 0);
		glRotatef(chain[i]->rotation.z+chain[i]->rotationOffset.z, 0, 0, 1);
		glTranslatef(chain[i]->beforeScaleOffset.x, chain[i]->beforeScaleOffset.y, 0);		
		glScalef(chain[i]->scale.x, chain[i]->scale.y, 0);
		
		if (chain[i]->isfh())
		{
			//glDisable(GL_CULL_FACE);
			glRotatef(180, 0, 1, 0);
		}
		glTranslatef(chain[i]->internalOffset.x, chain[i]->internalOffset.y, 0);
		
	}

	if (collidePosition.x != 0 || collidePosition.y != 0)
		glTranslatef(collidePosition.x, collidePosition.y, 0);
#endif
}

float RenderObject::getWorldRotation()
{
	Vector up = getWorldCollidePosition(Vector(0,1));
	Vector orig = getWorldPosition();
	float rot = 0;
	MathFunctions::calculateAngleBetweenVectorsInDegrees(orig, up, rot);
	return rot;
}

Vector RenderObject::getWorldCollidePosition(const Vector &vec)
{
#ifdef BBGE_BUILD_OPENGL
	glPushMatrix();
	glLoadIdentity();

	matrixChain();

	if (vec.x != 0 || vec.y != 0)
	{
		glTranslatef(vec.x, vec.y, 0);
	}

	float m[16];
	glGetFloatv(GL_MODELVIEW_MATRIX, m);
	float x = m[12];
	float y = m[13];
	float z = m[14];

	glPopMatrix();
	return Vector(x,y,z);
#elif BBGE_BUILD_DIRECTX
	return vec;
#endif
}

void RenderObject::fhTo(bool fh)
{
	if ((fh && !_fh) || (!fh && _fh))
	{
		flipHorizontal();
	}
}

void RenderObject::flipHorizontal()
{
	bool wasFlippedHorizontal = _fh;

	_fh = !_fh;

	if (wasFlippedHorizontal != _fh)
	{
		onFH();
	}
	/*
	if (wasFlippedHorizontal && !_fh)
		for (int i = 0; i < this->collisionMask.size(); i++)
			collisionMask[i].x = -collisionMask[i].x;
	else if (!wasFlippedHorizontal && _fh)
		for (int i = 0; i < this->collisionMask.size(); i++)
			collisionMask[i].x = -collisionMask[i].x;
	*/
}

void RenderObject::flipVertical()
{
	//bool wasFlippedVertical = _fv;
	_fv = !_fv;
	/*
	if (wasFlippedVertical && !_fv)
		for (int i = 0; i < this->collisionMask.size(); i++)
			collisionMask[i].y = -collisionMask[i].y;
	else if (!wasFlippedVertical && _fv)
		for (int i = 0; i < this->collisionMask.size(); i++)
			collisionMask[i].y = -collisionMask[i].y;
	*/
}

void RenderObject::destroy()
{
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		// must do this first
		// otherwise child will try to remove THIS
		(*i)->parent = 0;
		switch ((*i)->pm)
		{
		case PM_STATIC:
			(*i)->destroy();
			break;
		case PM_POINTER:
			(*i)->destroy();
			delete (*i);
			break;
		}
	}
	children.clear();

	if (parent)
	{
		parent->removeChild(this);
		parent = 0;
	}
	if (texture)
	{
		texture->removeRef(); 
		texture = 0;
	}
}

void RenderObject::copyProperties(RenderObject *target)
{
	this->color						= target->color;
	this->position					= target->position;
	this->alpha						= target->alpha;
	this->velocity					= target->velocity;
}

const RenderObject &RenderObject::operator=(const RenderObject &r)
{
	errorLog("Operator= not defined for RenderObject. Use 'copyProperties'");
	return *this;
}

Vector RenderObject::getRealPosition()
{
	if (parent)
	{
		return position + offset + parent->getRealPosition();
	}
	return position + offset;
}

Vector RenderObject::getRealScale()
{
	if (parent)
	{
		return scale * parent->getRealScale();
	}
	return scale;
}

void RenderObject::setStateDataObject(StateData *state)
{
	stateData = state;
}


void RenderObject::toggleCull(bool value)
{
	cull = value;
}

void RenderObject::moveToFront()
{
	if (layer != -1)
		core->renderObjectLayers[this->layer].moveToFront(this);
}

void RenderObject::moveToBack()
{
	if (layer != -1)
		core->renderObjectLayers[this->layer].moveToBack(this);
}

void RenderObject::enableMotionBlur(int sz, int off)
{
	motionBlur = true;
	motionBlurPositions.resize(sz);
	motionBlurFrameOffsetCounter = 0;
	motionBlurFrameOffset = off;
	for (int i = 0; i < motionBlurPositions.size(); i++)
	{
		motionBlurPositions[i].position = position;
		motionBlurPositions[i].rotz = rotation.z;
	}
}

void RenderObject::disableMotionBlur()
{
	motionBlurTransition = true;
	motionBlurTransitionTimer = 1.0;
	motionBlur = false;
}

bool RenderObject::isfhr()
{
	if (parent)
		return parent->isfhr();
	else
		return this->isfh();
}

bool RenderObject::isfvr()
{
	if (parent)
		return parent->isfvr();
	else
		return this->isfv();
}

void RenderObject::render()
{
	//HACK: possible optimization here: 
	/*
	if (layer != LR_NONE && children.empty() && renderPass != RENDER_ALL)
	{
		RenderObjectLayer *l = core->getRenderObjectLayer(this->layer);
		if (l && l->currentPass != renderPass) return;
	}
	*/
	
	/// new (breaks anything?)
	if (alpha.x == 0 || alphaMod == 0) return;

	if (motionBlur || motionBlurTransition)
	{
		Vector oldPos = position;
		float oldAlpha = alpha.x;
		float oldRotZ = rotation.z;
		for (int i = 0; i < motionBlurPositions.size(); i++)
		{
			position = motionBlurPositions[i].position;
			rotation.z = motionBlurPositions[i].rotz;
			alpha = 1.0-(float(i)/float(motionBlurPositions.size()));
			alpha *= 0.5;
			if (motionBlurTransition)
			{
				alpha *= motionBlurTransitionTimer;
			}
			renderCall();
		}		
		position = oldPos;
		alpha.x = oldAlpha;
		rotation.z = oldRotZ;

		renderCall();
	}
	else
		renderCall();
}

Vector RenderObject::getFollowCameraPosition()
{
	Vector pos = position;
	float f = followCamera;
	int fcl=0;
	
	if (layer != -1)
	{
		if (f == 0)	f = core->renderObjectLayers[layer].followCamera;
		fcl = core->renderObjectLayers[layer].followCameraLock;
	}


	if (f > 0 && f < 1)
	{
		switch (fcl)
		{
		case FCL_HORZ:
			pos.x = position.x - core->screenCenter.x;
			pos.x *= f;
			pos.x = core->screenCenter.x + pos.x;
		break;
		case FCL_VERT:
			pos.y = position.y - core->screenCenter.y;
			pos.y *= f;
			pos.y = core->screenCenter.y + pos.y;
		break;
		default:
			pos = position - core->screenCenter;
			pos *= f;
			pos = core->screenCenter + pos;
		break;
		}
	}
	return pos;
}

void RenderObject::renderCall()
{

	//RenderObjectLayer *rlayer = core->getRenderObjectLayer(getTopLayer());

	if (positionSnapTo)
		this->position = *positionSnapTo;

	position += offset;

#ifdef BBGE_BUILD_DIRECTX
	if (!RENDEROBJECT_FASTTRANSFORM)
		core->getD3DMatrixStack()->Push();
#endif

#ifdef BBGE_BUILD_OPENGL
	if (!RENDEROBJECT_FASTTRANSFORM)
		glPushMatrix();
	if (!RENDEROBJECT_SHAREATTRIBUTES)
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);
	}
#endif


	if (!RENDEROBJECT_FASTTRANSFORM)
	{
		if (layer != LR_NONE)
		{
			RenderObjectLayer *l = &core->renderObjectLayers[layer];
			if (l->followCamera != NO_FOLLOW_CAMERA)
			{
				followCamera = l->followCamera;
			}
		}
		if (followCamera!=0 && !parent)
		{
			if (followCamera == 1)
			{
#ifdef BBGE_BUILD_OPENGL
			 	glLoadIdentity();
				glScalef(core->globalResolutionScale.x, core->globalResolutionScale.y,0);
				glTranslatef(position.x, position.y, position.z);
				if (isfh())
				{
					glDisable(GL_CULL_FACE);
					glRotatef(180, 0, 1, 0);
				}

				if (core->mode == Core::MODE_3D)
				{
					glRotatef(rotation.x+rotationOffset.x, 1, 0, 0);
					glRotatef(rotation.y+rotationOffset.y, 0, 1, 0);
				}

				glRotatef(rotation.z+rotationOffset.z, 0, 0, 1);
#endif
#ifdef BBGE_BUILD_DIRECTX
				core->getD3DMatrixStack()->LoadIdentity();
				core->scaleMatrixStack(core->globalResolutionScale.x, core->globalResolutionScale.y,0);
				core->translateMatrixStack(position.x, position.y, 0);
				if (isfh())
				{
					//HACK: disable cull ->
					core->getD3DMatrixStack()->RotateAxisLocal(&D3DXVECTOR3(0, 1, 0), D3DXToRadian(180));
				}
				core->rotateMatrixStack(rotation.z + rotationOffset.z);
#endif
			}
			else
			{
				Vector pos = getFollowCameraPosition();

#ifdef BBGE_BUILD_OPENGL
				glTranslatef(pos.x, pos.y, pos.z);
				if (isfh())
				{
					glDisable(GL_CULL_FACE);
					glRotatef(180, 0, 1, 0);
				}
				if (core->mode == Core::MODE_3D)
				{
					glRotatef(rotation.x+rotationOffset.x, 1, 0, 0);
					glRotatef(rotation.y+rotationOffset.y, 0, 1, 0);
				}
				glRotatef(rotation.z+rotationOffset.z, 0, 0, 1);
#endif
#ifdef BBGE_BUILD_DIRECTX
				core->translateMatrixStack(pos.x, pos.y, 0);
				if (isfh())
				{
					//HACK: disable cull ->
					core->getD3DMatrixStack()->RotateAxisLocal(&D3DXVECTOR3(0, 1, 0), D3DXToRadian(180));
				}
				core->rotateMatrixStack(rotation.z + rotationOffset.z);
#endif
			}
		}
		else
		{

#ifdef BBGE_BUILD_OPENGL
			glTranslatef(position.x, position.y, position.z);//position.z);
#endif
#ifdef BBGE_BUILD_DIRECTX
			core->translateMatrixStack(position.x, position.y, 0);
#endif

#ifdef BBGE_BUILD_OPENGL
			if (RenderObject::renderPaths && position.path.getNumPathNodes() > 0)
			{
				glRotatef(0, 0, 1, -rotation.z);
				glLineWidth(4);
				glEnable(GL_BLEND);
				
				int i = 0;
				glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
				glBindTexture(GL_TEXTURE_2D, 0);

				glBegin(GL_LINES);
				for (i = 0; i < position.path.getNumPathNodes()-1; i++)
				{
					glVertex2f(position.path.getPathNode(i)->value.x-position.x, position.path.getPathNode(i)->value.y-position.y);
					glVertex2f(position.path.getPathNode(i+1)->value.x-position.x, position.path.getPathNode(i+1)->value.y-position.y);
				}
				glEnd();

				glPointSize(20);
				glBegin(GL_POINTS);
				glColor4f(0.5,0.5,1,1);
				for (i = 0; i < position.path.getNumPathNodes(); i++)
				{
					glVertex2f(position.path.getPathNode(i)->value.x-position.x, position.path.getPathNode(i)->value.y-position.y);
				}
				glEnd();
				//glPopMatrix();
			}
#endif
#ifdef BBGE_BUILD_OPENGL

			if (core->mode == Core::MODE_3D)
			{
				glRotatef(rotation.x+rotationOffset.x, 1, 0, 0);
				glRotatef(rotation.y+rotationOffset.y, 0, 1, 0);
			}

			glRotatef(rotation.z+rotationOffset.z, 0, 0, 1); 
			if (isfh())
			{
				glDisable(GL_CULL_FACE);
				glRotatef(180, 0, 1, 0);
			}
#endif
#ifdef BBGE_BUILD_DIRECTX
			//core->getD3DMatrixStack()->RotateAxisLocal(&D3DXVECTOR3(0, 0, 1), rotation.z+rotationOffset.z);
			core->rotateMatrixStack(rotation.z + rotationOffset.z);
			if (isfh())
			{
				//HACK: disable cull
				core->getD3DDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
				//core->getD3DMatrixStack()->Scale(-1, 1, 1);
				//core->applyMatrixStackToWorld();
				core->getD3DMatrixStack()->RotateAxisLocal(&D3DXVECTOR3(0, 1, 0), D3DXToRadian(180));
				//core->applyMatrixStackToWorld();
			}
#endif
		}
				
#ifdef BBGE_BUILD_OPENGL	
		glTranslatef(beforeScaleOffset.x, beforeScaleOffset.y, beforeScaleOffset.z);
		if (core->mode == Core::MODE_3D)
			glScalef(scale.x, scale.y, scale.z);
		else
			glScalef(scale.x, scale.y, 1);
		glTranslatef(internalOffset.x, internalOffset.y, internalOffset.z);
#endif
#ifdef BBGE_BUILD_DIRECTX
		core->translateMatrixStack(beforeScaleOffset.x, beforeScaleOffset.y, 0);
		core->scaleMatrixStack(scale.x, scale.y, 1);
		core->translateMatrixStack(internalOffset.x, internalOffset.y, 0);

		core->applyMatrixStackToWorld();
#endif


		//glDisable(GL_CULL_FACE);
		if (renderOrigin)
		{
#ifdef BBGE_BUILD_OPENGL
			  glBegin(GL_TRIANGLES);
				glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
				glVertex3f(0.0f, 5.0f, 0.0f);
				glVertex3f(50.0f, 0.0f, 0.0f);
				glVertex3f(0.0f, -5.0f, 0.0f);

				glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
				glVertex3f(0.0f, 0.0f, 5.0f);
				glVertex3f(0.0f, 50.0f, 0.0f);
				glVertex3f(0.0f, 0.0f, -5.0f);

				glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
				glVertex3f(5.0f, 0.0f, 0.0f);
				glVertex3f(0.0f, 0.0f, 50.0f);
				glVertex3f(-5.0f, 0.0f, 0.0f);
			glEnd();
#endif
		}
	}

	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		if (!(*i)->isDead() && (*i)->renderBeforeParent)
			(*i)->render();
	}


	if (useColor)
	{
#ifdef BBGE_BUILD_OPENGL
		if (rlayer && (rlayer->color.x != 1.0 || rlayer->color.y != 1.0 || rlayer->color.z != 1.0))
			glColor4f(color.x * rlayer->color.x, color.y * rlayer->color.y, color.z * rlayer->color.z, alpha.x*alphaMod);
		else
			glColor4f(color.x, color.y, color.z, alpha.x*alphaMod);
#elif defined(BBGE_BUILD_DIRECTX)
		core->setColor(color.x, color.y, color.z, alpha.x*alphaMod);
#endif
	}
	
	if (texture)
	{

#ifdef BBGE_BUILD_OPENGL
		if (texture->textures[0] != lastTextureApplied || repeatTexture != lastTextureRepeat)
		{
			texture->apply(repeatTexture);
			lastTextureRepeat = repeatTexture;
			lastTextureApplied = texture->textures[0];
		}
#endif
#ifdef BBGE_BUILD_DIRECTX
		texture->apply(repeatTexture);
#endif
	}
	else
	{
		if (lastTextureApplied != 0 || repeatTexture != lastTextureRepeat)
		{
#ifdef BBGE_BUILD_OPENGL
			glBindTexture(GL_TEXTURE_2D, 0);
#endif
#ifdef BBGE_BUILD_DIRECTX
			core->bindTexture(0, 0);
#endif
			lastTextureApplied = 0;
			lastTextureRepeat = repeatTexture;
		}
	}
	
	applyBlendType();


	bool doRender = true;
	int pass = renderPass;
	if (core->currentLayerPass != RENDER_ALL && renderPass != RENDER_ALL)
	{
		RenderObject *top = getTopParent();
		if (top)
		{
			if (top->overrideRenderPass != OVERRIDE_NONE)
				pass = top->overrideRenderPass;
		}

		doRender = (core->currentLayerPass == pass);
	}

	if (renderCollisionShape && !collisionRects.empty())
	{
#ifdef BBGE_BUILD_OPENGL
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);

		//glLoadIdentity();
		//core->setupRenderPositionAndScale();
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1.0f, 0.5f, 1.0f, 0.5f);
		glPointSize(5);

		for (int i = 0; i < collisionRects.size(); i++)
		{
			RectShape *r = &collisionRects[i];

			glBegin(GL_QUADS);
				glVertex3f(r->x1, r->y1, 0);
				glVertex3f(r->x1, r->y2, 0);
				glVertex3f(r->x2, r->y2, 0);
				glVertex3f(r->x2, r->y1, 0);
			glEnd();
			glBegin(GL_POINTS);
				glVertex3f(r->x1, r->y1, 0);
				glVertex3f(r->x1, r->y2, 0);
				glVertex3f(r->x2, r->y2, 0);
				glVertex3f(r->x2, r->y1, 0);
			glEnd();
		}

		glPopMatrix();
		glDisable(GL_BLEND);

		glPopAttrib();
#endif
	}

	if (renderCollisionShape && !collisionMask.empty())
	{
#ifdef BBGE_BUILD_OPENGL
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);
		
		/*
		glTranslatef(-offset.x, -offset.y,0);		
		glTranslatef(collidePosition.x, collidePosition.y,0);
		*/
		
		
		glLoadIdentity();
		core->setupRenderPositionAndScale();
		

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glColor4f(1,1,0,0.5);

		for (int i = 0; i < transformedCollisionMask.size(); i++)
			{
			Vector collide = this->transformedCollisionMask[i];
			//Vector collide = getWorldCollidePosition(collisionMask[i]);
			//Vector collide = collisionMask[i];
			/*
			if (isPieceFlippedHorizontal())
			{
				collide.x = -collide.x;
			}
			*/
			glTranslatef(collide.x, collide.y, 0);
			RenderObject *parent = this->getTopParent();
			if (parent)
				drawCircle(collideRadius*parent->scale.x, 45);
			glTranslatef(-collide.x, -collide.y, 0);
		}
		
		//glTranslatef(-collidePosition.x, -collidePosition.y,0);
		glDisable(GL_BLEND);
		glPopMatrix();
		glPopAttrib();

		//glTranslatef(offset.x, offset.y,0);
#endif
	}
	else if (renderCollisionShape && collideRadius > 0)
	{
		/*
		glPushMatrix();
		glBindTexture(GL_TEXTURE_2D, 0);
		//glScalef(-scale.x, -scale.y, 0);
		glTranslatef(-offset.x, -offset.y,0);
		glEnable(GL_BLEND);
		glTranslatef(collidePosition.x, collidePosition.y,0);
		//glEnable(GL_ALPHA_TEST);
		//glAlphaFunc(GL_GREATER, 0);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glColor4f(1,0,0,0.5);
		drawCircle(collideRadius, 8);
		glDisable(GL_BLEND);
		glTranslatef(offset.x, offset.y,0);
		glPopMatrix();
		*/
	}

	if (doRender)
		onRender();

		//collisionShape.render();
	if (!RENDEROBJECT_SHAREATTRIBUTES)
	{
		glPopAttrib();
	}

	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		if (!(*i)->isDead() && !(*i)->renderBeforeParent)
			(*i)->render();
	}


	if (!RENDEROBJECT_FASTTRANSFORM)
	{
#ifdef BBGE_BUILD_OPENGL
		glPopMatrix();
#endif
#ifdef BBGE_BUILD_DIRECTX
		core->getD3DMatrixStack()->Pop();
		core->applyMatrixStackToWorld();
#endif
	}


	position -= offset;

	if (integerizePositionForRender)
	{
		position = savePosition;
	}
}

void RenderObject::addDeathNotify(RenderObject *r)
{
	deathNotifications.remove(r);
	deathNotifications.push_back(r);
	r->addDeathNotifyInternal(this);
}

void RenderObject::addDeathNotifyInternal(RenderObject *r)
{
	deathNotifications.remove(r);
	deathNotifications.push_back(r);
}

void RenderObject::deathNotify(RenderObject *r)
{
	deathNotifications.remove(r);
}

Vector RenderObject::getCollisionMaskNormal(int index)
{
	Vector sum;
	int num=0;
	for (int i = 0; i < this->transformedCollisionMask.size(); i++)
	{
		if (i != index)
		{
			Vector diff = transformedCollisionMask[index] - transformedCollisionMask[i];
			if (diff.isLength2DIn(128))
			{
				sum += diff;
				num++;
			}
		}
	}
	if (!sum.isZero())
	{
		sum /= num;
		
		sum.normalize2D();

		/*
		std::ostringstream os;
		os << "found [" << num << "] circles, got normal [" << sum.x << ", " << sum.y << "]";
		debugLog(os.str());
		*/
	}

	return sum;
}

void RenderObject::lookAt(const Vector &pos, float t, float minAngle, float maxAngle, float offset)
{
	Vector myPos = this->getWorldPosition();
	float angle = 0;
	
	if (myPos.x == pos.x && myPos.y == pos.y)
	{
		return;
	}
	MathFunctions::calculateAngleBetweenVectorsInDegrees(myPos, pos, angle);	

	RenderObject *p = parent;
	while (p)
	{
		angle -= p->rotation.z;
		p = p->parent;
	}

	if (isPieceFlippedHorizontal())
	{
		angle = 180-angle;
		
		/*
		minAngle = -minAngle;
		maxAngle = -maxAngle;
		std::swap(minAngle, maxAngle);
		*/
		//std::swap(minAngle, maxAngle);
		/*
		minAngle = -(180+minAngle);
		maxAngle = -(180+maxAngle);
		*/
		/*
		if (minAngle > maxAngle)
			std::swap(minAngle, maxAngle);
		*/
		offset = -offset;
	}
	float a = angle;
	angle += offset;
	float offsetAngle = angle;
	if (angle < minAngle)
		angle = minAngle;
	if (angle > maxAngle)
		angle = maxAngle;

	int amt = 10;
	if (isPieceFlippedHorizontal())
	{
		if (pos.x < myPos.x-amt)
		{
			angle = 0;
		}
	}
	else
	{
		if (pos.x > myPos.x+amt)
		{
			angle = 0;
		}
	}

	rotation.interpolateTo(Vector(0,0,angle), t);
}

void RenderObject::removeAllChildren()
{	
	if (!children.empty())
	{
		removeChild(children.front());
		removeAllChildren();
	}
}

void RenderObject::recursivelyRemoveEveryChild()
{
	if (!children.empty())
	{
		RenderObject *child = (children.front());
		child->recursivelyRemoveEveryChild();
		removeChild(child);
		recursivelyRemoveEveryChild();
	}
}

void RenderObject::update(float dt)
{
	if (ignoreUpdate)
	{
		return;
	}
	if (useOldDT)
	{
		dt = core->get_old_dt();
	}
	if (!_dead)
	{
		dt *= updateMultiplier;
		onUpdate(dt);

		for (Children::iterator i = children.begin(); i != children.end(); i++)
		{
			if ((*i)->updateAfterParent && (((*i)->pm == PM_POINTER) || ((*i)->pm == PM_STATIC)))
			{
				(*i)->update(dt);
			}
		}
	}
}

void RenderObject::removeChild(RenderObject *r)
{
	children.remove(r);
	r->parent = 0;

	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->removeChild(r);
	}
}

void RenderObject::enqueueChildDeletion(RenderObject *r)
{
	if (r->parent == this)
	{
		childGarbage.push_back(r);
	}
}

void RenderObject::safeKill()
{
	alpha = 0;
	life = 0;
	onEndOfLife();
	deathEvent.call();
	for (RenderObjectList::iterator i = deathNotifications.begin(); i != deathNotifications.end(); i++)
	{
		(*i)->deathNotify(this);
	}
	//dead = true;
	if (this->parent)
	{
		parent->enqueueChildDeletion(this);
		/*
		parent->removeChild(this);
		core->enqueueRenderObjectDeletion(this);
		*/
	}
	else
	{
		if (stateData)
			stateData->removeRenderObject(this);
		else
			core->enqueueRenderObjectDeletion(this);
	}
}

void RenderObject::updateLife(float dt)
{
	if (decayRate > 0)
	{
		life -= decayRate*dt;
		if (life<=0)
		{
			safeKill();
		}
	}
	if (fadeAlphaWithLife && !alpha.interpolating)
	{
		alpha = ((life*lifeAlphaFadeMultiplier)/maxLife);
	}
}

Vector RenderObject::getNormal()
{
	float a = MathFunctions::toRadians(getAbsoluteRotation().z);
	return Vector(sinf(a),cosf(a));
}

// HACK: this is probably a slow implementation
Vector RenderObject::getForward()
{
	Vector v = getWorldCollidePosition(Vector(0,-1, 0));
	Vector r = v - getWorldCollidePosition();
	r.normalize2D();

	/*
	std::ostringstream os;
	os << "forward v(" << v.x << ", " << v.y << ") ";
	os << "r(" << r.x << ", " << r.y << ") ";
	debugLog(os.str());
	*/
	return r;
}

Vector RenderObject::getAbsoluteRotation()
{
	Vector r = rotation;
	if (parent)
	{
		return parent->getAbsoluteRotation() + r;
	}
	return r;
}

void RenderObject::onUpdate(float dt)
{
	if (isDead()) return;
	//collisionShape.updatePosition(position);
	updateLife(dt);

	/*
	width.update(dt);
	height.update(dt);
	*/


	/*
	if (!parent && !children.empty() && shareAlphaWithChildren)
	{
		propogateAlpha();
	}
	*/


	/*
	if (flipTimer.updateCheck(dt))
	{
		if (flipState == 0)
		{
			_fh = !_fh;
			flipState = 1;
		}
	}
	*/

	if (shareAlphaWithChildren && !children.empty())
	{
		for (Children::iterator i = children.begin(); i != children.end(); i++)
		{
			(*i)->alpha = this->alpha;
		}
	}

	if (shareColorWithChildren && !children.empty())
	{
		for (Children::iterator i = children.begin(); i != children.end(); i++)
		{
			(*i)->color = this->color;
		}
	}
	position += velocity * dt;
	velocity += gravity * dt;
	position.update(dt);
	velocity.update(dt);
	scale.update(dt);
	rotation.update(dt);
	color.update(dt);
	alpha.update(dt);
	offset.update(dt);
	internalOffset.update(dt);
	beforeScaleOffset.update(dt);
	rotationOffset.update(dt);

	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		if (!(*i)->updateAfterParent && (((*i)->pm == PM_POINTER) || ((*i)->pm == PM_STATIC)))
		{
			(*i)->update(dt);
		}
	}
	
	if (!childGarbage.empty())
	{
		for (Children::iterator i = childGarbage.begin(); i != childGarbage.end(); i++)
		{
			removeChild(*i);
			(*i)->destroy();
			delete (*i);
		}
		childGarbage.clear();
	}

	if (motionBlur)
	{
		if (motionBlurFrameOffsetCounter >= motionBlurFrameOffset)
		{
			motionBlurFrameOffsetCounter = 0;
			motionBlurPositions[0].position = position;
			motionBlurPositions[0].rotz = rotation.z;
			for (int i = motionBlurPositions.size()-1; i > 0; i--)
			{
				motionBlurPositions[i] = motionBlurPositions[i-1];
			}
		}
		else
			motionBlurFrameOffsetCounter ++;
	}
	if (motionBlurTransition)
	{
		motionBlurTransitionTimer -= dt*2;
		if (motionBlurTransitionTimer <= 0)
		{
			motionBlur = motionBlurTransition = false;
			motionBlurTransitionTimer = 0;
		}
	}

//	updateCullVariables();
}

void RenderObject::propogateAlpha()
{
	/*
	if (!shareAlphaWithChildren) return;
	for (int i = 0; i < children.size(); i++)
	{
		children[i]->alpha = this->alpha * children[i]->parentAlphaModifier.getValue();
		children[i]->propogateAlpha();
	}

	*/
	/*
	if (shareAlphaWithChildren && !children.empty())
	{
		for (int i = 0; i < children.size(); i++)
		{

			//children[i]->alpha = this->alpha * children[i]->parentAlphaModifier.getValue();
		}
	}
	*/
}

void RenderObject::unloadDevice()
{
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->unloadDevice();
	}
}

void RenderObject::reloadDevice()
{
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->reloadDevice();
	}
}

void RenderObject::setTexture(const std::string &n)
{
	std::string name = n;
	stringToLowerUserData(name);

	if (name.empty()) return;
	if (texture)
	{
		if (texture->name != core->getInternalTextureName(name))
		{
			texture->removeRef();
			texture = 0;
		}
		else
		{
			return;
		}
	}
	Texture *t = core->addTexture(name);
	setTexturePointer(t, NO_ADD_REF);
}

double RenderObject::getSortDepth()
{
	return double(position.y);
}

void RenderObject::addChild(RenderObject *r, ParentManaged pm, RenderBeforeParent rbp, ChildOrder order)
{
	if (r->parent)
	{
		errorLog("Engine does not support multiple parents");
		exit(0);
	}

	if (order == CHILD_BACK)
		children.push_back(r);
	else
		children.push_front(r);

	r->pm = pm;

	if (rbp == RBP_OFF)
		r->renderBeforeParent = 0;
	else if (rbp == RBP_ON)
		r->renderBeforeParent = 1;

	r->parent = this;
}

StateData *RenderObject::getStateData()
{
	if (parent)
	{
		return parent->getStateData();
	}
	else
		return stateData;
}

void RenderObject::setPositionSnapTo(InterpolatedVector *positionSnapTo)
{
	this->positionSnapTo = positionSnapTo;
}

void RenderObject::setOverrideCullRadius(int ovr)
{
	overrideCullRadius = ovr;
}

int RenderObject::getCullRadius()
{
	// THIS WILL HARDLY EVER GET CALLED
	// Quad::getCullRadius is what runs things majorly
	if (overrideCullRadius)
		return overrideCullRadius;

	return 0;
}

#ifdef BBGE_BUILD_WINDOWS
	inline bool RenderObject::isOnScreen()
#else
	bool RenderObject::isOnScreen()
#endif
{
	if (alpha.x == 0) return false;
	
	// HACK: assume all children are visible
	if (parent || !cull) return true;

	if (followCamera == 1) return true;
	//if (followCamera != 0) return true;

	// note: radii are sqr-ed for speed
	int checkRadius = getCullRadius();

	if ((this->getFollowCameraPosition() - core->cullCenter).getSquaredLength2D() > ((checkRadius*checkRadius)*core->invGlobalScale + (core->cullRadius*core->cullRadius)))
		return false;
	return true;	
}

void RenderObject::propogateParentManagedStatic()
{
	for (Children::iterator i = children.begin(); i != children.end(); i++)
	{
		(*i)->pm = PM_STATIC;
		(*i)->propogateParentManagedStatic();
	}
}

bool RenderObject::isCoordinateInRadius(const Vector &pos, float r)
{
	Vector d = pos-getRealPosition();
	
	return (d.getSquaredLength2D() < r*r);
}
