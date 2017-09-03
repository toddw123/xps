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

#ifndef __GANAR_H
#define __GANAR_H

class GanarAI : public CreatureAIScript
{
public:
	uint32 m_power;
	bool m_blocklinkeddmg;
	std::set<uint32> m_linkedtargets;
	GanarAI() { m_power = 0; m_blocklinkeddmg = false; }

	void OnDealDamage(int32 damage, SpellEntry* spell);

	void OnCombatStop() { m_linkedtargets.clear(); m_power = 0; }
	void OnDeath() { OnCombatStop(); }

	static CreatureAIScript* Create() { return new GanarAI; }
};
#endif
