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
#include "GridRender.h"
#include "Game.h"
#include "Avatar.h"

namespace MiniMapRenderSpace
{
	typedef std::vector<Quad*> Buttons;
	Buttons buttons;

	const int BUTTON_RADIUS = 15;

	Texture *texCook=0;
	Texture *texWaterBit=0;
	Texture *texMinimapBtm = 0;
	Texture *texMinimapTop = 0;
	Texture *texRipple=0;
	Texture *texNaija=0;
	Texture *texHealthBar=0;
	Texture *texMarker=0;

	float waterSin = 0;

	int jumpOff=0;
	float jumpTimer = 0.5;
	float jumpTime = 1.5;
	float incr=0;
}

using namespace MiniMapRenderSpace;

MiniMapRender::MiniMapRender() : RenderObject()
{
	toggleOn = 1;

	radarHide = false;

	doubleClickDelay = 0;
	mb = false;
	_isCursorIn = false;
	lastCursorIn = false;
	followCamera = 1;
	doRender = true;
	float shade = 0.75;
	color = Vector(shade, shade, shade);
	cull = false;
	a = 1.0;

	texCook				= core->addTexture("GUI/ICON-FOOD");
	texWaterBit			= core->addTexture("GUI/MINIMAP/WATERBIT");
	texMinimapBtm		= core->addTexture("GUI/MINIMAP/BTM");
	texMinimapTop		= core->addTexture("GUI/MINIMAP/TOP");
	texRipple			= core->addTexture("GUI/MINIMAP/RIPPLE");
	texNaija			= core->addTexture("GEMS/NAIJA-TOKEN");
	texHealthBar		= core->addTexture("PARTICLES/glow-masked"); 
	texMarker			= core->addTexture("gui/minimap/marker");

	buttons.clear();

	Quad *q = 0;

	q = new Quad();
	q->setTexture("gui/open-menu");
	q->scale = Vector(1.5, 1.5);
	buttons.push_back(q);

	q->position = Vector(80, 80);

	addChild(q, PM_POINTER, RBP_OFF);
}

void MiniMapRender::destroy()
{
	RenderObject::destroy();

	UNREFTEX(texCook);
	UNREFTEX(texWaterBit);
	UNREFTEX(texMinimapBtm);
	UNREFTEX(texMinimapTop);
	UNREFTEX(texRipple);
	UNREFTEX(texNaija);
	UNREFTEX(texHealthBar);
	UNREFTEX(texMarker);
}

bool MiniMapRender::isCursorIn()
{
	return _isCursorIn || lastCursorIn;
}

void MiniMapRender::slide(int slide)
{
	switch(slide)
	{
	case 0:
		dsq->game->miniMapRender->offset.interpolateTo(Vector(0, 0), 0.28, 0, 0, 1);
	break;
	case 1:
		dsq->game->miniMapRender->offset.interpolateTo(Vector(0, -470), 0.28, 0, 0, 1);
	break;
	}
}



bool MiniMapRender::isCursorInButtons()
{
	for (Buttons::iterator i = buttons.begin(); i != buttons.end(); i++)
	{
		if ((core->mouse.position - (*i)->getWorldPosition()).isLength2DIn(BUTTON_RADIUS))
		{
			return true;
		}
	}

	return ((core->mouse.position - position).isLength2DIn(50));
}

void MiniMapRender::clickEffect(int type)
{
	dsq->clickRingEffect(getWorldPosition(), type);
}

void MiniMapRender::toggle(int t)
{
	for (int i = 0; i < buttons.size(); i++)
	{
		buttons[i]->renderQuad = (bool)t;
	}
	toggleOn = t;
}

