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
#include "Magroth.h"

bool MagrothMapScript::ChangeOwner( Player* newowner )
{
	if (m_nextownerchange >= g_mstime)
		return false;
	m_nextownerchange = g_mstime + 300000;

	newowner->GivePVPExp(100);

	ApplyBonuses(newowner);
	
	m_ownerid = newowner->m_playerId;
	m_ownerlinkid = newowner->GetLinkID();
	
	ApplyAllBonuses();
	SaveSettings();
	return true;
}

void MagrothMapScript::ApplyAllBonuses()
{
	if (m_mapmgr == NULL)
		return;

	for each (auto var in m_mapmgr->m_playerStorage)
	{
		auto plr = var.second;
		ApplyBonuses(plr);
	}

	std::vector<Creature*> _cres;
	this->m_mapmgr->GetCreaturesById(18, _cres);
	this->m_mapmgr->GetCreaturesById(19, _cres);
	this->m_mapmgr->GetCreaturesById(73, _cres);

	for (int i = 0; i < _cres.size(); ++i)
	{
		if (!_cres[i]->IsAlive())
			continue;
		_cres[i]->StopAttacking();
	}
}

void MagrothMapScript::ApplyBonuses( Player* p )
{
	if (p->m_playerId == m_ownerid)
	{
		if (p->m_gender == GENDER_FEMALE)
			p->m_unitspecialname = UNIT_NAME_SPECIAL_QUEEN;
		else
			p->m_unitspecialname = UNIT_NAME_SPECIAL_KING;
		p->CastSpell(15032, true);
		p->m_unitspecialname2 = UNIT_NAME_SPECIAL2_MAGROTH;

	}
	else if (m_ownerlinkid != 0 && p->GetLinkID() == m_ownerlinkid)
	{
		p->m_unitspecialname = p->GetClassTitle();
		p->m_unitspecialname2 = UNIT_NAME_SPECIAL2_MAGROTH;

		if (p->HasAura(SPELL_AURA_KING_AURA))
			p->RemoveAura(SPELL_AURA_KING_AURA);
	}
	else
	{
		if (p->HasAura(SPELL_AURA_KING_AURA))
			p->RemoveAura(SPELL_AURA_KING_AURA);
	}
}

void MagrothMapScript::SaveSettings()
{
	g_ServerSettings.UpdateSetting("magroth_owner", m_ownerid);
	g_ServerSettings.UpdateSetting("magroth_owner_link", m_ownerlinkid);
	g_ServerSettings.UpdateSetting("magroth_next_switch", m_nextownerchange);
}

MagrothMapScript::MagrothMapScript() { m_nextglobalwarning = 0; m_ownerid = g_ServerSettings.GetUInt("magroth_owner"); m_heraldid = 0; m_ownerlinkid = g_ServerSettings.GetUInt("magroth_owner_link"); m_nextownerchange = g_ServerSettings.GetUInt("magroth_next_switch"); }

void MagrothMapScript::OnPlayerAddToWorld( Player* p )
{
	ApplyBonuses(p);
}

void MagrothMapScript::OnPlayerLinkBreak(Player * p)
{
	if (p->m_playerId != m_ownerid)
		return;

	m_ownerlinkid = p->GetLinkID();
	SaveSettings();
	ApplyAllBonuses();
}

void MagrothMapScript::OnPlayerLinkJoin(Player * o, Player * p)
{
	if (o->m_playerId != m_ownerid)
		return;
	m_ownerlinkid = o->GetLinkID(); //incase -break and -lead, pick up new ID
	ApplyBonuses(p);
}

