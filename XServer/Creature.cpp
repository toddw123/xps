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

SQLStore<CreatureProto> sCreatureProto;
SQLStore<CreatureSpells> sCreatureSpells;
SQLStore<CreatureSpawn> sCreatureSpawn;
SQLStore<CreatureQuestGiver> sCreatureQuestGivers;
SQLStore<QuestEntry> sQuests;
SQLStore<QuestText> sQuestText;
SQLStore<QuestLoc> sQuestLocs;
SQLStore<CreatureEquipment> sCreatureEquipment;

Creature::Creature(CreatureProto* proto, CreatureSpawn* spawn)
{
	if (proto->unitflags & UNIT_FLAG_INHERIT_PROTO)
	{
		CreatureProto* nproto;
		if (spawn != NULL && spawn->inherited != -1)
			nproto = sCreatureProto.GetEntry(10000 + spawn->inherited);
		else
			nproto = sCreatureProto.GetEntry(10000 + proto->displayid);
		if (nproto != NULL)
		{
			m_namestr = proto->name;
			proto = nproto;
		}
	}

	m_incombat = false;
	m_level = 0;
	m_waypointType = WAYPOINT_TYPE_NORMAL;
	m_waypointDirection = 0;
	m_currentwaypoint = 0;
	m_petstate = PET_STATE_FOLLOW;
	m_petownerid = 0;
	m_ispet = false;
	m_norespawn = false;
	m_typeId = TYPEID_CREATURE;
	m_invis = false;
	m_spawnx = 0;
	m_spawny = 0;
	m_spawno = 0;

	m_proto = proto;
	m_spawn = spawn;
	m_randommovetimer = 0;

	m_charDisplay = proto->displayid;
	if (m_namestr.size() == 0)
		m_namestr = proto->name;
	m_faction = proto->faction;
	m_health = proto->health;
	m_maxhealth = proto->health;

	if (proto->color1 != 100)
		SetModels(-1, -1, -1, -1, proto->color1, proto->color2);
	else
		SetModels(-1, -1, -1, -1, RandomUInt(20), RandomUInt(20));

	if (spawn != NULL)
	{
		m_positionX = spawn->x;
		m_positionY = spawn->y;
		m_spawnx = spawn->x;
		m_spawny = spawn->y;
	}

	m_rotation = 90.0f;

	if (proto->scriptid != 0)
		m_script = sScriptMgr.CreateCreatureAIScript(this, proto->scriptid);
	else
		m_script = NULL;

	if (m_spawn != NULL && m_proto->boss != 1)
		SetLevel(m_spawn->level);
	else
		SetLevel(m_proto->level);

	//lookup spells
	InitCreatureSpells();

	m_quests = sCreatureQuestGivers.GetEntry(m_proto->entry);

	m_questofferid = 0;
	m_questplrid = 0;
	m_randmovex = 0;
	m_randmovey = 0;
}

void Creature::Update()
{
	Unit::Update();

	if (!IsAlive())
	{
		ClearPOI();
		return;
	}

	UpdatePOI();

	if ((m_updateCounter % 20) == 0 && IsPet())
	{
		Unit* owner = GetOwner();
		if (owner == NULL || !owner->IsAlive())
			SetHealth(0);
	}

	if (IsPet())
	{
		Unit* owner = GetOwner();
		if (owner == NULL)
		{
			if (m_summoned)
				AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
			else
			{
				m_ispet = false;
				m_petownerid = 0;
				m_petstate = PET_STATE_FOLLOW;
				m_summoned = false;
				SetPosition(m_spawnx, m_spawny);
				ClearPOI();
			}
		}
	}

	HandleSpellAI();
}

