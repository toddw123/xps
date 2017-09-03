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

SQLStore<SpellAura> sSpellAuras;

#define SKILL_MOD_DURATION_CASTER(thing) \
	Unit* caster = GetCaster(); \
	if (caster) \
		ModifyDuration(thing); \
	else \
		ModifyDuration(1000);

SpellAuraHandler g_SpellAuraHandlers[NUM_SPELL_AURAS + 1] = 
{
	&Aura::SpellAuraDummy,
	&Aura::SpellAuraPoison,
	&Aura::SpellAuraTeuri,
	&Aura::SpellAuraExtrudere,
	&Aura::SpellAuraVelocitas,
	&Aura::SpellAuraRobur,
	&Aura::SpellAuraNightVision,
	&Aura::SpellAuraInvisibility,
	&Aura::SpellAuraWhitePotion,
	&Aura::SpellAuraMutatioNimbus,
	&Aura::SpellAuraMutatioFlamma,
	&Aura::SpellAuraNoxaTueri,
	&Aura::SpellAuraAlmus,
	&Aura::SpellAuraOmniMultum,
	&Aura::SpellAuraRagePotion,
	&Aura::SpellAuraStupify,
	&Aura::SpellAuraBattleScroll,
	&Aura::SpellAuraMultiProjectile,
	&Aura::SpellAuraKingAura,
	&Aura::SpellAuraDazeAura,
	&Aura::SpellAuraBleedAura,
	&Aura::SpellAuraFlameAura,
	&Aura::SpellAuraShadowAura,
	&Aura::SpellAuraOmniShield,
	&Aura::SpellAuraSureHit,
	&Aura::SpellAuraBerserk,
	&Aura::SpellAuraDodge,
	&Aura::SpellAuraIntensity,
	&Aura::SpellAuraHide,
	&Aura::SpellAuraInnerStrength,
	&Aura::SpellAuraStrength,
	&Aura::SpellAuraFocusedStrength,
	&Aura::SpellAuraFocusedAttack,
	&Aura::SpellAuraBreaker,
	&Aura::SpellAuraDamageShield,
	&Aura::SpellAuraSpellPower,
	&Aura::SpellAuraShadowPower,
	&Aura::SpellAuraLeech,
	&Aura::SpellAuraCorruptArmor,
	&Aura::SpellAuraDullSenses,
	&Aura::SpellAuraDisintegration,
	&Aura::SpellAuraCorruptArmorEffect,
	&Aura::SpellAuraDullSensesEffect,
	&Aura::SpellAuraDisintegrationEffect,
	&Aura::SpellAuraFierce,
};

Aura::Aura( Unit* caster, Unit* target, uint32 spellid, uint32 forcedvalue )
{
	appliedval = -1;
	m_charges = 0;
	m_areaaura = 0;
	m_forcedvalue = forcedvalue;
	m_dispelblock = false;
	m_target = target;
	m_casterid = caster->m_serverId;
	m_spell = sSpellData.GetEntry(spellid);
	ASSERT(m_spell);
	m_data = sSpellAuras.GetEntry(m_spell->triggerspell);
	ASSERT(m_data);

	//if the target has an aura in our slot remove it
	if (target->HasAura(m_data->slot) != NULL)
		target->m_auras[m_data->slot]->Remove();
	target->m_auras[m_data->slot] = this;

	if (m_data->duration > 0)
		AddEvent(&Aura::Remove, EVENT_AURA_REMOVE, m_data->duration, 1, EVENT_FLAG_NOT_IN_WORLD);
	
	//call handler
	(*this.*g_SpellAuraHandlers[m_data->effect])(true);
}

Unit* Aura::GetCaster()
{
	if (m_target->m_mapmgr == NULL)
		return NULL;
	Object* o = m_target->m_mapmgr->GetObject(m_casterid);
	if (o == NULL || !o->IsUnit())
		return NULL;
	return TO_UNIT(o);
}

