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

SQLStore<SpellEntry> sSpellData;
SQLStore<SpellLifetime> sSpellLifetime;

initialiseSingleton(SpellStorage);

SpellEffectHandler g_SpellEffectHandlers[NUM_SPELL_EFFECTS + 1] =
{
	&Spell::SpellEffectDummy,
	&Spell::SpellEffectDamage,
	&Spell::SpellEffectCreateProjectile,
	&Spell::SpellEffectCreateBeam,
	&Spell::SpellEffectCreateObject,
	&Spell::SpellEffectHeal,
	&Spell::SpellEffectApplyAura,
	&Spell::SpellEffectShortTeleport,
	&Spell::SpellEffectCreateBomb,
	&Spell::SpellEffectAttackRun,
	&Spell::SpellEffectMeleeAttack,
	&Spell::SpellEffectDispel,
	&Spell::SpellEffectMultiShot,
	&Spell::SpellEffectCreatePortal,
	&Spell::SpellEffectReditus,
	&Spell::SpellEffectResetStats,
	&Spell::SpellEffectSummonCreature,
	&Spell::SpellEffectCharm,
	&Spell::SpellEffectCurePosion,
	&Spell::SpellEffectInstaPort,
	&Spell::SpellEffectApplyAuraSelf,
	&Spell::SpellEffectApplyAura,
	&Spell::SpellEffectApplyAreaAura,
	&Spell::SpellEffectApplyAreaAuraNeg,
	&Spell::SpellEffectDamageBasedLevel,
	&Spell::SpellEffectHealAll,
	&Spell::SpellEffectThrowWeapon,
	&Spell::SpellEffectThrowReturnWeapon,
};

SpellEffect::SpellEffect(Object* caster, uint8 spelltype, int32 x, int32 y, float rotation, Spell* spl, TargetData* targets)
{
	m_damageflags = 0;
	m_nocollide = false;
	m_creationTime = GetMSTime();
	m_hasdecayed = false;
	m_effectType = spelltype;
	m_casterid = caster->m_serverId;
	m_targetid = 0;
	m_positionX = x;
	m_positionY = y;
	m_rotation = rotation;
	m_typeId = TYPEID_SPELLEFFECT;
	m_didcollide = false;
	m_didcollidewithmap = false;

	SpellEntry* sp = NULL;
	if (spl != NULL)
	{
		sp = spl->m_data;
		m_damageflags = spl->m_damageflags;
	}

	m_spell = sp;

	if (sp != NULL)
	{
		if (sp->decaytime != 0)
			AddEvent(&SpellEffect::Decay, EVENT_SPELLEFFECT_DECAY, sp->decaytime, 1, EVENT_FLAG_NOT_IN_WORLD);

		//lookup lifetime
		if (sp->duration != 0)
		{
			AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, sp->duration, 1, EVENT_FLAG_NOT_IN_WORLD);
		}
		else
		{
			SpellLifetime* lt = sSpellLifetime.GetEntry(sp->display);
			if (lt != NULL)
				AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, lt->lifetime, 1, EVENT_FLAG_NOT_IN_WORLD);
		}

	}

	if (targets != NULL)
		m_targets = *targets;
}

void SpellEffect::Update()
{
	switch (m_effectType)
	{
	case SPELL_TYPE_PORTAL: CollideCheck(); break;
	case SPELL_TYPE_PROJECTILE:
		{
			if (!m_hasdecayed)
			{
				Object* target = NULL;
				if (m_targetid) //things like arrows, "follow target dumbly"
				{
					target = m_mapmgr->GetObject(m_targetid);
					if (target != NULL)
						m_rotation = CalcFacingDeg(target->m_positionX, target->m_positionY) - 90;
				}
				float rot = m_rotation * PI / 180.0f;
				float speed = 22.0f;
				int32 newx = m_positionX - int16(sinf(rot) * speed);
				int32 newy = m_positionY + int16(cosf(rot) * speed);

				if (!m_nocollide && !m_mapmgr->InLineOfSight(m_positionX, m_positionY, newx, newy, PASS_FLAG_WALK, NULL, &m_collisionHitPoint))
				{
					m_didcollidewithmap = true;
					SetPosition(m_collisionHitPoint.x, m_collisionHitPoint.y);
					Decay();
					return;
				}

				if (target != NULL && GetDistance(target) <= GetDistance(newx, newy, target->m_positionX, target->m_positionY))
				{
					OnCollision(target);
					return;
				}

				SetPosition(newx, newy);
			}
		} break;
	case SPELL_TYPE_BEAM:
		{
			//this is special, we move to the caster's position
			Object* caster = m_mapmgr->GetObject(m_casterid);
			if (caster != NULL && (caster->m_positionX != m_positionX || caster->m_positionY != m_positionY))
				SetPosition(caster->m_positionX, caster->m_positionY);
		}
	}
}

void SpellEffect::CollideCheck()
{
	if (m_effectType != SPELL_TYPE_PROJECTILE && m_effectType != SPELL_TYPE_PORTAL)
		return;
	for (std::map<Unit*, uint8>::iterator itr=m_inrangeUnits.begin(); itr!=m_inrangeUnits.end(); ++itr)
		if (GetDistance(itr->first) < m_bounding && itr->first->IsAlive())
		{
			if (m_effectType == SPELL_TYPE_PORTAL)
			{
				if (OnPortalCollide(itr->first))
					return;
			}
			else
			{
				if (OnCollision(itr->first))
					return;
			}
		}
}

bool SpellEffect::OnCollision( Object* o )
{
	if (m_hasdecayed)
		return false;
	if (m_didcollide)
		return false;
	if (m_didcollidewithmap)
		return false;
	if (m_effectType != SPELL_TYPE_PROJECTILE || m_spell == NULL || !o->IsUnit())
		return false;
	if (o->m_serverId == m_casterid && m_targetid != m_casterid)
		return false;
	if (m_targetid != 0 && o->m_serverId != m_targetid)
		return false;
	Object* caster = m_mapmgr->GetObject(m_casterid);
	if (caster == NULL || !caster->IsUnit())
		return false;
	Unit* u = TO_UNIT(o);
	Unit* ucaster = NULL;
	if (caster != NULL && caster->IsUnit())
		ucaster = TO_UNIT(caster);
	if (u->IsPet() && TO_CREATURE(u)->GetOwner() == caster && m_targetid != u->m_serverId)
		return false;
	if (u->IsPlayer() && TO_PLAYER(u)->m_invis && m_targetid != u->m_serverId)
		return false;
	if (!u->IsAlive())
		return false;
	

	if (m_targetid == 0) //freefire casts shouldn't hit friendlies
	{
		uint32 reaction = ucaster->GetReaction(u);

		if (reaction != REACTION_NEUTRAL_ATTACKABLE && reaction != REACTION_HOSTILE)
			return false;
	}

	if (u->m_bonuses[REFLECT_PROJECTILE] && RandomUInt(99) < u->m_bonuses[REFLECT_PROJECTILE])
	{
		m_rotation += 180;
		if (m_rotation > 360)
			m_rotation -= 360;
		if (m_targetid != 0)
			m_targetid = m_casterid;
		return true;
	}

	m_didcollide = true;

	//cast collide spell onto target
	SpellEntry* sp = sSpellData.GetEntry(m_spell->triggerspell);
	if (sp != NULL)
	{
		TargetData t;
		t.flags = SPELL_TARGET_UNIT;
		t.x = u->m_serverId;
		Spell s((Unit*)caster, sp->id, t, true, this);
		s.m_damageflags = m_damageflags;
		s.Cast();

		if (m_spell->id == 15002 || m_spell->id == 15003) //bow projectile spells
		{
			if (ucaster != NULL)
			{
				Item* weapon = ucaster->GetCurrentWeapon();

				if (weapon != NULL && weapon->m_data->type == ITEM_TYPE_BOW && ucaster->m_bonuses[BONUS_EXPLODING_ARROWS] > 0)
					ucaster->CastSpell(15053, u->m_positionX, u->m_positionY, true);
			}
		}
	}

	//turn our visual into death visual
	Decay();
	return true;
}

