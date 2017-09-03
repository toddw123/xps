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

#ifndef __UNIT_H
#define __UNIT_H

#include "MapCell.h"

enum PointOfInterest //what char is currently doing
{
	POI_IDLE,
	POI_ATTACKING,
	POI_SCRIPT,
	POI_EVADE, //evading, cannot take damage
	POI_PET_FOLLOW_OWNER,
	POI_PET_FOLLOW_OWNER_BATTLE, //battle mode, searches for targets and attacks, dun dun dun
	POI_PET_MOVE, //when they click them to move them
	NUM_POI,
};

enum Reaction
{
	REACTION_FRIENDLY,
	REACTION_NEUTRAL,
	REACTION_NEUTRAL_ATTACKABLE,
	REACTION_HOSTILE,
	REACTION_SCRIPT_NOT_SET_USE_SERVER_DEFAULT,
	NUM_REACTIONS,
};

enum PointOfInterestFailReason
{
	POI_CANNOT_CREATE_PATH,
	POI_UNIT_IS_DEAD,
	POI_STUCK_ON_TERRAIN,
	POI_OUT_OF_RANGE,
	NUM_POI_FAIL_REASONS,
};

enum Animations
{
	ANIMATION_NONE1H			= 0,
	ANIMATION_ATTACKMOVE1H		= 2,
	ANIMATION_STRIKE1H			= 3,
	ANIMATION_STRIKE1H_2		= 4,
	ANIMATION_STRIKE1H_BLOCK	= 6,
	ANIMATION_STRIKE1H_BLOCK_2	= 7,
	ANIMATION_HIT				= 8,
	ANIMATION_HIT_2				= 9,
	ANIMATION_STRIKEBOW			= 13,
	ANIMATION_STRIKEBOW_2		= 14,
	ANIMATION_STRIKEPOLEARM		= 15,
	ANIMATION_STRIKEPOLEARM_2	= 16,
	ANIMATION_STRIKE2H			= 17,
	ANIMATION_STRIKE2H_2		= 18,
	ANIMATION_STRIKEBLADES		= 19,
	ANIMATION_STRIKEBLADES_2	= 20,
	ANIMATION_DEATH				= 21,
	ANIMATION_STRIKE2H_FEMALE	= 24,
	ANIMATION_STRIKE2H_FEMALE_2	= 25,
	ANIMATION_NONEPOLEARM		= 26,
	ANIMATION_ATTACKMOVEPOLEARM	= 27,
	ANIMATION_SHIELDBLOCK		= 35,
	ANIMATION_SHIELDBLOCK_2		= 36,
	ANIMATION_ATTACKRUN			= 42,
	ANIMATION_SPINATTACK		= 43,
	ANIMATION_STAB				= 46,
	ANIMATION_STAB_2			= 47,
	ANIMATION_STRIKE1HFAST		= 48,
	ANIMATION_STRIKE1HFAST_2	= 49,
	ANIMATION_MOVING			= 50,
	ANIMATION_SIT				= 51,
	ANIMATION_POINT				= 52,
	ANIMATION_POINT_2			= 53,
	ANIMATION_VICTORY			= 54,
	ANIMATION_VICTORY_2			= 55,
	ANIMATION_DANCE				= 56,
	ANIMATION_DANCE_2			= 57,
	ANIMATION_SPIRITRUN			= 78,
};

enum DamageFlags
{
	DAMAGE_FLAG_NO_REFLECT				= 0x01,
	DAMAGE_FLAG_DO_NOT_APPLY_BONUSES	= 0x02,
	DAMAGE_FLAG_DO_NOT_APPLY_RESISTS	= 0x04,
	DAMAGE_FLAG_CANNOT_PROC				= 0x08,
	DAMAGE_FLAG_IS_POISON_ATTACK		= 0x10,
	DAMAGE_FLAG_IS_MELEE_ATTACK			= 0x20,
};

