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

#ifndef __ITEM_H
#define __ITEM_H

#define CURRENT_ITEM_VERSION 12

#pragma pack(push, 1)
struct ItemVisual
{
	uint32 id;
	char* name;
	uint32 somnixmalevisual;
	uint32 somnixfemalevisual;
	uint32 scallionvisual;
	uint32 lurkervisual;
	uint32 halforcvisual;
};

struct ItemSpell
{
	uint32 id;
	uint32 spellid[5];
	uint32 value[5];
};

struct ItemProto
{
	uint32 id;
	const char* name;
	uint32 modelid;
	uint32 type;
	uint32 type2;
	uint32 type3;
	uint32 basevalue; //weapon max damage or armor protection
	uint32 levelreq;
	uint32 strreq;
	uint32 intreq;
	uint32 wisreq;
	uint32 bonuses[10];
	uint32 bonusvalues[10];
	uint32 displayid;
	uint32 equiplimit;
	uint32 propflags;
	uint32 propflags2;
	float rarity;
};

#pragma pack(pop)

enum ItemDisabledType
{
	ITEM_NOT_DISABLED,
	ITEM_DISABLED_NORMAL,
	ITEM_DISABLED_DROPRANDOM,
	ITEM_DISABLED_BOSSDROP_TEMP,
};

enum ItemFlags
{
	ITEM_FLAG_LIMBO			= 0x0001, //do not load this item
	ITEM_FLAG_REINSERT		= 0x0002, //reinserting this item for some reason, either give it a new slot in inventory or try and reinsert it another time
};

enum ItemProps
{
	ITEM_PROP_DUMMY_GREEN_BORDER		= 0x0001,
	ITEM_PROP_1							= 0x0002,
	ITEM_PROP_2							= 0x0004,
	ITEM_PROP_3							= 0x0008,
	ITEM_PROP_4							= 0x0010,
	ITEM_PROP_5							= 0x0020,
	ITEM_PROP_6							= 0x0040,
	ITEM_PROP_7							= 0x0080,
	ITEM_PROP_HIDDEN_HEALTH				= 0x0100,
	ITEM_PROP_HIDDEN_DOOMBRINGER		= 0x0200,
	ITEM_PROP_HIDDEN_LEECHING			= 0x0400,
	ITEM_PROP_HIDDEN_ENERGISED			= 0x0800,
	ITEM_PROP_HIDDEN_ANIMUS				= 0x1000,
	ITEM_PROP_HIDDEN_CHAOTIC			= 0x2000,
	ITEM_PROP_NON_DROP					= 0x4000,
	ITEM_PROP_NON_TRADE					= 0x8000,
};

extern SQLStore<ItemVisual> sItemVisuals;
extern SQLStore<ItemSpell> sItemSpells;
extern SQLStore<ItemProto> sItemProtos;

enum ItemTypes
{
	ITEM_TYPE_ARMOR			= 0x41,
	ITEM_TYPE_BOW			= 0x42,
	ITEM_TYPE_STACKABLE		= 0x45,
	ITEM_TYPE_BLADES		= 0x46,
	ITEM_TYPE_USEABLE		= 0x49,
	ITEM_TYPE_THROWING		= 0x4D,
	ITEM_TYPE_POLE_ARM		= 0x50,
	ITEM_TYPE_ARROW			= 0x52,
	ITEM_TYPE_2H_WEAPON		= 0x54,
	ITEM_TYPE_1H_WEAPON		= 0x57
};

enum ArmorTypes
{
	ARMOR_HELMET	= 1,
	ARMOR_BODY		= 2,
	ARMOR_SHIELD	= 3,
	ARMOR_GLOVES	= 4,
	ARMOR_MISC		= 5, //things like quest items
	ARMOR_EXTRA		= 6, //LSAs, redos, max ammys, str ammys
	ARMOR_RING		= 7,
};

enum WeaponType
{
	WEAPON_TYPE_UNKNOWN,
	WEAPON_TYPE_DAGGER,
	WEAPON_TYPE_POLE_ARM,
	WEAPON_TYPE_SWORD,
	WEAPON_TYPE_SHORT_SWORD,
	WEAPON_TYPE_BLUNT,
	WEAPON_TYPE_AXE,
	WEAPON_TYPE_BLUNT_2H,
	WEAPON_TYPE_SWORD_2H,
	WEAPON_TYPE_AXE_2H,
	WEAPON_TYPE_BOW,
};

enum ItemType3
{
	ITEM_TYPE_PROJECTILE = 12,
};

enum ItemSlots
{
	ITEM_SLOT_WEAPON1,
	ITEM_SLOT_WEAPON2,
	ITEM_SLOT_HEAD,
	ITEM_SLOT_BODY,
	ITEM_SLOT_SHIELD,
	ITEM_SLOT_GLOVES,
	ITEM_SLOT_RING1,
	ITEM_SLOT_RING2,
	ITEM_SLOT_RING3,
	ITEM_SLOT_RING4,
	ITEM_SLOT_RING5,
	ITEM_SLOT_RING6,
	ITEM_SLOT_AMULET1,
	ITEM_SLOT_AMULET2,
	ITEM_SLOT_AMULET3,
	ITEM_SLOT_INVENTORY,
};

