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

#ifndef __DREADBARAW_H
#define __DREADBARAW_H


class DreadBarawAIScript : public CreatureAIScript
{
public:
	void OnCombatStart()
	{
		AddEvent(&DreadBarawAIScript::SummonDreadFiend, EVENT_UNKNOWN, 7500, 0, EVENT_FLAG_NOT_IN_WORLD);
	}
	void OnCombatStop() { RemoveEvents(EVENT_UNKNOWN); DestroyFiends(); }

	void Update()
	{
	}

	void SummonDreadFiend();
	void DestroyFiends();

	static CreatureAIScript* Create() { return new DreadBarawAIScript; }
};

class BarawVoidZoneAIScript : public CreatureAIScript
{
public:
	float m_randomrot;
	void OnRespawn()
	{
		m_unit->SetUnitFlag(UNIT_FLAG_UNATTACKABLE | UNIT_FLAG_CANNOT_MOVE | UNIT_FLAG_CANNOT_ATTACK);
		m_unit->SendHealthUpdate(); //don't show health bar
		AddEvent(&BarawVoidZoneAIScript::UpdateEvent, EVENT_UNKNOWN, 1000, 0, EVENT_FLAG_NOT_IN_WORLD);
	}

	void CreateChild();

	void UpdateEvent();
	static CreatureAIScript* Create() { return new BarawVoidZoneAIScript; }
};

#endif