void Aura::Remove()
{
	(*this.*g_SpellAuraHandlers[m_data->effect])(false);

	for (std::set<uint16>::iterator itr = m_areaauratargets.begin(); itr != m_areaauratargets.end(); ++itr)
	{
		Object* obj = m_target->m_mapmgr->GetObject(*itr);
		if (obj == NULL || !obj->IsUnit())
			continue;
		Unit* u = TO_UNIT(obj);
		if (!u->HasAura(m_data->slot))
			continue;
		u->m_auras[m_data->slot]->Remove();
	}

	m_target->m_auras[m_data->slot] = NULL;
	m_target = NULL;
	RemoveEvents();
	DecRef();
}

int32 Aura::GetInstanceID()
{
	if (m_target == NULL)
		return -1;
	return m_target->GetInstanceID();
}

void Aura::EventAuraPoison()
{
	Unit* caster = GetCaster();
	if (caster == NULL)
		return;
	uint32 poisondamage = GetValue();
	caster->DealDamage(m_target, poisondamage, m_spell, DAMAGE_FLAG_CANNOT_PROC | DAMAGE_FLAG_NO_REFLECT | DAMAGE_FLAG_IS_POISON_ATTACK);
}

void Aura::SpellAuraPoison( bool apply )
{
	if (apply)
	{
		m_target->SetBuffFlag(BUFF_FLAG_POISON);
		AddEvent(&Aura::EventAuraPoison, EVENT_AURA_EFFECT, 2000, 0, EVENT_FLAG_NOT_IN_WORLD);
	}
	else
		m_target->RemoveBuffFlag(BUFF_FLAG_POISON);
}

void Aura::SpellAuraTeuri( bool apply )
{
	if (apply)
	{
		appliedval = GetValue() * float(100 + m_target->m_bonuses[BONUS_ARMOR_SPELLS] + (m_target->GetSkillValue(SKILL_ARMOR) + m_target->GetSkillValue(SKILL_ARMOR2)) * 2) / 100;
		ApplyBonusesToValue(appliedval);
	}

	if (apply)
	{
		m_target->SetBuffFlag(BUFF_FLAG_TEURI);
		m_target->m_bonuses[PROTECTION_TEURI] += appliedval;
	}
	else
	{
		m_target->RemoveBuffFlag(BUFF_FLAG_TEURI);
		m_target->m_bonuses[PROTECTION_TEURI] -= appliedval;
	}
}

void Aura::SpellAuraExtrudere( bool apply )
{
	if (apply)
	{
		appliedval = GetValue() * float(100 + m_target->m_bonuses[BONUS_ARMOR_SPELLS] + (m_target->GetSkillValue(SKILL_ARMOR) + m_target->GetSkillValue(SKILL_ARMOR2)) * 2) / 100;
		ApplyBonusesToValue(appliedval);
	}

	uint16 eggflags = 0;
	if (appliedval >= 4 && appliedval <= 6)
		eggflags = AURA_FLAG_PINK_EGG;
	if (appliedval >= 7 && appliedval <= 11)
		eggflags = AURA_FLAG_PINK_EGG;
	if (appliedval >= 12 && appliedval <= 16)
		eggflags = AURA_FLAG_BLUE_EGG;
	if (appliedval >= 17 && appliedval <= 19)
		eggflags = AURA_FLAG_PINK_EGG | AURA_FLAG_BLUE_EGG;
	if (appliedval >= 20)
		eggflags = AURA_FLAG_PINK_EGG | AURA_FLAG_BLUE_EGG | AURA_FLAG_DIVINUS_TEURI;

	if (apply)
	{
		if (eggflags)
			m_target->SetSpellFlag(eggflags);
		m_target->SetBuffFlag(BUFF_FLAG_EXTRUDERE);
		m_target->m_bonuses[PROTECTION_EXTRUDERE] += appliedval;
	}
	else
	{
		m_target->RemoveBuffFlag(BUFF_FLAG_EXTRUDERE);
		m_target->RemoveSpellFlag(AURA_FLAG_DIVINUS_TEURI | AURA_FLAG_PINK_EGG | AURA_FLAG_BLUE_EGG);
		m_target->m_bonuses[PROTECTION_EXTRUDERE] -= appliedval;
	}
}

void Aura::SpellAuraVelocitas( bool apply )
{
	if (apply)
		m_target->SetBuffFlag(BUFF_FLAG_VELOCITAS);
	else
		m_target->RemoveBuffFlag(BUFF_FLAG_VELOCITAS);
}

