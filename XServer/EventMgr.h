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

#ifndef __EVENTMGR_H
#define __EVENTMGR_H

#include "CallBacks.h"
#include "EventHandler.h"

#define sEventMgr EventMgr::getSingleton()

enum EventTypes
{
	EVENT_UNKNOWN,
	EVENT_ACCOUNT_LOGOFF_TIMEOUT,
	EVENT_OBJECT_DELETE,
	EVENT_OBJECT_SETPOSITION,
	EVENT_CREATURE_RESPAWN,
	EVENT_CREATURE_WAYPOINTMOVE,
	EVENT_CREATURE_UPDATEQUEST,
	EVENT_CREATURE_RETRACT_QUEST_OFFER,
	EVENT_CREATURE_INVIS,
	EVENT_CREATURE_ADDWAYPOINT,
	EVENT_SPELLEFFECT_BEAMFIRE,
	EVENT_SPELLEFFECT_OBJECTFIRE,
	EVENT_SPELLEFFECT_DECAY,
	EVENT_AURA_REMOVE,
	EVENT_AURA_EFFECT,
	EVENT_PLAYER_ITEM_LOAD,
	EVENT_PLAYER_RESPAWN,
	EVENT_PLAYER_REMOVEPVPCOMBAT,
	EVENT_PLAYER_REMOVEPVPDIMINISHTOKEN,
	EVENT_UNIT_SETHEALTH,
	EVENT_UNIT_SETPOSITION,
	EVENT_UNIT_CHATSAY,
	EVENT_UNIT_REMOVESPECIALNAME,
	EVENT_UNIT_REMOVESPECIALNAME2,
	EVENT_UNIT_DEALDAMAGE,
	EVENT_UNIT_FORCE_RETURN_WEAPON,
	EVENT_UNIT_UPDATEPETS,
	EVENT_UNIT_CASTSPELL,
	EVENT_PLAYER_SAVE,
	EVENT_ITEM_FORCEDELETE,
	EVENT_TIMEDOBJECT_CLOSE,
	EVENT_TIMEDOBJECT_SWITCHSTATE,
	EVENT_TIMEDOBJECT_STARTSWITCHSTATE,
	EVENT_MAPMGR_REMOVEPLAYER,
	EVENT_MAPMGR_GLOBALMESSAGE,
	EVENT_MAPMGR_SAVEALLPLAYERS,
	EVENT_MAPMGR_PATHPLANESCALCULATED,
	EVENT_MAPMGR_RESETDIFFDATA,
	EVENT_WORLD_LOADPENDINGDATA,
	EVENT_ACCOUNTMGR_REMOVELOCK,
	EVENT_ACCOUNT_CHARLIST_CACHE,
	EVENT_ACCOUNT_CHARLIST_CACHE_DELETE,
	EVENT_ACCOUNT_PLAYER_LOAD,
	EVENT_SEND_HEALTHUPDATE,
	NUM_EVENT_TYPES,
};

class Event;
class EventHolder;
class EventableObject;
class EventMgr : public Singleton<EventMgr>
{
public:

	void AddHolder(EventHolder* h);

	EventHolder* GetHolder(int32 id)
	{
		std::map<int32, EventHolder*>::iterator itr = m_eventholders.find(id);

		if (itr == m_eventholders.end())
			return GetHolder(-1);
		return itr->second;
	}

	void RemoveEvents(EventableObject* obj);
	void RemoveEvents(EventableObject* obj, EventTypes type);

	std::map<int32, EventHolder*> m_eventholders;
};

#endif
