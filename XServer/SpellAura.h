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

#ifndef __SPELLAURA_H
#define __SPELLAURA_H

#pragma pack(push, 1)
struct SpellAura
{
	uint32 id;
	char* name;
	uint32 slot;
	uint32 effect;
	uint32 duration;
	uint32 dispellable;
	uint32 canoverride;
};
#pragma pack(pop)

extern SQLStore<SpellAura> sSpellAuras;

enum SpellAuras
{
	SPELL_AURA_POISON,
	SPELL_AURA_TEURI,
	SPELL_AURA_EXTRUDERE,
	SPELL_AURA_VELOCITAS,
	SPELL_AURA_ROBUR,
	SPELL_AURA_NIGHT_VISION,
	SPELL_AURA_INVISIBILITY,
	SPELL_AURA_WHITE_POT,
	SPELL_AURA_MUTATIO_NIMBUS,
	SPELL_AURA_MUTATIO_FLAMMA,
	SPELL_AURA_NOXA_TUERI,
	SPELL_AURA_ALMUS,
	SPELL_AURA_OMNIMULTUM,
	SPELL_AURA_RAGE_POTION,
	SPELL_AURA_STUPIFY,
	SPELL_AURA_BATTLE_SCROLL,
	SPELL_AURA_MULTI_PROJECTILE,
	SPELL_AURA_KING_AURA,
	SPELL_AURA_DAZE_AURA,
	SPELL_AURA_BLEED_AURA,
	SPELL_AURA_FLAME_AURA,
	SPELL_AURA_SHADOW_AURA,
	SPELL_AURA_OMNI_SHIELD,
	SPELL_AURA_SURE_HIT,
	SPELL_AURA_BERSERK,
	SPELL_AURA_DODGE,
	SPELL_AURA_INTENSITY,
	SPELL_AURA_HIDE,
	SPELL_AURA_INNER_STRENGTH,
	SPELL_AURA_STRENGTH,
	SPELL_AURA_FOCUSED_STRENGTH,
	SPELL_AURA_FOCUSED_ATTACK,
	SPELL_AURA_BREAKER,
	SPELL_AURA_DAMAGE_SHIELD,
	SPELL_AURA_SPELL_POWER,
	SPELL_AURA_SHADOW_POWER,
	SPELL_AURA_LEECH,
	SPELL_AURA_CORRUPT_ARMOR,
	SPELL_AURA_DULL_SENSES,
	SPELL_AURA_DISINTEGRATION,
	SPELL_AURA_CORRUPT_ARMOR_EFFECT,
	SPELL_AURA_DULL_SENSES_EFFECT,
	SPELL_AURA_DISINTEGRATION_EFFECT,
	SPELL_AURA_FIERCE,
	NUM_SPELL_AURAS,
};

enum AreaAuraType
{
	AREA_AURA_NONE,
	AREA_AURA_FRIENDLY,
	AREA_AURA_HOSTILE,
};

class Aura;
typedef void(Aura::*SpellAuraHandler)(bool apply);

extern SpellAuraHandler g_SpellAuraHandlers[NUM_SPELL_AURAS + 1];

class Aura : public EventableObject
{
public:
	uint16 m_casterid;
	Unit* m_target;
	SpellEntry* m_spell;
	SpellAura* m_data;
	bool m_dispelblock;
	int32 appliedval;
	uint32 m_forcedvalue;
	uint8 m_areaaura;
	std::set<uint16> m_areaauratargets;
	uint32 m_charges;
	Aura(Unit* caster, Unit* target, uint32 spellid, uint32 forcedvalue = 0);

	~Aura()
	{
		RemoveEvents();
	}

	Unit* GetCaster();
	int32 GetInstanceID();

	uint32 GetValue() { if (m_forcedvalue == 0) return m_spell->value; return m_forcedvalue; }
	void ApplyBonusesToValue(int32 & val);

	void ModifyDuration(uint32 newduration)
	{
		RemoveEvents(EVENT_AURA_REMOVE);
		AddEvent(&Aura::Remove, EVENT_AURA_REMOVE, newduration, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void RemoveCharge() { --m_charges; if (m_charges == 0) Remove(); }

	void Remove();

	void SetAreaAura(uint8 type);
	void UpdateAreaAura();

	//ok auras
	void SpellAuraDummy(bool apply) {}
	void SpellAuraPoison(bool apply);
	void SpellAuraTeuri(bool apply);
	void SpellAuraExtrudere(bool apply);
	void SpellAuraVelocitas(bool apply);
	void SpellAuraRobur(bool apply);
	void SpellAuraNightVision(bool apply);
	void SpellAuraInvisibility(bool apply);
	void SpellAuraWhitePotion(bool apply);
	void SpellAuraMutatioNimbus(bool apply);
	void SpellAuraMutatioFlamma(bool apply);
	void SpellAuraNoxaTueri(bool apply);
	void SpellAuraAlmus(bool apply);
	void SpellAuraOmniMultum(bool apply);
	void SpellAuraRagePotion(bool apply);
	void SpellAuraStupify(bool apply);
	void SpellAuraBattleScroll(bool apply);
	void SpellAuraMultiProjectile(bool apply);
	void SpellAuraKingAura(bool apply);
	void SpellAuraDazeAura(bool apply);
	void SpellAuraBleedAura(bool apply);

	void EventAuraMultiProjectile();
	void EventAuraPoison();



	//skill stuff
	void SpellAuraFlameAura(bool apply);
	void EventFlameAura();

	void SpellAuraShadowAura(bool apply);
	void EventShadowAura();

	void SpellAuraOmniShield(bool apply);
	void SpellAuraSureHit(bool apply);
	void SpellAuraBerserk(bool apply);
	void SpellAuraDodge(bool apply);
	void SpellAuraIntensity(bool apply);
	void SpellAuraHide(bool apply);

	void SpellAuraInnerStrength(bool apply);
	void SpellAuraStrength(bool apply);
	void SpellAuraFocusedStrength(bool apply);

	void SpellAuraFocusedAttack(bool apply);
	void SpellAuraBreaker(bool apply);
	void SpellAuraDamageShield(bool apply);

	void SpellAuraSpellPower(bool apply);
	void SpellAuraShadowPower(bool apply);
	void SpellAuraLeech(bool apply);

	void SpellAuraCorruptArmor(bool apply);
	void SpellAuraDullSenses(bool apply);
	void SpellAuraDisintegration(bool apply);
	void SpellAuraCorruptArmorEffect(bool apply);
	void SpellAuraDullSensesEffect(bool apply);
	void SpellAuraDisintegrationEffect(bool apply);

	void SpellAuraFierce(bool apply);

	void EventAuraDisintegration();
};

#endif
