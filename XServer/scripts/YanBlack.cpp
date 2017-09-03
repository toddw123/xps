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

void YanBlackAIScript::OnDeath()
{
	//create jeloc
	int32 x = 1906 * 20 + 10;
	int32 y = 391 * 20 + 10;

	Event* ev = m_unit->GetEvent(EVENT_CREATURE_RESPAWN);

	if (ev == NULL)
		return;

	if (ev->m_firedelay <= g_mstime)
	{
		ev->DecRef();
		return;
	}

	uint32 timetorespawn = ev->m_firedelay - g_mstime;
	ev->DecRef();

	Object* followobj = m_unit->m_mapmgr->GetObject(m_unit->m_POIObject);
	if (followobj == NULL || !followobj->IsUnit() || TO_UNIT(followobj)->m_faction == FACTION_EVIL)
		m_unit->m_mapmgr->CreateCreature(71, x, y, timetorespawn, true); //vel
	else
		m_unit->m_mapmgr->CreateCreature(66, x, y, timetorespawn, true);

	//drop the walls
	TimedObject* tobj1 = m_unit->m_mapmgr->GetTimedObject(1918, 390);
	TimedObject* tobj2 = m_unit->m_mapmgr->GetTimedObject(1918, 391);
	TimedObject* tobj3 = m_unit->m_mapmgr->GetTimedObject(1918, 392);

	if (tobj1 == NULL || tobj2 == NULL || tobj3 == NULL)
		return;
	tobj1->SetOpen();
	tobj2->SetOpen();
	tobj3->SetOpen();


	Creature* oresethguards[6];

	int32 spawnx = (1907 + RandomUInt(2) - 1) * 20 + 10;
	int32 spawny = (391 + RandomUInt(2) - 1) * 20 + 10;

	for (uint32 i = 0; i < 6; ++i)
	{
		oresethguards[i] = m_unit->m_mapmgr->CreateCreature(34, spawnx, spawny, timetorespawn, false);
		oresethguards[i]->SetUnitFlag(UNIT_FLAG_UNATTACKABLE);
	}

	uint32 i = 0;

	oresethguards[i++]->EventAddWayPointT(0, 1907, 390, 0);
	oresethguards[i++]->EventAddWayPointT(0, 1907, 392, 0);
	oresethguards[i++]->EventAddWayPointT(2000, 1917, 389, 0);
	oresethguards[i++]->EventAddWayPointT(2000, 1917, 393, 0);
	oresethguards[i++]->EventAddWayPointT(4000, 1919, 389, 0);
	oresethguards[i++]->EventAddWayPointT(4000, 1919, 393, 0);

	RemoveEvents(EVENT_UNKNOWN);
	ResetOverpower();
}

void YanBlackAIScript::OnRespawn()
{
	//try and find jeloc
	Creature* jeloc = m_unit->m_mapmgr->GetCreatureById(66);
	if (jeloc == NULL)
		jeloc = m_unit->m_mapmgr->GetCreatureById(71);
	if (jeloc != NULL)
		jeloc->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);

	//drop the walls
	TimedObject* tobj1 = m_unit->m_mapmgr->GetTimedObject(1918, 390);
	TimedObject* tobj2 = m_unit->m_mapmgr->GetTimedObject(1918, 391);
	TimedObject* tobj3 = m_unit->m_mapmgr->GetTimedObject(1918, 392);

	if (tobj1 == NULL || tobj2 == NULL || tobj3 == NULL)
		return;
	tobj1->SetClosed();
	tobj2->SetClosed();
	tobj3->SetClosed();

	RemoveEvents(EVENT_UNKNOWN);
	ResetOverpower();
}

void YanBlackAIScript::OnCombatStop()
{
	RemoveEvents(EVENT_UNKNOWN);
	ResetOverpower();
	m_unit->EventChatMsg(0, "Pathetic...");
	m_unit->RemoveUnitFlag(UNIT_FLAG_EXPLICIT_ANIMATIONS);
}

void YanBlackAIScript::OnCombatStart()
{
	m_unit->SendChatMsg("Intruders! We must protect Osmos!");
	AddEvent(&YanBlackAIScript::BeginWhirlwind, EVENT_UNKNOWN, RandomUInt(10000) + 10000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&YanBlackAIScript::ReInforce, EVENT_UNKNOWN, 10000, 0, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&YanBlackAIScript::Update, EVENT_UNKNOWN, 1000, 0, EVENT_FLAG_NOT_IN_WORLD);
}

