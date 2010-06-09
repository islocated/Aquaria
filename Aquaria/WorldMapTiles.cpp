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
#include "DSQ.h"

WorldMapTile::WorldMapTile()
{
	revealed = false;
	prerevealed = false;
	scale = scale2 = 1;
	layer = 0;
	index = -1;
	vis = 0;
	visSize = 0;
	q = 0;
	stringIndex = 0;
}

void WorldMapTile::visToList()
{
	if (vis)
	{
		for (int x = 0; x < visSize; x++)
		{
			for (int y = 0; y < visSize; y++)
			{
				if (vis[x][y].z > 0.5)
					list.push_back(IntPair(x, y));
			}
		}
	}
}

void WorldMapTile::listToVis(float ab, float av)
{
	for (int x = 0; x < visSize; x++)
	{
		for (int y = 0; y < visSize; y++)
		{
			vis[x][y].z = ab;
		}
	}

	for (int i = 0; i < list.size(); i++)
	{
		int x,y;
		x = int(list[i].x);
		y = int(list[i].y);
		vis[x][y].z = av;
	}
}

void WorldMapTile::clearList()
{
	list.clear();
}

WorldMap::WorldMap()
{
	gw=gh=0;
}

void WorldMap::load(const std::string &file)
{
	worldMapTiles.clear();

	std::string line;

	std::ifstream in(file.c_str());
	
	while (std::getline(in, line))
	{
		WorldMapTile t;
		std::istringstream is(line);
		is >> t.index >> t.stringIndex >> t.name >> t.layer >> t.scale >> t.gridPos.x >> t.gridPos.y >> t.prerevealed >> t.scale2;
		t.revealed = t.prerevealed;
		stringToUpper(t.name);
		worldMapTiles.push_back(t);
	}
}

void WorldMap::save(const std::string &file)
{
	std::ofstream out(file.c_str());
	
	for (int i = 0; i < worldMapTiles.size(); i++)
	{
		WorldMapTile *t = &worldMapTiles[i];
		out << t->index << " " << t->name << " " << t->layer << " " << t->scale << " " << t->gridPos.x << " " << t->gridPos.y << " " << t->prerevealed << " " << t->scale2 << std::endl;
	}
}

void WorldMap::revealMap(const std::string &name)
{
	WorldMapTile *t = getWorldMapTile(name);
	if (t)
	{
		t->revealed = true;
	}
}

void WorldMap::revealMapIndex(int index)
{
	WorldMapTile *t = getWorldMapTileByIndex(index);
	if (t)
	{
		t->revealed = true;
	}
}

WorldMapTile *WorldMap::getWorldMapTile(const std::string &name)
{
	std::string n = name;
	stringToUpper(n);
	for (int i = 0; i < worldMapTiles.size(); i++)
	{
		if (worldMapTiles[i].name == n)
		{
			return &worldMapTiles[i];
		}
	}
	return 0;
}

WorldMapTile *WorldMap::getWorldMapTileByIndex(int index)
{
	for (int i = 0; i < worldMapTiles.size(); i++)
	{
		if (worldMapTiles[i].index == index)
		{
			return &worldMapTiles[i];
		}
	}
	return 0;
}

/*



void WorldMap::revealMapIndex(int index)
{
	if (index < 0 || index >= worldMapTiles.size()) return;

	worldMapTiles[index].revealed = true;
}
*/

void WorldMap::hideMap()
{
	for (int i = 0; i < worldMapTiles.size(); i++)
	{
		worldMapTiles[i].revealed = false;
	}
}

int WorldMap::getNumWorldMapTiles()
{
	return worldMapTiles.size();
}

WorldMapTile *WorldMap::getWorldMapTile(int index)
{
	if (index < 0 || index >= worldMapTiles.size()) return 0;

	return &worldMapTiles[index];
}