void SpellEffect::Decay()
{
	if (m_hasdecayed)
		return;
	m_hasdecayed = true;

	if (m_spell == NULL)
	{
		RemoveEvents(EVENT_OBJECT_DELETE);
		AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
		return;
	}

	if (!m_didcollide && m_didcollidewithmap)
	{
		if (m_spell->id == 15002 || m_spell->id == 15003) //bow projectile spells
		{
			Object* caster = m_mapmgr->GetObject(m_casterid);
			if (caster != NULL && caster->IsUnit())
			{
				Unit* ucaster = TO_UNIT(caster);

				Item* weapon = ucaster->GetCurrentWeapon();

				if (weapon != NULL && weapon->m_data->type == ITEM_TYPE_BOW && ucaster->m_bonuses[BONUS_EXPLODING_ARROWS] > 0)
				{
					ucaster->CastSpell(15053, m_positionX, m_positionY, true);
				}
			}
		}
	}

	if (m_spell->effect == SPELL_EFFECT_THROW_WEAPON)
	{
		//we need to create a new one targetting caster lol
		Object* caster = m_mapmgr->GetObject(m_casterid);
		if (caster != NULL && caster->IsUnit())
		{
			TargetData targs;
			targs.flags |= SPELL_TARGET_GROUND;
			targs.x = m_positionX;
			targs.y = m_positionY;
			Spell retspell(TO_UNIT(caster), 15045, targs, true);
			retspell.m_forcedvalue = m_visualId;
			retspell.Cast();
		}
	}

	if (m_spell->effect == SPELL_EFFECT_THROW_RETURN_WEAPON)
	{
		Object* caster = m_mapmgr->GetObject(m_casterid);
		if (caster != NULL && caster->IsUnit())
			TO_UNIT(caster)->SetThrownWeapon(0);
	}


	m_visualId = m_spell->deathdisplay;
	//remake event
	RemoveEvents(EVENT_OBJECT_DELETE);
	AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
}

void SpellEffect::BeamFire()
{
	if (m_spell == NULL || m_spell->triggerspell == 0)
	{
		Delete();
		return;
	}
	Object* caster = m_mapmgr->GetObject(m_casterid);
	Object* target = m_mapmgr->GetObject(m_targetid);
	if (caster == NULL || !caster->IsUnit() || !TO_UNIT(caster)->IsAlive() || target == NULL || !target->IsUnit() || (caster != target && !caster->IsInRange(target)))
	{
		Delete();
		return;
	}
	SpellEntry* sp = sSpellData.GetEntry(m_spell->triggerspell);
	if (sp == NULL)
	{
		Delete();
		return;
	}
	TargetData t;
	t.flags = SPELL_TARGET_UNIT;
	t.x = target->m_serverId;
	Spell s((Unit*)caster, sp->id, t, true, this);
	s.m_damageflags = m_damageflags;
	s.Cast();
	//now delete
	Delete();
}

void SpellEffect::ObjectFireSpell()
{
	if (m_spell == NULL || m_spell->triggerspell == 0)
		return;
	Object* caster = m_mapmgr->GetObject(m_casterid);
	if (caster == NULL || !caster->IsUnit())
		return;
	SpellEntry* sp = sSpellData.GetEntry(m_spell->triggerspell);
	if (sp == NULL)
		return;
	TargetData t;
	t.flags = SPELL_TARGET_GROUND;
	t.x = m_positionX;
	t.y = m_positionY;
	Spell s((Unit*)caster, sp->id, t, true, this);
	s.m_damageflags = m_damageflags;
	s.Cast();
}