enum UnitFlags
{
	UNIT_FLAG_CANNOT_MOVE				= 0x0001,
	UNIT_FLAG_CANNOT_ATTACK				= 0x0002,
	UNIT_FLAG_UNATTACKABLE				= 0x0004,
	UNIT_FLAG_INVISIBLE					= 0x0008,
	UNIT_FLAG_SHOW_HEALTH_PCT			= 0x0010,
	UNIT_FLAG_CANNOT_RESTORE_OR_REVIVE	= 0x0020,
	UNIT_FLAG_CANNOT_CHANGE_TARGET		= 0x0040,
	UNIT_FLAG_DO_NOT_ATTACK_PLAYERS		= 0x0080,
	UNIT_FLAG_INHERIT_PROTO				= 0x0100,
	UNIT_FLAG_INACTIVE_QUESTGIVER		= 0x0200,
	UNIT_FLAG_NO_PATHFINDING			= 0x0400,
	UNIT_FLAG_IS_GHOST					= 0x0800,
	UNIT_FLAG_ADMIN_VISION				= 0x1000,
	UNIT_FLAG_EXPLICIT_ANIMATIONS		= 0x2000,
	UNIT_FLAG_NPC_FIGHTS_NO_DAMAGE		= 0x4000,
};

enum UnitUpdates
{
	UPDATETYPE_MOVEMENT		= 0x01,
	UPDATETYPE_AURAS		= 0x02,
	UPDATETYPE_MODELS		= 0x04,
	UPDATETYPE_ANIMATION	= 0x08,
	UPDATETYPE_EFFECTS		= 0x10,
	UPDATETYPE_CHATMSG		= 0x20,
};

enum ContinentType
{
	CONTINENT_TYPE_ANTI_PK_GOOD,
	CONTINENT_TYPE_ANTI_PK_EVIL,
	CONTINENT_TYPE_FULL_PK,
	CONTINENT_TYPE_NO_PK,
};

enum ItemBonuses
{
	CHANCE_HIT_UNDEAD			= 1,
	ON_HIT_DISPEL_CHANCE		= 2,
	MANA_BONUS					= 3,
	MANA_REGENERATION			= 4,
	ON_DAMAGE_MANA_STEAL		= 5,
	HEALTH_BONUS				= 6,
	HEALTH_REGENERATION			= 7,
	ON_DAMAGE_HEALTH_STEAL		= 8,
	RAGE_BONUS					= 9,
	HIT_BONUS					= 10,
	DEFEND_BONUS				= 11,
	ON_DAMAGE_ICE_DAMAGE		= 12,
	ON_DAMAGE_FIRE_DAMAGE		= 13,
	ON_DAMAGE_ACID_DAMAGE		= 14,
	ON_DAMAGE_LIGHTNING_DAMAGE	= 15,
	REFLECT_PROJECTILE			= 20,
	REFLECT_DAMAGE				= 21,
	PROTECTION_ICE				= 22,
	PROTECTION_FIRE				= 23,
	PROTECTION_SHADOW			= 24,
	PROTECTION_ENERGY			= 25,
	PROTECTION_SPIRIT			= 26,
	PROTECTION_ALL_MAGIC		= 29,
	PROTECTION_ARMOR_BONUS		= 30,
	ATTACK_SPEED				= 36,
	ON_HIT_CHANCE_LETHAL		= 43,
	CAST_SPEED					= 44,
	BONUS_ABILITY_LEVEL			= 51,
	LIGHTNING_BOW				= 52,
	FIRE_DRAGON_STAFF			= 53,
	ICE_DRAGON_STAFF			= 54,
	FIRE_MAGIC_BONUS			= 55,
	ICE_MAGIC_BONUS				= 56,
	ENERGY_MAGIC_BONUS			= 57,
	SHADOW_MAGIC_BONUS			= 58,
	SPIRIT_MAGIC_BONUS			= 59,
	ALL_MAGIC_BONUS				= 61,
	BONUS_PLUS_DAMAGE			= 62,
	BONUS_MAXIMUS_CHANCE		= 63,
	BONUS_REDITUS_CHANCE		= 64,
	BONUS_LUMEN_WAND_BEAM		= 65,
	BONUS_RADIUS_WAND_BEAM		= 66,
	BONUS_SHOT_SPEED			= 68,
	BONUS_FRAGOR_CHANCE			= 70,
	BONUS_ARMOR_SPELLS			= 71,
	REQUIRE_READ				= 100,
	ITEM_BUILTIN_END			= 101,
	BONUS_SKILL,
	ON_HIT_FEROCIAS_ATTACK,
	BONUS_DAMAGE_PCT,
	ON_HIT_STUPIFY_CHANCE,
	RESIST_STUPIFY,
	RESIST_DISPEL,
	RESIST_POISON,
	DEFENSE,
	PROTECTION_TEURI,
	PROTECTION_EXTRUDERE,
	ON_HIT_DISPEL_CHANCE_CHAOTIC,
	ON_HIT_LEECH_CHANCE,
	ON_HIT_ENERGISE_CHANCE,
	ON_HIT_ANIMUS_CHANCE,
	BONUS_EXPLODING_ARROWS,
	NUM_ITEM_BONUSES,
};

