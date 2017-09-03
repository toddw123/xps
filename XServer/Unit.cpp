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

void InitSkillLimits()
{
	//first memset it all

	//notes on skill limits
	/*

	you might get confused on some of the skill limits because of the optimizer making
	these bitwise because of an integer multiplication

	(1717986919i64 * word_3728564[v3]) >> 32

	thats optimized code

	unoptimized code: word_3728564[v3] * 0.4

	and

	(0xFFFFFFFF88888889ui64 * (word_3728564[v4] + word_3728564[v3]) >> 32)

	the Fs are mistakes the decompiler, 88888889 is the real value in hex

	.text:0040B222                 mov     eax, 88888889h
	.text:0040B227                 imul    ecx

	(2290649225 * (word_3728564[v4] + word_3728564[v3]) >> 32)

	Which is also optimized integer multiplication.

	Unoptimized: (0.533333333 * (word_3728564[v4] + word_3728564[v3]))

	*/
}

Unit::Unit()
{
	m_lastrage = 0;
	m_lastspellid = 0;
	m_lastspell = 0;
	m_followOwnerAngle = 0;
	m_destinationAngle = -1;
	m_lastdamager = 0;
	m_thrownweapon = 0;
	m_nextchatEvent = 0;
	m_unitspecialname = 0;
	m_unitspecialname2 = 0;
	m_unitflags = 0;
	m_unitAnimation = 0;
	m_updateCounter = 0;
	m_regenTimer = 0;
	m_endmove = 0;
	
	m_deathstate = STATE_ALIVE;
	m_nextmove = 0;
	m_attackRun = false;
	for (uint32 i=0; i<NUM_SPELL_AURAS; ++i)
		m_auras[i] = 0;
	m_spellflags = 0;
	m_buffflags = 0;
	m_nextspell = 0;
	m_health = 200;
	m_maxhealth = 200;

	m_laststrike = 0;
	m_laststrikeblock = false;
	m_spelleffect = 0;
	m_POIObject = 0;
	m_POIX = 0;
	m_POIY = 0;
	m_POI = 0;
	m_pendingSay = false;
	m_serverId = 0;
	m_typeId = TYPEID_UNIT;
	m_speed = 7;
	m_destinationX = 0;
	m_destinationY = 0;
	m_destinationRadius = 0;
	m_weaponDisplay = 0;
	m_shieldDisplay = 0;
	m_helmetDisplay = 0;
	m_color1 = 0;
	m_color2 = 0;
	m_mana = 0;
	m_maxmana = 0;

	for (uint32 i = 0; i < NUM_ITEM_BONUSES; ++i)
		m_bonuses[i] = 0;
	for (uint32 i = 0; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS + NUM_STORAGE_ITEMS; ++i)
		m_items[i] = NULL;
}

int Unit::GetUnitType()
{
	return this->m_typeId;
}

void Unit::SetUpdateBlock( uint8 block )
{
	for (std::map<Unit*, uint8>::iterator itr=m_inrangeUnits.begin(); itr!=m_inrangeUnits.end(); ++itr)
	{
		std::map<Unit*, uint8>::iterator itr2 = itr->first->m_inrangeUnits.find(this);
		if (itr2 != itr->first->m_inrangeUnits.end())
			itr2->second |= block;
	}
}