void Aura::SpellAuraRobur( bool apply )
{
	if (apply)
		m_target->SetBuffFlag(BUFF_FLAG_ROBUR);
	else
		m_target->RemoveBuffFlag(BUFF_FLAG_ROBUR);
}

void Aura::SpellAuraNightVision( bool apply )
{
	if (apply)
		m_target->SetBuffFlag(BUFF_FLAG_NIGHT_VISION);
	else
		m_target->RemoveBuffFlag(BUFF_FLAG_NIGHT_VISION);

}

void Aura::SpellAuraInvisibility( bool apply )
{
	if (apply)
	{
		m_target->RemoveAura(SPELL_AURA_HIDE);
		m_target->SetSpellFlag(AURA_FLAG_INVISIBILITY);
	}
	else
		m_target->RemoveSpellFlag(AURA_FLAG_INVISIBILITY);

	m_target->AddEvent(&Unit::SendHealthUpdate, EVENT_SEND_HEALTHUPDATE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
}

void Aura::SpellAuraWhitePotion( bool apply )
{
	if (apply)
		m_target->SetSpellFlag(AURA_FLAG_WHITE_POTION);
	else
		m_target->RemoveSpellFlag(AURA_FLAG_WHITE_POTION);
}

void Aura::SpellAuraMutatioNimbus( bool apply )
{
	if (apply)
	{
		m_target->SetSpellFlag(AURA_FLAG_MUTATIO_NIMBUS);
		m_target->RemoveAura(SPELL_AURA_INVISIBILITY);
		m_target->RemoveAura(SPELL_AURA_HIDE);
		m_target->m_speed += 15;
	}
	else
	{
		m_target->RemoveSpellFlag(AURA_FLAG_MUTATIO_NIMBUS);
		m_target->m_speed -= 15;
	}
}

void Aura::SpellAuraMutatioFlamma( bool apply )
{
	if (apply)
	{
		m_target->SetSpellFlag(AURA_FLAG_MUTATIO_FLAMMA);
		m_target->RemoveAura(SPELL_AURA_INVISIBILITY);
		m_target->RemoveAura(SPELL_AURA_HIDE);
		m_target->m_speed += 15;
	}
	else
	{
		m_target->RemoveSpellFlag(AURA_FLAG_MUTATIO_FLAMMA);
		m_target->m_speed -= 15;
	}
}

void Aura::SpellAuraNoxaTueri( bool apply )
{
	if (apply)
		m_target->SetBuffFlag(BUFF_FLAG_NOXA);
	else
		m_target->RemoveBuffFlag(BUFF_FLAG_NOXA);
}

void Aura::SpellAuraAlmus( bool apply )
{
}

void Aura::ApplyBonusesToValue( int32 & val )
{
	Unit* caster = GetCaster();
	if (caster == NULL || m_spell == NULL)
		return;

	float bonus = 1;
	SpellData* dat = sSpellStorage.GetSpell(m_data->id);
	int32 school = -1;
	if (dat != NULL)
		school = dat->school;
	if (m_spell != NULL && m_spell->forcedschool != -1)
		school = m_spell->forcedschool;
	switch (school)
	{
	case SPELL_SCHOOL_FIRE:
		{
			bonus = caster->GetMagicBonus(FIRE_MAGIC_BONUS);
		} break;
	case SPELL_SCHOOL_ICE:
		{
			bonus = caster->GetMagicBonus(ICE_MAGIC_BONUS);
		} break;
	case SPELL_SCHOOL_ENERGY:
		{
			bonus = caster->GetMagicBonus(ENERGY_MAGIC_BONUS);
		} break;
	case SPELL_SCHOOL_SHADOW:
		{
			bonus = caster->GetMagicBonus(SHADOW_MAGIC_BONUS);
		} break;
	case SPELL_SCHOOL_SPIRIT:
		{
			bonus = caster->GetMagicBonus(SPIRIT_MAGIC_BONUS);
		} break;
	}
	val *= bonus;
}

void Aura::SpellAuraOmniMultum( bool apply )
{
	if (apply)
	{
		appliedval = GetValue() * float(100 + m_target->m_bonuses[BONUS_ARMOR_SPELLS] + (m_target->GetSkillValue(SKILL_ARMOR) + m_target->GetSkillValue(SKILL_ARMOR2)) * 2) / 100;
		ApplyBonusesToValue(appliedval);
	}

	if (apply)
	{
		m_target->SetBuffFlag(BUFF_FLAG_OMNIMULTIM);
		m_target->m_bonuses[PROTECTION_TEURI] += appliedval;
		m_target->m_bonuses[PROTECTION_EXTRUDERE] += (appliedval / 8);
	}
	else
	{
		m_target->RemoveBuffFlag(BUFF_FLAG_OMNIMULTIM);
		m_target->m_bonuses[PROTECTION_TEURI] -= appliedval;
		m_target->m_bonuses[PROTECTION_EXTRUDERE] -= (appliedval / 8);
	}
}

void Aura::SpellAuraRagePotion( bool apply )
{
	if (apply)
		m_target->SetBuffFlag(BUFF_FLAG_RED_POTION);
	else
		m_target->RemoveBuffFlag(BUFF_FLAG_RED_POTION);
}

void Aura::SpellAuraStupify( bool apply )
{
	if (apply)
		m_target->SetBuffFlag(BUFF_FLAG_STUPIFY);
	else
		m_target->RemoveBuffFlag(BUFF_FLAG_STUPIFY);
}

void Aura::SpellAuraBattleScroll( bool apply )
{
	if (apply)
		m_target->SetBuffFlag(BUFF_FLAG_BATTLE_SCROLL);
	else
		m_target->RemoveBuffFlag(BUFF_FLAG_BATTLE_SCROLL);
}

void Aura::EventAuraMultiProjectile()
{
	Unit* caster = GetCaster();
	if (caster == NULL || !caster->IsAlive())
		return;
	float rot = (caster->m_rotation - 90 + RandomUInt(50) - 25) * PI / 180.0f;
	//so we target this location
	int32 x = caster->m_positionX - sinf(rot) * 10; 
	int32 y = caster->m_positionY + cosf(rot) * 10;

	TargetData tar;
	tar.flags = SPELL_TARGET_GROUND;
	tar.x = x;
	tar.y = y;
	Spell s(caster, m_spell->value, tar, true);
	s.Cast();
}

void Aura::SpellAuraMultiProjectile( bool apply )
{
	Unit* caster = GetCaster();
	if (caster == NULL)
		return;
	//so do we have the correct bonuses?

	if (caster->IsPlayer())
	{
		if (m_spell->id >= 362)
		{
			if (caster->m_bonuses[ICE_DRAGON_STAFF] == 0)
				return;
		}
		else
		{
			if (caster->m_bonuses[FIRE_DRAGON_STAFF] == 0)
				return;

		}
	}

	if (apply)
		AddEvent(&Aura::EventAuraMultiProjectile, EVENT_AURA_EFFECT, 200, 5, EVENT_FLAG_NOT_IN_WORLD);
}

void Aura::SpellAuraKingAura( bool apply )
{
	if (apply)
	{
		m_target->m_bonuses[ALL_MAGIC_BONUS] += 10;
		m_target->m_bonuses[BONUS_DAMAGE_PCT] += 10;
	}
	else
	{
		m_target->m_bonuses[ALL_MAGIC_BONUS] -= 10;
		m_target->m_bonuses[BONUS_DAMAGE_PCT] -= 10;
	}
}

void Aura::SpellAuraDazeAura( bool apply )
{
	if (apply)
		m_target->SetSkillFlag(SKILL_FLAG_DAZED);
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_DAZED);
}

