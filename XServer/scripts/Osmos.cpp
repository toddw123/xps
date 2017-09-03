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

void OsmosAIScript::OnDeath()
{
	if (m_unit->m_mapmgr == NULL)
		return;
	RemoveEvents();

	Creature* jeloc = m_unit->m_mapmgr->GetCreatureById(66);
	if (jeloc == NULL)
		jeloc = m_unit->m_mapmgr->GetCreatureById(71);
	if (jeloc == NULL)
		return;
	JelocAIScript* aiscript = (JelocAIScript*)jeloc->m_script;
	if (aiscript == NULL)
		return;
	aiscript->OnOsmosDeath(m_unit);


	Creature* oresethguards[22];

	int32 spawnx = 1893 * 20 + 10;
	int32 spawny = 405 * 20 + 10;

	for (uint32 i = 0; i < 4; ++i)
	{
		oresethguards[i] = m_unit->m_mapmgr->CreateCreature(34, spawnx, spawny, 3600000, false);
		oresethguards[i]->SetUnitFlag(UNIT_FLAG_UNATTACKABLE);
		if ((spawnx / 20) == 1903)
		{
			spawny += 20;
			spawnx = 1893 * 20 + 10;
		}
		else
			spawnx += 20;
	}

	uint32 i = 0;

	oresethguards[i++]->EventAddWayPointT(0, 1896, 410, 0);
	oresethguards[i++]->EventAddWayPointT(500, 1900, 410, 0);
	oresethguards[i++]->EventAddWayPointT(1000, 1897, 396, 0);
	oresethguards[i++]->EventAddWayPointT(1500, 1899, 396, 0);

	//create commander
	Creature* commander = m_unit->m_mapmgr->CreateCreature(80, 1897 * 20 + 10, 401 * 20 + 10, 3600000, false);
	commander->EventChatMsg(1000, "Guards, to your posts!");
	commander->SetUnitFlag(UNIT_FLAG_UNATTACKABLE);
}

void OsmosAIScript::OnSetUnit()
{
	m_unit->SetUnitFlag(UNIT_FLAG_UNATTACKABLE | UNIT_FLAG_CANNOT_MOVE);
}

void OsmosAIScript::OnRespawn()
{
	m_unit->SetUnitFlag(UNIT_FLAG_UNATTACKABLE);
	m_eventswitch = false;
	RemoveEvents(EVENT_UNKNOWN);
	AddEvent(&OsmosAIScript::SearchTargets, EVENT_UNKNOWN, 5000, 0, EVENT_FLAG_NOT_IN_WORLD);
}

void OsmosAIScript::SummonRound( int32 min, int32 max )
{
	//guardians are 45, warriors 46
	uint32 unit;

	uint32 numunits = min == max? min : RandomUInt(max - min) + min;

	for (uint32 i = 0; i < numunits; ++i)
	{
		switch (RandomUInt(2))
		{
		case 0: unit = 44; break;
		case 1: unit = 45; break;
		case 2: unit = 46; break;
		}

		CreatureProto* prot = sCreatureProto.GetEntry(unit);
		if (prot == NULL)
			continue;

		int32 randomx = RandomUInt(2) - 1;
		int32 randomy = RandomUInt(2) - 1;
		int32 posx = (1889 + randomx) * 20 + 10;
		int32 posy = (391 + randomy) * 20 + 10;
		Creature* cre = m_unit->m_mapmgr->CreateCreature(unit, posx, posy, 75000);
		cre->AddWayPointT(1898 + randomx, 411 + randomy, 0);
		cre->AddWayPointT(1898 + randomx, 403 + randomy, 0);
	}
}

void OsmosAIScript::StartEvent()
{
	if (m_eventswitch)
		return;
	m_eventswitch = true;
	m_unit->SetAnimationType(ANIM_TYPE_STANDING);
	//no need to search for people to start event now
	RemoveEvents(EVENT_UNKNOWN);
	m_unit->EventChatMsg(0, "You dare challenge me?");
	m_unit->EventChatMsg(3000, "Guards, deal with these pests.");
	m_unit->ResetChatMsgTimer();
	m_unit->EventChatMsg(65000, "I suppose I will have to take matters into my own hands.");
	AddEvent(&OsmosAIScript::SummonRound, 1, 2,EVENT_UNKNOWN, 14000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&OsmosAIScript::SummonRound, 2, 2,EVENT_UNKNOWN, 20000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&OsmosAIScript::SummonRound, 3, 3,EVENT_UNKNOWN, 30000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&OsmosAIScript::SummonRound, 3, 3,EVENT_UNKNOWN, 40000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&OsmosAIScript::SummonRound, 3, 3,EVENT_UNKNOWN, 50000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&OsmosAIScript::SummonRound, 3, 3,EVENT_UNKNOWN, 55000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&OsmosAIScript::AllowCombat, EVENT_UNKNOWN, 70000, 1, EVENT_FLAG_NOT_IN_WORLD);


	//move jeloc/vel into aggro range
	Creature* jeloc = m_unit->m_mapmgr->GetCreatureById(66);
	if (jeloc == NULL)
		jeloc = m_unit->m_mapmgr->GetCreatureById(71);
	if (jeloc == NULL)
		return;
	
	jeloc->AddWayPointT(1898, 400, 0);
}

void OsmosAIScript::AllowCombat()
{
	Unit* newtarget = m_unit->FindTarget();
	if (newtarget == NULL)
	{
		OnCombatStop();
		return;
	}
	m_unit->Attack(newtarget);
	m_unit->RemoveUnitFlag(UNIT_FLAG_UNATTACKABLE);
	AddEvent(&OsmosAIScript::SummonRound, 1, 2, EVENT_UNKNOWN, 10000, 0, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&OsmosAIScript::Disintegrate, EVENT_UNKNOWN, 60000, 0, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&OsmosAIScript::Dullsenses, EVENT_UNKNOWN, 30000, 0, EVENT_FLAG_NOT_IN_WORLD);
}

void OsmosAIScript::SearchTargets()
{
	if (m_unit == NULL || m_unit->m_mapmgr == NULL || !m_unit->m_active)
		return;
	Unit* tar = m_unit->FindTarget();
	if (tar != NULL)
		StartEvent();
}
