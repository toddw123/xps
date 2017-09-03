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

#ifndef __MAPCELL_H
#define __MAPCELL_H

class Unit;
class Object;
class ItemGroup;
class MapMgr;
class MapCell
{
public:
	MapCell(uint16 x, uint16 y);
	MapMgr* m_mapmgr;
	uint16 x, y;
	std::set<Object*> m_objects;
	std::map<uint32, ItemGroup*> m_items;
	bool active;
	//Amount of players on the cell
	volatile long m_players;


	void DecPlayerRefs();
	void IncPlayerRefs();
	void Activate();
	void Deactivate();
	INLINED void InsertObject(Object* u) { m_objects.insert(u); };
	void RemoveObject(Object* u) { m_objects.erase(u); }
};

#endif
