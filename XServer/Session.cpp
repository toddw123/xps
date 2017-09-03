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

PacketHandler PacketHandlers[NUM_MSG_TYPES];

void Session::HandleMovement(ServerPacket &data)
{
	uint32 unk, x, y;
	uint8 moveaction;

	data >> unk >> x >> y >> moveaction;

	uint8 checksum, counter;
	data >> checksum >> counter;

 	int16 lastmovecounter = m_lastmovecounter;
 	int16 iscounter = counter;
 	int16 diff = abs(lastmovecounter - iscounter);

	if (!m_player->CanMove() || !m_player->IsAlive())
		return;

	m_player->StopMovement();

	if (m_player->m_extralog)
	{
		LOG_ACTION_U(m_player, "EXTRALOG", "Movement: action %u arg1 %u arg2 %u counter %u checksum %u", moveaction, x, y, counter, checksum);
		LOG_ACTION_U(m_player, "MOVE", "Movement: action %u arg1 %u arg2 %u counter %u checksum %u", moveaction, x, y, counter, checksum);
	}

	if (moveaction == 0x4D || moveaction == 0x4C) //movement
	{
		Unit* moveunit = m_player;
		if (m_player->m_petcontrolid != 0)
		{
			moveunit = TO_UNIT(m_player->m_mapmgr->GetObject(m_player->m_petcontrolid));
			m_player->m_petcontrolid = 0;
			if (moveunit == NULL)
				moveunit = m_player;
		}

		if (moveunit->HasAura(SPELL_AURA_MULTI_PROJECTILE))
		{
			moveunit->m_rotation = moveunit->CalcFacingDeg(x, y);
			moveunit->SetUpdateBlock(UPDATETYPE_MOVEMENT);
			return;
		}

		if (moveunit->HasUnitFlag(UNIT_FLAG_IS_GHOST))
		{
			moveunit->SetPosition(x, y);
			moveunit->m_rotation = moveunit->CalcFacingDeg(x, y);
			moveunit->SetUpdateBlock(UPDATETYPE_MOVEMENT);
		}
		
		moveunit->CreatePath(x, y);
		moveunit->m_destinationRadius = 0;
		moveunit->ClearPOI();
		moveunit->m_attackRun = false;
		moveunit->SetCrowdTarget(x, y);

		//set mode to stop if its a pet
		if (moveunit->IsPet())
			TO_CREATURE(moveunit)->m_petstate = PET_STATE_STOP;
	}
	else if (moveaction == 0x41) //attack unit
	{
		if (m_player->HasAura(SPELL_AURA_MUTATIO_NIMBUS) || m_player->HasAura(SPELL_AURA_MUTATIO_FLAMMA))
			return;

		Object* obj = m_player->m_mapmgr->GetObject(x);
		if (obj == NULL || !m_player->IsInRange(obj))
			return;

		if (obj->IsCreature() && TO_CREATURE(obj)->m_proto->boss == 4 && m_player->GetDistance(obj) < 50 && !TO_UNIT(obj)->IsHostile(m_player)) //storage merch
		{
			TO_CREATURE(obj)->SendStorageList(m_player);
			return;
		}

		if (obj->IsPet() && TO_CREATURE(obj)->GetOwner() == m_player)
		{
			m_player->m_petcontrolid = obj->m_serverId;
			return;
		}

		if (obj->IsPlayer() && TO_PLAYER(obj)->m_invis)
			return;

		if (m_player->HasAura(SPELL_AURA_MULTI_PROJECTILE))
		{
			m_player->m_rotation = m_player->CalcFacingDeg(obj->m_positionX, obj->m_positionY);
			m_player->SetUpdateBlock(UPDATETYPE_MOVEMENT);
			return;
		}

		//pet attack?
		if (m_player->m_petcontrolid)
		{
			Object* pet = m_player->m_mapmgr->GetObject(m_player->m_petcontrolid);
			m_player->m_petcontrolid = 0;
			if (pet != NULL)
				TO_UNIT(pet)->Attack(obj);
		}
		else
		{
			if (m_player->m_account->m_data->accountlevel > 0)
			{
				if (m_commandflags & COMMAND_FLAG_DELETE_CREATURE && obj->IsCreature())
				{
					auto cre = TO_CREATURE(obj);
					m_commandflags &= ~COMMAND_FLAG_DELETE_CREATURE;
					if (cre->m_spawn == NULL || m_player->m_account->m_data->accountlevel > 10)
						obj->AddEvent(&Creature::RemoveCreature, EVENT_UNKNOWN, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
				}

				if (m_commandflags & COMMAND_FLAG_SET_CREATURE_LEVEL && obj->IsCreature())
				{
					m_commandflags &= ~COMMAND_FLAG_SET_CREATURE_LEVEL;
					TO_CREATURE(obj)->SetLevel(m_commandarg);

					if (TO_CREATURE(obj)->m_spawn != NULL)
					{
						TO_CREATURE(obj)->m_spawn->level = m_commandarg;
						sCreatureSpawn.Save(TO_CREATURE(obj)->m_spawn);
					}
				}

				if (m_commandflags & COMMAND_FLAG_SET_CREATURE_FLAG && obj->IsCreature())
				{
					m_commandflags &= ~COMMAND_FLAG_SET_CREATURE_FLAG;
					TO_CREATURE(obj)->m_unitflags = m_commandarg;

					if (TO_CREATURE(obj)->m_proto != NULL)
					{
						TO_CREATURE(obj)->m_proto->unitflags = m_commandarg;
						sCreatureProto.Save(TO_CREATURE(obj)->m_proto);
					}
				}

				if (m_commandflags & COMMAND_FLAG_SET_CREATURE_FACTION && obj->IsCreature())
				{
					m_commandflags &= ~COMMAND_FLAG_SET_CREATURE_FACTION;
					TO_CREATURE(obj)->m_faction = m_commandarg;

					if (TO_CREATURE(obj)->m_proto != NULL)
					{
						TO_CREATURE(obj)->m_proto->faction = m_commandarg;
						sCreatureProto.Save(TO_CREATURE(obj)->m_proto);
					}
				}

				if (m_commandflags & COMMAND_FLAG_GET_CREATURE_INFO && obj->IsCreature())
				{
					m_commandflags &= ~COMMAND_FLAG_GET_CREATURE_INFO;
					Creature* cre = TO_CREATURE(obj);

					InfoMessage msg;
					msg.AddMessage("Creature %s (%u) - Level %u", cre->m_namestr.c_str(), cre->m_proto->entry, cre->GetLevel());
					if (cre->m_spawn == NULL)
						msg.AddMessage("This creature has no spawn entry");
					else
						msg.AddMessage("Creature spawn %u, x %u y %u level %u", cre->m_spawn->Id, cre->m_spawn->x, cre->m_spawn->y, cre->m_spawn->level);

					msg.AddMessage("Melee damage: %u", cre->GetMeleeDamage());
					msg.AddMessage("Attack speed: %u", cre->GetAttackSpeed());

					m_player->SendInfoMessage(msg);
				}
			}

			if (obj->IsUnit())
				m_player->SetTarget(TO_UNIT(obj));
			m_player->Attack(obj);
		}
	}
	else
	{
		sLog.Debug("Movement", "Unknown movement unk %u x %u y %u type %u", unk, x, y, moveaction);
	}
}

void Session::HandleChat(ServerPacket &data)
{
	uint16 ID, zeros;
	std::string chatmsg;

	data >> ID >> zeros >> chatmsg;

	bool muted = false;
	if (m_player->m_account->m_data->muteexpiretime >= time(NULL))
		muted = true;
	if (m_player->m_namestr == "NONAME")
		muted = true;

	sLog.File(m_player->m_namestr.c_str(), "%s", chatmsg.c_str());

	std::string escapedmsg;
	sDatabase.EscapeStr(chatmsg, escapedmsg);

	LOG_ACTION_U(m_player, "CHAT", "%s", escapedmsg.c_str());

	if (chatmsg.size() > 1 && chatmsg.c_str()[0] == '-' && chatmsg.c_str()[1] == '-' && !muted)
	{

		if (m_player->m_account->m_data->accountlevel == 0 && m_player->m_nextchatmessage > g_mstime)
		{
			m_player->SendChatMsg("<Cannot send global message (spam filter)>", true);
			return;
		}

		m_player->m_nextchatmessage = g_mstime + 1000;

		GlobalSay psay;

		psay.name = m_player->m_namestr;
		replace(psay.name, "\xB3", "--", 0);
		psay.message = chatmsg.substr(2);
		psay.to_link = 0;
		psay.to_player = 0;

		sInstanceMgr.HandleGlobalMessage(psay);

		LOG_ACTION_U(m_player, "GCHAT", "%s", escapedmsg.c_str());
	}
	else
	{
		m_player->m_lastSay = chatmsg;
		replace(m_player->m_lastSay, "\xB3", "--", 0);
		m_player->m_pendingSay = true;
		if (!muted)
			m_player->SetUpdateBlock(UPDATETYPE_CHATMSG); //chat block
		m_player->ProcTileScript(TILE_PROC_FLAG_CHAT);

		uint32 mark = 0;
		if (sscanf(chatmsg.c_str(), "mark%u", &mark))
		{
			if (mark == 0)
				mark = 10; //mark 0 is actually mark 10
			//all this dividing shit is so the mark is set to the center of the tile

			if (mark < NUM_MARKS)
			{
				m_player->m_marks[mark].x = (m_player->m_positionX / 20) * 20 + 10;
				m_player->m_marks[mark].y = (m_player->m_positionY / 20) * 20 + 10;
				m_player->m_marks[mark].mapid = m_player->m_mapid;
				m_player->m_marksLoaded = true; //allow new player marks to save
			}
		}

		if (chatmsg.size() > 0 && (chatmsg.c_str()[0] == '-' || 
			(chatmsg.size() >= 5 &&  chatmsg.c_str()[0] == 't' && chatmsg.c_str()[1] == 'r' && chatmsg.c_str()[2] == 'a' && chatmsg.c_str()[3] == 'd' && chatmsg.c_str()[4] == 'e')
			))
		{
			if (m_player->m_invis)
				m_player->RemoveUpdateBlock(UPDATETYPE_CHATMSG);
			HandleChatCommand(chatmsg);
		}
	}
}

void Session::HandleChatCommand(std::string &str)
{
	std::vector<char*> argv;

	char* split = strtok((char*)str.c_str(), " ");

	if (split != NULL)
	{
		while (split != NULL)
		{
			argv.push_back(split);
			split = strtok(NULL, " ");
		}
	}
	else
		argv.push_back((char*)str.c_str()); //only 1 argument (command)

	if (strcmp(argv[0], "trade") == 0)
	{
		if (m_player->m_tradetarget)
			m_player->CancelTrade();

		for (std::map<Unit*, uint8>::iterator itr = m_player->m_inrangeUnits.begin(); itr != m_player->m_inrangeUnits.end(); ++itr)
		{
			if (!itr->first->IsPlayer())
				continue;
			if (m_player->GetDistance(itr->first) > 50)
				continue;
			if (itr->first->m_lastSay == "trade" && TO_PLAYER(itr->first)->m_tradetarget == 0)
			{
				ServerPacket data(MSG_CLIENT_INFO, 29);
				data << uint32(29);
				data << itr->first->m_namestr;
				m_player->Send(&data);
				ServerPacket data2(MSG_CLIENT_INFO, 29);
				data2 << uint32(29);
				data2 << m_player->m_namestr;
				TO_PLAYER(itr->first)->Send(&data2);
				m_player->m_tradetarget = TO_PLAYER(itr->first)->m_playerId;
				TO_PLAYER(itr->first)->m_tradetarget = m_player->m_playerId;

				//reset lastsay
				itr->first->m_lastSay == "N/A";
				m_player->m_lastSay == "N/A";
			}
		}
	}

	if (strcmp(argv[0], "-dumdance") == 0)
	{
		if (m_player->IsAlive())
			m_player->SetAnimation(ANIMATION_DANCE, ANIMATION_DANCE_2);
		m_player->RemoveUpdateBlock(UPDATETYPE_CHATMSG);
		m_player->m_pendingSay = false;
	}

	if (strcmp(argv[0], "-point") == 0)
	{
		if (m_player->IsAlive())
			m_player->SetAnimation(ANIMATION_POINT, ANIMATION_POINT_2);
		m_player->RemoveUpdateBlock(UPDATETYPE_CHATMSG);
		m_player->m_pendingSay = false;
	}

	if (strcmp(argv[0], "-victory") == 0)
	{
		if (m_player->IsAlive())
			m_player->SetAnimation(ANIMATION_VICTORY, ANIMATION_VICTORY_2);
		m_player->RemoveUpdateBlock(UPDATETYPE_CHATMSG);
		m_player->m_pendingSay = false;
	}

	if (strcmp(argv[0], "-list") == 0)
	{
		m_player->BuildOnlineList();
	}

	if (strcmp(argv[0], "-ignore") == 0)
	{
		if (argv.size() < 2)
			return;

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}
		m_player->m_ignoredNames.insert(name);
	}

	if (strcmp(argv[0], "-follow") == 0)
	{
		for (std::set<uint16>::iterator itr = m_player->m_pets.begin(); itr != m_player->m_pets.end(); ++itr)
		{
			Object* o = m_player->m_mapmgr->GetObject(*itr);
			if (o == NULL || !o->IsCreature())
				continue;
			TO_CREATURE(o)->m_petstate = PET_STATE_FOLLOW;
			TO_CREATURE(o)->ClearPOI();
		}
	}

	if (strcmp(argv[0], "-battle") == 0)
	{
		for (std::set<uint16>::iterator itr = m_player->m_pets.begin(); itr != m_player->m_pets.end(); ++itr)
		{
			Object* o = m_player->m_mapmgr->GetObject(*itr);
			if (o == NULL || !o->IsCreature())
				continue;
			TO_CREATURE(o)->m_petstate = PET_STATE_BATTLE;
			TO_CREATURE(o)->ClearPOI();
		}
	}

	if (strcmp(argv[0], "-stop") == 0)
	{
		for (std::set<uint16>::iterator itr = m_player->m_pets.begin(); itr != m_player->m_pets.end(); ++itr)
		{
			Object* o = m_player->m_mapmgr->GetObject(*itr);
			if (o == NULL || !o->IsCreature())
				continue;
			TO_CREATURE(o)->StopMovement();
			TO_CREATURE(o)->m_petstate = PET_STATE_STOP;
			TO_CREATURE(o)->ClearPOI();
		}
	}

	if (strcmp(argv[0], "-drop") == 0)
	{
		for (std::set<uint16>::iterator itr = m_player->m_pets.begin(); itr != m_player->m_pets.end(); ++itr)
		{
			Object* o = m_player->m_mapmgr->GetObject(*itr);
			if (o == NULL || !o->IsCreature())
				continue;
			Creature* cre = TO_CREATURE(o);
			for (uint32 i = 0; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS; ++i)
			{
				if (cre->m_items[i] != NULL)
					cre->m_items[i]->Drop();
			}
		}
	}

	if (strcmp(argv[0], "-destroy") == 0)
	{
		for (std::set<uint16>::iterator itr = m_player->m_pets.begin(); itr != m_player->m_pets.end(); ++itr)
		{
			Object* o = m_player->m_mapmgr->GetObject(*itr);
			if (o == NULL || !o->IsUnit() || !TO_UNIT(o)->IsAlive())
				continue;
			o->AddEvent(&Unit::SetHealth, (uint32)0, EVENT_UNIT_SETHEALTH, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
		}
	}

	if (strcmp(argv[0], "-lead") == 0)
	{
		if (m_player->GetLinkID() == 0 || m_player->IsLinkOwner())
		{
			m_player->m_leadtimer = g_mstime + 10000;
			m_player->SendChatMsg("\xB3""Accepting Invites\xB3");
		}
		else
			m_player->SendChatMsg("\xB3""Cannot use -lead: You are not the owner of this link.\xB3");
	}

	if (strcmp(argv[0], "-kick") == 0)
	{
		if (!m_player->IsLinkOwner())
			return;
		Player* closestlinkedplr = NULL;
		float closestlinkdist = 99999.9f;

		for (std::map<Unit*, uint8>::iterator itr = m_player->m_inrangeUnits.begin(); itr != m_player->m_inrangeUnits.end(); ++itr)
		{
			if (!itr->first->IsPlayer())
				continue;
			Player* p = TO_PLAYER(itr->first);

			if (p->GetLinkID() != m_player->GetLinkID())
				continue;

			float dist = m_player->GetDistance(p);
			if (dist < closestlinkdist)
			{
				closestlinkedplr = p;
				closestlinkdist = dist;
			}
		}

		if (closestlinkedplr == NULL)
			return;
		closestlinkedplr->EventChatMsg(0, "Kicked by %s", m_player->m_namestr.c_str());
		closestlinkedplr->SetLinkID(0);
	}

	if (strcmp(argv[0], "-join") == 0)
	{
		if (m_player->GetLinkID() != 0)
			m_player->SendChatMsg("\xB3""Cannot use -join: You are already in a link.\xB3");

		std::string joinname;

		if (argv.size() > 1)
		{
			for (uint32 i = 1; i < argv.size(); ++i)
			{
				joinname += argv[i];
				if (i != argv.size() - 1)
					joinname += " ";
			}

			sAccountMgr.FixName(joinname);

			for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
			{
				Player* plr = itr->second;
				if (plr->m_namestr != joinname)
					continue;
				
				bool ownslink = false;
				if (plr->IsLinkOwner())
					ownslink = true;
				if (plr->GetLinkID() == 0)
					ownslink = true;

				if (ownslink && plr->m_leadtimer >= g_mstime)
				{
					plr->AddToLink(m_player);
					plr->m_leadtimer = 0;
				}
			}
		}
		else
		{
			for (std::map<Unit*, uint8>::iterator itr = m_player->m_inrangeUnits.begin(); itr != m_player->m_inrangeUnits.end(); ++itr)
			{
				if (!itr->first->IsPlayer())
					continue;
				Player* plr = TO_PLAYER(itr->first);
				bool ownslink = false;
				if (plr->IsLinkOwner())
					ownslink = true;
				if (plr->GetLinkID() == 0)
					ownslink = true;

				if (ownslink && plr->m_leadtimer >= g_mstime)
				{
					plr->AddToLink(m_player);
					plr->m_leadtimer = 0;
				}
			}
		}

		m_player->UpdateAllHealthBars();
	}

	if (strcmp(argv[0], "-break") == 0)
	{
		if (m_player->GetLinkID() != 0)
			m_player->SendChatMsg("\xB3""Left link\xB3");
		m_player->SetLinkID(0);
		m_player->SetLinkOwner(false);
		m_player->UpdateAllHealthBars();

		SpecialLocation* loc = m_player->m_mapmgr->GetSpecLoc(m_player->m_positionX, m_player->m_positionY);

		if (loc != NULL && loc->flags & SPEC_LOC_FLAG_NO_ITEM_DROP)
			m_player->GoHome();

		m_player->m_mapmgr->OnPlayerLinkBreak(m_player);
	}

	if (strcmp(argv[0], "-quitquest") == 0)
	{
		m_player->m_currentquest = NULL;
		m_player->m_currentquestcount = 0;
		m_player->SendChatMsg("\xB3""Quest Removed\xB3");
		m_player->UpdateAllCreatureQuestInfo();
	}

	/*if (strcmp(argv[0], "-adept") == 0)
	{
		if (m_player->m_level < 60 || m_player->m_class == CLASS_ADEPT)
			return;

		//ok reset everything
		m_player->m_level = 1;
		m_player->m_experience = 0;
		m_player->m_oldclass = m_player->m_class;
		m_player->m_class = CLASS_ADEPT;
		m_player->ResetStats();
		m_player->EventChatMsg(0, "You have become an adept!");
		m_player->EventChatMsg(3000, "Your level has been reset to 1.");
		m_player->SaveToDB();
		m_player->UpdateBodyArmorVisual();
	}*/

	if (strcmp(argv[0], "-unadept") == 0)
	{
		if (m_player->m_class != CLASS_ADEPT)
			return;

		sDatabase.Execute("insert into `player_adept_data` VALUES (%u, %u, %u);", m_player->m_playerId, m_player->m_level, m_player->m_experience);

		//m_player->m_level = 70;
		//m_player->m_experience = 0;
		m_player->m_class = m_player->m_oldclass;
		m_player->ResetStats();
		m_player->EventChatMsg(0, "Adept class change reverted!");

		//remove old equip
		for (uint32 i = 0; i < NUM_EQUIPPED_ITEMS; ++i)
		{
			if (m_player->m_items[i] == NULL)
				continue;

			if (i != ITEM_SLOT_AMULET1 && i != ITEM_SLOT_AMULET2 && i != ITEM_SLOT_AMULET3 && !(m_player->GetItemClass() & m_player->m_items[i]->m_data->propflags))
			{
				m_player->m_items[i]->UnEquip();

				if (m_player->m_items[i] != NULL) //couldnt remove
					m_player->m_items[i]->Delete();
			}
		}

		m_player->SaveToDB();
		m_player->UpdateBodyArmorVisual();
		m_player->m_account->Logout(true);
	}

	if (strcmp(argv[0], "-monsters") == 0)
	{
		m_player->m_attackstate = ATTACK_STATE_MONSTERS;
		m_player->SendChatMsg("\xB3Monsters only\xB3");
		m_player->UpdateAllHealthBars();
	}

	if (strcmp(argv[0], "-stuck") == 0)
	{
		if (m_player->InPVPAttackMode())
			m_player->SendChatMsg("Cannot -stuck in combat.");
		else if (!m_player->IsAlive())
		{
			if (m_player->m_bosskilledid)
			{
				Object* boss = m_player->m_mapmgr->GetObject(m_player->m_bosskilledid);
				if (boss != NULL && boss->IsCreature() && TO_UNIT(boss)->IsAlive() && TO_UNIT(boss)->IsAttacking())
				{
					TO_CREATURE(boss)->m_killedplayers.insert(m_player->m_playerId);
					TO_CREATURE(boss)->m_damagedunits.erase(m_player->m_serverId);
					m_player->m_bosskilledid = 0;
					m_player->Respawn();
				}
			}
			else
				m_player->SendChatMsg("Cannot -stuck while dead.");
		}
		else
			m_player->GoHome();
	}

	/*if (strcmp(argv[0], "-enemies") == 0)
	{
		if (m_player->m_account->m_data->accountlevel > 0)
			return;
		m_player->m_attackstate = ATTACK_STATE_ENEMIES;
		m_player->SendChatMsg("\xB3""All enemies\xB3");
		m_player->UpdateAllHealthBars();
	}*/

	if (strcmp(argv[0], "-all") == 0)
	{
		if (m_player->m_account->m_data->accountlevel > 0)
			return;
		m_player->m_attackstate = ATTACK_STATE_ALL;
		m_player->SendChatMsg("\xB3""Attacking all\xB3");
		m_player->SetPVPCombat();
		m_player->UpdateAllHealthBars();
	}

	if (strcmp(argv[0], "-respec") == 0)
	{
		m_player->ResetStats();
	}

	if (strcmp(argv[0], "-castle") == 0)
	{
		if (m_player->m_unitspecialname2 == UNIT_NAME_SPECIAL2_MAGROTH)
			m_player->SetPosition(332 * 20 + 10, 2015 * 20 + 10);
	}

	if (strcmp(argv[0], "-gcoff") == 0)
	{
		m_player->m_globalchatDisabled = true;
		m_player->SendChatMsg("\xB3""Global Chat Disabled\xB3");
	}

	if (strcmp(argv[0], "-gcon") == 0)
	{
		m_player->m_globalchatDisabled = false;
		m_player->SendChatMsg("\xB3""Global Chat Enabled\xB3");
	}

	if (strcmp(argv[0], "-loc") == 0)
	{
		char buf[1024];
		sprintf(buf, "\xB3Location: %u, %u", m_player->m_positionX / 20, m_player->m_positionY / 20);
		m_player->SendChatMsg(buf);
	}

	if (strcmp(argv[0], "-rename") == 0)
	{
		if (argv.size() < 2 || m_player->m_namestr != "NONAME")
			return;

		static byte chars[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ ";

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		uint32 spaces = 0;
		for (std::string::iterator itr=name.begin(); itr!=name.end(); ++itr)
		{
			char c = (*itr);
			if (c == 0x20)
				spaces++;

			//check the char is valid
			for (int j=0; j < sizeof(chars)/sizeof(byte); j++)
			{
				if (chars[j] == c)
					goto next;
			}
			return;

next:
			continue;
		}

		sAccountMgr.FixName(name);

		if (spaces > 2 || sAccountMgr.PlayerExists(name))
			return;

		m_player->m_namestr = name;
		sAccountMgr.m_usednames.insert(name);
	}

	if (m_player->m_account->m_data->accountlevel == 0) //admin commands below
		return;

	if (strcmp(argv[0], "-script") == 0)
	{
		if (m_player->m_account->m_data->accountlevel < 10) //dev only command
			return;
		std::string scriptname = argv[1];

		int32 mapid = m_player->m_mapmgr->m_mapid;
		int32 tilex = m_player->m_positionX / 20;
		int32 tiley = m_player->m_positionY / 20;

		sScriptMgr.CreateTileScript(m_player->m_mapmgr, tilex, tiley, scriptname);

		//db
		sDatabase.Execute("insert into `map_scripts` values (%u, %u, %u, \"%s\");", mapid, tilex, tiley, scriptname.c_str());
	}

	if (strcmp(argv[0], "-createtele") == 0)
	{
		if (m_pastex == 0 && m_pastey == 0)
		{
			m_pastex = m_player->m_positionX;
			m_pastey = m_player->m_positionY;
			m_player->SendChatMsg("Position1Set");
		}
		else
		{
			uint32 x = m_pastex / 20;
			uint32 y = m_pastey / 20;
			m_pastex = 0; m_pastey = 0;

			uint32 key = x | (y << 16);

			Point p;
			p.x = m_player->m_positionX / 20;
			p.y = m_player->m_positionY / 20;

			TeleCoord t;
			t.loc = p;
			t.factionmask = 3;
			t.mapid = m_player->m_mapmgr->m_mapid;
			t.spelleffect = 0;

			m_player->m_mapmgr->m_telecoords.insert(std::make_pair(key, t));
			m_player->SendChatMsg("TeleLinkCreated");

			sDatabase.Execute("insert into `tele_coords` (`mapid`, `x`, `y`, `mapid2`, `x2`, `y2`, `spelleffect`, `factionmask`) VALUES (%u, %u, %u, %u, %u, %u, %u, %u);", t.mapid, x, y, t.mapid, t.loc.x, t.loc.y, 0, 3);
		}
	}

	if (strcmp(argv[0], "-deltele") == 0)
	{
		float dist = 99999.9f;
		uint32 key = 0xFFFFFFFF;
		for (auto itr = m_player->m_mapmgr->m_telecoords.begin(); itr != m_player->m_mapmgr->m_telecoords.end(); ++itr)
		{
			uint32 x = itr->first & 0xFFFF;
			uint32 y = itr->first >> 16;

			x = x * 20 + 10;
			y = y * 20 + 10;

			float tmpdist = m_player->GetDistance(x, y);

			if (tmpdist < dist)
			{
				key = itr->first;
				dist = tmpdist;
			}
		}

		if (key != 0xFFFFFFFF)
		{
			auto itr = m_player->m_mapmgr->m_telecoords.find(key);

			if (itr != m_player->m_mapmgr->m_telecoords.end())
			{
				TeleCoord & tele = itr->second;
				sDatabase.Execute("delete from `tele_coords` where `id` = %u;", tele.id);
				m_player->m_mapmgr->m_telecoords.erase(itr);
			}
		}
	}

	if (strcmp(argv[0], "-vision") == 0)
	{
		if (m_player->HasUnitFlag(UNIT_FLAG_ADMIN_VISION))
			m_player->RemoveUnitFlag(UNIT_FLAG_ADMIN_VISION);
		else
			m_player->SetUnitFlag(UNIT_FLAG_ADMIN_VISION);
	}

	if (strcmp(argv[0], "-rebuildmap") == 0)
	{
		m_player->m_mapmgr->BuildMesh(0);
	}

	if (strcmp(argv[0], "-rebuildtile") == 0)
	{
		sMeshBuilder.QueueMeshBuild(m_player->m_mapmgr, m_player->m_cell->x, m_player->m_cell->y, MESH_BUILD_PRIORITY);
	}

	if (strcmp(argv[0], "-saveall") == 0)
	{
		sAccountMgr.SaveAllPlayers();
	}

	if (strcmp(argv[0], "-savemesh") == 0)
	{
		m_player->m_mapmgr->saveAll("test.mesh", m_player->m_mapmgr->m_navMesh);
	}

	if (strcmp(argv[0], "-saveacc") == 0)
	{
		sAccountMgr.SaveAllAccounts();
	}
	if (strcmp(argv[0], "-ghost") == 0)
	{
		if (m_player->HasUnitFlag(UNIT_FLAG_IS_GHOST))
			m_player->RemoveUnitFlag(UNIT_FLAG_IS_GHOST);
		else
			m_player->SetUnitFlag(UNIT_FLAG_IS_GHOST);
	}

	if (strcmp(argv[0], "-setbonus") == 0)
	{
		if (argv.size() < 3)
			return;
		uint32 bonus = atol(argv[1]);
		uint32 val = atol(argv[2]);

		if (bonus >= NUM_ITEM_BONUSES)
			return;
		m_player->m_bonuses[bonus] = val;
		m_player->Recalculate();
	}

	if (strcmp(argv[0], "-set") == 0)
	{
		if (argv.size() < 3)
			return;

		std::string setting = argv[1];
		std::string val = argv[2];

		g_ServerSettings.UpdateSetting(setting, val);
	}

	if (strcmp(argv[0], "-settings") == 0)
	{
		InfoMessage msg;
		g_ServerSettings.BuildInfoMessage(msg);
		m_player->SendInfoMessage(msg);
	}
	
	if (strcmp(argv[0], "-cast") == 0)
	{
		if (argv.size() < 2)
			return;
		SpellEntry* sp = sSpellData.GetEntry(atol(argv[1]));
		if (sp == NULL)
			return;
		m_player->CastSpell(atol(argv[1]), true);	
	}

	if (strcmp(argv[0], "-clearcd") == 0)
	{
		m_player->m_skillcooldowns.clear();
	}

	if (strcmp(argv[0], "-setskill") == 0)
	{
		if (argv.size() < 3)
			return;
		uint32 bonus = atol(argv[1]);
		uint32 val = atol(argv[2]);

		if (bonus >= NUM_SKILLS)
			return;
		m_player->m_skills[bonus] = val;
		m_player->Recalculate();
	}


	if (strcmp(argv[0], "-setflatmod") == 0)
	{
		if (argv.size() < 3)
			return;
		uint32 mod = atol(argv[1]);
		uint32 val = atol(argv[2]);

		if (mod >= NUM_SPELL_MODS)
			return;
		m_player->AddFlatMod(0xFFFFFFFF, mod, val);
	}


	if (strcmp(argv[0], "-setpctmod") == 0)
	{
		if (argv.size() < 3)
			return;
		uint32 mod = atol(argv[1]);
		uint32 val = atol(argv[2]);

		if (mod >= NUM_SPELL_MODS)
			return;
		m_player->AddPctMod(0xFFFFFFFF, mod, val);
	}

	if (strcmp(argv[0], "-rev") == 0)
	{
		if (!m_player->IsAlive())
			m_player->Revive();
		m_player->m_bosskilledid = 0;
	}

	if (strcmp(argv[0], "-revf") == 0)
	{
		m_player->m_canResurrect = true;
		m_player->SendActionResult(PLAYER_ACTION_PRESS_F12_TO_RETURN);
		m_player->RemoveEvents(EVENT_PLAYER_RESPAWN);
	}

	if (strcmp(argv[0], "-ctrade") == 0)
	{
		m_player->CancelTrade();
	}

	if (strcmp(argv[0], "-utrade") == 0)
	{
		m_player->SendTradeUpdate();
	}

	if (strcmp(argv[0], "-aggro") == 0)
	{
		for (std::map<Unit*, uint8>::iterator itr = m_player->m_inrangeUnits.begin(); itr != m_player->m_inrangeUnits.end(); ++itr)
		{
			itr->first->m_POIObject = m_player->m_serverId;
			itr->first->m_POI = POI_ATTACKING;
		}
	}

	if (strcmp(argv[0], "-createitem") == 0)
	{
		if (argv.size() < 2)
			return;
		uint32 spawntime = 0;
		if (argv.size() >= 3)
			spawntime = atol(argv[2]);
		ItemProto* dat = sItemStorage.GetItem(atol(argv[1]));
		if (dat == NULL)
			return;
		Item* test = new Item(atol(argv[1]), m_player); //mystic staff
		if (spawntime != 0)
		{
			test->AddEvent(&Item::Delete, EVENT_ITEM_FORCEDELETE, spawntime, 1, EVENT_FLAG_NOT_IN_WORLD);
			test->m_expiretime = g_time + (spawntime / 1000);
		}
		test->Drop();
	}

	if (strcmp(argv[0], "-broadcast") == 0)
	{
		if (argv.size() < 2)
			return;
		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			std::string msg;
			for (uint32 i = 1; i < argv.size(); ++i)
			{
				msg += argv[i];
				msg += " ";
			}
			p->SendChatMsg(msg.c_str());
		}
	}

	if (strcmp(argv[0], "-setanim") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->m_unitAnimation = atol(argv[1]);
	}

	if (strcmp(argv[0], "-setmodel") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->SetModels(atol(argv[1]));
	}

	if (strcmp(argv[0], "-setweapon") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->SetModels(-1, atol(argv[1]));
	}

	if (strcmp(argv[0], "-setshield") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->SetModels(-1, -1, atol(argv[1]));
	}

	if (strcmp(argv[0], "-sethelm") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->SetModels(-1, -1, -1, atol(argv[1]));
	}

	if (strcmp(argv[0], "-spawn") == 0)
	{
		if (m_player->m_account->m_data->accountlevel < 10) //dev only command
			return;

		if (argv.size() < 2)
			return;
		uint32 entry = atol(argv[1]);
		uint32 movetype = 0;
		if (argv.size() >= 3)
			movetype = atol(argv[2]);
		CreatureProto* p = sCreatureProto.GetEntry(entry);
		if (p == NULL)
			return;
		CreatureSpawn* s = new CreatureSpawn;
		memset(s, 0, sizeof(CreatureSpawn));
		s->entry = entry;
		s->Id = sCreatureSpawn.GetMaxEntry() + 1;
		s->x = m_player->m_positionX;
		s->y = m_player->m_positionY;
		s->movetype = movetype;
		s->mapid = m_player->m_mapmgr->m_mapid;
		sCreatureSpawn.Insert(s->Id, s);
		sCreatureSpawn.Save(s);
		Creature* cre = new Creature(p, s);
		m_player->m_mapmgr->AddObject(cre);
	}

	if (strcmp(argv[0], "-spawnt") == 0)
	{
		if (m_player->m_account->m_data->accountlevel < 10) //dev only command
			return;

		if (argv.size() < 3)
			return;
		uint32 entry = atol(argv[1]);
		uint32 level = atol(argv[2]);
		uint32 movetype = 0;
		if (argv.size() >= 4)
			movetype = atol(argv[3]);
		CreatureProto* p = sCreatureProto.GetEntry(entry);
		if (p == NULL)
			return;
		CreatureSpawn* s = new CreatureSpawn;
		memset(s, 0, sizeof(CreatureSpawn));
		s->entry = entry;
		s->Id = sCreatureSpawn.GetMaxEntry() + 1;
		s->x = (m_player->m_positionX / 20) * 20 + 10;
		s->y = (m_player->m_positionY / 20) * 20 + 10;
		s->movetype = movetype;
		s->mapid = m_player->m_mapmgr->m_mapid;
		s->level = level;
		sCreatureSpawn.Insert(s->Id, s);
		sCreatureSpawn.Save(s);
		Creature* cre = new Creature(p, s);
		m_player->m_mapmgr->AddObject(cre);
	}

	if (strcmp(argv[0], "-tspawn") == 0)
	{
		if (argv.size() < 4)
			return;
		uint32 entry = atol(argv[1]);
		uint32 movetype = 0;
		uint32 level = atol(argv[2]);
		uint32 spawntime = atol(argv[3]);
		CreatureProto* p = sCreatureProto.GetEntry(entry);
		if (p == NULL)
			return;
		Creature* cre = new Creature(p, NULL);
		cre->m_positionX = m_player->m_positionX;
		cre->m_positionY = m_player->m_positionY;
		cre->m_spawnx = m_player->m_positionX;
		cre->m_spawny = m_player->m_positionY;
		cre->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, spawntime, 1, EVENT_FLAG_NOT_IN_WORLD);
		cre->SetLevel(level);
		cre->InitCreatureSpells();
		m_player->m_mapmgr->AddObject(cre);
	}

	if (strcmp(argv[0], "-mapinfo") == 0)
	{
		char buf[1024];
		int32 x = m_player->m_positionX / 20;
		int32 y = m_player->m_positionY / 20;
		ASSERT(x < 3220 && y < 3220);
		sprintf(buf, "1: %u 2: %u 3: %u 4: %u flags: %02X", m_player->m_mapmgr->m_maplayer1[x + y * 3220], m_player->m_mapmgr->m_maplayer2[x + y * 3220], m_player->m_mapmgr->m_maplayer3[x + y * 3220], m_player->m_mapmgr->m_maplayer4[x + y * 3220], m_player->m_mapmgr->m_passflags[x][y]);
		m_player->SendChatMsg(buf);
	}

	if (strcmp(argv[0], "-continent") == 0)
	{
		char buf[1024];
		sprintf(buf, "Continent type: %u", m_player->GetContinentType());
		m_player->SendChatMsg(buf);
	}

	if (strcmp(argv[0], "-loc") == 0)
	{
		char buf[1024];
		sprintf(buf, "Location: %u, %u", m_player->m_positionX / 20, m_player->m_positionY / 20);
		m_player->SendChatMsg(buf);
	}

	if (strcmp(argv[0], "-locx") == 0)
	{
		char buf[1024];
		sprintf(buf, "Location: %u, %u", m_player->m_positionX, m_player->m_positionY);
		m_player->SendChatMsg(buf);
	}

	if (strcmp(argv[0], "-seteff") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->m_spelleffect = 0;
		m_player->SetSpellEffect(atol(argv[1]));
	}

	if (strcmp(argv[0], "-setit") == 0)
	{
		if (argv.size() < 3)
			return;
		uint32 itemslot = NUM_EQUIPPED_ITEMS + atol(argv[1]);
		uint32 val = atol(argv[2]);
		if (m_player->m_items[itemslot] == NULL)
			return;
		m_player->m_items[itemslot]->m_stackcount = val;
	}

	if (strcmp(argv[0], "-ait") == 0)
	{
		if (argv.size() < 3)
			return;
		uint32 itemslot = NUM_EQUIPPED_ITEMS + atol(argv[1]);
		uint32 val = atol(argv[2]);
		if (m_player->m_items[itemslot] == NULL)
			return;
		m_player->m_items[itemslot]->m_stackcount |= val;
	}

	if (strcmp(argv[0], "-rit") == 0)
	{
		if (argv.size() < 3)
			return;
		uint32 itemslot = NUM_EQUIPPED_ITEMS + atol(argv[1]);
		uint32 val = atol(argv[2]);
		if (m_player->m_items[itemslot] == NULL)
			return;
		m_player->m_items[itemslot]->m_stackcount &= ~val;
	}

	if (strcmp(argv[0], "-aitf") == 0)
	{
		if (argv.size() < 3)
			return;
		uint32 itemslot = NUM_EQUIPPED_ITEMS + atol(argv[1]);
		uint32 val = atol(argv[2]);
		if (m_player->m_items[itemslot] == NULL)
			return;
		m_player->m_items[itemslot]->m_stackcount |= (1 << val);
	}

	if (strcmp(argv[0], "-ritf") == 0)
	{
		if (argv.size() < 3)
			return;
		uint32 itemslot = NUM_EQUIPPED_ITEMS + atol(argv[1]);
		uint32 val = atol(argv[2]);
		if (m_player->m_items[itemslot] == NULL)
			return;
		m_player->m_items[itemslot]->m_stackcount &= ~(1 << val);
	}


	if (strcmp(argv[0], "-setsf") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->m_spellflags = 0;
		m_player->SetSpellFlag(atol(argv[1]));
	}

	if (strcmp(argv[0], "-setbf") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->m_buffflags = 0;
		m_player->SetBuffFlag(atol(argv[1]));
	}

	if (strcmp(argv[0], "-setskf") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->m_skillflags = 0;
		m_player->SetSkillFlag(atol(argv[1]));
	}

	if (strcmp(argv[0], "-setname") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->m_unitspecialname = atol(argv[1]);
	}

	if (strcmp(argv[0], "-setname2") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->m_unitspecialname2 = atol(argv[1]);
	}
	
	if (strcmp(argv[0], "-invis") == 0)
	{
		if (m_player->m_invis)
			m_player->RemoveUnitFlag(UNIT_FLAG_UNATTACKABLE);
		else
			m_player->SetUnitFlag(UNIT_FLAG_UNATTACKABLE);
		m_player->m_invis = !m_player->m_invis;
		//clear spell effects
		m_player->ClearSpellEffect();
		m_player->RemoveSpellFlag(0xFFFF); //remove all spell flags
		m_player->SendHealthUpdate();
	}

	if (strcmp(argv[0], "-tele") == 0)
	{
		if (argv.size() < 3)
			return;

		int32 x = atol(argv[1]);
		int32 y = atol(argv[2]);

		int32 mapid = m_player->m_mapid;

		if (argv.size() >= 4)
			mapid = atol(argv[3]);

		if (mapid == m_player->m_mapid)
			m_player->SetPosition(x, y);
		else
		{
			m_player->m_pendingmaptransfer.newmapid = mapid;
			m_player->m_pendingmaptransfer.newx = x;
			m_player->m_pendingmaptransfer.newy = y;
			m_player->m_pendingmaptransfer.playerportal = false;
			m_player->m_mapmgr->RemoveObject(m_player);
		}
	}

	if (strcmp(argv[0], "-noitem") == 0)
	{
		if (argv.size() < 2)
			return;

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				for (int32 i = NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS - 1; i >= 0; --i)
				{
					if (p->m_items[i] == NULL)
						continue;
					p->m_items[i]->Delete();
				}
				p->SaveToDB();
			}
		}
	}

	if (strcmp(argv[0], "-acinfo") == 0)
	{
		if (argv.size() < 2)
			return;

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				char tmpbuf[1024];
				sprintf(tmpbuf, "Account Number: %u, IP: %s, LstAct: %us", p->m_account->m_data->Id, inet_ntoa(p->m_account->netinfo.sin_addr), time(NULL) - p->m_account->m_lastaction);
				m_player->SendChatMsg(tmpbuf, true);
			}
		}
	}

	if (strcmp(argv[0], "-banip") == 0)
	{
		if (argv.size() < 2)
			return;

		char* ip = argv[1];
		uint32 ipaddr = inet_addr(ip);
		sAccountMgr.AddBannedIPEntry(ipaddr, 0xFFFFFFFF);
	}

	//cidr version
	if (strcmp(argv[0], "-banipc") == 0)
	{
		if (argv.size() < 2)
			return;

		char* ip = argv[1];
		uint32 ipaddr = inet_addr(ip);
		uint32 mask = 0;
		uint32 nb = atol(argv[2]);
		for (uint32 i = 0; i < nb; ++i)
			mask |= (1 << i);
		sAccountMgr.AddBannedIPEntry(ipaddr, mask);
	}

	if (strcmp(argv[0], "-restart") == 0)
	{
		if (argv.size() > 1)
			g_restartTimer = g_time + atol(argv[1]);
		else
			g_restartEvent = true;
	}

	if (strcmp(argv[0], "-delmaps") == 0)
	{
		FILE* f = fopen("delmaps", "wb");
		if (f != NULL)
			fclose(f);
	}

	if (strcmp(argv[0], "-upmaps") == 0)
	{
		FILE* f = fopen("upmaps", "wb");
		if (f != NULL)
			fclose(f);
	}

	if (strcmp(argv[0], "-update") == 0)
	{
		FILE* f = fopen("update", "wb");
		if (f != NULL)
			fclose(f);
		if (argv.size() > 1)
			g_restartTimer = g_time + atol(argv[1]);
		else
			g_restartEvent = true;
	}

	if (strcmp(argv[0], "-login") == 0)
	{
		if (argv.size() < 2)
			return;

		uint32 id = atol(argv[1]);

		m_player->m_account->m_forcePlayerLoginIDAdmin = id;
	}

	if (strcmp(argv[0], "-ban") == 0)
	{
		if (argv.size() < 2)
			return;

		uint32 argtime = atol(argv[1]);
		char* timearg = argv[1];
		if (timearg[strlen(timearg) - 1] == 'd')
			argtime *= 60 * 60 * 24;
		if (timearg[strlen(timearg) - 1] == 'h')
			argtime *= 60 * 60;
		if (timearg[strlen(timearg) - 1] == 'm')
			argtime *= 60;

		std::string name;

		for (uint32 i = 2; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				p->m_account->m_data->banexpiretime = time(NULL) + argtime;
				sAccountData.Save(p->m_account->m_data);
				sAccountMgr.Logout(p->m_account, false);
			}
		}
	}

	if (strcmp(argv[0], "-log") == 0)
	{
		if (argv.size() < 2)
			return;

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				p->SetExtraLog();
			}
		}
	}

	if (strcmp(argv[0], "-bana") == 0)
	{
		if (argv.size() < 3)
			return;

		uint32 argtime = atol(argv[1]);
		char* timearg = argv[1];
		if (timearg[strlen(timearg) - 1] == 'd')
			argtime *= 60 * 60 * 24;
		if (timearg[strlen(timearg) - 1] == 'h')
			argtime *= 60 * 60;
		if (timearg[strlen(timearg) - 1] == 'm')
			argtime *= 60;

		uint32 accountid = atol(argv[2]);

		Account* acc = sAccountMgr.GetAccount(accountid);

		if (acc == NULL)
			return;
		acc->m_data->banexpiretime = time(NULL) + argtime;
		acc->Save();
		acc->DecRef();
	}

	if (strcmp(argv[0], "-unban") == 0)
	{
		if (argv.size() < 2)
			return;

		uint32 acctid = atol(argv[1]);
		AccountData* data = sAccountData.GetEntry(acctid);
		if (data == NULL)
			return;
		data->banexpiretime = 0;
		sAccountData.Save(data);
	}

	if (strcmp(argv[0], "-frename") == 0)
	{
		if (argv.size() < 2)
			return;

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				p->m_namestr = "NONAME";
				p->EventChatMsg(0, "You have been flagged for a rename.");
				p->EventChatMsg(3000, "Please use -rename newname to change your name, thanks.");
				p->EventChatMsg(3000, "Note: until you choose a name, you are muted.");
			}
		}
	}

	if (strcmp(argv[0], "-setlevel") == 0)
	{
		if (argv.size() < 3)
			return;

		uint32 level = atol(argv[1]);

		std::string name;

		for (uint32 i = 2; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				p->m_level = level;
				p->ResetStats();
			}
		}
	}

	if (strcmp(argv[0], "-setalign") == 0)
	{
		if (argv.size() < 3)
			return;

		uint32 level = atol(argv[1]);

		std::string name;

		for (uint32 i = 2; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;

			if (p->m_namestr == name)
			{
				p->SetAlign(level);
				p->Recalculate();
			}
		}
	}

	if (strcmp(argv[0], "-setexp") == 0)
	{
		if (argv.size() < 3)
			return;

		uint32 level = atol(argv[1]);

		std::string name;

		for (uint32 i = 2; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				p->m_experience = level;
			}
		}
	}

	if (strcmp(argv[0], "-mute") == 0)
	{
		if (argv.size() < 2)
			return;

		uint32 argtime = atol(argv[1]);
		char* timearg = argv[1];
		if (timearg[strlen(timearg) - 1] == 'd')
			argtime *= 60 * 60 * 24;
		if (timearg[strlen(timearg) - 1] == 'h')
			argtime *= 60 * 60;
		if (timearg[strlen(timearg) - 1] == 'm')
			argtime *= 60;

		std::string name;

		for (uint32 i = 2; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				char mutepostfix[256];
				uint32 mutetime = sAccountMgr.GenerateTimePostfix(argtime * 1000, mutepostfix);
				p->EventChatMsg(0, "You have been muted!");
				p->EventChatMsg(3000, "Your mute will last %u %s.", mutetime, mutepostfix);

				p->m_account->m_data->muteexpiretime = time(NULL) + argtime;
				sAccountData.Save(p->m_account->m_data);
			}
		}
	}

	if (strcmp(argv[0], "-unmute") == 0)
	{
		if (argv.size() < 2)
			return;

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				p->SendChatMsg("You have been unmuted by an admin.");
				p->m_account->m_data->muteexpiretime = 0;
				sAccountData.Save(p->m_account->m_data);
			}
		}
	}

	if (strcmp(argv[0], "-stopfollow") == 0)
	{
		m_player->m_cheatfollowobj = 0;
		m_player->RemoveUnitFlag(UNIT_FLAG_INVISIBLE);
	}

	if (strcmp(argv[0], "-follow") == 0)
	{
		if (argv.size() < 2)
			return;

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name && !p->m_invis)
			{
				m_player->m_cheatfollowobj = p->m_serverId;
				m_player->SetUnitFlag(UNIT_FLAG_INVISIBLE);
			}
		}
	}

	if (strcmp(argv[0], "-masssummon") == 0)
	{
		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
			itr->second->AddEvent(&Object::SetPosition, (int32)m_player->m_positionX, (int32)m_player->m_positionY, EVENT_UNIT_SETPOSITION, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	if (strcmp(argv[0], "-summon") == 0)
	{
		if (argv.size() < 2)
			return;

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				p->AddEvent(&Unit::SetPosition, (int32)m_player->m_positionX, (int32)m_player->m_positionY, EVENT_UNIT_SETPOSITION, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
		}
	}

	if (strcmp(argv[0], "-teleu") == 0)
	{
		if (argv.size() < 2)
			return;

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name && !p->m_invis)
				m_player->SetPosition(p->m_positionX, p->m_positionY);
		}
	}

	if (strcmp(argv[0], "-setmodel") == 0)
	{
		if (argv.size() < 1)
			return;
		m_player->SetModels(atol(argv[1]));
	}

	if (strcmp(argv[0], "-reload") == 0)
	{
		std::string reloadtype = "all";
		if (argv.size() >= 2)
			reloadtype = argv[1];

		if (reloadtype == "all" || reloadtype == "settings")
		{
			g_ServerSettings.Load();
		}

		if (reloadtype == "all" || reloadtype == "quests")
		{
			sQuests.Reload();
			sQuestText.Reload();
			sQuestLocs.Reload();
			sCreatureQuestGivers.Reload();
		}

		if (reloadtype == "all" || reloadtype == "creatures")
		{
			sCreatureProto.Reload();
			sCreatureEquipment.Reload();
			sAccountMgr.LoadUsedNPCNames();
		}

		if (reloadtype == "all" || reloadtype == "spells")
		{
			sCreatureSpells.Reload();
			sSpellData.Reload();
			sSpellAuras.Reload();
			
			//reload all creature spells
			for (unordered_map<uint16, Object*>::iterator itr = m_player->m_mapmgr->m_objectmap.begin(); itr != m_player->m_mapmgr->m_objectmap.end(); ++itr)
			{
				if (itr->second->IsCreature())
					TO_CREATURE(itr->second)->InitCreatureSpells();
			}
		}

		if (reloadtype == "all" || reloadtype == "loot")
		{
			sItemStorage.m_lootmap.clear();
			sItemStorage.LoadLoot();
		}	
	}

	if (strcmp(argv[0], "-delete") == 0)
	{
		m_player->m_session->m_commandflags |= COMMAND_FLAG_DELETE_CREATURE;
	}

	if (strcmp(argv[0], "-trade") == 0)
	{
		if (m_player->m_tradetarget)
			m_player->CancelTrade();

		ServerPacket data(MSG_CLIENT_INFO, 29);
		data << uint32(29);
		data << m_player->m_namestr;
		m_player->Send(&data);
		m_player->m_tradetarget = m_player->m_playerId;
		m_player->m_lastSay == "N/A";
	}

	if (strcmp(argv[0], "-clevel") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->m_session->m_commandflags |= COMMAND_FLAG_SET_CREATURE_LEVEL;
		m_player->m_session->m_commandarg = atol(argv[1]);
	}

	if (strcmp(argv[0], "-cflag") == 0)
	{
		if (argv.size() < 2)
			return;
		m_player->m_session->m_commandflags |= COMMAND_FLAG_SET_CREATURE_LEVEL;
		m_player->m_session->m_commandarg = atol(argv[1]);
	}

	if (strcmp(argv[0], "-cinfo") == 0)
	{
		m_player->m_session->m_commandflags |= COMMAND_FLAG_GET_CREATURE_INFO;
	}

	if (strcmp(argv[0], "-respawn") == 0)
	{
		for (unordered_map<uint16, Object*>::iterator itr = m_player->m_mapmgr->m_objectmap.begin(); itr != m_player->m_mapmgr->m_objectmap.end(); ++itr)
		{
			Object* o = itr->second;
			if (!o->IsCreature())
				continue;
			Creature* cre = TO_CREATURE(o);
			if (cre->m_spawn == NULL)
				continue;
			CreatureSpawn* spawn = cre->m_spawn;
			CreatureProto* proto = cre->m_proto;
			Creature* newcre = new Creature(proto, spawn);
			m_player->m_mapmgr->AddObject(newcre);
			cre->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
		}
	}

	if (strcmp(argv[0], "-despawn") == 0)
	{
		for (unordered_map<uint16, Object*>::iterator itr = m_player->m_mapmgr->m_objectmap.begin(); itr != m_player->m_mapmgr->m_objectmap.end(); ++itr)
		{
			Object* o = itr->second;
			if (!o->IsCreature())
				continue;
			Creature* cre = TO_CREATURE(o);
			if (cre->m_spawn == NULL)
				cre->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
		}
	}

	if (strcmp(argv[0], "-pinfo") == 0)
	{
		if (argv.size() < 2)
			return;

		std::string name;

		for (uint32 i = 1; i < argv.size(); ++i)
		{
			name += argv[i];
			if (i != argv.size() - 1)
				name += " ";
		}

		sAccountMgr.FixName(name);

		for (unordered_map<uint32, Player*>::iterator itr = m_player->m_mapmgr->m_playerStorage.begin(); itr != m_player->m_mapmgr->m_playerStorage.end(); ++itr)
		{
			Player* p = itr->second;
			if (p->m_namestr == name)
			{
				InfoMessage msg;
				msg.AddMessage("Player %s:", p->m_namestr.c_str());
				msg.AddMessage("Melee damage: %u", p->GetMeleeDamage());
				msg.AddMessage("Health: %u / %u", p->m_health, p->m_maxhealth);
				msg.AddMessage("Skills:");

				for (uint32 i = 0; i < NUM_SKILLS; ++i)
				{
					if (p->m_skills[i] == 0)
						continue;
					SkillEntry* skill = sSkillStore.GetEntry(i);
					if (skill == NULL)
						continue;
					msg.AddMessage("%u - %s", p->m_skills[i], skill->name);
				}

				m_player->SendInfoMessage(msg);
			}
		}
	}

}