void InitSkillLimits();

enum Factions
{
	FACTION_GOOD,
	FACTION_EVIL,
	FACTION_NPC_EVIL_HOSTILE,
	FACTION_NPC_EVIL,
	FACTION_NEUTRAL,
	NUM_FACTIONS,
};

enum AnimationType
{
	ANIM_TYPE_STANDING,
	ANIM_TYPE_MOVING,
	ANIM_TYPE_ATTACK_MOVING,
	ANIM_TYPE_ATTACK_SWING,
	ANIM_TYPE_ATTACK_MISS,
	ANIM_TYPE_ATTACK_BLOCK,
	ANIM_TYPE_HIT,
};

enum BuffFlags
{
	BUFF_FLAG_POISON			= 0x0001,
	BUFF_FLAG_TEURI				= 0x0002,
	BUFF_FLAG_VELOCITAS			= 0x0004,
	BUFF_FLAG_NIGHT_VISION		= 0x0008,
	BUFF_FLAG_STUPIFY			= 0x0010,
	BUFF_FLAG_BATTLE_SCROLL		= 0x0020,
	BUFF_FLAG_RED_POTION		= 0x0040,
	BUFF_FLAG_ROBUR				= 0x0080,
	BUFF_FLAG_EXTRUDERE			= 0x0100,
	BUFF_FLAG_NOXA				= 0x0200,
	BUFF_FLAG_OMNIMULTIM		= 0x0400,
	BUFF_FLAG_WEATHER_RAIN		= 0x0800,
	BUFF_FLAG_MANA_REGEN		= 0x1000,
	BUFF_FLAG_HEALTH_REGEN		= 0x2000,
};

enum SkillFlags
{
	SKILL_FLAG_DAZED			= 0x0001,
	SKILL_FLAG_OMNI_SHIELD		= 0x0002,
	SKILL_FLAG_BERSERK			= 0x0004,
	SKILL_FLAG_INTENSITY		= 0x0008,
	SKILL_FLAG_FOCUSED_STRENGTH	= 0x0010,
	SKILL_FLAG_INNER_STRENGTH	= 0x0020,
	SKILL_FLAG_BREAKER			= 0x0040,
	SKILL_FLAG_DAMAGE_SHIELD	= 0x0080,
	SKILL_FLAG_CORRUPT_ARMOR	= 0x0100, //goes on cleric
	SKILL_FLAG_CORRUPT_ARMOR2	= 0x0200, //goes on attacker
	SKILL_FLAG_SHADOW_POWER		= 0x0400,
	SKILL_FLAG_DISINTEGRATION	= 0x0800,
	SKILL_FLAG_DULL_SENSES		= 0x1000,
	SKILL_FLAG_DULL_SENSES2		= 0x2000,
	SKILL_FLAG_BLEED			= 0x4000,
};

enum AuraFlags
{
	AURA_FLAG_WHITE_PEARL		= 0x0001,
	AURA_FLAG_MUTATIO_NIMBUS	= 0x0002,
	AURA_FLAG_MUTATIO_FLAMMA	= 0x0004,
	AURA_FLAG_INVISIBILITY		= 0x0008,
	AURA_FLAG_DIVINUS_TEURI		= 0x0010,
	AURA_FLAG_STOP_ANIMATION	= 0x0020,
	AURA_FLAG_WHITE_POTION		= 0x0040,
	AURA_FLAG_HIDE_SKILL		= 0x0080,
	AURA_FLAG_PINK_EGG			= 0x0100,
	AURA_FLAG_BLUE_EGG			= 0x0200,
	AURA_FLAG_FREEZE_SOLID		= 0x0400 | AURA_FLAG_STOP_ANIMATION,
};

