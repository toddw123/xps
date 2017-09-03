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

void VaeltarAI::SpawnAnimus()
{
	AddEvent(&VaeltarAI::SpawnAnimus, EVENT_UNKNOWN, 60000, 1, EVENT_FLAG_NOT_IN_WORLD);
	for (uint32 i = 0; i < 15; ++i)
	{
		int32 x = VAELTAR_GUARDIAN_X * 20 + 10;
		int32 y = VAELTAR_GUARDIAN_Y * 20 + 10;

		//randomnes
		x += RandomUInt(60) - 30;
		y += RandomUInt(60) - 30;

		m_unit->m_mapmgr->CreateCreature(104, x, y, 60000, true);
	}
}