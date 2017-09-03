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

void JelocAIScript::OnSetUnit()
{
	m_unit->SetUnitFlag(UNIT_FLAG_SHOW_HEALTH_PCT);
	m_unit->EventChatMsg(0, "Quickly, we must strike while we have the chance!");
	m_unit->AddWayPointT(1915, 391, 0);
	m_unit->AddWayPointT(1921, 397, 0);
	m_unit->AddWayPointT(1906, 397, 0);
	m_unit->AddWayPointT(1906, 401, 0);
	m_unit->AddWayPointT(1917, 401, 0);
	m_unit->AddWayPointT(1920, 403, 0);
	m_unit->AddWayPointT(1920, 413, 0);
	m_unit->AddWayPointT(1898, 412, 0);
}

void JelocAIScript::OnReachWayPoint()
{
	if (m_reachedlastpoint)
		return;
	if (m_unit->m_currentwaypoint == 7) //last point
	{
		m_reachedlastpoint = true;
		m_unit->EventChatMsg(0, "Ahead lies Osmos, the King of Drethor.");
		m_unit->EventChatMsg(3000, "Defeat Osmos, and you will be rewarded greatly.");
	}
	
	if (m_waypointtype == 1 && m_unit->m_currentwaypoint == 0)
	{
		m_reachedlastpoint = true;
		m_unit->EventChatMsg(3000, "Congratulations all, this is a glorious day,");
		m_unit->EventChatMsg(3000, "the traitor has been finally been brought to justice.");
		AddEvent(&JelocAIScript::GiveRewards, EVENT_UNKNOWN, 9000, 1, EVENT_FLAG_NOT_IN_WORLD);
		AddEvent(&JelocAIScript::PortAway, EVENT_UNKNOWN, 10000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	if (m_waypointtype == 2 && m_unit->m_currentwaypoint == 0)
	{
		m_unit->CreatePortPop();
		m_unit->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void JelocAIScript::OnOsmosDeath( Creature* osmos )
{
	m_waypointtype = 1;
	m_reachedlastpoint = false;
	m_unit->ClearWayPoints();
	m_unit->AddWayPoint(osmos->m_positionX, osmos->m_positionY, 0);

	//copy the damaged units to jeloc
	m_damagedunits = osmos->m_damagedunits;
}

void JelocAIScript::GiveRewards()
{
	m_unit->m_damagedunits = m_damagedunits;
	m_unit->m_killedplayers = m_killedplayers;
	m_unit->AwardExp(true);
}

void JelocAIScript::PortAway()
{
	int32 tilex = 1898;
	int32 tiley = 403;
	m_unit->CastSpell(237, tilex * 20 + 10, tiley * 20 + 10);
	AddEvent(&JelocAIScript::MoveToPort, EVENT_UNKNOWN, 5000, 1, EVENT_FLAG_NOT_IN_WORLD);
}

void JelocAIScript::OnDeath()
{
	m_unit->m_waypointMap.clear();

	Creature* osmos = m_unit->m_mapmgr->GetCreatureById(39);

	if (osmos == NULL || !osmos->IsAlive())
		return;

	osmos->CastSpell(15009, true);
	if (osmos->HasAura(SPELL_AURA_WHITE_POT))
		osmos->m_auras[SPELL_AURA_WHITE_POT]->m_dispelblock = true;
}