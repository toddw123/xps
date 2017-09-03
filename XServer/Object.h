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

#ifndef __OBJECT_H
#define __OBJECT_H

enum PathResult
{
	PATH_RESULT_FAIL,
	PATH_RESULT_OK,
	PATH_RESULT_PATH_IS_SAME,
};

enum UnitType
{
	TYPEID_OBJECT,
	TYPEID_UNIT,
	TYPEID_CREATURE,
	TYPEID_PLAYER,
	TYPEID_SPELLEFFECT,
	TYPEID_ITEM,
	TYPEID_TIMEDOBJECT,
	NUM_TYPES,
};

//for my lazy ass
#define TO_UNIT(o) ((Unit*)o)
#define TO_ITEM(o) ((Item*)o)
#define TO_PLAYER(o) ((Player*)o)
#define TO_CREATURE(o) ((Creature*)o)
#define TO_SPELLEFFECT(o) ((SpellEffect*)o)
#define TO_TIMEDOBJECT(o) ((TimedObject*)o)

class SpellEffect;
class ItemGroup;
class TimedObject;
class Object : public EventableObject
{
public:
	float m_rotation;
	float m_bounding;
	//Keep projectile ids local to each client, we can only have 256 so we have to do this :S
	//The cell the player is actually in.
	MapMgr* m_mapmgr;
	MapCell* m_cell;
	//The cells surrounding the cell the player is in.
	uint32 m_pathpoint;
	int32 m_pathx;
	int32 m_pathy;
	std::vector<MapCell*> m_cellVector;
	std::map<SpellEffect*, uint8> m_inrangeSpellEffects;
	std::set<ItemGroup*> m_inrangeItemGroups;
	std::set<TimedObject*> m_inrangeTimedObjects;
	std::map<Unit*, uint8> m_inrangeUnits;
	std::vector<PathPoint> m_pathdata;
	std::set<uint16> m_summonedObjects;
	uint16 m_positionX;
	uint16 m_positionY;
	uint16 m_serverId;
	uint8 m_typeId;
	uint8 m_projectilidcounter;
	bool m_deleteonremove;
	bool m_active;
	bool m_inworld;
	uint32 m_lastmove;

	Object();
	~Object()
	{
		RemoveEvents();
	}

	virtual void Update() { }

	virtual void CollideCheck() {}

	INLINED uint8 ConvertRotation(float rot) { return uint8(-float(rot / 360 * 256)); }
	INLINED float ConvertRotation(uint8 rot)
	{
		float f = float(rot) / 256 * 360;
		return -f;
	}

	INLINED bool IsPlayer() { return (m_typeId == TYPEID_PLAYER); }
	INLINED bool IsUnit() { return (m_typeId == TYPEID_PLAYER || m_typeId == TYPEID_UNIT || m_typeId == TYPEID_CREATURE); }
	INLINED bool IsCreature() { return (m_typeId == TYPEID_CREATURE); }
	bool IsPet();
	INLINED bool IsSpellEffect() { return (m_typeId == TYPEID_SPELLEFFECT); }
	INLINED bool IsItem() { return (m_typeId == TYPEID_ITEM); }
	INLINED bool IsTimedObject() { return m_typeId == TYPEID_TIMEDOBJECT; }

	void AddInRangeObject(Object* o);
	void RemoveInRangeObject(Object* o);
	virtual void OnAddInRangeObject(Object* o) {}
	virtual void OnRemoveInRangeObject(Object* o) {}
	void UpdateInRangeSet();

	bool IsInRange(Object* o);

	float GetDistance(Object* o);
	float GetDistance(uint16 x, uint16 y);
	float GetDistance(uint16 x, uint16 y, uint16 x2, uint16 y2);

	bool IsInRectRange(Object* o)
	{
		return IsInRectRange(o->m_positionX, o->m_positionY);
	}

	bool IsInRectRange(int32 x, int32 y)
	{
		int32 dx = x - m_positionX;
		int32 dy = y - m_positionY;
		return (abs(dx) <= SIGHT_RANGE_X && abs(dy) <= SIGHT_RANGE_Y);
	}

	void Delete();

	float CalcFacing(int32 x, int32 y, int32 x2, int32 y2)
	{
		float dx = x2 - x;
		float dy = y2 - y;

		float distance = sqrt((float)((dx * dx) + (dy * dy)));

		float angle = 0.0f;

		if (dx == 0)
		{
			if (dy == 0)
				angle = 0.0;
			else if (dy > 0.0)
				angle = float(PI) / 2;
			else
				angle = float(PI) * 3.0f / 2;
		}
		else if (dy == 0)
		{
			if (dx > 0.0)
				angle = 0.0;
			else
				angle = float(PI);
		}
		else
		{
			if (dx < 0.0)
				angle = atanf(dy/dx)+PI;
			else if (dy < 0.0)
				angle = atanf(dy/dx)+(2*PI);
			else
				angle = atanf(dy/dx);
		}

		return angle;
	}

	float CalcFacing(int32 x, int32 y) { return CalcFacing(m_positionX, m_positionY, x, y); }
	float CalcFacingDeg(int32 x, int32 y)
	{
		float f = CalcFacing(x, y);
		return (f * float(180/PI));
	}

	float CalcFacingDeg(int32 x, int32 y, int32 x2, int32 y2)
	{
		float f = CalcFacing(x, y, x2, y2);
		return (f * float(180/PI));
	}

	int32 GetInstanceID() { if (m_mapmgr == NULL) return -1; return m_mapmgr->GetInstanceID(); }

	void SetPosition(int32 x, int32 y);

	virtual bool CreatePath(int32 destx, int32 desty);

	virtual void OnInactive() {}
	virtual void OnActive() {}
	bool CanSee(Object* o);

	void AddInRangeItemGroup(ItemGroup* i);

	void RemoveInRangeItemGroup(ItemGroup* i);

	virtual void OnAddToWorld() {}
	virtual void OnPreRemoveFromWorld();
	virtual void OnRemoveFromWorld() {}

	bool IsInFront(Object* o);

	bool InLineOfSight(Object* o, bool fortrans = false);

	void CreatePortPop();

	void ProcTileScript(uint32 flags);
	void ProcTileScript(uint32 x, uint32 y, uint32 flags);
};

#endif
