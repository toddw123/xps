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

#ifndef __WORLDQUEST_H
#define __WORLDQUEST_H

enum WorldQuestType
{
	WORLD_QUEST_TYPE_FILL_BAR,
	WORLD_QUEST_TYPE_CONTROLLABLE_POI_FACTION,
};

struct WorldQuestStatus
{
	uint32 questid;
	uint32 type;	
	uint32 current;
	uint32 target;
};

class WorldQuestHandler
{
public:
	std::map<uint32, WorldQuestStatus> m_status;


};

#endif
