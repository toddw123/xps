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

Player::Player(uint32 id)
{
	m_PVPExpireTime = 0;
	m_experience = 0;
	m_pvplevel = 1;
	m_pvpexp = 0;
	m_tradetarget = 0;
	for (uint32 i = 0; i < 10; ++i)
		m_tradeitems[i] = 0;
	m_tradeaccept = false;
	m_nochatupdate = false;
	m_currentquest = NULL;
	m_extralog = false;
	m_alignlock = 0;
	m_nextchatmessage = 0;
	m_temppvpexp = 0;
	m_nextpvpexpupdate = 0;
	m_bosskilledid = 0;
	m_mapid = 0;
	m_pendingmaptransfer.newmapid = 0xFFFFFFFF;
	m_pendingmaptransfer.playerportal = false;
	m_pvpdiminish = 1;
	m_oldclass = CLASS_FIGHTER;
	m_leadtimer = 0;
	m_skillflags = 0;
	m_weaponSwitch = 0;
	m_skillpoints = 0;
	m_pvppoints = 0;
	for (uint32 i = 0; i < NUM_SKILLS; ++i)
		m_skills[i] = 0;
	m_transdelay = 0;
	m_globalchatDisabled = false;
	m_cheatfollowobj = 0;
	m_betraypvpcombat = false;
	m_pvpcombatexpire = 0;
	m_pvpattackexpire = 0;
	m_petcontrolid = 0;
	m_pvplevel = 1;
	m_pvpexp = 0;
	m_attackstate = ATTACK_STATE_MONSTERS;
	SetAlign(100);
	m_nextspelltrigger = false;
	m_marksLoaded = false;
	m_itemsLoaded = false;
	m_skillsLoaded = false;
	for (uint32 i = 0; i < NUM_MARKS; ++i)
		memset(&m_marks[i], 0, sizeof(Point));
	m_canResurrect = false;
	m_playerId = id;
	m_nextspellid = 0;
	m_faction = FACTION_GOOD;
	m_charDisplay = 0;
	ItemVisual* vis = sItemVisuals.GetEntry(0);
	if (vis != NULL)
		m_charDisplay = vis->somnixmalevisual;
	m_typeId = TYPEID_PLAYER;
	m_session = new Session(this);
	m_invis = false;

	for (uint32 i = 0; i < NUM_SPELL_GROUPS; ++i)
	{
		m_flatmods[i] = NULL;
		m_pctmods[i] = NULL;
	}
}

Player::~Player()
{
	if (m_account->m_onlinePlayer == m_playerId)
		m_account->m_onlinePlayer = 0;
	delete m_session;
	if (m_cell != NULL)
		m_cell->DecPlayerRefs();

	sAccountMgr.RemovePlayerLock(m_playerId, PLAYER_LOCK_PENDING_SAVE_CALL);

	for (uint32 i = 0; i < NUM_SPELL_GROUPS; ++i)
	{
		if (m_pctmods[i] != NULL)
			delete[] m_pctmods[i];
		if (m_flatmods[i] != NULL)
			delete[] m_flatmods[i];
	}
}

void Player::BuildUpdatePacket()
{
	m_packetseq = !m_packetseq;

	if (m_pendingSay || m_globalSays.size() > 0)
		m_packetseq = false;

	ServerPacket data(SMSG_UPDATE_DATA);
	data << m_positionX << m_positionY;

	if (m_packetseq)
			data << m_spellflags;
	else
	{
		m_buffseq = !m_buffseq;
		if (m_buffseq)
			data << m_buffflags;
		else
			data << uint16(m_skillflags | 0x8000);
	}
	data << uint16(m_color1 | (m_color2 << 5));
	data << uint16(uint8(m_inrangeItemGroups.size() + m_inrangeTimedObjects.size())); //might be related to animation speed? for insta
	uint32 inrangeunitpos = data.WPos();
	uint8 inrangeunits = m_inrangeUnits.size();
	data << uint8(0); //this is filled in later

	data << uint8(0); //unklol 11

	uint8 flags = 0;
	if (m_pendingSay || m_globalSays.size() > 0)
		flags |= 1;
	if (m_packetseq) //sending aura data in spell flags
		flags |= 2;
	if (InPVPMode())
		flags |= 4; //pvp mode
	data << flags;

	data << ConvertRotation(m_rotation) << m_unitAnimation;
	data << m_spelleffect;

	size_t spellstoshow = 50;
	
	data << uint8(std::min(m_inrangeSpellEffects.size(), spellstoshow));
	data << uint8(205); //unklol2 17
	data << uint8(104); //unklol3 18

	uint8 tmp = (uint32)(float(m_health) / m_maxhealth * 255);
	data << tmp;
	tmp = 0;
	if (m_maxmana != 0) //prevent a fpu exception
		tmp = (uint32)(float(m_mana) / m_maxmana * 255);
	data << tmp;

	uint8 xppct = 0;
	if (m_level < MAX_LEVEL)
	{
		uint32 lastlevelxp = g_expByLevel[m_level - 1];
		if (m_level >= 78)
			lastlevelxp = 0;
		uint32 expneeded = g_expByLevel[m_level] - lastlevelxp;
		float exppct = m_experience - lastlevelxp;
		exppct /= expneeded;
		if (exppct > 1)
			exppct = 1;
		if (exppct < 0)
			exppct = 0;
		xppct = (exppct * 255);
	}

	data << xppct;

	for (std::set<TimedObject*>::iterator itr = m_inrangeTimedObjects.begin(); itr != m_inrangeTimedObjects.end(); ++itr)
	{
		TimedObject* tobj = *itr;
		data << uint8(3) << tobj->GetState();
		int16 tilex = tobj->m_positionX / 20;
		int16 tiley = tobj->m_positionY / 20;
		data << tilex << tiley;
	}

	for (std::set<ItemGroup*>::iterator itr = m_inrangeItemGroups.begin(); itr != m_inrangeItemGroups.end(); ++itr)
	{
		ItemGroup* g = *itr;
		data << uint8(2) << uint8(0);
		data << uint16(g->m_cellx) << uint16(g->m_celly);

		for (uint32 itemc = 0; itemc < 3; ++itemc)
		{
			if (itemc < g->m_items.size())
				data << uint16(g->m_items[itemc]->m_data->id);
			else
				data << uint16(0);
		}
	}

	size_t spells_shown = 0;

	for (std::map<SpellEffect*, uint8>::iterator itr=m_inrangeSpellEffects.begin(); itr!=m_inrangeSpellEffects.end(); ++itr)
	{
		++spells_shown;

		if (spells_shown > spellstoshow)
			break;

		SpellEffect* p = itr->first;
		data << p->m_visualId << p->ConvertRotation(p->m_rotation);

		switch (p->m_effectType)
		{
		case SPELL_TYPE_BOMB:
			{
				data << p->m_targetid;
			} break;
		default:
			{
				data << p->m_positionX;
			} break;
		}
		data << p->m_positionY << itr->second;

		if (p->m_spell != 0)
			data << uint8(p->m_spell->spelllevel * 8);
		else
			data << uint8(0x10);

		if (p->m_effectType == SPELL_TYPE_BEAM)
		{
			if (p->m_visualId == FULGAR) //fucking ej
			{
				Object* t = m_mapmgr->GetObject(p->m_targetid);
				if (t == NULL || GetDistance(t) > SIGHT_RANGE * 4) //this is to stop client crash, why fulgur can crash client? because it uses a seperate texture every jump (or 2 textures past maximus and 3 textures past omni)
					data << m_positionX << m_positionY;
				else
					data << t->m_positionX << t->m_positionY;
			}
			else
				data << p->m_targetid << p->m_casterid;
		}
	}

	for (std::map<Unit*, uint8>::iterator itr=m_inrangeUnits.begin(); itr!=m_inrangeUnits.end(); ++itr)
	{
		if (itr->first->HasUnitFlag(UNIT_FLAG_INVISIBLE))
		{
			--inrangeunits;
			continue;
		}

		AppendUpdateData(&data, itr->first, itr->second);
		itr->second = 0;
	}

	//now fill in inrange units
	uint32 oldwpos = data.WPos(inrangeunitpos);
	data << uint8(inrangeunits);
	data.WPos(oldwpos);

	if (m_pendingSay)
	{
		data << m_lastSay;
		m_pendingSay = false;
	}
	else if (m_globalSays.size() > 0)
	{
		GlobalSay & say = m_globalSays.front();
		data << uint8(0xFF);
		data.Write((const uint8*)say.name.c_str(), say.name.size());
		data << uint8(58);
		data << say.message;
		m_globalSays.pop_front();
	}


	Send(&data);
}

void Player::AppendUpdateData(ServerPacket* p, Unit* u, uint8 block)
{
	if (block == 0)
	{
		*p << uint16(u->m_serverId | 0x8000);
		return;
	}

	*p << u->m_serverId;
	//block = 0x3F; //debug
	*p << block;

	/* Update block is a flag, it works like this
	& 0x1: send uint16 m_positionX, uint16 m_positionY, uint8 m_rotation
	& 0x2: send uint8 unk, uint8 unk2, uint8 unk3 (GO UNKS) (these are spell aura visuals)
	& 0x4: send uint16 model, uint16 weapon, uint8 shield, uint8 helmet, uint8 color1, uint8 color
	& 0x8: send uint8 anim
	& 0x10: send uint8 spell visual flags
	& 0x20: send uint8 chatmsg + 1 (nul-byte pad), string chatmsg, if first uint8 is -1, send string name (terminated with 58), string chatmsg (null terminated)
	
	thats all
	*/
	if (block & UPDATETYPE_MOVEMENT)
		*p << u->m_positionX << u->m_positionY << u->ConvertRotation(u->m_rotation);

	if (block & UPDATETYPE_AURAS)
		*p << u->m_spellflags << uint8(0); //when reversing the client ive found ONE use for this uint8, it makes calvs heads glow

	if (block & UPDATETYPE_MODELS)
		*p << u->m_charDisplay << u->m_weaponDisplay << u->m_shieldDisplay << u->m_helmetDisplay << u->m_color1 << u->m_color2;
	
	if (block & UPDATETYPE_ANIMATION)
		*p << u->m_unitAnimation;

	if (block & UPDATETYPE_EFFECTS)
		*p << uint8(u->m_spelleffect); //this is last spell effect visual

	if (block & UPDATETYPE_CHATMSG) //+1 on size (null padding)
		*p << uint8(u->m_lastSay.size() + 1) << u->m_lastSay;
}