enum ItemClassMask
{
	ITEM_CLASS_FIGHTER		= 0x01, //also barbarians I THINK
	ITEM_CLASS_RANGER		= 0x02,
	ITEM_CLASS_PALADIN		= 0x04,
	ITEM_CLASS_CLERIC		= 0x08, //also druids
	ITEM_CLASS_WIZARD		= 0x10,
	ITEM_CLASS_WARLOCK		= 0x20,
	ITEM_CLASS_DARKWAR		= 0x40,
	ITEM_CLASS_ALL			= 0x7F, //duh adepts
};

enum ItemPropFlags
{
	PROP_FLAG_WAND1		= 0x02,
	PROP_FLAG_WAND2		= 0x04,
	PROP_FLAG_WEAPON	= 0x80,
};

#define HANDLE_BONUS(apply, bonus, val) if (apply) m_owner->m_bonuses[bonus] += val; else m_owner->m_bonuses[bonus] -= val;

class ItemStorage : public Singleton<ItemStorage>
{
public:
	unordered_map<uint32, ItemProto*> m_itemData;
	uint32 m_maxitemId;
	unordered_map<uint32, std::vector<uint32>> m_lootmap;
	unordered_map<uint32, uint32> m_bodyvisuals[NUM_RACES][NUM_GENDERS];
	unordered_map<std::string, uint32> m_mdjrefs;
	unordered_map<uint32, uint32> m_mdjtomdl;

	unordered_map<uint32, std::map<uint32, uint32>> m_npcequipmap;

	ItemStorage() { Load(); UpdateDBData(); LoadNPCEquip(); }

	std::vector<uint32>* GetLootTable(uint32 lootentry)
	{
		unordered_map<uint32, std::vector<uint32>>::iterator itr = m_lootmap.find(lootentry);
		if (itr != m_lootmap.end())
			return &itr->second;
		return NULL;
	}

	ItemProto* GetItem(uint32 id)
	{
		return sItemProtos.GetEntry(id);
// 		unordered_map<uint32, ItemProto*>::iterator itr = m_itemData.find(id);
// 		if (itr == m_itemData.end())
// 			return NULL;
// 		return itr->second;
	}

	void Load();

	void LoadLoot();

	void GetSlotForItem(ItemProto* it, uint32 & slot, uint32 & numslots);

	void GetSlotForItem(uint32 itemid, uint32 & slot, uint32 & numslots);

	bool IsWeapon(uint8 type)
	{
		switch (type)
		{
		case 66:
		case 70:
		case 77:
		case 80:
		case 82:
		case 84:
		case 87:
			return true;
		}

		return false;
	}

	bool IsStackable(uint8 type)
	{
		return (type == ITEM_TYPE_USEABLE || type == ITEM_TYPE_STACKABLE);
	}

	uint32 GetMaxItemID() { return sItemProtos.GetMaxEntry(); }

	uint8 ItemDisabled(uint32 itemid)
	{
		switch (itemid)
		{
		case 0: //nothing
		case 344: //ejs staff
		case 187: //black cloth
		case 308: //vos ring
			return ITEM_DISABLED_NORMAL; break;
		case 186: //moon robe
		case 105: //moon staff
		case 106: //sun staff
		case 107: //sword of skull
		case 108: //vis gladius
		case 252: //xencon helm
		case 255: //xencon staff
		case 400: //xencon shield
		case 401: //xencon sword
			return ITEM_DISABLED_BOSSDROP_TEMP; break;
		}
		return ITEM_NOT_DISABLED;
	}

	void CheckItemType(Item* it, bool & twohweaponbarb, bool & twohweapon, bool & shield);

	uint32 GetArmorVisual(Player* p, int32 forcedrace = -1, int32 forcedgender = -1);

	void LoadArmorVisuals();

	void loadcharactermodel(const char* model, uint32 modelid, double scalex, double scaley, double scalez)
	{
		unordered_map<std::string, uint32>::iterator itr = m_mdjrefs.find(model);

		if (itr != m_mdjrefs.end())
			return;

		m_mdjrefs.insert(std::make_pair(model, modelid));
	}

	void setmodelmdjref(uint32 mdj, uint32 mdl)
	{
		m_mdjtomdl.insert(std::make_pair(mdj, mdl));
	}

	void LoadMDJData();

	void UpdateDBData()
	{
		for (unordered_map<uint32, ItemProto*>::iterator itr = m_itemData.begin(); itr != m_itemData.end(); ++itr)
		{
			uint32 slot, numslots;
			uint32 isstackable = IsStackable(itr->second->type);
			uint32 isring;

			GetSlotForItem(itr->first, slot, numslots);

			if (slot == ITEM_SLOT_RING1)
				isring = 1;
			else
				isring = 0;

			sDatabase.Execute("replace into `item_data` VALUES (%u, \"%s\", %u, %u);", itr->first, itr->second->name, isstackable, isring);
		}
	}

