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

void ReflectionBossAIScript::OnRespawn()
{
	//drop the walls
	TimedObject* tobj1 = m_unit->m_mapmgr->GetTimedObject(1905, 390);
	TimedObject* tobj2 = m_unit->m_mapmgr->GetTimedObject(1905, 391);
	TimedObject* tobj3 = m_unit->m_mapmgr->GetTimedObject(1905, 392);

	if (tobj1 == NULL || tobj2 == NULL || tobj3 == NULL)
		return;
	tobj1->SetClosed();
	tobj2->SetClosed();
	tobj3->SetClosed();
}

void ReflectionBossAIScript::OnDeath()
{
	RemoveEvents();
	m_unit->m_mapmgr->CreateCreature(116, 1901 * 20 + 10, 391 * 20 + 10, 60000, false);
}

void ReflectionBossAIScript::OnCombatStart()
{
	m_unit->EventChatMsg(0, "You seek to challenge the authority of Osmos?");
	m_unit->EventChatMsg(3000, "All I can offer you...");
	m_unit->EventChatMsg(6000, "... is a quick death.");
}