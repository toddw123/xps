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

#ifndef __CARASAEQUARWALLS_H
#define __CARASAEQUARWALLS_H

#include "Shared.h"

class CaraSaWall : public TileScript
{
public:
	void Proc(Object* o)
	{
		if (!o->IsPlayer())
			return;

		if (TO_PLAYER(o)->m_lastSay != "divinus") //eq = "infinitus"
			return;

		DropWallAtPoint(2502, 161);
		DropWallAtPoint(2503, 161);
		DropWallAtPoint(2502, 162);
		DropWallAtPoint(2503, 162);
	}

	void DropWallAtPoint(int32 x, int32 y)
	{
		TimedObject* obj = m_mapmgr->GetTimedObject(x, y);

		if (obj == NULL || obj->IsOpen())
			return;

		obj->SetOpen();
		obj->AddEvent(&TimedObject::SetClosed, EVENT_TIMEDOBJECT_CLOSE, 30000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	uint32 GetProcFlags() { return TILE_PROC_FLAG_CHAT; }
	static TileScript* Create() { return new CaraSaWall; }
};

class EquarWall : public TileScript
{
public:
	void Proc(Object* o)
	{
		if (!o->IsPlayer())
			return;

		if (TO_PLAYER(o)->m_lastSay != "infinitus") //eq = "infinitus"
			return;

		DropWallAtPoint(2572, 161);
		DropWallAtPoint(2573, 161);
		DropWallAtPoint(2572, 162);
		DropWallAtPoint(2573, 162);
	}

	void DropWallAtPoint(int32 x, int32 y)
	{
		TimedObject* obj = m_mapmgr->GetTimedObject(x, y);

		if (obj == NULL || obj->IsOpen())
			return;

		obj->SetOpen();
		obj->AddEvent(&TimedObject::SetClosed, EVENT_TIMEDOBJECT_CLOSE, 30000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	uint32 GetProcFlags() { return TILE_PROC_FLAG_CHAT; }
	static TileScript* Create() { return new EquarWall; }
};

class LokiCaptainSummoner : public TileScript
{
public:

	uint32 m_counter;
	uint32 m_lastsummontime;

	LokiCaptainSummoner()
	{
		m_counter = 0;
		m_lastsummontime = 0;
	}

	void Proc(Object* o)
	{
		if (!o->IsPlayer())
			return;

		if (m_lastsummontime + (60 * 60) >= g_time)
			return;

		char msg[128];
		sprintf(msg, "%u", m_counter++);

		TO_PLAYER(o)->SendChatMsg(msg);
		TO_PLAYER(o)->CastSpell(15054, m_tilex * 20 + 10, m_tiley * 20 + 10, true);

		if (m_counter >= 100)
		{
			m_counter = 0;
			m_lastsummontime = g_time;

			//summon a random boss!
			std::vector<uint32> possible;
			possible.push_back(20);
			possible.push_back(22);
			possible.push_back(48);
			possible.push_back(75);
			possible.push_back(77);

			uint32 boss = possible[RandomUInt(possible.size() - 1)];

			CreatureProto* p = sCreatureProto.GetEntry(boss);
			if (p == NULL)
				return;
			Creature* cre = new Creature(p, NULL);
			cre->m_positionX = TO_PLAYER(o)->m_positionX;
			cre->m_positionY = TO_PLAYER(o)->m_positionY;
			cre->m_spawnx = TO_PLAYER(o)->m_positionX;
			cre->m_spawny = TO_PLAYER(o)->m_positionY;
			cre->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 3600000, 1, EVENT_FLAG_NOT_IN_WORLD);
			cre->SetLevel(70);
			cre->InitCreatureSpells();
			TO_PLAYER(o)->m_mapmgr->AddObject(cre);
		}
	}


	uint32 GetProcFlags() { return TILE_PROC_BUMP_INTO; }
	static TileScript* Create() { return new LokiCaptainSummoner; }
};


class ManaPool : public TileScript
{
public:
	void Proc(Object* o)
	{
		if (!o->IsPlayer())
			return;

		TO_PLAYER(o)->SetMana(TO_PLAYER(o)->GetMaxMana());
	}


	uint32 GetProcFlags() { return TILE_PROC_BUMP_INTO; }
	static TileScript* Create() { return new ManaPool; }
};

class FierceShrine : public TileScript
{
public:
	void Proc(Object* o)
	{
		if (!o->IsPlayer())
			return;

		auto p = TO_PLAYER(o);

		p->ModCurrency(CURRENCY_FIERCE_USES, 100, 100);
		p->SetSpellEffect(69);
	}


	uint32 GetProcFlags() { return TILE_PROC_BUMP_INTO; }
	static TileScript* Create() { return new FierceShrine; }
};

#endif
