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

#ifndef __SKILLS_H
#define __SKILLS_H

#pragma pack(push, 1)
struct SkillEntry
{
	uint32 id;
	uint32 serverindex;
	char* name;
	uint32 reqskill;
	uint32 reqskillvalue;
	uint32 basepoints;
	uint32 scalebase;
	uint32 cooldown; //in minutes
};
#pragma pack(pop)

extern SQLStore<SkillEntry> sSkillStore;

#define SKILL_READ_RUNES           1
#define SKILL_SWIM                 2
#define SKILL_PICK_LOCK            3
#define SKILL_DAGGERS              7
#define SKILL_POLE_ARM             8
#define SKILL_SWORDS               9
#define SKILL_BLUNT_WEAPONS        10
#define SKILL_AXES                 11
#define SKILL_2_HANDED_WEAPONS     12
#define SKILL_BOWS                 13
#define SKILL_DAZE                 14
#define SKILL_SHIELD               15
#define SKILL_THRUST               16
#define SKILL_BLEED                17
#define SKILL_WEAPON_TACTICS       18
#define SKILL_DEFENSE_TACTICS      19
#define SKILL_RESIST_FIRE          20
#define SKILL_RESIST_ICE           21
#define SKILL_RESIST_ENERGY        22
#define SKILL_RESIST_SHADOW        23
#define SKILL_RESIST_SPIRIT        24
#define SKILL_FIRE_DEVOTION        25
#define SKILL_FLAME_AURA           26
#define SKILL_ICE_DEVOTION         27
#define SKILL_FREEZE_SOLID         28
#define SKILL_ENERGY_DEVOTION      29
#define SKILL_AWAY                 30
#define SKILL_SHADOW_DEVOTION      31
#define SKILL_SHADOW_AURA          32
#define SKILL_SPIRIT_DEVOTION      33
#define SKILL_OMNI_SHIELD          34
#define SKILL_WEAPON_MASTERY       35
#define SKILL_DEFENSE_MASTERY      36
#define SKILL_ARCHERY_MASTERY      37
#define SKILL_RESIST_MASTERY       38
#define SKILL_SHIELD_MASTERY       39
#define SKILL_RESIST_MAGIC         40
#define SKILL_RAGE_CHANCE          43
#define SKILL_FEROCIOUS_HIT        44
#define SKILL_SURE_HIT             45
#define SKILL_BERSERK              46
#define SKILL_HIDE                 47
#define SKILL_LETHAL               48
#define SKILL_ARCHERY              49
#define SKILL_DODGE                50
#define SKILL_INTENSITY            51
#define SKILL_RAGE_CHANCE2         52
#define SKILL_FEROCIOUS_HIT2       53
#define SKILL_ARMOR                54
#define SKILL_HEAL_ALL             55
#define SKILL_INNER_STRENGTH       56
#define SKILL_RAGE_CHANCE3         57
#define SKILL_FEROCIOUS_HIT3       58
#define SKILL_ARMOR2               59
#define SKILL_HEAL_ALL2            60
#define SKILL_INNER_STRENGTH2      61
#define SKILL_RAGE_CHANCE4         62
#define SKILL_FEROCIOUS_HIT4       63
#define SKILL_STRENGTH             64
#define SKILL_FOCUSED_STR          65
#define SKILL_MAGNIFY_STR          66
#define SKILL_HIDE2                67
#define SKILL_FOCUSED_ATTACK       68
#define SKILL_BREAKER              69
#define SKILL_LETHAL2              70
#define SKILL_BURN                 71
#define SKILL_DAMAGE_SHIELD        72
#define SKILL_SPELL_POWER          73
#define SKILL_SHADOW_POWER         74
#define SKILL_LEECH                75
#define SKILL_CORRUPT_ARMOR        76
#define SKILL_CRITICAL_ENERGY      77
#define SKILL_DISINTEGRATION       78
#define SKILL_INTENSIFY            79
#define SKILL_DULL_SENSES          80
#define SKILL_CRITICAL_ENERGY2     81
#define SKILL_HEALTH               82
#define SKILL_MAGIC                83
#define SKILL_MAXIMUS              84
#define SKILL_REDITUS              85
#define SKILL_NON_DROP             86
#define SKILL_REVENGE              87
#define NUM_SKILLS                 88

enum SkillScaleType
{
	SKILL_SCALE_NONE,
	SKILL_SCALE_STRENGTH,
	SKILL_SCALE_AGILITY,
	SKILL_SCALE_CONSTITUTION,
	SKILL_SCALE_INTELLIGENCE,
	SKILL_SCALE_WISDOM,
	SKILL_SCALE_WISDOM_INTELLIGENCE,
	SKILL_SCALE_STRENGTH_AGILITY,
	SKILL_SCALE_LEVEL,
};

#endif