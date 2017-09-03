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

#ifndef __JELOC_H
#define __JELOC_H


class JelocAIScript : public CreatureAIScript
{
public:
	bool m_reachedlastpoint;
	uint8 m_waypointtype;
	std::set<uint16> m_damagedunits;
	std::set<uint32> m_killedplayers;
	JelocAIScript() { m_reachedlastpoint = false; m_waypointtype = false; }
	void OnSetUnit();
	void OnReachWayPoint();
	void OnOsmosDeath(Creature* osmos);

	void GiveRewards();
	void OnDeath();
	void OnRespawn() { m_unit->SetSpawnT(1898, 412); }

	void PortAway();
	void MoveToPort()
	{
		m_waypointtype = 2;
		m_reachedlastpoint = false;
		m_unit->ClearWayPoints();
		m_unit->AddWayPointT(1898, 403, 0);
	}

	static CreatureAIScript* Create() { return new JelocAIScript; }
};

#endif
