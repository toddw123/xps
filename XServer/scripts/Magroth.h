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

#ifndef __MAGROTH_H
#define __MAGROTH_H

class MagrothMapScript : public MapScript
{
public:
	uint32 m_ownerid;
	uint32 m_ownerlinkid;
	uint16 m_heraldid;
	uint32 m_nextownerchange;
	uint32 m_nextglobalwarning;

	MagrothMapScript();

	void OnPlayerAddToWorld(Player* p);
	void OnPlayerLinkBreak(Player* p);
	void OnPlayerLinkJoin(Player* o, Player* p);

	bool ChangeOwner(Player* newowner);
	void ApplyAllBonuses();
	void ApplyBonuses(Player* p);
	void SetHerald(uint16 id) { m_heraldid = id; }

	void SaveSettings();

	static MapScript* Create() { return new MagrothMapScript; }
};

class MagrothCrystalAIScript : public CreatureAIScript
{
public:
	MagrothCrystalAIScript() {}
	void OnSetUnit() { m_unit->SetUnitFlag(UNIT_FLAG_INVISIBLE | UNIT_FLAG_UNATTACKABLE); }
	void OnActive() { AddEvent(&MagrothCrystalAIScript::UpdateEvent, EVENT_UNKNOWN, 700, 0, EVENT_FLAG_NOT_IN_WORLD); }
	void OnInactive() { RemoveEvents(EVENT_UNKNOWN); }
	void UpdateEvent();

	static CreatureAIScript* Create() { return new MagrothCrystalAIScript; }
};

class MagrothGuardAIScript : public CreatureAIScript
{
public:
	void OnRespawn();
	void OnCombatStart();
	uint32 GetReaction(Unit* other)
	{
		Unit* o = other;
		if (other->IsCreature())
			o = TO_CREATURE(other)->GetOwner();

		if (o == NULL || !o->IsPlayer())
			return REACTION_SCRIPT_NOT_SET_USE_SERVER_DEFAULT; //let server handle this

		MagrothMapScript* script = (MagrothMapScript*)m_unit->m_mapmgr->GetMapScript(1);

		if (script == NULL)
			return REACTION_SCRIPT_NOT_SET_USE_SERVER_DEFAULT;

		if (TO_PLAYER(o)->m_playerId == script->m_ownerid || (TO_PLAYER(o)->GetLinkID() == script->m_ownerlinkid && script->m_ownerlinkid != 0))
			return REACTION_FRIENDLY;
		return REACTION_HOSTILE;
	}

	static CreatureAIScript* Create() { return new MagrothGuardAIScript; }
};

class MagrothHeraldAIScript : public CreatureAIScript
{
public:
	bool m_eventreachedwp;

	MagrothHeraldAIScript() { m_eventreachedwp = false; }

	void OnSetUnit() { m_unit->SetUnitFlag(UNIT_FLAG_UNATTACKABLE); AddEvent(&MagrothHeraldAIScript::StartEvent, EVENT_OBJECT_DELETE, 30000, 1, EVENT_FLAG_NOT_IN_WORLD); }

	void OnActive() { AddEvent(&MagrothHeraldAIScript::Update, EVENT_UNKNOWN, 2000, 0, EVENT_FLAG_NOT_IN_WORLD); }
	void OnInactive() { RemoveEvents(EVENT_UNKNOWN); }
	void OnAddToWorld();
	void Update();

	void StartEvent()
	{
		return;
		AddEvent(&MagrothHeraldAIScript::StartEvent, EVENT_OBJECT_DELETE, 3600000, 1, EVENT_FLAG_NOT_IN_WORLD);
		m_unit->ClearWayPoints();
		m_eventreachedwp = false;

		m_unit->AddWayPointT(755, 2330, 0);
	}

	void OnReachWayPoint()
	{
		if (m_eventreachedwp)
			return;
		m_eventreachedwp = true;
		m_unit->EventChatMsg(0, "The Magroth Tournament shall begin.");
		m_unit->EventChatMsg(0, "Prepare yourselves, warriors, for this will be one of the");
		m_unit->EventChatMsg(0, "toughest challenges you will face.");

		AddEvent(&MagrothHeraldAIScript::StartChallenges, EVENT_OBJECT_DELETE, 12000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void StartChallenges();

	static CreatureAIScript* Create() { return new MagrothHeraldAIScript; }
};

class GeneralThoramAIScript : public MagrothGuardAIScript
{
public:
	void OnSetUnit();
	static CreatureAIScript* Create() { return new GeneralThoramAIScript; }
};

#endif