void Session::HandleInterface(ServerPacket &data)
{
	uint16 id, zeros;
	uint32 interactiontype;

	data >> id >> zeros >> interactiontype;

	sLog.Debug("Interact", "Interaction %u", interactiontype);

	switch (interactiontype)
	{
	case 10: //set color 1
		{
			uint32 color;
			data >> color;
			sLog.Debug("COLOR1", "%u", color);
			m_player->SetModels(-1, -1, -1, -1, color);
		} break;
	case 11: //set color 2
		{
			uint32 color;
			data >> color;
			m_player->SetModels(-1, -1, -1, -1, -1, color);
		} break;
	case 1: //switch weapons
		{
			m_player->SwitchWeapons();
		} break;
	case 2: //increase stat
		{
			if (m_player->m_points == 0)
				return;
			uint32 stat;
			data >> stat;
			switch (stat)
			{
			case 1: ++m_player->m_strength; break;
			case 2: ++m_player->m_agility; break;
			case 3: ++m_player->m_constitution; break;
			case 4: ++m_player->m_intellect; break;
			case 5: ++m_player->m_wisdom; break;
			}
			--m_player->m_points;
			m_player->OnStatsChange();
			m_player->Recalculate();
			m_player->SendAllStats();
		} break;
	case 3: //increase skill point lol
		{
			//just for now do no checks
			uint32 skillid;
			data >> skillid;

			if (skillid >= NUM_SKILLS)
				return;
			SkillEntry* skill = sSkillStore.GetEntry(skillid);

			if (skill == NULL)
				return;

			if (skill->serverindex > 60)
			{
				if (m_player->m_pvppoints == 0)
					return;
			}
			else if (m_player->m_skillpoints == 0)
				return;

			uint32 maxpoints = 0;
			switch (skill->scalebase)
			{
			case SKILL_SCALE_NONE: maxpoints = skill->basepoints; break;
			case SKILL_SCALE_STRENGTH: maxpoints = skill->basepoints + (m_player->m_strength / 10); break;
			case SKILL_SCALE_AGILITY: maxpoints = skill->basepoints + (m_player->m_agility / 10); break;
			case SKILL_SCALE_CONSTITUTION: maxpoints = skill->basepoints + (m_player->m_constitution / 10); break;
			case SKILL_SCALE_INTELLIGENCE: maxpoints = skill->basepoints + (m_player->m_intellect / 10); break;
			case SKILL_SCALE_WISDOM: maxpoints = skill->basepoints + (m_player->m_wisdom / 10); break;
			case SKILL_SCALE_WISDOM_INTELLIGENCE: maxpoints = skill->basepoints + ((m_player->m_wisdom + m_player->m_intellect) * 0.5333333 / 8); break;
			case SKILL_SCALE_STRENGTH_AGILITY: maxpoints = skill->basepoints + ((m_player->m_strength + m_player->m_agility) * 0.5333333 / 8); break;
			}

			if (m_player->m_skills[skillid] >= maxpoints)
				return;

			//previous requirement
			if (skill->reqskill)
			{
				if (skill->reqskill == 42) //dummy: total resists
				{
					uint32 totalresist = m_player->GetSkillValue(SKILL_RESIST_FIRE);
					totalresist += m_player->GetSkillValue(SKILL_RESIST_ICE);
					totalresist += m_player->GetSkillValue(SKILL_RESIST_ENERGY);
					totalresist += m_player->GetSkillValue(SKILL_RESIST_SHADOW);
					totalresist += m_player->GetSkillValue(SKILL_RESIST_SPIRIT);
					if (totalresist < skill->reqskillvalue)
						return;
				}
				else if (m_player->m_skills[skill->reqskill] < skill->reqskillvalue)
					return;
			}

			m_player->ModSkill(skillid, 1);
			m_player->Recalculate();
			m_player->SendAllStats();

		} break;
	case 4: //request name of unit
		{
			uint16 targetid;
			data >> targetid;

			if (m_player->m_mapmgr == NULL)
				return;

			Object* obj = m_player->m_mapmgr->GetObject(targetid);
			if (obj == NULL || !obj->IsUnit())
				return;

			if (m_player->m_account->m_data->accountlevel == 0 && (TO_UNIT(obj)->HasAura(SPELL_AURA_INVISIBILITY) || TO_UNIT(obj)->HasAura(SPELL_AURA_HIDE) || TO_UNIT(obj)->HasAura(SPELL_AURA_MUTATIO_NIMBUS) || TO_UNIT(obj)->HasAura(SPELL_AURA_MUTATIO_FLAMMA)))
				return;

			if (obj->IsPlayer() && TO_PLAYER(obj)->m_invis)
				return;

			const char* name = TO_UNIT(obj)->m_namestr.c_str();
			char guild[1024];

			ServerPacket p(SMSG_NAME_RESPONSE, 0x49);
			if (obj->IsCreature() && (TO_CREATURE(obj)->m_proto->boss == 1 || TO_UNIT(obj)->HasUnitFlag(UNIT_FLAG_SHOW_HEALTH_PCT)))
			{
				double hppct = (double(TO_UNIT(obj)->m_health) / TO_UNIT(obj)->m_maxhealth);
				hppct *= 100;
				sprintf(guild, "%3.1f%%", hppct);
			}
			else if (obj->IsPet())
			{
				Unit* owner = TO_CREATURE(obj)->GetOwner();
				if (owner == m_player)
					sprintf(guild, "%u / %u", TO_UNIT(obj)->m_health, TO_UNIT(obj)->m_maxhealth);
				else if (owner != NULL)
					strcpy(guild, owner->m_namestr.c_str());
			}
			else if (obj->IsCreature())
			{
				if (TO_CREATURE(obj)->m_proto->boss == 2)
					strcpy(guild, "Guard");
				else if (TO_CREATURE(obj)->m_proto->boss == 3 || TO_CREATURE(obj)->m_proto->boss == 4)
					strcpy(guild, "Merchant");
				else
					guild[0] = 0;
			}
			else
				guild[0] = 0;

			p.Write((uint8*)name, strlen(name));
			p.WriteTo(22);
			p.Write((uint8*)guild, strlen(guild));

			p.WriteTo(43);
			if (obj->IsPlayer() && TO_PLAYER(obj)->m_alignment >= 0)
			{
				if (TO_PLAYER(obj)->m_faction == FACTION_GOOD)
					p << uint8(0x40 | TO_UNIT(obj)->m_unitspecialname); //jeloc
				else
					p << uint8(0x80 | TO_UNIT(obj)->m_unitspecialname); //vel
			}
			else if (obj->IsUnit()) //unit, not player
				p << TO_UNIT(obj)->m_unitspecialname;
			if (obj->IsUnit())
				p << TO_UNIT(obj)->m_unitspecialname2;
			m_player->Send(&p);
		} break;
	case 6: //logout
		{
			if (m_player->InPVPAttackMode() && !m_player->ProtectedByGround())
			{
				m_player->SendActionResult(PLAYER_ACTION_CANT_LOGOFF_IN_BATTLE);
				return;
			}

			if (!m_player->IsAlive())
			{
				m_player->SendChatMsg("Cannot log off while dead");
				return;
			}

			sAccountMgr.Logout(m_player->m_account, false);
			m_player->m_account->AddEvent(&Account::Logout, true, EVENT_ACCOUNT_LOGOFF_TIMEOUT, 10000, 1, 0);
			break;
		}
	case 7: //stop action
		{
			m_player->StopAction();
		} break;
	}
}

