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

#ifndef __BEAMCRYSTAL_H
#define __BEAMCRYSTAL_H


class BeamCrystalAIScript : public CreatureAIScript
{
public:
	void OnSetUnit() { m_unit->SetUnitFlag(UNIT_FLAG_INVISIBLE | UNIT_FLAG_UNATTACKABLE); }
	void OnActive() { AddEvent(&BeamCrystalAIScript::UpdateEvent, EVENT_UNKNOWN, 2000, 0, EVENT_FLAG_NOT_IN_WORLD); }
	void OnInactive() { RemoveEvents(EVENT_UNKNOWN); }
	void UpdateEvent();

	static CreatureAIScript* Create() { return new BeamCrystalAIScript; }
};

#endif