void Creature::OnInactive()
{
	if (IsPet())
	{
		Unit* owner = GetOwner();
		if (owner == NULL)
		{
			if (m_summoned)
				AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
			else
			{
				m_ispet = false;
				m_petownerid = 0;
				m_petstate = PET_STATE_FOLLOW;
				m_summoned = false;
				AddEvent(&Unit::SetPosition, (int32)m_spawnx, (int32)m_spawnx, EVENT_UNIT_SETPOSITION, 1, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
		}
		else
			AddEvent(&Unit::SetPosition, (int32)owner->m_positionX, (int32)owner->m_positionY, EVENT_UNIT_SETPOSITION, 1, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
	else
	{
		if (IsAttacking())
			StopAttacking();

		if (m_waypointMap.size() == 0)
			AddEvent(&Unit::SetPosition, (int32)m_spawnx, (int32)m_spawny, EVENT_OBJECT_DELETE, 1, 1, EVENT_FLAG_NOT_IN_WORLD);

		m_POIObject = 0;
		StopMovement();
	}

	if (m_script != NULL)
		m_script->OnInactive();
}

void Creature::OnHealthChange(uint32 old)
{
	Unit::OnHealthChange(old);
	if (!IsAlive())
	{
		m_currentwaypoint = 0;
		//make a respawn event :P
		RemoveEvents(EVENT_CREATURE_RESPAWN);
		RemoveEvents(EVENT_CREATURE_INVIS);

		if (m_norespawn)
			AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 5000, 1, EVENT_FLAG_NOT_IN_WORLD);
		else
		{
			uint32 respawntime = m_proto->respawntime;
			AddEvent(&Creature::Respawn, EVENT_CREATURE_RESPAWN, respawntime, 1, EVENT_FLAG_NOT_IN_WORLD);
			//make corpse invisible
			if (respawntime >= 6000)
			{
				AddEvent(&Creature::Invis, EVENT_CREATURE_INVIS, m_proto->boss == 1? 1800000 : 5000, 1, EVENT_FLAG_NOT_IN_WORLD);
				if (m_spawn != NULL && m_proto->boss == 1)
				{
					m_spawn->nextspawn = g_time + (respawntime / 1000);
					sCreatureSpawn.Save(m_spawn);
				}
			}
			else if (respawntime > 1000)
			{
				AddEvent(&Creature::Invis, EVENT_CREATURE_INVIS, respawntime - 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
		}

		if (m_script != NULL)
			m_script->OnDeath();

		//drop inventory
		uint32 startpos = NUM_EQUIPPED_ITEMS;
		if (IsPet() || m_proto->maxlootquality > 0)
			startpos = 0;
		for (uint32 i = startpos; i < NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS; ++i)
		{
			if (m_items[i] != NULL && !m_items[i]->m_nodrop)
			{
				if (sItemStorage.ItemDisabled(m_items[i]->m_data->id) == ITEM_DISABLED_NORMAL || sItemStorage.ItemDisabled(m_items[i]->m_data->id) == ITEM_DISABLED_BOSSDROP_TEMP)
				{
					Event* forcedeleteevent = m_items[i]->GetEvent(EVENT_ITEM_FORCEDELETE);
					if (forcedeleteevent == NULL)
					{
						m_items[i]->AddEvent(&Item::Delete, EVENT_ITEM_FORCEDELETE, m_proto->respawntime, 1, EVENT_FLAG_NOT_IN_WORLD);
						m_items[i]->m_expiretime = g_time + (m_proto->respawntime / 1000);
					}
					else
						forcedeleteevent->DecRef();
				}
				Item* it = m_items[i];
				m_items[i]->Drop();
			}
		}

		if (IsBoss())
		{
			AwardExp();
			AwardBossLoot();
			m_damagedunits.clear();
		}

		if (IsPet() && !m_summoned)
		{
			//remove pet status
			Unit* owner = GetOwner();
			if (owner != NULL)
			{
				owner->m_pets.erase(m_serverId);
				owner->AddEvent(&Unit::UpdatePetFollowAngles, EVENT_UNIT_UPDATEPETS, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
			}

			m_ispet = false;
			m_petownerid = 0;
			m_petstate = PET_STATE_FOLLOW;
		}

		Object* lastdamager = m_mapmgr->GetObject(m_lastdamager);

		if (lastdamager != NULL && lastdamager->IsPlayer())
			TO_PLAYER(lastdamager)->OnKill(this);
	}
}

void Creature::Respawn()
{
	if (m_proto->color1 == 100) //random
		SetModels(-1, -1, -1, -1, RandomUInt(20), RandomUInt(20));
	if (!IsPet())
		SetPosition(m_spawnx, m_spawny);
	if (m_proto->unitflags != 0)
		m_unitflags = m_proto->unitflags;
	Revive();

	for (int32 i = NUM_EQUIPPED_ITEMS + NUM_INVENTORY_ITEMS - 1; i >= 0; --i)
	{
		if (m_items[i] == NULL)
			continue;
		m_items[i]->Delete();
	}

	//did we have a weapon?
	if (m_proto->weapondisplay != 0)
	{
		//recreate it
		Item* it = new Item(m_proto->weapondisplay, this);
		it->m_nodrop = true;
		it->Equip();
	}

	if (m_proto->equipment != 0)
	{
		CreatureEquipment* equip = sCreatureEquipment.GetEntry(m_proto->equipment);
		if (equip != NULL)
		{
			for (uint32 i = 0; i < 5; ++i)
			{
				if (equip->items[i] == 0)
					continue;
				Item* it = new Item(equip->items[i], this);
				it->m_nodrop = true;
				it->Equip();
			}
		}
	}

	//creature equipment
	std::map<uint32, uint32>* equip = sItemStorage.GetNPCEquip(m_proto->entry);

	if (equip != NULL)
	{
		for (std::map<uint32, uint32>::iterator itr = equip->begin(); itr != equip->end(); ++itr)
		{
			Item* it = new Item(itr->first, this);
			it->m_stackcount = itr->second;
			it->m_nodrop = true;
			it->Equip();
			if (it->m_currentslot == 0xFFFFFFFF)
				it->Delete();
		}
	}

	if (m_invis)
		Uninvis();
	if (m_proto->boss == 0 && m_proto->maxlootquality != 0 && !IsPet())
	{
		uint32 numitems = RandomUInt(4);
		for (uint32 i = 0; i < numitems; ++i)
		{
			Item* it = GenerateLootItem(m_proto->minlootquality, m_proto->maxlootquality);

			if (it != NULL)
				it->AddToInventory();
		}
	}

	if (m_proto->boss == 0 && m_proto->lootentry)
	{
		std::vector<uint32>* loottable = sItemStorage.GetLootTable(m_proto->lootentry);
		if (loottable != NULL)
		{
			for (std::vector<uint32>::iterator itr = loottable->begin(); itr != loottable->end(); ++itr)
			{
				uint32 itemid = *itr;
				ItemProto* idata = sItemStorage.GetItem(itemid);
				if (idata == NULL)
					continue;
				Item* it = new Item(idata->id, this);
				it->AddToInventory();
			}
		}
	}

	if (m_script != NULL)
		m_script->OnRespawn();
}

void Creature::HandleSpellAI()
{
	for (std::vector<CreatureSpell>::iterator itr = m_knownspells.begin(); itr != m_knownspells.end(); ++itr)
	{
		SpellEntry* sp = sSpellData.GetEntry(itr->spellid);
		if (!IsAttacking() && itr->spellid != 41) //night vision for npcs who see invis
			continue;
		if (sp == NULL || !CanCast(itr->spellid))
			continue;
		if (sp->targetallowedtype == SPELL_TARGET_SELF)
		{
			if (sp->effect == SPELL_EFFECT_APPLY_AURA || sp->effect == SPELL_EFFECT_APPLY_AURA_NEG
				|| sp->effect == SPELL_EFFECT_APPLY_AREA_AURA || sp->effect == SPELL_EFFECT_APPLY_AREA_AURA_NEG)
			{
				SpellAura* aur = sSpellAuras.GetEntry(sp->triggerspell);
				if (aur == NULL || HasAura(aur->slot))
					continue;
			}
			TargetData targ;
			targ.flags = SPELL_TARGET_SELF;
			Spell s(this, itr->spellid, targ);
			s.Cast();

			if (itr->spellid == 41 && m_POIObject == 0) //night vision
				SetAnimationType(ANIM_TYPE_STANDING);
		}

		Object* followobj = m_mapmgr->GetObject(m_POIObject);
		Unit* ufollowobj = (followobj != NULL && followobj->IsUnit()) ? (Unit*)followobj : NULL;

		if (sp->targetallowedtype & SPELL_TARGET_UNIT && ufollowobj != NULL && IsInRange(ufollowobj) && InLineOfSight(ufollowobj, true))
		{
			if ((sp->effect == SPELL_EFFECT_SHORT_TELEPORT || sp->effect == SPELL_EFFECT_ATTACK_RUN) && GetDistance(followobj) <= followobj->m_bounding * 2)
				continue;

			if (sp->effect == SPELL_EFFECT_DISPEL && !ufollowobj->HasAura(SPELL_AURA_MUTATIO_NIMBUS) && !ufollowobj->HasAura(SPELL_AURA_MUTATIO_FLAMMA) && !ufollowobj->HasAura(SPELL_AURA_WHITE_POT))
				continue;

			if (sp->effect == SPELL_EFFECT_ATTACK_RUN && m_attackRun)
				continue; //already attack running
			TargetData targ;
			targ.flags = SPELL_TARGET_UNIT;
			targ.x = m_POIObject;
			Spell s(this, itr->spellid, targ);
			s.Cast();
		}

		if (sp->targetallowedtype & SPELL_TARGET_RANDOM_RADIUS)
		{
			float angle = (float)RandomUInt(360) * (PI / 180);
			float radius = RandomUInt(sp->radius / 2) + (sp->radius / 2);
			TargetData targ;
			targ.flags = SPELL_TARGET_GROUND;
			targ.x = m_positionX + sinf(angle) * radius;
			targ.y = m_positionY + cosf(angle) * radius;
			Spell s(this, itr->spellid, targ);
			s.Cast();
		}

		if (sp->targetallowedtype & SPELL_TARGET_GROUND && followobj != NULL && IsInRange(followobj))
		{
			TargetData targ;
			targ.flags = SPELL_TARGET_GROUND;
			targ.x = followobj->m_positionX;
			targ.y = followobj->m_positionY;
			Spell s(this, itr->spellid, targ);
			s.Cast();
		}
	}
}

void Creature::CreateWeapon()
{
	if (m_proto->weapondisplay == 0)
		return;
	ItemProto* i = sItemStorage.GetItem(m_proto->weapondisplay);
	if (i == NULL)
		return;
	Item* it = new Item(i->id, this);
	it->AddToInventory();
	it->Equip();
}

uint32 Creature::GetMeleeDamage()
{
	if (IsPet())
	{
		Unit* owner = GetOwner();
		float basedmg;
		//no weapon
		if (GetCurrentWeapon() == NULL)
			basedmg = owner? owner->GetLevel() : m_proto->level;
		else
		{
			uint32 maxdmg = GetCurrentWeapon()->m_data->basevalue;
			basedmg = RandomUInt(maxdmg / 2) + (maxdmg / 2);
		}
		uint32 dmg = basedmg;
		if (HasAura(SPELL_AURA_ROBUR) != NULL)
			dmg *= float(100 + m_auras[SPELL_AURA_ROBUR]->m_spell->value) / 100;
		if (HasAura(SPELL_AURA_BATTLE_SCROLL))
			dmg *= float(100 + m_auras[SPELL_AURA_BATTLE_SCROLL]->m_spell->value) / 100;

		if (owner != NULL)
			dmg *= float(100 + owner->GetLevel()) / 100;

		return dmg;
	}
	else
	{
		uint32 dmg = m_proto->meleedamage;

		if (m_level != m_proto->level && m_proto->boss != 1)
			dmg *= float(m_level) / m_proto->level;

		if (HasAura(SPELL_AURA_ROBUR) != NULL)
			dmg *= float(100 + m_auras[SPELL_AURA_ROBUR]->m_spell->value) / 100;
		if (HasAura(SPELL_AURA_BATTLE_SCROLL))
			dmg *= float(100 + m_auras[SPELL_AURA_BATTLE_SCROLL]->m_spell->value) / 100;
		return dmg;
	}
}

Unit* Creature::GetOwner()
{
	if (!IsPet() || m_mapmgr == NULL)
		return NULL;
	Object* o = m_mapmgr->GetObject(m_petownerid);
	if (o != NULL && o->IsUnit())
		return TO_UNIT(o);
	return NULL;
}

void Creature::SetAsPet( uint16 ownerid, bool summoned /*= false*/, uint8 petstate /*= PET_STATE_FOLLOW*/ )
{
	m_summoned = summoned;
	m_ispet = true;
	m_petownerid = ownerid;
	m_petstate = petstate;
	ClearPOI();

	if (summoned)
		DestroyInventory();
}

void Creature::OnPreRemoveFromWorld()
{
	if (IsPet())
	{
		Unit* owner = GetOwner();
		if (owner != NULL)
		{
			owner->m_pets.erase(m_serverId);
			owner->AddEvent(&Unit::UpdatePetFollowAngles, EVENT_UNIT_UPDATEPETS, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
		}
	}

	Unit::OnPreRemoveFromWorld();
}

void Creature::OnAddToWorld()
{
	Respawn(); //initializes all the combat stuff
	if (IsPet())
	{
		Unit* owner = GetOwner();
		if (owner != NULL)
		{
			owner->m_pets.insert(m_serverId);

			if (owner->IsPlayer() && m_summoned)
			{
				m_maxhealth = m_proto->health / m_proto->level * owner->GetLevel();
				SetHealth(m_maxhealth);
			}
		}
	}

	if (m_spawn != NULL && m_spawn->nextspawn > g_time)
	{
		uint32 mstospawn = (m_spawn->nextspawn - g_time) * 1000;
		AddEvent(&Creature::Respawn, EVENT_CREATURE_RESPAWN, mstospawn, 1, EVENT_FLAG_NOT_IN_WORLD);
		Invis();
	}

	if (m_script != NULL)
		m_script->OnAddToWorld();

	m_bonuses[BONUS_DAMAGE_PCT] += m_proto->damagebonus;
	m_bonuses[ALL_MAGIC_BONUS] += m_proto->damagebonus;
	m_bonuses[CAST_SPEED] += m_proto->speedbonus;
	m_bonuses[ATTACK_SPEED] += m_proto->speedbonus;

	m_speed = m_proto->speed;

	if (m_speed == 0)
		SetUnitFlag(UNIT_FLAG_CANNOT_MOVE);
}

void Creature::Uninvis()
{
	m_invis = false;
	m_mapmgr->OnObjectChangePosition(this);

	//try and count players
	if (!IsPet() && !m_proto->boss && !m_proto->invisblock)
	{
		for (std::map<Unit*, uint8>::iterator itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
		{
			if (itr->first->IsPlayer() && !TO_PLAYER(itr->first)->m_invis)
			{
				Invis();
				AddEvent(&Creature::Uninvis, EVENT_CREATURE_INVIS, 10000, 1, EVENT_FLAG_NOT_IN_WORLD);
				break;
			}
		}
	}
}

Creature::~Creature()
{
	if (m_script != NULL) delete m_script;
}

Unit* Creature::FindTarget()
{
	if (m_POI != POI_IDLE && m_POI != POI_ATTACKING && m_POI != POI_PET_FOLLOW_OWNER_BATTLE) //switch target or aquire, we dont interrupt other POIs
		return NULL;
	if (HasUnitFlag(UNIT_FLAG_INACTIVE_QUESTGIVER))
		return NULL;
	uint32 inrangeunitsize = m_inrangeUnits.size();
	if (m_mapmgr == NULL || inrangeunitsize == 0 || inrangeunitsize > 2048)
		return NULL;
	Unit** objs = (Unit**)alloca(sizeof(Unit*) * inrangeunitsize);
	uint32 numobjs = 0;

	for (std::map<Unit*, uint8>::iterator itr=m_inrangeUnits.begin(); itr!=m_inrangeUnits.end(); ++itr)
	{
		if (itr->first->IsAlive() && IsHostile(itr->first))
		{
			if (!m_mapmgr->InLineOfSight(m_positionX, m_positionY, itr->first->m_positionX, itr->first->m_positionY, PASS_FLAG_GAS))
				continue;
			if ((itr->first->HasAura(SPELL_AURA_INVISIBILITY) || itr->first->HasAura(SPELL_AURA_HIDE)) && !HasAura(SPELL_AURA_NIGHT_VISION))
				continue;
			if ((m_proto->boss != 2 || IsPet()) && itr->first->ProtectedByGround())
				continue;
			if (itr->first->HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
				continue;
			if (HasUnitFlag(UNIT_FLAG_DO_NOT_ATTACK_PLAYERS) && itr->first->IsPlayer())
				continue;
			if (m_POI == POI_IDLE && m_proto->boss == 1 && GetDistance(itr->first) >= 55)
				continue;
			objs[numobjs++] = itr->first;
		}
	}

	if (numobjs == 0)
		return NULL;
	return objs[RandomUInt(numobjs - 1)];
}

void Creature::OnCombatStart()
{
	if (m_script != NULL)
		m_script->OnCombatStart();
	//put a random cooldown on all spells so people dont get raped by beams instantly
	for (std::vector<CreatureSpell>::iterator itr = m_knownspells.begin(); itr != m_knownspells.end(); ++itr)
	{
		if (itr->cooldown < 60000)
			AddSpellCooldown(itr->spellid, RandomUInt(itr->cooldown));
		else
			AddSpellCooldown(itr->spellid, itr->cooldown); //enrage spell
	}
}

void Creature::AddWayPoint( int32 x, int32 y, uint32 waittime )
{
	WayPoint p;
	p.x = x;
	p.y = y;
	p.waittime = waittime;
	m_waypointMap.push_back(p);
}

void Creature::AwardExp( bool samefaction /*= false*/ )
{
	uint32 numplayers = 0;

	for (std::set<uint16>::iterator itr = m_damagedunits.begin(); itr != m_damagedunits.end(); ++itr)
	{
		Object* damageobj = m_mapmgr->GetObject(*itr);
		if (damageobj == NULL || !damageobj->IsPlayer())
			continue;
		if (TO_PLAYER(damageobj)->IsAlive() && !IsInRange(damageobj))
			continue;
		if (!samefaction && m_faction == TO_UNIT(damageobj)->m_faction)
			continue;
		if (m_killedplayers.find(TO_PLAYER(damageobj)->m_playerId) != m_killedplayers.end())
			continue;
		++numplayers;
	}

	float bonusval = 1.0f;

	if (numplayers > 3)
		bonusval = float(100 + (numplayers * 2)) / 100;

	for (std::set<uint16>::iterator itr = m_damagedunits.begin(); itr != m_damagedunits.end(); ++itr)
	{
		Object* damageobj = m_mapmgr->GetObject(*itr);
		if (damageobj == NULL || !damageobj->IsPlayer())
			continue;
		if (TO_PLAYER(damageobj)->IsAlive() && !IsInRange(damageobj))
			continue;
		if (!samefaction && m_faction == TO_UNIT(damageobj)->m_faction)
			continue;
		if (m_killedplayers.find(TO_PLAYER(damageobj)->m_playerId) != m_killedplayers.end())
			continue;
		Player* p = TO_PLAYER(damageobj);
		p->OnKill(this);

		uint32 basexp = m_proto->exp;
		if (m_proto->entry >= 10000 && m_proto->boss == 0)
			basexp = m_proto->exp * m_level;
		uint32 exp = basexp;

		exp = NormaliseExp(exp, m_proto->level, p->m_level, false);

		//if (exp == 0)
		//	continue;

		uint32 bonusexp = (exp * bonusval) - exp;
		
		exp *= g_ServerSettings.GetFloat("mod_exp_boss");
		bonusexp *= g_ServerSettings.GetFloat("mod_exp_boss");
		
		if (bonusexp > 0)
			p->EventChatMsg(0, "\xB3""Gained %u exp (%u group bonus)!\xB3", exp + bonusexp, bonusexp);
		else
			p->EventChatMsg(0, "\xB3""Gained %u exp!\xB3", exp);
		p->GiveExp(exp + bonusexp);
	}
}

void Creature::AwardItem( Item* it, Player* plr )
{
	if (sItemStorage.ItemDisabled(it->m_data->id) == ITEM_DISABLED_BOSSDROP_TEMP)
	{
		plr->EventChatMsg(0, "You recieve loot: %s (temporary)", it->m_data->name);

		Event* forcedeleteevent = it->GetEvent(EVENT_ITEM_FORCEDELETE);
		if (forcedeleteevent == NULL)
		{
			it->AddEvent(&Item::Delete, EVENT_ITEM_FORCEDELETE, (3600000 * 6), 1, EVENT_FLAG_NOT_IN_WORLD);
			it->m_expiretime = g_time + (3600 * 6);
		}
		else
			forcedeleteevent->DecRef();
	}
	else
	{
		if (sItemStorage.IsStackable(it->m_data->type))
			plr->EventChatMsg(0, "You recieve loot: %s (%u)", it->m_data->name, it->m_stackcount);
		else
			plr->EventChatMsg(0, "You recieve loot: %s", it->m_data->name);
	}

	it->m_owner = plr;
	it->AddToInventory();

	LOG_ACTION_U(this, "AWARDITEM", "Awarded %s (itemid:%llu) to %s", it->m_data->name, it->m_itemguid, plr->m_namestr.c_str());
	LOG_ACTION_U(plr, "RECIEVEITEM", "Recieved %s (itemid:%llu) from %s", it->m_data->name, it->m_itemguid,  m_namestr.c_str());

}

void Creature::OnActive()
{
	Unit::OnActive();
	if (m_script != NULL)
		m_script->OnActive();
}

void Creature::EventAddWayPointT( uint32 delay, int32 x, int32 y, uint32 waittime )
{
	AddEvent(&Creature::AddWayPointT, x, y, waittime, EVENT_CREATURE_ADDWAYPOINT, delay, 1, EVENT_FLAG_NOT_IN_WORLD);
}

void Creature::RemoveCreature()
{
	AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);

	if (m_spawn != NULL)
	{
		m_spawn->x = 0xFFFFFFFF;
		m_spawn->y = 0xFFFFFFFF;
		m_spawn->mapid = 0;
		m_spawn->entry = 0;
		sCreatureSpawn.Save(m_spawn);
	}
}

Player* Creature::GetRandomPlayer( bool allowsamefaction /*= true*/ )
{
	//pick a random player in range
	uint32 numplayers = 0;
	Player** plrs = (Player**)alloca(m_inrangeUnits.size() * sizeof(Player*));

	for (std::map<Unit*, uint8>::iterator itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
	{
		if (!itr->first->IsAlive() || itr->first->HasUnitFlag(UNIT_FLAG_UNATTACKABLE) || !itr->first->IsPlayer())
			continue;
		if (!allowsamefaction && !IsHostile(itr->first))
			continue;
		plrs[numplayers++] = TO_PLAYER(itr->first);
	}

	if (numplayers > 0)
		return plrs[RandomUInt(numplayers - 1)];
	return NULL;
}

bool Creature::CanCast( uint32 spellid, bool triggered /*= false*/ )
{
	std::map<uint32, uint32>::iterator itr = m_spellcooldownmap.find(spellid);
	if (itr != m_spellcooldownmap.end() && itr->second > GetMSTime())
		return false;

	SpellEntry* en = sSpellData.GetEntry(spellid);

	if (en == NULL)
		return false;

	//Throw weapon: can't cast without weapon, can't cast if weapon hasn't been caught back yet
	if (en->effect == SPELL_EFFECT_THROW_WEAPON)
	{
		Item* curweapon = GetCurrentWeapon();
		if (curweapon == NULL)
			return false;
		if (curweapon->m_data->id == m_thrownweapon)
			return false;
	}
	return true;
}

bool Creature::HasSpell( uint32 spellid )
{
	for (std::vector<CreatureSpell>::iterator itr = m_knownspells.begin(); itr != m_knownspells.end(); ++itr)
	{
		if ((*itr).spellid == spellid)
			return true;
	}
	return false;
}

void Creature::InitCreatureSpells()
{
	m_knownspells.clear();
	if (m_proto->spellset != 0)
	{
		CreatureSpells* spells = sCreatureSpells.GetEntry(m_proto->spellset);
		if (spells != NULL)
		{
			for (uint32 i=0; i<5; ++i)
			{
				CreatureSpell sptmp;
				sptmp.spellid = spells->spells[i];
				sptmp.cooldown = spells->cooldowns[i];

				//allow creatures with ids > 10000 to rank up spells
				if (m_proto->entry >= 10000)
				{
					uint32 curspellid = spells->spells[i];
					SpellEntry* spell = sSpellData.GetEntry(curspellid);
					SpellData* spelld = sSpellStorage.GetSpell(curspellid); 
					SpellEntry* nspell = sSpellData.GetEntry(curspellid + 1);
					SpellData* nspelld = sSpellStorage.GetSpell(curspellid + 1); 
					while (spell != NULL && spelld != NULL && nspell != NULL && nspelld != NULL)
					{
						SpellEntry* spell = sSpellData.GetEntry(curspellid);
						SpellData* spelld = sSpellStorage.GetSpell(curspellid); 
						SpellEntry* nspell = sSpellData.GetEntry(curspellid + 1);
						SpellData* nspelld = sSpellStorage.GetSpell(curspellid + 1); 

						if (m_level < nspelld->classlevel[CLASS_ADEPT])
							break;

						//isnt a spell chain
						if (spell->spelllevel + 1 != nspell->spelllevel)
							break;
						
						++curspellid;
					}

					sptmp.spellid = curspellid;
				}

				m_knownspells.push_back(sptmp);
			}
		}
	}
}

void Creature::UpdateQuests()
{
	if (m_unitflags & UNIT_FLAG_INACTIVE_QUESTGIVER)
		return;

	//firstly look for players who have completed the quest
	for (std::map<Unit*, uint8>::iterator itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
	{
		for (uint32 i = 0; i < 5; ++i)
		{
			QuestEntry* quest = sQuests.GetEntry(m_quests->quests[i]);
			if (quest == NULL)
				continue;

			QuestText* text = sQuestText.GetEntry(m_quests->quests[i]);
			if (!itr->first->IsPlayer() || IsHostile(itr->first))
				continue;

			Player* p = TO_PLAYER(itr->first);

			if (p->m_currentquest == quest && p->m_currentquestcount == quest->reqcount)
			{
				//reward time (YAY)
				if (text == NULL)
					GiveQuestReward(quest->id, p->m_playerId);
				else
				{
					uint32 timer = 0;

					ResetChatMsgTimer();

					for (uint32 j = 0; j < 3; ++j)
					{
						if (strcmp(text->endtext[j], "NONE") != 0)
						{
							timer += 3000;
							std::string txt = text->endtext[j];
							ConvertQuestText(p, txt);
							EventChatMsg(0, "%s", txt.c_str());
						}
					}

					//ok
					SetUnitFlag(UNIT_FLAG_INACTIVE_QUESTGIVER);
					AddEvent(&Unit::RemoveUnitFlag, (uint32)UNIT_FLAG_INACTIVE_QUESTGIVER, EVENT_CREATURE_UPDATEQUEST, timer + 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
					AddEvent(&Creature::GiveQuestReward, quest->id, p->m_playerId, EVENT_CREATURE_UPDATEQUEST, timer + 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
					return;
				}
			}
		}
	}

	//now we look for players using the keyword
	for (std::map<Unit*, uint8>::iterator itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
	{
		if (!itr->first->IsPlayer() || IsHostile(itr->first))
			continue;
		for (uint32 i = 0; i < 5; ++i)
		{
			QuestEntry* quest = sQuests.GetEntry(m_quests->quests[i]);
			if (quest == NULL)
				continue;

			QuestText* text = sQuestText.GetEntry(m_quests->quests[i]);

			Player* p = TO_PLAYER(itr->first);

			if (p->m_currentquest != NULL)
				continue;

			if (quest->startkeyword == NULL)
				continue;

			if (p->m_lastSay.find(quest->startkeyword) == std::string::npos)
				continue;

			if (quest->flags & 1 && p->m_completedquests.find(quest->id) != p->m_completedquests.end())
				continue;
			if (quest->reqquest != 0 && p->m_completedquests.find(quest->reqquest) == p->m_completedquests.end())
				continue;

			if (text == NULL)
				GivePlayerQuest(quest->id, p->m_playerId);
			else
			{
				uint32 timer = 0;

				ResetChatMsgTimer();

				for (uint32 j = 0; j < 3; ++j)
				{
					if (strcmp(text->starttext[j], "NONE") != 0)
					{
						timer += 3000;
						std::string txt = text->starttext[j];
						ConvertQuestText(p, txt);
						EventChatMsg(0, "%s", txt.c_str());
					}
				}

				//ok
				SetUnitFlag(UNIT_FLAG_INACTIVE_QUESTGIVER);
				AddEvent(&Unit::RemoveUnitFlag, (uint32)UNIT_FLAG_INACTIVE_QUESTGIVER, EVENT_CREATURE_UPDATEQUEST, timer + 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
				AddEvent(&Creature::GivePlayerQuest, quest->id, p->m_playerId, EVENT_CREATURE_UPDATEQUEST, timer + 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
				return;
			}
		}
	}

	//now we look for players eligable
	for (std::map<Unit*, uint8>::iterator itr = m_inrangeUnits.begin(); itr != m_inrangeUnits.end(); ++itr)
	{
		if (!itr->first->IsPlayer() || IsHostile(itr->first) || itr->first->HasUnitFlag(UNIT_FLAG_INVISIBLE))
			continue;

		for (uint32 i = 0; i < 5; ++i)
		{
			QuestEntry* quest = sQuests.GetEntry(m_quests->quests[i]);
			if (quest == NULL)
				continue;

			QuestText* text = sQuestText.GetEntry(m_quests->quests[i]);

			Player* p = TO_PLAYER(itr->first);

			if (m_blockedquestplayers.find(p->m_playerId) != m_blockedquestplayers.end())
				continue;

			if (!p->IsEligableForQuest(quest))
				continue;

			if (text != NULL)
			{
				uint32 timer = 0;

				ResetChatMsgTimer();

				for (uint32 j = 0; j < 3; ++j)
				{
					if (strcmp(text->offertext[j], "NONE") != 0)
					{
						timer += 3000;
						std::string txt = text->offertext[j];
						ConvertQuestText(p, txt);
						EventChatMsg(0, "%s", txt.c_str());	
					}
				}

				//ok
				SetUnitFlag(UNIT_FLAG_INACTIVE_QUESTGIVER);
				AddEvent(&Creature::SetQuestOfferData, quest->id, p->m_playerId, EVENT_CREATURE_UPDATEQUEST, timer + 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
				m_questplrid = p->m_playerId;
				AddEvent(&Creature::RetractQuestOffer, EVENT_CREATURE_RETRACT_QUEST_OFFER, timer + 15000, 1, EVENT_FLAG_NOT_IN_WORLD);
				return;
			}
		}
	}
}

void Creature::RetractQuestOffer()
{
	BlockPlayerQuestOffers(m_questplrid);
	m_questofferid = 0;
	m_questplrid = 0;

	if (!IsAttacking())
		m_POIObject = 0;
	RemoveUnitFlag(UNIT_FLAG_INACTIVE_QUESTGIVER);
	ResetChatMsgTimer();
	SendChatMsg("Fine, don't answer");
}

void Creature::UpdateQuestOffer()
{
	if (m_questofferid == 0) //player accepted, still following him
		return;
	Player* p = m_mapmgr->GetPlayer(m_questplrid);
	QuestEntry* quest = sQuests.GetEntry(m_questofferid);
	QuestText* text = sQuestText.GetEntry(m_questofferid);

	if (p == NULL || quest == NULL)
		return;

	if (p->m_lastSay.find("no") != std::string::npos)
	{
		RemoveEvents(EVENT_CREATURE_RETRACT_QUEST_OFFER);
		BlockPlayerQuestOffers(m_questplrid);
		m_questplrid = 0;
		m_questofferid = 0;
		ResetChatMsgTimer();
		SendChatMsg("Fine, good day.");
		AddEvent(&Unit::RemoveUnitFlag, (uint32)UNIT_FLAG_INACTIVE_QUESTGIVER, EVENT_CREATURE_UPDATEQUEST, 5000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	if (p->m_lastSay.find("yes") == std::string::npos)
		return;

	RemoveEvents(EVENT_CREATURE_RETRACT_QUEST_OFFER);

	m_questofferid = 0;

	if (text == NULL)
		GivePlayerQuest(m_questplrid, m_questofferid);
	else
	{
		uint32 timer = 0;

		ResetChatMsgTimer();

		for (uint32 j = 0; j < 3; ++j)
		{
			if (strcmp(text->starttext[j], "NONE") != 0)
			{
				timer += 3000;
				std::string txt = text->starttext[j];
				ConvertQuestText(p, txt);
				EventChatMsg(0, "%s", txt.c_str());
			}
		}

		//ok
		SetUnitFlag(UNIT_FLAG_INACTIVE_QUESTGIVER);

		AddEvent(&Creature::QuestGiveComplete, EVENT_CREATURE_UPDATEQUEST, timer + 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
		AddEvent(&Creature::GivePlayerQuest, quest->id, p->m_playerId, EVENT_CREATURE_UPDATEQUEST, timer + 1000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
}

void Creature::BlockPlayerQuestOffers( uint32 plrid )
{
	m_blockedquestplayers.insert(plrid);
	AddEvent(&Creature::RemoveBlockedPlayer, plrid, EVENT_CREATURE_UPDATEQUEST, RandomUInt(300000), 1, EVENT_FLAG_NOT_IN_WORLD);
}

void Creature::GiveQuestReward( uint32 questid, uint32 playerid )
{
	Player* p = m_mapmgr->GetPlayer(playerid);
	QuestEntry* quest = sQuests.GetEntry(questid);

	if (p == NULL || quest == NULL)
		return;

	p->ResetChatMsgTimer();

	if (quest->exp > 0)
	{
		auto mod = g_ServerSettings.GetFloat("mod_exp_quest");
		p->EventChatMsg(0, "\xB3""Gained %u exp!\xB3", uint32(quest->exp * mod));
		p->GiveExp(quest->exp * mod);
	}

	p->m_currentquest = NULL;
	p->m_currentquestcount = 0;

	if (quest->flags & 1)
		p->m_completedquests.insert(questid);

	if (quest->rewardchance == 100) //only give 1 reward out of random rewards
	{
		uint32 numrewards = 0;
		for (uint32 i = 0; i < 5; ++i)
		{
			if (quest->rewards[i] == 0)
				break;
			++numrewards;
		}

		if (numrewards > 0)
		{
			Item* it = new Item(quest->rewards[RandomUInt(numrewards - 1)], p);
			if (!it->AddToInventory())
				it->Drop();
		}
	}
	else
	{
		for (uint32 i = 0; i < 5; ++i)
		{
			if (quest->rewards[i] == 0)
				continue;

			if (quest->rewardchance > 0 && RandomUInt(99) >= quest->rewardchance)
				continue;

			Item* it = new Item(quest->rewards[i], p);
			if (!it->AddToInventory())
				it->Drop();
		}
	}
}

void Creature::SendStorageList( Player* to )
{
	ServerPacket p(SMSG_MERCHANT_DATA);

	uint8 bufdata[154] = { 0x01, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x63, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x01, 0x00,
		0x00, 0x00, 0x0B, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0D, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x0E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xD7, 0x00,};

	p.Write(bufdata, 154);

	to->Send(&p);
	to->SendAllStats();
}

bool Creature::IsBoss()
{
	return m_proto->boss == 1;
}

Item* Creature::GenerateLootItem( float minqual, float maxqual, bool norandomroll /*= false*/, Player* personal_for )
{
	uint32 attempts = 0;

	while (attempts < 1000)
	{
		++attempts;
		uint32 itemid = RandomUInt(sItemStorage.GetMaxItemID());
		if (sItemStorage.ItemDisabled(itemid) == ITEM_DISABLED_NORMAL)
			continue;

		if (sItemStorage.ItemDisabled(itemid) == ITEM_DISABLED_BOSSDROP_TEMP && personal_for == NULL)
			continue;

		ItemProto* idata = sItemStorage.GetItem(itemid);
		if (idata == NULL)
			continue;

		if (idata->rarity == 500.0f) //quest items
			continue;
		//if (idata->rarity >= 899.9f) //xencon and admin items
		//	continue;

		auto mod_loot_max = g_ServerSettings.GetInt("mod_loot_max");
		auto mod_loot_min = g_ServerSettings.GetInt("mod_loot_min");
		auto mod_loot_roll_chance_exp = g_ServerSettings.GetFloat("mod_loot_roll_chance_exp");
		auto mod_loot_roll_chance_lin = g_ServerSettings.GetInt("mod_loot_roll_chance_lin");

		if (personal_for != NULL)
		{
			mod_loot_max = 0;
			mod_loot_min = 0;
			mod_loot_roll_chance_exp = 1.0f;
			mod_loot_roll_chance_lin = 0;
		}

		if (idata->rarity > (m_proto->maxlootquality + mod_loot_max))
			continue;
		if (idata->rarity < (m_proto->minlootquality + mod_loot_min))
			continue;

		if (!norandomroll)
		{
			if (idata->rarity > 60) //exponential roll
			{
				int32 dice_sides = idata->rarity * idata->rarity;
				dice_sides /= mod_loot_roll_chance_exp;
				if (RandomUInt(dice_sides) != 0)
					continue;
			}
			else
			{
				int32 dice_sides = idata->rarity / 5;
				dice_sides -= mod_loot_roll_chance_lin;
				if (dice_sides < 0)
					dice_sides = 0;

				if (RandomUInt(dice_sides) != 0)
					continue;
			}
		}
		Item* it = new Item(idata->id, this);

		if (personal_for != NULL)
		{
			if (!it->IsForMyClass(personal_for))
			{
				it->Delete();
				continue;
			}
		}


		return it;
	}

	return NULL;
}

void Creature::AwardBossLoot()
{
	for (std::set<uint16>::iterator itr = m_damagedunits.begin(); itr != m_damagedunits.end(); ++itr)
	{
		Object* damageobj = m_mapmgr->GetObject(*itr);
		if (damageobj == NULL || !damageobj->IsPlayer() || *itr == m_serverId)
			continue;
		if (TO_UNIT(damageobj)->IsAlive() && !IsInRange(damageobj))
			continue;
		if (m_killedplayers.find(TO_PLAYER(damageobj)->m_playerId) != m_killedplayers.end())
			continue;
	
		uint32 num = g_ServerSettings.GetUInt("boss_item_numrolls");
		uint32 roll = g_ServerSettings.GetUInt("boss_item_chance");

		for (uint32 i = 0; i < num; ++i)
		{
			if (RandomUInt(99) >= roll)
				continue;

			Item* it = GenerateLootItem(m_proto->minlootquality, m_proto->maxlootquality, true, TO_PLAYER(damageobj));

			if (it != NULL) //why would it be null here? fucked up db?
				AwardItem(it, TO_PLAYER(damageobj));
		}
	}
}

void Creature::UpdatePOI( uint32 forcedpoi /*= 0xFFFFFFFF*/ )
{
	uint32 poi = m_POI;
	if (forcedpoi != 0xFFFFFFFF)
		poi = forcedpoi;
	switch (poi)
	{
	case POI_IDLE:
		{
			IdlePOIHandler();
			return;
		} break;
	case POI_ATTACKING:
		{
			if (m_proto->boss == 0 && (m_updateCounter % 30) != 0) //update every 3 secs for normal npcs
				break;
			Object* poi = GetPOIObject();
			//random chance to change target
			if (RandomUInt(99) == 0 || poi == NULL || !poi->IsUnit() || !TO_UNIT(poi)->IsAlive())
			{
				Unit* tar = FindTarget();
				if (tar != NULL)
					Attack(tar);
				else
					StopAttacking();
			}

			if (poi != NULL && poi->IsUnit() && !IsInRange(poi))
			{
				//try switching to someone in range
				Unit* tar = FindTarget();
				if (tar != NULL)
					Attack(tar);
			}
		} break;
	case POI_PET_FOLLOW_OWNER_BATTLE:
		{
			Unit* tar = FindTarget();
			if (tar != NULL)
				Attack(tar);
			else
				UpdatePOI(POI_PET_FOLLOW_OWNER);
		} break;
	case POI_PET_FOLLOW_OWNER:
		{
			Object* owner = GetOwner();

			if (owner == NULL)
			{
				AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, 0);
				return;
			}

			int32 x, y;
			x = owner->m_positionX - (sinf((owner->m_rotation * PI / 180.0f) + m_followOwnerAngle) * 10);
			y = owner->m_positionY + (cosf((owner->m_rotation * PI / 180.0f) + m_followOwnerAngle) * 10);

			SetPOI(POI_PET_FOLLOW_OWNER, x, y);
		} break;
	}
}

Object* Creature::GetPOIObject()
{
	return m_mapmgr->GetObject(m_POIObject);
}

void Creature::IdlePOIHandler()
{
	m_attackRun = false;

	if (IsPet())
	{
		//set POI based on battle mode
		if (m_petstate == PET_STATE_FOLLOW)
			SetPOI(POI_PET_FOLLOW_OWNER, 0);
		else if (m_petstate == PET_STATE_BATTLE)
			SetPOI(POI_PET_FOLLOW_OWNER_BATTLE, 0);
		else
			return; //stop mode, do nothing
		UpdatePOI();
		return;
	}

	if (m_proto->boss > 0 && GetHealth() < GetMaxHealth())
		SetHealth(GetMaxHealth());

	//npc doing nothing, try and attack stuff?
	if (!HasUnitFlag(UNIT_FLAG_UNATTACKABLE))
	{
		Unit* tar = FindTarget();
		if (tar != NULL)
		{
			Attack(tar);
			return;
		}
	}

	m_destinationRadius = 0;

	int32 tilex = m_positionX / 20;
	int32 tiley = m_positionY / 20;
	int32 spawntx = m_spawnx / 20;
	int32 spawnty = m_spawny / 20;

	bool passable = true;

	if (tilex < 0 || tiley < 0 || spawntx < 0 || spawnty < 0 || tilex >= 3220 || tiley >= 3220 || spawntx >= 3220 || spawnty >= 3220)
		passable = false;


	if (!passable)
	{
		SetPosition(m_spawnx, m_spawny);
		StopMovement();
	}

	if (m_quests != NULL && (m_updateCounter % 20) == 0)
	{
		UpdateQuests();
		if (m_questplrid != 0)
			UpdateQuestOffer();
	}

	if (m_questplrid != 0)
	{
		Player* plr = m_mapmgr->GetPlayer(m_questplrid);

		if (plr != NULL)
		{
			m_POIObject = plr->m_serverId;
			m_destinationRadius = plr->m_bounding;
			m_destinationAngle = -1;
		}
	}
	else if (m_waypointMap.size() > 0)
	{
		if (m_waypointDirection == 0 && m_currentwaypoint >= m_waypointMap.size())
		{
			m_currentwaypoint = m_waypointMap.size() - 1;
			if (m_waypointType == WAYPOINT_TYPE_CIRCLE)
				m_waypointDirection = 1; //backwards
		}

		if (m_waypointDirection == 1 && m_currentwaypoint == -1)
		{
			m_currentwaypoint = 0;
			if (m_waypointType == WAYPOINT_TYPE_CIRCLE)
				m_waypointDirection = 0; //forwards
		}

		WayPoint & curpoint = m_waypointMap[m_currentwaypoint];
		float disttopoint = GetDistance(curpoint.x, curpoint.y);
		if (disttopoint > 1)
		{
			if (!CreatePath(curpoint.x, curpoint.y))
				SetPosition(curpoint.x, curpoint.y);
		}
		else
		{
			Event* ev = GetEvent(EVENT_CREATURE_WAYPOINTMOVE);
			if (ev == NULL)
			{
				if (m_script != NULL)
					m_script->OnReachWayPoint();
				uint32 nextpoint;
				if (m_waypointDirection == 0)
					nextpoint = m_currentwaypoint + 1;
				else
					nextpoint = m_currentwaypoint - 1;
				if (m_waypointType == WAYPOINT_TYPE_CIRCLE || nextpoint < m_waypointMap.size())
					AddEvent(&Creature::MoveToNextWayPoint, EVENT_CREATURE_WAYPOINTMOVE, curpoint.waittime, 1, EVENT_FLAG_NOT_IN_WORLD);
			}
			else
				ev->DecRef();
		}
	}
	else if (m_spawn != NULL && m_spawn->movetype == MOVETYPE_RANDOM)
	{
		if (m_randommovetimer < GetMSTime())
		{
			m_randmovex = m_spawnx + RandomUInt(300) - 150;
			m_randmovey = m_spawny + RandomUInt(300) - 150;
			if (CreatePath(m_randmovex, m_randmovey))
				m_randommovetimer = GetMSTime() + RandomUInt(15000);
			else if (!CreatePath(m_spawnx, m_spawny))
				SetPosition(m_spawnx, m_spawny);
		}
		else
			CreatePath(m_randmovex, m_randmovey);
	}
	else if (!CreatePath(m_spawnx, m_spawny))
		SetPosition(m_spawnx, m_spawny);
}

void Creature::OnPOIFail( uint32 reason )
{
	switch (m_POI)
	{
	case POI_ATTACKING:
		{
			if (reason == POI_CANNOT_CREATE_PATH && m_proto->boss == 1)
			{
				//bosses who can't reach their target reset health
				SetHealth(GetMaxHealth());
			}

			if (reason == POI_OUT_OF_RANGE)
				StopAttacking();

		} break;
	}
}

void Creature::OnPOIChange( uint32 newpoi )
{
	if (newpoi == POI_ATTACKING && !m_incombat)
	{
		m_incombat = true;
		if (m_script != NULL)
			m_script->OnCombatStart();
	}

	if (newpoi != POI_ATTACKING && newpoi != POI_SCRIPT && m_incombat)
	{
		m_incombat = false;
		if (m_script != NULL)
			m_script->OnCombatStop();
	}
}

void Creature::OnAddInRangeObject( Object* o )
{
	Unit::OnAddInRangeObject(o);
	if (o->IsPlayer())
	{
		Player* plr = TO_PLAYER(o);

		SendPlayerQuestInfo(plr);

	}
}

void Creature::SendPlayerQuestInfo( Player* plr, uint32 type )
{
	Packet data(SMSG_QUEST_UI_UPDATE);
	data << uint32(m_serverId) << uint32(type);
	plr->Send(&data);
}

void Creature::SendPlayerQuestInfo( Player* plr )
{
	if (m_quests != NULL)
	{
		for (uint32 i = 0; i < 5; ++i)
		{
			QuestEntry* quest = sQuests.GetEntry(m_quests->quests[i]);

			if (quest == NULL)
				continue;

			if (!plr->IsEligableForQuest(quest))
				continue;

			SendPlayerQuestInfo(plr, 1);
			break;
		}
	}
}