	void LoadNPCEquip()
	{
		MYSQL_RES* resource = sDatabase.Query("select * from `npc_equipment`;");
		if (resource == NULL || mysql_num_rows(resource) == 0)
			return;
		for (MYSQL_ROW info = sDatabase.Fetch(resource); info != NULL; info = sDatabase.Fetch(resource))
		{
			uint32 npcid = atol(info[0]);
			uint32 itemid = atol(info[1]);
			uint32 props = atol(info[2]);

			AddNPCEquip(npcid, itemid, props);
		}
	}

	void AddNPCEquip(uint32 npcid, uint32 itemid, uint32 itemprops)
	{
		unordered_map<uint32, std::map<uint32, uint32>>::iterator itr = m_npcequipmap.find(npcid);

		if (itr == m_npcequipmap.end())
		{
			std::map<uint32, uint32> newmap;
			newmap.insert(std::make_pair(itemid, itemprops));
			m_npcequipmap.insert(std::make_pair(npcid, newmap));
		}
		else
			itr->second.insert(std::make_pair(itemid, itemprops));
	}

	std::map<uint32, uint32>* GetNPCEquip(uint32 npcid)
	{
		unordered_map<uint32, std::map<uint32, uint32>>::iterator itr = m_npcequipmap.find(npcid);
		if (itr == m_npcequipmap.end())
			return NULL;
		return &itr->second;
	}
};

#define sItemStorage ItemStorage::getSingleton()

class Item;
class ItemGroup
{
public:
	int32 m_cellx;
	int32 m_celly;
	uint32 m_key;
	GrowableArray<Item*> m_items;
	std::set<Object*> m_inrangeObjects;
	MapCell* m_cell;

	ItemGroup(int32 x, int32 y, MapCell* cell)
	{
		m_cellx = x;
		m_celly = y;
		m_key = x | (y << 16);
		m_cell = cell;
		cell->m_items.insert(std::make_pair(m_key, this));
	}

	~ItemGroup();

	void RemoveItem(Item* i);

	void AddItem(Item* i);
};

class Item : public EventableObject
{
public:
	uint64 m_itemguid;
	ItemProto* m_data;
	Unit* m_owner;
	ItemGroup* m_group;
	uint32 m_currentslot;
	uint32 m_stackcount;
	uint32 m_expiretime;
	bool bonusesapplied;
	MapMgr* m_mapmgr;
	bool bonusabilityapplied;
	uint32 m_itemversion;
	uint32 m_itemflags;
	bool m_nodrop;

	int32 GetInstanceID()
	{
		if (m_mapmgr != NULL)
			return m_mapmgr->GetInstanceID();
		if (m_owner != NULL)
			return m_owner->GetInstanceID();
		return -1;
	}

	Item(uint32 itemid, Unit* owner, int32 forcedstack = -1);

	~Item();

	void Drop(int32 amount = -1);

	void RemoveFromInventory();

	void ApplyBonuses();

	void HandleBonuses(bool apply);

	void RemoveBonuses();

	bool Equip(int32 forcedslot = -1);

	bool IsForMyClass(Player* plr) { return (plr->GetItemClass() & m_data->propflags) != 0; }

	void ApplyItemVisual(uint32 slot);
	void ApplyItemVisual() { ApplyItemVisual(m_currentslot); }
	void UnEquip(bool forced = false);

	void RemoveItemVisual(uint32 curslot);
	void RemoveItemVisual() { RemoveItemVisual(m_currentslot); }
	bool AddToInventory(int32 forcedslot = -1);
	void MoveItem(uint32 numslot, uint32 numtomove);

	void Delete();

	void OnUse()
	{
		if (!sItemStorage.IsStackable(m_data->type))
		{
			Delete();
			return;
		}
		if (m_stackcount <= 1)
			Delete();
		else
			--m_stackcount;
	}

	bool CheckRequirements()
	{
		if (m_owner == NULL || !m_owner->IsPlayer())
			return false;
		Player* p = TO_PLAYER(m_owner);
		bool passed = true;
		if (p->m_class != CLASS_ADEPT && m_data->levelreq && p->m_level < m_data->levelreq)
			return false;
		if (m_data->strreq && p->m_strength < m_data->strreq)
			return false;
		if (m_data->intreq && p->m_intellect < m_data->intreq)
			return false;
		if (m_data->wisreq && p->m_wisdom < m_data->wisreq)
			return false;
		return true;
	}

	void ApplyProperties();

	void HandleBodyProperties(bool apply);
	void HandleHelmProperties(bool apply);
	void HandleGloveProperties(bool apply);
	void HandleWeaponProperties(bool apply);
	void HandleSpellWeaponProperties(bool apply);
	void HandleMeleeWeaponProperties(bool apply);
	void HandleBowWeaponProperties( bool apply );
	void HandleRingProperties(bool apply);
	void HandleShieldProperties(bool apply);
};

#endif