bool SpellEffect::OnPortalCollide( Object* o )
{
	if (m_creationTime >= GetMSTime() - 5000)
		return false; //takes 5secs to open

	Object* caster = m_mapmgr->GetObject(m_casterid);

	if (caster == NULL || !caster->IsPlayer())
		return false;

	if (!o->IsPlayer())
		return false;
	Player* u = TO_PLAYER(o);

	if (!u->IsAlive())
		return false;

	if (u->m_lastSay != "-port" && u != caster)
		return false;
	Player* pcaster = TO_PLAYER(caster);
	if (pcaster->m_marks[m_targets.mark].x != 0 && pcaster->m_marks[m_targets.mark].y != 0)
	{
		u->EventTeleport(pcaster->m_marks[m_targets.mark].x, pcaster->m_marks[m_targets.mark].y, pcaster->m_marks[m_targets.mark].mapid);
		u->SetSpellEffect(5);
		u->StopMovement();
	}

	return true;
}
void Spell::Cast()
{
	m_damageflags |= m_data->damageflags;

	if (!(m_data->targetallowedtype & m_targets.flags) || (!m_triggered && m_caster->m_nextspell > GetMSTime()))
		return;

	if (!m_caster->CanCast(m_data->id, m_triggered))
		return;

	if (m_caster->IsPlayer())
	{
		Player* pcaster = TO_PLAYER(m_caster);

		if (m_data->skillreq)
		{
			pcaster->AddSkillCooldown(m_data->skillreq);
			if (m_data->skillreq2)
				pcaster->AddSkillCooldown(m_data->skillreq2);
		}
	}

	if (!m_triggered)
	{
		if (!m_caster->IsAlive())
			return;

		m_caster->RemoveAura(SPELL_AURA_HIDE);
		if ((!m_caster->IsCreature() || TO_CREATURE(m_caster)->m_proto->boss != 1))
		{
			if (m_caster->HasAura(SPELL_AURA_DAZE_AURA))
			{
				m_caster->m_auras[SPELL_AURA_DAZE_AURA]->Remove();
				m_caster->SetSpellEffect(30);
				return;
			}
		}

		if (m_caster->HasAura(SPELL_AURA_DULL_SENSES_EFFECT) && RandomUInt(99) < 10)
		{
			m_caster->SetSpellEffect(30);
			return;
		}

		//cant cast while mutated
		if (m_caster->HasAura(SPELL_AURA_MUTATIO_NIMBUS) || m_caster->HasAura(SPELL_AURA_MUTATIO_FLAMMA))
			return;

		if (m_caster->HasAura(SPELL_AURA_MULTI_PROJECTILE))
			m_caster->m_auras[SPELL_AURA_MULTI_PROJECTILE]->Remove();

		if (m_caster->IsPlayer())
		{
			SpellData* data = sSpellStorage.GetSpell(m_data->id);

			if (m_caster->m_laststrikeblock && m_caster->m_laststrike + m_caster->GetAttackSpeed() >= g_mstime)
			{
				//cancel this spell, set next spell and cancel attack
				m_caster->StopMovement();
				m_caster->SetNextSpell(m_data->id, m_targets, false);
				return;
			}

			if (!m_triggered && data != NULL)
			{
				if (data->manareq > m_caster->GetMana())
					return;
				m_caster->SetMana(m_caster->GetMana() - data->manareq);
			}

			switch (m_data->effect)
			{
			case SPELL_EFFECT_CREATE_BEAM:
			case SPELL_EFFECT_CREATE_BOMB:
			case SPELL_EFFECT_CREATE_PROJECTILE:
			case SPELL_EFFECT_DAMAGE:
			case SPELL_EFFECT_DISPEL:
			case SPELL_EFFECT_CREATE_OBJECT:
			case SPELL_EFFECT_MULTI_SHOT:
			case SPELL_EFFECT_SPIN_ATTACK:
			case SPELL_EFFECT_ATTACK_RUN:
			case SPELL_EFFECT_APPLY_AURA_SELF:
			case SPELL_EFFECT_APPLY_AURA_NEG:
				{
						if (m_caster->ProtectedByGround(true))
							return;
				} break;
			}
		}

		float speedbonus = 1;
		SpellData* dat = sSpellStorage.GetSpell(m_data->id);
		int32 school = -1;
		if (dat != NULL)
			school = dat->school;
		if (m_data->forcedschool != -1)
			school = m_data->forcedschool;
		switch (school)
		{
		case SPELL_SCHOOL_PHYSICAL:
		case SPELL_SCHOOL_SMASHING:
		case SPELL_SCHOOL_SLASHING:
		case SPELL_SCHOOL_PIERCING:
			{
				if (m_caster->IsPlayer())
				{
					Player* pcaster = TO_PLAYER(m_caster);
					speedbonus = float(100 - m_caster->m_bonuses[ATTACK_SPEED] * 0.2) / 100;
				}
				else
					speedbonus = float(100 - m_caster->m_bonuses[ATTACK_SPEED] * 0.1) / 100;
			}
			break;
			//rest are spells
		default: speedbonus = float(100 - m_caster->m_bonuses[CAST_SPEED]) / 100;
		}

		if (speedbonus < 0.25f)
			speedbonus = 0.25f;

		if (m_caster->m_lastspellid == m_data->id && g_mstime - m_caster->m_lastspell < m_data->castdelay)
			m_caster->m_nextspell = m_caster->m_nextspell + (m_data->castdelay * speedbonus);
		else
			m_caster->m_nextspell = g_mstime + (m_data->castdelay * speedbonus);
		m_caster->m_lastspellid = m_data->id;
		m_caster->m_lastspell = g_mstime;
		float rotation = 0;
		if (m_targets.flags & SPELL_TARGET_GROUND)
			rotation = m_caster->CalcFacingDeg(m_targets.x, m_targets.y);
		else if (m_targets.flags & SPELL_TARGET_UNIT)
		{
			Object* o = m_caster->m_mapmgr->GetObject(m_targets.x);
			if (o != NULL)
			{
				if (o == m_caster)
					rotation = m_caster->m_rotation;
				else
					rotation = m_caster->CalcFacingDeg(o->m_positionX, o->m_positionY);
			}
		}
		else if (m_targets.flags & SPELL_TARGET_SELF)
			rotation = m_caster->m_rotation;
		m_caster->m_rotation = rotation;
		m_caster->SetUpdateBlock(UPDATETYPE_MOVEMENT | UPDATETYPE_ANIMATION);
		m_caster->StopMovement();
		if (m_caster->IsPlayer() && m_caster->IsAttacking() && m_data->id < 15000) //normal spells, not special
			m_caster->ClearPOI();

		if (m_data->casteranimation != 0)
			m_caster->SetAnimation(m_data->casteranimation, m_data->casteranimation2);
	}
	
	GrowableArray<Unit*> targets;

	if (m_data->targettype & SPELL_TARGET_GROUND && m_targets.flags & SPELL_TARGET_GROUND && !m_triggered)
	{
		if (!m_caster->IsInRectRange(m_targets.x, m_targets.y))
			return;
	}


	//figure out what to do
	if (m_data->targettype & SPELL_TARGET_UNIT && m_targets.flags & SPELL_TARGET_UNIT)
	{
		Object* o = m_caster->m_mapmgr->GetObject(m_targets.x);

		if (o == NULL)
			goto SKIPUNITCHECK;

		if (m_caster != o && !m_caster->IsInRange(o) && !m_triggered)
			goto SKIPUNITCHECK;

		if (m_caster != o && m_caster->GetDistance(o) >= 200 && !m_triggered)
			goto SKIPUNITCHECK;

		//few simple rules :P
		if (!m_triggered && m_caster == o && !(m_data->targetallowedtype & SPELL_TARGET_SELF))
			goto SKIPUNITCHECK;
		if (o->IsUnit() && !TO_UNIT(o)->IsAlive() && !(m_data->targetallowedtype & SPELL_TARGET_DEAD))
			goto SKIPUNITCHECK;
		if (o->IsPlayer() && TO_PLAYER(o)->m_invis)
			goto SKIPUNITCHECK;
		if (TO_UNIT(o)->HasUnitFlag(UNIT_FLAG_UNATTACKABLE) && (!m_caster->IsCreature() || !m_triggered))
			goto SKIPUNITCHECK;
		if (o != m_caster && m_triggerspelleffect == NULL && !m_caster->m_mapmgr->InLineOfSight(m_caster->m_positionX, m_caster->m_positionY, o->m_positionX, o->m_positionY, PASS_FLAG_TELEPORT))
			goto SKIPUNITCHECK;

		if (o->IsUnit())
		{
			targets.push(TO_UNIT(o));

			if (m_caster->IsPlayer() && !m_triggered)
				TO_PLAYER(m_caster)->SetTarget(TO_UNIT(o));
		}
	}
SKIPUNITCHECK:

	if (m_data->targettype & SPELL_TARGET_SELF && m_targets.flags & SPELL_TARGET_UNIT && m_targets.x == m_caster->m_serverId && !(m_data->targettype & SPELL_TARGET_AOE))
		targets.push(m_caster);

	if (m_data->targettype & SPELL_TARGET_SELF && m_targets.flags & SPELL_TARGET_SELF && !(m_data->targettype & SPELL_TARGET_AOE))
		targets.push(m_caster);


	if (m_data->targettype & (SPELL_TARGET_AOE | SPELL_TARGET_AOE_FRIENDLY))
	{
		for (std::map<Unit*, uint8>::iterator itr = m_caster->m_inrangeUnits.begin(); itr != m_caster->m_inrangeUnits.end(); ++itr)
		{
			if (m_triggerspelleffect != NULL && itr->first->GetDistance(m_triggerspelleffect->m_positionX, m_triggerspelleffect->m_positionY) > GetRadius())
				continue;
			if (m_triggerspelleffect == NULL && itr->first->GetDistance(m_caster->m_positionX, m_caster->m_positionY) > GetRadius())
				continue;
			if (itr->first->IsPlayer() && TO_PLAYER(itr->first)->m_invis)
				continue;
			if (m_triggerspelleffect != NULL && !m_caster->m_mapmgr->InLineOfSight(m_triggerspelleffect->m_positionX, m_triggerspelleffect->m_positionY, itr->first->m_positionX, itr->first->m_positionY, PASS_FLAG_TELEPORT))
				continue;
			if (m_triggerspelleffect == NULL && !m_caster->m_mapmgr->InLineOfSight(m_caster->m_positionX, m_caster->m_positionY, itr->first->m_positionX, itr->first->m_positionY, PASS_FLAG_TELEPORT))
				continue;
			if (m_caster->IsCreature() && TO_CREATURE(m_caster)->m_proto->boss == 1 && itr->first->ProtectedByGround())
				continue;
			if (itr->first->HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
				continue;
			if (m_data->targettype & SPELL_TARGET_AOE && !m_caster->IsAttackable(itr->first))
				continue;
			if (m_data->targettype & SPELL_TARGET_AOE_FRIENDLY)
			{
				if (m_caster->IsPlayer())
				{
					if (itr->first->IsPlayer())
					{
						if (TO_PLAYER(m_caster)->GetLinkID() == 0)
							continue;
						if (TO_PLAYER(m_caster)->GetLinkID() != TO_PLAYER(itr->first)->GetLinkID())
							continue;
					}
					else if (itr->first->IsPet())
					{
						Unit* owner = TO_CREATURE(itr->first)->GetOwner();
						if (owner == NULL || !owner->IsPlayer() || TO_PLAYER(owner)->m_attackstate == ATTACK_STATE_ALL)
							continue;
						if (TO_PLAYER(owner)->GetLinkID() == 0 || TO_PLAYER(owner)->GetLinkID() != TO_PLAYER(m_caster)->GetLinkID())
							continue;
					}
					else
						continue;
				}
				else if (m_caster->IsHostile(itr->first))
					continue;
			}
			if (!(m_data->targettype & SPELL_TARGET_DEAD) && !itr->first->IsAlive())
				continue;
			targets.push(itr->first);
			if (m_data->maxtargets != 0 && targets.size() >= m_data->maxtargets)
				break;
		}
	}

	if (m_data->targettype & SPELL_TARGET_PETS && m_caster->IsPlayer())
	{
		for (std::set<uint16>::iterator itr = m_caster->m_pets.begin(); itr != m_caster->m_pets.end(); ++itr)
		{
			Object* pet = m_caster->m_mapmgr->GetObject(*itr);
			if (pet == NULL || !pet->IsPet() || !pet->IsInRange(m_caster))
				continue;
			targets.push(TO_UNIT(pet));
		}
	}

	//set target effects if possible
	if (m_data->display != 0)
	{
			for (uint32 i=0; i<targets.size(); ++i)
			{
				if (m_data->casterdisplay != 0 && targets[i] == m_caster)
					continue;
				targets[i]->SetSpellEffect(m_data->display);
			}
	}
	
	if (m_data->casterdisplay != 0)
		m_caster->SetSpellEffect(m_data->casterdisplay);

	if (m_caster->IsCreature() && !m_triggered)
	{
		TO_CREATURE(m_caster)->AddSpellCooldown(m_data->id);
	}

	(*this.*g_SpellEffectHandlers[m_data->effect])(targets);
}

void Spell::SpellEffectDamage( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		m_caster->DealDamage(targets[i], GetValue(), m_triggerspelleffect? m_triggerspelleffect->m_spell : m_data, m_damageflags);
	}
}

