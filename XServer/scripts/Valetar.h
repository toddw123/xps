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

#ifndef __VAELTAR_H
#define __VAELTAR_H

#define VAELTAR_GUARDIAN_X 458
#define VAELTAR_GUARDIAN_Y 1259

class VaeltarAI : public CreatureAIScript
{
public:
	void OnCombatStart()
	{
		AddEvent(&VaeltarAI::SpawnGuardian, EVENT_UNKNOWN, 5000, 1, EVENT_FLAG_NOT_IN_WORLD);
		SpawnAnimus();
		AddEvent(&VaeltarAI::Dispelga, EVENT_UNKNOWN, 30000, 0, EVENT_FLAG_NOT_IN_WORLD);
	}

	void SpawnGuardian() 
	{
		Creature* guard = m_unit->m_mapmgr->CreateCreature(94, VAELTAR_GUARDIAN_X * 20 + 10, VAELTAR_GUARDIAN_Y * 20 + 10, 0, false);
		guard->AddWayPointT(460, 1247, 0);
		guard->AddWayPointT(449, 1246, 0);
	}

	void OnGuardianDeath()
	{
		if (m_unit->IsAlive())
			AddEvent(&VaeltarAI::SpawnGuardian, EVENT_UNKNOWN, 15000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void OnCombatStop()
	{
		if (m_unit->m_mapmgr != NULL)
		{
			Creature* oldcreep = m_unit->m_mapmgr->GetCreatureById(94);
			if (oldcreep != NULL)
				oldcreep->Delete();
		}
		RemoveEvents();
	}

	void OnDeath()
	{
		Creature* oldcreep = m_unit->m_mapmgr->GetCreatureById(94);
		if (oldcreep != NULL)
			oldcreep->Delete();
		RemoveEvents();
	}

	void Dispelga()
	{
		m_unit->SendChatMsg("<Dispelga>");
		for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
		{
			if (!itr->first->IsAlive() || !m_unit->IsHostile(itr->first))
				continue;
			itr->first->SetSpellEffect(30);
			itr->first->RemoveAllAuras();
		}
	}

	void SpawnAnimus();

	static CreatureAIScript* Create() { return new VaeltarAI; }
};

class VaeltarGuardianAI : public CreatureAIScript
{
public:
	void OnDeath()
	{
		Creature* vaeltar = m_unit->m_mapmgr->GetCreatureById(93);

		if (vaeltar == NULL || !vaeltar->IsAlive())
			return;

		m_unit->DealDamage(vaeltar, 100000000, NULL, false);

		VaeltarAI* script = (VaeltarAI*)vaeltar->m_script;

		if (script == NULL)
			return;

		script->OnGuardianDeath();
	}

	static CreatureAIScript* Create() { return new VaeltarGuardianAI; }
};

#endif