enum DeathState
{
	STATE_ALIVE,
	STATE_DEAD,
};

enum UnitNameSpecials
{
	UNIT_NAME_SPECIAL_NONE,
	UNIT_NAME_SPECIAL_KING,
	UNIT_NAME_SPECIAL_QUEEN,
	UNIT_NAME_SPECIAL_KNIGHT,
	UNIT_NAME_SPECIAL_ADEPT,
};

enum UnitNameSpecials2
{
	UNIT_NAME_SPECIAL2_NONE,
	UNIT_NAME_SPECIAL2_NONE2,
	UNIT_NAME_SPECIAL2_NONE3,
	UNIT_NAME_SPECIAL2_DRETHOR,
	UNIT_NAME_SPECIAL2_MAGROTH,
	UNIT_NAME_SPECIAL2_OSINEX,
};

#define MAX_LEVEL 70

static uint32 g_expByLevel[MAX_LEVEL + 11] = {
0,				200,			1000,			2000,			4000,
7000,			12000,			22000,			35000,			50000,
70000,			100000,			150000,			250000,			450000,
750000,			1250000,		1750000,		2250000,		2750000,
3500000,		4250000,		5000000,		6000000,		7000000,
8000000,		10000000,		12000000,		14500000,		17000000,
19500000,		22000000,		25000000,		28000000,		32000000,
37000000,		42000000,		48000000,		54000000,		60000000,
67000000,		74000000,		82000000,		90000000,		98000000,
108000000,		118000000,		130000000,		150000000,		160000000,
180000000,		200000000,		230000000,		260000000,		290000000,
320000000,		350000000,		400000000,		450000000,		500000000,
600000000,		700000000,		800000000,		1000000000,		1200000000,
1400000000,		1600000000,		1800000000,		2000000000,		2500000000,
4250000000,		4250000000,		4250000000,		4250000000,		4250000000,
4250000000,		4250000000,		4250000000,		4250000000, 4250000000, 4250000000 };


INLINED static uint32 GetLevelXPRequired(uint32 level)
{
	if (level > MAX_LEVEL || level == 0)
		return 0;
	uint32 this_level = g_expByLevel[level - 1];
	uint32 next_level = g_expByLevel[level];
	return next_level - this_level;
}

INLINED static uint32 NormaliseExp(uint32 exp, uint32 level, uint32 new_level, bool allow_increase = false)
{
	uint32 req_old = GetLevelXPRequired(level);
	uint32 req_new = GetLevelXPRequired(new_level);

	if (allow_increase == false && req_new >= req_old)
		req_new = req_old;

	float ratio = float(req_new) / float(req_old);
	float newexp = float(exp) * ratio;

	sLog.Debug("EXPNORM", "base %u l %u nl %u ro %u rn %u r %f nexp %f", exp, level, new_level, req_old, req_new, ratio, newexp);

	return uint32(newexp);
}

#define NUM_EQUIPPED_ITEMS 15
#define NUM_INVENTORY_ITEMS 50
#define NUM_STORAGE_ITEMS 50

struct GlobalSay
{
	std::string name;
	std::string message;
	uint32 to_player;
	uint32 to_link;
};

enum Stats
{
	STAT_UNK,
	STAT_STRENGTH,
	STAT_AGILITY,
	STAT_CONS,
	STAT_INTEL,
	STAT_WISDOM,
	STAT_MAXHEALTH,
	STAT_MAXMANA,
	STAT_UNK2,
	STAT_TRUELEVEL,
	STAT_LEVEL,
	STAT_CURHEALTH,
	STAT_CURMANA,
	STAT_UNK4,
	STAT_UNK5,
	STAT_POINTS = 15,
	STAT_PATH = 19,
	STAT_GENDER = 20,
	STAT_RACE = 21,
	STAT_CLASS = 22,
	STAT_UNK6,
	STAT_UNK7,
	STAT_UNK8,
	STAT_ALIGNMENT = 26,
	NUM_STATS = 41,
};

class Item;
class Player;
class Unit : public Object
{
public:
	std::string m_namestr;
	uint8 m_unitAnimation;
	bool m_attackRun;
	uint8 m_spelleffect;
	uint8 m_faction;
	float m_destinationRadius;
	float m_destinationX;
	float m_destinationY;
	float m_startX;
	float m_startY;
	uint32 m_startmove; //ms
	uint32 m_endmove; //ms