void YanBlackAIScript::DoSpin()
{
	AddEvent(&YanBlackAIScript::BeginWhirlwind, EVENT_UNKNOWN, RandomUInt(2000) + 10000, 1, EVENT_FLAG_NOT_IN_WORLD);

	m_unit->SendChatMsg("<Whirlwind>");
	m_unit->StopAttack(5200);
	m_unit->m_speed = 3;
	AddEvent(&YanBlackAIScript::DoSpinEvent, EVENT_UNKNOWN, 250, 20, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&YanBlackAIScript::EndWhirlwind, EVENT_UNKNOWN, 5200, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&YanBlackAIScript::RandomMove, EVENT_UNKNOWN, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&YanBlackAIScript::RandomMove, EVENT_UNKNOWN, 1000, 4, EVENT_FLAG_NOT_IN_WORLD);

	//white pot
	m_unit->CastSpell(15009, true);
	if (m_unit->HasAura(SPELL_AURA_WHITE_POT))
		m_unit->m_auras[SPELL_AURA_WHITE_POT]->m_dispelblock = true;
}

void YanBlackAIScript::DoSpinEvent()
{
	m_unit->CastSpell(316, true);
	m_unit->CastSpell(293, true);
	m_unit->SetAnimation(43, 44, true);
}

void YanBlackAIScript::Teleport()
{
	Player* plr = m_unit->GetRandomPlayer();

	if (plr == NULL)
		return;

	m_unit->CastSpell(191, plr, true);
}

void YanBlackAIScript::Update()
{
	if (m_unit->m_spawn != NULL && m_unit->m_mapmgr != NULL && !m_unit->m_mapmgr->IsNoMarkLocation(m_unit->m_positionX, m_unit->m_positionY))
	{
		m_unit->SetUnitFlag(UNIT_FLAG_UNATTACKABLE);
		m_unit->StopAttacking();
		AddEvent(&YanBlackAIScript::RemoveUnattackable, EVENT_CREATURE_RESPAWN, 10000, 1, EVENT_FLAG_NOT_IN_WORLD);
		m_unit->SendChatMsg("Lure me out of Drethor? Fools.");
		m_unit->ResetChatMsgTimer();
		m_unit->EventChatMsg(3000, "If you want to fight me, it will be on my soil.");
	}
}

void YanBlackAIScript::RemoveUnattackable()
{
	m_unit->RemoveUnitFlag(UNIT_FLAG_UNATTACKABLE);
}

void YanBlackAIScript::RandomMove()
{
	uint32 attempts = 0;
	bool found = false;
	int32 newx, newy;

	int32 minx, miny, maxx, maxy;

	if (m_unit->m_spawn == NULL)
	{
		minx = (m_unit->m_positionX / 20) - 10;
		maxx = (m_unit->m_positionX / 20) + 10;
		miny = (m_unit->m_positionY / 20) - 10;
		maxy = (m_unit->m_positionY / 20) + 10;
	}
	else
	{
		minx = 1906;
		miny = 388;
		maxx = 1917;
		maxy = 394;
	}

	while (attempts < 100)
	{
		++attempts;
		int32 x = RandomUInt(minx, maxx);
		int32 y = RandomUInt(miny, maxy);

		newx = x * 20 + 10;
		newy = y * 20 + 10;

		if (m_unit->CreatePath(newx, newy))
		{
			found = true;
			break;
		}
	}

	if (found)
		m_unit->SetPOI(POI_SCRIPT, newx, newy);
}

void YanBlackAIScript::EndWhirlwind()
{
	if (m_unit->HasAura(SPELL_AURA_WHITE_POT))
		m_unit->m_auras[SPELL_AURA_WHITE_POT]->Remove();
	m_unit->SetPOI(POI_ATTACKING); //resuming attack, don't trigger oncombatstart or oncombatstop
	m_unit->StopMovement();
	Unit* tar = m_unit->FindTarget();
	if (tar != NULL)
		m_unit->Attack(tar);
	else
		m_unit->StopAttacking();
	m_unit->RemoveUnitFlag(UNIT_FLAG_EXPLICIT_ANIMATIONS);

	m_unit->m_speed = TO_CREATURE(m_unit)->m_proto->speed;
}

void SorcererAI::SummonFireEmitter()
{
	uint32 posx = RandomUInt(1894, 1903);
	uint32 posy = RandomUInt(388, 394);

	m_unit->m_mapmgr->CreateCreature(119, posx * 20 + 10, posy * 20 + 10, 10000, false);
}