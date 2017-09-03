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

#ifndef __PLAYER_H
#define __PLAYER_H

enum PlayerCurrency
{
	CURRENCY_MONEY,
	CURRENCY_FIERCE_USES,
	CURRENCY_LINK_ID,
	CURRENCY_LINK_IS_OWNER,
	NUM_CURRENCIES,
};

enum Classes
{
	CLASS_FIGHTER			= 0,
	CLASS_RANGER			= 1,
	CLASS_PALADIN			= 2,
	CLASS_CLERIC			= 3,
	CLASS_WIZARD			= 4,
	CLASS_WARLOCK			= 5,
	CLASS_WITCH				= 6,
	CLASS_DRUID				= 7,
	CLASS_DARKWAR			= 8,
	CLASS_BARBARIAN			= 9,
	CLASS_ADEPT				= 10,
	CLASS_DARK_PALADIN		= 11,
	CLASS_DIABOLIST			= 12,
	NUM_CLASSES,
};

enum Races
{
	RACE_DREG			= 0,
	RACE_ELF			= 1,
	RACE_GNOME			= 2,
	RACE_SCALLION		= 3,
	RACE_HUMAN			= 4,
	RACE_BEHOCK			= 5,
	RACE_SOMNIX			= 6,
	RACE_DARK_ELF		= 7,
	RACE_LURKER			= 8,
	RACE_HALF_ORC		= 9,
	RACE_MINION			= 10,
	NUM_RACES,
};

enum Gender
{
	GENDER_OTHER			= 0,
	GENDER_MALE				= 1,
	GENDER_FEMALE			= 2,
	GENDER_DUMMY_FEMALEMAGE	= 3,
	NUM_GENDERS,
};

enum Path
{
	PATH_LIGHT		= 0,
	PATH_DARKNESS	= 1,
	NUM_PATHS,
};

enum SpellModifiers
{
	SPELL_MOD_CAST_TIME,
	SPELL_MOD_DAMAGE,
	NUM_SPELL_MODS,
};

#define NUM_SPELL_GROUPS 32

static float g_healthPercentByClass[NUM_CLASSES] = 
{
	1.08f, //fighter
	0.60f, //ranger
	1.0f, //paladin
	0.65f, //cleric
	0.60f, //wizard
	0.65f, //warlock
	0.65f, //witch
	0.75f, //druid
	0.85f, //darkwar
	1.25f, //barbarian
	0.45f, //adept
	1.0f, //dark paladin
	0.65f, //diabolist
};


static float g_manaPercentByClass[NUM_CLASSES] = 
{
	0.01f, //fighter
	0.05f, //ranger
	0.20f, //paladin
	1.00f, //cleric
	1.00f, //wizard
	0.90f, //warlock
	0.90f, //witch
	0.80f, //druid
	0.15f, //darkwar
	0.01f, //barbarian
	10.0f, //adept
	0.20f, //dark paladin
	1.00f, //diabolist
};

#define NUM_MARKS 11

enum PVPKillReason
{
	PVP_KILL_DEATH,
	PVP_KILL_ALMUS,
	PVP_KILL_REDITUS,
};

enum PlayerActionResults
{
	PLAYER_ACTION_NONE,
	PLAYER_ACTION_LEVEL_UP,
	PLAYER_ACTION_INVENTORY_FULL,
	PLAYER_ACTION_TOO_MUCH_WEIGHT,
	PLAYER_ACTION_PRESS_F12_TO_RETURN,
	PLAYER_ACTION_LOCKED,
	PLAYER_ACTION_LOCK_PICKED,
	PLAYER_ACTION_PICK_FAILED,
	PLAYER_ACTION_EMPTY,
	PLAYER_ACTION_YOU_OWN,
	PLAYER_ACTION_GAINED_STRENGTH,
	PLAYER_ACTION_GAINED_AGILITY,
	PLAYER_ACTION_GAINED_CONSTITUTION,
	PLAYER_ACTION_GAINED_INTELLECT,
	PLAYER_ACTION_GAINED_WISDOM,
	PLAYER_ACTION_CANT_READ,
	PLAYER_ACTION_CANT_LOGOFF_IN_BATTLE,
	PLAYER_ACTION_KEY_USED,
	PLAYER_ACTION_KEY_FAILED,
	PLAYER_ACTION_YOU_FEEL_POWERFUL,
	PLAYER_ACTION_GAINED_SKILL,
	PLAYER_ACTION_SERVER_FULL,
	PLAYER_ACTION_NOT_USABLE_IN_BATTLE,
	PLAYER_ACTION_CANT_USE_ITEM,
	PLAYER_ACTION_REQUIREMENTS_NOT_MET,
	PLAYER_ACTION_PVP_LEVEL_UP,
};

