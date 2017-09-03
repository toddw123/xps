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

Object::Object()
{
	m_lastmove = 0;
	m_active = false;
	m_pathpoint = 0;
	m_pathx = -1;
	m_pathy = -1;
	m_projectilidcounter = 0;
	m_mapmgr = NULL;
	m_deleteonremove = false;
	m_bounding = 22; //meh
	m_typeId = TYPEID_OBJECT;
	m_positionX = 924;
	m_positionY = 1149;
	m_rotation = 0;
	m_cell = NULL;
}

void Object::AddInRangeObject(Object* o)
{
	OnAddInRangeObject(o);
	if (o->IsUnit())
		m_inrangeUnits.insert(std::make_pair(TO_UNIT(o), 0x1F)); //mark update block with every bit so we send all info to client
	if (o->IsSpellEffect())
		m_inrangeSpellEffects.insert(std::make_pair(TO_SPELLEFFECT(o), ++m_projectilidcounter));
	if (o->IsTimedObject())
		m_inrangeTimedObjects.insert(TO_TIMEDOBJECT(o));
	if (IsCreature() && TO_CREATURE(this)->m_script != NULL)
		TO_CREATURE(this)->m_script->OnAddInRangeObject(o);
}

void Object::RemoveInRangeObject(Object* o)
{
	OnRemoveInRangeObject(o);
	if (o->IsUnit())
		m_inrangeUnits.erase(TO_UNIT(o));
	if (o->IsSpellEffect())
		m_inrangeSpellEffects.erase(TO_SPELLEFFECT(o));
	if (o->IsTimedObject())
		m_inrangeTimedObjects.erase(TO_TIMEDOBJECT(o));
	if (IsCreature() && TO_CREATURE(this)->m_script != NULL)
		TO_CREATURE(this)->m_script->OnRemoveInRangeObject(o);
}

void Object::UpdateInRangeSet()
{
	std::map<Unit*, uint8>::iterator itr = m_inrangeUnits.begin();
	Unit* utmp;
	while (itr != m_inrangeUnits.end())
	{
		utmp = itr->first;
		++itr;
		if (!IsInRectRange(utmp)|| !m_inworld || !utmp->CanSee(this))
		{
			RemoveInRangeObject(utmp);
			utmp->RemoveInRangeObject(this);
		}
	}

	std::map<SpellEffect*, uint8>::iterator itr2 = m_inrangeSpellEffects.begin();
	SpellEffect* stmp;
	while (itr2 != m_inrangeSpellEffects.end())
	{
		stmp = itr2->first;
		++itr2;
		if (!IsInRectRange(stmp) || !m_inworld)
		{
			RemoveInRangeObject(stmp);
			stmp->RemoveInRangeObject(this);
		}
	}

	std::set<ItemGroup*>::iterator iitr = m_inrangeItemGroups.begin(), iitr2;
	while (iitr != m_inrangeItemGroups.end())
	{
		iitr2 = iitr++;
		int32 x = (*iitr2)->m_cellx * 20 + 10;
		int32 y = (*iitr2)->m_celly * 20 + 10;
		if (!IsInRectRange(x, y) || !m_inworld)
			RemoveInRangeItemGroup(*iitr2);
	}

	std::set<TimedObject*>::iterator titr = m_inrangeTimedObjects.begin(), titr2;
	while (titr != m_inrangeTimedObjects.end())
	{
		titr2 = titr++;
		if (!IsInRectRange(*titr2) || !m_inworld)
			RemoveInRangeObject(*titr2);
	}
}

float Object::GetDistance(Object* o)
{
	if (o == NULL)
		return 99999.9f;

	//difference
	float dx = m_positionX - o->m_positionX;
	float dy = m_positionY - o->m_positionY;
;
	return sqrt(pow(dx, 2) + pow(dy, 2));
}

float Object::GetDistance(uint16 x, uint16 y)
{
	//difference
	float dx = m_positionX - x;
	float dy = m_positionY - y;

	return sqrt(pow(dx, 2) + pow(dy, 2));
}

