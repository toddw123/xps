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

void TimedObject::OpenWall()
{
	SetState(0);
	//update the passable data
	int16 tilex = m_positionX / 20;
	int16 tiley = m_positionY / 20;
	ASSERT(tilex >= 0 && tiley >= 0 && tilex < 3220 && tiley < 3220);
	if (!m_passablestate)
	{
		m_passablestate = true;
	}
}

void TimedObject::CloseWall()
{
	SetState(1);
	//update the passable data
	int16 tilex = m_positionX / 20;
	int16 tiley = m_positionY / 20;
	ASSERT(tilex >= 0 && tiley >= 0 && tilex < 3220 && tiley < 3220);
	if (m_passablestate)
	{
		m_passablestate = false;
	}
}

void TimedObject::OpenChest()
{
	if (m_mapmgr == NULL)
		return;
	uint8 rotation = m_mapmgr->GetMapLayer3(m_positionX, m_positionY);
	SetState(rotation | 0x80);
}

void TimedObject::CloseChest()
{
	if (m_mapmgr == NULL)
		return;
	uint8 rotation = m_mapmgr->GetMapLayer3(m_positionX, m_positionY);
	SetState(rotation);
}

void TimedObject::OnPreRemoveFromWorld()
{
	m_mapmgr->m_timedobjecttree.remove(m_losdata);
	m_mapmgr->m_timedobjecttree.balance();
	delete m_losdata;
	m_losdata = NULL;
}

void TimedObject::OnAddToWorld()
{
	//this may seem dumb, it's to get the top corner of the bounding box
	int32 tilex = (m_positionX / 20) * 20;
	int32 tiley = (m_positionY / 20) * 20;
	G3D::Vector3 lowbounds(tilex, tiley, -1);
	G3D::Vector3 hibounds(tilex + 20, tiley + 20, 1);
	m_losdata = new TimedObjectData;
	m_losdata->m_bounds.set(lowbounds, hibounds);
	m_losdata->passable = IsOpen();
	m_losdata->m_mapmgr = m_mapmgr;
	m_mapmgr->m_timedobjecttree.insert(m_losdata);
	m_mapmgr->m_timedobjecttree.balance();
}

void TimedObject::StartSwitchStates( uint32 timer )
{
	AddEvent(&TimedObject::SwitchStates, EVENT_TIMEDOBJECT_SWITCHSTATE, timer, 0, EVENT_FLAG_NOT_IN_WORLD);
}