enum PlayerAttackState
{
	ATTACK_STATE_MONSTERS,
	ATTACK_STATE_ENEMIES,
	ATTACK_STATE_ALL,
};

struct PendingTransfer
{
	uint32 newmapid;
	uint32 newx;
	uint32 newy;
	bool playerportal;
};

class InfoMessage
{
public:
	std::vector<std::string> messages;

	void AddMessage(const char* format, ...)
	{
		va_list ap;
		va_start(ap, format);

		char buffer[2048];

		vsprintf(buffer, format, ap);

		messages.push_back(buffer);

		va_end(ap);
	}
};

class Account;
struct QuestEntry;
class Player : public Unit
{
public:
	Account* m_account;
	uint32 m_playerId;
	uint32 m_accountId;
	uint16 m_strength;
	uint16 m_agility;
	uint16 m_constitution;
	uint16 m_intellect;
	uint16 m_wisdom;
	uint16 m_path;
	uint16 m_gender;
	uint16 m_race;
	uint32 m_oldclass;
	uint32 m_class;
	uint32 m_points;
	uint32 m_experience;
	Session* m_session;
	bool m_packetseq;
	bool m_buffseq;
	uint16 m_cheatfollowobj;
	bool m_canResurrect;
	array<Point, NUM_MARKS> m_marks;
	bool m_marksLoaded;
	bool m_itemsLoaded;
	bool m_skillsLoaded;
	int16 m_alignment;
	uint8 m_attackstate;
	uint16 m_pvplevel;
	uint32 m_pvpexp;
	uint16 m_petcontrolid;
	uint32 m_pvpcombatexpire;
	uint32 m_pvpattackexpire;
	bool m_betraypvpcombat;
	bool m_globalchatDisabled;
	uint32 m_transdelay;
	uint8 m_skillpoints;
	uint8 m_pvppoints;
	array<uint32, NUM_SKILLS> m_skills;
	std::map<uint32, uint32> m_skillcooldowns;
	uint8 m_weaponSwitch;
	std::set<std::string> m_ignoredNames;
	uint32 m_leadtimer;
	uint32 m_pvpdiminish;
	uint32 m_mapid;
	PendingTransfer m_pendingmaptransfer;
	uint32 m_bosskilledid;
	uint32 m_temppvpexp;
	uint32 m_nextpvpexpupdate;
	uint32 m_nextchatmessage;
	uint32 m_alignlock;
	uint32* m_flatmods[NUM_SPELL_GROUPS];
	float* m_pctmods[NUM_SPELL_GROUPS];
	bool m_extralog;
	bool m_invis;
	QuestEntry* m_currentquest;
	uint32 m_currentquestcount;
	bool m_nochatupdate;
	std::set<uint32> m_completedquests;

	uint32 m_PVPExpireTime;

	//lua target stuff
	uint16 m_currenttarget;
	uint32 m_currenttargetlastupdate;

	Player(uint32 id);
	~Player();

	void Update();
	void UpdatePacketQueue();

	void Send(ServerPacket* p);
	void Send(Packet* p);


	void BuildInitialLoginPacket();
	void BuildUpdatePacket();

	bool InPVPMode();
	bool InPVPAttackMode() { return m_pvpattackexpire > g_mstime; }

	void AppendUpdateData(ServerPacket* p, Unit* u, uint8 block);

	void CalculateHealth();
	void CalculateMana();

	uint32 GetBonus(uint32 s);

	void SendAllStats();

	void SendExperience();
	void SendStats();
	void SendItems();
	void SendSkills();

	void ResetStats();

	void SendInfoMessage(InfoMessage & msg);