void Unit::Strike( Object* obj, bool ignoretimer /*= false*/ )
{
	//do block/etc later
	if (obj == NULL || !obj->IsUnit() || !TO_UNIT(obj)->IsAlive() || !IsInRange(obj))
		return;

#define CHECK_CANNOT_ATTACK(attackspeed) ((m_laststrike > g_mstime - attackspeed || m_nextspell > GetMSTime()) && !ignoretimer)

	if (TO_UNIT(obj)->HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
		return;

	if (ProtectedByGround() && IsPlayer())
		return;

	Item* curweapon = GetCurrentWeapon();

	if (curweapon != NULL && curweapon->m_data->id == m_thrownweapon)
		return;

	if (curweapon != NULL && curweapon->m_data->type == ITEM_TYPE_BOW)
	{
		FaceObject(obj);
		if (CHECK_CANNOT_ATTACK(GetAttackSpeed()))
			return;
		if (IsCreature() && (!InLineOfSight(obj)) || TO_UNIT(obj)->ProtectedByGround())
			return;
		TargetData tar;
		tar.flags = SPELL_TARGET_UNIT;
		tar.x = obj->m_serverId;
		uint32 spellid = 15003;
		if (m_bonuses[LIGHTNING_BOW] > 0)
			spellid = 15002; //shoot lightning arrow
		Spell s(this, spellid, tar, true);
		s.Cast();
		m_laststrike = g_mstime;
		m_laststrikeblock = true;
		if (!ignoretimer)
			SetAnimationType(ANIM_TYPE_ATTACK_SWING);
		RemoveAura(SPELL_AURA_HIDE);

		uint32 ragebonus = m_bonuses[RAGE_BONUS] + GetSkillValue(SKILL_RAGE_CHANCE) + GetSkillValue(SKILL_RAGE_CHANCE2) + GetSkillValue(SKILL_RAGE_CHANCE3) + GetSkillValue(SKILL_RAGE_CHANCE4);
		if (ragebonus && RandomUInt(99) < ragebonus && m_lastrage + 2000 <= g_mstime)
		{
			m_laststrike = 0; //reset attack timer
			m_lastrage = g_mstime;
		}
	}
	else if (m_bonuses[BONUS_LUMEN_WAND_BEAM] > 0)
	{
		if (CHECK_CANNOT_ATTACK(400))
			return;

		FaceObject(obj);
		TargetData tar;
		tar.flags = SPELL_TARGET_UNIT;
		tar.x = obj->m_serverId;
		Spell sp(this, 238, tar, true);
		sp.m_forcedvalue = GetMeleeDamage() * float(m_bonuses[BONUS_RADIUS_WAND_BEAM]) / 100;
		sp.Cast();
		StopMovement(700);
		SetAnimation(10, 11);
		m_laststrike = g_mstime;
		m_laststrikeblock = true;
		RemoveAura(SPELL_AURA_HIDE);
		StopAttacking();
	}
	else if (m_bonuses[BONUS_RADIUS_WAND_BEAM] > 0)
	{
		if (CHECK_CANNOT_ATTACK(1000))
			return;

		TargetData tar;
		tar.flags = SPELL_TARGET_UNIT;
		tar.x = obj->m_serverId;
		Spell sp(this, 261, tar, true);
		sp.m_forcedvalue = GetMeleeDamage() * float(200 + m_bonuses[BONUS_RADIUS_WAND_BEAM]) / 100;
		sp.Cast();
		StopMovement(700);
		SetAnimation(10, 11);
		m_laststrike = g_mstime;
		m_laststrikeblock = true;
		RemoveAura(SPELL_AURA_HIDE);
		StopAttacking();
	}
	else if (curweapon != NULL && curweapon->m_data->type3 == ITEM_TYPE_PROJECTILE && GetDistance(obj) > obj->m_bounding && curweapon->m_data->intreq == 0)
	{
		FaceObject(obj);
		if (CanCast(15044) && InLineOfSight(obj))
		{
			if (CHECK_CANNOT_ATTACK(GetAttackSpeed()))
				return;

			m_laststrike = g_mstime;
			m_laststrikeblock = true;
			CastSpell(15044, TO_UNIT(obj), true);
			SetAnimationType(ANIM_TYPE_ATTACK_SWING);
			RemoveAura(SPELL_AURA_HIDE);
		}
	}
	else //melee
	{
		if (GetDistance(obj) > obj->m_bounding * 1.2f)
			return;

		FaceObject(obj);

		if (CHECK_CANNOT_ATTACK(GetAttackSpeed()))
			return;

		Unit* u = TO_UNIT(obj);
		RemoveAura(SPELL_AURA_HIDE);
		if (DidHit(u))
		{
			bool candaze = false;
			bool canthrust = false;
			bool canbleed = false;

			if (curweapon != NULL)
			{
				switch (curweapon->m_data->type2)
				{
				case WEAPON_TYPE_AXE_2H:
				case WEAPON_TYPE_SWORD_2H:
				case WEAPON_TYPE_BLUNT:
				case WEAPON_TYPE_BLUNT_2H:
					candaze = true; break;
				case WEAPON_TYPE_DAGGER:
					canthrust = true; break;
				case WEAPON_TYPE_SHORT_SWORD:
					canthrust = true; canbleed = true; break;
				case WEAPON_TYPE_AXE:
				case WEAPON_TYPE_SWORD:
					canbleed = true; break;
				}
			}

			uint32 spelleff = 1; //blood effect

			if (!u->IsAttacking() && u->m_unitAnimation != 10 && u->m_unitAnimation != 11 && (!IsCreature() || TO_CREATURE(this)->m_proto->boss != 1))
				u->SetAnimationType(ANIM_TYPE_HIT);
			SetAnimationType(ANIM_TYPE_ATTACK_SWING);
			uint32 damage = float(GetMeleeDamage()) * (float(RandomUInt(99)) / 1000 + 0.95);

			if (m_bonuses[ON_DAMAGE_ICE_DAMAGE])
			{
				spelleff = 32;
				uint32 damage2 = damage * (m_bonuses[ON_DAMAGE_ICE_DAMAGE]) / 100;
				AddEvent(&Unit::DealDamageID, uint32(u->m_serverId), int32(damage2), (SpellEntry*)NULL, uint32(DAMAGE_FLAG_DO_NOT_APPLY_BONUSES | DAMAGE_FLAG_DO_NOT_APPLY_RESISTS | DAMAGE_FLAG_NO_REFLECT | DAMAGE_FLAG_CANNOT_PROC), EVENT_UNIT_DEALDAMAGE, 200, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
			if (m_bonuses[ON_DAMAGE_FIRE_DAMAGE])
			{
				spelleff = 34;
				uint32 damage2 = damage * float(m_bonuses[ON_DAMAGE_FIRE_DAMAGE]) / 100;
				AddEvent(&Unit::DealDamageID, uint32(u->m_serverId), int32(damage2), (SpellEntry*)NULL, uint32(DAMAGE_FLAG_DO_NOT_APPLY_BONUSES | DAMAGE_FLAG_DO_NOT_APPLY_RESISTS | DAMAGE_FLAG_NO_REFLECT | DAMAGE_FLAG_CANNOT_PROC), EVENT_UNIT_DEALDAMAGE, 200, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
			if (m_bonuses[ON_DAMAGE_ACID_DAMAGE])
			{
				spelleff = 36;
				uint32 damage2 = damage * float(m_bonuses[ON_DAMAGE_ACID_DAMAGE]) / 100;
				AddEvent(&Unit::DealDamageID, uint32(u->m_serverId), int32(damage2), (SpellEntry*)NULL, uint32(DAMAGE_FLAG_DO_NOT_APPLY_BONUSES | DAMAGE_FLAG_DO_NOT_APPLY_RESISTS | DAMAGE_FLAG_NO_REFLECT | DAMAGE_FLAG_CANNOT_PROC), EVENT_UNIT_DEALDAMAGE, 200, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
			if (m_bonuses[ON_DAMAGE_LIGHTNING_DAMAGE])
			{
				spelleff = 40;
				uint32 damage2 = damage * float(m_bonuses[ON_DAMAGE_LIGHTNING_DAMAGE]) / 100;
				AddEvent(&Unit::DealDamageID, uint32(u->m_serverId), int32(damage2), (SpellEntry*)NULL, uint32(DAMAGE_FLAG_DO_NOT_APPLY_BONUSES | DAMAGE_FLAG_DO_NOT_APPLY_RESISTS | DAMAGE_FLAG_NO_REFLECT | DAMAGE_FLAG_CANNOT_PROC), EVENT_UNIT_DEALDAMAGE, 200, 1, EVENT_FLAG_NOT_IN_WORLD);
			}

			u->SetSpellEffect(spelleff); //blood efffect

			if (candaze && RandomUInt(99) < GetSkillValue(SKILL_DAZE))
				CastSpell(15033, u, true);
			if (canthrust && RandomUInt(99) < GetSkillValue(SKILL_THRUST))
				damage *= 1.10;
			if (canbleed && RandomUInt(99) < GetSkillValue(SKILL_BLEED))
			{
				//bleed is 40% damage, but we proc 4 times from aura
				uint32 bleedvalue = damage * 0.4 * 0.25;
				TargetData tar;
				tar.flags = SPELL_TARGET_UNIT;
				tar.x = u->m_serverId;
				Spell s(this, 15034, tar, true);
				s.m_forcedvalue = bleedvalue;
				s.Cast();
			}

			if (HasAura(SPELL_AURA_BREAKER) && RandomUInt(99) < 60)
			{
				CastSpell(164, u, true);
				u->RemoveAllDispellableAuras();
			}

			//handle procs
			if (m_bonuses[ON_HIT_DISPEL_CHANCE] && RandomUInt(99) < m_bonuses[ON_HIT_DISPEL_CHANCE])
				CastSpell(164, u, true);
			if (m_bonuses[BONUS_FRAGOR_CHANCE] && RandomUInt(99) < m_bonuses[BONUS_FRAGOR_CHANCE])
				CastSpell(253, true, (uint32)DAMAGE_FLAG_CANNOT_PROC | DAMAGE_FLAG_NO_REFLECT);

			//mod health
			DealDamage(u, damage);

			m_attackRun = false;
		}
		else
		{
			bool block = (u->m_items[ITEM_SLOT_SHIELD] != NULL && curweapon != NULL);

			if (block)
			{
				SetAnimationType(ANIM_TYPE_ATTACK_BLOCK);
				u->SetAnimation(ANIMATION_SHIELDBLOCK, ANIMATION_SHIELDBLOCK_2);
			}
			else
				SetAnimationType(ANIM_TYPE_ATTACK_MISS);
		}

		m_laststrike = g_mstime;
		m_laststrikeblock = true;

		uint32 ragebonus = m_bonuses[RAGE_BONUS] + GetSkillValue(SKILL_RAGE_CHANCE) + GetSkillValue(SKILL_RAGE_CHANCE2) + GetSkillValue(SKILL_RAGE_CHANCE3) + GetSkillValue(SKILL_RAGE_CHANCE4);
		if (ragebonus && RandomUInt(99) < ragebonus && m_lastrage + 2000 <= g_mstime)
		{
			m_laststrike = 0; //reset attack timer
			m_lastrage = g_mstime;
		}
	}
}

void Unit::Update()
{
// 	char test[1024];
// 	sprintf(test, "POI %u U %u X %u Y %u", m_POI, m_POIObject, m_POIX, m_POIY);
// 	SendChatMsg(test);
	Object::Update();

	Object* followingObject = NULL;
	++m_updateCounter;

	if (!(m_updateCounter % 10))
	{
		if (ProtectedByGround()) //square
		{
			RemoveAura(SPELL_AURA_INVISIBILITY);
			RemoveAura(SPELL_AURA_HIDE);
		}
	}

#define SLIDER_SPEED 5

	switch (m_mapmgr->GetMapLayer1(m_positionX, m_positionY))
	{
	case 240: //slider: up
		{
			//try a smaller one
			for (uint32 i = SLIDER_SPEED; i > 0; --i)
			{
				if (m_mapmgr->CanPass(m_positionX / 20, (m_positionY - i) / 20) || m_mapmgr->GetMapLayer1(m_positionX, m_positionY) == 240)
					SetPosition(m_positionX, m_positionY - i);
			}
			StopMovement();
		} break;
	case 241: //slider: down
		{
			//try a smaller one
			for (uint32 i = SLIDER_SPEED; i > 0; --i)
			{
				if (m_mapmgr->CanPass(m_positionX / 20, (m_positionY + i) / 20) || m_mapmgr->GetMapLayer1(m_positionX, m_positionY) == 241)
					SetPosition(m_positionX, m_positionY + i);
			}
			StopMovement();
		} break;
	case 242: //slider: left
		{
			//try a smaller one
			for (uint32 i = SLIDER_SPEED; i > 0; --i)
			{
				if (m_mapmgr->CanPass((m_positionX - i) / 20, m_positionY / 20) || m_mapmgr->GetMapLayer1(m_positionX, m_positionY) == 242)
					SetPosition(m_positionX - i, m_positionY );
			}
			StopMovement();
		} break;
	case 243: //slider: right
		{
			//try a smaller one
			for (uint32 i = SLIDER_SPEED; i > 0; --i)
			{
				if (m_mapmgr->CanPass((m_positionX + i) / 20, m_positionY / 20) || m_mapmgr->GetMapLayer1(m_positionX, m_positionY) == 243)
					SetPosition(m_positionX + i, m_positionY );
			}
			StopMovement();
		} break;
	}

	switch (m_mapmgr->GetMapLayer2(m_positionX, m_positionY))
	{
	case 0xCF: //cliff drop
		{
			if (HasAura(SPELL_AURA_MUTATIO_NIMBUS) || HasAura(SPELL_AURA_MUTATIO_FLAMMA))
				break;

			StopMovement(200);
			if (m_unitAnimation != 21)
				SetAnimation(21, 0);

			if (g_mstime - m_lastmove >= 5000 && IsAlive())
				SetHealth(0);
		} break;
	}

	if (m_POIObject != 0)
	{
		followingObject = m_mapmgr->GetObject(m_POIObject);
		if (followingObject == NULL)
		{
			StopAttacking();
			StopMovement();
		}
		else if (!followingObject->IsUnit() || !TO_UNIT(followingObject)->IsAlive() || !IsAlive())
		{
			StopMovement();
		}
		else
		{
			float distobj = GetDistance(followingObject);
			bool distancecheck = (distobj < SIGHT_RANGE * 3);

			int32 pathx = followingObject->m_positionX;
			int32 pathy = followingObject->m_positionY;
			bool canmakepath = CreatePath(pathx, pathy);
			if (!distancecheck || !canmakepath)
			{
				if (!distancecheck)
					OnPOIFail(POI_OUT_OF_RANGE);
				else if (!canmakepath)
					OnPOIFail(POI_CANNOT_CREATE_PATH);
				ClearPOI();
			}
		}
	}

	if (m_POIX != 0 && m_POIY != NULL)
	{
		if (!CreatePath(m_POIX, m_POIY))
		{
			OnPOIFail(POI_CANNOT_CREATE_PATH);
			ClearPOI();
		}
	}

	UpdateMovement(followingObject);

	if ((HasAnimationType(ANIM_TYPE_MOVING) || HasAnimationType(ANIM_TYPE_ATTACK_MOVING) || m_unitAnimation == ANIMATION_ATTACKRUN) && m_lastmove < (g_mstime - 100))
		SetAnimationType(ANIM_TYPE_STANDING);

	//Melee Attack
	if (IsAlive() && IsAttacking() && m_POIObject != 0 && !HasUnitFlag(UNIT_FLAG_CANNOT_ATTACK))
	{
		if (followingObject == NULL)
			followingObject = m_mapmgr->GetObject(m_POIObject);
		Strike(followingObject);
	}

	if ((IsPlayer() || IsPet()) && IsAlive() && m_regenTimer < GetMSTime())
		Regen();
}

void Unit::SetModels( int32 charmodel /*= -1*/, int32 weaponmodel /*= -1*/, int32 shieldmodel /*= -1*/, int32 helmmodel /*= -1*/, int32 color1 /*= -1*/, int32 color2 /*= -1*/ )
{
	if (charmodel != -1)
		m_charDisplay = charmodel;
	if (weaponmodel != -1)
		m_weaponDisplay = weaponmodel;
	if (shieldmodel != -1)
		m_shieldDisplay = shieldmodel;
	if (helmmodel != -1)
		m_helmetDisplay = helmmodel;
	if (color1 != -1)
		m_color1 = color1;
	if (color2 != -1)
		m_color2 = color2;

	SetUpdateBlock(UPDATETYPE_MODELS); //model block

	if (IsPlayer())
	{
		if (HasAura(SPELL_AURA_FIERCE))
		{
			auto itemclass = TO_PLAYER(this)->GetItemClass();
			if (itemclass & (ITEM_CLASS_WIZARD | ITEM_CLASS_WARLOCK | ITEM_CLASS_CLERIC))
				m_charDisplay = 536;
			else
				m_charDisplay = 533;
		}

		ServerPacket datas2(MSG_UPDATE_LOCAL_MODELS);
		datas2 << m_shieldDisplay;
		datas2 << m_helmetDisplay;
		datas2 << m_charDisplay; //display id
		datas2 << m_weaponDisplay; //weaponid
		datas2 << m_serverId;
		uint16 checksum = 0;
		uint8* datas2buf = datas2.GetBuffer();
		for (uint8 i = 0; i < 6; ++i)
			checksum += *(int8*)&datas2buf[i + 1];
		datas2 << checksum;

		TO_PLAYER(this)->Send(&datas2);
	}
}

void Unit::MoveToNextPathPoint()
{
	if (m_pathpoint >= m_pathdata.size())
		return;

	PathPoint* nextnode = &m_pathdata[m_pathpoint++];

	m_startX = m_positionX;
	m_startY = m_positionY;
	m_destinationX = nextnode->x;
	m_destinationY = nextnode->y;

	m_startmove = nextnode->prevarrivetime;
	m_endmove = nextnode->arrivetime;
}

void Unit::OnHealthChange( uint32 old )
{
	m_lasttargetupdate = g_mstime;

	SendHealthUpdate();


	if (GetHealth() == 0)
	{
		//remove poison LOL
		RemoveAura(SPELL_AURA_POISON);
		RemoveAura(SPELL_AURA_DISINTEGRATION_EFFECT);
		RemoveAura(SPELL_AURA_HIDE);

		bool bossinrange = false;
		for (std::map<Unit*, uint8>::iterator itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
		{
			if (!itr->first->IsCreature() || TO_CREATURE(itr->first)->m_proto->boss != 1)
				continue;

			bossinrange = true;
			break;
		}

		if (!bossinrange && !HasUnitFlag(UNIT_FLAG_CANNOT_RESTORE_OR_REVIVE))
		{
			uint32 maximuschance = m_bonuses[BONUS_MAXIMUS_CHANCE];
			if (maximuschance > 75)
				maximuschance = 75;
			if (maximuschance && RandomUInt(99) < maximuschance)
			{
				Player* pthis = NULL;
				if (IsPet())
				{
					Unit* owner = TO_CREATURE(this)->GetOwner();
					if (owner != NULL && owner->IsPlayer())
						pthis = TO_PLAYER(owner);
				}
				if (pthis != NULL)
				{
					Object* other = m_mapmgr->GetObject(m_lastdamager);
					if (other != NULL && other->IsPet())
						other = TO_CREATURE(other)->GetOwner();
					if (other != NULL && other->IsPlayer())
						TO_PLAYER(other)->GivePVPExp(pthis, PVP_KILL_ALMUS);
				}
				SetHealth(m_maxhealth);
				SetSpellEffect(52);
				RemoveAllDispellableAuras();
				return;
			}

			if (HasAura(SPELL_AURA_ALMUS))
			{
				Player* pthis = NULL;
				if (IsPlayer())
					pthis = TO_PLAYER(this);
				if (IsPet())
				{
					Unit* owner = TO_CREATURE(this)->GetOwner();
					if (owner != NULL && owner->IsPlayer())
						pthis = TO_PLAYER(owner);
				}
				if (pthis != NULL)
				{
					Object* other = m_mapmgr->GetObject(m_lastdamager);
					if (other != NULL && other->IsPet())
						other = TO_CREATURE(other)->GetOwner();
					if (other != NULL && other->IsPlayer())
						TO_PLAYER(other)->GivePVPExp(pthis, PVP_KILL_ALMUS);
				}
				m_auras[SPELL_AURA_ALMUS]->Remove();
				SetHealth(m_maxhealth);
				SetSpellEffect(52);
				RemoveAllDispellableAuras();
				return;
			}

			//maximus amulet
			Item* itemtmp = FindEquippedItem(227);
			if (itemtmp != NULL)
			{
				itemtmp->Delete();
				SetHealth(m_maxhealth);
				SetSpellEffect(52);
				RemoveAllDispellableAuras();
				return;
			}


			if (IsPlayer() && m_bonuses[BONUS_REDITUS_CHANCE] && RandomUInt(99) < m_bonuses[BONUS_REDITUS_CHANCE])
			{
				Player* pthis = NULL;
				if (IsPlayer())
					pthis = TO_PLAYER(this);
				if (IsPet())
				{
					Unit* owner = TO_CREATURE(this)->GetOwner();
					if (owner != NULL && owner->IsPlayer())
						pthis = TO_PLAYER(owner);
				}
				if (pthis != NULL)
				{
					Object* other = m_mapmgr->GetObject(m_lastdamager);
					if (other != NULL && other->IsPlayer())
						TO_PLAYER(other)->GivePVPExp(pthis, PVP_KILL_REDITUS);
				}
				SetHealth(1);
				TO_PLAYER(this)->PortToMark(1);
				RemoveAllDispellableAuras();
				return;
			}

			itemtmp = FindEquippedItem(222);
			if (itemtmp != NULL && IsPlayer())
			{
				Player* pthis = NULL;
				if (IsPlayer())
					pthis = TO_PLAYER(this);
				if (IsPet())
				{
					Unit* owner = TO_CREATURE(this)->GetOwner();
					if (owner != NULL && owner->IsPlayer())
						pthis = TO_PLAYER(owner);
				}
				if (pthis != NULL)
				{
					Object* other = m_mapmgr->GetObject(m_lastdamager);
					if (other != NULL && other->IsPet())
						other = TO_CREATURE(other)->GetOwner();
					if (other != NULL && other->IsPlayer())
						TO_PLAYER(other)->GivePVPExp(pthis, PVP_KILL_REDITUS);
				}
				itemtmp->Delete();
				SetHealth(1);
				TO_PLAYER(this)->PortToMark(1);
				RemoveAllDispellableAuras();
				return;
			}
		}

		StopMovement();
		RemoveAllDispellableAuras();

		if (m_unitAnimation != 21 && m_unitAnimation != 45)
		{
			if (RandomUInt(1) == 0)
				m_unitAnimation = 21;
			else
				m_unitAnimation = 45;
		}
		SetUpdateBlock(UPDATETYPE_ANIMATION);
		m_deathstate = STATE_DEAD;
	}

	if (m_health > m_maxhealth)
		m_health = m_maxhealth;
}

uint8 Unit::GetReaction( Unit* other )
{
	if (this == other)
		return REACTION_FRIENDLY;

	if (IsCreature() && TO_CREATURE(this)->m_script != NULL)
	{
		uint32 script_react = TO_CREATURE(this)->m_script->GetReaction(other);

		if (script_react != REACTION_SCRIPT_NOT_SET_USE_SERVER_DEFAULT)
			return script_react;
	}

	if (other->IsCreature() && TO_CREATURE(other)->m_script != NULL)
	{
		uint32 script_react = TO_CREATURE(other)->m_script->GetReaction(this);

		if (script_react != REACTION_SCRIPT_NOT_SET_USE_SERVER_DEFAULT)
			return script_react;
	}

	if (IsPet())
	{
		Unit* owner = TO_CREATURE(this)->GetOwner();

		if (owner != NULL)
			return owner->GetReaction(other);
	}

	if (other->IsPet())
	{
		Unit* owner = TO_CREATURE(other)->GetOwner();

		if (owner != NULL)
			return GetReaction(owner);
	}

	if (IsPlayer())
	{
		Player* pthis = TO_PLAYER(this);
		if (other->HasUnitFlag(UNIT_FLAG_DO_NOT_ATTACK_PLAYERS))
			return REACTION_FRIENDLY;
		if (other->IsPlayer() && pthis->GetLinkID() != 0 && pthis->GetLinkID() == TO_PLAYER(other)->GetLinkID())
			return REACTION_FRIENDLY;
		if (m_mapmgr == NULL)
			return REACTION_FRIENDLY;
		if (m_mapmgr->m_serverType == 0) //GVE
		{
			if (pthis->m_attackstate == ATTACK_STATE_ALL)
				return REACTION_HOSTILE;
			if (pthis->m_attackstate == ATTACK_STATE_ENEMIES && other->IsPlayer() && (other->m_faction != m_faction || TO_PLAYER(other)->m_alignment < 0))
				return REACTION_HOSTILE;
			if (pthis->m_attackstate == ATTACK_STATE_MONSTERS && (other->m_faction == m_faction || other->IsPlayer()))
				return REACTION_FRIENDLY;
		}
		if (m_mapmgr->m_serverType == 1) //ANTIPK
		{
			//if on -all or -enemies, and enemy is in pvp mode, we can attack
			if (other->IsPlayer() && pthis->m_attackstate != ATTACK_STATE_MONSTERS && TO_PLAYER(other)->InPVPMode())
				return REACTION_HOSTILE;
			if (other->m_faction == FACTION_EVIL) //in antipk mode, evil npcs are not hostile to players
				return REACTION_FRIENDLY;
		}
		if (m_mapmgr->m_serverType == 2) //FULLPK
		{
			if (other->IsPlayer())
			{
				auto plrother = TO_PLAYER(other);

				if (pthis->GetLinkID() != 0 && pthis->GetLinkID() == plrother->GetLinkID()) //linked = friendly
					return REACTION_FRIENDLY;
				if (pthis->m_attackstate == ATTACK_STATE_MONSTERS)
					return REACTION_FRIENDLY;
				
				return REACTION_HOSTILE;
			}
		}
	}

	if (other->IsPlayer() && !IsPlayer())
	{
		if (m_faction == FACTION_GOOD || m_faction == FACTION_EVIL || m_faction == FACTION_NEUTRAL)
		{
			if (TO_PLAYER(other)->m_alignment < 0)
				return REACTION_HOSTILE; //alignment is bad, we attack!!!
		}
	}

	if (m_faction == FACTION_NEUTRAL && (other->m_faction == FACTION_NPC_EVIL || other->m_faction == FACTION_NPC_EVIL_HOSTILE))
		return REACTION_HOSTILE;
	if (m_faction == FACTION_GOOD)
	{
		if (other->m_faction == FACTION_EVIL || other->m_faction == FACTION_NPC_EVIL || other->m_faction == FACTION_NPC_EVIL_HOSTILE)
			return REACTION_HOSTILE;
	}
	if (m_faction == FACTION_EVIL)
	{
		if (other->m_faction == FACTION_GOOD || other->m_faction == FACTION_NPC_EVIL || other->m_faction == FACTION_NPC_EVIL_HOSTILE)
			return REACTION_HOSTILE;
	}
	if (m_faction == FACTION_NPC_EVIL_HOSTILE && other->m_faction != FACTION_NPC_EVIL && other->m_faction != FACTION_NPC_EVIL_HOSTILE)
		return REACTION_HOSTILE;
	if (other->m_faction == FACTION_NPC_EVIL || m_faction == FACTION_NPC_EVIL)
		return REACTION_NEUTRAL_ATTACKABLE;
	return REACTION_FRIENDLY;
}

bool Unit::IsHostile( Unit* other )
{
	uint8 reaction = GetReaction(other);

	if (other->IsPlayer() && !IsPlayer())
	{
		if (HasUnitFlag(UNIT_FLAG_DO_NOT_ATTACK_PLAYERS))
			return false;
	}

	switch (reaction)
	{
	case REACTION_NEUTRAL:
	case REACTION_NEUTRAL_ATTACKABLE:
	case REACTION_FRIENDLY:
		{
			return false;
		} break;
	}

	return true;
}

void Unit::OnCollision( Object* o )
{
	if (o->IsUnit())
	{
		//pick random new spot
// 		int32 newx = m_positionX - 20 + RandomUInt(40);
// 		int32 newy = m_positionY - 20 + RandomUInt(40);
// 		if (m_mapmgr->CanPass(newx / 20, newy / 20))
// 			AddEvent(&Unit::SetPosition, newx, newy, EVENT_SET_POSITION, 0, 1, 0);
	}
}

bool Unit::IsAttackable( Unit* other )
{
	if (IsHostile(other))
		return true;
	return false;
}

void Unit::DealDamage( Unit* other, int32 damage, SpellEntry* spell /*= NULL*/, uint32 damageflags )
{
	if (other == this || !other->IsAlive() || other->HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
		return;

	if (other->m_POI == POI_EVADE)
		return;

	Unit* owner = NULL;
	Player* powner = NULL;
	Unit* oowner = NULL;

	if (IsPet())
		owner = TO_CREATURE(this)->GetOwner();
	if (owner == NULL)
		owner = this;
	if (owner->IsPlayer())
		powner = TO_PLAYER(owner);
	if (other->IsPet())
	{
		oowner = TO_CREATURE(other)->GetOwner();
		if (!TO_CREATURE(other)->m_summoned && oowner != NULL && oowner->IsCreature() && TO_CREATURE(oowner)->m_proto->boss != 2) //guard
			damage *= 0.1;
	}
	if (oowner == NULL)
		oowner = other;
	if (powner != NULL && !owner->IsAttackable(other))
		return;

	if (other->IsPlayer() && TO_PLAYER(other)->m_pendingmaptransfer.newmapid != 0xFFFFFFFF)
		return;

	if (other->IsPlayer() && IsPlayer() && TO_PLAYER(this)->m_account->m_data->accountlevel > 0)
		return;

	//dummy fight no dmg
	if ((owner->HasUnitFlag(UNIT_FLAG_NPC_FIGHTS_NO_DAMAGE) || oowner->HasUnitFlag(UNIT_FLAG_NPC_FIGHTS_NO_DAMAGE)) && (powner == NULL && !oowner->IsPlayer()))
		return;

	if (other->HasAura(SPELL_AURA_OMNI_SHIELD))
	{
		other->m_auras[SPELL_AURA_OMNI_SHIELD]->RemoveCharge();
		return;
	}

	//safety square
	if (other->ProtectedByGround() && (!IsCreature() || TO_CREATURE(this)->m_proto->boss == 0))
		return;

	if (powner != NULL && oowner->IsCreature() && TO_CREATURE(oowner)->m_proto->boss == 1)
	{
		if (TO_CREATURE(oowner)->m_killedplayers.find(powner->m_playerId) != TO_CREATURE(oowner)->m_killedplayers.end())
		{
			SendChatMsg("Cannot deal damage this attempt.");
			return;
		}
	}

	/*if (powner != NULL)
	{
		if (other->IsPlayer())
		{
			uint32 continent = GetContinentType();
			if (continent == CONTINENT_TYPE_ANTI_PK_GOOD && other->m_faction == FACTION_GOOD && TO_PLAYER(other)->m_alignment > 0 && !TO_PLAYER(other)->InPVPMode())
				return;
			if (continent == CONTINENT_TYPE_ANTI_PK_EVIL && other->m_faction == FACTION_EVIL && TO_PLAYER(other)->m_alignment > 0 && !TO_PLAYER(other)->InPVPMode())
				return;
			if (continent == CONTINENT_TYPE_NO_PK && TO_PLAYER(other)->m_alignment > 0 && !TO_PLAYER(other)->InPVPMode())
				return;
			if (continent != CONTINENT_TYPE_FULL_PK && other->GetLevel() < 30 && !TO_PLAYER(other)->InPVPMode())
				return;
		}
	}*/

	bool usearmor = false;

	if (damageflags & DAMAGE_FLAG_IS_MELEE_ATTACK)
		usearmor = true;

	//apply bonuses and resists
	if (spell != NULL)
	{
		float bonus = 1;
		float resist = 1;
		SpellData* spdata = sSpellStorage.GetSpell(spell->id);
		int32 school = -1;
		if (spdata != NULL)
			school = spdata->school;
		if (spell->forcedschool != -1)
			school = spell->forcedschool;
		switch (school)
		{
		case SPELL_SCHOOL_PHYSICAL:
		case SPELL_SCHOOL_SLASHING:
		case SPELL_SCHOOL_PIERCING:
		case SPELL_SCHOOL_SMASHING:
			{
				usearmor = true;
			} break;
		case SPELL_SCHOOL_FIRE:
			{
				bonus = GetMagicBonus(FIRE_MAGIC_BONUS);
				resist = other->GetMagicResist(PROTECTION_FIRE, true);
			} break;
		case SPELL_SCHOOL_ICE:
			{
				bonus = GetMagicBonus(ICE_MAGIC_BONUS);
				resist = other->GetMagicResist(PROTECTION_ICE, true);
			} break;
		case SPELL_SCHOOL_ENERGY:
			{
				bonus = GetMagicBonus(ENERGY_MAGIC_BONUS);
				resist = other->GetMagicResist(PROTECTION_ENERGY, true);

				//critical energy skill
				uint32 criticalenergy = GetSkillValue(SKILL_CRITICAL_ENERGY) + GetSkillValue(SKILL_CRITICAL_ENERGY2);
				if (criticalenergy > 0 && RandomUInt(99) < criticalenergy)
					damage *= 1.5;
			} break;
		case SPELL_SCHOOL_SHADOW:
			{
				bonus = GetMagicBonus(SHADOW_MAGIC_BONUS);
				resist = other->GetMagicResist(PROTECTION_SHADOW, true);
			} break;
		case SPELL_SCHOOL_SPIRIT:
			{
				bonus = GetMagicBonus(SPIRIT_MAGIC_BONUS);
				resist = other->GetMagicResist(PROTECTION_SPIRIT, true);
			} break;
		default:
			{
				//apply this to non school spells
				if (!(damageflags & DAMAGE_FLAG_DO_NOT_APPLY_RESISTS))
					bonus *= float(100 + m_bonuses[BONUS_DAMAGE_PCT]) / 100;
			} break;
		}

		if (!(damageflags & DAMAGE_FLAG_DO_NOT_APPLY_BONUSES))
			damage *= bonus;

		if (damageflags & DAMAGE_FLAG_IS_POISON_ATTACK && HasAura(SPELL_AURA_SHADOW_POWER))
			damage *= 1.20f;


		if (IsCreature() && m_level != TO_CREATURE(this)->m_proto->level && TO_CREATURE(this)->m_proto->boss != 1)
			damage *= float(100 + m_level - TO_CREATURE(this)->m_proto->level) / 100;

		if (spell != NULL && HasAura(SPELL_AURA_SPELL_POWER))
		{
			damage *= float(100 + GetSkillValue(SKILL_SPELL_POWER) * 4) / 100;
			m_auras[SPELL_AURA_SPELL_POWER]->RemoveCharge();
		}

		if (!(damageflags & DAMAGE_FLAG_DO_NOT_APPLY_RESISTS))
			damage /= resist;
	}
	else
		usearmor = true;

	if (usearmor)
	{
		float bonus = 1;

		if (!(damageflags & DAMAGE_FLAG_DO_NOT_APPLY_BONUSES))
			bonus *= float(100 + m_bonuses[BONUS_DAMAGE_PCT]) / 100;

		uint32 lethal = m_bonuses[ON_HIT_CHANCE_LETHAL] + GetSkillValue(SKILL_LETHAL) + GetSkillValue(SKILL_LETHAL2);
		if (!(damageflags & DAMAGE_FLAG_CANNOT_PROC) && lethal > 0 && RandomUInt(99) < lethal)
		{
			bonus *= 1.2f;
		}

		uint32 ferocious = m_bonuses[ON_HIT_FEROCIAS_ATTACK] + GetSkillValue(SKILL_FEROCIOUS_HIT) + GetSkillValue(SKILL_FEROCIOUS_HIT2) + GetSkillValue(SKILL_FEROCIOUS_HIT3) + GetSkillValue(SKILL_FEROCIOUS_HIT4);
		if (!(damageflags & DAMAGE_FLAG_CANNOT_PROC) && ferocious > 0 && RandomUInt(99) < ferocious)
		{
			bonus *= 1.2f;
		}


		uint32 armorbonus = other->m_bonuses[PROTECTION_ARMOR_BONUS];

		if (other->HasAura(SPELL_AURA_CORRUPT_ARMOR_EFFECT))
			armorbonus *= 0.7;

		float resist = float(100 - float(armorbonus + other->m_bonuses[PROTECTION_TEURI]) / 10 * 1.3) / 100;

		if (resist < 0.25)
			resist = 0.25;

		if (!(damageflags & DAMAGE_FLAG_DO_NOT_APPLY_BONUSES))
			damage *= bonus;
		if (!(damageflags & DAMAGE_FLAG_DO_NOT_APPLY_RESISTS))
			damage *= resist;

		damage += m_bonuses[BONUS_PLUS_DAMAGE];
	}

	if (damage <= 0)
		return;

	if (HasAura(SPELL_AURA_WHITE_POT) && (!IsPlayer() || !other->IsPlayer()) && (!other->IsCreature() || TO_CREATURE(other)->m_proto->boss == 0))
		damage *= 2.5f;

	if (!(damageflags & DAMAGE_FLAG_CANNOT_PROC))
	{
		if (m_bonuses[ON_HIT_DISPEL_CHANCE_CHAOTIC] && RandomUInt(99) < m_bonuses[ON_HIT_DISPEL_CHANCE_CHAOTIC])
		{
			CastSpell(164, other, true, DAMAGE_FLAG_CANNOT_PROC | DAMAGE_FLAG_NO_REFLECT);
			other->RemoveAllDispellableAuras();
		}

		if (m_bonuses[ON_HIT_DISPEL_CHANCE_CHAOTIC] && RandomUInt(99) < m_bonuses[ON_HIT_DISPEL_CHANCE_CHAOTIC])
		{
			CastSpell(29, other, true, DAMAGE_FLAG_CANNOT_PROC | DAMAGE_FLAG_NO_REFLECT);
		}

		if (m_bonuses[ON_HIT_ENERGISE_CHANCE] && RandomUInt(99) < m_bonuses[ON_HIT_ENERGISE_CHANCE])
		{
			SetMana(m_maxmana);
			CastSpell(172, other, true, DAMAGE_FLAG_CANNOT_PROC | DAMAGE_FLAG_NO_REFLECT);
		}

		if (m_bonuses[ON_HIT_ANIMUS_CHANCE] && RandomUInt(99) < m_bonuses[ON_HIT_ANIMUS_CHANCE])
		{
			other->SetMana(0);
			CastSpell(40, other, true, DAMAGE_FLAG_CANNOT_PROC | DAMAGE_FLAG_NO_REFLECT);
		}
	}

	if (!HasUnitFlag(UNIT_FLAG_UNATTACKABLE) && IsAlive() && !(damageflags & DAMAGE_FLAG_NO_REFLECT))
	{
		if (other->HasAura(SPELL_AURA_NOXA_TUERI) && !HasAura(SPELL_AURA_NOXA_TUERI))
		{
			uint32 noxadamage = other->m_auras[SPELL_AURA_NOXA_TUERI]->m_spell->value;
			if (other->HasAura(SPELL_AURA_DAMAGE_SHIELD))
				noxadamage *= 1.10;
			other->DealDamage(this, noxadamage, other->m_auras[SPELL_AURA_NOXA_TUERI]->m_spell, DAMAGE_FLAG_NO_REFLECT | DAMAGE_FLAG_CANNOT_PROC);
		}

		if (!other->IsCreature() && other->m_bonuses[REFLECT_DAMAGE] && RandomUInt(99) < other->m_bonuses[REFLECT_DAMAGE])
		{
			other->DealDamage(this, damage, NULL, DAMAGE_FLAG_NO_REFLECT | DAMAGE_FLAG_CANNOT_PROC);
		}


		if (!(damageflags & DAMAGE_FLAG_CANNOT_PROC) && !HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
		{
			//handle away skill
			if (!HasUnitFlag(UNIT_FLAG_CANNOT_MOVE) && other->GetSkillValue(SKILL_AWAY) > 0 && RandomUInt(99) < (other->GetSkillValue(SKILL_AWAY) * 2))
			{
				int32 newx = -1;
				int32 newy = -1;
				uint32 tries = 0;

				while (!m_mapmgr->CanPass(newx / 20, newy / 20, GetPassFlags()) || !m_mapmgr->InLineOfSight(m_positionX, m_positionY, newx, newy, PASS_FLAG_TELEPORT))
				{
					newx = m_positionX + RandomUInt(250) - 125;
					newy = m_positionY + RandomUInt(250) - 125;

					++tries;
					if (tries >= 100)
						break;
				}

				if (tries < 100)
				{
					//HACK
					float oldrot = m_rotation;
					SetPosition(newx, newy);
					CastSpell(191, newx, newy, true, DAMAGE_FLAG_CANNOT_PROC);
					StopMovement();
					StopAttacking();
					m_rotation = oldrot;
				}
			}

			if (!(damageflags & DAMAGE_FLAG_CANNOT_PROC) && other->GetSkillValue(SKILL_FREEZE_SOLID) > 0 && (m_lastfreeze + 2000 <= g_mstime) && (RandomUInt(99) < other->GetSkillValue(SKILL_FREEZE_SOLID)))
			{
				//float cast_speed_mod = float(100 - m_bonuses[CAST_SPEED] / 100);
				//float attack_mod = GetAttackSpeed() / GetAttackSpeedBase();

				float freeze_mod = 1 * 1000;

				m_nextspell = g_mstime + freeze_mod;
				StopAttack(freeze_mod);
				m_laststrikeblock = false;
				StopMovement(freeze_mod);
				m_lastfreeze = g_mstime;
				SetSpellEffect(18);
				SetSpellFlag(AURA_FLAG_FREEZE_SOLID);
				AddEvent(&Unit::RemoveSpellFlag, (uint16)AURA_FLAG_FREEZE_SOLID, EVENT_UNKNOWN, freeze_mod, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
		}
	}

	DoDealDamage(other, damage);

	if (IsCreature())
	{
		if (TO_CREATURE(this)->m_script != NULL)
			TO_CREATURE(this)->m_script->OnDealDamage(damage, spell);
	}

	if (other->IsCreature())
	{
		if (TO_CREATURE(other)->m_script != NULL && !(damageflags & DAMAGE_FLAG_NO_REFLECT))
			TO_CREATURE(other)->m_script->OnTakeDamage(this, damage, spell);
	}

	if (other->IsCreature() && TO_CREATURE(other)->m_proto->boss)
		TO_CREATURE(other)->m_damagedunits.insert(m_serverId);
	else if (IsAlive() && (!oowner->IsCreature() || TO_CREATURE(oowner)->m_proto->boss == 0))
	{
		bool leech = false;
		if (m_bonuses[ON_HIT_LEECH_CHANCE] && RandomUInt(99) < m_bonuses[ON_HIT_LEECH_CHANCE])
		{
			leech = true;
		}

		//now we steal a percent ;)
		float steal = leech ? 1 : float(m_bonuses[ON_DAMAGE_HEALTH_STEAL]) / 100;
		steal *= damage;
		ModHealth(steal);

		steal = leech ? 1 : float(m_bonuses[ON_DAMAGE_MANA_STEAL]) / 100;
		steal *= damage;
		ModMana(steal);

		if (HasAura(SPELL_AURA_LEECH))
		{
			steal = float(GetSkillValue(SKILL_LEECH) * 5) / 100;
			steal *= damage;
			ModHealth(steal);
		}
	}

	if (!other->IsAlive())
	{
		//award exp
		if (other->IsCreature() && !other->IsPet())
		{
			Creature* creother = TO_CREATURE(other);
			if (creother->m_proto->boss == 1)
			{
				//revive all players if possible
				for (std::map<Unit*, uint8>::iterator itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
				{
					if (!itr->first->IsPlayer())
						continue;
					if (creother->m_proto->boss == 1)
						itr->first->CastSpell(8, itr->first, true);
				}
			}
			else if (powner != NULL)
			{
				uint32 basexp = TO_CREATURE(other)->m_proto->exp;
				if (TO_CREATURE(other)->m_proto->entry >= 10000 && TO_CREATURE(other)->m_proto->boss == 0)
					basexp = TO_CREATURE(other)->m_proto->exp * other->m_level;
				uint32 exp = basexp * g_ServerSettings.GetFloat("mod_exp");
				powner->GiveExp(exp);
			}
		}

		if (powner != NULL)
		{
			uint32 serverType = m_mapmgr->m_serverType;
			bool alignpos = true;
			if ((powner->m_faction == oowner->m_faction && (serverType == 0 || oowner == NULL)) || oowner->m_faction == FACTION_NEUTRAL)
			{
				if (!oowner->IsPlayer() && !oowner->IsPet())
					alignpos = false;
				else if (!oowner->IsPet())
				{
					if (TO_PLAYER(oowner)->m_alignment >= 0 && !TO_PLAYER(oowner)->m_betraypvpcombat)
						alignpos = false;
				}
			}

			if (alignpos && powner->m_alignlock <= g_time)
				powner->ModAlign(1);
			else
			{
				if (other->IsCreature() && TO_CREATURE(other)->m_proto->boss == 2)
				{
					powner->m_alignlock = g_time + (60 * 20);
					powner->ModAlign(-2000);
				}

				if (other->IsPlayer() && !TO_PLAYER(other)->InPVPMode())
				{
					powner->m_alignlock = g_time + (60 * 20);
					powner->ModAlign(-2000);
				}
			}

			if (other->IsPlayer())
				powner->GivePVPExp(TO_PLAYER(other));
		}
	}
	else
	{
		if (other->IsCreature() && !HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
		{
			Creature* creother = TO_CREATURE(other);
			if (!creother->IsAttacking() && GetDistance(creother) <= SIGHT_RANGE * 3)
				creother->Attack(this);
		}

		if (other->HasAura(SPELL_AURA_CORRUPT_ARMOR))
			other->CastSpell(15038, this, true);
		if (other->HasAura(SPELL_AURA_DULL_SENSES))
			other->CastSpell(15039, this, true);
		if (other->HasAura(SPELL_AURA_DISINTEGRATION))
			other->CastSpell(15040, this, true);
	}

	if (powner != NULL)
	{
		if (oowner->IsPlayer())
		{
			powner->SetPVPCombat();
 			if (powner->m_faction == oowner->m_faction && TO_PLAYER(oowner)->m_alignment >= 0 && !TO_PLAYER(oowner)->m_betraypvpcombat)
 				powner->SetBetrayPVPCombat();
		}
	}
}

void Unit::RemoveAllAuras()
{
	for (uint32 i=0; i<NUM_SPELL_AURAS; ++i)
		if (m_auras[i] != NULL)
			m_auras[i]->Remove();
}

void Unit::SetAnimation( uint8 anim, uint8 anim2, bool isexplicit /*= false*/ )
{
	if (HasUnitFlag(UNIT_FLAG_EXPLICIT_ANIMATIONS) && !isexplicit)
		return;
	if (m_unitAnimation == anim && anim2 != 0)
		m_unitAnimation = anim2;
	else
		m_unitAnimation = anim;
	SetUpdateBlock(UPDATETYPE_ANIMATION);
}

void Unit::StopMovement( int32 howlong /*= 0*/ )
{
	m_pathdata.clear();
	m_pathx = -1;
	m_pathy = -1;
	m_endmove = 0;
	m_destinationX = 0;
	m_destinationY = 0;
	if (howlong != 0)
	{
		uint32 newval = g_mstime + howlong;

		if (newval >= m_nextmove)
			m_nextmove = newval;
	}
	if (IsPlayer())
		m_POIObject = 0;
}

void Unit::SetSpellEffect( uint8 effect )
{
	if (IsPlayer() && TO_PLAYER(this)->m_invis)
		return;
	if (effect >= 100)
		return;
	if (m_spelleffect == effect)
	{
		if (m_spelleffect % 2)
			++m_spelleffect;
		else
			--m_spelleffect;
	}
	else
		m_spelleffect = effect;
	SetUpdateBlock(UPDATETYPE_EFFECTS);
}

uint32 Unit::FindFreeInventorySlot( int32 itemid /*= -1*/, int32 reqfreestackspace /*= -1*/ )
{
	for (uint32 i = NUM_EQUIPPED_ITEMS; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS; ++i)
		if (m_items[i] == NULL || (itemid != -1 && m_items[i]->m_data->id == itemid && m_items[i]->m_stackcount < uint32(255 - reqfreestackspace)))
			return i;
	return 0xFFFFFFFF;
}

uint32 Unit::GetAttackSpeed()
{
	uint32 basespeed = GetAttackSpeedBase();
	if (m_auras[SPELL_AURA_VELOCITAS] != NULL)
		basespeed /= float(100 + m_auras[SPELL_AURA_VELOCITAS]->m_spell->value) / 100;
	basespeed /= float(100 + m_bonuses[ATTACK_SPEED]) / 100;
	if (HasAura(SPELL_AURA_RAGE_POTION))
		basespeed /= 2;
	return basespeed;
}

void Unit::Revive()
{
	RemoveEvents(EVENT_PLAYER_RESPAWN);
	m_deathstate = STATE_ALIVE;
	SetHealth(m_maxhealth);
	SetMana(m_maxmana);
	m_unitAnimation = 0;
	SetUpdateBlock(UPDATETYPE_ANIMATION);

	if (IsPlayer())
		TO_PLAYER(this)->m_canResurrect = false;
}

void Unit::GetAnimationType( uint32 type, uint32 & buf1, uint32 & buf2 )
{
	uint32 itemtype = ITEM_TYPE_1H_WEAPON; //default
	Item* curweapon = GetCurrentWeapon();
	if (curweapon != NULL)
		itemtype = curweapon->m_data->type;

	if (type == ANIM_TYPE_MOVING && IsAttacking())
		type = ANIM_TYPE_ATTACK_MOVING;

	switch (itemtype)
	{
	default:
		{
			switch (type)
			{
			case ANIM_TYPE_STANDING: buf1 = ANIMATION_NONE1H; buf2 = 0; break;
			case ANIM_TYPE_MOVING: buf1 = ANIMATION_MOVING; buf2 = 0; break;
			case ANIM_TYPE_ATTACK_MOVING: buf1 = ANIMATION_ATTACKMOVE1H; buf2 = 0; break;
			case ANIM_TYPE_ATTACK_MISS:
			case ANIM_TYPE_ATTACK_SWING:
				{
					if (curweapon != NULL && curweapon->m_data->type2 == WEAPON_TYPE_SHORT_SWORD)
					{
						buf1 = ANIMATION_STAB;
						buf2 = ANIMATION_STAB_2;
					}
					else
					{
						if (GetAttackSpeed() < 600)
						{
							buf1 = ANIMATION_STRIKE1HFAST;
							buf2 = ANIMATION_STRIKE1HFAST_2;
						}
						else
						{
							buf1 = ANIMATION_STRIKE1H;
							buf2 = ANIMATION_STRIKE1H_2;
						}
					}
				}break;
			case ANIM_TYPE_ATTACK_BLOCK: buf1 = ANIMATION_STRIKE1H_BLOCK; buf2 = ANIMATION_STRIKE1H_BLOCK_2; break;
			}
		} break;
	case ITEM_TYPE_BOW:
			{
				switch (type)
				{
				case ANIM_TYPE_STANDING: buf1 = ANIMATION_NONE1H; buf2 = 0; break;
				case ANIM_TYPE_MOVING: buf1 = ANIMATION_MOVING; buf2 = 0; break;
				case ANIM_TYPE_ATTACK_MOVING: buf1 = ANIMATION_ATTACKMOVE1H; buf2 = 0; break;
				case ANIM_TYPE_ATTACK_MISS:
				case ANIM_TYPE_ATTACK_BLOCK:
				case ANIM_TYPE_ATTACK_SWING:
					{
						buf1 = ANIMATION_STRIKEBOW;
						buf2 = ANIMATION_STRIKEBOW_2;
					} break;
				}
			} break;
	case ITEM_TYPE_POLE_ARM:
		{
			switch (type)
			{
			case ANIM_TYPE_STANDING: buf1 = ANIMATION_NONEPOLEARM; buf2 = 0; break;
			case ANIM_TYPE_MOVING: buf1 = ANIMATION_MOVING; buf2 = 0; break;
			case ANIM_TYPE_ATTACK_MOVING: buf1 = ANIMATION_ATTACKMOVEPOLEARM; buf2 = 0; break;
			case ANIM_TYPE_ATTACK_MISS:
			case ANIM_TYPE_ATTACK_BLOCK:
			case ANIM_TYPE_ATTACK_SWING:
				{
					if (IsPlayer() && TO_PLAYER(this)->m_class == CLASS_BARBARIAN)
					{
						if (GetAttackSpeed() < 600)
						{
							buf1 = ANIMATION_STRIKE1HFAST;
							buf2 = ANIMATION_STRIKE1HFAST_2;
						}
						else
						{
							buf1 = ANIMATION_STRIKE1H;
							buf2 = ANIMATION_STRIKE1H_2;
						}
					}
					else { buf1 = ANIMATION_STRIKEPOLEARM; buf2 = ANIMATION_STRIKEPOLEARM_2; }
				}
				break;
			}
		} break;
	case ITEM_TYPE_2H_WEAPON:
		{
			switch (type)
			{
			case ANIM_TYPE_STANDING: buf1 = ANIMATION_NONE1H; buf2 = 0; break;
			case ANIM_TYPE_MOVING: buf1 = ANIMATION_MOVING; buf2 = 0; break;
			case ANIM_TYPE_ATTACK_MOVING: buf1 = ANIMATION_ATTACKMOVE1H; buf2 = 0; break;
			case ANIM_TYPE_ATTACK_MISS:
			case ANIM_TYPE_ATTACK_BLOCK:
			case ANIM_TYPE_ATTACK_SWING: buf1 = ANIMATION_STRIKE2H_FEMALE; buf2 = ANIMATION_STRIKE2H_FEMALE_2; break;
			}
		} break;
	case ITEM_TYPE_BLADES:
		{
			switch (type)
			{
			case ANIM_TYPE_STANDING: buf1 = ANIMATION_NONE1H; buf2 = 0; break;
			case ANIM_TYPE_MOVING: buf1 = ANIMATION_MOVING; buf2 = 0; break;
			case ANIM_TYPE_ATTACK_MOVING: buf1 = ANIMATION_ATTACKMOVE1H; buf2 = 0; break;
			case ANIM_TYPE_ATTACK_MISS:
			case ANIM_TYPE_ATTACK_BLOCK:
			case ANIM_TYPE_ATTACK_SWING: buf1 = ANIMATION_STRIKEBLADES; buf2 = ANIMATION_STRIKEBLADES_2; break;
			}
		} break;
	}

	//special one
	if ((type == ANIM_TYPE_MOVING || type == ANIM_TYPE_ATTACK_MOVING) && m_attackRun)
	{
		buf1 = ANIMATION_ATTACKRUN;
		buf2 = 0;
	}

	if (type == ANIM_TYPE_HIT)
	{
		buf1 = ANIMATION_HIT;
		buf2 = ANIMATION_HIT_2;
	}
}

uint32 Unit::GetItemClass()
{
	return ITEM_CLASS_ALL;
}

void Unit::Regen()
{
	m_regenTimer = GetMSTime() + 3000;
	if (!HasAura(SPELL_AURA_DISINTEGRATION_EFFECT) && !HasAura(SPELL_AURA_POISON))
	{
		uint32 regenhealth = float(m_maxhealth) * float(m_bonuses[HEALTH_REGENERATION]) / 100;
		if (regenhealth < 1)
			regenhealth = 1;
		ModHealth(regenhealth);
	}

	if (IsPlayer())
	{
		uint32 regenmana = float(m_maxmana) * float(m_bonuses[MANA_REGENERATION]) / 100;
		if (regenmana < m_bonuses[MANA_REGENERATION] * 5)
			regenmana = m_bonuses[MANA_REGENERATION] * 5;
		if (regenmana < 1)
			regenmana = 1;

		ModMana(regenmana);

		if (m_mapmgr != NULL && (m_mapmgr->GetMapLayer1(m_positionX, m_positionY) == 72 || m_mapmgr->GetMapLayer2(m_positionX, m_positionY) == 82)) //poison (1: 72) and lava (2: 82)
		{
			int32 tenpct = m_maxhealth * 0.1;
			ModHealth(-tenpct);
			SetAnimationType(ANIM_TYPE_HIT);
		}

		if (m_mapmgr != NULL && m_mapmgr->GetMapLayer1(m_positionX, m_positionY) == 71)
		{
			SetMana(0);
		}
	}
}

uint32 Unit::CountEquippedItem( uint32 itemid )
{
	uint32 retval = 0;
	for (uint32 i = 0; i < NUM_EQUIPPED_ITEMS; ++i)
		if (m_items[i] != NULL && m_items[i]->m_data->id == itemid)
			++retval;
	return retval;
}

Unit::~Unit()
{
	RemoveAllAuras();
	DestroyInventory();
}

Item* Unit::FindInventoryItem( uint32 itemid )
{
	for (uint32 i = NUM_EQUIPPED_ITEMS; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS; ++i)
		if (m_items[i] != NULL && m_items[i]->m_data->id == itemid)
			return m_items[i];
	return NULL;
}

bool Unit::ProtectedByGround( bool canbeprotected /*= false*/ )
{
	if (IsPlayer() && TO_PLAYER(this)->m_alignment < 0 && !canbeprotected)
		return false;

	if (m_mapmgr->GetMapLayer2(m_positionX, m_positionY) != 46) //safety square
		return false;

	switch (m_mapmgr->GetMapLayer3(m_positionX, m_positionY))
	{
	case 46:
		return (m_faction == FACTION_GOOD);
	case 53:
	case 62:
		return (m_faction == FACTION_GOOD || m_faction == FACTION_EVIL);
	case 57:
		return (m_faction == FACTION_EVIL);
	}
	return false;
}

float Unit::GetMagicBonus( uint32 bonustype )
{
	int32 bonus = m_bonuses[bonustype] + m_bonuses[ALL_MAGIC_BONUS];

	switch (bonustype)
	{
	case FIRE_MAGIC_BONUS:
		{
			bonus += GetSkillValue(SKILL_FIRE_DEVOTION) * 2;
			bonus += GetSkillValue(SKILL_BURN) * 2;
		}break;
	case ICE_MAGIC_BONUS: bonus += GetSkillValue(SKILL_ICE_DEVOTION) * 2; break;
	case ENERGY_MAGIC_BONUS: bonus += GetSkillValue(SKILL_ENERGY_DEVOTION) * 2; break;
	case SHADOW_MAGIC_BONUS: bonus += GetSkillValue(SKILL_SHADOW_DEVOTION) * 2; break;
	case SPIRIT_MAGIC_BONUS: bonus += GetSkillValue(SKILL_SPIRIT_DEVOTION) * 2; break;
	}

	if (IsPlayer())
	{
		Player* pthis = TO_PLAYER(this);

		switch (pthis->m_class)
		{
		case CLASS_CLERIC:
		case CLASS_DIABOLIST:
		case CLASS_DRUID:
		case CLASS_WIZARD:
		case CLASS_WARLOCK:
		case CLASS_WITCH:
		case CLASS_DARKWAR:
		case CLASS_ADEPT:
			{
				int32 dmgstat = max(pthis->m_intellect, pthis->m_wisdom);

				if (dmgstat < pthis->m_constitution)
					bonus -= pthis->m_constitution - dmgstat;
			} break;
		}

		if (pthis->m_class == CLASS_WIZARD)
			bonus += 10;
	}
	return (float(100 + bonus) / 100);
}

void Unit::RemoveUpdateBlock( uint8 block )
{
	for (std::map<Unit*, uint8>::iterator itr=m_inrangeUnits.begin(); itr!=m_inrangeUnits.end(); ++itr)
	{
		std::map<Unit*, uint8>::iterator itr2 = itr->first->m_inrangeUnits.find(this);
		if (itr2 != itr->first->m_inrangeUnits.end())
			itr2->second &= ~block;
	}
}

void Unit::Attack( Object* obj )
{
	if (m_POIObject == obj->m_serverId && m_POI == POI_ATTACKING)
		return;
	Item* curweapon = GetCurrentWeapon();

	if (curweapon != NULL && curweapon->m_data->type3 == ITEM_TYPE_PROJECTILE)
	{
		StopMovement();
		m_destinationRadius = 99999.9f;
		SetPOI(POI_ATTACKING, obj->m_serverId);
		m_attackRun = false;
	}
	else if (m_bonuses[BONUS_RADIUS_WAND_BEAM] > 0 || m_bonuses[BONUS_LUMEN_WAND_BEAM] > 0)
	{
		StopMovement();
		m_destinationRadius = 99999.9f;
		SetPOI(POI_ATTACKING, obj->m_serverId);
	}
	else
	{
		m_destinationRadius = obj->m_bounding;
		SetPOI(POI_ATTACKING, obj->m_serverId);
		//m_attackRun = false;
	}
}

uint32 Unit::GetLevel()
{
	return m_level;
}

void Unit::EventChatMsg( uint32 delay, const char* format, ... )
{
	va_list ap;
	va_start(ap, format);

	char buffer[2048];

	vsprintf(buffer, format, ap);

	if (g_mstime < m_nextchatEvent + 3000)
	{
		//give time for last message
		if (delay == 0)
			delay += 3000;
		delay += m_nextchatEvent - GetMSTime();
	}

	std::string msg = buffer;
	AddEvent(&Unit::EventSendChatMsg, msg, EVENT_UNIT_CHATSAY, delay, 1, EVENT_FLAG_NOT_IN_WORLD);
	m_nextchatEvent = GetMSTime() + delay;

	va_end(ap);
}

void Unit::DestroyInventory()
{
	for (int32 i = NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS - 1; i >= 0; --i)
	{
		if (m_items[i] != NULL)
			m_items[i]->Delete();
	}
}

void Unit::OnRemoveFromWorld()
{

}

void Unit::OnPreRemoveFromWorld()
{
	SetThrownWeapon(0); //changing map = never get weapon back
	for (std::set<uint16>::iterator itr = m_pets.begin(); itr != m_pets.end(); ++itr)
	{
		Object* o = m_mapmgr->GetObject(*itr);
		if (o == NULL || !o->IsUnit())
			continue;
		o->AddEvent(&Unit::SetHealth, (uint32)0, EVENT_UNIT_SETHEALTH, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
	m_pets.clear();

	if (!IsPlayer() || TO_PLAYER(this)->m_pendingmaptransfer.newmapid == 0xFFFFFFFF)
		RemoveAllAuras();
	RemoveAreaAuras();
	Object::OnPreRemoveFromWorld();
}

uint32 Unit::GetSummonedObjectCount( uint32 visual )
{
	if (m_mapmgr == NULL)
		return 0;
	uint32 count = 0;
	for (std::set<uint16>::iterator itr = m_summonedObjects.begin(); itr != m_summonedObjects.end(); ++itr)
	{
		Object* o = m_mapmgr->GetObject(*itr);
		if (o == NULL || !o->IsSpellEffect() || TO_SPELLEFFECT(o)->m_spell == NULL)
			continue;
		if (TO_SPELLEFFECT(o)->m_spell->display == visual || TO_SPELLEFFECT(o)->m_visualId == visual)
			++count;
	}
	return count;
}

void Unit::GetSummonedObject( uint32 visual, std::vector<Object*>* out)
{
	if (m_mapmgr == NULL || out == NULL)
		return;
	for (std::set<uint16>::iterator itr = m_summonedObjects.begin(); itr != m_summonedObjects.end(); ++itr)
	{
		Object* o = m_mapmgr->GetObject(*itr);
		if (o == NULL || !o->IsSpellEffect() || TO_SPELLEFFECT(o)->m_spell == NULL)
			continue;
		if (TO_SPELLEFFECT(o)->m_spell->display == visual || TO_SPELLEFFECT(o)->m_visualId == visual)
			out->push_back(o);
	}
}

void Unit::SetUnitSpecialName( uint8 val, uint32 duration )
{
	m_unitspecialname = val;
	RemoveEvents(EVENT_UNIT_REMOVESPECIALNAME);
	if (duration != 0)
		AddEvent(&Unit::SetUnitSpecialName, (uint8)0, (uint32)0, EVENT_UNIT_REMOVESPECIALNAME, duration, 1, EVENT_FLAG_NOT_IN_WORLD);
}

void Unit::SetUnitSpecialName2( uint8 val, uint32 duration )
{
	m_unitspecialname2 = val;
	RemoveEvents(EVENT_UNIT_REMOVESPECIALNAME2);
	if (duration != 0)
		AddEvent(&Unit::SetUnitSpecialName2, (uint8)0, (uint32)0, EVENT_UNIT_REMOVESPECIALNAME2, duration, 1, EVENT_FLAG_NOT_IN_WORLD);
}

uint8 Unit::GetClassTitle()
{
	if (!IsPlayer())
		return UNIT_NAME_SPECIAL_KNIGHT;
	switch (TO_PLAYER(this)->m_class)
	{
	case CLASS_FIGHTER:
	case CLASS_RANGER:
	case CLASS_PALADIN:
	case CLASS_BARBARIAN:
	case CLASS_DARK_PALADIN:
	case CLASS_DARKWAR:
		return UNIT_NAME_SPECIAL_KNIGHT;
	}

	return UNIT_NAME_SPECIAL_ADEPT;
}

void Unit::StopAttacking()
{
	if (IsCreature())
	{
		Unit* newtarget = HasUnitFlag(UNIT_FLAG_UNATTACKABLE)? NULL : TO_CREATURE(this)->FindTarget();
		if (newtarget != NULL)
			Attack(newtarget);
		else
			ClearPOI();
	}
	else
		ClearPOI();
}

void Unit::RemoveAllDispellableAuras()
{
	for (uint32 i=0; i<NUM_SPELL_AURAS; ++i)
		if (m_auras[i] != NULL && m_auras[i]->m_data->dispellable && !m_auras[i]->m_dispelblock)
			m_auras[i]->Remove();
}

Item* Unit::FindEquippedItem( uint32 itemid )
{
	for (uint32 i = 0; i < NUM_EQUIPPED_ITEMS; ++i)
		if (m_items[i] != NULL && m_items[i]->m_data->id == itemid)
			return m_items[i];
	return NULL;
}

uint32 Unit::GetSkillValue( uint32 skillid )
{
	if (!IsPlayer())
		return 0;
	return TO_PLAYER(this)->m_skills[skillid];
}

float Unit::GetMagicResist( uint32 resistbase, bool includextruder /*= false*/ )
{
	int32 bonus = m_bonuses[resistbase] + m_bonuses[PROTECTION_ALL_MAGIC];

	bonus += GetSkillValue(SKILL_RESIST_MASTERY);
	bonus += GetSkillValue(SKILL_RESIST_MAGIC);

	switch (resistbase)
	{
	case PROTECTION_FIRE:
		{
			bonus += GetSkillValue(SKILL_FIRE_DEVOTION) * 2;
			bonus += GetSkillValue(SKILL_RESIST_FIRE);
			bonus -= GetSkillValue(SKILL_ICE_DEVOTION);
			bonus -= GetSkillValue(SKILL_SPIRIT_DEVOTION);
		} break;
	case PROTECTION_ICE:
		{
			bonus += GetSkillValue(SKILL_ICE_DEVOTION) * 2;
			bonus += GetSkillValue(SKILL_RESIST_ICE);
			bonus -= GetSkillValue(SKILL_FIRE_DEVOTION);
			bonus -= GetSkillValue(SKILL_ENERGY_DEVOTION);
		} break;
	case PROTECTION_ENERGY:
		{
			bonus += GetSkillValue(SKILL_ENERGY_DEVOTION) * 2;
			bonus += GetSkillValue(SKILL_RESIST_ENERGY);
			bonus -= GetSkillValue(SKILL_SHADOW_DEVOTION);
		} break;
	case PROTECTION_SHADOW:
		{
			bonus += GetSkillValue(SKILL_SHADOW_DEVOTION) * 2;
			bonus += GetSkillValue(SKILL_RESIST_SHADOW);
			bonus -= GetSkillValue(SKILL_ICE_DEVOTION);
			bonus -= GetSkillValue(SKILL_ENERGY_DEVOTION);
			bonus -= GetSkillValue(SKILL_SPIRIT_DEVOTION);
		} break;
	case PROTECTION_SPIRIT:
		{
			bonus += GetSkillValue(SKILL_SPIRIT_DEVOTION) * 2;
			bonus += GetSkillValue(SKILL_RESIST_SPIRIT);
			bonus -= GetSkillValue(SKILL_FIRE_DEVOTION);
			bonus -= GetSkillValue(SKILL_SHADOW_DEVOTION);
		}
	}

	if (HasAura(SPELL_AURA_CORRUPT_ARMOR_EFFECT))
		bonus -= 12;

	if (includextruder)
	{
		bonus += m_bonuses[PROTECTION_EXTRUDERE];
	}

	return (float(100 + bonus) / 100);
}

void Unit::EventDealDamage( uint32 serverid, int32 damage )
{
	if (m_mapmgr == NULL)
		return;
	Object* obj = m_mapmgr->GetObject(serverid);
	if (obj == NULL || !obj->IsUnit())
		return;
	DealDamage(TO_UNIT(obj), damage);
}

Item* Unit::GetCurrentWeapon()
{
	if (IsPlayer())
		return m_items[ITEM_SLOT_WEAPON1 + TO_PLAYER(this)->m_weaponSwitch];
	return m_items[ITEM_SLOT_WEAPON1];
}

bool Unit::DidHit( Unit* u )
{
	//behind: 100% hit
	if (!u->IsInFront(this))
		return true;

	auto otherweap = u->GetCurrentWeapon();

	//Mage staffs and bows
	if (otherweap != NULL && (otherweap->m_data->type2 == WEAPON_TYPE_BOW || (otherweap->m_data->type == ITEM_TYPE_POLE_ARM && otherweap->m_data->propflags & ITEM_CLASS_WIZARD)))
		return true;

	int32 hitchance = 100;
	if (u->IsPlayer())
	{
		hitchance -= TO_PLAYER(u)->m_agility / 2;
		if (IsPlayer())
			hitchance += TO_PLAYER(this)->m_agility / 2;
	}

	if (!IsCreature())
	{
		hitchance -= u->GetLevel() / 5;
		hitchance += GetLevel() / 5;
	}

	hitchance -= u->m_bonuses[DEFEND_BONUS];
	hitchance += m_bonuses[HIT_BONUS];
	if (HasAura(SPELL_AURA_STUPIFY))
		hitchance -= m_auras[SPELL_AURA_STUPIFY]->m_spell->value;

	if (HasAura(SPELL_AURA_FIERCE))
		hitchance += 10;
	if (u->HasAura(SPELL_AURA_FIERCE))
		hitchance -= 10;
	if (HasAura(SPELL_AURA_WHITE_POT))
		hitchance += 5;
	if (u->HasAura(SPELL_AURA_WHITE_POT))
		hitchance -= 5;

	hitchance += GetSkillValue(SKILL_WEAPON_TACTICS);
	hitchance += GetSkillValue(SKILL_WEAPON_MASTERY);

	hitchance -= u->GetSkillValue(SKILL_DEFENSE_TACTICS);
	hitchance -= u->GetSkillValue(SKILL_DEFENSE_MASTERY);


	//handle skills whoo
	if (HasAura(SPELL_AURA_INTENSITY))
		hitchance += 20;
	if (u->HasAura(SPELL_AURA_INTENSITY))
		hitchance -= 20;


	if (GetCurrentWeapon() != NULL)
	{
		switch (GetCurrentWeapon()->m_data->type2)
		{
		case WEAPON_TYPE_BOW: hitchance += GetSkillValue(SKILL_BOWS) + GetSkillValue(SKILL_ARCHERY) + GetSkillValue(SKILL_ARCHERY_MASTERY); break;
		case WEAPON_TYPE_DAGGER: hitchance += GetSkillValue(SKILL_DAGGERS); break;
		case WEAPON_TYPE_POLE_ARM: hitchance += GetSkillValue(SKILL_POLE_ARM); break;
		case WEAPON_TYPE_SWORD: hitchance += GetSkillValue(SKILL_SWORDS); break;
		case WEAPON_TYPE_BLUNT: hitchance += GetSkillValue(SKILL_BLUNT_WEAPONS); break;
		case WEAPON_TYPE_AXE: hitchance += GetSkillValue(SKILL_AXES); break;
		case WEAPON_TYPE_BLUNT_2H:
		case WEAPON_TYPE_AXE_2H:
		case WEAPON_TYPE_SWORD_2H: hitchance += GetSkillValue(SKILL_2_HANDED_WEAPONS); break;
		}
	}

	//shield skills
	if (u->m_items[ITEM_SLOT_SHIELD] != NULL)
		hitchance -= u->GetSkillValue(SKILL_SHIELD) + u->GetSkillValue(SKILL_SHIELD_MASTERY);

	if (hitchance < 25)
		hitchance = 25;

	//handle dodge and sure hit
	if (HasAura(SPELL_AURA_SURE_HIT) && !u->HasAura(SPELL_AURA_DODGE))
	{
		m_auras[SPELL_AURA_SURE_HIT]->RemoveCharge();
		hitchance = 100;
	}
	if (!HasAura(SPELL_AURA_SURE_HIT) && u->HasAura(SPELL_AURA_DODGE))
	{
		u->m_auras[SPELL_AURA_DODGE]->RemoveCharge();
		hitchance = 0;
	}
	if (hitchance >= 100)
		return true;
	if ((int32)RandomUInt(99) < hitchance)
		return true;
	return false;
}

void Unit::SetNextSpell( uint32 spellid, TargetData & targets, bool triggered )
{
	m_nextspellid = spellid;
	m_nextspelltargets = targets;
	m_nextspelltrigger = false;
}

void Unit::SetThrownWeapon( uint32 id )
{
	m_thrownweapon = id;

	Item* currentweapon = GetCurrentWeapon();
	if (currentweapon != NULL && currentweapon->m_data->id == id)
	{
		//make this weapon disappear :)
		SetModels(-1, 0);
	}

	if (currentweapon != NULL && id == 0)
	{
		currentweapon->ApplyItemVisual();
		RemoveEvents(EVENT_UNIT_FORCE_RETURN_WEAPON);
	}
}

void Unit::RemoveAreaAuras()
{
	for (uint32 i = 0; i < NUM_SPELL_AURAS; ++i)
	{
		if (HasAura(i) && m_auras[i]->m_areaaura != 0)
			m_auras[i]->Remove();
	}
}

void Unit::UpdatePetFollowAngles()
{
	int32 i = 0;
	float inc = (PI * 2) / m_pets.size();
	for (std::set<uint16>::iterator itr = m_pets.begin(); itr != m_pets.end(); ++itr)
	{
		Object* obj = m_mapmgr->GetObject(*itr);

		if (obj == NULL || !obj->IsPet())
			continue;

		TO_UNIT(obj)->m_followOwnerAngle = inc * i;
		++i;
	}
}

void Unit::SendChatMsg( const char* message, bool localonly /*= false*/ )
{
	ProcTileScript(TILE_PROC_FLAG_CHAT);
	m_lastSay = message;
	if (!localonly)
		SetUpdateBlock(UPDATETYPE_CHATMSG);
	else
		RemoveUpdateBlock(UPDATETYPE_CHATMSG);
	if (IsPlayer())
	{
		m_pendingSay = true;
		if (!TO_PLAYER(this)->m_nochatupdate)
			TO_PLAYER(this)->BuildUpdatePacket();
	}
}

uint32 Unit::GetNumFreeInventorySlots()
{
	uint32 freeslots = 0;
	for (uint32 i = NUM_EQUIPPED_ITEMS; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS; ++i)
	{
		if (m_items[i] == NULL)
			freeslots++;
	}
	return freeslots;
}

uint32 Unit::FindFreeStorageSlot( int32 itemid /*= -1*/, int32 reqfreestackspace /*= -1*/ )
{
	for (uint32 i = NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS + NUM_STORAGE_ITEMS; ++i)
		if (m_items[i] == NULL || (itemid != -1 && m_items[i]->m_data->id == itemid && m_items[i]->m_stackcount < uint32(255 - reqfreestackspace)))
			return i;
	return 0xFFFFFFFF;
}

uint32 Unit::GetPassFlags()
{
	uint32 res = PASS_FLAG_WALK;
	if (IsPlayer())
		res |= PASS_FLAG_SWIM;
	if (HasAura(SPELL_AURA_MUTATIO_NIMBUS) || HasAura(SPELL_AURA_MUTATIO_FLAMMA))
		res |= PASS_FLAG_GAS;
	return res;
}

void Unit::UpdateMovement( Object* followingObject )
{
	if (followingObject == NULL && m_POIObject != 0)
		followingObject = m_mapmgr->GetObject(m_POIObject);

	//if we have a agent status in the crowd, call it to get our position
	if (m_crowdAgentIndex != -1 && m_mapmgr != nullptr)
	{
		auto agent = m_mapmgr->m_crowd->getAgent(m_crowdAgentIndex);

		bool hasmoved = m_mapmgr->m_crowd->agentIsMoving(*agent);


		m_rotation = CalcFacingDeg(agent->position[0], agent->position[2]);

		m_positionX = agent->position[0];
		m_positionY = agent->position[2];
		SetUpdateBlock(UPDATETYPE_MOVEMENT);
		

		//Following object logic
		if (followingObject != nullptr)
		{
			float target[3] = { followingObject->m_positionX, 0, followingObject->m_positionY };
		}
		//else if ()
	}
	else
	{
		//Movement
		if (IsAlive())
		{
			float destradius = m_destinationRadius;

			float speed = m_speed;
			if (m_attackRun)
				speed *= 4.5f;

			//do we have a path?
		MOVESTART:

			bool finished = false;

			int32 tilex = m_positionX / 20;
			int32 tiley = m_positionY / 20;

			if (m_pathdata.size() && m_pathpoint < m_pathdata.size())
				destradius = 0;

			if (CanMove() && m_endmove > 0)
			{
				//are we in an invalid position? wtf :P
				float dx = m_destinationX - m_positionX;
				float dy = m_destinationY - m_positionY;
				float distance = sqrt((float)((dx * dx) + (dy * dy)));
				float angle = CalcFacingDeg(m_destinationX, m_destinationY);
				uint16 newx = 0, newy = 0;

				G3D::Vector2 splinestart(m_startX, m_startY);
				G3D::Vector2 splineend(m_destinationX, m_destinationY);
				G3D::Vector2 dir = (splinestart - splineend);
				float dirlen = dir.length();
				//G3D::Vector2 dirn = dir.direction();

				float timefrac = 0;

				if (g_mstime >= m_startmove)
					timefrac = float(g_mstime - m_startmove) / float(m_endmove - m_startmove);
				G3D::Vector2 newpos;
				if (timefrac * dirlen >= dirlen - (destradius * 0.8))
				{
					//we're done moving
					//make reverse vector to destradius
					G3D::Vector2 dirrev = splineend - splinestart;
					G3D::Vector2 dirrevn = dirrev.direction();

					if (GetDistance(m_destinationX, m_destinationY) >= destradius) //dont move outwards
						newpos = splineend - dirrevn * (destradius * 0.8);
					else
					{
						newpos.x = m_positionX;
						newpos.y = m_positionY;
					}
					finished = true;
				}
				else
				{
					newpos = splinestart - dir * timefrac;
				}
				newx = newpos.x;
				newy = newpos.y;

				//weve finished moving
				if (finished)
				{
					if (m_pathpoint >= m_pathdata.size())
					{
						if (IsPet() && TO_CREATURE(this)->m_petownerid == m_POIObject)
							m_rotation = followingObject->m_rotation;
						else if (followingObject != NULL)
							m_rotation = CalcFacingDeg(followingObject->m_positionX, followingObject->m_positionY);
						else
							m_rotation = CalcFacingDeg(m_destinationX, m_destinationY);
						SetUpdateBlock(UPDATETYPE_MOVEMENT);
						SetPosition(newx, newy);
						m_endmove = 0;
						m_destinationX = 0;
						m_destinationY = 0;
						GroundItemPickup();
					}
					else
					{
						SetPosition(newx, newy);
						MoveToNextPathPoint();
						goto MOVESTART;
					}
				}
				else
				{
					m_rotation = angle;

					if (!HasAnimationType(ANIM_TYPE_MOVING))
						SetAnimationType(ANIM_TYPE_MOVING);
					SetUpdateBlock(UPDATETYPE_MOVEMENT);
				}

				if (!HasUnitFlag(UNIT_FLAG_NO_PATHFINDING))
					SetPosition(newx, newy);
				else
				{
					bool pass = true;
					if (!m_mapmgr->CanPass(newx / 20, newy / 20, GetPassFlags()))
						pass = false;
					if (!m_mapmgr->InLineOfSight(m_positionX, m_positionY, newx, newy, GetPassFlags()))
						pass = false;

					if (pass)
						SetPosition(newx, newy);
					else
						StopMovement();
				}
			}
			else
			{
				//m_attackRun = false;
				if (IsPet() && TO_CREATURE(this)->m_petownerid == m_POIObject)
					m_rotation = followingObject->m_rotation;
				else if (followingObject != NULL)
				{
					m_rotation = CalcFacingDeg(followingObject->m_positionX, followingObject->m_positionY);
					SetUpdateBlock(UPDATETYPE_MOVEMENT);
				}
			}
		}
	}
}

void Unit::SetCrowdTarget(float x, float y)
{
	float target[3] = { x, 0, y };
	float ext[3] = { 2, 4, 2 };
	dtQueryFilter filter;
	filter.setIncludeFlags(0xFF);

	float nearpt[3];
	dtPolyRef nearpoly;

	auto status = m_mapmgr->m_navQuery->findNearestPoly(target, ext, &filter, &nearpoly, nearpt);

	if (status != DT_SUCCESS)
	{
		//deal with pathfinding failure!
	}
	//else
		//m_mapmgr->m_crowd->requestMoveTarget(m_crowdAgentIndex, nearpoly, target);
}

void Unit::GroundItemPickup()
{
	uint32 tilex = m_positionX / 20;
	uint32 tiley = m_positionY / 20;
	//try and pick up items here if not attacking
	if ((!IsAttacking() && IsPlayer()) || (IsPet() && TO_CREATURE(this)->m_petstate == PET_STATE_STOP))
	{
		uint32 key = tilex | (tiley << 16);
		std::map<uint32, ItemGroup*>::iterator itr = m_cell->m_items.find(key);
		if (itr != m_cell->m_items.end())
		{
			//we pick up up to 5 items
			ItemGroup* igroup = itr->second;
			for (uint32 icount = 0; icount < 5; ++icount)
			{
				if (igroup->m_items.size() == 1)
				{
					Item* it = igroup->m_items[0];
					it->m_owner = this;
					it->AddToInventory();
					if (IsPet() && it->m_currentslot != 0xFFFFFFFF)
						it->Equip();
					if (IsPlayer())
						LOG_ACTION_I(it, "Picked up by %s", m_namestr.c_str());
					break; //igroup is now deleted
				}
				else
				{
					Item* it = igroup->m_items[0];
					it->m_owner = this;
					it->AddToInventory();
					if (IsPet() && it->m_currentslot != 0xFFFFFFFF)
						it->Equip();
					if (IsPlayer())
						LOG_ACTION_I(it, "Picked up by %s", m_namestr.c_str());
				}
			}
		}
	}
}

void Unit::DoDealDamage( Unit* other, uint32 damage )
{
	other->m_lastdamager = m_serverId;
	other->ModHealth(-damage);

	Packet data(SMSG_DAMAGE_INFO);
	data << damage;
	data << int32(other->m_positionX) << int32(other->m_positionY);

	if (IsPlayer())
		TO_PLAYER(this)->Send(&data);
	//send damage data to admins
	SendToAdmins(data);

}

void Unit::SendToAdmins( Packet & data )
{
	for (auto itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
	{
		Unit* u = itr->first;

		if (!u->IsPlayer())
			continue;
		if (TO_PLAYER(u)->m_account->m_data->accountlevel == 0)
			continue;

		TO_PLAYER(u)->Send(&data);
	}
}

void Unit::SendToAll( Packet & data )
{
	for (auto itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
	{
		Unit* u = itr->first;

		if (!u->IsPlayer())
			continue;
		TO_PLAYER(u)->Send(&data);
	}
}

void Unit::SendHealthUpdate(Player* to)
{
	uint8 shouldshow = 0;
	if (HealthBarShouldShow())
		shouldshow = 1;

	Packet data(SMSG_HEALTH_UPDATE);
	data << uint32(m_serverId) << m_health << m_maxhealth << shouldshow;

	uint32 reaction_wpos = data.WPos();

	if (to != NULL)
	{
		data << uint8(GetReaction(to));
		to->Send(&data);
	}
	else
	{
			for (auto itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
			{
				Unit* u = itr->first;

				if (!u->IsPlayer())
					continue;

				data.WPos(reaction_wpos);
				data << uint8(GetReaction(u));

				TO_PLAYER(u)->Send(&data);
			}
	}
}

bool Unit::HealthBarShouldShow()
{
	if (HasAura(SPELL_AURA_INVISIBILITY) || HasAura(SPELL_AURA_HIDE))
		return false;
	if (HasUnitFlag(UNIT_FLAG_INVISIBLE) || HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
		return false;
	return true;
}

void Unit::OnActive()
{
	Object::OnActive();

	if (m_mapmgr != nullptr)
	{
		float pos[3] = { m_positionX, 0, m_positionY };

		dtCrowdAgent ag;

		if (!m_mapmgr->m_crowd->addAgent(ag, pos))
			return;
		m_crowdAgentIndex = ag.id;
	}
}

void Unit::OnInactive()
{
	Object::OnInactive();
	if (m_mapmgr != nullptr)
	{
		if (m_crowdAgentIndex != -1)
		{
			m_mapmgr->m_crowd->removeAgent(m_crowdAgentIndex);
			m_crowdAgentIndex = -1;
		}
	}
}

inline void Unit::CreateAttackCrowdBehavior()
{
	if (m_crowdAgentIndex == -1)
		return;

	auto obj = m_mapmgr->GetObject(m_POIObject);

	if (obj == nullptr || !obj->IsUnit() || TO_UNIT(obj)->m_crowdAgentIndex == -1)
		return;


	auto agent = m_mapmgr->m_crowd->getAgent(m_crowdAgentIndex);

	if (agent == nullptr)
		return;

	auto b = dtSeekBehavior::allocate(1);
	auto params = b->getBehaviorParams(0);
	params->targetID = TO_UNIT(obj)->m_crowdAgentIndex;
	params->distance = obj->m_bounding;
	params->predictionFactor = 1.0f;

	m_mapmgr->m_crowd->pushAgentBehavior(m_crowdAgentIndex, b);
}

void Unit::DealDamageID( uint32 id, int32 damage, SpellEntry* spell /*= NULL*/, uint32 damageflags /*= 0*/ )
{
	Object* obj = m_mapmgr->GetObject(id);

	if (obj == NULL || !obj->IsUnit())
		return;

	DealDamage(TO_UNIT(obj), damage, spell, damageflags);
}