float Object::GetDistance( uint16 x, uint16 y, uint16 x2, uint16 y2 )
{
	//difference
	float dx = x - x2;
	float dy = y - y2;
	return sqrt(pow(dx, 2) + pow(dy, 2));
}

void Object::Delete()
{
	if (IsSpellEffect())
		TO_SPELLEFFECT(this)->Decay();
	Destroy();
	if (m_mapmgr == NULL)
	{
		DecRef();
		return;
	}

	m_deleteonremove = true;
	m_mapmgr->RemoveObject(this);
}

void Object::SetPosition( int32 x, int32 y )
{
	//ASSERT(x > 0 && y > 0 && x <= 65000 && y <= 65000);

	if (x < 0 || y < 0 || x >= 65000 || y >= 65000)
		return;

	m_lastmove = g_mstime;
	m_positionX = x;
	m_positionY = y;

	if (m_mapmgr != NULL)
	{
		m_mapmgr->OnObjectChangePosition(this);
		CollideCheck();

		if (IsPlayer())
		{
			auto plr = TO_PLAYER(this);

			if (plr->m_crowdAgentIndex != -1)
			{
				float pos[3] = { x , 0, y };
				auto agent = m_mapmgr->m_crowd->pushAgentPosition(plr->m_crowdAgentIndex, pos);
			}

			uint32 tilex = x / 20;
			uint32 tiley = y / 20;
			uint32 key = tilex | (tiley << 16);
			unordered_multimap<uint32, TeleCoord>::iterator itr = m_mapmgr->m_telecoords.lower_bound(key);
			for (; itr != m_mapmgr->m_telecoords.upper_bound(key); ++itr)
			{
				if (itr->second.factionmask & (1 << TO_PLAYER(this)->m_faction))
				{
					TeleCoord & coord = itr->second;
					if (coord.mapid == m_mapmgr->m_mapid)
					{
						AddEvent(&Object::SetPosition, int32((coord.loc.x * 20) + 10), int32((coord.loc.y * 20) + 10), EVENT_UNIT_SETPOSITION, 100, 1, EVENT_FLAG_NOT_IN_WORLD);

						if (coord.spelleffect)
							TO_PLAYER(this)->SetSpellEffect(itr->second.spelleffect);
					}
					else
					{
						Player* plr = TO_PLAYER(this);
						plr->m_pendingmaptransfer.newmapid = coord.mapid;
						plr->m_pendingmaptransfer.newx = coord.loc.x * 20 + 10;
						plr->m_pendingmaptransfer.newy = coord.loc.y * 20 + 10;
						plr->m_pendingmaptransfer.playerportal = false;
						m_mapmgr->RemoveObject(plr); //removal code now takes care of it :)
					}
					TO_PLAYER(this)->StopMovement();
				}
			}
		}
	}
}

bool Object::IsInRange( Object* o )
{
	if (o->IsUnit())
		return (m_inrangeUnits.find(TO_UNIT(o)) != m_inrangeUnits.end());
	if (o->IsSpellEffect())
		return (m_inrangeSpellEffects.find(TO_SPELLEFFECT(o)) != m_inrangeSpellEffects.end());
	if (o->IsTimedObject())
		return (m_inrangeTimedObjects.find(TO_TIMEDOBJECT(o)) != m_inrangeTimedObjects.end());
	return false;
}