void Session::HandlePlayerUpdate(ServerPacket &data)
{
	uint16 ID;
	uint16 requesttype;

	data >> ID >> requesttype;

	switch (requesttype)
	{
	case 9: //spell list
		{
			//test this
			/* 2D 06 00 00 00 00 00 07 00 01 00 04 00 00 00 00 00 */
			ServerPacket p(MSG_CLIENT_INFO);
			p << uint16(0x10) << uint32(0);
			p << uint16(m_player->m_class) << uint16(m_player->m_level + m_player->m_bonuses[BONUS_ABILITY_LEVEL]) << uint16(m_player->m_mana);
			p << uint8(0); //star data, 0xFF = has star
			p << uint8(0); //unk
			p << uint16(0);
			m_player->Send(&p);
		} break;
	case 6: //equip item
		{
			uint16 slot;
			data >> slot;

			if (slot >= NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS)
				return;

			if (m_player->m_tradetarget)
				m_player->CancelTrade();

			if (m_player->m_items[NUM_EQUIPPED_ITEMS + slot] != NULL)
				m_player->m_items[NUM_EQUIPPED_ITEMS + slot]->Equip();
		} break;
	case 5: //uequip item
		{
			uint16 slot;
			data >> slot;

			if (slot >= NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS)
				return;

			if (m_player->m_tradetarget)
				m_player->CancelTrade();

			if (m_player->m_items[slot] != NULL)
				m_player->m_items[slot]->UnEquip();
		} break;
	case 7: //drop item
		{
			uint16 slot;
			uint16 zeros;
			uint32 dropnum;
			data >> slot >> zeros >> dropnum;

			if (slot >= NUM_INVENTORY_ITEMS)
				return;

			if (m_player->m_tradetarget)
				m_player->CancelTrade();

			if (m_player->m_items[NUM_EQUIPPED_ITEMS + slot] != NULL)
			{
				LOG_ACTION_U(m_player, "DROPITEM", "Dropping %s (itemid:%llu)", m_player->m_items[NUM_EQUIPPED_ITEMS + slot]->m_data->name, m_player->m_items[NUM_EQUIPPED_ITEMS + slot]->m_itemguid);
				LOG_ACTION_I(m_player->m_items[NUM_EQUIPPED_ITEMS + slot], "Dropped by %s", m_player->m_namestr.c_str());
				m_player->m_items[NUM_EQUIPPED_ITEMS + slot]->Drop(dropnum);
			}
		} break;
	case 9002: //start trade
		{
			m_player->InitiateTrade();
		} break;
	case 9001: //refresh trade
		{
			m_player->SendTradeUpdate();
		} break;
	case 9000:
		{
			//add a trade item
			uint16 slot, amount;
			data >> slot >> amount;

			if (slot >= NUM_INVENTORY_ITEMS)
				return;

			if (m_player->m_tradetarget)
			{
				m_player->AddTradeItem(slot + NUM_EQUIPPED_ITEMS);
			}
		} break;
	case 10: //cancel trade
		{
			m_player->CancelTrade();
		} break;
	case 11: //accept trade
		{
			m_player->AcceptTrade();
		} break;
	case 16: //open storage
		{
			m_player->OpenStorage();

			LOG_ACTION_U(m_player, "STORAGE", "Opened storage");
		} break;
	case 17: //put item into storage
		{
			uint16 slot, amount;

			data >> slot >> amount;

			if (slot >= NUM_INVENTORY_ITEMS)
				return;

			Item* it = m_player->m_items[NUM_EQUIPPED_ITEMS + slot];

			if (it == NULL)
				return;

			uint32 storageslot = 0xFFFFFFFF;

			//find a free storage slot
			if (sItemStorage.IsStackable(it->m_data->type))
				storageslot = m_player->FindFreeStorageSlot(it->m_data->id, amount);
			else
				storageslot = m_player->FindFreeStorageSlot();

			if (storageslot == 0xFFFFFFFF) //couldn't find free slot
				return;

			it->MoveItem(storageslot, amount);

			LOG_ACTION_U(m_player, "STORAGE", "Moved %s (itemid:%llu) to storage, new slot %u(amount %u)", it->m_data->name, it->m_itemguid, storageslot, amount);
			LOG_ACTION_I(it, "Moved into storage of %s (slot %u amount %u)", m_player->m_namestr.c_str(), storageslot, amount);

		} break;
	case 18: //take item from storage
		{
			uint16 slot, amount;
			data >> slot >> amount;

			if (slot >= NUM_STORAGE_ITEMS)
				return;

			Item* it = m_player->m_items[NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS + slot];

			if (it == NULL)
				return;

			uint32 invslot = 0xFFFFFFFF;

			//find a free storage slot
			if (sItemStorage.IsStackable(it->m_data->type))
				invslot = m_player->FindFreeInventorySlot(it->m_data->id, amount);
			else
				invslot = m_player->FindFreeInventorySlot();

			if (invslot == 0xFFFFFFFF) //couldn't find free slot
				return;

			it->MoveItem(invslot, amount);
			LOG_ACTION_U(m_player, "STORAGE", "Moved %s (itemid:%llu) from storage, new slot %u (amount %u)", it->m_data->name, it->m_itemguid, invslot, amount);
			LOG_ACTION_I(it, "Moved out of storage of %s (slot %u amount %u)", m_player->m_namestr.c_str(), invslot, amount);
		} break;
	case 1: //request items
		{
			m_player->SendItems();
		} break;
	case 2:
		{
			m_player->SendSkills();
		} break;
	case 3:
		{
			m_player->SendExperience();
		} break;
	case 4:
		{
			m_player->SendStats();
		} break;
	}
}


