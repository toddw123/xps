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

#ifndef __CREATURE_H
#define __CREATURE_H

#pragma pack(push, 1)
struct CreatureSpawn
{
	uint32 Id;
	uint32 entry;
	uint32 x;
	uint32 y;
	uint32 mapid;
	uint32 movetype;
	uint32 level;
	int32 inherited;
	uint32 nextspawn;
};

struct CreatureProto
{
	uint32 entry;
	char* name;
	uint32 displayid;
	uint32 level;
	uint32 health;
	uint32 faction;
	uint32 meleedamage;
	uint32 speed;
	uint32 exp;
	uint32 spellset;
	uint32 weapondisplay;
	uint32 equipment;
	uint32 color1;
	uint32 color2;
	float minlootquality;
	float maxlootquality;
	uint32 lootentry;
	uint32 respawntime;
	uint32 invisblock;
	uint32 boss;
	uint32 cancharm;
	uint32 scriptid;
	uint32 unitflags;
	uint32 damagebonus;
	uint32 speedbonus;
};

struct CreatureEquipment
{
	uint32 entry;
	uint32 items[5];
};

struct CreatureSpells
{
	uint32 id;
	uint32 spells[5];
	uint32 cooldowns[5];
};

struct CreatureQuestGiver
{
	uint32 entry;
	uint32 quests[5];
};

struct QuestEntry
{
	uint32 id;
	uint32 flags;
	uint32 reqquest;
	uint32 reqlevel;
	uint32 maxlevel;
	uint32 reqmob;
	uint32 reqcount;
	uint32 exp;
	uint32 rewardchance;
	uint32 rewards[5];
	char* startkeyword;
	uint32 locs[5];
};

struct QuestText
{
	uint32 id;
	char* offertext[3];
	char* starttext[3];
	char* endtext[3];
};

struct QuestLoc
{
	uint32 id;
	uint32 x;
	uint32 y;
	uint32 x2;
	uint32 y2;
};
#pragma pack(pop)

extern SQLStore<CreatureProto> sCreatureProto;
extern SQLStore<CreatureSpells> sCreatureSpells;
extern SQLStore<CreatureSpawn> sCreatureSpawn;
extern SQLStore<CreatureQuestGiver> sCreatureQuestGivers;
extern SQLStore<QuestEntry> sQuests;
extern SQLStore<QuestText> sQuestText;
extern SQLStore<QuestLoc> sQuestLocs;
extern SQLStore<CreatureEquipment> sCreatureEquipment;

enum CreatureMoveType
{
	MOVETYPE_NONE,
	MOVETYPE_RANDOM,
};

struct CreatureSpell
{
	uint32 spellid;
	uint32 cooldown;
};

enum PetStates
{
	PET_STATE_FOLLOW,
	PET_STATE_BATTLE,
	PET_STATE_STOP,
};

struct WayPoint
{
	uint16 x;
	uint16 y;
	uint32 waittime;
};

enum WayPointType
{
	WAYPOINT_TYPE_NORMAL,
	WAYPOINT_TYPE_CIRCLE,
};

class CreatureAIScript;
class Creature : public Unit
{
public:
	Creature(CreatureProto* proto, CreatureSpawn* spawn);

	void InitCreatureSpells();
	~Creature();

	uint16 m_spawnx;
	uint16 m_spawny;
	float m_spawno;
	CreatureProto* m_proto;
	CreatureSpawn* m_spawn;
	std::vector<CreatureSpell> m_knownspells;
	std::map<uint32, uint32> m_spellcooldownmap;
	std::set<uint16> m_damagedunits; //for bosses, people who have damaged him
	std::set<uint32> m_killedplayers; //for bosses, people who have been killed by him (can't damage him until reset)
	uint32 m_randommovetimer;
	bool m_invis;
	bool m_norespawn;
	bool m_ispet;
	uint8 m_petstate;
	bool m_summoned;
	uint16 m_petownerid;
	std::vector<WayPoint> m_waypointMap;
	int32 m_currentwaypoint;
	uint8 m_waypointDirection;
	uint8 m_waypointType;
	CreatureQuestGiver* m_quests;
	uint32 m_questplrid; //current player I'm offering to
	uint32 m_questofferid; //current quest I'm offering
	std::set<uint32> m_blockedquestplayers; //dont offer quests to these people
	uint32 m_randmovex;
	uint32 m_randmovey;

	CreatureAIScript* m_script;

	Unit* GetOwner();
	void SetAsPet(uint16 ownerid, bool summoned = false, uint8 petstate = PET_STATE_FOLLOW);
	void MoveToNextWayPoint()
	{
		if (m_waypointDirection == 0)
			++m_currentwaypoint;
		else
			--m_currentwaypoint;
	}

