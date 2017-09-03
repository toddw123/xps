/* 
 * XenServer, an online game server
 * Copyright (C) 2008 XenServer team
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MAPMGR_H
#define __MAPMGR_H


#include "MapCell.h"

#define DT_MAX_CROWD_AGENTS 20000

#define RECAST_TILE_SIZE 512

#define SIGHT_RANGE 175
#define SIGHT_RANGE_X 240
#define SIGHT_RANGE_Y 140

#define CELL_X_MIN 0
#define CELL_Y_MIN 0
#define CELL_X_MAX 65535
#define CELL_Y_MAX 65535

#define CELL_HEIGHT 512
#define CELL_WIDTH 512

#define CELL_X_COUNT ((CELL_X_MAX / CELL_WIDTH) + 1)
#define CELL_Y_COUNT ((CELL_Y_MAX / CELL_HEIGHT) + 1)

class PathCellData;
class PathTileData;
class TimedObjectData;
void OnIntersectPathCellData(const G3D::Ray & r, const PathCellData* b, float & dist);
void OnIntersectPathTileData(const G3D::Ray & r, const PathTileData* b, float & dist);
void OnIntersectTimedObjectData(const G3D::Ray & r, const TimedObjectData* b, float & dist);

struct NavMeshSetHeader
{
	int magic;
	int version;
	int numTiles;
	dtNavMeshParams params;
};

struct NavMeshTileHeader
{
	dtTileRef tileRef;
	int dataSize;
};

static const int NAVMESHSET_MAGIC = 'M'<<24 | 'S'<<16 | 'E'<<8 | 'T'; //'MSET';
static const int NAVMESHSET_VERSION = 1;

#define CALL_MAPSCRIPT_FUNC(func, ...) \
		{ \
			for (unordered_map<uint32, MapScript*>::iterator mapitrtmp = m_mapscripts.begin(); mapitrtmp != m_mapscripts.end(); ++mapitrtmp) \
				mapitrtmp->second->func(__VA_ARGS__); \
		}

enum PassableFlags
{
	PASS_FLAG_WALK				= 0x01,
	PASS_FLAG_SWIM				= 0x02,
	PASS_FLAG_GAS				= 0x04,
	PASS_FLAG_TELEPORT			= 0x08,
};

struct Point
{
	int32 x;
	int32 y;
	int32 mapid;
};

struct PathPoint
{
	int32 x;
	int32 y;
	uint32 prevarrivetime; //basically when we start moving there
	uint32 arrivetime; //ms
};

struct Vec3
{
	float x;
	float y;
	float z;
};

struct Triangle
{
	Vec3 points[3];

	Triangle(float x, float y, float z, float x2, float y2, float z2, float x3, float y3, float z3)
	{
		points[0].x = x;
		points[0].y = y;
		points[0].z = z;

		points[1].x = x2;
		points[1].y = y2;
		points[1].z = z2;

		points[2].x = x3;
		points[2].y = y3;
		points[2].z = z3;
	}
};

class PathPlane;
struct PathPlanePoint
{
public:
	uint8 blocked;
	uint32 passflags;
	PathPlane* plane;
};

class MapMgr;
class PathTileData
{
public:
	MapMgr* m_mapmgr;
	G3D::AABox m_box;
	uint32 passflags;
	uint8 maplayer1;
	uint8 maplayer2;
	uint8 maplayer3;
	
	uint32 GetTileX()
	{
		return m_box.low().x / 20;
	}
	uint32 GetTileY()
	{
		return m_box.low().y / 20;
	}
};

class PathCellData
{
public:
	bool loaded;
	PathCellData() { loaded = false; }
	G3D::AABox m_bounds;
	G3D::KDTree<PathTileData*> m_tree;
	size_t hashCode() const { return m_bounds.hashCode(); }
	int operator == (const PathCellData & rhs) const { return m_bounds == rhs.m_bounds; }
};

class TimedObjectData
{
public:
	G3D::AABox m_bounds;
	bool passable;
	MapMgr* m_mapmgr;
};

template <> struct HashTrait<TimedObjectData*>
{
	static size_t hashCode(const TimedObjectData* p) { return p->m_bounds.hashCode(); }
};

template<> struct BoundsTrait<TimedObjectData*>
{
	static void getBounds(const TimedObjectData* v, G3D::AABox& out) { v->m_bounds.getBounds(out); }
};

template <> struct HashTrait<PathCellData*>
{
	static size_t hashCode(const PathCellData* p) { return p->hashCode(); }
};

template<> struct BoundsTrait<PathCellData*>
{
	static void getBounds(const PathCellData* v, G3D::AABox& out) { v->m_bounds.getBounds(out); }
};

template <> struct HashTrait<PathTileData*>
{
	static size_t hashCode(const PathTileData* p) { return p->m_box.hashCode(); }
};

template<> struct BoundsTrait<PathTileData*>
{
	static void getBounds(const PathTileData* v, G3D::AABox& out) { v->m_box.getBounds(out); }
};

class PathPlane
{
public:
	uint32 planeid;
	GrowableArray<PathPlanePoint*> m_points;
};

class MapMgr;
class PathCalculatorThread : public ThreadBase
{
public:
	MapMgr* mgr;
	bool run();
};

struct TeleCoord
{
	uint32 id;
	Point loc;
	uint32 mapid;
	uint32 spelleffect;
	uint32 factionmask;
};

struct SpecialLocation
{
	G3D::AABox bounds;
	uint32 flags;
};

enum SpecialLocationFlags
{
	SPEC_LOC_FLAG_NO_TRANS_OR_PORT		= 0x00000001,
	SPEC_LOC_FLAG_NO_ITEM_DROP			= 0x00000002,
};

class TileScript;
struct TileScriptEntry //so we can do an array lookup for flags, quickest way
{
	uint32 procflags;
	TileScript* script;
};


class Unit;
class MapCell;
class Object;
class Creature;
class TimedObject;
class MapScript;
class TileScript;
class MapMgr : public ThreadBase, public EventHolder, public micropather::Graph, public EventableObject
{
public:
	MapCell* m_cells[CELL_X_COUNT][CELL_Y_COUNT];
	uint8* m_maplayer1;
	uint8* m_maplayer2;
	uint8* m_maplayer3;
	uint8* m_maplayer4;
	uint32 m_finalval; //for client checksum
	uint32 m_passflags[3220][3220];
	uint32 m_raypassflag;
	PathTileData* m_rayhitTile;
	G3D::Vector3 m_rayHitPoint;
	uint32 m_rayhity;
	bool m_pathcalculated;
	PathCellData** m_pathcelldata;
	G3D::KDTree<PathCellData*> m_pathtree;
	G3D::KDTree<TimedObjectData*> m_timedobjecttree;
	unordered_multimap<uint32, TeleCoord> m_telecoords;
	GrowableArray<SpecialLocation*> m_nomarklocations; //THIS NEEDS MOVE TO CELL LEVEL IN FUTURE FOR PERFORMANCE REASONS
	unordered_map<uint32, MapScript*> m_mapscripts;

	TileScriptEntry m_tilescripts[3220][3220];

	uint32 m_mapid;
	
	dtCrowd* m_crowd = dtAllocCrowd();
	dtNavMesh* m_navMesh;
	FastMutex m_navLoaderLock;
	dtNavMeshQuery* m_navQuery;

	uint32 m_serverType;

	void CreateMap(int32 cellx, int32 celly);
	void LoadMap(int32 cellx, int32 celly);
	void UnloadMap(int32 cellx, int32 celly)
	{
		sLog.Debug("MapMgr", "Unloading map info for cell %u, %u (%u points)", cellx, celly, m_pathcelldata[cellx][celly].m_tree.size());
		m_pathcelldata[cellx][celly].loaded = false;
		for (G3D::KDTree<PathTileData*>::Iterator itr = m_pathcelldata[cellx][celly].m_tree.begin(); itr != m_pathcelldata[cellx][celly].m_tree.end(); ++itr)
			delete *itr;
		m_pathcelldata[cellx][celly].m_tree.clear();

		RemoveMesh(cellx, celly);
	}
	bool IsLoaded(int32 cellx, int32 celly)
	{
		return m_pathcelldata[cellx][celly].loaded;
	}

	void OnTileBuildComplete(int32 x, int32 y, uint8* data, int datasize);

	void Init();

	void OnPathCompleted(PathPlanePoint* pathplanes);

	FastMutex m_insertMutex;
	FastMutex m_removeMutex;
	std::set<Object*> m_pendingInserts;
	std::set<Object*> m_pendingRemoves;
	unordered_map<uint32, Player*> m_playerStorage;
	std::set<Object*> m_activeObjects;
	std::set<Object*>::iterator m_activeObjectIterator;

	unordered_map<uint16, Object*> m_objectmap;

	void AddActiveObject(Object* o);

	void RemoveActiveObject(Object* o);

	INLINED Object* GetObject(uint16 id)
	{
		if (id == 0)
			return NULL;
		unordered_map<uint16, Object*>::iterator itr = m_objectmap.find(id);
		if (itr != m_objectmap.end())
			return itr->second;
		return NULL;
	}

	int32 GetInstanceID() { return m_mapid; }

	INLINED void AddObject(Object* u)
	{
		m_insertMutex.Acquire();
		m_pendingInserts.insert(u);
		m_insertMutex.Release();
	}

	void RemoveObject(Object* u)
	{
		m_removeMutex.Acquire();
		m_pendingRemoves.insert(u);
		m_removeMutex.Release();
	}

	void RemovePlayerByAccount(Account* acc);

	void EventRemovePlayerByAccount(uint32 accountid);

	MapMgr(uint32 mapid);


	bool run();

	float GetDistanceToObject(Object* u1, Object* u2);
	bool IsInRectRange(Object* u1, Object* u2);

	INLINED MapCell* GetCell(uint16 x, uint16 y) { return m_cells[x / CELL_WIDTH][y / CELL_HEIGHT]; }

	int32 GetCellX(int32 x) { return x / CELL_WIDTH; }
	int32 GetCellY(int32 y) { return y / CELL_WIDTH; }

	INLINED bool IsValidCell(uint16 cellx, uint16 celly) { return (cellx < CELL_X_COUNT && celly < CELL_Y_COUNT); }

	std::vector<MapCell*> GetCells(uint16 x, uint16 y, uint16 width, uint16 height)
	{
		std::vector<MapCell*> m_cellVector;
		
		MapCell* m_mainCell = GetCell(x, y);

		uint16 x1 = m_mainCell->x - width;
		uint16 x2 = m_mainCell->x + width + 1;
		uint16 y1 = m_mainCell->y - height;
		uint16 y2 = m_mainCell->y + height + 1;

		for (; x1!=x2; ++x1)
			for (uint16 tmp=y1; tmp!=y2; ++tmp)
				if (IsValidCell(x1, tmp))
					m_cellVector.push_back(m_cells[x1][tmp]);

		return m_cellVector;
	}

	void OnObjectChangePosition(Object* o);
	
	//GUID stuff
	std::deque<uint16> m_reusableIds;
	uint16 m_higuid;

	INLINED uint16 GenerateGUID()
	{
		uint16 id = 0;
		if (m_reusableIds.size() > 0)
		{
			id = m_reusableIds.front();
			m_reusableIds.pop_front();
		}
		else
			id = ++m_higuid;
		if (m_higuid == 0xFFFF)
			sLog.Error("MapMgr", "Hit GUID max.");
		return id;
	}

	INLINED void AddReusableGUID(uint16 id) { m_reusableIds.push_back(id); }

	uint32 CalculateCanPass(int32 x, int32 y);

	INLINED uint32 CanPass(int32 x, int32 y, uint32 passflags = PASS_FLAG_WALK)
	{
		if (x < 0 || y < 0 || x >= 3220 || y >= 3220)
			return 0;
		return m_passflags[x][y] & passflags;
	}

	INLINED uint8 GetMapLayer1(int32 x, int32 y)
	{
		int32 cellx = x / 20;
		int32 celly = y / 20;
		if (cellx >= 3220 || celly >= 3220)
			return 0;
		return m_maplayer1[cellx + celly * 3220];
	}

	INLINED uint8 GetMapLayer2(int32 x, int32 y)
	{
		int32 cellx = x / 20;
		int32 celly = y / 20;
		if (cellx >= 3220 || celly >= 3220)
			return 0;
		return m_maplayer2[cellx + celly * 3220];
	}

	INLINED uint8 GetMapLayer3(int32 x, int32 y)
	{
		int32 cellx = x / 20;
		int32 celly = y / 20;
		if (cellx >= 3220 || celly >= 3220)
			return 0;
		return m_maplayer3[cellx + celly * 3220];
	}

	INLINED uint8 GetMapLayer4(int32 x, int32 y)
	{
		int32 cellx = x / 20;
		int32 celly = y / 20;
		if (cellx >= 3220 || celly >= 3220)
			return 0;
		return m_maplayer4[cellx + celly * 3220];
	}

	//pathing stuff goes here
	INLINED void NodeToXY(void* node, int32* x, int32* y) 
	{
		int index = (int)node;
		*y = index / CELL_X_MAX;
		*x = index - *y * CELL_X_MAX;
	}

	void* XYToNode(int32 x, int32 y)
	{
		return (void*)(y * CELL_X_MAX + x);
	}

	float LeastCostEstimate(void* start, void* end)
	{
		int32 startx, starty, endx, endy;
		NodeToXY(start, &startx, &starty);
		NodeToXY(end, &endx, &endy);
		int32 dx = startx - endx;
		int32 dy = starty - endy;
		return (dx * dx + dy * dy);
	}

	void AdjacentCost(void* node, std::vector<micropather::StateCost>* n)
	{
	}

	bool InLineOfSight(int32 x1, int32 y1, int32 x2, int32 y2, uint32 passflags = PASS_FLAG_WALK, PathTileData** hit = NULL, G3D::Vector3* hitpoint = NULL);

	void PrintStateInfo(void* state) {}

	bool CreatePath(int32 startx, int32 starty, int32 endx, int32 endy, std::vector<PathPoint>* buf, float speed, uint32 nextmovetime, uint32 passflags = PASS_FLAG_WALK);

	void ComputePlanes(int32 x, int32 y, int32 x2, int32 y2, PathPlanePoint* pathplane);
	void MergePlanes(PathPlane* n, PathPlane* o);

	void SaveAllPlayers();
	TimedObject* GetTimedObject(int32 tilex, int32 tiley);
	Creature* GetCreatureById(uint32 id);
	void GetCreaturesById(uint32 id, std::vector<Creature*> & cres);
	void DestroyCreaturesById(uint32 id);
	Creature* CreateCreature(uint32 id, int32 x, int32 y, uint32 spawntime, bool respawn = false);
	Player* GetPlayer(uint32 playerid);

	void OnPlayerLinkBreak(Player* plr);
	void OnPlayerLinkJoin(Player* owner, Player* p);

	bool IsNoMarkLocation(int32 x, int32 y);
	SpecialLocation* GetSpecLoc(int32 x, int32 y)
	{
		G3D::Vector3 loc(x, y, 0);
		for (uint32 i = 0; i < m_nomarklocations.size(); ++i)
		{
			if (m_nomarklocations[i]->bounds.contains(loc))
				return m_nomarklocations[i];
		}
		return NULL;
	}

	MapScript* GetMapScript(uint32 scriptid)
	{
		unordered_map<uint32, MapScript*>::iterator itr = m_mapscripts.find(scriptid);
		if (itr == m_mapscripts.end())
			return NULL;
		return itr->second;
	}

	void HandleGlobalMessage(GlobalSay message);


	void SetMapLayer1(int32 x, int32 y, int32 v) { m_maplayer1[x + y * 3220] = v; }
	void SetMapLayer2(int32 x, int32 y, int32 v) { m_maplayer2[x + y * 3220] = v; }
	void SetMapLayer3(int32 x, int32 y, int32 v) { m_maplayer3[x + y * 3220] = v; }
	void SetMapLayer4(int32 x, int32 y, int32 v) { m_maplayer4[x + y * 3220] = v; }

	void SaveMap();

	void WriteMapLayer(MapCrypt & c, FILE* fo, uint8* ptr);

	void BuildMesh(uint32 flags);
	void BuildMap();
	void BuildMeshTile(int32 tx, int32 ty, uint32 flags);
	void LoadMesh(int32 tx, int32 ty)
	{
		char filename[1024];
		sprintf(filename, "map/%u_%u_%u.mesh", m_mapid, tx, ty);

		FILE* f = fopen(filename, "rb");

		if (f == NULL)
		{
			sMeshBuilder.QueueMeshBuild(this, tx, ty, MESH_BUILD_PRIORITY | MESH_BUILD_SAVE_TO_DISK); //prioritise where players are so they can play de game!
			return;
		}

		fseek(f, 0, SEEK_END);
		uint32 fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		uint8* data = (uint8*)dtAlloc(fsize, DT_ALLOC_PERM);
		fread(data, 1, fsize, f);
		fclose(f);

		m_navLoaderLock.Acquire();
		m_navMesh->addTile(data, fsize, DT_TILE_FREE_DATA, 0, 0);
		m_navLoaderLock.Release();
	}

	void RemoveMesh(int32 x, int32 y)
	{
		m_navLoaderLock.Acquire();
		m_navMesh->removeTile(m_navMesh->getTileRefAt(x, y, 0), 0, 0);
		m_navLoaderLock.Release();
	}

	bool FindPath(float x, float y, float x2, float y2, std::vector<PathPoint>* buf, float speed, uint32 passflags, uint32 nextmovetime)
	{
		float start[3];
		float end[3];
		start[0] = x;
		start[1] = 0;
		start[2] = y;
		end[0] = x2;
		end[1] = 0;
		end[2] = y2;

		float ext[3];
		ext[0] = 2;
		ext[1] = 4;
		ext[2] = 2;

		dtPolyRef startref, endref;
		dtQueryFilter filter;
		filter.setIncludeFlags(0xFF);

		dtPolyRef pathrefs[10000];
		int numpathrefs;

		GETFGUARD(m_navLoaderLock);

		m_navQuery->findNearestPoly(start, ext, &filter, &startref, NULL);
		m_navQuery->findNearestPoly(end, ext, &filter, &endref, NULL);

		if (!startref || !endref)
			return false;

		m_navQuery->findPath(startref, endref, start, end, &filter, pathrefs, &numpathrefs, 10000);

		if (numpathrefs == 0 || pathrefs[numpathrefs - 1] != endref)
			return false; //couldn't generate path, TODO: add support for partial generation

		float straightpath[10000 * 3];
		int straightpathcount;

		m_navQuery->findStraightPath(start, end, pathrefs, numpathrefs, straightpath, NULL, NULL, &straightpathcount, 10000);

		uint32 arrivetime = g_mstime;
		float lastx = x;
		float lasty = y;
		//test
		for (int i = 1; i < straightpathcount; ++i)
		{
			PathPoint p;

			p.x = straightpath[i * 3];
			p.y = straightpath[i * 3 + 2];

			p.prevarrivetime = arrivetime;
			p.arrivetime = arrivetime + (GetDistance(p.x, p.y, lastx, lasty) / speed * 100);
			arrivetime = p.arrivetime;

			lastx = straightpath[i * 3];
			lasty = straightpath[i * 3 + 2];

			buf->push_back(p);
		}

		return true;
	}

	float GetDistance( float x, float y, float x2, float y2 )
	{
		//difference
		float dx = x - x2;
		float dy = y - y2;
		return sqrt(pow(dx, 2) + pow(dy, 2));
	}


	void saveAll(const char* path, const dtNavMesh* mesh)
	{
		if (!mesh) return;
		
		FILE* fp = fopen(path, "wb");
		if (!fp)
			return;
		
		// Store header.
		NavMeshSetHeader header;
		header.magic = NAVMESHSET_MAGIC;
		header.version = NAVMESHSET_VERSION;
		header.numTiles = 0;
		for (int i = 0; i < mesh->getMaxTiles(); ++i)
		{
			const dtMeshTile* tile = mesh->getTile(i);
			if (!tile || !tile->header || !tile->dataSize) continue;
			header.numTiles++;
		}
		memcpy(&header.params, mesh->getParams(), sizeof(dtNavMeshParams));
		fwrite(&header, sizeof(NavMeshSetHeader), 1, fp);

		// Store tiles.
		for (int i = 0; i < mesh->getMaxTiles(); ++i)
		{
			const dtMeshTile* tile = mesh->getTile(i);
			if (!tile || !tile->header || !tile->dataSize) continue;

			NavMeshTileHeader tileHeader;
			tileHeader.tileRef = mesh->getTileRef(tile);
			tileHeader.dataSize = tile->dataSize;
			fwrite(&tileHeader, sizeof(tileHeader), 1, fp);

			fwrite(tile->data, tile->dataSize, 1, fp);
		}

		fclose(fp);
	}

	WorldQuestHandler m_WorldQuestHandler;
};

#endif