void Aura::SpellAuraBleedAura( bool apply )
{
	if (apply)
	{
		m_target->SetSkillFlag(SKILL_FLAG_BLEED);
		AddEvent(&Aura::EventAuraPoison, EVENT_AURA_EFFECT, 400, 4, EVENT_FLAG_NOT_IN_WORLD);
	}
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_BLEED);
}

void Aura::UpdateAreaAura()
{
	Unit* caster = GetCaster();
	if (caster == NULL || m_spell == NULL)
		return;
	for (std::set<uint16>::iterator itr = m_areaauratargets.begin(); itr != m_areaauratargets.end(); ++itr)
	{
		Object* obj = m_target->m_mapmgr->GetObject(*itr);
		if (obj == NULL || !obj->IsUnit())
			continue;
		Unit* u = TO_UNIT(obj);
		if (!u->HasAura(m_data->slot))
			continue;
		bool remove = false;
		if (m_areaaura == AREA_AURA_FRIENDLY)
		{
			if (m_target->IsPlayer())
			{
				if (!u->IsPlayer())
					remove = true;
				if (TO_PLAYER(m_target)->GetLinkID() == 0 || TO_PLAYER(u)->GetLinkID() != TO_PLAYER(m_target)->GetLinkID())
					remove = true;
			}
			else if (m_target->IsHostile(u))
				remove = true;
		}	
		if (m_areaaura == AREA_AURA_HOSTILE && !m_target->IsHostile(u))
			remove = true;
		if (m_target->GetDistance(u) > m_spell->radius)
			remove = true;

		if (remove)
			u->m_auras[m_data->slot]->Remove();
	}

	//don't update inactive creatures
	if (!m_target->IsPlayer() && !m_target->m_active)
		return;

	for (std::map<Unit*, uint8>::iterator itr = m_target->m_inrangeUnits.begin(); itr != m_target->m_inrangeUnits.end(); ++itr)
	{
		if (itr->first->HasAura(m_data->slot) || itr->first->HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
			continue;
		if (m_target->GetDistance(itr->first) > m_spell->radius)
			continue;
		if (m_areaaura == AREA_AURA_FRIENDLY)
		{
			if (m_target->IsPlayer())
			{
				if (!itr->first->IsPlayer())
					continue;
				if (TO_PLAYER(m_target)->GetLinkID() == 0 || TO_PLAYER(itr->first)->GetLinkID() != TO_PLAYER(m_target)->GetLinkID())
					continue;
			}
			else if (m_target->IsHostile(itr->first))
				continue;
		}
		if (m_areaaura == AREA_AURA_HOSTILE && !m_target->IsHostile(itr->first))
			continue;
		new Aura(caster, itr->first, m_spell->id, m_forcedvalue);
		m_areaauratargets.insert(itr->first->m_serverId);
	}
}

void Aura::SetAreaAura( uint8 type )
{
	m_areaaura = type;
	AddEvent(&Aura::UpdateAreaAura, EVENT_AURA_EFFECT, 500, 0, EVENT_FLAG_NOT_IN_WORLD);
}

void Aura::SpellAuraFlameAura( bool apply )
{
	if (apply)
	{
		m_target->RemoveAura(SPELL_AURA_SHADOW_AURA);
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_FLAME_AURA) * 3000 + 200);
		AddEvent(&Aura::EventFlameAura, EVENT_AURA_EFFECT, 1000, 0, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void Aura::EventFlameAura()
{
	m_target->CastSpell(15036, true);
}

void Aura::SpellAuraShadowAura( bool apply )
{
	if (apply)
	{
		m_target->RemoveAura(SPELL_AURA_FLAME_AURA);
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_SHADOW_AURA) * 2000 + 200);
		AddEvent(&Aura::EventShadowAura, EVENT_AURA_EFFECT, 1000, 0, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void Aura::EventShadowAura()
{
	m_target->CastSpell(15037, true);
}

void Aura::SpellAuraOmniShield( bool apply )
{
	if (apply)
	{
		m_target->SetSkillFlag(SKILL_FLAG_OMNI_SHIELD);
		m_charges = m_target->GetSkillValue(SKILL_OMNI_SHIELD);
	}
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_OMNI_SHIELD);
}

void Aura::SpellAuraHide( bool apply )
{
	if (apply)
	{
		bool can_hide = false;
		int32 cellx = m_target->m_positionX / 20;
		int32 celly = m_target->m_positionY / 20;

		for (int32 y = -1; y < 2; ++y)
		{
			for (int32 x = -1; x < 2; ++x)
			{
				if (!m_target->m_mapmgr->CanPass(cellx + x, celly + y))
					can_hide = true;
				if (m_target->m_mapmgr->GetMapLayer2((cellx + x) * 20, (celly + y) * 20) == 46) //square
				{
					can_hide = false;
					goto ENDHIDECHECK;
				}
			}
		}

ENDHIDECHECK:

		if (!can_hide)
		{
			ModifyDuration(0); //remove the aura and don't activate it's effects
			return;
		}
		m_target->RemoveAura(SPELL_AURA_INVISIBILITY);
		m_target->SetSpellFlag(AURA_FLAG_HIDE_SKILL);
		//remove the skill message say
		m_target->RemoveUpdateBlock(UPDATETYPE_CHATMSG);
	}
	else
		m_target->RemoveSpellFlag(AURA_FLAG_HIDE_SKILL);

	m_target->AddEvent(&Unit::SendHealthUpdate, EVENT_SEND_HEALTHUPDATE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
}

void Aura::SpellAuraSureHit( bool apply )
{
	m_charges = m_target->GetSkillValue(SKILL_SURE_HIT);
}

void Aura::SpellAuraDodge( bool apply )
{
	m_charges = m_target->GetSkillValue(SKILL_DODGE);
}

void Aura::SpellAuraBerserk( bool apply )
{
	if (apply)
	{
		m_target->SetSkillFlag(SKILL_FLAG_BERSERK);
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_BERSERK) * 3000);
	}
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_BERSERK);
}