	int32 m_POIX;
	int32 m_POIY;
	uint16 m_POIObject;
	uint32 m_POI;
	float m_speed;
	std::string m_lastSay;
	std::deque<GlobalSay> m_globalSays;
	bool m_pendingSay;
	uint32 m_nextspell;
	uint32 m_lastspell;
	uint32 m_lastspellid;
	uint32 m_nextspellid;
	TargetData m_nextspelltargets;
	bool m_nextspelltrigger;
	uint16 m_buffflags;
	uint16 m_spellflags;
	uint16 m_skillflags;
	uint32 m_nextmove;
	array<Aura*, NUM_SPELL_AURAS> m_auras;
	array<Item*, NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS + NUM_STORAGE_ITEMS> m_items;
	uint32 m_updateCounter;
	std::set<uint16> m_pets;
	uint32 m_unitflags;
	uint8 m_unitspecialname;
	uint8 m_unitspecialname2;
	uint32 m_thrownweapon;
	float m_destinationAngle;
	float m_followOwnerAngle;
	uint32 m_mana;
	uint32 m_maxmana;
	uint32 m_level;

	bool HasUnitFlag(uint32 flag) { return ((m_unitflags & flag) != 0); }
	void SetUnitFlag(uint32 flag) { m_unitflags |= flag; }
	void RemoveUnitFlag(uint32 flag) { m_unitflags &= ~flag; }

	uint32 GetSummonedObjectCount(uint32 visual);
	void GetSummonedObject(uint32 visual, std::vector<Object*>* out);
	uint32 GetSkillValue(uint32 skillid);

	array<uint32, NUM_ITEM_BONUSES> m_bonuses;

	uint32 m_laststrike;
	uint32 m_lastrage;
	uint32 m_lastfreeze;
	bool m_laststrikeblock;

	uint16 m_charDisplay;
	uint16 m_weaponDisplay;
	uint8 m_shieldDisplay;
	uint8 m_helmetDisplay;
	uint8 m_color1;
	uint8 m_color2;

	//Health
	uint32 m_health;
	uint32 m_maxhealth;
	uint8 m_deathstate;
	uint32 m_regenTimer;
	uint32 m_nextchatEvent;
	uint32 m_lastdamager;

	//Lua
	uint32 m_lasttargetupdate; //for target frame data

	//
	int32 m_crowdAgentIndex = -1;

	INLINED uint32 GetHealth() { return m_health; }
	INLINED uint32 GetMaxHealth() { return m_maxhealth; }
	INLINED void SetHealth(uint32 v) { uint32 old = m_health; m_health = v; OnHealthChange(old); }
	INLINED void ModHealth(int32 m)
	{
		uint32 old = m_health;
		if (m < 0 && (uint32)abs(m) > m_health)
			m = -(int32)m_health;
		if (m_health + m > m_maxhealth)
			m = m_maxhealth - m_health;
		m_health += m;
		OnHealthChange(old);
	}

	INLINED uint32 GetMaxMana() { return m_maxmana; }
	INLINED uint32 GetMana() { return m_mana; }
	INLINED void SetMana(uint32 v) { m_mana = v; }

	INLINED void ModMana(int32 m)
	{
		if (m < 0 && (uint32)abs(m) > m_mana)
			m = -(int32)m_mana;
		if (m_mana + m > m_maxmana)
			m = m_maxmana - m_mana;
		m_mana += m;
	}

	virtual void OnHealthChange(uint32 old);

	void SendHealthUpdate() { SendHealthUpdate(NULL); }
	void SendHealthUpdate(Player* to);

	INLINED bool IsAlive() { return m_deathstate == STATE_ALIVE; }
	void Revive();

	Unit();
	~Unit();

	void RemoveAllAuras();

	void RemoveAreaAuras();

	void RemoveAllDispellableAuras();

	int GetUnitType();

	INLINED void ChangeCell(MapCell* c)
	{
		ASSERT(m_cell && c);
		m_cell->RemoveObject(this);
		c->InsertObject(this);
		m_cell = c;
	}

	void SetUpdateBlock(uint8 block);
	void RemoveUpdateBlock(uint8 block);