void MiniMapRender::onUpdate(float dt)
{
	RenderObject::onUpdate(dt);	
	position.z = 2.9;

	waterSin += dt;

	if (doubleClickDelay > 0)
	{
		doubleClickDelay -= dt;
	}

	position.x = core->getVirtualWidth() - 55 - core->getVirtualOffX();

	radarHide = false;

	if (dsq->darkLayer.isUsed() && dsq->game->avatar)
	{
		Path *p = dsq->game->getNearestPath(dsq->game->avatar->position, PATH_RADARHIDE);

		float t=dt*2;
		if ((dsq->continuity.form != FORM_SUN && dsq->game->avatar->isInDarkness())
			|| (p && p->isCoordinateInside(dsq->game->avatar->position)))
		{
			radarHide = true;
			a -= t;
			if (a < 0)
				a = 0;
		}
		else
		{
			a += t;
			if (a > 1)
				a = 1;
		}

	}
	else
	{
		a = 1;
	}

	if (dsq->game->avatar && dsq->game->avatar->isInputEnabled())
	{
		float v = dsq->game->avatar->health/5.0;
		if (v < 0)
			v = 0;
		if (!lerp.isInterpolating())
			lerp.interpolateTo(v, 0.1);
		lerp.update(dt);


		jumpTimer += dt*0.5;
		if (jumpTimer > jumpTime)
		{
			jumpTimer = 0.5;
		}
		incr += dt*2;
		if (incr > 3.14)
			incr -= 3.14;
	}

	_isCursorIn = false;
	if (alpha.x == 1)
	{		
		if (!dsq->game->isInGameMenu() && (!dsq->game->isPaused() || (dsq->game->isPaused() && dsq->game->worldMapRender->isOn())))
		{
			if (isCursorInButtons())
			{
				if (!core->mouse.buttons.left || mb)
					_isCursorIn = true;
			}

			if (_isCursorIn || lastCursorIn)
			{

				if (core->mouse.buttons.left && !mb)
				{
					mb = true;
				}
				else if (!core->mouse.buttons.left && mb)
				{
					mb = false;

					bool btn=false;

					if (!dsq->game->worldMapRender->isOn())
					{
						for (int i = 0; i < buttons.size(); i++)
						{
							if ((buttons[i]->getWorldPosition() - core->mouse.position).isLength2DIn(BUTTON_RADIUS))
							{
								switch(i)
								{
								case 0:
								{
									doubleClickDelay = 0;
									dsq->game->showInGameMenu();
									btn = true;
								}
								break;
								}
							}
							if (btn) break;
						}
					}

					if (!btn && !dsq->mod.isActive() && !radarHide)
					{
						if (dsq->game->worldMapRender->isOn())
						{
							dsq->game->worldMapRender->toggle(false);
							clickEffect(1);
						}
						else
						{
							if (doubleClickDelay > 0)
							{
								
								if (dsq->continuity.gems.empty())
									dsq->continuity.pickupGem("Naija-Token");

								dsq->game->worldMapRender->toggle(true);

								clickEffect(0);

								doubleClickDelay = 0;
							}
							else
							{
								doubleClickDelay = DOUBLE_CLICK_DELAY;

								clickEffect(0);
							}
						}
					}
				}

				if (isCursorInButtons())
				{
					if (mb)
					{
						_isCursorIn = true;
					}
				}
			}
			else
			{
				mb = false;
			}
			lastCursorIn = _isCursorIn;
		}
	}
}