void Spell::SpellEffectCreateProjectile( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		//calc rotation
		float rot = m_caster->CalcFacingDeg(targets[i]->m_positionX, targets[i]->m_positionY) - 90;
		SpellEffect* eff = new SpellEffect(m_caster, SPELL_TYPE_PROJECTILE, m_caster->m_positionX, m_caster->m_positionY, rot, this);
		eff->m_visualId = m_data->display;
		eff->m_targetid = targets[i]->m_serverId;
		if (m_data->value > 0)
			eff->m_targetid = targets[i]->m_serverId;
		eff->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 750, 1, EVENT_FLAG_NOT_IN_WORLD);
		m_caster->m_mapmgr->AddObject(eff);
	}

	if (m_targets.flags & SPELL_TARGET_GROUND)
	{
		float rot = m_caster->CalcFacingDeg(m_targets.x, m_targets.y) - 90;
		SpellEffect* eff = new SpellEffect(m_caster, SPELL_TYPE_PROJECTILE, m_caster->m_positionX, m_caster->m_positionY, rot, this);
		eff->m_visualId = m_data->display;
		eff->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 4000, 1, EVENT_FLAG_NOT_IN_WORLD);
		m_caster->m_mapmgr->AddObject(eff);
	}

}

void Spell::SpellEffectCreateBeam( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		//calc rotation
		float rot = m_caster->CalcFacingDeg(targets[i]->m_positionX, targets[i]->m_positionY) - 90;
		SpellEffect* eff = new SpellEffect(m_caster, SPELL_TYPE_BEAM, m_caster->m_positionX, m_caster->m_positionY, rot, this);
		eff->m_targetid = targets[i]->m_serverId;
		eff->m_visualId = m_data->display;
		eff->m_damageflags |= m_damageflags;
		m_caster->m_mapmgr->AddObject(eff);
		eff->AddEvent(&SpellEffect::BeamFire, EVENT_SPELLEFFECT_BEAMFIRE, 700, 1, EVENT_FLAG_NOT_IN_WORLD);

		if (!m_triggered)
			m_caster->StopMovement(700);
	}
}

