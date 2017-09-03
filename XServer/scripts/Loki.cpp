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

void LokiAI::Shout()
{
	AddEvent(&LokiAI::Shout, EVENT_UNKNOWN, RandomUInt(20000) + 10000, 1, EVENT_FLAG_NOT_IN_WORLD);
	Unit* unittarget = NULL;
	if (m_unit->m_POIObject == 0)
		return;
	Object* folobj = m_unit->m_mapmgr->GetObject(m_unit->m_POIObject);
	if (folobj == NULL || !folobj->IsUnit())
		return;
	unittarget = TO_UNIT(folobj);

	switch (RandomUInt(2))
	{
	case 0: m_unit->EventChatMsg(0, "LOL nice maxis %s", unittarget->m_namestr.c_str()); break;
	case 1: m_unit->EventChatMsg(0, "crappy level %u", unittarget->GetLevel()); break;
	case 2: m_unit->EventChatMsg(0, "lol beat %s for mage rings", unittarget->m_namestr.c_str()); break;
	}
}

void LokiAI::OnCombatStart()
{
	AddEvent(&LokiAI::Shout, EVENT_UNKNOWN, RandomUInt(20000) + 10000, 1, EVENT_FLAG_NOT_IN_WORLD);
}

void LokiAI::Update()
{
	if (m_unit->IsAttacking() || !m_unit->IsAlive())
		return;
	GlobalSay s;
	s.name = m_unit->m_namestr;

	switch (RandomUInt(2))
	{
	case 0: s.message = "haha you all fail no1 can beat me"; break;
	case 1: s.message = "im in cons, come and get me hahaha"; break;
	case 2: s.message = "ima take your jeloc, then your mage rings"; break;
	}

	sInstanceMgr.HandleGlobalMessage(s);
}