void Aura::SpellAuraIntensity( bool apply )
{
	if (apply)
	{
		m_target->SetSkillFlag(SKILL_FLAG_INTENSITY);
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_INTENSITY) * 3000);
	}
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_INTENSITY);
}

void Aura::SpellAuraInnerStrength( bool apply )
{
	if (apply)
	{
		m_target->SetSkillFlag(SKILL_FLAG_INNER_STRENGTH);
		SKILL_MOD_DURATION_CASTER((caster->GetSkillValue(SKILL_INNER_STRENGTH) + caster->GetSkillValue(SKILL_INNER_STRENGTH2)) * 3000);
		m_target->m_bonuses[BONUS_DAMAGE_PCT] += 10;
	}
	else
	{
		m_target->RemoveSkillFlag(SKILL_FLAG_INNER_STRENGTH);
		m_target->m_bonuses[BONUS_DAMAGE_PCT] -= 10;
	}
}

void Aura::SpellAuraFocusedStrength( bool apply )
{
	if (apply)
	{
		m_target->SetSkillFlag(SKILL_FLAG_FOCUSED_STRENGTH);
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_FOCUSED_STR) * 3000);
		m_target->m_bonuses[PROTECTION_ALL_MAGIC] += 20;
		m_target->m_bonuses[BONUS_DAMAGE_PCT] += 10;
	}
	else
	{
		m_target->RemoveSkillFlag(SKILL_FLAG_FOCUSED_STRENGTH);
		m_target->m_bonuses[PROTECTION_ALL_MAGIC] -= 20;
		m_target->m_bonuses[BONUS_DAMAGE_PCT] -= 10;

	}
}

