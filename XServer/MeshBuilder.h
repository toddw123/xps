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

#ifndef __MESHBUILDER_H
#define __MESHBUILDER_H

enum MeshBuildFlags
{
	MESH_BUILD_SAVE_TO_DISK			= 0x01,
	MESH_BUILD_PRIORITY				= 0x02,
};

class MapMgr;
class MeshBuilderThread;
class PendingBuild
{
public:
	MapMgr* m_mgr; //we hold a reference!
	uint32 m_flags;
	int32 m_x;
	int32 m_y;
	uint64 m_key;
	bool m_isvalid; //if isvalid == false, we have been superceded

	PendingBuild(MapMgr* mgr, uint64 m_key, int32 x, int32 y, uint32 flags);
	~PendingBuild();
};

class MeshBuilder : public Singleton<MeshBuilder>
{
public:
	LockedDeque<PendingBuild> m_pendingqueue;
	std::map<uint64, PendingBuild*> m_pendingbuilds;
	FastMutex m_lock;

	PendingBuild* GetNextPending()
	{
		return m_pendingqueue.pop_front();
	}

	uint64 GenerateKey(int32 mapid, int32 x, int32 y) { return (((uint64)mapid << 60) | ((uint64)x << 32) | y); }

	void QueueMeshBuild(MapMgr* mgr, int32 x, int32 y, uint32 flags);

	void RemovePending(uint64 key, PendingBuild* obj)
	{
		GETFGUARD(m_lock);

		auto itr = m_pendingbuilds.find(key);

		if (itr != m_pendingbuilds.end() && itr->second == obj)
			m_pendingbuilds.erase(itr);
	}

	void StartWorkers(int32 num);
};

#define sMeshBuilder MeshBuilder::getSingleton()

class MeshBuilderThread : public ThreadBase
{
public:
	MeshBuilderThread() {}
	bool run();
};


#endif
