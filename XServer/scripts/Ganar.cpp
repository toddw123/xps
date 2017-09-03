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

void GanarAI::OnDealDamage( int32 damage, SpellEntry* spell )
{
	int32 poweradd = damage / 200;

	if (poweradd == 0)
		return;

	m_power += poweradd;

	if (m_power >= 100)
	{
		m_power = 0;

		uint32 numplrs = 0;
		Player** plrs = (Player**)alloca(sizeof(Player*) * m_unit->m_inrangeUnits.size());
		
		for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
		{
			if (!itr->first->IsPlayer() || !itr->first->IsAlive() || itr->first->HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
				continue;
			if (m_linkedtargets.find(TO_PLAYER(itr->first)->m_playerId) != m_linkedtargets.end())
				continue;
			plrs[numplrs++] = TO_PLAYER(itr->first);
		}

		if (numplrs == 0)
		{
			m_unit->SendChatMsg("<Blood Surge>");
			m_unit->SetHealth(m_unit->m_maxhealth);
			m_unit->SetSpellEffect(62);
		}
		else
		{
			Player* chosen = plrs[RandomUInt(numplrs - 1)];
			m_linkedtargets.insert(chosen->m_playerId);
			chosen->SendChatMsg("<Blood Link>");
			m_unit->SendChatMsg("<Blood Link>");
		}
	}
	else
	{
		m_unit->ResetChatMsgTimer();
		m_unit->EventChatMsg(0, "<%u>", m_power);
	}

	//ok now lets do the blood stuff
	if (m_blocklinkeddmg)
		return;
	m_blocklinkeddmg = true;

	for (std::set<uint32>::iterator itr = m_linkedtargets.begin(); itr != m_linkedtargets.end(); ++itr)
	{
		Player* plr = m_unit->m_mapmgr->GetPlayer(*itr);
		if (plr == NULL || !plr->IsAlive())
			continue;
		m_unit->DealDamage(plr, damage / 5, NULL, DAMAGE_FLAG_DO_NOT_APPLY_RESISTS | DAMAGE_FLAG_NO_REFLECT | DAMAGE_FLAG_CANNOT_PROC);
		m_unit->ModHealth(damage * 1.5f);
		plr->SetSpellEffect(62);
	}

	m_blocklinkeddmg = false;
}