void Aura::SpellAuraStrength( bool apply )
{
	if (apply)
		m_charges = m_target->GetSkillValue(SKILL_STRENGTH);
}

void Aura::SpellAuraFocusedAttack( bool apply )
{
	if (apply)
		m_charges = m_target->GetSkillValue(SKILL_FOCUSED_ATTACK);
}

void Aura::SpellAuraBreaker( bool apply )
{
	if (apply)
	{
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_BREAKER) * 3000);
		m_target->SetSkillFlag(SKILL_FLAG_BREAKER);
		m_target->m_bonuses[PROTECTION_ALL_MAGIC] += 20;
	}
	else
	{
		m_target->RemoveSkillFlag(SKILL_FLAG_BREAKER);
		m_target->m_bonuses[PROTECTION_ALL_MAGIC] -= 20;
	}
}

void Aura::SpellAuraDamageShield( bool apply )
{
	if (apply)
	{
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_DAMAGE_SHIELD) * 60 * 1000);
		m_target->SetSkillFlag(SKILL_FLAG_DAMAGE_SHIELD);
	}
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_DAMAGE_SHIELD);
}

void Aura::SpellAuraSpellPower( bool apply )
{
	if (apply)
		m_charges = 3;
}

void Aura::SpellAuraShadowPower( bool apply )
{
	if (apply)
	{
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_SHADOW_POWER) * 60 * 1000);
		m_target->SetSkillFlag(SKILL_FLAG_SHADOW_POWER);
	}
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_SHADOW_POWER);
}

