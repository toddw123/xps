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

#ifndef __SPELL_H
#define __SPELL_H

#define FULGAR 250

class Spell;
typedef void(Spell::*SpellEffectHandler)(GrowableArray<Unit*> & targets);

enum SpellEffects
{
	SPELL_EFFECT_DUMMY,
	SPELL_EFFECT_DAMAGE,
	SPELL_EFFECT_CREATE_PROJECTILE,
	SPELL_EFFECT_CREATE_BEAM,
	SPELL_EFFECT_CREATE_OBJECT,
	SPELL_EFFECT_HEAL,
	SPELL_EFFECT_APPLY_AURA,
	SPELL_EFFECT_SHORT_TELEPORT,
	SPELL_EFFECT_CREATE_BOMB,
	SPELL_EFFECT_ATTACK_RUN,
	SPELL_EFFECT_SPIN_ATTACK,
	SPELL_EFFECT_DISPEL,
	SPELL_EFFECT_MULTI_SHOT,
	SPELL_EFFECT_CREATE_PORTAL,
	SPELL_EFFECT_REDITUS,
	SPELL_EFFECT_RESET_STATS,
	SPELL_EFFECT_SUMMON_CREATURE,
	SPELL_EFFECT_CHARM_CREATURE,
	SPELL_EFFECT_SONATIO,
	SPELL_EFFECT_INSTA_PORT,
	SPELL_EFFECT_APPLY_AURA_SELF,
	SPELL_EFFECT_APPLY_AURA_NEG,
	SPELL_EFFECT_APPLY_AREA_AURA,
	SPELL_EFFECT_APPLY_AREA_AURA_NEG,
	SPELL_EFFECT_DAMAGE_BASED_LEVEL,
	SPELL_EFFECT_HEAL_ALL,
	SPELL_EFFECT_THROW_WEAPON,
	SPELL_EFFECT_THROW_RETURN_WEAPON,
	NUM_SPELL_EFFECTS,
};

extern SpellEffectHandler g_SpellEffectHandlers[NUM_SPELL_EFFECTS + 1];

enum SpellTargetTypes
{
	SPELL_TARGET_SELF			= 0x0001,
	SPELL_TARGET_UNIT			= 0x0002,
	SPELL_TARGET_GROUND			= 0x0004,
	SPELL_TARGET_MARK			= 0x0008,
	SPELL_TARGET_AOE			= 0x0010,
	SPELL_TARGET_DEAD			= 0x0020,
	SPELL_TARGET_RANDOM_RADIUS	= 0x0040,
	SPELL_TARGET_AOE_FRIENDLY	= 0x0080,
	SPELL_TARGET_PETS			= 0x0100,
};

enum SpellTypes
{
	SPELL_TYPE_EFFECT,
	SPELL_TYPE_PROJECTILE,
	SPELL_TYPE_BEAM,
	SPELL_TYPE_BOMB,
	SPELL_TYPE_OBJECT,
	SPELL_TYPE_PORTAL,
	NUM_SPELL_TYPES,
};

enum SpellSchool
{
	SPELL_SCHOOL_PHYSICAL,
	SPELL_SCHOOL_SLASHING,
	SPELL_SCHOOL_SMASHING,
	SPELL_SCHOOL_PIERCING,
	SPELL_SCHOOL_SPIRIT,
	SPELL_SCHOOL_FIRE,
	SPELL_SCHOOL_ENERGY,
	SPELL_SCHOOL_POISON,
	SPELL_SCHOOL_ICE,
	SPELL_SCHOOL_ACID,
	SPELL_SCHOOL_SHADOW,
};

#pragma pack(push, 1)
struct SpellEntry
{
	uint32 id;
	char* name;
	uint32 spelllevel;
	uint32 spellmask;
	uint32 targetallowedtype;
	uint32 targettype;
	uint32 casteranimation;
	uint32 casteranimation2;
	uint32 casterdisplay;
	uint32 display;
	uint32 deathdisplay;
	uint32 effect;
	uint32 value;
	uint32 triggerspell;
	uint32 triggervalue;
	uint32 maxtargets;
	uint32 amplitude;
	uint32 radius;
	uint32 castdelay;
	uint32 decaytime;
	uint32 duration;
	uint32 skillreq;
	uint32 skillreq2; //some have 2 (paladins and dark paladins, you FUCKS)s
	int32 forcedschool;
	uint32 forcedtriggered;
	uint32 damageflags;
};

struct SpellLifetime
{
	uint32 id;
	char* description;
	uint32 lifetime;
};
#pragma pack(pop)

extern SQLStore<SpellEntry> sSpellData;
extern SQLStore<SpellLifetime> sSpellLifetime;

struct SpellData
{
	uint32 id;
	uint32 school;
	uint32 manareq;
	uint32 skillreq;
	uint32 classlevel[13];
	std::string name;
};

class SpellStorage : public Singleton<SpellStorage>
{
public:
	unordered_map<uint32, SpellData*> m_data;
	uint32 m_maxspellId;

	SpellStorage();

	SpellData* GetSpell(uint32 id)
	{
		unordered_map<uint32, SpellData*>::iterator itr = m_data.find(id);
		if (itr != m_data.end())
			return itr->second;
		return NULL;
	}

