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

#include "Shared.h"

inline unsigned int nextPow2(unsigned int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

inline unsigned int ilog2(unsigned int v)
{
	unsigned int r;
	unsigned int shift;
	r = (v > 0xffff) << 4; v >>= r;
	shift = (v > 0xff) << 3; v >>= shift; r |= shift;
	shift = (v > 0xf) << 2; v >>= shift; r |= shift;
	shift = (v > 0x3) << 1; v >>= shift; r |= shift;
	r |= (v >> 1);
	return r;
}

MapMgr::MapMgr(uint32 mapid)
{
	m_serverType = 2;
	m_navMesh = dtAllocNavMesh();
	m_navQuery = dtAllocNavMeshQuery();

	dtNavMeshParams params;

	int tileBits = rcMin((int)ilog2(nextPow2(RECAST_TILE_SIZE * RECAST_TILE_SIZE)), 12);
	if (tileBits > 12) tileBits = 12;
	int polyBits = 22 - tileBits;

	params.maxPolys = 1 << polyBits;
	params.maxTiles = 1 << tileBits;
	params.tileHeight = RECAST_TILE_SIZE;
	params.tileWidth = RECAST_TILE_SIZE;
	params.orig[0] = 0; params.orig[1] = -1; params.orig[2] = 0;

	m_navMesh->init(&params);
	m_navQuery->init(m_navMesh, 2048);

	m_mapid = mapid;
	m_raypassflag = false;
	m_pathcelldata = new PathCellData*[CELL_X_COUNT];
	for (uint32 i=0; i<CELL_X_COUNT; ++i)
		m_pathcelldata[i] = new PathCellData[CELL_Y_COUNT];

	for (uint32 x=0; x<CELL_X_COUNT; ++x)
	{
		for (uint32 y=0; y<CELL_Y_COUNT; ++y)
		{
			m_cells[x][y] = new MapCell(x, y);
			m_cells[x][y]->m_mapmgr = this;
			//need to calc aabox of cell now so we have a valid hash code
			G3D::Vector3 l(x * CELL_WIDTH, y * CELL_HEIGHT, -1);
			G3D::Vector3 h((x + 1) * CELL_WIDTH, (y + 1) * CELL_HEIGHT, 1);
			m_pathcelldata[x][y].m_bounds.set(l, h);
			m_pathtree.insert(&m_pathcelldata[x][y]);
		}
	}
	m_pathtree.balance();


	m_pathcalculated = false;

	m_activeObjectIterator = m_activeObjects.end();

	m_higuid = 1;

	for (unordered_map<uint32, CreatureSpawn*>::iterator itr=sCreatureSpawn.m_data.begin(); itr!=sCreatureSpawn.m_data.end(); ++itr)
	{
		CreatureSpawn* cspawn = itr->second;

		if (cspawn->mapid != m_mapid)
			continue;

		CreatureProto* cproto = sCreatureProto.GetEntry(cspawn->entry);
		Creature* cre = new Creature(cproto, cspawn);
		AddObject(cre);
	}

	//try and load the map data
	m_maplayer1 = new uint8[3220 * 3220];
	memset(m_maplayer1, 0, 3220 * 3220);
	m_maplayer2 = new uint8[3220 * 3220];
	memset(m_maplayer2, 0, 3220 * 3220);
	m_maplayer3 = new uint8[3220 * 3220];
	memset(m_maplayer3, 0, 3220 * 3220);
	m_maplayer4 = new uint8[3220 * 3220];
	memset(m_maplayer4, 0, 3220 * 3220);

	char mapfilename[2048];

	if (m_mapid == 0)
		strcpy(mapfilename, "main.map");
	else
		sprintf(mapfilename, "main%d.map", m_mapid);

	FILE* f = fopen(mapfilename, "rb");
	MapCrypt crypt(0, 0);
	
	if (f != NULL)
	{
		for (uint32 i=0; i<3220 * 3220;)
		{
			uint8 b;
			fread(&b, 1, 1, f);
			crypt.DecryptByte(b);

			uint32 numdescriptors = b & 0x7F;

			if (!(b & 0x80))
			{
				uint8 b2;
				for (uint32 j=0; j<numdescriptors + 1; ++j)
				{
					fread(&b2, 1, 1, f);
					crypt.DecryptByte(b2);
					m_maplayer1[i++] = b2;
				}
			}
			else
			{
				uint8 b2;
				fread(&b2, 1, 1, f);
				crypt.DecryptByte(b2);
				memset(&m_maplayer1[i], b2, numdescriptors);
				i += numdescriptors;
			}
		}

		for (uint32 i=0; i<3220 * 3220;)
		{
			uint8 b;
			fread(&b, 1, 1, f);
			crypt.DecryptByte(b);

			uint32 numdescriptors = b & 0x7F;

			if (!(b & 0x80))
			{
				uint8 b2;
				for (uint32 j=0; j<numdescriptors + 1; ++j)
				{
					fread(&b2, 1, 1, f);
					crypt.DecryptByte(b2);
					m_maplayer2[i++] = b2;
				}
			}
			else
			{
				uint8 b2;
				fread(&b2, 1, 1, f);
				crypt.DecryptByte(b2);
				memset(&m_maplayer2[i], b2, numdescriptors);
				i += numdescriptors;
			}
		}

		for (uint32 i=0; i<3220 * 3220;)
		{
			uint8 b;
			fread(&b, 1, 1, f);
			crypt.DecryptByte(b);

			uint32 numdescriptors = b & 0x7F;

			if (!(b & 0x80))
			{
				uint8 b2;
				for (uint32 j=0; j<numdescriptors + 1; ++j)
				{
					fread(&b2, 1, 1, f);
					crypt.DecryptByte(b2);
					m_maplayer3[i++] = b2;
				}
			}
			else
			{
				uint8 b2;
				fread(&b2, 1, 1, f);
				crypt.DecryptByte(b2);
				memset(&m_maplayer3[i], b2, numdescriptors);
				i += numdescriptors;
			}
		}

		for (uint32 i=0; i<3220 * 3220;)
		{
			uint8 b;
			fread(&b, 1, 1, f);
			crypt.DecryptByte(b);

			uint32 numdescriptors = b & 0x7F;

			if (!(b & 0x80))
			{
				uint8 b2;
				for (uint32 j=0; j<numdescriptors + 1; ++j)
				{
					fread(&b2, 1, 1, f);
					crypt.DecryptByte(b2);
					m_maplayer4[i++] = b2;
				}
			}
			else
			{
				uint8 b2;
				fread(&b2, 1, 1, f);
				crypt.DecryptByte(b2);
				memset(&m_maplayer4[i], b2, numdescriptors);
				i += numdescriptors;
			}
		}

		fread(&m_finalval, 4, 1, f);
		fclose(f);

		m_crowd->init(DT_MAX_CROWD_AGENTS, 50, m_navMesh);
	}

	MYSQL_RES* res = sDatabase.Query("select * from `tele_coords`;");
	if (res != NULL && mysql_num_rows(res) != 0)
	{
		MYSQL_ROW row = sDatabase.Fetch(res);
		while (row != NULL)
		{
			uint32 r = 0;
			uint32 id = atol(row[r++]);
			uint32 mapid = atol(row[r++]);

			if (mapid == m_mapid)
			{
				uint32 x = atol(row[r++]);
				uint32 y = atol(row[r++]);
				uint32 mapid2 = atol(row[r++]);
				uint32 x2 = atol(row[r++]);
				uint32 y2 = atol(row[r++]);
				uint32 key = x | (y << 16);
				Point p;
				p.x = x2;
				p.y = y2;
				TeleCoord t;
				t.id = id;
				t.loc = p;
				t.mapid = mapid2;
				t.spelleffect = atol(row[r++]);
				t.factionmask = atol(row[r++]);
				m_telecoords.insert(std::make_pair(key, t));
			}
			row = sDatabase.Fetch(res);
		}
	}
	sDatabase.Free(res);


	res = sDatabase.Query("select * from `timed_objects`;");
	if (res != NULL && mysql_num_rows(res) != 0)
	{
		MYSQL_ROW row = sDatabase.Fetch(res);
		while (row != NULL)
		{
			uint32 r = 1;
			uint32 mapid = atol(row[r++]);

			if (m_mapid == mapid)
			{
				uint32 x = atol(row[r++]) * 20;
				uint32 y = atol(row[r++]) * 20;
				uint32 type = atol(row[r++]);
				uint32 timer = atol(row[r++]);
				uint32 offset = atol(row[r++]);

				TimedObject* tobj = new TimedObject;
				tobj->SetPosition(x, y);
				tobj->m_timedtype = type;
				tobj->m_mapmgr = this;
				tobj->AddEvent(&TimedObject::SetClosed, EVENT_TIMEDOBJECT_CLOSE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
				if (timer != 0)
					tobj->AddEvent(&TimedObject::StartSwitchStates, timer, EVENT_TIMEDOBJECT_STARTSWITCHSTATE, offset, 1, EVENT_FLAG_NOT_IN_WORLD);

				AddObject(tobj);
			}

			row = sDatabase.Fetch(res);
		}
	}
	sDatabase.Free(res);

	res = sDatabase.Query("select * from `locations_blocked`;");
	if (res != NULL && mysql_num_rows(res) != 0)
	{
		MYSQL_ROW row = sDatabase.Fetch(res);
		while (row != NULL)
		{
			uint32 r = 1;
			uint32 mapid = atol(row[r++]);

			if (mapid == m_mapid)
			{
				uint32 x1 = atol(row[r++]) * 20;
				uint32 y1 = atol(row[r++]) * 20;
				uint32 x2 = atol(row[r++]) * 20 + 20;
				uint32 y2 = atol(row[r++]) * 20 + 20;
				uint32 flags = atol(row[r++]);

				G3D::Vector3 lo(x1, y1, -1);
				G3D::Vector3 hi(x2, y2, 1);
				G3D::AABox aabox(lo, hi);
				SpecialLocation* loc = new SpecialLocation;
				loc->bounds = aabox;
				loc->flags = flags;
				m_nomarklocations.push(loc);
			}
			row = sDatabase.Fetch(res);
		}
	}
	sDatabase.Free(res);

	sScriptMgr.CreateMapScripts(this);
	sScriptMgr.CreateTileScripts(this);
}

void MapMgr::ComputePlanes(int32 x, int32 y, int32 x2, int32 y2, PathPlanePoint* pathlayer)
{
	if (x >= 0 && y >= 0 && x2 >= 0 && y2 >= 0 && x < 3220 && y < 3220 && x2 < 3220 && y2 < 3220 && pathlayer[x + y * 3220].plane != NULL && pathlayer[x + y * 3220].plane != pathlayer[x2 + y2 * 3220].plane)
	{
		//MergePlanes(pathlayer[x + y * 3220].plane, pathlayer[x2 + y2 * 3220].plane);
	}
}

void MapMgr::MergePlanes(PathPlane* n, PathPlane* o)
{
	//reduce allocs
	if (n->m_points.size() < o->m_points.size())
	{
		MergePlanes(o, n);
		return;
	}
	//ok firstly we iterate over old
	for (uint32 i = 0; i < o->m_points.size(); ++i)
	{
		o->m_points[i]->plane = n;
		n->m_points.push(o->m_points[i]);
	}

	delete o;
}

float MapMgr::GetDistanceToObject(Object* u1, Object* u2)
{
	if (u1 == NULL || u2 == NULL)
		return 0.0f;
	
	//difference
	float dx = u1->m_positionX - u2->m_positionX;
	float dy = u1->m_positionY - u2->m_positionY;

	return sqrt(pow(dx, 2) + pow(dy, 2));
}

bool MapMgr::IsInRectRange(Object* u1, Object* u2)
{
	int32 dx = u1->m_positionX - u2->m_positionX;
	int32 dy = u1->m_positionY - u2->m_positionY;

	return (abs(dx) <= SIGHT_RANGE_X && abs(dy) <= SIGHT_RANGE_Y);
}

bool MapMgr::run()
{
	Init();
	
	while (THREAD_LOOP_CONDS)
	{
		uint32 t1 = GetMSTime();

		m_crowd->update(float(UPDATE_INTERVAL) / float(1000), nullptr); //update interval is in seconds

		EventUpdate();
		m_insertMutex.Acquire();
		if (m_pendingInserts.size() > 0)
		{
			for (std::set<Object*>::iterator itr=m_pendingInserts.begin(); itr!=m_pendingInserts.end(); ++itr)
			{
				(*itr)->m_mapmgr = this;
				(*itr)->m_inworld = true;
				(*itr)->m_serverId = GenerateGUID();

				bool fireteleportcomplete = false;

				if ((*itr)->IsPlayer() && TO_PLAYER(*itr)->m_pendingmaptransfer.newmapid != 0xFFFFFFFF) //map transfer
				{
					//update x, y
					(*itr)->m_positionX = TO_PLAYER(*itr)->m_pendingmaptransfer.newx;
					(*itr)->m_positionY = TO_PLAYER(*itr)->m_pendingmaptransfer.newy;

					//reset thing
					TO_PLAYER(*itr)->m_pendingmaptransfer.newmapid = 0xFFFFFFFF;

					fireteleportcomplete = true;
				}

				OnObjectChangePosition(*itr); //this will push to correct cell and build inrange, w000t.
				if ((*itr)->IsPlayer())
				{
					TO_PLAYER(*itr)->m_mapid = m_mapid;
					m_playerStorage.insert(std::make_pair(TO_PLAYER(*itr)->m_playerId, TO_PLAYER(*itr)));
					TO_PLAYER(*itr)->BuildInitialLoginPacket();
					AddActiveObject(*itr);
					CALL_MAPSCRIPT_FUNC(OnPlayerAddToWorld, TO_PLAYER(*itr));

					if (fireteleportcomplete)
						TO_PLAYER(*itr)->OnTeleportComplete();
				}
				m_objectmap.insert(std::make_pair((*itr)->m_serverId, (*itr)));
				(*itr)->OnAddToWorld();
			}
			m_pendingInserts.clear();
		}
		m_insertMutex.Release();

		m_removeMutex.Acquire();
		if (m_pendingRemoves.size() > 0)
		{
			for (std::set<Object*>::iterator itr=m_pendingRemoves.begin(); itr!=m_pendingRemoves.end(); ++itr)
			{
				(*itr)->OnPreRemoveFromWorld();
				(*itr)->m_mapmgr = NULL;
				(*itr)->m_inworld = false;
				AddReusableGUID((*itr)->m_serverId);
				(*itr)->UpdateInRangeSet();
				for (std::vector<MapCell*>::iterator itr2=(*itr)->m_cellVector.begin(); itr2!=(*itr)->m_cellVector.end(); ++itr2)
				{
					(*itr2)->RemoveObject(*itr);

					if ((*itr)->IsPlayer())
						(*itr2)->DecPlayerRefs();
				}
				RemoveActiveObject(*itr);
				(*itr)->m_cellVector.clear();
				(*itr)->m_cell = NULL;
				m_objectmap.erase((*itr)->m_serverId);

				if ((*itr)->IsPlayer())
				{
					m_playerStorage.erase(TO_PLAYER(*itr)->m_playerId);

					Player* p = TO_PLAYER(*itr);

					if (p->m_pendingmaptransfer.newmapid != 0xFFFFFFFF)
					{
						MapMgr* newmap = sInstanceMgr.GetMap(p->m_pendingmaptransfer.newmapid);

						if (newmap == NULL)
							p->Delete(); //uh oh
						else
							newmap->AddObject(p);
					}
				}

				(*itr)->OnRemoveFromWorld();

				if ((*itr)->m_deleteonremove)
					(*itr)->DecRef();
			}
			m_pendingRemoves.clear();
		}
		m_removeMutex.Release();

		m_activeObjectIterator = m_activeObjects.begin();

		while (m_activeObjectIterator != m_activeObjects.end())
		{
			Object* o = *m_activeObjectIterator;
			++m_activeObjectIterator;
			o->Update();
		}

		for (unordered_map<uint32, Player*>::iterator itr = m_playerStorage.begin(); itr != m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			p->BuildUpdatePacket();

			//logout inactive player?
			if (p->m_account->m_lastaction <= time(NULL) - 300)
			{
				p->m_account->Logout(false);
				p->m_account->Logout(true);
			}
		}


		uint32 t2 = GetMSTime();
		int32 diff = t2 - t1;
		if (diff >= 0 && diff < UPDATE_INTERVAL)
			Sleep(UPDATE_INTERVAL - diff); //fucker eating ma cpu
	}

	return false;
}

void MapMgr::OnObjectChangePosition(Object* o)
{
	MapCell* new_cell = GetCell(o->m_positionX, o->m_positionY);
	ASSERT(new_cell);

	if (o->m_cell != new_cell)
	{
		//this can be null if we are adding to world
		if (o->m_cell != NULL)
			o->m_cell->RemoveObject(o);
		new_cell->InsertObject(o);

		if (new_cell->active && m_activeObjects.find(o) == m_activeObjects.end())
			AddActiveObject(o);
		else if (!new_cell->active && !o->IsPlayer())
			RemoveActiveObject(o);

		o->m_cell = new_cell;

		std::vector<MapCell*> cellVector = GetCells(o->m_positionX, o->m_positionY, 1, 1);

		if (o->IsPlayer()) //do activation/deactivation of cells
		{
			//Get a 3x3 box around the unit
			
			std::vector<MapCell*>::iterator itr, itr2;

			for (itr=cellVector.begin(); itr!=cellVector.end(); ++itr)
			{
				bool activate = true;
				for (itr2=o->m_cellVector.begin(); itr2!=o->m_cellVector.end(); ++itr2)
					if ((*itr) == (*itr2))
					{
						activate = false;
						break;
					}

				if (activate)
					(*itr)->IncPlayerRefs();
			}

			for (itr=o->m_cellVector.begin(); itr!=o->m_cellVector.end(); ++itr)
			{
				bool deactivate = true;
				for (itr2=cellVector.begin(); itr2!=cellVector.end(); ++itr2)
					if ((*itr) == (*itr2))
					{
						deactivate = false;
						break;
					}

				if (deactivate)
					(*itr)->DecPlayerRefs();
			}
		}

		o->m_cellVector = cellVector;
	}

	//Remove out of range inrange
	o->UpdateInRangeSet();

	//And finally add new inrange stuff from our 3x3 cell grid that is inrange
	for (std::vector<MapCell*>::iterator itr=o->m_cellVector.begin(); itr!=o->m_cellVector.end(); ++itr)
	{
		for (std::set<Object*>::iterator itr2=(*itr)->m_objects.begin(); itr2!=(*itr)->m_objects.end(); ++itr2)
		{
			if (o->IsInRectRange(*itr2) && o != (*itr2) && !o->IsInRange(*itr2) && o->CanSee(*itr2))
			{
				o->AddInRangeObject(*itr2);
				(*itr2)->AddInRangeObject(o);
			}
		}

		for (std::map<uint32, ItemGroup*>::iterator itr2 = (*itr)->m_items.begin(); itr2 != (*itr)->m_items.end(); ++itr2)
		{
			int32 x = itr2->second->m_cellx * 20 + 10;
			int32 y = itr2->second->m_celly * 20 + 10;
			if (o->IsInRectRange(x, y))
				o->AddInRangeItemGroup(itr2->second);
		}
	}
}

bool MapMgr::CreatePath( int32 startx, int32 starty, int32 endx, int32 endy, std::vector<PathPoint>* buf, float speed, uint32 nextmovetime, uint32 passflags /*= PASS_FLAG_WALK*/ )
{
	return FindPath(startx, starty, endx, endy, buf, speed, passflags, nextmovetime);
}

void MapMgr::Init()
{
	for (uint32 x = 0; x < 3220; ++x)
		for (uint32 y = 0; y < 3220; ++y)
			m_passflags[x][y] = CalculateCanPass(x, y);
	BuildMap();
	BuildMesh(MESH_BUILD_SAVE_TO_DISK);
}

void MapMgr::RemoveActiveObject( Object* o )
{
	if (m_activeObjectIterator != m_activeObjects.end() && *m_activeObjectIterator == o)
		++m_activeObjectIterator;
	m_activeObjects.erase(o);
	o->m_active = false;
	o->OnInactive();
}

void MapMgr::AddActiveObject( Object* o )
{
	m_activeObjects.insert(o);
	o->m_active = true;
	o->OnActive();
}

bool MapMgr::InLineOfSight( int32 x1, int32 y1, int32 x2, int32 y2, uint32 passflags, PathTileData** hit, G3D::Vector3* hitpoint)
{
	if (x1 == x2 && y1 == y2)
		return true;
	G3D::Vector3 p1(x1, y1, 0);
	G3D::Vector3 p2(x2, y2, 0);
	float dist = abs((p2 - p1).magnitude());
	float originaldist = dist;
	G3D::Ray r = G3D::Ray::fromOriginAndDirection(p1, (p2 - p1) / dist);

	m_raypassflag = passflags;
	m_pathtree.intersectRay(r, OnIntersectPathCellData, dist);
	m_timedobjecttree.intersectRay(r, OnIntersectTimedObjectData, dist);

	if (hit != NULL)
		*hit = m_rayhitTile;
	if (hitpoint != NULL)
		*hitpoint = m_rayHitPoint;

	return (dist == originaldist);
}

void MapMgr::CreateMap( int32 cellx, int32 celly )
{
	char filename[1024];
	sprintf(filename, "maps/%u_%u_%u.map", m_mapid, cellx, celly);
	FILE* ftest = fopen(filename, "r");
	if (ftest != NULL)
	{
		fclose(ftest);
		return;
	}
	FILE* f = fopen(filename, "wb");
	if (f == NULL)
		return;

	//uhh
	int32 startx = cellx * CELL_WIDTH / 20;
	int32 endx = (cellx + 1) * CELL_WIDTH / 20;
	int32 starty = celly * CELL_HEIGHT / 20;
	int32 endy = (celly + 1) * CELL_HEIGHT / 20;
	GrowableArray<PathTileData> arr;
	for (int32 x = startx; x <= endx; ++x)
	{
		for (int32 y = starty; y <= endy; ++y)
		{
			uint32 passflags = CalculateCanPass(x, y);
			if (passflags & PASS_FLAG_WALK)
				continue; //no need, nothing will every stop it, save some ray time
			int32 realx = x * 20;
			int32 realy = y * 20;
			//make a box in the g3d tree blocking line of sight checks
			//this box is bigger then the tile, it's to prevent pathing errors
			G3D::Vector3 l(realx, realy, -1);
			G3D::Vector3 h(19.9f + realx, 19.9f + realy, 1);
			G3D::AABox a(l, h);
			PathTileData tdata;
			tdata.m_box = a;
			tdata.passflags = passflags;
			tdata.maplayer1 = GetMapLayer1(realx, realy);
			tdata.maplayer2 = GetMapLayer2(realx, realy);
			tdata.maplayer3 = GetMapLayer3(realx, realy);
			arr.push(tdata);
		}
	}

	uint32 numpoints = arr.size();
	sLog.Debug("MapMgr", "Creating map info for cell %u, %u (%u points)", cellx, celly, arr.size());
	fwrite(&numpoints, 4, 1, f);
	for (uint32 i = 0; i < numpoints; ++i)
	{
		const PathTileData & tdata = arr[i];
		fwrite(&tdata.passflags, 4, 1, f);
		float x = tdata.m_box.low().x;
		float y = tdata.m_box.low().y;
		float z = tdata.m_box.low().z;
		fwrite(&x, 4, 1, f);
		fwrite(&y, 4, 1, f);
		fwrite(&z, 4, 1, f);	
		x = tdata.m_box.high().x;
		y = tdata.m_box.high().y;
		z = tdata.m_box.high().z;
		fwrite(&x, 4, 1, f);
		fwrite(&y, 4, 1, f);
		fwrite(&z, 4, 1, f);
		uint8 m1 = tdata.maplayer1;
		uint8 m2 = tdata.maplayer2;
		uint8 m3 = tdata.maplayer3;
		fwrite(&m1, 1, 1, f);
		fwrite(&m2, 1, 1, f);
		fwrite(&m3, 1, 1, f);
	}

	fclose(f);
}

uint32 MapMgr::CalculateCanPass( int32 x, int32 y )
{
#define ADD_FLAG1(val, flag) if (m_maplayer1[x + y * 3220] == val) flags |= flag
#define ADD_FLAG2(val, flag) if (m_maplayer2[x + y * 3220] == val) flags |= flag
	uint32 flags = PASS_FLAG_WALK | PASS_FLAG_GAS | PASS_FLAG_TELEPORT;

	if (x < 0 || y < 0 || x >= 3220 || y >= 3220)
		return 0;

	if (m_maplayer1[x + y * 3220] >= 0x45)
		flags &= ~(PASS_FLAG_WALK | PASS_FLAG_GAS);

	switch (m_maplayer1[x + y * 3220])
	{
	case 203: //small plant/shrub
	case 0:
	case 0x47:
	case 0x48:
	case 0x49:
	case 0x4E:
	case 0x4F:
	case 0x55:
	case 0x56:
	case 0x57:
	case 0x58:
	case 0x59:
	case 0x5D:
	case 0x64:
	case 0x65: //a plant, or shrub (data of visual in layer 3, no rotation)
	case 0x66:
	case 0x62:
	case 0xA5:
	case 0xA7:
	case 0xAB:
	case 0xAE:
	case 0xB3:
	case 0xDD:
	case 0xF0:
	case 0xF1:
	case 0xF2:
	case 0xF3:
	case 0xF5:
	case 0xF9:
		{
			flags |= (PASS_FLAG_WALK | PASS_FLAG_GAS);
		} break;
	case 0xCC:
	case 0x6F:
		{
			flags &= ~(PASS_FLAG_WALK | PASS_FLAG_GAS);
		} break;
	}

	ADD_FLAG1(69, PASS_FLAG_GAS);
	ADD_FLAG1(70, PASS_FLAG_GAS);
	ADD_FLAG1(90, PASS_FLAG_GAS);
	ADD_FLAG1(96, PASS_FLAG_GAS);
	ADD_FLAG1(103, PASS_FLAG_GAS);
	ADD_FLAG1(104, PASS_FLAG_GAS);
	ADD_FLAG1(105, PASS_FLAG_GAS);
	ADD_FLAG1(106, PASS_FLAG_GAS);
	ADD_FLAG1(107, PASS_FLAG_GAS);
	ADD_FLAG1(108, PASS_FLAG_GAS);
	ADD_FLAG1(109, PASS_FLAG_GAS);
	ADD_FLAG1(112, PASS_FLAG_GAS);
	ADD_FLAG1(113, PASS_FLAG_GAS);
	ADD_FLAG1(114, PASS_FLAG_GAS);
	ADD_FLAG1(118, PASS_FLAG_GAS);
	ADD_FLAG1(119, PASS_FLAG_GAS);
	ADD_FLAG1(120, PASS_FLAG_GAS);
	ADD_FLAG1(142, PASS_FLAG_GAS);
	ADD_FLAG1(143, PASS_FLAG_GAS);
	ADD_FLAG1(163, PASS_FLAG_GAS);
	ADD_FLAG1(164, PASS_FLAG_GAS);
	ADD_FLAG1(166, PASS_FLAG_GAS);
	ADD_FLAG1(168, PASS_FLAG_GAS);
	ADD_FLAG1(180, PASS_FLAG_GAS);
	ADD_FLAG1(200, PASS_FLAG_GAS);
	ADD_FLAG1(201, PASS_FLAG_GAS);
	ADD_FLAG1(202, PASS_FLAG_GAS);
	ADD_FLAG1(203, PASS_FLAG_GAS);
	ADD_FLAG1(215, PASS_FLAG_GAS);
	ADD_FLAG1(235, PASS_FLAG_GAS);
	ADD_FLAG1(254, PASS_FLAG_GAS); //bed

	if (m_maplayer2[x + y * 3220] >= 85) //most walls and shiz
		flags &= ~(PASS_FLAG_WALK | PASS_FLAG_GAS);

	switch (m_maplayer2[x + y * 3220])
	{
	case 0x4A: //mystic bars
	case 0xCF: //pit
	case 0xD0: //pit
	case 0xD1: //pit
	case 0xD2: //pit
	case 0xD3: //pit
	case 0xFF: //rust wall
		flags &= ~(PASS_FLAG_WALK | PASS_FLAG_GAS); break;
	case 0xDD: //door opening
	case 0xE6: //drop wall
	case 0xE7: //drop wall
	case 0xE8: //drop wall
	case 0xE9: //drop wall
	case 0xEA: //drop wall
	case 0xF9: //teleports
	case 0xAD: //fake wall
		flags |= (PASS_FLAG_WALK | PASS_FLAG_GAS); break;
	case 0x50: //new water
		{
			uint8 waterdepth = m_maplayer4[x + y * 3220];
			if (waterdepth > 24) //this is where it goes deep like old version
			{
				flags &= ~PASS_FLAG_WALK;
				flags |= PASS_FLAG_SWIM;
			}
		}
	}

	ADD_FLAG2(116, PASS_FLAG_WALK | PASS_FLAG_GAS);
	ADD_FLAG2(123, PASS_FLAG_GAS);
	ADD_FLAG2(158, PASS_FLAG_GAS);

	return flags;
}

void MapMgr::LoadMap( int32 cellx, int32 celly )
{
	char filename[1024];
	sprintf(filename, "maps/%u_%u_%u.map", m_mapid, cellx, celly);
	FILE* f = fopen(filename, "rb");
	if (f == NULL)
	{
		sLog.Debug("Map", "Cannot open map for %u, %u", cellx, celly);
		return;
	}
	uint32 numpoints;
	fread(&numpoints, 4, 1, f);
	sLog.Debug("MapMgr", "Loading map info for cell %u, %u (%u points)", cellx, celly, numpoints);
	for (uint32 i=0; i<numpoints; ++i)
	{
		uint32 passflags;
		fread(&passflags, 4, 1, f);
		float x, y, z;
		fread(&x, 4, 1, f);
		fread(&y, 4, 1, f);
		fread(&z, 4, 1, f);
		G3D::Vector3 v(x, y, z);
		fread(&x, 4, 1, f);
		fread(&y, 4, 1, f);
		fread(&z, 4, 1, f);
		G3D::Vector3 v2(x, y, z);
		uint8 m1, m2, m3;
		fread(&m1, 1, 1, f);
		fread(&m2, 1, 1, f);
		fread(&m3, 1, 1, f);
		G3D::AABox a(v, v2);
		PathTileData* tmp = new PathTileData;
		tmp->m_mapmgr = this;
		tmp->m_box = a;
		tmp->passflags = passflags;
		tmp->maplayer1 = m1;
		tmp->maplayer2 = m2;
		tmp->maplayer3 = m3;
		m_pathcelldata[cellx][celly].m_tree.insert(tmp);
	}
	m_pathcelldata[cellx][celly].loaded = true;
	m_pathcelldata[cellx][celly].m_tree.balance();
	fclose(f);

	LoadMesh(cellx, celly);
}

void MapMgr::SaveAllPlayers()
{
	for (unordered_map<uint32, Player*>::iterator itr = m_playerStorage.begin(); itr != m_playerStorage.end(); ++itr)
		itr->second->SaveToDB();
}

TimedObject* MapMgr::GetTimedObject( int32 tilex, int32 tiley )
{
	int32 cellx = tilex * 20 + 10;
	int32 celly = tiley * 20 + 10;

	MapCell* cell = GetCell(cellx, celly);
	if (cell == NULL)
		return NULL;
	Object* o;
	for (std::set<Object*>::iterator itr = cell->m_objects.begin(); itr != cell->m_objects.end(); ++itr)
	{
		o = *itr;
		if (!o->IsTimedObject())
			continue;
		if ((o->m_positionX / 20) == tilex && (o->m_positionY / 20) == tiley)
			return TO_TIMEDOBJECT(o);
	}

	return NULL;
}

Creature* MapMgr::GetCreatureById( uint32 id )
{
	Object* o;
	for (unordered_map<uint16, Object*>::iterator itr = m_objectmap.begin(); itr != m_objectmap.end(); ++itr)
	{
		o = itr->second;
		if (!o->IsCreature())
			continue;
		if (TO_CREATURE(o)->m_proto->entry == id)
			return TO_CREATURE(o);
	}
	return NULL;
}

Creature* MapMgr::CreateCreature( uint32 id, int32 x, int32 y, uint32 spawntime, bool respawn /*= false*/ )
{
	CreatureProto* prot = sCreatureProto.GetEntry(id);
	if (prot == NULL)
		return NULL;
	Creature* cre = new Creature(prot, NULL);
	cre->SetSpawn(x, y);
	cre->SetPosition(x, y);
	cre->m_norespawn = !respawn;
	if (spawntime != 0)
		cre->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, spawntime, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddObject(cre);
	return cre;
}

bool MapMgr::IsNoMarkLocation( int32 x, int32 y )
{
	SpecialLocation* loc = GetSpecLoc(x, y);
	if (loc != NULL && loc->flags & SPEC_LOC_FLAG_NO_TRANS_OR_PORT)
		return true;
	return false;
}

Player* MapMgr::GetPlayer( uint32 playerid )
{
	unordered_map<uint32, Player*>::iterator itr = m_playerStorage.find(playerid);
	if (itr == m_playerStorage.end())
		return NULL;
	return itr->second;
}

void MapMgr::OnPlayerLinkBreak(Player * plr)
{
	CALL_MAPSCRIPT_FUNC(OnPlayerBreakLink, plr);
}

void MapMgr::OnPlayerLinkJoin(Player * owner, Player * p)
{
	CALL_MAPSCRIPT_FUNC(OnPlayerJoinLink, owner, p);
}

void MapMgr::RemovePlayerByAccount( Account* acc )
{
	AddEvent(&MapMgr::EventRemovePlayerByAccount, acc->m_data->Id, EVENT_MAPMGR_REMOVEPLAYER, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
}

void MapMgr::EventRemovePlayerByAccount( uint32 accountid )
{
	for (unordered_map<uint32, Player*>::iterator itr = m_playerStorage.begin(); itr != m_playerStorage.end(); ++itr)
	{
		if (itr->second->m_account->m_data->Id == accountid)
		{
			itr->second->SaveToDB();
			itr->second->Delete();
		}
	}
}

void MapMgr::HandleGlobalMessage( GlobalSay message )
{
	for (unordered_map<uint32, Player*>::iterator itr = m_playerStorage.begin(); itr != m_playerStorage.end(); ++itr)
	{
		Player* p = itr->second;
		if (p->m_ignoredNames.find(message.name) != p->m_ignoredNames.end())
			continue;
		if (message.to_player != 0 && p->m_playerId != message.to_player)
			continue;
		if (message.to_link != 0 && p->GetLinkID() != message.to_link)
			continue;
		p->m_globalSays.push_back(message);
	}
}

void MapMgr::SaveMap()
{
	//rudamentry save lol

	char filename[1024];

	if (m_mapid == 0)
		sprintf(filename, "main.map");
	else
		sprintf(filename, "main%d.map", m_mapid);

	FILE* fo = fopen(filename, "wb");

	if (fo == NULL)
		return;

	MapCrypt c(0, 0);

	WriteMapLayer(c, fo, m_maplayer1);
	WriteMapLayer(c, fo, m_maplayer2);
	WriteMapLayer(c, fo, m_maplayer3);
	WriteMapLayer(c, fo, m_maplayer4);


	fwrite(&m_finalval, 4, 1, fo);

	fclose(fo);
}

void MapMgr::WriteMapLayer( MapCrypt & c, FILE* fo, uint8* ptr )
{
	uint8 pieces = 1;
	uint8 matchedpiece = 0;
	for (uint32 i = 0; i < 3220 * 3220;)
	{
		int32 x = i % 3220;
		int32 y = i / 3220;
		if (i < (3220 * 3220 - 1) && x < 3219)
		{
			if (pieces == 1 && ptr[i] == ptr[i + 1])
			{
				//first match
				matchedpiece = ptr[i];
				pieces |= 0x80;
				++pieces;
				++i;
				continue;
			}
			if (pieces & 0x80 && ptr[i + 1] == matchedpiece && pieces < 0xFF)
			{
				++pieces; 
				++i;
				continue;
			}
		}

		if (pieces == 1)
		{
			uint32 piecesleft = (3220 * 3220) - i;
			pieces = 0;
			uint8 encpieces = pieces;
			c.EncryptByte(encpieces);
			uint8* tmpbuf = new uint8[pieces + 1];
			memcpy(tmpbuf, &ptr[i], pieces + 1);
			for (uint8 j = 0; j < pieces + 1; ++j)
				c.EncryptByte(tmpbuf[j]);
			fwrite(&encpieces, 1, 1, fo);
			fwrite(tmpbuf, 1, pieces + 1, fo);
			delete[] tmpbuf;
			i += pieces + 1;
		}
		else
		{
			uint8 enc = pieces;
			c.EncryptByte(enc);
			fwrite(&enc, 1, 1, fo);
			enc = ptr[i];
			c.EncryptByte(enc);
			fwrite(&enc, 1, 1, fo);
			++i; //WE INCREASE BY 1 LOL
		}
		pieces = 1;
	}
}

void MapMgr::DestroyCreaturesById( uint32 id )
{
	Object* o;
	for (unordered_map<uint16, Object*>::iterator itr = m_objectmap.begin(); itr != m_objectmap.end(); ++itr)
	{
		o = itr->second;
		if (!o->IsCreature())
			continue;
		if (TO_CREATURE(o)->m_proto->entry == id)
			o->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void MapMgr::GetCreaturesById( uint32 id, std::vector<Creature*> & cres )
{
	Object* o;
	for (unordered_map<uint16, Object*>::iterator itr = m_objectmap.begin(); itr != m_objectmap.end(); ++itr)
	{
		o = itr->second;
		if (!o->IsCreature())
			continue;
		if (TO_CREATURE(o)->m_proto->entry == id)
			cres.push_back(TO_CREATURE(o));
	}
}

void MapMgr::OnPathCompleted( PathPlanePoint* pathplanes )
{
}

void MapMgr::BuildMesh( uint32 flags )
{
	uint32 numtiles = 65536 / RECAST_TILE_SIZE;
	for (uint32 x = 0; x < numtiles; ++x)
	{
		for (uint32 y = 0; y < numtiles; ++y)
		{
			sMeshBuilder.QueueMeshBuild(this, x, y, flags);
		}
	}
}

void MapMgr::BuildMap()
{
	uint32 numtiles = 65536 / RECAST_TILE_SIZE;
	for (uint32 x = 0; x < numtiles; ++x)
	{
		for (uint32 y = 0; y < numtiles; ++y)
		{
			CreateMap(x, y);
		}
	}
}

void MapMgr::BuildMeshTile( int32 tx, int32 ty, uint32 flags )
{
	sLog.Debug("MapMgr", "Building mesh %u %u flags %u", tx, ty, flags);
	char filename[1024];
	sprintf(filename, "map/%u_%u_%u.mesh", m_mapid, tx, ty);

	if (flags & MESH_BUILD_SAVE_TO_DISK) //if saving to disk, we don't run if already saved to disk, duh
	{
		FILE* ftest = fopen(filename, "r");
		if (ftest != NULL)
		{
			fclose(ftest);
			return;
		}
	}

	//config
	uint32 maxtrisperchunk = 65535;
	uint32 rcheight = 1;
	uint32 rcwidth = 1;
	uint32 boundmin = 0;
	uint32 boundmax = 65536;
	uint32 bordersize = 50;
	uint32 walkableheight = 3;
	uint32 walkableclimb = 1;
	float bmin[3];
	float bmax[3];

	//calc bounds
	bmin[0] = boundmin + tx * RECAST_TILE_SIZE;
	bmin[2] = boundmin + ty * RECAST_TILE_SIZE;
	bmax[0] = boundmin + (tx + 1) * RECAST_TILE_SIZE;
	bmax[2] = boundmin + (ty + 1) * RECAST_TILE_SIZE;
	bmin[1] = -50;
	bmax[1] = 50;

	//bounds MUST be increased for bordering
	bmin[0] -= bordersize;
	bmin[2] -= bordersize;
	bmax[0] += bordersize;
	bmax[2] += bordersize;

	rcContext ctx;
	rcHeightfield* hf = rcAllocHeightfield();

	if (hf == NULL)
	{
		sLog.Error("MESHBUILD", "Cannot allocate heightfield");
		return;
	}

	if (!rcCreateHeightfield(&ctx, *hf, RECAST_TILE_SIZE + (bordersize * 2), RECAST_TILE_SIZE + (bordersize * 2), bmin, bmax, rcwidth, rcheight))
		sLog.Error("MESHBUILD", "error: rcCreateHeightField");


	uint8* triflags = new uint8[maxtrisperchunk];
	memset(triflags, 0, maxtrisperchunk);
	float* tris = new float[maxtrisperchunk * 9];
	int* triinds = new int[maxtrisperchunk * 3];
	uint32 numtris = 0;

	for (int x = bmin[0] - (bordersize * 2); x <= bmax[0] + (bordersize * 2); x += 20)
	{
		for (int y = bmin[2] - (bordersize * 2); y <= bmax[2] + (bordersize * 2); y += 20)
		{
			int32 gametilex = x / 20;
			int32 gametiley = y / 20;
			uint32 passflags = CanPass(gametilex, gametiley, PASS_FLAG_WALK);

			if (!(passflags & PASS_FLAG_WALK))
			//if (passflags == 0)
				continue; // we don't need this

			tris[numtris * 9] = gametilex * 20 + 20;
			tris[numtris * 9 + 1] = 0;
			tris[numtris * 9 + 2] = gametiley * 20 + 20;
			tris[numtris * 9 + 3] = gametilex * 20 + 20;
			tris[numtris * 9 + 4] = 0;
			tris[numtris * 9 + 5] = gametiley * 20;
			tris[numtris * 9 + 6] = gametilex * 20;
			tris[numtris * 9 + 7] = 0;
			tris[numtris * 9 + 8] = gametiley * 20;

			triflags[numtris] = passflags;
			triinds[numtris * 3 + 0] = numtris * 3 + 0;
			triinds[numtris * 3 + 1] = numtris * 3 + 1;
			triinds[numtris * 3 + 2] = numtris * 3 + 2;

			++numtris;

			tris[numtris * 9] = gametilex * 20 + 20;
			tris[numtris * 9 + 1] = 0;
			tris[numtris * 9 + 2] = gametiley * 20 + 20;
			tris[numtris * 9 + 3] = gametilex * 20;
			tris[numtris * 9 + 4] = 0;
			tris[numtris * 9 + 5] = gametiley * 20;
			tris[numtris * 9 + 6] = gametilex * 20;
			tris[numtris * 9 + 7] = 0;
			tris[numtris * 9 + 8] = gametiley * 20 + 20;

			triflags[numtris] = passflags;
			triinds[numtris * 3 + 0] = numtris * 3 + 0;
			triinds[numtris * 3 + 1] = numtris * 3 + 1;
			triinds[numtris * 3 + 2] = numtris * 3 + 2;

			++numtris;
		}
	}

	ASSERT(numtris < maxtrisperchunk);

	rcRasterizeTriangles(&ctx, tris, numtris * 3, triinds, triflags, numtris, *hf, walkableclimb);

	rcCompactHeightfield* chf = rcAllocCompactHeightfield();

	if (!rcBuildCompactHeightfield(&ctx, walkableheight, walkableclimb, *hf, *chf))
		sLog.Error("MESHBUILD", "error: rcBuildCompactHeightfield");

	if (!rcErodeWalkableArea(&ctx, 5, *chf))
		sLog.Error("MESHBUILD", "error: rcErodeWalkableArea");
	if (!rcBuildDistanceField(&ctx, *chf))
		sLog.Error("MESHBUILD", "error: rcBuildDistanceField");
	if (!rcBuildRegions(&ctx, *chf, bordersize, 1, 200))
		sLog.Error("MESHBUILD", "error: rcBuildRegions");

	//contours
	rcContourSet* cs = rcAllocContourSet();

	if (!rcBuildContours(&ctx, *chf, 3, 20, *cs))
		sLog.Error("MESHBUILD", "error: rcBuildContours");

	rcPolyMesh* pm = rcAllocPolyMesh();
	rcPolyMeshDetail* dm = rcAllocPolyMeshDetail();

	if (!rcBuildPolyMesh(&ctx, *cs, 6, *pm))
		sLog.Error("MESHBUILD", "error: rcBuildPolyMesh");
	if (!rcBuildPolyMeshDetail(&ctx, *pm, *chf, 4, 64, *dm))
		sLog.Error("MESHBUILD", "error: rcBuildPolyMeshDetail");

	ASSERT(pm->nverts < 0xFFFF);

	for (int i = 0; i < pm->npolys; ++i)
	{
		if (pm->areas[i] != RC_NULL_AREA)
			pm->flags[i] = pm->areas[i];
		else
			pm->flags[i] = 0;
	}

	dtNavMeshCreateParams params;
	memset(&params, 0, sizeof(params));

	params.verts = pm->verts;
	params.vertCount = pm->nverts;
	params.polys = pm->polys;
	params.polyAreas = pm->areas;
	params.polyFlags = pm->flags;
	params.polyCount = pm->npolys;
	params.nvp = pm->nvp;
	params.detailMeshes = dm->meshes;
	params.detailVerts = dm->verts;
	params.detailVertsCount = dm->nverts;
	params.detailTris = dm->tris;
	params.detailTriCount = dm->ntris;
	params.walkableHeight = walkableheight;
	params.walkableRadius = 2;
	params.walkableClimb = walkableclimb;
	params.bmin[0] = pm->bmin[0];
	params.bmin[1] = pm->bmin[1];
	params.bmin[2] = pm->bmin[2];
	params.bmax[0] = pm->bmax[0];
	params.bmax[1] = pm->bmax[1];
	params.bmax[2] = pm->bmax[2];
	params.cs = rcwidth;
	params.ch = rcheight;
	params.buildBvTree = true;
	params.tileX = tx;
	params.tileY = ty;
	params.tileLayer = 0;

	uint8* data = NULL;
	int datasize = 0;

	if (!dtCreateNavMeshData(&params, &data, &datasize))
		sLog.Error("MESHBUILD", "error: dtCreateNavMeshData");

	dtTileRef tref = 0;

	if (data != NULL)
	{
		if (flags & MESH_BUILD_SAVE_TO_DISK)
		{
			FILE* ftile = fopen(filename, "wb");

			if (ftile != NULL)
			{
				fwrite(data, datasize, 1, ftile);
				fclose(ftile);
			}
		}
		AddEvent(&MapMgr::OnTileBuildComplete, tx, ty, data, datasize, EVENT_MAPMGR_PATHPLANESCALCULATED, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
	else
	{
		FILE* ftile = fopen(filename, "wb");
		fclose(ftile);
	}

	rcFreePolyMesh(pm);
	rcFreePolyMeshDetail(dm);
	rcFreeHeightField(hf);
	//now we can free chf and cs
	rcFreeCompactHeightfield(chf);
	rcFreeContourSet(cs);

	delete[] triflags;
	delete[] tris;
	delete[] triinds;
}

void MapMgr::OnTileBuildComplete( int32 x, int32 y, uint8* data, int datasize )
{
	GETFGUARD(m_navLoaderLock);
	//this is an event given to the mapmgr when a tile has been rebuilt for one reason or another, such as map changing
	//building is done in another thread, this is just notice to the map manager that data is old and needs reloaded
	if (IsLoaded(x, y))
	{
		RemoveMesh(x, y);
		m_navMesh->addTile(data, datasize, DT_TILE_FREE_DATA, 0, NULL);

		//all npcs in this cell must now have their movement interrupted so they dont go somewhere invalid
		//this may appear bad but there is no validity checks in npc movement code
		//for performance reasons
		//TODO: what if the NPC isn't on this cell, but one of it's current splines are?
		MapCell* cell = GetCell(x, y);

		for (std::set<Object*>::iterator itr = cell->m_objects.begin(); itr != cell->m_objects.end(); ++itr)
		{
			if ((*itr)->IsCreature())
				TO_CREATURE(*itr)->StopMovement(0);
		}
	}
}

bool PathCalculatorThread::run()
{
	mgr->Init();
	return true;
}

void OnIntersectPathCellData( const G3D::Ray & r, const PathCellData* b, float & dist )
{
	if (b->m_tree.size() > 0)
		b->m_tree.intersectRay(r, OnIntersectPathTileData, dist);
	else if (!b->loaded)
		dist = 0;
}

void OnIntersectPathTileData( const G3D::Ray & r, const PathTileData* b, float & dist )
{
	if ((b->passflags & b->m_mapmgr->m_raypassflag) == 0)
	{
		auto time = r.intersectionTime(b->m_box);

		if (time < dist)
		{
			b->m_mapmgr->m_rayhitTile = const_cast<PathTileData*>(b);

			b->m_mapmgr->m_rayHitPoint = r.origin() + (r.direction() * time);
			dist = time;
		}
	}
	if (b->m_mapmgr->m_raypassflag != 0)
	{
		switch (b->maplayer2)
		{
			case 0x4A:
			case 0xAD:
			case 0xFF:
			case 0x9B:
			case 0x9C:
			case 0x8E:
			case 0x8D:
			case 0x8C:
			case 0x7A: //cron wall
			{
				auto time = r.intersectionTime(b->m_box);

				if (time < dist)
				{
					b->m_mapmgr->m_rayhitTile = const_cast<PathTileData*>(b);

					b->m_mapmgr->m_rayHitPoint = r.origin() + (r.direction() * time);
					dist = time;
				}
			}
		}
		return;
	}

	auto time = r.intersectionTime(b->m_box);

	if (time < dist)
	{
		b->m_mapmgr->m_rayhitTile = const_cast<PathTileData*>(b);

		b->m_mapmgr->m_rayHitPoint = r.origin() + (r.direction() * time);
		dist = time;
	}
}

void OnIntersectTimedObjectData(const G3D::Ray & r, const TimedObjectData* b, float & dist)
{
	if (!b->passable)
	{
		auto time = r.intersectionTime(b->m_bounds);

		if (time < dist)
		{
			b->m_mapmgr->m_rayHitPoint = r.origin() + (r.direction() * time);
			dist = time;
		}
	}
}