void Session::HandleCastSpell( ServerPacket & data )
{
	uint32 id;
	uint16 targettype;
	uint16 spellid;
	uint16 x, y;
	data >> id >> targettype >> spellid >> x >> y;

	if (m_player->m_extralog)
	{
		LOG_ACTION_U(m_player, "EXTRALOG", "Cast: spellid %u targettype %u arg1 %u arg2 %u", spellid, targettype, x, y);
		LOG_ACTION_U(m_player, "CAST", "Cast: spellid %u targettype %u arg1 %u arg2 %u", spellid, targettype, x, y);
	}

	SpellEntry* sp = sSpellData.GetEntry(spellid);
	if (sp == NULL)
	{
		sLog.Debug("Session", "Tried to cast unknown spell %u", spellid);
		return;
	}
	TargetData targets;
	memset(&targets, 0, sizeof(TargetData));

	switch (targettype)
	{
	case 0x50: targets.flags |= SPELL_TARGET_UNIT; break;
	case 0x4E: targets.flags |= SPELL_TARGET_SELF; break;
	case 0x4C: targets.flags |= SPELL_TARGET_GROUND; break;
	case 0x31:
	case 0x32:
	case 0x33:
	case 0x34:
	case 0x35:
	case 0x36:
	case 0x37:
	case 0x38:
	case 0x39:
	case 0x3A: targets.flags |= SPELL_TARGET_SELF | SPELL_TARGET_GROUND; targets.mark = targettype - 0x30; break;
	}
	targets.x = x;
	targets.y = y;

	//EJ's stupid 2 spell queue
	if (!m_player->IsAlive() || m_player->m_nextspell > GetMSTime() || m_player->m_nextspellid != 0)
		m_player->SetNextSpell(spellid, targets, false);
	else
	{
		Spell s(m_player, spellid, targets);
		s.Cast();
	}
}

