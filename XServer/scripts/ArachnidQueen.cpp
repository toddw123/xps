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

void ArachnidQueenAI::UpdateEvent()
{
	if (!m_unit->IsAttacking())
		return;

	Object* tar = m_unit->m_mapmgr->GetObject(m_unit->m_POIObject);

	if (tar == NULL || !tar->IsUnit() || !TO_UNIT(tar)->IsAlive())
		return;

	std::vector<Creature*> cres;
	m_unit->m_mapmgr->GetCreaturesById(51, cres);

	if (cres.size() == 0)
		return;

	Creature* cre;

	bool sendmessage = false;

	for (std::vector<Creature*>::iterator itr = cres.begin(); itr != cres.end(); ++itr)
	{
		cre = *itr;

		if (!cre->IsAlive() || !cre->m_active)
			continue;
		cre->Attack(tar);
		sendmessage = true;
	}

	if (sendmessage)
	{
		m_unit->ResetChatMsgTimer();
		m_unit->EventChatMsg(0, "My pretties, attack that one, %s", TO_UNIT(tar)->m_namestr.c_str());
	}
}