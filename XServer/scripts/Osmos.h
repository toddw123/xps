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

#ifndef __OSMOS_H
#define __OSMOS_H


class OsmosAIScript : public CreatureAIScript
{
public:
	bool m_eventswitch;
	OsmosAIScript() { m_eventswitch = false; }

	void SearchTargets();
	void OnCombatStop() { OnRespawn(); }
	void OnSetUnit();
	void OnDeath();
	void OnRespawn();
	void StartEvent();

	void AllowCombat();
	void SummonRound(int32 min, int32 max);

	void Disintegrate()
	{
		m_unit->SendChatMsg("<Disintegration>");

		for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
		{
			if (!itr->first->IsPlayer() || !itr->first->IsAlive() || itr->first->HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
				continue;

			m_unit->CastSpell(15040, itr->first, true);
		}
	}

	void Dullsenses()
	{
		m_unit->SendChatMsg("<Dull Senses>");

		for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
		{
			if (!itr->first->IsPlayer() || !itr->first->IsAlive() || itr->first->HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
				continue;

			m_unit->CastSpell(15039, itr->first, true);
		}
	}

	static CreatureAIScript* Create() { return new OsmosAIScript; }
};

#endif