void Session::HandleUseItem(ServerPacket & data)
{
	if (!m_player->IsAlive())
		return;

	uint32 id;
	uint16 targettype;
	uint16 itemid;
	uint16 x, y;
	data >> id >> targettype >> itemid >> x >> y;

	if (m_player->HasAura(SPELL_AURA_MUTATIO_NIMBUS) || m_player->HasAura(SPELL_AURA_MUTATIO_FLAMMA))
		return;

	Item* itm = m_player->FindInventoryItem(itemid);
	if (itm == NULL)
		return;

	ItemSpell* it = sItemSpells.GetEntry(itemid);
	if (it == NULL)
		return;

	//NOT YET FULLY IMPLEMENTED - SPELL QUEUE NEEDS REWRITE
	for (int i = 0; i < 5; ++i)
	{	
		SpellEntry* sp = sSpellData.GetEntry(it->spellid[i]);
		if (sp == NULL)
			continue;
		TargetData targets;
		memset(&targets, 0, sizeof(TargetData));

		switch (targettype)
		{
		case 0x50: targets.flags |= SPELL_TARGET_UNIT; break;
		case 0x4E: targets.flags |= SPELL_TARGET_SELF; break;
		case 0x4C: targets.flags |= SPELL_TARGET_GROUND; break;
		case 0x31:
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		case 0x3A: targets.flags |= SPELL_TARGET_SELF | SPELL_TARGET_GROUND; targets.mark = targettype - 0x30; break;
		}
		targets.x = x;
		targets.y = y;

		//EJ's stupid 2 spell queue
		if (m_player->m_nextspell > GetMSTime() || m_player->m_nextspellid != 0)
			m_player->SetNextSpell(sp->id, targets, true);
		else
		{
			Spell s(m_player, sp->id, targets, true);
			s.m_forcedvalue = it->value[i];
			s.Cast();
		}
	}

	itm->OnUse();
}

