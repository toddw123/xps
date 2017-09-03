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

#ifndef __LOKI_H
#define __LOKI_H


class LokiAI : public CreatureAIScript
{
public:
	void OnDeath()
	{
		AddEvent(&LokiAI::OnDeathEvent, EVENT_UNKNOWN, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void OnSetUnit() { AddEvent(&LokiAI::Update, EVENT_UNKNOWN, 15000, 0, EVENT_FLAG_NOT_IN_WORLD); }

	void Update();

	void OnDeathEvent()
	{
		//simulate reditus pop
		m_unit->CreatePortPop();
		m_unit->Invis();

		GlobalSay s;
		s.name = m_unit->m_namestr;
		s.message = "LOL noobs ran out of maxys";
		sInstanceMgr.HandleGlobalMessage(s);
		RemoveEvents();
	}

	void OnKilledPlayer(Player* p)
	{
		Item* body = p->GetItem(ITEM_SLOT_BODY);

		if (body == NULL)
			return;
		m_unit->EventChatMsg(0, "beat %s for %s", p->m_namestr.c_str(), body->m_data->name);
	}

	void OnCombatStart();
	void OnCombatStop() { RemoveEvents(); OnSetUnit(); }

	void Shout();

	static CreatureAIScript* Create() { return new LokiAI; }
};

class CaptainAI : public CreatureAIScript
{
public:
	void OnSetUnit()
	{
		m_unit->m_bonuses[CAST_SPEED] = 100;
		m_unit->m_bonuses[BONUS_MAXIMUS_CHANCE] = 97;
	}

	static CreatureAIScript* Create() { return new CaptainAI; }
};

#endif
