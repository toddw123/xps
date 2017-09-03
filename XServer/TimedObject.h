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

#ifndef __TIMEDOBJECT_H
#define __TIMEDOBJECT_H

enum TimedObjectType
{
	TIMEDOBJECT_WALL,
	TIMEDOBJECT_CHEST,
};

enum ObjectState
{
	OBJECT_STATE_OPEN = 0x80,
};

class TimedObject : public Object
{
public:
	uint8 m_state;
	uint8 m_timedtype;
	TimedObjectData* m_losdata;
	bool m_passablestate;
	TimedObject()
	{
		m_losdata = NULL;
		m_typeId = TYPEID_TIMEDOBJECT;
		m_timedtype = TIMEDOBJECT_WALL;
		m_passablestate = true;
	}

	void SetType(uint8 type) { m_timedtype = type; }
	uint8 GetState() { return m_state; }
	void SetState(uint8 newstate) { m_state = newstate; }

	void SwitchStates() { if (IsOpen()) SetClosed(); else SetOpen(); }
	void StartSwitchStates(uint32 timer);

	bool IsOpen()
	{
		if (m_timedtype == TIMEDOBJECT_WALL)
			return (GetState() == 0);
		if (m_timedtype == TIMEDOBJECT_CHEST)
			return (GetState() & 0x80) != 0;
		return true;
	}

	void SetOpen()
	{
		if (m_timedtype == TIMEDOBJECT_WALL)
			OpenWall();
		if (m_timedtype == TIMEDOBJECT_CHEST)
			OpenChest();
		if (m_losdata != NULL)
			m_losdata->passable = true;
		if (m_mapmgr != NULL && m_cell != NULL)
			sMeshBuilder.QueueMeshBuild(m_mapmgr, m_cell->x, m_cell->y, MESH_BUILD_PRIORITY);
	}

	void SetClosed()
	{
		if (m_timedtype == TIMEDOBJECT_WALL)
			CloseWall();
		if (m_timedtype == TIMEDOBJECT_CHEST)
			CloseChest();
		if (m_losdata != NULL)
			m_losdata->passable = false;
		if (m_mapmgr != NULL && m_cell != NULL)
			sMeshBuilder.QueueMeshBuild(m_mapmgr, m_cell->x, m_cell->y, MESH_BUILD_PRIORITY);
	}

	void OpenWall();
	void CloseWall();
	void OpenChest();
	void CloseChest();

	void OnAddToWorld();

	void OnPreRemoveFromWorld();
};

#endif