void Aura::SpellAuraLeech( bool apply )
{
	if (apply)
		m_charges = 3;
}

void Aura::SpellAuraCorruptArmor( bool apply )
{
	if (apply)
	{
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_CORRUPT_ARMOR) * 60 * 1000);
		m_target->SetSkillFlag(SKILL_FLAG_CORRUPT_ARMOR);
	}
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_CORRUPT_ARMOR);
}

void Aura::SpellAuraDullSenses( bool apply )
{
	if (apply)
	{
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_DULL_SENSES) * 60 * 1000);
		m_target->SetSkillFlag(SKILL_FLAG_DULL_SENSES);
	}
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_DULL_SENSES);
}

void Aura::SpellAuraDisintegration( bool apply )
{
	if (apply)
	{
		SKILL_MOD_DURATION_CASTER(caster->GetSkillValue(SKILL_DISINTEGRATION) * 60 * 1000);
		m_target->SetSkillFlag(SKILL_FLAG_SHADOW_POWER);
	}
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_SHADOW_POWER);
}

void Aura::EventAuraDisintegration()
{
	Unit* caster = GetCaster();
	if (caster == NULL)
		return;
	uint32 damage = m_target->m_maxhealth * 0.10f;
	if (m_target->IsCreature() && TO_CREATURE(m_target)->m_proto->boss == 1)
		damage = 500;
	caster->DealDamage(m_target, damage, NULL, DAMAGE_FLAG_DO_NOT_APPLY_BONUSES | DAMAGE_FLAG_DO_NOT_APPLY_RESISTS | DAMAGE_FLAG_NO_REFLECT | DAMAGE_FLAG_CANNOT_PROC);
}

void Aura::SpellAuraCorruptArmorEffect( bool apply )
{
	if (apply)
		m_target->SetSkillFlag(SKILL_FLAG_CORRUPT_ARMOR2);
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_CORRUPT_ARMOR2);
}

void Aura::SpellAuraDullSensesEffect( bool apply )
{
	if (apply)
		m_target->SetSkillFlag(SKILL_FLAG_DULL_SENSES2);
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_DULL_SENSES2);
}

void Aura::SpellAuraDisintegrationEffect(bool apply)
{
	if (apply)
	{
		AddEvent(&Aura::EventAuraDisintegration, EVENT_AURA_EFFECT, 1000, 0, EVENT_FLAG_NOT_IN_WORLD);
		m_target->SetSkillFlag(SKILL_FLAG_DISINTEGRATION);
	}
	else
		m_target->RemoveSkillFlag(SKILL_FLAG_DISINTEGRATION);
}

void Aura::SpellAuraFierce(bool apply)
{
	if (apply)
	{
		if (m_target->IsPlayer())
		{
			if (!TO_PLAYER(m_target)->ModCurrency(CURRENCY_FIERCE_USES, -1))
			{
				ModifyDuration(0);
				return;
			}

			auto p = TO_PLAYER(m_target);
			p->AddEvent(&Player::UpdateBodyArmorVisual, EVENT_UNKNOWN, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
		}
		appliedval = 10;
		m_target->m_bonuses[BONUS_DAMAGE_PCT] += appliedval;
		m_target->m_bonuses[ALL_MAGIC_BONUS] += appliedval;
	}
	else
	{
		if (m_target->IsPlayer())
		{
			auto p = TO_PLAYER(m_target);
			p->AddEvent(&Player::UpdateBodyArmorVisual, EVENT_UNKNOWN, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
		}

		if (appliedval != -1)
		{
			m_target->m_bonuses[BONUS_DAMAGE_PCT] -= appliedval;
			m_target->m_bonuses[ALL_MAGIC_BONUS] -= appliedval;
		}
	}
}
