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

#ifndef __CONSATTACK_H
#define __CONSATTACK_H


class QueenRissaAI : public CreatureAIScript
{
public:
	bool m_eventstarted;
	uint32 m_nexteventtime;
	uint32 m_lastnpc;
	QueenRissaAI() { m_eventstarted = false; m_nexteventtime = 0; m_lastnpc = 0; }

	void OnRespawn() { m_eventstarted = false; }

	void OnActive() { AddEvent(&QueenRissaAI::Update, EVENT_UNKNOWN, 2000, 0, EVENT_FLAG_NOT_IN_WORLD); }
	void OnInactive() { RemoveEvents(EVENT_UNKNOWN); }

	void StartEvent();

	void Update();

	void OnSkoreDeath()
	{
		m_nexteventtime = g_mstime + (60 * 1000 * 60);
		m_eventstarted = false;
		m_lastnpc = 0;
	}

	void OnDeath();

	static CreatureAIScript* Create() { return new QueenRissaAI; }
};

class SkoreAI : public CreatureAIScript
{
public:
	void OnDeath()
	{
		Creature* rissa = m_unit->m_mapmgr->GetCreatureById(105);

		if (rissa == NULL || rissa->m_script == NULL)
			return;

		QueenRissaAI* script = (QueenRissaAI*)rissa->m_script;

		script->OnSkoreDeath();


		switch (m_unit->m_proto->entry)
		{
		case 106: m_unit->m_mapmgr->CreateCreature(107, 775 * 20 + 10, 2350 * 20 + 10, 3600000, false); break;
		case 107: m_unit->m_mapmgr->CreateCreature(108, 775 * 20 + 10, 2350 * 20 + 10, 3600000, false); break;
		case 108: m_unit->m_mapmgr->CreateCreature(109, 775 * 20 + 10, 2350 * 20 + 10, 3600000, false); break;
		case 109: m_unit->m_mapmgr->CreateCreature(110, 775 * 20 + 10, 2350 * 20 + 10, 3600000, false); break;
		}

	}

	void OnAddToWorld()
	{
		m_unit->AddWayPointT(775, 2375, 0);
		m_unit->AddWayPointT(788, 2375, 0);
		m_unit->AddWayPointT(789, 2361, 0);
		AddEvent(&SkoreAI::SummonGuards, EVENT_UNKNOWN, 500, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void SummonGuards()
	{
		m_unit->CastSpell(15051, true);
	}

	void OnRissaDeath()
	{
		m_unit->SetUnitFlag(UNIT_FLAG_UNATTACKABLE);
		m_unit->SetHealth(m_unit->m_maxhealth);

		m_unit->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 10000, 1, EVENT_FLAG_NOT_IN_WORLD);
		m_unit->EventChatMsg(0, "Muhahaha");
		m_unit->EventChatMsg(3000, "Osmos will finally lay waste to these lands");
	}

	static CreatureAIScript* Create() { return new SkoreAI; }
};

#endif