void Player::BuildInitialLoginPacket()
{
	ServerPacket datas(SMSG_INITIAL_LOGIN_DATA);
	datas << m_positionX; //0
	datas << uint16(0);
	datas << m_positionY; //4
	datas << uint16(0);
	datas << m_mapid; //8
	datas << m_serverId; //12
	datas << uint16(0); //14 high bits server id dont use
	datas << uint16(0); //16 tutorial flag
	datas << uint16(0); //18 unk3
	datas << m_serverId; //20 if this != previous id, do not login, don't know why this is here, maybe an old hack by that hollow guy
	datas << uint16(0); //22 high bits server id dont use
	datas << float(200); //24 current time
	datas << float(0); //28 rate of time change (per 50 milliseconds)
	datas << uint8(0); //unk7


	Send(&datas);
	//no args to send packet
	SetModels();
}

void Player::UpdatePacketQueue()
{
	for (auto pack = m_account->m_pendingPackets.pop_front(); pack != NULL; pack = m_account->m_pendingPackets.pop_front())
	{
		//handle this
		uint8 opcode;
		pack->readpos = 0;
		*pack >> opcode;

		if (opcode == CMSG_REDIRECT_DATA)
		{
			ServerPacket* spack = new ServerPacket;
			spack->Write(&pack->GetBuffer()[1], pack->GetSize() - 1);
			m_account->m_pendingData.push_back(spack);
		}
		delete pack;
	}


	while (m_account->m_pendingData.size() > 0)
	{
		ServerPacket* p = m_account->m_pendingData.pop_front();

		if (p == NULL)
			continue;

		if (p->writepos < 1)
		{
			delete p;
			continue;
		}

		if (m_extralog)
		{
			LOG_ACTION_U(this, "EXTRALOG", "Packet: opcode %u cryptseed %u", p->GetBuffer()[0], p->GetBuffer()[p->GetSize() - 1]);
			LOG_ACTION_U(this, "PACKET", "Packet: opcode %u cryptseed %u", p->GetBuffer()[0], p->GetBuffer()[p->GetSize() - 1]);
		}

		if (p->GetBuffer()[0] < NUM_MSG_TYPES)
		{
			PacketHandler handler = PacketHandlers[p->GetBuffer()[0]];
			if (handler != NULL)
				(m_session->*handler)(*p);
		}

		delete p;
	}

	if (IsAlive() && m_nextspellid != 0 && m_nextspell <= GetMSTime())
	{
		Spell s(this, m_nextspellid, m_nextspelltargets);
		m_nextspellid = 0;
		s.Cast();
	}

	//follow cheat
	if (m_cheatfollowobj != NULL)
	{
		Object* folobj = m_mapmgr->GetObject(m_cheatfollowobj);
		if (folobj != NULL)
			SetPosition(folobj->m_positionX, folobj->m_positionY);
	}

	if (m_nextpvpexpupdate <= g_mstime)
	{
		m_nextpvpexpupdate = g_mstime + 600000;
		m_temppvpexp /= 2;
	}
}

void Player::Update()
{
	UpdatePacketQueue();

	//update lua target if required
	if (m_currenttarget != 0)
	{
		Object* target = m_mapmgr->GetObject(m_currenttarget);

		if (target == NULL || !target->IsUnit() || !TO_UNIT(target)->IsAlive() || !IsInRange(target) || TO_UNIT(target)->HasAura(SPELL_AURA_INVISIBILITY) || TO_UNIT(target)->HasAura(SPELL_AURA_HIDE))
			ClearTarget();
		else
		{
			Unit* utarget = TO_UNIT(target);

			if (utarget->m_lasttargetupdate > m_currenttargetlastupdate || m_currenttargetlastupdate == 0)
			{
				m_currenttargetlastupdate = g_mstime;
				BuildTargetUpdate(utarget);
			}
		}
	}

	//whats exp for next level
	if (m_level < MAX_LEVEL && m_experience >= g_expByLevel[m_level])
	{
		sLog.Debug("PlayerLevelUp", "%s is now level %u!", m_namestr.c_str(), m_level + 1);
		SendActionResult(PLAYER_ACTION_LEVEL_UP);
		++m_level;
		++m_points;
		++m_skillpoints;

		int32 statbonus = 0;

		if (m_level == 10)
			statbonus = 2;
		if (m_level == 36)
			statbonus = 7;

		if (statbonus > 0)
		{
			m_strength += statbonus;
			m_agility += statbonus;
			m_constitution += statbonus;
			m_intellect += statbonus;
			m_wisdom += statbonus;
		}

		if (m_level >= 70)
			m_experience = 0;

		Recalculate();
		SendAllStats();
		SetHealth(m_maxhealth);
	}

	Unit::Update();
}

void Player::Send( ServerPacket* p )
{
	m_account->WritePacket(p);
}

void Player::Send( Packet* p )
{
	m_account->WritePacket(p);
}

void Player::CalculateHealth()
{
	double corehealth = m_level * 40 + GetBonus(m_constitution) * 30 + m_constitution * 30 + m_strength * 5;
	if (m_level >= 70)
		corehealth *= double(70) / 70;
	else
		corehealth *= double(m_level) / 70;	//get bonus to health

	float bonus = corehealth * float(m_bonuses[HEALTH_BONUS] + GetSkillValue(SKILL_HEALTH)) / 100;
	m_maxhealth = corehealth * g_healthPercentByClass[m_class];
	m_maxhealth += bonus;

	if (m_health > m_maxhealth)
		SetHealth(m_maxhealth);
}

void Player::SendAllStats()
{
	SendExperience();
	SendStats();
	SendItems();
	SendSkills();


// 	data << uint16(0);
// 	data << m_strength << m_agility << m_constitution << m_intellect << m_wisdom;
// 	data << uint16(m_maxhealth) << uint16(m_maxmana);
// 	data << uint32(0);
// 	data << uint16(m_level + m_bonuses[BONUS_ABILITY_LEVEL]) << uint16(m_health) << uint16(m_mana);
// 	data << uint32(0) << uint16(m_points);
// 
// 	data.WriteTo(41);
// 	data << m_gender << m_race << m_class;
// 	data.WriteTo(53);
// 	data << m_alignment;
// 
// 	data.WriteTo(83);
// 	data << m_skillpoints;
// 	data.WriteTo(143);
// 	data << m_pvppoints;
// 
// 	for (uint32 i = 0; i < NUM_SKILLS; ++i)
// 	{
// 		if (GetSkillValue(i) == 0)
// 			continue;
// 		SkillEntry* skill = sSkillStore.GetEntry(i);
// 		if (skill == NULL || skill->serverindex == 0)
// 			continue;
// 		data.WPos(83 + skill->serverindex);
// 		data << uint8(GetSkillValue(i));
// 	}
// 	//write past skills so we cant overwrite now
// 	data.WPos(83 + 81);
// 
// 	//items
// 	data.WriteTo(165);
// 	for (uint32 i = 0; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS; ++i)
// 	{
// 		if (m_items[i] == NULL)
// 		{
// 			data.WriteZero(4);
// 		}
// 		else
// 		{
// 			data << uint16(m_items[i]->m_data->id);
// 			data << uint16(m_items[i]->m_stackcount);
// 		}
// 	}
// 
// 	data.WriteTo(425);
// 	data << m_experience;
// 	data << uint32(0); //money
// 
// 	//825 = pvp exp
// 	data.WriteTo(433);
// 	data << m_pvpexp;
// 	//831 = pvp level
// 	data.WriteTo(439);
// 	data << m_pvplevel;
// 	data << uint16(0); //account tokens
// 
// 	data.WriteTo(445);
// 	//try this
// 	//values are float(100 + bonus) / 100, all hardcoded to 0
// // 	float x = float(100) / 100;
// // 	for (uint32 i=0; i<10; ++i)
// // 		data << x;
// 
// 	//bonuses
// 	data << GetMagicBonus(FIRE_MAGIC_BONUS);
// 	data << GetMagicBonus(ICE_MAGIC_BONUS);
// 	data << GetMagicBonus(ENERGY_MAGIC_BONUS);
// 	data << GetMagicBonus(SHADOW_MAGIC_BONUS);
// 	data << GetMagicBonus(SPIRIT_MAGIC_BONUS);
// 	//resists
// 	data << GetMagicResist(PROTECTION_FIRE);
// 	data << GetMagicResist(PROTECTION_ICE);
// 	data << GetMagicResist(PROTECTION_SHADOW);
// 	data << GetMagicResist(PROTECTION_ENERGY);
// 	data << GetMagicResist(PROTECTION_SPIRIT);
// 
// 	int16 checksum = 0;
// 	for (uint32 i=0; i<485; ++i)
// 		checksum += *(int8*)&data.GetBuffer()[i + 1];
// 	data.WriteTo(485);
// 	data << checksum;
// 
// 	Send(&data);
}

