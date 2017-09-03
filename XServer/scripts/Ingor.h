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

#ifndef __INGOR_H
#define __INGOR_H
 
class IngorAI : public CreatureAIScript
{
public:
	uint32 m_nextcast;
	uint32 m_nextcast2;
	IngorAI() { m_nextcast = 0; m_nextcast2 = 0; }
	void OnCombatStart()
	{
		AddEvent(&IngorAI::Update, EVENT_UNKNOWN, 500, 0, EVENT_FLAG_NOT_IN_WORLD);
	}
	void OnCombatStop() { RemoveEvents(EVENT_UNKNOWN); }
	void OnDeath() { RemoveEvents(EVENT_UNKNOWN); }

	void OnSetUnit() { m_unit->SetUnitFlag(UNIT_FLAG_CANNOT_MOVE); }

	void Update();

	void BombUpdate(uint32 starttimems, uint32 lengthms, uint32 playerid);

	static CreatureAIScript* Create() { return new IngorAI; }
};

class ToxicBugAI : public CreatureAIScript
{
public:
	void OnRespawn()
	{
		m_unit->CastSpell(19, m_unit, true);

		if (m_unit->HasAura(SPELL_AURA_POISON))
			m_unit->m_auras[SPELL_AURA_POISON]->RemoveEvents(EVENT_AURA_REMOVE);
		AddEvent(&ToxicBugAI::Update, EVENT_UNKNOWN, 500, 0, EVENT_FLAG_NOT_IN_WORLD);
	}
	void OnDeath() { RemoveEvents(); }


	void Update()
	{
		//m_unit->SendChatMsg("lol");
		if (!m_unit->HasAura(SPELL_AURA_POISON))
			m_unit->SetHealth(0);
	}

	static CreatureAIScript* Create() { return new ToxicBugAI; }
};

#endif
