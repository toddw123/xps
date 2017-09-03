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

initialiseSingleton(MeshBuilder);

PendingBuild::PendingBuild( MapMgr* mgr, uint64 key, int32 x, int32 y, uint32 flags)
{
	mgr->AddRef();
	m_mgr = mgr;
	m_x = x;
	m_y = y;
	m_key = key;
	m_flags = flags;
	m_isvalid = true;
}

PendingBuild::~PendingBuild()
{
	m_mgr->DecRef();
}

bool MeshBuilderThread::run()
{
	while (THREAD_LOOP_CONDS)
	{
		PendingBuild* pending = sMeshBuilder.GetNextPending();

		while (pending != NULL && THREAD_LOOP_CONDS)
		{
			if (pending->m_isvalid)
				pending->m_mgr->BuildMeshTile(pending->m_x, pending->m_y, pending->m_flags);

			sMeshBuilder.RemovePending(pending->m_key, pending);
			delete pending;
			pending = sMeshBuilder.GetNextPending();
		}

		Sleep(1);
	}
	return true;
}

void MeshBuilder::StartWorkers( int32 num )
{
	for (int32 i = 0; i < num; ++i)
		sThreadPool.AddTask(new MeshBuilderThread);
}

void MeshBuilder::QueueMeshBuild( MapMgr* mgr, int32 x, int32 y, uint32 flags )
{
	uint64 key = GenerateKey(mgr->m_mapid, x, y);

	GETFGUARD(m_lock);

	auto itr = m_pendingbuilds.find(key);

	if (itr != m_pendingbuilds.end())
	{
		if (!(flags & MESH_BUILD_PRIORITY) || (itr->second->m_flags & MESH_BUILD_PRIORITY)) //not priority and already queued, or already queued a priority build
		{
			//just update flags and return in this case, we already have everything we need to do the work
			itr->second->m_flags = flags;
			return;
		}
		
		//priority build overtaking a normal build, remove current and make a new entry at front of queue
		itr->second->m_isvalid = false;
		m_pendingbuilds.erase(key);
	}


	PendingBuild* pend = new PendingBuild(mgr, key, x, y, flags);
	m_pendingbuilds.insert(std::make_pair(key, pend));

	if (flags & MESH_BUILD_PRIORITY)
		m_pendingqueue.push_front(pend);
	else
		m_pendingqueue.push_back(pend);
}
