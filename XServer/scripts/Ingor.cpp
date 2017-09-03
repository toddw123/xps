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


void IngorAI::Update()
{
	if (m_nextcast < g_mstime)
	{
		m_nextcast = g_mstime + 5000;

		Player* plr = m_unit->GetRandomPlayer();

		if (plr != NULL)
		{
			AddEvent(&IngorAI::BombUpdate, (uint32)g_mstime, (uint32)5000, (uint32)plr->m_playerId, EVENT_UNKNOWN, 500, 0, EVENT_FLAG_NOT_IN_WORLD);
			m_unit->ResetChatMsgTimer();
			m_unit->EventChatMsg(0, "< Infest >");
		}
	}

	if (m_nextcast2 < g_mstime)
	{
		m_nextcast2 = g_mstime + 7000;
		m_unit->m_mapmgr->CreateCreature(97, m_unit->m_positionX, m_unit->m_positionY, 300000);
		m_unit->SendChatMsg("< Summon Toxic Bug >");
	}
}

void IngorAI::BombUpdate( uint32 starttimems, uint32 lengthms, uint32 playerid )
{
	Player* plr = m_unit->m_mapmgr->GetPlayer(playerid);

	if (plr == NULL)
		return;

	uint32 timeexpired = g_mstime - starttimems;

	if (timeexpired > lengthms)
	{
		//EXPLODE
		plr->SetSpellEffect(16);
		m_unit->DealDamage(plr, 4000, NULL, false);

		//remove current event
		if (t_currentEvent == NULL)
			return; //wtf

		t_currentEvent->Remove();
	}
	else
	{
		plr->ResetChatMsgTimer();
		plr->EventChatMsg(0, "< %u >", (lengthms - timeexpired) / 1000);
	}
}