	void AddWayPoint(int32 x, int32 y, uint32 waittime);
	void AddWayPointT(int32 x, int32 y, uint32 waittime) { AddWayPoint(x * 20 + 10, y * 20 + 10, waittime); }
	void EventAddWayPointT(uint32 delay, int32 x, int32 y, uint32 waittime);
	void ClearWayPoints() { m_currentwaypoint = 0; m_waypointMap.clear(); }

	void Update();

	void OnInactive();
	void OnHealthChange(uint32 old);

	bool IsBoss();

	void Respawn();

	Item* GenerateLootItem(float minqual, float maxqual, bool norandomroll = false, Player* personal_for = NULL);

	uint32 GetMeleeDamage();
	void SetSpawn(int32 x, int32 y) { m_spawnx = x; m_spawny = y; }
	void SetSpawnT(int32 x, int32 y) { SetSpawn(x * 20 + 10, y * 20 + 10); }

	void SetLevel(uint32 level)
	{
		uint32 oldlevel = m_level;
		if (level == 0)
			level = 1;
		m_level = level;

		m_bonuses[PROTECTION_ARMOR_BONUS] -= oldlevel * 3;
		m_bonuses[PROTECTION_ARMOR_BONUS] += level * 3;

		if (m_proto != NULL && m_proto->boss == 1)
			return;
		m_maxhealth = m_proto->health * float(m_level / m_proto->level);
		SetHealth(m_maxhealth);
	}

	void HandleSpellAI();
	bool CanCast(uint32 spellid, bool triggered = false);

	bool HasSpell(uint32 spellid);

	void AddSpellCooldown(uint32 spellid, uint32 cooldown = 0)
	{
		//find it
		CreatureSpell* cresp = NULL;
		for (uint32 i = 0; i < m_knownspells.size(); ++i)
		{
			if (m_knownspells[i].spellid == spellid)
			{
				cresp = &m_knownspells[i];
				break;
			}
		}

		if (cresp == NULL)
			return;

		uint32 cooldowntime;
		if (cooldown != 0)
			cooldowntime = GetMSTime() + cooldown;
		else
			cooldowntime = GetMSTime() + cresp->cooldown;

		std::map<uint32, uint32>::iterator itr = m_spellcooldownmap.find(spellid);
		if (itr == m_spellcooldownmap.end())
			m_spellcooldownmap.insert(std::make_pair(spellid, cooldowntime));
		else
			itr->second = cooldowntime;
	}

	void Invis()
	{
		m_invis = true;
		UpdateInRangeSet();
	}

	void Uninvis();

	void CreateWeapon();

	void OnPreRemoveFromWorld();
	void OnAddToWorld();

	Unit* FindTarget();

	void OnCombatStart();

	void AwardExp(bool samefaction = false);
	void AwardItem(Item* it, Player* plr);

	void OnActive();

	void RemoveCreature();

	Player* GetRandomPlayer(bool allowsamefaction = true);

	void UpdateQuests();
	void UpdateQuestOffer();

	void QuestGiveComplete() 
	{
		RemoveUnitFlag(UNIT_FLAG_INACTIVE_QUESTGIVER);
		m_questplrid = 0;
	}

	void SetQuestOfferData(uint32 questid, uint32 playerid)
	{
		m_questofferid = questid;
		m_questplrid = playerid;
	}

	void RetractQuestOffer();

	void GiveQuestReward(uint32 questid, uint32 playerid);

	void GivePlayerQuest(uint32 questid, uint32 playerid)
	{
		Player* p = m_mapmgr->GetPlayer(playerid);
		QuestEntry* quest = sQuests.GetEntry(questid);

		if (p == NULL || quest == NULL)
			return;
		p->m_currentquest = quest;
		p->m_currentquestcount = 0;

		//remove quest ui from EVERYTHING in area of player
		p->SendQuestInfoOfAll(0);		
	}

	void ConvertQuestText(Player* p, std::string & str)
	{
		replace(str, "<NAME>", p->m_namestr.c_str(), 0);
	}

	void BlockPlayerQuestOffers(uint32 plrid);

	void RemoveBlockedPlayer(uint32 plrid)
	{
		m_blockedquestplayers.erase(plrid);
	}

	void SendStorageList(Player* to);

	void AwardBossLoot();

	void OnPOIFail(uint32 reason);

	void UpdatePOI(uint32 forcedpoi = 0xFFFFFFFF);

	void IdlePOIHandler();

	Object* GetPOIObject();


	bool m_incombat;

	void OnPOIChange(uint32 newpoi);

	void OnAddInRangeObject(Object* o);

	void SendPlayerQuestInfo( Player* plr );
	void SendPlayerQuestInfo( Player* plr, uint32 type );
};

#endif