void Spell::SpellEffectCreateObject( GrowableArray<Unit*> & targets )
{
	//limit
	if (m_data->value > 0)
	{
		uint32 currentcount = m_caster->GetSummonedObjectCount(m_data->display);

		if (currentcount >= m_data->value)
		{
			if (m_data->value == 1) //delete old and make new
			{
				std::vector<Object*> curobjs;
				m_caster->GetSummonedObject(m_data->display, &curobjs);
				
				for (auto itr = curobjs.begin(); itr != curobjs.end(); ++itr)
					(*itr)->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
			else
				return;
		}
	}

	for (uint32 i=0; i<targets.size(); ++i)
	{
		//calc rotation
		float rot = m_caster->CalcFacingDeg(targets[i]->m_positionX, targets[i]->m_positionY) - 90;
		SpellEffect* eff = new SpellEffect(m_caster, SPELL_TYPE_OBJECT, targets[i]->m_positionX, targets[i]->m_positionY, rot, this);
		eff->m_targetid = targets[i]->m_serverId;
		eff->m_visualId = m_data->display;
		eff->m_damageflags |= m_damageflags;

		m_caster->m_mapmgr->AddObject(eff);
		//do we trigger a spell?
		if (m_data->triggerspell)
		{
			uint32 times = m_data->amplitude? 0 : 1; //0 = infinite
			eff->AddEvent(&SpellEffect::ObjectFireSpell, EVENT_SPELLEFFECT_OBJECTFIRE, m_data->amplitude, times, EVENT_FLAG_NOT_IN_WORLD);
		}
	}

	if (m_targets.flags & SPELL_TARGET_GROUND)
	{
		float rot = m_caster->CalcFacingDeg(m_targets.x, m_targets.y) - 90;
		SpellEffect* eff = new SpellEffect(m_caster, SPELL_TYPE_OBJECT, m_targets.x, m_targets.y, rot, this);
		eff->m_visualId = m_data->display;
		eff->m_damageflags |= m_damageflags;

		m_caster->m_mapmgr->AddObject(eff);

		if (m_data->triggerspell)
		{
			uint32 times = m_data->amplitude? 0 : 1; //0 = infinite
			eff->AddEvent(&SpellEffect::ObjectFireSpell, EVENT_SPELLEFFECT_OBJECTFIRE, m_data->amplitude, times, EVENT_FLAG_NOT_IN_WORLD);
		}
	}
}

void Spell::SpellEffectHeal( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		if (targets[i]->IsPlayer() && m_caster->IsPlayer() && targets[i] != m_caster)
		{
			if (TO_PLAYER(targets[i])->InPVPMode())
				TO_PLAYER(m_caster)->SetPVPCombat();
		}

		if (!targets[i]->IsAlive())
		{
			if (!targets[i]->IsPlayer())
				continue;
			if (TO_PLAYER(targets[i])->m_bosskilledid != 0)
			{
				Object* boss = targets[i]->m_mapmgr->GetObject(TO_PLAYER(targets[i])->m_bosskilledid);

				if (boss != NULL && boss->IsCreature() && TO_CREATURE(boss)->IsAlive() && TO_CREATURE(boss)->IsAttacking())
					continue;
			}
			TO_PLAYER(targets[i])->m_canResurrect = true;
			TO_PLAYER(targets[i])->SendActionResult(PLAYER_ACTION_PRESS_F12_TO_RETURN);
			targets[i]->RemoveEvents(EVENT_PLAYER_RESPAWN);
		}
		uint32 val = GetValue();
		if (m_caster->IsPlayer())
			val *= float(100 + (TO_PLAYER(m_caster)->m_wisdom * 2)) / 100;

		targets[i]->ModHealth(val);

		if (targets[i] == m_caster && m_caster->IsPlayer() && m_caster->m_laststrike <= g_mstime && (g_mstime - m_caster->m_laststrike) <= 3000) //slow next melee attack
			m_caster->m_laststrike = g_mstime;

		if (m_data->id == 15052) //maximus potion hack
			targets[i]->SetMana(targets[i]->m_maxmana);
	}
}

void Spell::SpellEffectApplyAura( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		SpellAura* aura = sSpellAuras.GetEntry(m_data->triggerspell);

		if (aura == NULL)
			continue;
		if (!aura->canoverride && targets[i]->HasAura(aura->slot))
		{
			int32 thisval = GetValue();
			int32 otherval = -1;
			Aura* otheraura = targets[i]->m_auras[aura->slot];
			if (otheraura->m_spell != NULL)
				otherval = otheraura->m_spell->value;
			if (otheraura->appliedval != -1)
				otherval = otheraura->appliedval;

			if (thisval < otherval)
				continue;
		}

		if (targets[i]->IsPlayer() && m_caster->IsPlayer() && targets[i] != m_caster)
		{
			if (TO_PLAYER(targets[i])->InPVPMode())
				TO_PLAYER(m_caster)->SetPVPCombat();
		}

		Aura* a = new Aura(m_caster, targets[i], m_data->id, m_forcedvalue);
	}
}

void Spell::SpellEffectShortTeleport( GrowableArray<Unit*> & targets )
{
	if (m_caster->IsPlayer())
	{
		Player* pcaster = TO_PLAYER(m_caster);
		if (GetMSTime() < pcaster->m_transdelay)
		{
			pcaster->ClearSpellEffect();
			return;
		}
	}

	for (uint32 i=0; i<targets.size(); ++i)
	{
		float rot = (targets[i]->CalcFacingDeg(m_caster->m_positionX, m_caster->m_positionY) - 90) * PI / 180.0f;
		int32 x = targets[i]->m_positionX - sinf(rot) * (targets[i]->m_bounding * 0.9f);
		int32 y = targets[i]->m_positionY + cosf(rot) * (targets[i]->m_bounding * 0.9f);
		if (m_caster->m_mapmgr->CanPass(x / 20, y / 20) && m_caster->m_mapmgr->InLineOfSight(m_caster->m_positionX, m_caster->m_positionY, x, y, PASS_FLAG_TELEPORT))
		{
			if (!(m_damageflags & DAMAGE_FLAG_CANNOT_PROC))
			{
				m_caster->m_rotation = m_caster->CalcFacingDeg(x, y);
				m_caster->SetUpdateBlock(UPDATETYPE_MOVEMENT);
			}

			m_caster->SetPosition(x, y);
			m_caster->SetUpdateBlock(0x1F);
			m_caster->SetAnimationType(ANIM_TYPE_STANDING);
			m_caster->StopMovement();
		}
		else
		{
			x = targets[i]->m_positionX;
			y = targets[i]->m_positionY;
			if (m_caster->IsPlayer() && TO_PLAYER(m_caster)->m_account->m_data->accountlevel == 0 && m_caster->m_mapmgr->IsNoMarkLocation(x, y))
				return;

			if (m_caster->m_mapmgr->CanPass(x / 20, y / 20) && m_caster->m_mapmgr->InLineOfSight(m_caster->m_positionX, m_caster->m_positionY, x, y, PASS_FLAG_TELEPORT))
			{
				m_caster->SetPosition(x, y);
				m_caster->SetUpdateBlock(0x1F);
				m_caster->SetAnimationType(ANIM_TYPE_STANDING);
				m_caster->StopMovement();
			}
		}
	}

	if (m_targets.flags & SPELL_TARGET_GROUND)
	{
		if (m_caster->IsPlayer() && TO_PLAYER(m_caster)->m_account->m_data->accountlevel == 0 && m_caster->m_mapmgr->IsNoMarkLocation(m_targets.x, m_targets.y))
			return;
		if ((m_caster->m_mapmgr->CanPass(m_targets.x / 20, m_targets.y / 20) && m_caster->m_mapmgr->InLineOfSight(m_caster->m_positionX, m_caster->m_positionY, m_targets.x, m_targets.y, PASS_FLAG_TELEPORT))  || m_caster->m_mapmgr->GetMapLayer2(m_targets.x, m_targets.y) == 0xCF)
		{
			if (!(m_damageflags & DAMAGE_FLAG_CANNOT_PROC))
			{
				m_caster->m_rotation = m_caster->CalcFacingDeg(m_targets.x, m_targets.y);
				m_caster->SetUpdateBlock(UPDATETYPE_MOVEMENT);
			}

			m_caster->SetPosition(m_targets.x, m_targets.y);
			m_caster->SetUpdateBlock(0x1F);
			m_caster->SetAnimationType(ANIM_TYPE_STANDING);
			m_caster->StopMovement();
		}
		else
			m_caster->ClearSpellEffect();
	}

	if (m_caster->IsPlayer())
	{
		//this prepares client for teleport
		ServerPacket data(MSG_CLIENT_INFO);
		data << uint32(0x13);
		data << m_caster->m_positionX;
		data << uint16(0);
		data << m_caster->m_positionY;
		data << uint16(0);
		TO_PLAYER(m_caster)->Send(&data);

		for (std::map<Unit*, uint8>::iterator itr = m_caster->m_inrangeUnits.begin(); itr != m_caster->m_inrangeUnits.end(); ++itr)
			itr->second |= 0x1F;
	}
}