void Player::SaveToDB()
{
	sLog.Debug("Save", "Saving player %s.", m_namestr.c_str());

	bool saveevent = (t_currentEvent != NULL && t_currentEvent->m_eventType == EVENT_PLAYER_SAVE);

	uint32 lockflags = sAccountMgr.GetPlayerLockFlags(m_playerId);
	if (lockflags & PLAYER_LOCK_PENDING_SAVE_CALL && !saveevent)
		return; //someone else will handle this

	if (lockflags & PLAYER_LOCK_PENDING_DB_WRITE)
	{
		//try again later, hold a ref with event so no delete
		AddEvent(&Player::SaveToDB, EVENT_PLAYER_SAVE, 1000, 1, 0);
		sAccountMgr.AddPlayerLock(m_playerId, PLAYER_LOCK_PENDING_SAVE_CALL);
		return;
	}

	sAccountMgr.AddPlayerLock(m_playerId, PLAYER_LOCK_PENDING_DB_WRITE);
	PlayerSaveSQLEvent* sqlev = new PlayerSaveSQLEvent;
	sqlev->playerid = m_playerId;

	sqlev->AddQuery("replace into `players` VALUES (%u, %u, \"%s\", %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %u, %d, %u, %u, %u, %u, %u, %u, %u, %u);",
		m_playerId,
		m_accountId,
		m_namestr.c_str(),
		0,
		m_strength,
		m_agility,
		m_constitution,
		m_intellect,
		m_wisdom,
		m_path,
		m_gender,
		m_race,
		m_class,
		m_oldclass,
		m_level,
		m_experience,
		m_points,
		m_positionX,
		m_positionY,
		m_alignment,
		m_pvplevel,
		m_pvpexp,
		m_mapid,
		m_bosskilledid,
		m_color1,
		m_color2,
		m_alignlock,
		m_temppvpexp);

	if (m_marksLoaded)
	{
		//save marks
		sqlev->AddQuery("delete from `player_marks` where `playerid` = %u;", m_playerId);
		for (uint32 i = 0; i < NUM_MARKS; ++i)
		{
			if (m_marks[i].x == 0 && m_marks[i].y == 0)
				continue;
			sqlev->AddQuery("insert into `player_marks` VALUES (%u, %u, %u, %u, %u);", m_playerId, i, m_marks[i].x, m_marks[i].y, m_marks[i].mapid);
		}
	}

	if (m_itemsLoaded)
	{
		sqlev->AddQuery("delete from `player_items` where `playerid` = %u and `itemflags` = 0;", m_playerId);
		for (uint32 i = 0; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS + NUM_STORAGE_ITEMS; ++i)
		{
			if (m_items[i] == NULL)
				continue;
			if (m_items[i]->m_itemguid != 0)
				sqlev->AddQuery("delete from `player_items` where `itemguid` = %llu;", m_items[i]->m_itemguid);
			sqlev->AddQuery("insert into `player_items` VALUES (%llu, %u, %u, %u, %u, %u, %u, %u); ", m_items[i]->m_itemguid, m_playerId, m_items[i]->m_data->id, i, m_items[i]->m_stackcount, m_items[i]->m_expiretime, m_items[i]->m_itemversion, m_items[i]->m_itemflags);
		}
	}

	if (m_skillsLoaded)
	{
		sqlev->AddQuery("delete from `player_skills` where `playerid` = %u;", m_playerId);
		for (uint32 i = 0; i < NUM_SKILLS; ++i)
		{
			if (m_skills[i] == 0)
				continue;
			sqlev->AddQuery("insert into `player_skills` VALUES (%u, %u, %u); ", m_playerId, i, m_skills[i]);
		}
	}

	//save death tokens
	sqlev->AddQuery("delete from `player_deathtokens` where `playerid` = %u;", m_playerId);

	std::vector<Event*> deathtokenevents;
	GetEvent(EVENT_PLAYER_REMOVEPVPDIMINISHTOKEN, deathtokenevents);

	for (std::vector<Event*>::iterator itr = deathtokenevents.begin(); itr != deathtokenevents.end(); ++itr)
	{
		Event* e = *itr;

		if (e->m_firedelay <= g_mstime)
			continue; //no time left

		uint32 timeleft = e->m_firedelay - g_mstime;

		e->DecRef();

		sqlev->AddQuery("insert into `player_deathtokens` VALUES (%u, %u);", m_playerId, timeleft);
	}

	//skill cooldowns
	sqlev->AddQuery("delete from `player_skillcooldowns` where `playerid` = %u;", m_playerId);

	for (std::map<uint32, uint32>::iterator itr = m_skillcooldowns.begin(); itr != m_skillcooldowns.end(); ++itr)
	{
		if (itr->second <= g_mstime)
			continue;
		uint32 timeleft = itr->second - g_mstime;
		sqlev->AddQuery("insert into `player_skillcooldowns` VALUES (%u, %u, %u);", m_playerId, itr->first, timeleft);
	}

	sqlev->AddQuery("delete from `player_completedquests` where `playerid` = %u;", m_playerId);
	for (std::set<uint32>::iterator itr = m_completedquests.begin(); itr != m_completedquests.end(); ++itr)
	{
		sqlev->AddQuery("insert into `player_completedquests` VALUES (%u, %u);", m_playerId, *itr);
	}

	sqlev->AddQuery("delete from `player_currency` where `playerid` = %u;", m_playerId);
	for (auto itr = m_currencies.begin(); itr != m_currencies.end(); ++itr)
	{
		sqlev->AddQuery("insert into `player_currency` VALUES (%u, %u, %d);", m_playerId, itr->first, itr->second);
	}

	sDatabase.Execute(sqlev);

	if (saveevent)
		sAccountMgr.RemovePlayerLock(m_playerId, PLAYER_LOCK_PENDING_SAVE_CALL);
}

uint32 Player::GetMeleeDamage()
{
	float basedmg;
	auto curweap = GetCurrentWeapon();
	//no weapon
	if (curweap == NULL)
		basedmg = 10;
	else
	{
		uint32 maxdmg = curweap->m_data->basevalue;
		basedmg = RandomUInt(maxdmg / 2) + (maxdmg / 2);
	}
	float bonusdmg;

	switch (m_class)
	{
	case CLASS_DIABOLIST:
	case CLASS_CLERIC:
	case CLASS_DRUID:
	case CLASS_WIZARD:
	case CLASS_WARLOCK:
	case CLASS_WITCH:
		{
			bonusdmg = m_strength * 4 + GetBonus(m_strength) * 4;
		} break;
	case CLASS_RANGER:
		{
			if (GetCurrentWeapon() != NULL && GetCurrentWeapon()->m_data->type == ITEM_TYPE_BOW)
				bonusdmg = m_strength * 6 + GetBonus(m_strength) * 12 + m_agility * 2 + GetBonus(m_agility) * 4;
			else
				bonusdmg = m_strength * 8 + GetBonus(m_strength) * 16;
		} break;
	case CLASS_BARBARIAN:
		{
			if (curweap != NULL && (curweap->m_data->type == ITEM_TYPE_2H_WEAPON || curweap->m_data->type == ITEM_TYPE_POLE_ARM))
				bonusdmg = m_strength * 25 + GetBonus(m_strength) * 50;
			else
				bonusdmg = m_strength * 8 + GetBonus(m_strength) * 16;
		}
	default: bonusdmg = m_strength * 8 + GetBonus(m_strength) * 16; break;
	}

	//handle skills whoo
	if (GetCurrentWeapon() != NULL)
	{
		switch (GetCurrentWeapon()->m_data->type2)
		{
		case WEAPON_TYPE_BOW: bonusdmg += GetSkillValue(SKILL_BOWS) + GetSkillValue(SKILL_ARCHERY) * 4 + GetSkillValue(SKILL_ARCHERY_MASTERY); break;
		case WEAPON_TYPE_DAGGER: bonusdmg += GetSkillValue(SKILL_DAGGERS); break;
		case WEAPON_TYPE_POLE_ARM: bonusdmg += GetSkillValue(SKILL_POLE_ARM); break;
		case WEAPON_TYPE_SWORD: bonusdmg += GetSkillValue(SKILL_SWORDS); break;
		case WEAPON_TYPE_BLUNT: bonusdmg += GetSkillValue(SKILL_BLUNT_WEAPONS); break;
		case WEAPON_TYPE_AXE: bonusdmg += GetSkillValue(SKILL_AXES); break;
		case WEAPON_TYPE_BLUNT_2H:
		case WEAPON_TYPE_AXE_2H:
		case WEAPON_TYPE_SWORD_2H: bonusdmg += GetSkillValue(SKILL_2_HANDED_WEAPONS); break;
		}
	}

	if (HasAura(SPELL_AURA_STRENGTH))
	{
		m_auras[SPELL_AURA_STRENGTH]->RemoveCharge();
		bonusdmg += 5 + GetSkillValue(SKILL_MAGNIFY_STR);
	}

	if (HasAura(SPELL_AURA_FOCUSED_ATTACK))
	{	
		bonusdmg += 5 * m_auras[SPELL_AURA_FOCUSED_ATTACK]->m_charges;
		m_auras[SPELL_AURA_FOCUSED_ATTACK]->RemoveCharge();
	}

	if (bonusdmg < 0)
		bonusdmg = 0;
	int32 dmg = basedmg * float(100 + bonusdmg) / 100;
	if (HasAura(SPELL_AURA_ROBUR))
		dmg *= float(100 + m_auras[SPELL_AURA_ROBUR]->m_spell->value) / 100;
	if (HasAura(SPELL_AURA_BATTLE_SCROLL))
		dmg *= float(100 + m_auras[SPELL_AURA_BATTLE_SCROLL]->m_spell->value) / 100;

	if (m_level > 50)
		dmg *= float(m_level) / 50;

	if (m_constitution > m_strength)
	{
		//damage penalty is cons - strength
		dmg *= float(100 - (m_constitution - m_strength)) / 100;
	}

	if (GetCurrentWeapon() != NULL)
	{
		if (m_class != CLASS_RANGER && GetCurrentWeapon()->m_data->type == ITEM_TYPE_BOW)
			dmg *= 0.5f;
	}

	return std::max(dmg, (int32)m_level);
}

