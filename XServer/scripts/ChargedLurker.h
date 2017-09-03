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

#ifndef __CHARGEDLURKER_H
#define __CHARGEDLURKER_H


class ChargedLurkerAIScript : public CreatureAIScript
{
public:
	void OnSetUnit()
	{
		m_unit->SetUnitFlag(UNIT_FLAG_SHOW_HEALTH_PCT);
		AddEvent(&ChargedLurkerAIScript::ShoutTimer, EVENT_UNKNOWN, 500, 0, EVENT_FLAG_NOT_IN_WORLD);
		AddEvent(&ChargedLurkerAIScript::Explode, EVENT_OBJECT_DELETE, 60000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void Explode()
	{
		if (!m_unit->IsAlive())
			return;
		m_unit->CastSpell(15023, true);
		m_unit->SetHealth(0);

		Creature* imex = m_unit->m_mapmgr->GetCreatureById(30);

		if (imex != NULL)
			imex->SetHealth(imex->m_maxhealth);
	}

	void ShoutTimer()
	{
		if (!m_unit->IsAlive())
			return;
		Event* ev = GetEvent(EVENT_OBJECT_DELETE);
		if (ev == NULL)
			return;

		if (ev->m_firedelay < g_mstime)
		{
			ev->DecRef();
			return;
		}

		m_unit->ResetChatMsgTimer();
		m_unit->EventChatMsg(0, "< %u >", (ev->m_firedelay - g_mstime) / 1000);

		ev->DecRef();
	}

	static CreatureAIScript* Create() { return new ChargedLurkerAIScript; }
};

#endif
