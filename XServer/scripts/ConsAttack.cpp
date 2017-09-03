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

void QueenRissaAI::Update()
{
	if (m_eventstarted)
		return;

	for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
	{
		if (!itr->first->IsPlayer())
			continue;
		if (itr->first->m_lastSay == "-start")
		{
			if (m_nexteventtime <= g_mstime || m_lastnpc != 0)
				StartEvent();
			else
			{
				uint32 msnextevent = m_nexteventtime - g_mstime;

				char timepostfix[256];
				uint32 delay = sAccountMgr.GenerateTimePostfix(msnextevent, (char*)timepostfix);

				m_unit->ResetChatMsgTimer();
				m_unit->EventChatMsg(0, "<Next attempt in %u %s>", delay, timepostfix);
			}
			return;
		}
	}
}

void QueenRissaAI::StartEvent()
{
	m_eventstarted = true;
	m_unit->EventChatMsg(0, "Soldiers, we must stand strong.");
	m_unit->EventChatMsg(3000, "We cannot let osmos take this castle");

	uint32 unitid = 106;
	if (m_nexteventtime >= g_mstime && m_lastnpc != 0)
		unitid = m_lastnpc;

	Creature* skore = m_unit->m_mapmgr->CreateCreature(unitid, 775 * 20 + 10, 2350 * 20 + 10, 3600000, false);
}

void QueenRissaAI::OnDeath()
{
	Creature* skore = m_unit->m_mapmgr->GetCreatureById(106);

	if (skore == NULL)
		skore = m_unit->m_mapmgr->GetCreatureById(107);
	if (skore == NULL)
		skore = m_unit->m_mapmgr->GetCreatureById(108);
	if (skore == NULL)
		skore = m_unit->m_mapmgr->GetCreatureById(109);
	if (skore == NULL)
		skore = m_unit->m_mapmgr->GetCreatureById(110);
	if (skore == NULL)
		skore = m_unit->m_mapmgr->GetCreatureById(111);

	if (skore == NULL || skore->m_script == NULL)
		return;
	m_lastnpc = skore->m_proto->entry;

	SkoreAI* script = (SkoreAI*)skore->m_script;

	script->OnRissaDeath();
}