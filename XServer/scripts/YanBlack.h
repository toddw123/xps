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

#ifndef __YANBLACK_H
#define __YANBLACK_H


class YanBlackAIScript : public CreatureAIScript
{
public:
	uint32 m_overpowercounter;
	YanBlackAIScript() { m_overpowercounter = 0; }

	void OnSetUnit() { m_unit->SetUnitFlag(UNIT_FLAG_CANNOT_RESTORE_OR_REVIVE); }

	void OnAddToWorld()
	{
		if (m_unit->m_invis)
			AddEvent(&YanBlackAIScript::OnDeath, EVENT_UNKNOWN, 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void OnCombatStart();
	void OnCombatStop();
	void OnDeath();
	void OnRespawn();

	void DoSpin();

	void BeginWhirlwind()
	{
		AddEvent(&YanBlackAIScript::DoSpin, EVENT_UNKNOWN, 300, 1, EVENT_FLAG_NOT_IN_WORLD);
		m_unit->SetUnitFlag(UNIT_FLAG_EXPLICIT_ANIMATIONS);
	}

	void RandomMove();

	void DoSpinEvent();
	void Teleport();
	void EndWhirlwind();

	void ReInforce()
	{
		m_unit->m_bonuses[BONUS_DAMAGE_PCT] += 15;
		m_unit->m_bonuses[ATTACK_SPEED] += 15;
		++m_overpowercounter;

		m_unit->ResetChatMsgTimer();
		m_unit->EventChatMsg(0, "<Overpower %u>", m_overpowercounter);
	}

	void ResetOverpower()
	{
		m_unit->m_bonuses[BONUS_DAMAGE_PCT] -= (m_overpowercounter * 15);
		m_unit->m_bonuses[ATTACK_SPEED] -= (m_overpowercounter * 15);
		m_overpowercounter = 0;
	}

	void Update();
	void RemoveUnattackable();

	static CreatureAIScript* Create() { return new YanBlackAIScript; }
};

class SorcererAI : public CreatureAIScript
{
public:
	static CreatureAIScript* Create() { return new SorcererAI; }

	void OnAddToWorld()
	{
		m_unit->EventChatMsg(0, "Watch, mortals, our spell to break their defenses is ready.");
		m_unit->EventChatMsg(0, "Prepare yourself, horrors lie beyond those walls.");

		//Creature* fireballs[3];
		//uint32 i = 0;
		//fireballs[i++] = m_unit->m_mapmgr->CreateCreature(118, 1904 * 20 + 10, 390 * 20 + 10, 30000, false);
		//fireballs[i++] = m_unit->m_mapmgr->CreateCreature(118, 1904 * 20 + 10, 391 * 20 + 10, 30000, false);
		//fireballs[i++] = m_unit->m_mapmgr->CreateCreature(118, 1904 * 20 + 10, 392 * 20 + 10, 30000, false);
		m_unit->m_mapmgr->CreateCreature(120, 1904 * 20 + 15, 391 * 20 + 10, 50000, false);

		m_unit->m_rotation = m_unit->CalcFacingDeg(1905 * 20 + 10, 391 * 20 + 10);
		m_unit->SetUpdateBlock(UPDATETYPE_MOVEMENT);
		m_unit->SetAnimation(12, 0);


		AddEvent(&SorcererAI::SummonFireEmitter, EVENT_UNKNOWN, 500, 55, EVENT_FLAG_NOT_IN_WORLD);
		AddEvent(&SorcererAI::BlowupWalls, EVENT_UNKNOWN, 30000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void SummonFireEmitter();

	void BlowupWalls()
	{
		Creature* yan = m_unit->m_mapmgr->GetCreatureById(77);

		if (yan == NULL)
			return;

		Event* ev = yan->GetEvent(EVENT_CREATURE_RESPAWN);

		if (ev == NULL)
			return;

		if (ev->m_firedelay <= g_mstime)
		{
			ev->DecRef();
			return;
		}

		uint32 timetorespawn = ev->m_firedelay - g_mstime;
		ev->DecRef();
		uint32 basex = 1905, basey = 391;

		//drop the walls
		TimedObject* tobj1 = m_unit->m_mapmgr->GetTimedObject(1905, 390);
		TimedObject* tobj2 = m_unit->m_mapmgr->GetTimedObject(1905, 391);
		TimedObject* tobj3 = m_unit->m_mapmgr->GetTimedObject(1905, 392);

		if (tobj1 == NULL || tobj2 == NULL || tobj3 == NULL)
			return;
		tobj1->SetOpen();
		tobj2->SetOpen();
		tobj3->SetOpen();

		uint32 i = 0;
	}
};

#endif
