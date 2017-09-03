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

#ifndef __REFLECTIONBOSS_H
#define __REFLECTIONBOSS_H


class ReflectionBossAIScript : public CreatureAIScript
{
public:
	void OnCombatStart();

	void OnCombatStop() { RemoveEvents(); }

	void OnRespawn();

	void OnDeath();

	void OnAddToWorld()
	{
		if (m_unit->m_invis)
			AddEvent(&ReflectionBossAIScript::OnDeath, EVENT_UNKNOWN, 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void OnTakeDamage(Unit* from, int32 damage, SpellEntry* entry)
	{
		damage /= 5; //only do 20%

		if (entry == NULL || entry->id == 15002 || entry->id == 15003 || entry->id == 15004 || entry->id == 15005) //no spell or any bow dummy spells = deal damage to player, do not recast
			m_unit->DealDamage(from, damage, entry, DAMAGE_FLAG_DO_NOT_APPLY_BONUSES | DAMAGE_FLAG_DO_NOT_APPLY_RESISTS | DAMAGE_FLAG_NO_REFLECT);
		else
		{
			//ONLY COPY DAMAGE EFFECTS
			switch (entry->effect)
			{
				case SPELL_EFFECT_DAMAGE:
				case SPELL_EFFECT_CREATE_BEAM:
				case SPELL_EFFECT_CREATE_BOMB:
				case SPELL_EFFECT_CREATE_PROJECTILE:
				default:
				{
					m_unit->DealDamage(from, damage, entry, DAMAGE_FLAG_DO_NOT_APPLY_BONUSES | DAMAGE_FLAG_DO_NOT_APPLY_RESISTS | DAMAGE_FLAG_NO_REFLECT);
				} break;
			}

			TargetData targets;

			if (entry->targetallowedtype & SPELL_TARGET_UNIT)
			{
				targets.flags |= SPELL_TARGET_UNIT;
				targets.x = from->m_serverId;
			}
			else if (entry->targetallowedtype & SPELL_TARGET_GROUND)
			{
				targets.flags |= SPELL_TARGET_GROUND;
				targets.x = from->m_positionX;
				targets.x = from->m_positionY;
			}
			else if (entry->targetallowedtype & SPELL_TARGET_SELF)
			{
				targets.flags |= SPELL_TARGET_SELF;
			}


			Spell s(m_unit, entry->id, targets, true);

			s.m_damageflags |= DAMAGE_FLAG_DO_NOT_APPLY_BONUSES | DAMAGE_FLAG_DO_NOT_APPLY_RESISTS | DAMAGE_FLAG_NO_REFLECT;
			s.m_forcedvalue = damage;

			s.Cast();
		}
	}

	static CreatureAIScript* Create() { return new ReflectionBossAIScript; }
};

class ReflectionAIScript : public CreatureAIScript
{
public:
	void OnSetUnit()
	{
		m_unit->m_bonuses[REFLECT_DAMAGE] = 100;
	}
	static CreatureAIScript* Create() { return new ReflectionAIScript; }
};

class DummyFireballEmitterAI : public CreatureAIScript
{
public:
	void OnAddToWorld()
	{
		AddEvent(&DummyFireballEmitterAI::Shoot, EVENT_UNKNOWN, 400, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	void Shoot()
	{
		Creature* target = m_unit->m_mapmgr->GetCreatureById(120);
		if (target != NULL)
			m_unit->CastSpell(117, target, true);
	}

	static CreatureAIScript* Create() { return new DummyFireballEmitterAI; }
};

#endif