	void EventSetPosition(int32 x, int32 y)
	{
		SetUpdateBlock(0x1F);
		SetPosition(x, y);
	}

	void Strike(Object* obj, bool ignoretimer = false);

	bool DidHit( Unit* u );
	void SetSpellEffect(uint8 effect);
	void ClearSpellEffect() { SetSpellEffect(0); }

	void OnCollision(Object* o);

	virtual void Update();

	virtual void UpdateMovement( Object* followingObject = NULL );

	void SetCrowdTarget(float x, float y);

	void GroundItemPickup();

	void SetModels(int32 charmodel = -1, int32 weaponmodel = -1, int32 shieldmodel = -1, int32 helmmodel = -1, int32 color1 = -1, int32 color2 = -1);

	void SendChatMsg(const char* message, bool localonly = false);

	void EventChatMsg(uint32 delay, const char* format, ...);
	void ResetChatMsgTimer() { m_nextchatEvent = 0; }

	void EventSendChatMsg(std::string message)
	{
		SendChatMsg(message.c_str());
	}

	void MoveToNextPathPoint();

	uint8 GetReaction(Unit* other);
	bool IsHostile(Unit* other);
	bool IsAttackable(Unit* other);

	void StopMovement(int32 howlong = 0);

	INLINED bool CanMove() { return (m_nextmove <= g_mstime) && !HasUnitFlag(UNIT_FLAG_CANNOT_MOVE) && !HasAura(SPELL_AURA_MULTI_PROJECTILE); }

	void DealDamage(Unit* other, int32 damage, SpellEntry* spell = NULL, uint32 damageflags = 0);
	void DealDamageID(uint32 id, int32 damage, SpellEntry* spell = NULL, uint32 damageflags = 0);
	void EventDealDamage(uint32 serverid, int32 damage);
	virtual uint32 GetMeleeDamage() { return 0; }

	//buff flags
	void SetBuffFlag(uint16 flag) { m_buffflags |= flag; }
	void RemoveBuffFlag(uint16 flag) { m_buffflags &= ~flag; }
	void SetSkillFlag(uint16 flag) { m_skillflags |= flag; }
	void RemoveSkillFlag(uint16 flag) { m_skillflags &= ~flag; }

	INLINED void SetSpellFlag(uint16 flag)
	{
		m_spellflags |= flag;
		SetUpdateBlock(UPDATETYPE_AURAS);
	}

	INLINED void RemoveSpellFlag(uint16 flag)
	{
		m_spellflags &= ~flag;
		SetUpdateBlock(UPDATETYPE_AURAS);
	}

	void SetAnimation(uint8 anim, uint8 anim2, bool isexplicit = false);

	INLINED bool HasAnimation(uint8 anim, uint8 anim2)
	{
		if (m_unitAnimation == anim || (anim2 != 0 && m_unitAnimation == anim2))
			return true;
		return false;
	}
	
	bool HasAura(uint32 type) { return m_auras[type] != NULL; }
	void RemoveAura(uint32 type) { if (HasAura(type)) m_auras[type]->Remove(); }

	uint32 GetNumFreeInventorySlots();
	uint32 FindFreeInventorySlot(int32 itemid = -1, int32 reqfreestackspace = -1);
	uint32 FindFreeStorageSlot(int32 itemid = -1, int32 reqfreestackspace = -1);
	uint32 CountEquippedItem(uint32 itemid);
	Item* FindEquippedItem(uint32 itemid);

	virtual uint32 GetAttackSpeed();
	virtual uint32 GetAttackSpeedBase() { return 1000; }

	INLINED void SetAnimationType(uint32 type)
	{
		uint32 a1, a2;
		GetAnimationType(type, a1, a2);
		SetAnimation(a1, a2);
	}

	INLINED bool HasAnimationType(uint32 type)
	{
		uint32 a1, a2;
		GetAnimationType(type, a1, a2);
		return HasAnimation(a1, a2);
	}

	void GetAnimationType(uint32 type, uint32 & buf1, uint32 & buf2);

	virtual uint32 GetItemClass();
	virtual bool CanCast(uint32 spellid, bool triggered = false) { return true; }

	void CastSpell(uint16 spellid, Unit* u, bool triggered = false, uint32 damageflags = 0)
	{
		TargetData tar;
		tar.flags = SPELL_TARGET_UNIT;
		tar.x = u->m_serverId;
		Spell s(this, spellid, tar, triggered);
		s.m_damageflags = damageflags;
		s.Cast();
	}

