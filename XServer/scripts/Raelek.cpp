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

void RaelekAI::Update()
{
	if (m_unit->m_thrownweapon == 0)
	{
		for (int i = 0; i < 5; ++i)
		{
			Player* plr = m_unit->GetRandomPlayer();

			if (plr != NULL && plr->m_serverId != m_unit->m_POIObject)
				m_unit->CastSpell(15044, plr, true);
		}
	}
}