void MiniMapRender::onRender()
{
	if (!toggleOn)
		return;

	if (!dsq->game->avatar || dsq->game->avatar->getState() == Entity::STATE_TITLE || (dsq->disableMiniMapOnNoInput && !dsq->game->avatar->isInputEnabled()))
	{
		for (int i = 0; i < buttons.size(); i++)
			buttons[i]->renderQuad = false;
		return;
	}
	else
	{
		for (int i = 0; i < buttons.size(); i++)
			buttons[i]->renderQuad = true;
	}


#ifdef BBGE_BUILD_OPENGL

	glBindTexture(GL_TEXTURE_2D, 0);
	RenderObject::lastTextureApplied = 0;
	float alphaValue = alpha.x;
	

	const int sz2 = 80;//80;
	const int bsz2 = sz2*1.5;

	TileVector t(dsq->game->avatar->position);

	glLineWidth(1);
	
	if (alphaValue > 0)
	{
		int skip = 12;
		int useTile = TILE_SIZE*skip;
		Vector t2;
		t2.x = int(dsq->game->avatar->position.x/useTile);
		t2.y = int(dsq->game->avatar->position.y/useTile);
		Vector t2wp = (t2 * useTile) + useTile*0.5;

		texMinimapBtm->apply();

		glBegin(GL_QUADS);
			glColor4f(a, a, a, 1);
			glTexCoord2f(0, 1);
			glVertex2f(-bsz2, bsz2);
			glTexCoord2f(1, 1);
			glVertex2f(bsz2, bsz2);
			glTexCoord2f(1, 0);
			glVertex2f(bsz2, -bsz2);
			glTexCoord2f(0, 0);
			glVertex2f(-bsz2, -bsz2);
		glEnd();

		texMinimapBtm->unbind();


		float realSz2 = sz2*scale.x;
		float factor = float(core->getWindowWidth()) / float(core->getVirtualWidth());
		
		if (a > 0)
		{
			texWaterBit->apply();
			Vector off;
		
			off = t2wp - dsq->game->avatar->position;
			off *= sz2/800.0f;
			off *= 0.5;

			glScalef(0.5, 0.5,0);

			glBlendFunc(GL_SRC_ALPHA,GL_ONE);

			Vector rp;
			for (int y = t.y-sz2*2; y < t.y + sz2*2; y+=skip)
			{
				int rowStart = -1;
				float out = sin((float(y-(t.y-sz2*2))/float(sz2*4)) * 3.14f);
				int x1= t.x-int(sz2*2*out) - skip, x2 = t.x+int(sz2*2*out) + skip;

				for (int x = x1; x < x2; x+=skip)
				{	
					int ttx = (int(x/skip))*skip, tty = (int(y/skip))*skip;
					TileVector ttt(ttx, tty);
					if (!dsq->game->getGrid(ttt))
					{
						int bright = 0;

						if (ttt.worldVector().y < dsq->game->waterLevel.x)
						{
							glColor4f(0.1, 0.2, 0.5, 0.2*a);
						}
						else
						{
							glColor4f(0.1, 0.2, 0.9, 0.4*a);
						}

						Vector tt(int(((x*TILE_SIZE)+TILE_SIZE*0.5)/(skip*TILE_SIZE)), int(((y*TILE_SIZE)+TILE_SIZE*0.5)/(skip*TILE_SIZE)));
						tt *= TILE_SIZE*skip;
						tt.x += TILE_SIZE*skip*0.5;
						tt.y += TILE_SIZE*skip*0.5;

						if (tt.x < dsq->game->cameraMin.x)	continue;
						if (tt.x > dsq->game->cameraMax.x)	continue;
						if (tt.y < dsq->game->cameraMin.y)	continue;
						if (tt.y > dsq->game->cameraMax.y)	continue;
					
						rp = Vector(tt-dsq->game->avatar->position)*Vector(1.0/1600.0, 1.0/1600.0)*sz2;

						glTranslatef(rp.x, rp.y, 0);

						float v = sin(waterSin +  (tt.x + tt.y*sz2*2)*0.001 + sqr(tt.x+tt.y)*0.00001);
						
						int sz = 20 + fabs(v)*20;

						if (bright)
							sz = 10;			

						glBegin(GL_QUADS);
							glTexCoord2f(0, 1);
							glVertex2f(-sz, sz);
							glTexCoord2f(1, 1);
							glVertex2f(sz, sz);
							glTexCoord2f(1, 0);
							glVertex2f(sz, -sz);
							glTexCoord2f(0, 0);
							glVertex2f(-sz, -sz);
						glEnd();

						glTranslatef(-rp.x, -rp.y, 0);
					}
					
				}
			}
			texWaterBit->unbind();
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glBindTexture(GL_TEXTURE_2D, 0);
			glScalef(2, 2, 0);

		}
	}

	if (!radarHide)
	{
		for (int i = 0; i < dsq->game->paths.size(); i++)
		{
			int extraSize;
			extraSize = 0;
			Path *p = dsq->game->paths[i];
			if (!p->nodes.empty())
			{
				Vector pt(p->nodes[0].position);
				Vector d = pt - dsq->game->avatar->position;
				d.capLength2D(2800);
				{
					bool render = true;
					
					Vector rp = Vector(d)*Vector(1.0/1600.0, 1.0/1600.0)*sz2*0.5;

					extraSize = sin(game->getTimer()*3.14)*6 + 14;

					switch(p->pathType)
					{
					case PATH_COOK:
					{
						Path *p2 = dsq->game->getNearestPath(p->nodes[0].position, PATH_RADARHIDE);
						if (p2 && p2->isCoordinateInside(p->nodes[0].position))
						{
							if (!p2->isCoordinateInside(dsq->game->avatar->position))
							{
								render = false;
							}
						}
						if (render)
						{	
							glColor4f(1, 1, 1, 1);
							

							glTranslatef(rp.x, rp.y, 0);
							int sz = 16;

							texCook->apply();

							glBegin(GL_QUADS);
								glTexCoord2f(0, 1);
								glVertex2f(-sz, sz);
								glTexCoord2f(1, 1);
								glVertex2f(sz, sz);
								glTexCoord2f(1, 0);
								glVertex2f(sz, -sz);
								glTexCoord2f(0, 0);
								glVertex2f(-sz, -sz);
							glEnd();

							glTranslatef(-rp.x, -rp.y, 0);
							render = false;
							texCook->unbind();
							glBindTexture(GL_TEXTURE_2D, 0);
						}
					}
					break;
					case PATH_SAVEPOINT:
					{
						Path *p2 = dsq->game->getNearestPath(p->nodes[0].position, PATH_RADARHIDE);
						if (p2 && p2->isCoordinateInside(p->nodes[0].position))
						{
							if (!p2->isCoordinateInside(dsq->game->avatar->position))
							{
								render = false;
							}
						}
						if (render)
							glColor4f(1.0, 0, 0, alphaValue*0.75);
					}
					break;
					case PATH_WARP:
					{
						Path *p2 = dsq->game->getNearestPath(p->nodes[0].position, PATH_RADARHIDE);
						if (p2 && p2->isCoordinateInside(p->nodes[0].position))
						{
							if (!p2->isCoordinateInside(dsq->game->avatar->position))
							{
								render = false;
							}
						}
						
						if (render)
						{
							if (p->naijaHome)
							{
								glColor4f(1.0, 0.9, 0.2, alphaValue*0.75);	
							}
							else
							{
								glColor4f(1.0, 1.0, 1.0, alphaValue*0.75);
							}
						}
					}
					break;
					default:
					{
						render = false;
					}
					break;
					}
					
					if (render)
					{
						glTranslatef(rp.x, rp.y, 0);
						int sz = extraSize;

						texRipple->apply();

						glBegin(GL_QUADS);
							glTexCoord2f(0, 1);
							glVertex2f(-sz, sz);
							glTexCoord2f(1, 1);
							glVertex2f(sz, sz);
							glTexCoord2f(1, 0);
							glVertex2f(sz, -sz);
							glTexCoord2f(0, 0);
							glVertex2f(-sz, -sz);
						glEnd();

						glTranslatef(-rp.x, -rp.y, 0);
						render = false;
						texRipple->unbind();
						glBindTexture(GL_TEXTURE_2D, 0);
					}
				}
			}
		}
	}

	glColor4f(1,1,1, alphaValue);

	const int hsz = 20;
	texNaija->apply();

	glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(-hsz, hsz);
		glTexCoord2f(1, 1);
		glVertex2f(hsz, hsz);
		glTexCoord2f(1, 0);
		glVertex2f(hsz, -hsz);
		glTexCoord2f(0, 0);
		glVertex2f(-hsz, -hsz);
	glEnd();

	texNaija->unbind();
	glBindTexture(GL_TEXTURE_2D, 0);

	glColor4f(1,1,1,1);

	texMinimapTop->apply();
	glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(-bsz2, bsz2);
		glTexCoord2f(1, 1);
		glVertex2f(bsz2, bsz2);
		glTexCoord2f(1, 0);
		glVertex2f(bsz2, -bsz2);
		glTexCoord2f(0, 0);
		glVertex2f(-bsz2, -bsz2);
	glEnd();
	texMinimapTop->unbind();

	glBindTexture(GL_TEXTURE_2D, 0);


	float angle = 0;
	float stepSize = 6.28/128.0;

	glLineWidth(10 * (core->width / 1024.0));
	
	stepSize = 6.28 / 64;

	float oangle = -3.14*0.5;
	angle = oangle;
	float eangle = oangle + 3.14*lerp.x;
	float eangle2 = oangle + 3.14*(dsq->game->avatar->maxHealth/5.0);
	float steps = (eangle - oangle)/stepSize;
	float bit = (3.14/5.0);
	int step = 0;

	Vector gc;

	if (lerp.x >= 1)
	{
		gc = Vector(0,1,0.5);
	}
	else
	{
		gc = Vector(1-lerp.x, lerp.x*1, lerp.x*0.5);
		gc.normalize2D();
	}

	float rad = sz2 + 4;

	texHealthBar->apply();

	Vector c;
	float x,y;

	const int msz = 20;
	const int qsz = 32;
	const int qsz1 = 10;

	int jump = 0;

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	while (lerp.x != 0 && angle <= eangle)
	{
		c = gc;

		x = sin(angle)*rad+2;
		y = cos(angle)*rad;

		// !!! FIXME: loop invariant.
		glColor4f(c.x, c.y, c.z, 0.6);

		glBegin(GL_QUADS);
			glTexCoord2f(0, 1);
			glVertex2f(x-qsz1, y+qsz1);
			glTexCoord2f(1, 1);
			glVertex2f(x+qsz1, y+qsz1);
			glTexCoord2f(1, 0);
			glVertex2f(x+qsz1, y-qsz1);
			glTexCoord2f(0, 0);
			glVertex2f(x-qsz1, y-qsz1);
		glEnd();

		step++;

		angle += stepSize;
	}

	angle = oangle;

	float pa = jumpTimer;
	if (pa > 1)
		pa = (1.5 - pa) + 0.5;


	glBlendFunc(GL_SRC_ALPHA,GL_ONE);

	while (lerp.x != 0 && angle <= eangle)
	{
		c = gc;


		x = sin(angle)*rad+2;
		y = cos(angle)*rad;

		if (jump == 0)
		{
			// !!! FIXME: loop invariant.
			glColor4f(c.x, c.y, c.z, fabs(cos(angle-incr))*0.3 + 0.2);

			glBegin(GL_QUADS);
				glTexCoord2f(0, 1);
				glVertex2f(x-qsz, y+qsz);
				glTexCoord2f(1, 1);
				glVertex2f(x+qsz, y+qsz);
				glTexCoord2f(1, 0);
				glVertex2f(x+qsz, y-qsz);
				glTexCoord2f(0, 0);
				glVertex2f(x-qsz, y-qsz);
			glEnd();
		}

		step++;

		jump++;
		if (jump > 3)
			jump = 0;

		angle += stepSize;
	}
	texHealthBar->unbind();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glColor4f(1,1,1,1);

	texMarker->apply();

	x = sin(eangle2)*rad+2;
	y = cos(eangle2)*rad;

	glBegin(GL_QUADS);
		glTexCoord2f(0, 1);
		glVertex2f(x-msz, y+msz);
		glTexCoord2f(1, 1);
		glVertex2f(x+msz, y+msz);
		glTexCoord2f(1, 0);
		glVertex2f(x+msz, y-msz);
		glTexCoord2f(0, 0);
		glVertex2f(x-msz, y-msz);
	glEnd();

	texMarker->unbind();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glColor4f(1,1,1,1);

	glBindTexture(GL_TEXTURE_2D, 0);

#endif
}