void Spell::SpellEffectCreateBomb( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		//calc rotation
		float rot = m_caster->CalcFacingDeg(targets[i]->m_positionX, targets[i]->m_positionY) - 90;
		SpellEffect* eff = new SpellEffect(m_caster, SPELL_TYPE_BOMB, m_caster->m_positionX, m_caster->m_positionY, rot, this);
		eff->m_targetid = targets[i]->m_serverId;
		eff->m_visualId = m_data->display;
		eff->m_damageflags |= m_damageflags;

		m_caster->m_mapmgr->AddObject(eff);
		eff->AddEvent(&SpellEffect::BeamFire, EVENT_SPELLEFFECT_BEAMFIRE, 700, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void Spell::SpellEffectAttackRun( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		if (!targets[i]->IsAlive())
		{
			m_caster->CreatePath(targets[i]->m_positionX, targets[i]->m_positionY);
			m_caster->m_destinationRadius = 0;
			m_caster->ClearPOI();
		}
		else
			m_caster->Attack(targets[i]);
		m_caster->m_attackRun = true;
	}

	if (m_targets.flags & SPELL_TARGET_GROUND)
	{
		m_caster->m_attackRun = true;
		m_caster->m_destinationRadius = 0;
		m_caster->ClearPOI();
		m_caster->CreatePath(m_targets.x, m_targets.y);
	}
}

void Spell::SpellEffectMeleeAttack( GrowableArray<Unit*> & targets )
{
	if (m_caster->m_bonuses[BONUS_FRAGOR_CHANCE] && RandomUInt(99) < m_caster->m_bonuses[BONUS_FRAGOR_CHANCE])
		m_caster->CastSpell(253, true);

	Item* casterweapon = m_caster->GetCurrentWeapon();

	for (uint32 i=0; i<targets.size(); ++i)
	{
		if (!m_caster->DidHit(targets[i]))
		{
			targets[i]->SetSpellEffect(0);
			continue;
		}

		int32 damage = m_caster->GetMeleeDamage() * (float(GetValue()) / 100);
		m_caster->DealDamage(targets[i], damage, m_data, m_damageflags | DAMAGE_FLAG_IS_MELEE_ATTACK);

		if (!targets[i]->IsPlayer() && m_data->id == 306 && targets[i]->IsAttacking() && targets[i]->m_POIObject != m_caster->m_serverId)
			targets[i]->m_POIObject = m_caster->m_serverId;
	}
}

void Spell::SpellEffectDispel( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		if (m_caster->IsCreature())
			targets[i]->RemoveAllDispellableAuras();

		if (m_caster->IsPlayer())
		{
			int32 leveldiff = targets[i]->GetLevel() - m_caster->GetLevel();

			if (!m_triggered && targets[i]->IsPlayer() && m_caster->IsPlayer())
				TO_PLAYER(m_caster)->SetPVPCombat();

			if (leveldiff > 2)
				continue;

			if (!m_caster->IsHostile(targets[i]))
				continue;

			if (targets[i]->m_bonuses[RESIST_DISPEL] && RandomUInt(99) < targets[i]->m_bonuses[RESIST_DISPEL])
				continue;

			GrowableArray<uint32> dispellableslots;
			for (uint32 j = 0; j < NUM_SPELL_AURAS; ++j)
			{
				if (targets[i]->m_auras[j] != NULL && targets[i]->m_auras[j]->m_data->dispellable && !targets[i]->m_auras[j]->m_dispelblock)
					dispellableslots.push(j);
			}

			if (dispellableslots.size() == 0)
				continue;

			uint32 dispelslot = dispellableslots[RandomUInt(dispellableslots.size() - 1)];
			targets[i]->m_auras[dispelslot]->Remove();
		}
	}
}

void Spell::SpellEffectMultiShot( GrowableArray<Unit*> & targets )
{
	if (m_caster->GetCurrentWeapon() == NULL || m_caster->GetCurrentWeapon()->m_data->type != ITEM_TYPE_BOW)
		return;

	for (uint32 i=0; i<targets.size(); ++i)
	{
		m_caster->Strike(targets[i], true);
	}
}

void Spell::SpellEffectCreatePortal( GrowableArray<Unit*> & targets )
{
	//limit
	if (m_data->value > 0)
	{
		uint32 currentcount = m_caster->GetSummonedObjectCount(m_data->display);

		if (currentcount >= m_data->value)
		{
			if (m_data->value == 1) //delete old and make new
			{
				std::vector<Object*> curobjs;
				m_caster->GetSummonedObject(m_data->display, &curobjs);

				for (auto itr = curobjs.begin(); itr != curobjs.end(); ++itr)
					(*itr)->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
			else
				return;
		}
	}

	if (m_targets.mark == 0)
		return;

	int32 cellx = m_targets.x / 20;
	int32 celly = m_targets.y / 20;
	
	for (int32 y = -1; y < 2; ++y)
	{
		for (int32 x = -1; x < 2; ++x)
		{
			if (!m_caster->m_mapmgr->CanPass(cellx + x, celly + y))
				return;
			if (m_caster->m_mapmgr->GetMapLayer2((cellx + x) * 20, (celly + y) * 20) == 46) //square
				return;
		}
	}

	float rot = m_caster->CalcFacingDeg(m_targets.x, m_targets.y) - 90;
	SpellEffect* sp = new SpellEffect(m_caster, SPELL_TYPE_PORTAL, m_targets.x, m_targets.y, rot, this, &m_targets);
	sp->m_visualId = m_data->display;
	sp->AddEvent(&SpellEffect::SetVisual, uint32(m_data->display + 1), EVENT_SPELLEFFECT_OBJECTFIRE, 5000, 1, EVENT_FLAG_NOT_IN_WORLD);
	sp->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 20000, 1, EVENT_FLAG_NOT_IN_WORLD);
	m_caster->m_mapmgr->AddObject(sp);
	m_caster->m_rotation = m_caster->CalcFacingDeg(m_targets.x, m_targets.y);
	m_caster->SetAnimation(m_data->casteranimation, m_data->casteranimation2);
	m_caster->SetUpdateBlock(UPDATETYPE_MOVEMENT);
}

