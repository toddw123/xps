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

#ifndef __INSTANCEMGR_H
#define __INSTANCEMGR_H

class MapMgr;
struct GlobalSay;
class InstanceMgr : public Singleton<InstanceMgr>, public EventableObject
{
public:
	ConcurrentUnorderedMap<uint32, MapMgr*> m_maps;
	uint64 m_maxitemguid;
	FastMutex m_maxitemguidlock;

	MapMgr* GetMap(uint32 mapid)
	{
		ConcurrentUnorderedMap<uint32, MapMgr*>::iterator itr = m_maps.find(mapid);

		if (itr != m_maps.end())
			return itr.m_itr->second;
		return NULL;
	}

	MapMgr* CreateMap(uint32 mapid);

	void RemovePlayerByAccount(Account* acc);

	void HandleGlobalMessage(GlobalSay & say);

	void SaveAllPlayers();

	void UpdateMaxItemGUID();

	uint64 GenerateItemGUID()
	{
		FastGuard g(m_maxitemguidlock);
		++m_maxitemguid;
		return m_maxitemguid;
	}
};

#define sInstanceMgr InstanceMgr::getSingleton()

#endif