void MagrothCrystalAIScript::UpdateEvent()
{
	Creature* guard = m_unit->m_mapmgr->GetCreatureById(73);
	MagrothMapScript* script = (MagrothMapScript*)m_unit->m_mapmgr->GetMapScript(1);

	if (script == NULL)
		return;

	//lets loop inrange and see if anyone it hitting us lol
	for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
	{
		if (!itr->first->IsPlayer())
			continue;
		if (itr->first->HasUnitFlag(UNIT_FLAG_UNATTACKABLE | UNIT_FLAG_INVISIBLE))
			continue;
		if (script->m_ownerlinkid != 0 && TO_PLAYER(itr->first)->GetLinkID() == script->m_ownerlinkid)
			continue;

		if (m_unit->m_mapmgr->InLineOfSight(m_unit->m_positionX, m_unit->m_positionY, itr->first->m_positionX, itr->first->m_positionY, PASS_FLAG_TELEPORT))
			m_unit->CastSpell(271, itr->first, true);

		if (m_unit->GetDistance(itr->first) > m_unit->m_bounding)
			continue;

		if (guard != NULL && guard->IsAlive())
			continue;
		
		if (script != NULL && TO_PLAYER(itr->first)->GetLinkID() != script->m_ownerlinkid)
			script->ChangeOwner(TO_PLAYER(itr->first));
	}
}

void MagrothGuardAIScript::OnRespawn()
{
	
}

void MagrothGuardAIScript::OnCombatStart()
{
	MagrothMapScript* script = (MagrothMapScript*)m_unit->m_mapmgr->GetMapScript(1);

	if (script == NULL)
		return;

	if (script->m_ownerlinkid != 0 && script->m_nextglobalwarning <= g_mstime)
	{
		GlobalSay say;
		say.to_link = script->m_ownerlinkid;
		say.name = m_unit->m_namestr.c_str();
		say.message = "The castle is under attack!";
		sInstanceMgr.HandleGlobalMessage(say);
		script->m_nextglobalwarning = g_mstime + 4000;
	}

	if (script->m_heraldid != 0)
	{
		Object* herald = m_unit->m_mapmgr->GetObject(script->m_heraldid);
		if (herald != NULL && herald->IsUnit())
			TO_UNIT(herald)->SendChatMsg("The castle is under attack!");
	}
}

void MagrothHeraldAIScript::OnAddToWorld()
{
	MagrothMapScript* script = (MagrothMapScript*)m_unit->m_mapmgr->GetMapScript(1);
	if (script == NULL)
		return;
	script->SetHerald(m_unit->m_serverId);
}

void MagrothHeraldAIScript::Update()
{
	MagrothMapScript* script = (MagrothMapScript*)m_unit->m_mapmgr->GetMapScript(1);
	if (script == NULL)
		return;
	for (std::map<Unit*, uint8>::iterator itr = m_unit->m_inrangeUnits.begin(); itr != m_unit->m_inrangeUnits.end(); ++itr)
	{
		if (!itr->first->IsPlayer())
			continue;
		if (TO_PLAYER(itr->first)->m_playerId == script->m_ownerid && itr->first->m_lastSay == "-castle")
			itr->first->AddEvent(&Unit::EventSetPosition, int32(332 * 20 + 10), int32(2006 * 20 + 10), EVENT_UNIT_SETPOSITION, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void MagrothHeraldAIScript::StartChallenges()
{
	m_unit->ClearWayPoints();

	uint32 boss = RandomUInt(1);

	switch (boss)
	{
	case 0: boss = 112; break;
	case 1: boss = 113; break;
	}

	m_unit->EventChatMsg(0, "Your first challenge awaits.");
	m_unit->m_mapmgr->CreateCreature(boss, m_unit->m_positionX, m_unit->m_positionY - 40, 600000, false);
}

void GeneralThoramAIScript::OnSetUnit()
{
	m_unit->m_waypointType = WAYPOINT_TYPE_CIRCLE;
	m_unit->AddWayPointT(332, 2011, 5000);
	m_unit->AddWayPointT(332, 2023, 0);
	m_unit->AddWayPointT(320, 2023, 0);
	m_unit->AddWayPointT(320, 2000, 0);
	m_unit->AddWayPointT(343, 2000, 5000);
}