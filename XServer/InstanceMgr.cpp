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

initialiseSingleton(InstanceMgr);

MapMgr* InstanceMgr::CreateMap( uint32 mapid )
{
	MapMgr* mgr = new MapMgr(mapid);
	mgr->InitHolder();
	sThreadPool.AddTask(mgr);
	m_maps.insert(std::make_pair(mapid, mgr));
	return mgr;
}

void InstanceMgr::RemovePlayerByAccount( Account* acc )
{
	ConcurrentUnorderedMap<uint32, MapMgr*>::iterator itr = m_maps.begin();

	for (; itr != m_maps.end(); ++itr)
	{
		MapMgr* m = itr.m_itr->second;

		m->AddEvent(&MapMgr::RemovePlayerByAccount, acc, EVENT_MAPMGR_REMOVEPLAYER, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void InstanceMgr::HandleGlobalMessage( GlobalSay & say )
{
	ConcurrentUnorderedMap<uint32, MapMgr*>::iterator itr = m_maps.begin();

	for (; itr != m_maps.end(); ++itr)
	{
		MapMgr* m = itr.m_itr->second;
		m->AddEvent(&MapMgr::HandleGlobalMessage, say, EVENT_MAPMGR_GLOBALMESSAGE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void InstanceMgr::SaveAllPlayers()
{
	ConcurrentUnorderedMap<uint32, MapMgr*>::iterator itr = m_maps.begin();

	for (; itr != m_maps.end(); ++itr)
	{
		MapMgr* m = itr.m_itr->second;
		m->AddEvent(&MapMgr::SaveAllPlayers, EVENT_MAPMGR_SAVEALLPLAYERS, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void InstanceMgr::UpdateMaxItemGUID()
{
	MYSQL_RES* res = sDatabase.Query("select MAX(`itemguid`) from player_items;");

	if (res != NULL)
	{
		MYSQL_ROW row = sDatabase.Fetch(res);
		if (row[0] == NULL)
			m_maxitemguid = 1;
		else
		{
			sscanf(row[0], "%llu", &m_maxitemguid);
			++m_maxitemguid;
		}
		sLog.Debug("ITEMGUID", "Max item guid: %llu", m_maxitemguid);
		sDatabase.Free(res);
	}
}