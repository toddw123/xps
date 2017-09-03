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

MapCell::MapCell(uint16 xb, uint16 yb)
{
	x = xb;
	y = yb;
	m_players = 0;
	active = false;
}

void MapCell::IncPlayerRefs()
{
	if ((++m_players) == 1)
		Activate();
}

void MapCell::DecPlayerRefs()
{
	if ((--m_players) == 0)
		Deactivate();
}

void MapCell::Activate()
{
	active = true;
	m_mapmgr->LoadMap(x, y);

	for (std::set<Object*>::iterator itr=m_objects.begin(); itr!=m_objects.end(); ++itr)
	{
		Object* o = (*itr);
		if (!o->m_active)
			m_mapmgr->AddActiveObject(o);
	}
}

void MapCell::Deactivate()
{
	active = false;

	m_mapmgr->UnloadMap(x, y);


	for (std::set<Object*>::iterator itr=m_objects.begin(); itr!=m_objects.end(); ++itr)
	{
		Object* o = (*itr);
		if (!o->IsPlayer()) //TODO: Add Activity Makers (objects that keep areas active)
			m_mapmgr->RemoveActiveObject(o);
	}
}