void Spell::SpellEffectReditus( GrowableArray<Unit*> & targets )
{
	if (!m_caster->IsPlayer())
		return;
	Player* pcaster = TO_PLAYER(m_caster);
	if (pcaster->InPVPAttackMode())
	{
		pcaster->ClearSpellEffect();
		pcaster->SendActionResult(PLAYER_ACTION_NOT_USABLE_IN_BATTLE);
		return;
	}
	pcaster->PortToMark(1);
}

void Spell::SpellEffectResetStats( GrowableArray<Unit*> & targets )
{
	for (uint32 i = 0; i < targets.size(); ++i)
	{
		if (!targets[i]->IsPlayer())
			continue;
		Player* p = TO_PLAYER(targets[i]);
		p->ResetStats();
	}
}

void Spell::SpellEffectSummonCreature( GrowableArray<Unit*> & targets )
{
	uint32 spawnamount = m_data->triggerspell;

	if (m_caster->IsPlayer())
	{
		TO_PLAYER(m_caster)->ApplyFlatMod(spawnamount, SPELL_MOD_DAMAGE, m_data->spellmask);
		TO_PLAYER(m_caster)->ApplyPctMod(spawnamount, SPELL_MOD_DAMAGE, m_data->spellmask);
	}

	uint32 numspawned = targets.size() * spawnamount;
	if (m_targets.flags & SPELL_TARGET_GROUND)
		numspawned += spawnamount;

	if (m_caster->IsPlayer() && m_caster->m_pets.size() + numspawned > 4)
		return;

	m_caster->AddEvent(&Unit::UpdatePetFollowAngles, EVENT_UNIT_UPDATEPETS, 500, 1, EVENT_FLAG_NOT_IN_WORLD);

	CreatureProto* pt = sCreatureProto.GetEntry(m_data->value);
	if (pt == NULL)
		return;

	for (uint32 i=0; i<targets.size(); ++i)
	{
		for (uint32 j = 0; j < spawnamount; ++j)
		{
			Creature* cre = new Creature(pt, NULL);
			cre->SetAsPet(m_caster->m_serverId, true, m_caster->IsCreature());
			cre->SetPosition(targets[i]->m_positionX + RandomUInt(40) - 20, targets[i]->m_positionY + RandomUInt(40) - 20);
			cre->m_norespawn = true;
			if (m_data->duration)
				cre->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, m_data->duration, 1, EVENT_FLAG_NOT_IN_WORLD);
			m_caster->m_mapmgr->AddObject(cre);

			if (m_caster->IsPlayer())
				cre->m_petstate = PET_STATE_STOP;
		}
	}

	if (m_targets.flags & SPELL_TARGET_GROUND)
	{
		if (m_caster->IsPlayer() && m_caster->m_mapmgr != NULL && !m_caster->m_mapmgr->CanPass(m_targets.x / 20, m_targets.y / 20))
			return;
		for (uint32 j = 0; j < spawnamount; ++j)
		{
			Creature* cre = new Creature(pt, NULL);
			cre->SetAsPet(m_caster->m_serverId, true, m_caster->IsCreature());
			cre->SetPosition(m_targets.x, m_targets.y);
			cre->m_norespawn = true;
			if (m_data->duration)
				cre->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, m_data->duration, 1, EVENT_FLAG_NOT_IN_WORLD);
			m_caster->m_mapmgr->AddObject(cre);

			if (m_caster->IsPlayer())
				cre->m_petstate = PET_STATE_STOP;
		}
	}
}

void Spell::SpellEffectCharm( GrowableArray<Unit*> & targets )
{
	if (m_caster->IsPlayer() && m_caster->m_pets.size() >= 2)
		return;

	m_caster->AddEvent(&Unit::UpdatePetFollowAngles, EVENT_UNIT_UPDATEPETS, 500, 1, EVENT_FLAG_NOT_IN_WORLD);

	for (uint32 i=0; i<targets.size(); ++i)
	{
		if (!targets[i]->IsCreature())
			continue;
		if (targets[i]->IsPet())
			continue;
		if (TO_CREATURE(targets[i])->m_proto->boss)
			continue;
		if (!TO_CREATURE(targets[i])->m_proto->cancharm)
			continue;
		if (m_caster->GetLevel() < targets[i]->GetLevel())
			continue;
		TO_CREATURE(targets[i])->SetAsPet(m_caster->m_serverId);
		m_caster->m_pets.insert(targets[i]->m_serverId);
		targets[i]->SetHealth(targets[i]->m_maxhealth);
	}
}

void Spell::SpellEffectCurePosion( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		if (RandomUInt(30) < m_data->spelllevel)
			continue;

		if (targets[i]->IsPlayer() && m_caster->IsPlayer())
		{
			if (TO_PLAYER(targets[i])->InPVPMode())
				TO_PLAYER(m_caster)->SetPVPCombat();
		}

		if (targets[i]->HasAura(SPELL_AURA_POISON))
			targets[i]->m_auras[SPELL_AURA_POISON]->Remove();
	}
}

