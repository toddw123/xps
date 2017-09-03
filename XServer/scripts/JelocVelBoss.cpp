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

void JelocVelBossAIScript::SpawnVoidZone()
{
	Player* plr = m_unit->GetRandomPlayer(false);

	m_unit->SendChatMsg("< Summon Fire Wall >");

	float randomrot = 0;
	if (plr == NULL)
		randomrot = RandomUInt(360);
	else
		randomrot = m_unit->CalcFacingDeg(plr->m_positionX, plr->m_positionY) - 90;
	float angle = randomrot * (PI / 180);

	int32 x, y;

	if (plr != NULL && !m_unit->InLineOfSight(plr))
	{
		x = plr->m_positionX;
		y = plr->m_positionY;
	}
	else
	{
		x = m_unit->m_positionX - sinf(angle) * 20;
		y = m_unit->m_positionY + cosf(angle) * 20;
	}

	Creature* cre = m_unit->m_mapmgr->CreateCreature(76, x, y, 10000);
	VoidZoneAIScript* crescript = (VoidZoneAIScript*)cre->m_script;
	if (crescript == NULL)
		return;
	crescript->m_randomrot = randomrot;
	cre->m_faction = m_unit->m_faction;

	//kill all players who have damaged us, but are unreachable (or friendly)

	for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
	{
		if (!itr->first->IsPlayer())
			continue;
		if (itr->first->m_faction == m_unit->m_faction)
		{
			if (m_unit->m_damagedunits.find(itr->first->m_serverId) == m_unit->m_damagedunits.end())
				continue;
			m_unit->AddEvent(&Unit::EventDealDamage, (uint32)itr->first->m_serverId, 50000, EVENT_UNIT_DEALDAMAGE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
		}

		int32 stx = m_unit->m_positionX / 20;
		int32 sty = m_unit->m_positionY / 20;
		int32 etx = itr->first->m_positionX / 20;
		int32 ety = itr->first->m_positionY / 20;
	}
}

void JelocVelBossAIScript::SpawnVoidCircle()
{
	m_unit->EventChatMsg(0, "MUHAHAHAHAHAHA!");
	AddEvent(&JelocVelBossAIScript::SpawnVoidZone, EVENT_UNKNOWN, 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&JelocVelBossAIScript::SpawnVoidZone, EVENT_UNKNOWN, 2000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&JelocVelBossAIScript::SpawnVoidZone, EVENT_UNKNOWN, 3000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&JelocVelBossAIScript::SpawnVoidZone, EVENT_UNKNOWN, 4000, 1, EVENT_FLAG_NOT_IN_WORLD);
	AddEvent(&JelocVelBossAIScript::SpawnVoidZone, EVENT_UNKNOWN, 5000, 1, EVENT_FLAG_NOT_IN_WORLD);
}

void JelocVelBossAIScript::EventSpawnVoidCirclePart( uint32 radius )
{
	for (uint32 rot = 0; rot < 360; rot += 20)
	{
		float angle = rot * (PI / 180);
		int32 x = m_unit->m_positionX - sinf(rot) * radius;
		int32 y = m_unit->m_positionY + cosf(rot) * radius;

		Creature* cre = m_unit->m_mapmgr->CreateCreature(76, x, y, 5000);
		cre->m_faction = m_unit->m_faction;
	}
}

void VoidZoneAIScript::UpdateEvent()
{
	m_unit->SetSpellFlag(AURA_FLAG_MUTATIO_FLAMMA);
	for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
	{
		if (itr->first->m_faction != m_unit->m_faction && itr->first->IsPlayer() && m_unit->GetDistance(itr->first) < m_unit->m_bounding)
			m_unit->AddEvent(&Unit::EventDealDamage, uint32(itr->first->m_serverId), 1500, EVENT_UNIT_DEALDAMAGE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void VoidZoneAIScript::CreateChild()
{
	float angle = m_randomrot * (PI / 180);
	int32 x = m_unit->m_positionX - sinf(angle) * 20;
	int32 y = m_unit->m_positionY + cosf(angle) * 20;

	if (!m_unit->m_mapmgr->CanPass(x / 20, y / 20))
		return;
	Creature* cre = m_unit->m_mapmgr->CreateCreature(76, x, y, 10000);
	VoidZoneAIScript* crescript = (VoidZoneAIScript*)cre->m_script;
	if (crescript == NULL)
		return;
	crescript->m_randomrot = m_randomrot;
	cre->m_faction = m_unit->m_faction;
}