bool Object::CreatePath( int32 destx, int32 desty )
{
	if (destx == m_pathx && desty == m_pathy)
		return true;

	//update movement vectors
	if (IsUnit())
	{
		TO_UNIT(this)->UpdateMovement();
		TO_UNIT(this)->m_endmove = 0;

		if (!TO_UNIT(this)->CanMove())
			return true;
	}

	bool p;
	m_pathdata.clear();

	if (IsUnit() && TO_UNIT(this)->HasUnitFlag(UNIT_FLAG_NO_PATHFINDING))
	{
		PathPoint po;
		po.x = destx;
		po.y = desty;
		po.prevarrivetime = g_mstime;
		po.arrivetime = g_mstime + (float(GetDistance(destx, desty)) / TO_UNIT(this)->m_speed * 100);
		m_pathdata.push_back(po);
		p = true;
	}
	else
	{
		uint32 nextmove = g_mstime;
		if (TO_UNIT(this)->m_nextmove > g_mstime)
			nextmove = TO_UNIT(this)->m_nextmove;
		p = m_mapmgr->CreatePath(m_positionX, m_positionY, destx, desty, &m_pathdata, IsUnit()? TO_UNIT(this)->m_speed : 1, nextmove, IsUnit()? TO_UNIT(this)->GetPassFlags() : PASS_FLAG_WALK);
	}

	if (p)
	{
		m_pathpoint = 0;
		m_pathx = destx;
		m_pathy = desty;

		if (IsUnit() && m_pathdata.size() > 0)
			TO_UNIT(this)->MoveToNextPathPoint();
	}
	else
	{
		m_pathx = -1;
		m_pathy = -1;
	}

	return p;
}

bool Object::CanSee( Object* o )
{
	if (o->IsCreature() && TO_CREATURE(o)->m_invis)
		return false;
	return true;
}

void Object::AddInRangeItemGroup( ItemGroup* i )
{
	m_inrangeItemGroups.insert(i);
	i->m_inrangeObjects.insert(this);
}

void Object::RemoveInRangeItemGroup( ItemGroup* i )
{
	m_inrangeItemGroups.erase(i);
	i->m_inrangeObjects.erase(this);
}

bool Object::IsPet()
{
	return (IsCreature() && TO_CREATURE(this)->m_ispet);
}

bool Object::IsInFront( Object* o )
{
	double x = o->m_positionX - m_positionX;
	double y = o->m_positionY - m_positionY;

	double angle = atan2(y, x);
	angle = (angle >= 0.0) ? angle : 2.0 * PI + angle;
	angle -= m_rotation * PI / 180.0f;

	while(angle > PI)
		angle -= 2.0 * PI;

	while(angle < -PI)
		angle += 2.0 * PI;

	// replace M_PI in the two lines below to reduce or increase angle

	double left = -1.0 * (PI / 2.0);
	double right = (PI / 2.0);

	return (angle >= left && angle <= right);
}

void Object::CreatePortPop()
{
	if (IsPlayer() && TO_PLAYER(this)->m_invis)
		return;
	SpellEffect* eff = new SpellEffect(this, SPELL_TYPE_OBJECT, m_positionX, m_positionY, 0, NULL);
	eff->m_visualId = 200;
	m_mapmgr->AddObject(eff);
	eff->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 3000, 1, EVENT_FLAG_NOT_IN_WORLD);
}

void Object::OnPreRemoveFromWorld()
{
	for (std::set<uint16>::iterator itr = m_summonedObjects.begin(); itr != m_summonedObjects.end(); ++itr)
	{
		Object* obj = m_mapmgr->GetObject(*itr);
		if (obj != NULL)
			obj->AddEvent(&Object::Delete, EVENT_OBJECT_DELETE, 0, 1, EVENT_FLAG_NOT_IN_WORLD);
	}
	m_summonedObjects.clear();
}

bool Object::InLineOfSight( Object* o, bool fortrans /*= false*/ )
{
	if (m_mapmgr == NULL)
		return false;
	return m_mapmgr->InLineOfSight(m_positionX, m_positionY, o->m_positionX, o->m_positionY, fortrans? PASS_FLAG_TELEPORT : IsUnit()? TO_UNIT(this)->GetPassFlags() : PASS_FLAG_WALK);
}

void Object::ProcTileScript(uint32 flags)
{	
	int32 tx = m_positionX / 20;
	int32 ty = m_positionY / 20;

	ProcTileScript(tx, ty, flags);
}


void Object::ProcTileScript(uint32 x, uint32 y, uint32 flags)
{
	if (m_mapmgr == NULL)
		return;

	if (x >= 3220 || y >= 3220)
		return;

	if (m_mapmgr->m_tilescripts[x][y].procflags & flags)
		m_mapmgr->m_tilescripts[x][y].script->Proc(this);
}