	void SetExtraLog()
	{
		m_extralog = true;
		AddEvent(&Player::RemoveExtraLog, EVENT_UNKNOWN, 60 * 30 * 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void RemoveExtraLog() { m_extralog = false; }

	void Recalculate()
	{
		CalculateHealth();
		CalculateMana();
	}

	void SaveToDB();

	uint32 GetMeleeDamage();

	void StopAction();

	void OnItemLoad(MYSQL_RES* ev);
	void OnMarksLoad(MYSQL_RES* ev);
	void OnSkillsLoad(MYSQL_RES* ev);
	void OnDeathTokensLoad(MYSQL_RES* ev);
	void OnSkillCooldownLoad(MYSQL_RES* ev);
	void OnCompletedQuestsLoad(MYSQL_RES* ev);
	void OnCurrencyLoad(MYSQL_RES* ev);

	uint32 GetAttackSpeed();
	uint32 GetAttackSpeedBase() { return 2000; }

	void OnHealthChange(uint32 old);

	void GoHome()
	{
		int32 homex, homey;
		if (m_alignment <= 0)
		{
			homex = 87 * 20 + 10;
			homey = 609 * 20 + 10;
		}
		else if (m_faction == FACTION_GOOD)
		{
			homex = 920;
			homey = 1140;
		}
		else
		{
			homex = 54560;
			homey = 54620;
		}


		if (m_mapid == 0)
			SetPosition(homex, homey);

		if (m_mapid == 1)
		{
			SetPosition(1154 * 20 + 10, 646 * 20 + 10);
// 			m_pendingmaptransfer.newmapid = 0;
// 			m_pendingmaptransfer.newx = homex;
// 			m_pendingmaptransfer.newy = homey;
// 			m_pendingmaptransfer.playerportal = false;
// 			m_mapmgr->RemoveObject(this);
		}
	}

	void Respawn();

	uint32 GetItemClass();

	void SendActionResult(uint16 res);

	bool CanCast(uint32 spellid, bool triggered = false);

	void PortToMark(uint8 mark);

	void EventTeleport(int32 x, int32 y, uint32 mapid);

	void OnTeleportComplete();
	void OnAddToWorld();

	void OnRemoveFromWorld();

	void UnloadItems();
	void UnloadMarks();

	void ModAlign(int32 mod)
	{
		int32 newalign = GetAlign() + mod;
		if (newalign > 1000)
			newalign = 1000;
		if (newalign < -1000)
			newalign = -1000;

		if (m_alignlock > g_time && newalign >= -1)
			newalign = -1;

		SetAlign(newalign);
		if (m_class == CLASS_CLERIC || m_class == CLASS_PALADIN || m_class == CLASS_DIABOLIST)
			Recalculate();
	}

	void GiveExp(uint32 xp);

	void GivePVPExp(Player* other, PVPKillReason reason = PVP_KILL_DEATH);

	void GivePVPExp(uint32 xp);

	void RemovePVPDiminish()
	{
		--m_pvpdiminish;
	}

	void BuildOnlineList();

	void SendGreenMessage(const char* message)
	{
		ServerPacket data(0x7A, 400);
		data.Write((uint8*)message, 0x28);
		for (uint32 i = 0; i < 9; ++i)
		data.WriteZero(0x28);
		Send(&data);
	}

	void UpdateFaction()
	{
		//if (m_path == 0)
			m_faction = FACTION_GOOD;
		//else
		//	m_faction = FACTION_EVIL;
	}

	void UpdateBodyArmorVisual();

	void SetPVPCombat();
	void SetBetrayPVPCombat()
	{
		m_betraypvpcombat = true;
	}

	void RemovePVPCombat() 
	{
		m_pvpcombatexpire = 0;
		m_betraypvpcombat = false;
		m_pvpattackexpire = 0;
	}

	void OnStatsChange();

	uint8 GetPath();

	void SwitchWeapons();

	void AddSkillCooldown(uint32 skillid);

	bool SkillReady(uint32 skillid);

	void AddToLink(Player* plr);

	void ModSkill(uint32 skillid, int32 mod);


	//new modifier system
	void AddFlatMod(uint32 spellmask, uint32 modtype, uint32 val)
	{
		if (m_flatmods[modtype] == NULL)
		{
			m_flatmods[modtype] = new uint32[NUM_SPELL_GROUPS];
			memset(m_flatmods[modtype], 0, sizeof(uint32) * NUM_SPELL_GROUPS);
		}

		for (uint32 i = 0; i < NUM_SPELL_GROUPS; ++i)
		{
			if ((1 << i) & spellmask)
			{
				m_flatmods[modtype][i] += val;
			}
		}
	}

	void RemoveFlatMod(uint32 spellmask, uint32 modtype, uint32 val)
	{
		if (m_flatmods[modtype] == NULL)
			return;

		for (uint32 i = 0; i < NUM_SPELL_GROUPS; ++i)
		{
			if ((1 << i) & spellmask)
			{
				m_flatmods[modtype][i] -= val;
			}
		}
	}

	void AddPctMod(uint32 spellmask, uint32 modtype, uint32 val)
	{
		if (m_pctmods[modtype] == NULL)
		{
			m_pctmods[modtype] = new float[NUM_SPELL_GROUPS];
			for (uint32 i = 0; i < NUM_SPELL_GROUPS; ++i)
				m_pctmods[modtype][i] = 1.0f;
		}

		for (uint32 i = 0; i < NUM_SPELL_GROUPS; ++i)
		{
			if ((1 << i) & spellmask)
			{
				m_pctmods[modtype][i] *= float(100 + val) / 100;
			}
		}
	}

	void RemovePctMod(uint32 spellmask, uint32 modtype, uint32 val)
	{
		if (m_pctmods[modtype] == NULL)
			return;

		for (uint32 i = 0; i < NUM_SPELL_GROUPS; ++i)
		{
			if ((1 << i) & spellmask)
			{
				m_pctmods[modtype][i] /= float(100 + val) / 100;
			}
		}
	}

	//applying
	template<typename T> void ApplyFlatMod(T & val, uint32 modtype, uint32 spellmask)
	{
		if (m_flatmods[modtype] == NULL)
			return;
		for (uint32 i = 0; i < NUM_SPELL_GROUPS; ++i)
		{
			if ((1 << i) & spellmask)
				val += m_flatmods[modtype][i];
		}
	}

	template<typename T> void ApplyPctMod(T & val, uint32 modtype, uint32 spellmask)
	{
		if (m_pctmods[modtype] == NULL)
			return;
		for (uint32 i = 0; i < NUM_SPELL_GROUPS; ++i)
		{
			if ((1 << i) & spellmask)
				val *= m_pctmods[modtype][i];
		}
	}

	void OnKill(Creature* cre);

	//Trade stuff
	uint32 m_tradetarget;
	uint32 m_tradeitems[10]; //the indexes to our inventory
	bool m_tradeaccept;

	void InitiateTrade();
	void SendTradeUpdate();
	void CancelTrade();

	void AddTradeItem(uint32 slot);
	void AcceptTrade();
	void CompleteTrade();

	//Storage
	void OpenStorage();

	void CompactItems();

	bool CreatePath(int32 destx, int32 desty)
	{
		m_destinationX = destx;
		m_destinationY = desty;
		return true;
	}

	void UpdateMovement(Object* followingObject = NULL);

	void BuildTargetUpdate(Unit* target);
	void ClearTarget();
	void SetTarget(Unit* target);

	void OnRemoveInRangeObject(Object* o);
	void OnAddInRangeObject(Object* o);

	bool IsEligableForQuest(QuestEntry* quest);

	void SendQuestInfoOfAll(uint32 type);
	void UpdateAllCreatureQuestInfo();
	void UpdateAllHealthBars();

	void SetAlign(int32 newalign) { m_alignment = newalign; UpdateAllHealthBars(); }
	int32 GetAlign() { return m_alignment; }

	//currencies
	std::map<uint32, int32> m_currencies;

	bool ModCurrency(uint32 cur, int32 amount, int32 maxvalue = -1)
	{
		auto itr = m_currencies.find(cur);

		//cannot reduce currency if we do not have it
		if (itr == m_currencies.end() && amount < 0)
			return false;

		if (itr == m_currencies.end())
			m_currencies.insert(std::make_pair(cur, amount));
		else
		{
			auto current = itr->second;

			//do we have enough?
			if (amount < 0 && (current - amount) < 0)
				return false;

			itr->second += amount;
		}

		if (maxvalue != -1)
		{
			itr = m_currencies.find(cur);

			if (itr != m_currencies.end() && itr->second > maxvalue)
				itr->second = maxvalue;
		}

		return true;
	}

	void SetCurrency(uint32 cur, int32 amount)
	{
		auto itr = m_currencies.find(cur);

		if (itr == m_currencies.end())
			m_currencies.insert(std::make_pair(cur, amount));
		else
			itr->second = amount;
	}

	int32 GetCurrency(uint32 cur)
	{
		auto itr = m_currencies.find(cur);

		if (itr == m_currencies.end())
			return 0;
		return itr->second;
	}


	//LINKID
	uint32 GetLinkID()
	{
		auto linkid = GetCurrency(CURRENCY_LINK_ID);
		return *(uint32*)&linkid;
	}

	void SetLinkID(uint32 id)
	{
		int32 asi32 = *(int32*)&id;
		SetCurrency(CURRENCY_LINK_ID, asi32);
	}

	bool IsLinkOwner()
	{
		return GetCurrency(CURRENCY_LINK_IS_OWNER) == 1;
	}

	void SetLinkOwner(bool isowner)
	{
		int32 val = 0;
		if (isowner)
			val = 1;
		SetCurrency(CURRENCY_LINK_IS_OWNER, val);
	}
};

#endif
