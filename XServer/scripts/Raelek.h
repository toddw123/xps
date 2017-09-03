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

#ifndef __RAELEK_H
#define __RAELEK_H

class RaelekAI : public CreatureAIScript
{
public:
	uint32 m_nextcast;
	RaelekAI() { m_nextcast = 0; }
	void OnCombatStart() { AddEvent(&RaelekAI::Update, EVENT_UNKNOWN, 1000, 0, EVENT_FLAG_NOT_IN_WORLD); }
	void OnCombatStop() { RemoveEvents(EVENT_UNKNOWN); }
	void OnDeath() { RemoveEvents(EVENT_UNKNOWN); }

	void Update();

	static CreatureAIScript* Create() { return new RaelekAI; }
};

#endif
