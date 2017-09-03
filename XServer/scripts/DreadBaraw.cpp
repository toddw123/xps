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

void DreadBarawAIScript::SummonDreadFiend()
{
	uint32 numplrs = 0;
	Player** plrs = (Player**)alloca(sizeof(Player*) * m_unit->m_inrangeUnits.size());
	
	for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
	{
		if (!itr->first->IsPlayer() || !itr->first->IsAlive() || itr->first->HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
			continue;
		plrs[numplrs++] = TO_PLAYER(itr->first);
	}

	if (numplrs > 0)
	{
		Player* chosen = plrs[RandomUInt(numplrs - 1)];

		Creature* cre = m_unit->m_mapmgr->CreateCreature(30002, chosen->m_positionX, chosen->m_positionY, 3600000);
	}
}

void DreadBarawAIScript::DestroyFiends()
{
	if (m_unit->m_mapmgr == NULL)
		return;

	std::vector<Creature*> cres;
	m_unit->m_mapmgr->GetCreaturesById(30002, cres);

	for (auto itr = cres.begin(); itr != cres.end(); ++itr)
	{
		auto cre = *itr;
		cre->AddEvent(&Creature::RemoveCreature, EVENT_UNKNOWN, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void BarawVoidZoneAIScript::UpdateEvent()
{
	m_unit->SetSpellFlag(AURA_FLAG_MUTATIO_NIMBUS);
	for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
	{
		if (itr->first->m_faction != m_unit->m_faction && itr->first->IsPlayer() && m_unit->GetDistance(itr->first) < m_unit->m_bounding)
			m_unit->AddEvent(&Unit::EventDealDamage, uint32(itr->first->m_serverId), 1000, EVENT_UNIT_DEALDAMAGE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}