void Spell::SpellEffectInstaPort( GrowableArray<Unit*> & targets )
{
	uint32 mark = m_targets.mark;
	if (mark >= NUM_MARKS)
		return;
	if (!m_caster->IsPlayer())
		return;

	Player* pcaster = TO_PLAYER(m_caster);
	if (pcaster->InPVPAttackMode())
	{
		pcaster->ClearSpellEffect();
		pcaster->SendActionResult(PLAYER_ACTION_NOT_USABLE_IN_BATTLE);
		return;
	}

	if (pcaster->m_marks[mark].x == 0 && pcaster->m_marks[mark].y == 0)
		return;

	uint32 casterlevel = m_caster->GetLevel();

	if (casterlevel < 30)
		casterlevel = 30;

	for (uint32 i=0; i<targets.size(); ++i)
	{
		if (!targets[i]->IsPlayer())
			continue;
		int32 x = pcaster->m_marks[mark].x;
		int32 y = pcaster->m_marks[mark].y;
		uint32 mapid = pcaster->m_marks[mark].mapid;

		if (mapid != TO_PLAYER(targets[i])->m_mapid && !targets[i]->IsAlive())
			continue;
		targets[i]->AddEvent(&Player::EventTeleport, x, y, mapid, EVENT_UNIT_SETPOSITION, (1000 / (casterlevel / 30)), 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void Spell::SpellEffectApplyAuraSelf( GrowableArray<Unit*> & targets )
{
	new Aura(m_caster, m_caster, m_data->id);
}

void Spell::SpellEffectApplyAreaAura( GrowableArray<Unit*> & targets )
{
	for (uint32 i = 0; i < targets.size(); ++i)
	{
		Aura* a = new Aura(m_caster, targets[i], m_data->id, m_forcedvalue);
		a->SetAreaAura(AREA_AURA_FRIENDLY);
	}
}

void Spell::SpellEffectApplyAreaAuraNeg( GrowableArray<Unit*> & targets )
{
	for (uint32 i = 0; i < targets.size(); ++i)
	{
		Aura* a = new Aura(m_caster, targets[i], m_data->id, m_forcedvalue);
		a->SetAreaAura(AREA_AURA_HOSTILE);
	}
}

void Spell::SpellEffectDamageBasedLevel( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		m_caster->DealDamage(targets[i], GetValue() * m_caster->GetLevel(), m_triggerspelleffect? m_triggerspelleffect->m_spell : m_data, m_damageflags);
	}
}

void Spell::SpellEffectHealAll( GrowableArray<Unit*> & targets )
{
	for (uint32 i=0; i<targets.size(); ++i)
	{
		if (targets[i]->IsPlayer() && m_caster->IsPlayer() && targets[i] != m_caster)
		{
			if (TO_PLAYER(targets[i])->InPVPMode())
				TO_PLAYER(m_caster)->SetPVPCombat();
		}

		if (!targets[i]->IsAlive())
		{
			if (!targets[i]->IsPlayer())
				continue;
			TO_PLAYER(targets[i])->m_canResurrect = true;
			TO_PLAYER(targets[i])->SendActionResult(PLAYER_ACTION_PRESS_F12_TO_RETURN);
			targets[i]->RemoveEvents(EVENT_PLAYER_RESPAWN);
		}
		uint32 val = targets[i]->m_maxhealth * float(20 * (m_caster->GetSkillValue(SKILL_HEAL_ALL) + m_caster->GetSkillValue(SKILL_HEAL_ALL2))) / 100;
		targets[i]->ModHealth(val);
	}
}

void Spell::SpellEffectThrowWeapon( GrowableArray<Unit*> & targets )
{
	uint32 weapondisplayid = 0;

	Item* weapon = m_caster->GetCurrentWeapon();

	if (weapon != NULL && weapon->m_data->modelid < 100)
		weapondisplayid = weapon->m_data->modelid + 100;

	m_caster->SetThrownWeapon(weapon->m_data->id);
	m_caster->AddEvent(&Unit::EventForceReturnWeapon, EVENT_UNIT_FORCE_RETURN_WEAPON, 10000, 1, EVENT_FLAG_NOT_IN_WORLD);

	for (uint32 i=0; i<targets.size(); ++i)
	{
		//calc rotation
		float rot = m_caster->CalcFacingDeg(targets[i]->m_positionX, targets[i]->m_positionY) - 90;
		SpellEffect* eff = new SpellEffect(m_caster, SPELL_TYPE_PROJECTILE, m_caster->m_positionX, m_caster->m_positionY, rot, this);
		eff->m_visualId = weapondisplayid;
		eff->m_targetid = targets[i]->m_serverId;
		eff->m_damageflags |= m_damageflags;

		if (m_data->value > 0)
			eff->m_targetid = targets[i]->m_serverId;
		eff->AddEvent(&SpellEffect::Decay, EVENT_OBJECT_DELETE, 4000, 1, EVENT_FLAG_NOT_IN_WORLD);
		m_caster->m_mapmgr->AddObject(eff);
	}
}

void Spell::SpellEffectThrowReturnWeapon( GrowableArray<Unit*> & targets )
{
	if (m_targets.flags & SPELL_TARGET_GROUND)
	{
		float rot = m_caster->CalcFacingDeg(m_targets.x, m_targets.y, m_caster->m_positionX, m_caster->m_positionY) - 90;
		SpellEffect* eff = new SpellEffect(m_caster, SPELL_TYPE_PROJECTILE, m_targets.x, m_targets.y, rot, this);
		eff->m_visualId = m_forcedvalue;
		eff->m_targetid = m_caster->m_serverId;
		eff->m_nocollide = true;
		eff->AddEvent(&SpellEffect::Decay, EVENT_OBJECT_DELETE, 30000, 1, EVENT_FLAG_NOT_IN_WORLD);
		m_caster->m_mapmgr->AddObject(eff);
	}
}

SpellStorage::SpellStorage()
{
	m_maxspellId = 0;
	FILE* f = fopen("encsp.dat", "rb");

	if (f == NULL)
		return;

	Crypto c;

	uint32 spellsize = 0x5C;

	fseek(f, 0, SEEK_END);
	uint32 filesize = ftell(f);
	fseek(f, 0, SEEK_SET);
	m_maxspellId = (filesize / spellsize);

	sLog.Debug("SpellStorage", "Number of spells: %u", m_maxspellId);

	GrowableArray<uint8> tmpbuf;
	uint8 b;
	for (uint32 i = 0; i < filesize; ++i)
	{
		fread(&b, 1, 1, f);
		tmpbuf.push(b);
	}

	for (uint32 i = 0; i < m_maxspellId; ++i)
	{
		uint8* buffer = &tmpbuf[i * spellsize];
		c.Decrypt(buffer, spellsize, i);
	}


#define GET(type, offset) *(type*)&tmpbuf[i * spellsize + offset]
#define GETSTR(offset) (char*)&tmpbuf[i * spellsize + offset]

	for (uint32 i = 0; i < (filesize / spellsize); ++i)
	{
		SpellData* sp = new SpellData;
		memset(sp, 0, sizeof(SpellData));
		sp->id = i;
		sp->school = GET(uint8, 38);
		sp->manareq = GET(uint16, 30);
		sp->skillreq = GET(uint16, 36);
		sp->name = GETSTR(42);

		while (sp->name.size() > 0 && sp->name[sp->name.size() - 1] == ' ')
			sp->name.erase(sp->name.size() - 1);

		uint32 baselevel = GET(uint8, 32);

		//ok now 0x2D means they dont get it, if they do get it, they get it at level baselevel + value - 0x41
		for (uint32 j = 0; j < NUM_CLASSES; ++j)
		{
			uint8 val = GET(uint8, 14 + j);
			if (val == 0x2D)
				sp->classlevel[j] = 0xFFFFFFFF;
			else
				sp->classlevel[j] = baselevel + val - 0x41;
		}

		m_data.insert(std::make_pair(i, sp));
	}


	fclose(f);

	Dump();
#undef GET
#undef GETSTR
}

void SpellStorage::Dump()
{
	auto f = fopen("spelldump.txt", "w");
	fprintf(f, "function InitSpellData()\n");
	for (auto i = 0; i < m_maxspellId; ++i)
	{
		auto ent = GetSpell(i);
		if (ent == NULL)
			continue;
		fprintf(f, "\tSpellInfo(%u, \"%s\", %u", i, ent->name.c_str(), ent->manareq);

		//levels classes get it
		for (auto j = 0; j < NUM_CLASSES; ++j)
		{
			fprintf(f, ", %d", ent->classlevel[j]);
		}

		fprintf(f, ");\n");
	}
	fprintf(f, "end\n");
	fclose(f);
}