	void CastSpell(uint16 spellid, bool triggered = false, uint32 damageflags = 0)
	{
		TargetData tar;
		tar.flags = SPELL_TARGET_SELF;
		Spell s(this, spellid, tar, triggered);
		s.m_damageflags = damageflags;
		s.Cast();
	}

	void CastSpell(uint16 spellid, int32 x, int32 y, bool triggered = false, uint32 damageflags = 0)
	{
		TargetData tar;
		tar.flags = SPELL_TARGET_GROUND;
		tar.x = x;
		tar.y = y;
		Spell s(this, spellid, tar, triggered);
		s.m_damageflags = damageflags;
		s.Cast();
	}

	void Regen();

	Item* FindInventoryItem(uint32 itemid);
	bool ProtectedByGround(bool canbeprotected = false);

	float GetMagicBonus(uint32 bonustype);

	float GetMagicResist(uint32 resistbase, bool includextruder = false);

	uint32 GetContinentType()
	{
		/*if (m_mapmgr != NULL && m_mapmgr->m_mapid == 1)
			return CONTINENT_TYPE_FULL_PK;
		if (m_positionX < 0x8000 && m_positionY < 0x8000)
			return CONTINENT_TYPE_ANTI_PK_GOOD;
		if (m_positionX >= 0x8000 && m_positionY >= 0x8000)
			return CONTINENT_TYPE_ANTI_PK_EVIL;
		if (m_positionX >= 0x8000 && m_positionY < 0x8000)
			return CONTINENT_TYPE_NO_PK;*/
		return CONTINENT_TYPE_FULL_PK;
	}

	void Attack(Object* obj);

	uint32 GetLevel();

	void DestroyInventory();

	virtual void OnPreRemoveFromWorld();

	virtual void OnRemoveFromWorld();

	void SetUnitSpecialName(uint8 val, uint32 duration);
	void SetUnitSpecialName2(uint8 val, uint32 duration);

	uint8 GetClassTitle();

	void StopAttacking();
	Item* GetItem(uint32 slot) { return m_items[slot]; }
	Item* GetCurrentWeapon();

	void SetNextSpell(uint32 spellid, TargetData & targets, bool triggered);

	void StopAttack(int32 howlong)
	{
		uint32 newval = g_mstime + howlong;
		
		if (newval >= m_laststrike)
			m_laststrike = newval;
	}

	void SetThrownWeapon(uint32 id);

	void EventForceReturnWeapon() { SetThrownWeapon(0); }

	void UpdatePetFollowAngles();

	uint32 GetPassFlags();

	void FaceObject(Object* obj)
	{		
		m_rotation = CalcFacingDeg(obj->m_positionX, obj->m_positionY);
		SetUpdateBlock(UPDATETYPE_MOVEMENT);
	}

	void DoDealDamage(Unit* other, uint32 damage);

	void SendToAdmins( Packet & data );
	void SendToAll(Packet & data);

	bool HealthBarShouldShow();

	bool IsAttacking() { return m_POI == POI_ATTACKING; }
	void ClearPOI() { OnPOIChange(POI_IDLE); m_POI = POI_IDLE; m_POIObject = 0; m_POIX = 0; m_POIY = 0; }
	void SetPOI(uint32 poi, int32 x, int32 y) { OnPOIChange(poi); m_POI = poi; m_POIX = x; m_POIY = y; m_POIObject = 0; }
	void SetPOI(uint32 poi, uint32 unitid)
	{
		OnPOIChange(poi); m_POI = poi; m_POIObject = unitid; m_POIX = 0; m_POIY = 0;

		if (m_POI == POI_ATTACKING)
			CreateAttackCrowdBehavior();
	}
	void SetPOI(uint32 poi) { OnPOIChange(poi); m_POI = poi; m_POIObject = 0; m_POIX = 0; m_POIY = 0; }
	virtual void OnPOIFail(uint32 reason) {}
	virtual void OnPOIChange(uint32 newpoi) {}

	virtual void UpdatePOI() {}

	virtual void OnActive();
	virtual void OnInactive();

	void CreateAttackCrowdBehavior();
};

#endif