void Session::HandleKeyPress( ServerPacket & data )
{
	uint16 charid, zeros, type;
	data >> charid >> zeros >> type;

	if (m_player->m_canResurrect)
	{
		if (m_player->m_bosskilledid != 0)
		{
			Object* boss = m_player->m_mapmgr->GetObject(m_player->m_bosskilledid);
			if (boss != NULL && boss->IsUnit() && TO_UNIT(boss)->IsAlive() && TO_UNIT(boss)->IsAttacking())
				return;
		}
		m_player->m_bosskilledid = 0;
		m_player->Revive();
	}
}

void Session::HandleItemQuery( ServerPacket & data )
{
	uint32 charid;
	uint16 unk, itemid;

	data >> charid >> unk >> itemid;

	ItemProto* item = sItemStorage.GetItem(itemid);

	if (item == NULL)
		return;

	ServerPacket p(SMSG_QUERY_ITEM_RESPONSE, 200);
	p << itemid;
	p << item->name;

	p.WriteTo(27);
	p << uint8(item->levelreq); //level
	p << uint8(item->strreq); //str
	p << uint8(0);
	p << uint8(item->intreq); //int
	p << uint8(item->wisreq); //wis
	p << uint8(0);
	p << uint8(0);

	for (uint32 i = 0; i < 10; ++i)
		p << uint8(item->bonuses[i]);
	for (uint32 i = 0; i < 10; ++i)
		p << uint8(item->bonusvalues[i]);
	m_player->Send(&p);
}

//Evidyon-specific handlers
void Session::HandleEvidyonNameRequest(ServerPacket & data)
{
	uint16 targetID;
	data >> targetID;

	bool shouldSendName = true;

	Object* obj = m_player->m_mapmgr->GetObject(targetID);
	if (obj == NULL || !obj->IsUnit())
		return;

	if (m_player->m_account->m_data->accountlevel == 0 && (TO_UNIT(obj)->HasAura(SPELL_AURA_INVISIBILITY) || TO_UNIT(obj)->HasAura(SPELL_AURA_HIDE)))
		shouldSendName = false;

	if (obj->IsPlayer() && TO_PLAYER(obj)->m_invis)
		shouldSendName = false;

	//const char* name = TO_UNIT(obj)->m_namestr.c_str();

	string name;
	if(shouldSendName)
	{
		name = TO_UNIT(obj)->m_namestr;
	}
	else
	{
		name = "<unknown>";
	}

	ServerPacket p(SMSG_EVID_NAME_RESPONSE);

	p << targetID << name;
	m_player->Send(&p);
}