	void Dump();

};

#define sSpellStorage SpellStorage::getSingleton()

struct TargetData
{
	uint32 flags;
	int32 x; //this is the id of the unit if flags & SPELL_TARGET_UNIT
	int32 y;
	uint8 mark;
};

class SpellEffect : public Object
{
public:
	uint8 m_visualId;
	uint16 m_casterid;
	uint16 m_targetid;
	uint8 m_effectType;
	bool m_hasdecayed;
	bool m_nocollide;
	bool m_didcollide;
	bool m_didcollidewithmap;
	G3D::Vector3 m_collisionHitPoint;
	uint32 m_creationTime;
	TargetData m_targets;
	SpellEntry* m_spell;
	uint32 m_damageflags;

	SpellEffect(Object* caster, uint8 spelltype, int32 x, int32 y, float rotation, Spell* spl, TargetData* targets = NULL);
	void Update();
	void CollideCheck();
	bool OnCollision(Object* o);
	void Decay();
	void BeamFire();
	void ObjectFireSpell();
	bool OnPortalCollide(Object* o);
	void SetVisual(uint32 v) { m_visualId = v; }

	void OnAddToWorld()
	{
		Object* caster = m_mapmgr->GetObject(m_casterid);
		if (caster != NULL)
			caster->m_summonedObjects.insert(m_serverId);
	}

	void OnPreRemoveFromWorld()
	{
		Object* caster = m_mapmgr->GetObject(m_casterid);
		if (caster != NULL)
			caster->m_summonedObjects.erase(m_serverId);
	}
};

class Spell
{
public:
	Unit* m_caster;
	SpellEntry* m_data;
	SpellEffect* m_triggerspelleffect;
	TargetData m_targets;
	bool m_triggered;
	uint32 m_forcedvalue;
	uint32 m_forcedradius;
	uint32 m_damageflags;
	Spell(Unit* caster, uint32 spellid, TargetData & targets, bool triggered = false, SpellEffect* triggeredfrom = NULL)
	{
		m_damageflags = 0;
		m_caster = caster;
		m_data = sSpellData.GetEntry(spellid);
		m_targets = targets;
		m_triggered = triggered;
		if (m_data != NULL && m_data->forcedtriggered)
			m_triggered = true;
		m_forcedvalue = 0;
		m_forcedradius = 0;
		m_triggerspelleffect = triggeredfrom;
		if (triggeredfrom != NULL && triggeredfrom->m_spell != NULL)
		{
			m_forcedvalue = triggeredfrom->m_spell->triggervalue;
			m_forcedradius = triggeredfrom->m_spell->radius;
		}
	}

	uint32 GetValue() { if (m_forcedvalue == 0) return m_data->value; return m_forcedvalue; }
	uint32 GetRadius() { if (m_forcedradius == 0) return m_data->radius; return m_forcedradius; }

	void Cast();

	void SpellEffectDummy(GrowableArray<Unit*> & targets) {}
	void SpellEffectDamage(GrowableArray<Unit*> & targets);

	void SpellEffectCreateProjectile(GrowableArray<Unit*> & targets);

	void SpellEffectCreateBeam(GrowableArray<Unit*> & targets);
	void SpellEffectCreateObject(GrowableArray<Unit*> & targets);
	void SpellEffectHeal(GrowableArray<Unit*> & targets);
	void SpellEffectApplyAura(GrowableArray<Unit*> & targets);

	void SpellEffectShortTeleport(GrowableArray<Unit*> & targets);
	void SpellEffectCreateBomb(GrowableArray<Unit*> & targets);

	void SpellEffectAttackRun(GrowableArray<Unit*> & targets);
	void SpellEffectMeleeAttack(GrowableArray<Unit*> & targets);
	void SpellEffectDispel(GrowableArray<Unit*> & targets);
	void SpellEffectMultiShot(GrowableArray<Unit*> & targets);
	void SpellEffectCreatePortal(GrowableArray<Unit*> & targets);
	void SpellEffectReditus(GrowableArray<Unit*> & targets);
	void SpellEffectResetStats(GrowableArray<Unit*> & targets);

	void SpellEffectSummonCreature(GrowableArray<Unit*> & targets);
	void SpellEffectCharm(GrowableArray<Unit*> & targets);
	void SpellEffectCurePosion(GrowableArray<Unit*> & targets);
	void SpellEffectInstaPort(GrowableArray<Unit*> & targets);
	void SpellEffectApplyAuraSelf(GrowableArray<Unit*> & targets);
	
	void SpellEffectApplyAreaAura(GrowableArray<Unit*> & targets);
	void SpellEffectApplyAreaAuraNeg(GrowableArray<Unit*> & targets);
	void SpellEffectDamageBasedLevel(GrowableArray<Unit*> & targets);
	void SpellEffectHealAll(GrowableArray<Unit*> & targets);
	void SpellEffectThrowWeapon(GrowableArray<Unit*> & targets);
	void SpellEffectThrowReturnWeapon(GrowableArray<Unit*> & targets);
};


#endif