uint32 Player::GetBonus( uint32 s )
{
	if (s < 15)
		return 0;
	else if (s < 17)
		return 1;
	else if (s < 19)
		return 2;
	else if (s < 21)
		return 3;
	else if (s < 25)
		return 4;
	else if (s < 29)
		return 5;
	else if (s < 34)
		return 6;
	else if (s < 39)
		return 7;
	else if (s < 45)
		return 8;
	else if (s < 50)
		return 9;
	else if (s < 61)
		return 10;
	else
		return 12 + (int)floor((s - 61) / 10.0);
}

void Player::StopAction()
{
	StopMovement();
	RemoveAura(SPELL_AURA_INVISIBILITY);
	RemoveAura(SPELL_AURA_HIDE);
	RemoveAura(SPELL_AURA_MUTATIO_NIMBUS);
	RemoveAura(SPELL_AURA_MUTATIO_FLAMMA);
	ClearTarget();

	for (std::set<uint16>::iterator itr = m_summonedObjects.begin(); itr != m_summonedObjects.end(); ++itr)
	{
		Object* o = m_mapmgr->GetObject(*itr);
		if (o != NULL)
			o->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void Player::OnItemLoad( MYSQL_RES* ev )
{
	bool saveplayer = false;
	if (ev != NULL)
	{
		m_itemsLoaded = true;
		MYSQL_ROW row = sDatabase.Fetch(ev);
		while (row != NULL)
		{
			uint64 itemguid = 0;
			sscanf(row[0], "%llu", &itemguid);
			uint32 itemid = atol(row[2]);
			uint32 slot = atol(row[3]);
			uint32 props = atol(row[4]);
			uint32 expiretime = atol(row[5]);
			uint32 itemversion = atol(row[6]);
			uint32 itemflags = atol(row[7]);

			if (itemflags & ITEM_FLAG_LIMBO)
				goto ITEMLOADNEXTROW;

			if (expiretime != 0 && expiretime <= g_time)
				goto ITEMLOADNEXTROW;
			ItemProto* itdata = sItemStorage.GetItem(itemid);
			if (itdata == NULL)
				goto ITEMLOADNEXTROW;

			Item* i = new Item(itemid, this);
			i->m_itemguid = itemguid;
			i->m_stackcount = props;
			i->m_itemversion = itemversion;

			if (i->m_itemversion == 8)
			{
				if (!sItemStorage.IsStackable(i->m_data->id))
					i->ApplyProperties();
				i->m_itemversion = 9;
			}


			if (i->m_itemversion == 9)
			{
				if (i->m_data->id == 240)
				{
					i->Delete();
					goto ITEMLOADNEXTROW;
				}
				i->m_itemversion = 10;
			}

			if (i->m_itemversion == 10)
			{
				if (i->m_data->id == 0)
				{
					i->Delete();
					goto ITEMLOADNEXTROW;
				}
				i->m_itemversion = 11;
			}

			if (i->m_itemversion == 11)
			{
				if (!sItemStorage.IsStackable(i->m_data->id))
					i->ApplyProperties();
				i->m_itemversion = 12;
			}


			if (expiretime)
			{
				i->m_expiretime = expiretime;
				expiretime = (expiretime - g_time) * 1000;
				i->AddEvent(&Item::Delete, EVENT_ITEM_FORCEDELETE, expiretime, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
			else
			{
				if (m_account->m_data->accountlevel == 0 && sItemStorage.ItemDisabled(itemid) == ITEM_DISABLED_NORMAL)
				{
					//bugged
					i->Delete();
					goto ITEMLOADNEXTROW;
				}
			}

			if (itemflags & ITEM_FLAG_REINSERT)
			{
				if (!i->AddToInventory())
					i->Delete();
				else
				{
					sLog.Debug("ITEM", "Reinserted %llu to slot %u", i->m_itemguid, i->m_currentslot);
					saveplayer = true;
				}
			}
			else
			{
				if (slot < NUM_EQUIPPED_ITEMS)
				{
					i->Equip(slot);
					if (!i->CheckRequirements())
						i->RemoveBonuses();
				}
				else
					i->AddToInventory(slot);
			}

ITEMLOADNEXTROW:
			row = sDatabase.Fetch(ev);
		}
	}

	if (saveplayer)
		SaveToDB();
}

uint32 Player::GetItemClass()
{
	switch (m_class)
	{
	case CLASS_ADEPT: return ITEM_CLASS_ALL;
	case CLASS_FIGHTER:
	case CLASS_BARBARIAN: return ITEM_CLASS_FIGHTER;
	case CLASS_RANGER: return ITEM_CLASS_RANGER;
	case CLASS_PALADIN:
	case CLASS_DARK_PALADIN: return ITEM_CLASS_PALADIN;
	case CLASS_CLERIC:
	case CLASS_DIABOLIST:
	case CLASS_DRUID: return ITEM_CLASS_CLERIC;
	case CLASS_WIZARD: return ITEM_CLASS_WIZARD;
	case CLASS_WITCH:
	case CLASS_WARLOCK: return ITEM_CLASS_WARLOCK;
	case CLASS_DARKWAR: return ITEM_CLASS_DARKWAR;
	}
	return 0;
}

void Player::SendActionResult( uint16 res )
{
	m_account->SendActionResult(res);
}

void Player::PortToMark( uint8 mark )
{
	if (m_marks[mark].x != 0 && m_marks[mark].y != 0)
		EventTeleport(m_marks[mark].x, m_marks[mark].y, m_marks[mark].mapid);
}

void Player::OnMarksLoad( MYSQL_RES* ev )
{
	if (!m_marksLoaded && ev != NULL)
	{
		m_marksLoaded = true;
		MYSQL_ROW row = sDatabase.Fetch(ev);
		while (row != NULL)
		{
			uint32 r = 1;
			uint32 mark = atol(row[r++]);
			uint32 x = atol(row[r++]);
			uint32 y = atol(row[r++]);
			uint32 mapid = atol(row[r++]);
			m_marks[mark].x = x;
			m_marks[mark].y = y;
			m_marks[mark].mapid = mapid;
			row = sDatabase.Fetch(ev);
		}
	}
}

uint32 Player::GetAttackSpeed()
{
	uint32 basespeed = GetAttackSpeedBase();
	if (GetCurrentWeapon() != NULL && (GetCurrentWeapon()->m_data->type == ITEM_TYPE_2H_WEAPON || GetCurrentWeapon()->m_data->type == ITEM_TYPE_POLE_ARM))
		basespeed *= 1.05f;

	bool hasbow = false;
	if (GetCurrentWeapon() != NULL && GetCurrentWeapon()->m_data->type == ITEM_TYPE_BOW)
		hasbow = true;

	//agility = 0.4% per point, attack speed = 1% per point
	//rage potion = 5% attack speed
	//velo = 1% attack speed per spell base
	uint32 attspeed = m_bonuses[ATTACK_SPEED];
	if (hasbow)
		attspeed += m_bonuses[BONUS_SHOT_SPEED];

	if (HasAura(SPELL_AURA_RAGE_POTION))
	{
		if (m_class == CLASS_DARKWAR)
			attspeed += 5;
		attspeed += 5;
	}

	if (attspeed >= 75)
		attspeed = 75;

	basespeed *= float(100 - attspeed) / 100;

	if (HasAura(SPELL_AURA_VELOCITAS))
		basespeed /= float(100 + m_auras[SPELL_AURA_VELOCITAS]->m_spell->value) / 100;

	basespeed /= float(100 + (m_agility * 2 + GetBonus(m_agility) * 2)) /100;

	if (HasAura(SPELL_AURA_BERSERK))
		basespeed *= 0.8;
	if (HasAura(SPELL_AURA_INTENSITY))
		basespeed *= 0.9;

	return basespeed;
}

void Player::OnAddToWorld()
{
	sAccountMgr.AddOnlineIP(sAccountMgr.GetIP(m_account->netinfo));
	m_session->m_lastmovecounter = 0;
	SetUnitFlag(UNIT_FLAG_NO_PATHFINDING);

	if (m_pendingmaptransfer.playerportal && m_mapmgr->IsNoMarkLocation(m_positionX, m_positionY) && m_account->m_data->accountlevel == 0)
		GoHome();

	if (m_namestr == "NONAME")
	{
		EventChatMsg(3000, "You have been flagged for a rename.");
		EventChatMsg(3000, "Please use -rename newname to change your name, thanks.");
		EventChatMsg(3000, "Note: until you choose a name, you are muted.");
	}

	//mark online for website
	sDatabase.Execute("insert into `player_online` VALUES (%u);", m_playerId);
}

void Player::UnloadItems()
{
	for (uint32 i = 0; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS + NUM_STORAGE_ITEMS; ++i)
	{
		if (m_items[i] == NULL)
			continue;
		if (i < NUM_EQUIPPED_ITEMS)
			m_items[i]->HandleBonuses(false);
		m_items[i]->Delete();
		m_items[i] = NULL;
	}
	m_itemsLoaded = false;
}

void Player::OnRemoveFromWorld()
{
	m_unitspecialname = 0;
	m_unitspecialname2 = 0;
	sAccountMgr.RemoveOnlineIP(sAccountMgr.GetIP(m_account->netinfo));

	if (m_pendingmaptransfer.newmapid == 0xFFFFFFFF)
	{
		Unit::OnRemoveFromWorld();
		UnloadItems();
		UnloadMarks();
		m_ignoredNames.clear();

		//remove from pvp combat
		RemovePVPCombat();

		//mark online for website
		sDatabase.Execute("delete from `player_online` where `playerid` = %u;", m_playerId);
	}
}

void Player::GiveExp( uint32 xp )
{
	if (m_experience <= 0xFFFFFFFF - xp)
		m_experience += xp;
	else
		m_experience = 0xFFFFFFFF;
}

void Player::BuildOnlineList()
{
	InfoMessage msg;

	//every one starts with a space, each line is 0x28 bytes, null terminated (but still uses that amount of bytes, gay)
	char tmp[0x28], tmp2[0x28];
	char line[0x28];

	for (unordered_map<uint32, Player*>::iterator itr = m_mapmgr->m_playerStorage.begin(); itr != m_mapmgr->m_playerStorage.end(); ++itr)
	{
		char* line_to_use = line;

		Player* p = itr->second;
		if (p->m_invis)
			continue;
		sprintf(line, "%s - Level %u", p->m_namestr.c_str(), p->m_level);
		if (GetLinkID() != 0)
		{
			if (p->GetLinkID() == GetLinkID())
			{
				sprintf(tmp, "* %s", line);
				line_to_use = tmp;
				if (p->IsLinkOwner())
				{
					sprintf(tmp2, "%s (L)", tmp);
					line_to_use = tmp2;
				}
			}
		}
		msg.messages.push_back(line_to_use);
	}
	SendInfoMessage(msg);
}

void Player::UnloadMarks()
{
	for (uint32 i = 0; i < NUM_MARKS; ++i)
		memset(&m_marks[i], 0, sizeof(Point));
	m_marksLoaded = false;
}

void Player::OnHealthChange(uint32 old)
{
	Unit::OnHealthChange(old);
	
	int32 diff = m_health - old;

	if (diff != 0)
	{
		Packet data(SMSG_HEALTH_CHANGE_SELF);
		data << diff;
		Send(&data);
	}

	if (!IsAlive() && GetHealth() == 0)
	{
		//remove old events
		RemoveEvents(EVENT_PLAYER_RESPAWN);
		AddEvent(&Player::Respawn, EVENT_PLAYER_RESPAWN, 8000, 1, EVENT_FLAG_NOT_IN_WORLD);

		bool dropitems = true;

		for (std::map<Unit*, uint8>::iterator itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
		{
			if (!itr->first->IsCreature())
				continue;
			if (TO_CREATURE(itr->first)->m_script != NULL)
				TO_CREATURE(itr->first)->m_script->OnKilledPlayer(this);

			if (TO_CREATURE(itr->first)->m_proto->boss == 1)
			{
				m_bosskilledid = itr->first->m_serverId;
				dropitems = false;
			}
		}

		RemoveEvents(EVENT_PLAYER_REMOVEPVPCOMBAT);
		AddEvent(&Player::RemovePVPCombat, EVENT_PLAYER_REMOVEPVPCOMBAT, 0, 1, 0);

		//find a bag
		Item* curbag = FindEquippedItem(220);
		Item* curlsa = FindEquippedItem(201);

		if (curlsa != NULL)
		{
			//delete all lsas
			while (curlsa != NULL)
			{
				curlsa->Delete();
				curlsa = FindEquippedItem(201);
			}

			m_canResurrect = true;
			SendActionResult(PLAYER_ACTION_PRESS_F12_TO_RETURN);
			RemoveEvents(EVENT_PLAYER_RESPAWN);
		}

		SpecialLocation* loc = m_mapmgr->GetSpecLoc(m_positionX, m_positionY);

		if (loc != NULL && loc->flags & SPEC_LOC_FLAG_NO_ITEM_DROP)
			dropitems = false;

 		if (dropitems && GetLevel() >= 60 && m_account->m_data->accountlevel == 0)
 		{
			if (curbag != NULL && RandomUInt(9) == 0)
				curbag->Delete();
			if (curbag == NULL)
			{
				//drop a random equipped item
				uint32 dropslots[NUM_EQUIPPED_ITEMS];
				uint32 dropslotcounter = 0;
				for (int32 i = 0; i < NUM_EQUIPPED_ITEMS; ++i)
				{
					if (m_items[i] != NULL)
						dropslots[dropslotcounter++] = i;
				}

				if (dropslotcounter != 0)
					m_items[dropslots[RandomUInt(dropslotcounter - 1)]]->Drop();
			}

			//drop inventory
			for (uint32 i = NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS - 1; i >= NUM_EQUIPPED_ITEMS; --i)
			{
				if (m_items[i] != NULL)
					m_items[i]->Drop();
			}
 		}
	}
}

void Player::UpdateBodyArmorVisual()
{
	SetModels(sItemStorage.GetArmorVisual(this));
}

void Player::SetPVPCombat()
{
	m_pvpcombatexpire = g_mstime + 300000;
	m_pvpattackexpire = g_mstime + 300000;
	UpdateAllHealthBars();
}

void Player::EventTeleport( int32 x, int32 y, uint32 mapid )
{
	if (m_mapmgr == NULL || (m_mapmgr->IsNoMarkLocation(x, y) && m_account->m_data->accountlevel == 0))
		return;
	CreatePortPop();

	if (mapid == m_mapid)
	{
		SetPosition(x, y);
		OnTeleportComplete();
	}
	else
	{
		if (!IsAlive())
			return;
		m_pendingmaptransfer.newmapid = mapid;
		m_pendingmaptransfer.newx = x;
		m_pendingmaptransfer.newy = y;
		m_pendingmaptransfer.playerportal = true;
		m_mapmgr->RemoveObject(this);
	}
}

void Player::ResetStats()
{
	m_strength = 10;
	m_agility = 10;
	m_constitution = 10;
	m_intellect = 10;
	m_wisdom = 10;
	m_points = 25 + m_level - 1;

	for (uint32 i = 0; i < NUM_SKILLS; ++i)
		m_skills[i] = 0;
	m_skillpoints = m_level;
	m_pvppoints = m_pvplevel;

	int32 statbonus = 0;
	if (m_level >= 10)
		statbonus += 2;
	if (m_level >= 36)
		statbonus += 7;

	if (statbonus > 0)
	{
		m_strength += statbonus;
		m_agility += statbonus;
		m_constitution += statbonus;
		m_intellect += statbonus;
		m_wisdom += statbonus;
	}

	OnStatsChange();
	Recalculate();
	SendAllStats();
}

void Player::OnStatsChange()
{
	for (uint32 i = 0; i < NUM_EQUIPPED_ITEMS; ++i)
	{
		if (m_items[i] == NULL)
			continue;
		if (!m_items[i]->CheckRequirements())
			m_items[i]->RemoveBonuses();
		else
			m_items[i]->ApplyBonuses();
	}
}

uint8 Player::GetPath()
{
	uint32 serverType = m_config.GetIntDefault("Server", "Type", 0);

	if (serverType == 0) //GVE
	{
		if(m_race >= 0 && m_race <= 5)
			return PATH_LIGHT;
		else
			return PATH_DARKNESS;
	}
	if (serverType == 1) //ANTIPK
		return PATH_LIGHT; //all players are light in anti pk server
	return PATH_LIGHT;
}

void Player::SwitchWeapons()
{
	Item* curweapon = GetCurrentWeapon();

	if (curweapon != NULL)
	{
		curweapon->RemoveItemVisual();
		curweapon->RemoveBonuses();
	}

	if (m_weaponSwitch == 0)
		m_weaponSwitch = 1;
	else
		m_weaponSwitch = 0;

	curweapon = GetCurrentWeapon();
	if (curweapon != NULL)
	{
		curweapon->ApplyItemVisual();
		if (curweapon->CheckRequirements())
			curweapon->ApplyBonuses();
	}
}

void Player::AddSkillCooldown( uint32 skillid )
{
	SkillEntry* skill = sSkillStore.GetEntry(skillid);
	if (skill == NULL)
		return;
	uint32 cooldown = g_mstime + (skill->cooldown * 1000);

	std::map<uint32, uint32>::iterator itr = m_skillcooldowns.find(skillid);
	if (itr == m_skillcooldowns.end())
		m_skillcooldowns.insert(std::make_pair(skillid, cooldown));
	else
		itr->second = cooldown;

	//tell everyone you're using the skill
	char tmp[1024];
	sprintf(tmp, "<%s>", skill->name);
	SendChatMsg(tmp);
}

bool Player::SkillReady( uint32 skillid )
{
	std::map<uint32, uint32>::iterator itr = m_skillcooldowns.find(skillid);
	if (itr == m_skillcooldowns.end())
		return true;
	return (g_mstime >= itr->second);
}

void Player::AddToLink( Player* plr )
{
	if (m_mapmgr == NULL)
		return;
	if (GetLinkID() == 0)
	{
		//create link
		g_ServerSettings.Lock();
		auto var = g_ServerSettings.GetUInt("max_link_id") + 1;
		if (var == 1)
			var = 2;
		g_ServerSettings.UpdateSetting("max_link_id", var);
		g_ServerSettings.Unlock();

		SetLinkID(var);
	}
	plr->SetLinkID(GetLinkID());
	plr->SetLinkOwner(false);
	SetLinkOwner(true);
	plr->EventChatMsg(0, "Joined %s", m_namestr.c_str());
	m_mapmgr->OnPlayerLinkJoin(this, plr);
}

void Player::ModSkill( uint32 skillid, int32 mod )
{
	SkillEntry* skill = sSkillStore.GetEntry(skillid);
	if (skill == NULL)
		return;

	m_skills[skillid] += mod;;
	if (skill->serverindex > 60)
	{
		if (mod > m_pvppoints)
			return;
		m_pvppoints -= mod;
	}
	else
	{
		if (mod > m_skillpoints)
			return;
		m_skillpoints -= mod;
	}
}

void Player::OnSkillsLoad( MYSQL_RES* ev )
{
	if (!m_skillsLoaded && ev != NULL)
	{
		m_skillsLoaded = true;
		MYSQL_ROW row = sDatabase.Fetch(ev);
		while (row != NULL)
		{
			uint32 skillid = atol(row[1]);
			uint32 val = atol(row[2]);
			ModSkill(skillid, val);
			row = sDatabase.Fetch(ev);
		}
		Recalculate();
	}
}

void Player::OnTeleportComplete()
{
}

bool Player::CanCast( uint32 spellid, bool triggered /*= false*/ )
{
	SpellData* sp = sSpellStorage.GetSpell(spellid);
	SpellEntry* en = sSpellData.GetEntry(spellid);
	if (sp != NULL && !triggered)
	{
		if (spellid == 0) //"no spell" padding
			return false;
		if (sp->classlevel[m_class] > (m_level + m_bonuses[BONUS_ABILITY_LEVEL]))
			return false;
		if (sp->manareq > m_mana)
			return false;
	}
	if (en != NULL)
	{
		bool hasskill = false;
		if (en->skillreq != 0)
		{
			if (en->skillreq < NUM_SKILLS && GetSkillValue(en->skillreq) > 0 && SkillReady(en->skillreq))
				hasskill = true;
		}
		else
			hasskill = true;
		if (en->skillreq2 != 0)
		{
			if (en->skillreq2 < NUM_SKILLS && GetSkillValue(en->skillreq2) > 0 && SkillReady(en->skillreq2))
				hasskill = true;
		}
		if (!hasskill)
			return false;

		if (en->effect == SPELL_EFFECT_THROW_WEAPON)
		{
			Item* curweapon = GetCurrentWeapon();
			if (curweapon == NULL)
				return false;
			if (curweapon->m_data->id == m_thrownweapon)
				return false;
		}
	}
	return true;
}

void Player::GivePVPExp( Player* other, PVPKillReason reason /*= PVP_KILL_DEATH*/ )
{
	if (GetLevel() < 10)
		return;
	int32 leveldiff = other->GetLevel();
	leveldiff -= GetLevel();
	leveldiff = abs(leveldiff);

	if (leveldiff > 20)
	{
		if (reason == PVP_KILL_DEATH)
		{
			LOG_ACTION_U(this, "PVPKILL", "Killed %s (level difference %u, no pvp exp given)", other->m_namestr.c_str(), leveldiff);
			LOG_ACTION_U(other, "PVPDEATH", "Killed by %s", m_namestr.c_str());
		}

		if (reason == PVP_KILL_ALMUS)
		{
			LOG_ACTION_U(this, "PVPKILL", "Popped %s's almus (level difference %u, no pvp exp given)", other->m_namestr.c_str(), leveldiff);
			LOG_ACTION_U(other, "PVPDEATH", "Almus popped by %s", m_namestr.c_str());
		}

		if (reason == PVP_KILL_REDITUS)
		{
			LOG_ACTION_U(this, "PVPKILL", "Popped %s's reditus (level difference %u, no pvp exp given)", other->m_namestr.c_str(), leveldiff);
			LOG_ACTION_U(other, "PVPDEATH", "Reditus amulet popped by %s", m_namestr.c_str());
		}

		return;
	}

	uint32 xp = other->GetLevel() / other->m_pvpdiminish;

	//uint32 serverType = m_config.GetIntDefault("Server", "Type", 0);

	//if (serverType == 0 && m_faction == other->m_faction) //the fuck
	//	return;

	switch (reason)
	{
	case PVP_KILL_ALMUS: xp *= 0.5f; break;
	case PVP_KILL_REDITUS: xp *= 0.5f; break;
	}

	if (other->m_pvpdiminish >= 20)
		xp = 0;

	++other->m_pvpdiminish;

	other->AddEvent(&Player::RemovePVPDiminish, EVENT_PLAYER_REMOVEPVPDIMINISHTOKEN, 300000, 1, 0);

	if (reason == PVP_KILL_DEATH)
	{
		LOG_ACTION_U(this, "PVPKILL", "Killed %s (%u pvp exp)", other->m_namestr.c_str(), xp);
		LOG_ACTION_U(other, "PVPDEATH", "Killed by %s", m_namestr.c_str());
	}

	if (reason == PVP_KILL_ALMUS)
	{
		LOG_ACTION_U(this, "PVPKILL", "Popped %s's almus (%u pvp exp)", other->m_namestr.c_str(), xp);
		LOG_ACTION_U(other, "PVPDEATH", "Almus popped by %s", m_namestr.c_str());
	}

	if (reason == PVP_KILL_REDITUS)
	{
		LOG_ACTION_U(this, "PVPKILL", "Popped %s's reditus amulet (%u pvp exp)", other->m_namestr.c_str(), xp);
		LOG_ACTION_U(other, "PVPDEATH", "Reditus amulet popped by %s", m_namestr.c_str());
	}

	if (xp == 0)
		return;

	GivePVPExp(xp);
}

void Player::GivePVPExp( uint32 xp )
{
	if (m_pvpexp <= 0xFFFFFFFF - xp)
		m_pvpexp += xp;
	else
		m_pvpexp = 0xFFFFFFFF;

	if (m_pvplevel < MAX_LEVEL && m_pvpexp >= g_expByLevel[m_pvplevel])
	{
		SendActionResult(PLAYER_ACTION_PVP_LEVEL_UP);

		++m_pvppoints;
		++m_pvplevel;
	}
}

void Player::Respawn()
{
	if (m_bosskilledid != 0)
	{
		Object* boss = m_mapmgr->GetObject(m_bosskilledid);
		if (boss != NULL && boss->IsCreature() && TO_UNIT(boss)->IsAlive() && TO_CREATURE(boss)->m_incombat)
		{
			AddEvent(&Player::Respawn, EVENT_PLAYER_RESPAWN, 5000, 1, EVENT_FLAG_NOT_IN_WORLD);
			return;
		}
	}

	GoHome();
	Revive();
}

void Player::OnDeathTokensLoad( MYSQL_RES* ev )
{
	if (ev == NULL)
		return;
	for (MYSQL_ROW info = sDatabase.Fetch(ev); info != NULL; info = sDatabase.Fetch(ev))
	{
		uint32 timeleft = atol(info[1]);
		++m_pvpdiminish;
		AddEvent(&Player::RemovePVPDiminish, EVENT_PLAYER_REMOVEPVPDIMINISHTOKEN, timeleft, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void Player::SendInfoMessage( InfoMessage & msg )
{
	ServerPacket data(SMSG_CHAT_INFO_MESSAGE, 400);

	uint32 counter = 0;

	for (std::vector<std::string>::iterator itr = msg.messages.begin(); itr != msg.messages.end(); ++itr)
	{
		const char* cstr = (*itr).c_str();
		uint32 len = strlen(cstr) + 1;
		data.Write((uint8*)cstr, len > 0x28? 0x28 : len);
		if (len < 0x28)
			data.WriteZero(0x28 - len);

		++counter;
		if ((counter % 10) == 0)
		{
			Send(&data);
			data.Recreate(SMSG_CHAT_INFO_MESSAGE, 400);
		}
	}

	//send last stuff
	if ((counter % 10) != 0)
	{
		while ((counter % 10) != 0)
		{
			data.WriteZero(0x28);
			++counter;
		}
		Send(&data);
	}
}

void Player::OnSkillCooldownLoad( MYSQL_RES* ev )
{
	if (ev == NULL)
		return;
	for (MYSQL_ROW info = sDatabase.Fetch(ev); info != NULL; info = sDatabase.Fetch(ev))
	{
		uint32 skillid = atol(info[1]);
		uint32 timeleft = atol(info[2]);
		m_skillcooldowns.insert(std::make_pair(skillid, g_mstime + timeleft));
	}
}

void Player::CalculateMana()
{
	uint32 manastatval = max(m_intellect, m_wisdom);

	double coremana = m_level * 12 + manastatval * 20 + GetBonus(manastatval) * 20;

	if (m_level < 40)
		coremana *= double(m_level) / 40;

	float bonus = coremana * float(m_bonuses[MANA_BONUS] + GetSkillValue(SKILL_MAGIC)) / 100;
	m_maxmana = coremana * g_manaPercentByClass[m_class] + bonus;

	/*if (m_class == CLASS_CLERIC || m_class == CLASS_DIABOLIST || m_class == CLASS_PALADIN)
	{
		static float fullalign = 1.624;
		static float noalign = 0.25;
		static float range = fullalign - noalign;
		float alignpct = float(m_alignment + 1024) / 2048;
		m_maxmana *= noalign + (range * alignpct);
	}*/

	if (m_mana > m_maxmana)
		SetMana(m_maxmana);
}

void Player::OnKill( Creature* cre )
{
	if (m_currentquest != NULL && m_currentquest->reqmob == cre->m_proto->entry && m_currentquest->reqcount > m_currentquestcount)
	{
		if (m_currentquest->locs[0] != 0)
		{
			//check were in right locs
			uint32 tx = m_positionX / 20;
			uint32 ty = m_positionY / 20;
			bool locmatch = false;
			for (uint32 i = 0; i < 5; ++i)
			{
				if (m_currentquest->locs[i] == 0)
					continue;
				QuestLoc* loc = sQuestLocs.GetEntry(m_currentquest->locs[i]);
				if (loc == NULL)
					continue;
				if (tx >= loc->x && ty >= loc->y && tx < loc->x2 && ty < loc->y2)
				{
					locmatch = true;
					break;
				}
			}

			if (!locmatch)
				return;
		}

		++m_currentquestcount;
		ResetChatMsgTimer();
		m_nochatupdate = true;
		char buf[1024];
		sprintf(buf, "<%u of %u>", m_currentquestcount, m_currentquest->reqcount);
		SendChatMsg(buf);
		m_nochatupdate = false;
	}
}

void Player::InitiateTrade()
{
	//if (m_mapmgr == NULL || m_mapmgr->GetPlayer(otherplr) == NULL)
	//	return;

	//send packet
	ServerPacket data(SMSG_INITIATE_TRADE);

	for (uint32 i = NUM_EQUIPPED_ITEMS; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS; ++i)
	{
		if (m_items[i] == NULL)
			data << uint16(0) << uint16(0);
		else
			data << uint16(m_items[i]->m_data->id) << uint16(m_items[i]->m_stackcount);
	}

	data << uint32(5000000); //unknown, probly gold
	data << uint16(0); //unknown
	//data << uint16(3); //unknown

	int16 checksum = 0;
	for (uint32 i=0; i<205; ++i)
		checksum += *(int8*)&data.GetBuffer()[i + 1];
	data.WriteTo(207);
	data << checksum;
	Send(&data);
}

void Player::SendTradeUpdate()
{
	if (m_mapmgr == NULL)
	{
		CancelTrade();
		return;
	}

	Player* otherplr = m_mapmgr->GetPlayer(m_tradetarget);

	if (otherplr == NULL)
	{
		CancelTrade();
		return;
	}

	ServerPacket data(SMSG_TRADE_UPDATE);

	for (uint32 i = 0; i < 10; ++i)
	{
		if (m_tradeitems[i] != 0)
		{
			if (m_items[m_tradeitems[i]] == NULL)
			{
				CancelTrade();
				return;
			}
			data << uint16(m_items[m_tradeitems[i]]->m_data->id) << uint32(m_items[m_tradeitems[i]]->m_stackcount);
		}
		else
		{
			data << uint16(0);
			data << uint32(0);
		}
		data << uint32(0);
		data << uint32(0); //durability
		data << uint32(0);
	}

	for (uint32 i = 0; i < 10; ++i)
	{
		if (otherplr->m_tradeitems[i] != 0)
		{
			if (otherplr->m_items[otherplr->m_tradeitems[i]] == NULL)
			{
				CancelTrade();
				return;
			}
			data << uint16(otherplr->m_items[otherplr->m_tradeitems[i]]->m_data->id) << uint32(otherplr->m_items[otherplr->m_tradeitems[i]]->m_stackcount);
		}
		else
		{
			data << uint16(0);
			data << uint32(0);
		}
		data << uint32(0);
		data << uint32(0); //durability
		data << uint32(0);
	}

	Send(&data);
}

void Player::CancelTrade()
{
	ServerPacket data(MSG_CLIENT_INFO, 12);
	data << uint32(0x1E);
	Send(&data);

	uint32 targetplr = m_tradetarget;
	m_tradetarget = 0;
	for (uint32 i = 0; i < 10; ++i)
		m_tradeitems[i] = 0;
	m_tradeaccept = false;

	if (m_mapmgr == NULL)
		return;
	Player* otherplr = m_mapmgr->GetPlayer(targetplr);

	if (otherplr == NULL || otherplr->m_tradetarget != m_playerId)
		return;

	otherplr->CancelTrade();
}

void Player::AddTradeItem( uint32 slot )
{
	if (m_mapmgr == NULL)
		return;
	//FIND OTHER PLAYER FIRST
	Player* otherplr = m_mapmgr->GetPlayer(m_tradetarget);

	if (otherplr == NULL)
		return;

	//check if its already there
	for (uint32 i = 0; i < 10; ++i)
	{
		if (m_tradeitems[i] == slot)
			return;
	}

	bool couldadd = false;

	for (uint32 i = 0; i < 10; ++i)
	{
		if (m_tradeitems[i] == 0)
		{
			couldadd = true;
			m_tradeitems[i] = slot;
			break;
		}
	}
	
	//reset accepted flag
	m_tradeaccept = false;
	otherplr->m_tradeaccept = false;
	SendTradeUpdate();
	otherplr->SendTradeUpdate();
}

void Player::AcceptTrade()
{
	if (m_mapmgr == NULL)
	{
		CancelTrade();
		return;
	}
	if (m_tradetarget == 0)
		return;

	Player* otherplr = m_mapmgr->GetPlayer(m_tradetarget);

	if (otherplr == NULL)
	{
		CancelTrade();
		return;
	}

	if (otherplr->m_tradeaccept)
	{
		CompleteTrade();
		return;
	}

	m_tradeaccept = true;
}

void Player::CompleteTrade()
{
	if (m_mapmgr == NULL)
		return;
	Player* otherplr = m_mapmgr->GetPlayer(m_tradetarget);

	if (otherplr->m_tradetarget != m_playerId)
		return;

	//Ensure we have sufficient space
	uint32 numitems = 0, numitems2 = 0;

	for (uint32 i = 0; i < 10; ++i)
	{
		if (m_tradeitems[i] != 0)
			numitems++;
		if (otherplr->m_tradeitems[i] != 0)
			numitems2++;
	}

	if (numitems2 > numitems && (numitems2 - numitems) > GetNumFreeInventorySlots()) //player will gain too many items
	{
		CancelTrade();
		SendChatMsg("\xB3""Not enough free space\xB3");
		return;
	}

	if (numitems > numitems2 && (numitems - numitems2) > otherplr->GetNumFreeInventorySlots()) //player will gain too many items
	{
		CancelTrade();
		otherplr->SendChatMsg("\xB3""Not enough free space\xB3");
		return;
	}

	//lets start getting items!!
	Item* myitems[10];
	Item* otheritems[10];

	for (uint32 i = 0; i < 10; ++i)
	{
		if (m_tradeitems[i] == 0)
			myitems[i] = NULL;
		else
		{
			myitems[i] = m_items[m_tradeitems[i]];
			if (myitems[i] == NULL)
			{
				CancelTrade();
				return;
			}
		}
		if (otherplr->m_tradeitems[i] == 0)
			otheritems[i] = NULL;
		else
		{
			otheritems[i] = otherplr->m_items[otherplr->m_tradeitems[i]];
			if (otheritems[i] == NULL)
			{
				CancelTrade();
				return;
			}
		}
	}

	for (uint32 i = 0; i < 10; ++i)
	{
		if (myitems[i] != NULL)
		{
			myitems[i]->RemoveFromInventory();
		}
		if (otheritems[i] != NULL)
		{
			otheritems[i]->RemoveFromInventory();
		}
	}

	for (uint32 i = 0; i < 10; ++i)
	{
		if (myitems[i] != NULL)
		{
			myitems[i]->m_owner = otherplr;
			myitems[i]->AddToInventory();
		}
		if (otheritems[i] != NULL)
		{
			otheritems[i]->m_owner = this;
			otheritems[i]->AddToInventory();
		}
	}

	//now cancel
	CancelTrade();
}

void Player::OnCompletedQuestsLoad(MYSQL_RES* ev)
{
	if (ev == NULL)
		return;
	for (MYSQL_ROW info = sDatabase.Fetch(ev); info != NULL; info = sDatabase.Fetch(ev))
	{
		uint32 questid = atol(info[1]);
		m_completedquests.insert(questid);
	}
}

void Player::OnCurrencyLoad(MYSQL_RES* ev)
{
	if (ev == NULL)
		return;
	for (MYSQL_ROW info = sDatabase.Fetch(ev); info != NULL; info = sDatabase.Fetch(ev))
	{
		uint32 currency = atol(info[1]);
		int32 value = atol(info[2]);
		ModCurrency(currency, value);
	}
}

void Player::OpenStorage()
{
	ServerPacket data(SMSG_STORAGE_DATA);

	for (uint32 i = NUM_EQUIPPED_ITEMS; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS + NUM_STORAGE_ITEMS; ++i)
	{
		if (m_items[i] == NULL)
			data << uint16(0) << uint16(0) << uint8(0) << uint8(0);
		else
			data << uint16(m_items[i]->m_data->id) << uint16(m_items[i]->m_stackcount) << uint8(0) << uint8(0);
	}

	data.AddChecksumPartial();
	Send(&data);
}

void Player::CompactItems()
{
	for (uint32 i = NUM_EQUIPPED_ITEMS + 1; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS + NUM_STORAGE_ITEMS; ++i)
	{
		if (m_items[i] != NULL && m_items[i - 1] == NULL)
		{
			uint32 minpos = NUM_EQUIPPED_ITEMS;
			if (i >= NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS)
				minpos += NUM_INVENTORY_ITEMS;
			for (uint32 j = i; j > minpos; --j)
			{
				if (m_items[j - 1] != NULL)
					break;
				m_items[j - 1] = m_items[j];
				m_items[j - 1]->m_currentslot = j - 1;
				m_items[j] = NULL;
			}
		}
	}
}

void Player::UpdateMovement( Object* followingObject /*= NULL*/ )
{
	Unit::UpdateMovement();
	return;

	if (followingObject == NULL && m_POIObject != 0)
		followingObject = m_mapmgr->GetObject(m_POIObject);

	float speed = m_speed * (1 + float(m_agility) / 200);

	if (m_attackRun)
		speed *= 4;

	if (CanMove() && m_destinationX != 0 && m_destinationY != 0 && GetDistance(m_destinationX, m_destinationY) > m_destinationRadius)
	{
		//are we in an invalid position? wtf :P
		float dx = m_destinationX - m_positionX;
		float dy = m_destinationY - m_positionY;
		float distance = sqrt((float)((dx * dx) + (dy * dy)));
		float angle = CalcFacingDeg(m_destinationX, m_destinationY);
		uint16 newx = 0, newy = 0;

		bool finished = false;

		if (distance <= speed)
		{
			float drot = (CalcFacingDeg(m_destinationX, m_destinationY, m_positionX, m_positionY) - 90) * PI / 180.0f;
			newx = m_destinationX - sinf(drot) * (m_destinationRadius * 0.9);
			newy = m_destinationY + cosf(drot) * (m_destinationRadius * 0.9);
			finished = true;
		}
		else
		{
			newx = m_positionX + uint16(dx / (distance / speed));
			newy = m_positionY + uint16(dy / (distance / speed));
		}

		PathTileData* hit = NULL;

		bool pass = true;
		if (!m_mapmgr->CanPass(newx / 20, newy / 20, GetPassFlags()))
			pass = false;
		if (!m_mapmgr->InLineOfSight(m_positionX, m_positionY, newx, newy, GetPassFlags(), &hit))
			pass = false;


		if (pass)
		{
			if (finished)
			{
				if (followingObject != NULL)
					m_rotation = CalcFacingDeg(followingObject->m_positionX, followingObject->m_positionY);
				else
					m_rotation = CalcFacingDeg(m_destinationX, m_destinationY);
				SetPosition(newx, newy);
				SetUpdateBlock(UPDATETYPE_MOVEMENT);

				m_endmove = 0;
				m_destinationX = 0;
				m_destinationY = 0;
				GroundItemPickup();

				m_attackRun = false;
			}
			else
			{
				m_rotation = angle;
				SetPosition(newx, newy);
				if (!HasAnimationType(ANIM_TYPE_MOVING))
					SetAnimationType(ANIM_TYPE_MOVING);
				SetUpdateBlock(UPDATETYPE_MOVEMENT);
			}
		}
		else
		{
			if (hit != NULL)
				ProcTileScript(hit->GetTileX(), hit->GetTileY(), TILE_PROC_BUMP_INTO);
			StopMovement();
		}
	}
}

void Player::SetTarget( Unit* target )
{
	if (target == NULL)
		return;
	m_currenttarget = target->m_serverId;
	m_currenttargetlastupdate = 0;
	Packet p(SMSG_UPDATE_TARGET_NAME);
	p << target->m_namestr;
	Send(&p);
}

void Player::ClearTarget()
{
	m_currenttarget = 0;
	Packet p(SMSG_CLEAR_TARGET);
	Send(&p);
}

void Player::BuildTargetUpdate( Unit* target )
{
	if (target == NULL)
		return;
	Packet p(SMSG_UPDATE_TARGET);
	p << target->GetHealth() << target->GetMaxHealth();
	Send(&p);
}

void Player::SendStats()
{
	ServerPacket data(SMSG_PLAYER_STATS);

	data.WriteTo(1 + (2 * NUM_STATS) + 2);

#define WRITESTAT(stat, var) data.WPos(1 + (2 * stat)); data << uint16(var)
	WRITESTAT(STAT_STRENGTH, m_strength);
	WRITESTAT(STAT_AGILITY, m_agility);
	WRITESTAT(STAT_CONS, m_constitution);
	WRITESTAT(STAT_INTEL, m_intellect);
	WRITESTAT(STAT_WISDOM, m_wisdom);
	WRITESTAT(STAT_MAXHEALTH, m_maxhealth);
	WRITESTAT(STAT_MAXMANA, m_maxmana);

	WRITESTAT(STAT_TRUELEVEL, m_level);
	WRITESTAT(STAT_LEVEL, m_level + m_bonuses[BONUS_ABILITY_LEVEL]);

	WRITESTAT(STAT_CURHEALTH, m_health);
	WRITESTAT(STAT_CURMANA, m_mana);
	WRITESTAT(STAT_POINTS, m_points);

	WRITESTAT(STAT_GENDER, m_gender);
	WRITESTAT(STAT_RACE, m_race);
	WRITESTAT(STAT_CLASS, m_class);
	WRITESTAT(STAT_ALIGNMENT, m_alignment);

	data.WPos(1 + (2 * NUM_STATS) + 2);

	//bonuses
	data << GetMagicBonus(FIRE_MAGIC_BONUS);
	data << GetMagicBonus(ICE_MAGIC_BONUS);
	data << GetMagicBonus(ENERGY_MAGIC_BONUS);
	data << GetMagicBonus(SHADOW_MAGIC_BONUS);
	data << GetMagicBonus(SPIRIT_MAGIC_BONUS);
	//resists
	data << float(1.0f / GetMagicResist(PROTECTION_FIRE, true));
	data << float(1.0f / GetMagicResist(PROTECTION_ICE, true));
	data << float(1.0f / GetMagicResist(PROTECTION_SHADOW, true));
	data << float(1.0f / GetMagicResist(PROTECTION_ENERGY, true));
	data << float(1.0f / GetMagicResist(PROTECTION_SPIRIT, true));

	data << uint16(0); //unknown

	data.AddChecksumPartial();
	Send(&data);
}

void Player::SendExperience()
{
	ServerPacket data(SMSG_PLAYER_EXPERIENCE);

	data << uint32(m_experience);
	data << uint32(m_pvpexp);
	data << uint16(m_pvplevel);

	data.AddChecksumPartial();
	Send(&data);
}

void Player::SendItems()
{
	ServerPacket data(SMSG_PLAYER_ITEMS);

	//item struct is uint16 id uint16 stack uint8 hiddens uint8 unk
	//item equipped[15], item inventory[50], uint32 gold, uint16 tokens, uint16 armor, uint16 unk, int16 checksum

	for (uint32 i = 0; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS; ++i)
	{
		if (m_items[i] == NULL)
			data << uint16(0) << uint32(0); //blank data
		else
		{
			data << uint16(m_items[i]->m_data->id);
			data << uint32(m_items[i]->m_stackcount);
		}
	}

	data << uint16(0); //unk
	data << uint32(0); //gold, NYI
	data << uint16(0); //tokens, NYI
	data << uint16(m_bonuses[PROTECTION_ARMOR_BONUS]); //TODO: add support for corrupt lowering visual
	//data << uint16(0); //unk

	data.AddChecksumFull();
	Send(&data);
}

void Player::SendSkills()
{
	//uint8 skills[81]
	//int16 checksum

	ServerPacket data(SMSG_PLAYER_SKILLS);
	data.WriteTo(0 + 1);
	data << uint8(m_skillpoints);
	data.WriteTo(60 + 1);
	data << uint8(m_pvppoints);

	data.WriteTo(81 + 1 + 1);
	for (uint32 i = 0; i < NUM_SKILLS; ++i)
	{
		if (GetSkillValue(i) == 0)
			continue;
		SkillEntry* skill = sSkillStore.GetEntry(i);
		if (skill == NULL || skill->serverindex == 0)
			continue;
		data.WPos(1 + skill->serverindex);
		data << uint8(GetSkillValue(i));
	}
	//write past skills so we cant overwrite now
	data.WPos(81 + 1 + 1);
	data.AddChecksumPartial();
	Send(&data);
}

bool Player::InPVPMode()
{
	return m_pvpcombatexpire > g_mstime;
}

void Player::OnRemoveInRangeObject( Object* o )
{
	if (o->IsUnit())
	{
		Packet data(SMSG_REMOVE_OBJECT);
		data << uint32(o->m_serverId);
		Send(&data);
	}
}

void Player::OnAddInRangeObject( Object* o )
{
	if (o->IsUnit())
		TO_UNIT(o)->SendHealthUpdate(this);
}

bool Player::IsEligableForQuest( QuestEntry* quest )
{
	if (m_currentquest != NULL)
		return false;
	if (GetLevel() > quest->maxlevel || GetLevel() < quest->reqlevel)
		return false;
	if (quest->flags & 1 && m_completedquests.find(quest->id) != m_completedquests.end())
		return false;
	if (quest->reqquest != 0 && m_completedquests.find(quest->reqquest) == m_completedquests.end())
		return false;
	return true;
}

void Player::SendQuestInfoOfAll( uint32 type )
{
	for (auto itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
	{
		if (itr->first->IsCreature())
			TO_CREATURE(itr->first)->SendPlayerQuestInfo(this, type);
	}
}

void Player::UpdateAllCreatureQuestInfo()
{
	for (auto itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
	{
		if (itr->first->IsCreature())
			TO_CREATURE(itr->first)->SendPlayerQuestInfo(this);
	}
}

void Player::UpdateAllHealthBars()
{
	for (auto itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
	{
		Unit* u = itr->first;
		u->SendHealthUpdate(this);
		if (u->IsPlayer())
			SendHealthUpdate(TO_PLAYER(u));
	}
}