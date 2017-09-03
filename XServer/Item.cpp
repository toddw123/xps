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

SQLStore<ItemVisual> sItemVisuals;
SQLStore<ItemSpell> sItemSpells;
SQLStore<ItemProto> sItemProtos;

initialiseSingleton(ItemStorage);

void ItemStorage::Load()
{
	sItemProtos.Load("item_protos");

	if (sItemProtos.GetMaxEntry() == 0) //don't load this if we're using SQL table
	{
		FILE* f = fopen("encit.dat", "rb");
		if (f == NULL)
			return;

		uint32 itemsize = 0xDC;

		GrowableArray<uint8> tmpbuf;

		Crypto c;

		uint32 numvar1;
		fread(&numvar1, 4, 1, f);

		for (uint32 i = 0; i < numvar1 + 1; ++i)
		{
			uint32 tmp;
			fread(&tmp, 4, 1, f);
		}

		uint32 numvar2;
		fread(&numvar2, 4, 1, f);

		for (uint32 i = 0; i < numvar2 + 1; ++i)
		{
			uint32 tmp;
			fread(&tmp, 4, 1, f);
		}

		uint32 numitems;
		fread(&numitems, 4, 1, f);

		uint8 b;

		for (uint32 i = 0; i < numitems * itemsize; ++i)
		{
			fread(&b, 1, 1, f);
			tmpbuf.push(b);
		}

		for (uint32 i = 0; i < numitems; ++i)
		{
			uint8* buffer = &tmpbuf[i * itemsize];
			c.Decrypt(buffer, itemsize, i);
		}

		uint32 helmetcounter = 1;
		uint32 shieldcounter = 1;

#define GET(type, offset) *(type*)&tmpbuf[i * itemsize + offset]
#define GETSTR(offset) (char*)&tmpbuf[i * itemsize + offset]


		for (uint32 i = 0; i < numitems; ++i)
		{
			ItemProto* p = new ItemProto;
			memset(p, 0, sizeof(ItemProto));
			p->id = i;
			p->name = strdup(GETSTR(30));

			//website wants to know item names
			sDatabase.Execute("replace into `item_names` VALUES (%u, \"%s\");", i, p->name);

			p->modelid = GET(uint8, 2);
			p->type = GET(uint8, 0);
			p->levelreq = GET(uint8, 64);
			p->propflags = GET(uint8, 12);
			p->propflags2 = GET(uint8, 16);

			bool weapon = false;
			switch (p->type)
			{
			case 66:
			case 70:
			case 77:
			case 80:
			case 82:
			case 84:
			case 87:
				weapon = true; break;
			}

			p->type3 = GET(uint8, 8);

			if (weapon)
			{
				p->basevalue = GET(uint8, 4);
				p->type2 = GET(uint8, 6);
				p->displayid = i;
			}
			if (p->type == 65 || p->type == 69) //armor
			{
				p->type2 = GET(uint8, 8);
				if (p->type2 != ARMOR_MISC)
					p->basevalue = GET(uint8, 4);
				if (p->type2 == ARMOR_HELMET)
					p->displayid = helmetcounter++;
				if (p->type2 == ARMOR_SHIELD)
					p->displayid = shieldcounter++;
			}

			p->equiplimit = GET(uint8, 73);
			p->strreq = GET(uint8, 65);
			p->intreq = GET(uint8, 67);
			p->wisreq = GET(uint8, 68);
			p->rarity = GET(float, 56);

			//bonuses
			for (uint32 j = 0; j < 10; ++j)
			{
				p->bonuses[j] = GET(uint8, 77 + j);
				p->bonusvalues[j] = GET(uint8, 87 + j);
			}

			m_itemData.insert(std::make_pair(i, p));
			m_maxitemId = i;

			//if shield, warlocks and wizards can't use anymore
			uint32 slotbase, numslots;
			GetSlotForItem(p, slotbase, numslots);

			if (slotbase == ITEM_SLOT_SHIELD)
				p->propflags &= ~(ITEM_CLASS_WARLOCK | ITEM_CLASS_WIZARD);

			sItemProtos.Insert(p->id, p);
			sItemProtos.Save(p);
		}

#undef GET
#undef GETSTR
	}

	LoadLoot();

	LoadMDJData();
	LoadArmorVisuals();
}

void ItemStorage::GetSlotForItem( uint32 itemid, uint32 & slot, uint32 & numslots )
{
	ItemProto* it = GetItem(itemid);
	if (it == NULL)
	{
		slot = 0xFFFFFFFF;
		numslots = 0;
	}
	else
		GetSlotForItem(it, slot, numslots);
}

void ItemStorage::GetSlotForItem( ItemProto* it, uint32 & slot, uint32 & numslots )
{
	slot = 0xFFFFFFFF;
	numslots = 0;

	if (it->id == 220)
	{
		slot = ITEM_SLOT_AMULET1;
		numslots = 3;
		return;
	}

	if (IsWeapon(it->type))
	{
		slot = ITEM_SLOT_WEAPON1;
		numslots = 2;
		return;
	}

	if (it->type == 65 || it->type == 69) //armor
	{
		switch (it->type2)
		{
		case ARMOR_SHIELD: slot = ITEM_SLOT_SHIELD; numslots = 1; return;
		case ARMOR_HELMET: slot = ITEM_SLOT_HEAD; numslots = 1; return;
		case ARMOR_GLOVES: slot = ITEM_SLOT_GLOVES; numslots = 1; return;
		case ARMOR_RING: slot = ITEM_SLOT_RING1; numslots = 6; return;
		case ARMOR_BODY: slot = ITEM_SLOT_BODY; numslots = 1; return;
		case ARMOR_EXTRA: slot = ITEM_SLOT_AMULET1; numslots = 3; return;
		}
	}
}

void ItemStorage::LoadLoot()
{
	//load loot
	MYSQL_RES* res = sDatabase.Query("select * from `npc_loot`;");
	if (res != NULL && mysql_num_rows(res) != 0)
	{
		MYSQL_ROW row = sDatabase.Fetch(res);
		while (row != NULL)
		{
			uint32 entry = atol(row[0]);
			uint32 itemid = atol(row[1]);

			unordered_map<uint32, std::vector<uint32>>::iterator itr = m_lootmap.find(entry);

			if (itr != m_lootmap.end())
				itr->second.push_back(itemid);
			else
			{
				std::vector<uint32> tmpvec;
				tmpvec.push_back(itemid);
				m_lootmap.insert(std::make_pair(entry, tmpvec));
			}

			row = sDatabase.Fetch(res);
		}
	}
	sDatabase.Free(res);
}

void ItemStorage::CheckItemType( Item* it, bool & twohweaponbarb, bool & twohweapon, bool & shield )
{
	if (it == NULL)
		return;
	switch (it->m_data->type)
	{
	case ITEM_TYPE_2H_WEAPON:
	case ITEM_TYPE_POLE_ARM:
		twohweaponbarb = true;
		break;
	case ITEM_TYPE_BOW:
	case ITEM_TYPE_BLADES:
		twohweapon = true;
	case ITEM_TYPE_ARMOR:
		{
			if (it->m_data->type2 == ARMOR_SHIELD)
				shield = true;
		}
		break;
	}
}

uint32 ItemStorage::GetArmorVisual( Player* p, int32 forcedrace /*= -1*/, int32 forcedgender /*= -1*/ )
{
	if (p->m_invis)
		return 250;

	uint32 race = forcedrace == -1? p->m_race : forcedrace;
	uint32 gender = forcedgender == -1? p->m_gender : forcedgender;
	uint32 armorid = 0;

	Item* armor = p->GetItem(ITEM_SLOT_BODY);

	if (armor != NULL)
		armorid = armor->m_data->id;

	//few hacks here
	if (race == RACE_HUMAN && (p->m_class == CLASS_WIZARD || p->m_class == CLASS_WARLOCK || p->m_class == CLASS_WITCH || p->m_class == CLASS_ADEPT))
	{
		if (forcedgender != -1 && gender == GENDER_MALE && armorid == 0 && p->GetItem(ITEM_SLOT_HEAD) != NULL)
			armorid = 109; //leather armor, forces hood down

		if (forcedgender == -1 && (armor == NULL || armor->m_data->propflags & ITEM_CLASS_WIZARD))
		{
			if (gender == GENDER_MALE)
				gender = GENDER_OTHER; //other holds the wizard stuff
			else
				gender = GENDER_DUMMY_FEMALEMAGE;
		}
	}

	if (race >= NUM_RACES || gender >= NUM_GENDERS)
		return 0;

	unordered_map<uint32, uint32>::iterator itr = m_bodyvisuals[race][gender].find(armorid);

	if (itr == m_bodyvisuals[race][gender].end())
	{
		//try and return normal ones
		if (gender == GENDER_OTHER)
			return GetArmorVisual(p, race, GENDER_MALE);
		if (gender == GENDER_DUMMY_FEMALEMAGE)
			return GetArmorVisual(p, race, GENDER_FEMALE);
		if (gender == GENDER_FEMALE)
			return GetArmorVisual(p, race, GENDER_MALE);
		return 0;
	}
	return itr->second;
}

void ItemStorage::LoadArmorVisuals()
{
	std::string modelprefixes[NUM_RACES][NUM_GENDERS];

	MYSQL_RES* res = sDatabase.Query("select * from `item_visual_races`;");
	if (res != NULL && mysql_num_rows(res) != 0)
	{
		MYSQL_ROW row = sDatabase.Fetch(res);
		while (row != NULL)
		{
			uint32 r = 1;
			uint32 ra = atol(row[r++]);
			uint32 ge = atol(row[r++]);
			const char* mdl = row[r++];

			modelprefixes[ra][ge] = mdl;

			row = sDatabase.Fetch(res);
		}
	}
	sDatabase.Free(res);


	res = sDatabase.Query("select * from `item_visual_items`;");
	if (res != NULL && mysql_num_rows(res) != 0)
	{
		MYSQL_ROW row = sDatabase.Fetch(res);
		while (row != NULL)
		{
			uint32 r = 0;
			uint32 itemid = atol(row[r++]);
			++r; //ignore item name;			

			//now we do shit
			for (uint32 rac = 0; rac < NUM_RACES; ++rac)
			{
				for (uint32 gen = 0; gen < NUM_GENDERS; ++gen)
				{
					std::string postfix = row[r];
					if (modelprefixes[rac][gen].size() == 0)
						continue; //no model
					std::string prefx = modelprefixes[rac][gen];

retrymdlget:


					if (prefx == "wizard")
					{
						if (postfix == "l") //wizard leather is wizard2
							postfix = "2";
						if (postfix == "nl") //wizard leather is wizard2
							postfix = "2";
					}

					if (prefx == "femmag")
					{
						if (postfix == "l") //wizard leather is wizard2
							postfix = "N/A";
						if (postfix == "nl") //wizard leather is wizard2
							postfix = "N/A";

						//heres a fucking stupid one, itemid 0 for femmag is wizard
						if (itemid == 0)
							prefx = "wizard";
					}

					std::string modelstring = prefx;
					if (postfix != "N/A")
						modelstring += postfix;
					modelstring += ".mdj";

					unordered_map<std::string, uint32>::iterator itr = m_mdjrefs.find(modelstring);

					if (itr == m_mdjrefs.end())
					{
						sLog.Debug("MODEL", "Cannot find model for race %u gender %u (model %s)", rac, gen, modelstring.c_str());

						bool goback = true;

						//try and move down chain
						if (postfix == "rl") postfix = "ll";
						else if (postfix == "ll") postfix = "nl";
						else if (postfix == "nl") postfix = "l";
						else if (postfix == "l") goback = false;
						else if (postfix == "nc") postfix = "c";
						else if (postfix == "c") goback = false;
						else if (postfix == "RC") prefx = "dreg";
						else if (postfix == "ar") prefx = "dood";
						else if (postfix == "mr") prefx = "dood";
						else if (postfix == "bc") prefx = "dood";
						else goback = false;

						if (goback)
							goto retrymdlget;
					}
					else
					{
						unordered_map<uint32, uint32>::iterator itr2 = m_mdjtomdl.find(itr->second);

						if (itr2 != m_mdjtomdl.end())
							m_bodyvisuals[rac][gen].insert(std::make_pair(itemid, itr2->second));
					}
				}
			}


			row = sDatabase.Fetch(res);
		}
	}
	sDatabase.Free(res);
}

void ItemStorage::LoadMDJData()
{
	//COPIED FROM CLIENT
	loadcharactermodel("dood.mdj", 0, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femmag.mdj", 1, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("bauul.mdj", 2, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("tdbug.mdj", 3, 0.02999999932944775, 0.02999999932944775, 0.02999999932944775);
	loadcharactermodel("fem.mdj", 4, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("troll.mdj", 5, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("wizard.mdj", 6, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("spirit.mdj", 7, 0.119999997317791, 0.1000000014901161, 0.119999997317791);
	loadcharactermodel("skeleton.mdj", 8, 0.1000000014901161, 0.07999999821186066, 0.1000000014901161);
	loadcharactermodel("spider.mdj", 9, 0.07000000029802322, 0.07000000029802322, 0.07000000029802322);
	loadcharactermodel("chicken.mdj", 10, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dreg.mdj", 11, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("ldemon.mdj", 12, 0.09000000357627869, 0.07999999821186066, 0.09000000357627869);
	loadcharactermodel("ogre.mdj", 13, 0.119999997317791, 0.119999997317791, 0.119999997317791);
	loadcharactermodel("calva.mdj", 14, 0.1099999994039536, 0.09000000357627869, 0.1099999994039536);
	loadcharactermodel("scall.mdj", 15, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("hhound.mdj", 16, 0.119999997317791, 0.119999997317791, 0.119999997317791);
	loadcharactermodel("dragon.mdj", 17, 0.1500000059604645, 0.1500000059604645, 0.1500000059604645);
	loadcharactermodel("gnome.mdj", 18, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sgolum.mdj", 19, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("hawkd.mdj", 20, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("fireball.mdj", 21, 0.05000000074505806, 0.05000000074505806, 0.05000000074505806);
	loadcharactermodel("wizard2.mdj", 22, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodc.mdj", 23, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femc.mdj", 24, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregc.mdj", 25, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallc.mdj", 26, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomec.mdj", 27, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodp.mdj", 28, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femp.mdj", 29, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregp.mdj", 30, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallp.mdj", 31, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomep.mdj", 32, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("tarman.mdj", 33, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("spirit2.mdj", 34, 0.119999997317791, 0.1000000014901161, 0.119999997317791);
	loadcharactermodel("behemoth.mdj", 35, 0.1299999952316284, 0.1000000014901161, 0.1299999952316284);
	loadcharactermodel("wlf.mdj", 36, 0.119999997317791, 0.119999997317791, 0.119999997317791);
	loadcharactermodel("scorp.mdj", 37, 0.119999997317791, 0.119999997317791, 0.119999997317791);
	loadcharactermodel("spid2.mdj", 38, 0.1500000059604645, 0.1500000059604645, 0.1500000059604645);
	loadcharactermodel("hknt.mdj", 39, 0.1099999994039536, 0.1099999994039536, 0.1099999994039536);
	loadcharactermodel("doodrl.mdj", 40, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("feml.mdj", 41, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregl.mdj", 42, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scalll.mdj", 43, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomel.mdj", 44, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femmagll.mdj", 45, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("wizardll.mdj", 46, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodml.mdj", 47, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femml.mdj", 48, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregml.mdj", 49, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallml.mdj", 50, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomeml.mdj", 51, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femmagml.mdj", 52, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("wizardml.mdj", 53, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodsl.mdj", 54, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femsl.mdj", 55, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregsl.mdj", 56, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallsl.mdj", 57, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomesl.mdj", 58, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femmagsl.mdj", 59, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("wizardsl.mdj", 60, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodmp.mdj", 61, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femmp.mdj", 62, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregmp.mdj", 63, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallmp.mdj", 64, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomemp.mdj", 65, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodxl.mdj", 66, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femxl.mdj", 67, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregxl.mdj", 68, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallxl.mdj", 69, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomexl.mdj", 70, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femmagxl.mdj", 71, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("wizardxl.mdj", 72, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodsp.mdj", 73, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femsp.mdj", 74, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregsp.mdj", 75, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallsp.mdj", 76, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomesp.mdj", 77, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dooddr.mdj", 78, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femdr.mdj", 79, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregdr.mdj", 80, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scalldr.mdj", 81, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomedr.mdj", 82, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femmagdr.mdj", 83, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("wizarddr.mdj", 84, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("shadow.mdj", 85, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("imp.mdj", 86, 0.05000000074505806, 0.05000000074505806, 0.05000000074505806);
	loadcharactermodel("doodop.mdj", 87, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femop.mdj", 88, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregop.mdj", 89, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallop.mdj", 90, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomeop.mdj", 91, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodmr.mdj", 92, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregmr.mdj", 93, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallmr.mdj", 94, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomemr.mdj", 95, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodbc.mdj", 96, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregbc.mdj", 97, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallbc.mdj", 98, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomebc.mdj", 99, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodar.mdj", 100, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregar.mdj", 101, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallar.mdj", 102, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomear.mdj", 103, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dooddp.mdj", 104, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femdp.mdj", 105, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregdp.mdj", 106, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scalldp.mdj", 107, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomedp.mdj", 108, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("mummy.mdj", 109, 0.1500000059604645, 0.1000000014901161, 0.1500000059604645);
	loadcharactermodel("undead.mdj", 110, 0.119999997317791, 0.1000000014901161, 0.119999997317791);
	loadcharactermodel("malerobed.mdj", 111, 0.119999997317791, 0.1000000014901161, 0.119999997317791);
	loadcharactermodel("maleold.mdj", 112, 0.119999997317791, 0.1000000014901161, 0.119999997317791);
	loadcharactermodel("fempurp.mdj", 113, 0.1500000059604645, 0.1000000014901161, 0.1500000059604645);
	loadcharactermodel("orc.mdj", 114, 0.119999997317791, 0.1000000014901161, 0.119999997317791);
	loadcharactermodel("demonx.mdj", 115, 0.119999997317791, 0.1000000014901161, 0.119999997317791);
	loadcharactermodel("mino.mdj", 116, 0.119999997317791, 0.1000000014901161, 0.119999997317791);
	loadcharactermodel("doodbm.mdj", 117, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("fembm.mdj", 118, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregbm.mdj", 119, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallbm.mdj", 120, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomebm.mdj", 121, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femmagbm.mdj", 122, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("wizardbm.mdj", 123, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodsc.mdj", 124, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femsc.mdj", 125, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregsc.mdj", 126, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallsc.mdj", 127, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomesc.mdj", 128, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femmagsc.mdj", 129, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("wizardsc.mdj", 130, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("spider2.mdj", 131, 0.2000000029802322, 0.2000000029802322, 0.2000000029802322);
	loadcharactermodel("imp2.mdj", 132, 0.07000000029802322, 0.07000000029802322, 0.07000000029802322);
	loadcharactermodel("tarman2.mdj", 133, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodl.mdj", 134, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodnl.mdj", 135, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodnc.mdj", 136, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodnp.mdj", 137, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfem.mdj", 138, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfeml.mdj", 139, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemc.mdj", 140, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemp.mdj", 141, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemnl.mdj", 142, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemnc.mdj", 143, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemnp.mdj", 144, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemrl.mdj", 145, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemml.mdj", 146, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemsl.mdj", 147, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemmp.mdj", 148, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemxl.mdj", 149, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemsp.mdj", 150, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemdr.mdj", 151, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemop.mdj", 152, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregmr.mdj", 153, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregbc.mdj", 154, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregar.mdj", 155, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemdp.mdj", 156, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfembm.mdj", 157, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemsc.mdj", 158, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmale.mdj", 159, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalel.mdj", 160, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalec.mdj", 161, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalep.mdj", 162, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalenl.mdj", 163, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalenc.mdj", 164, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalenp.mdj", 165, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalerl.mdj", 166, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmaleml.mdj", 167, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalesl.mdj", 168, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalemp.mdj", 169, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalexl.mdj", 170, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalesp.mdj", 171, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmaledr.mdj", 172, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmaleop.mdj", 173, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodmr.mdj", 174, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodbc.mdj", 175, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodar.mdj", 176, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmaledp.mdj", 177, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalebm.mdj", 178, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmalesc.mdj", 179, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffem.mdj", 180, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffeml.mdj", 181, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemc.mdj", 182, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemp.mdj", 183, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemnl.mdj", 184, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemnc.mdj", 185, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemnp.mdj", 186, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemrl.mdj", 187, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemml.mdj", 188, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemsl.mdj", 189, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemmp.mdj", 190, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemxl.mdj", 191, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemsp.mdj", 192, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemdr.mdj", 193, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemop.mdj", 194, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodmr.mdj", 195, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodbc.mdj", 196, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodar.mdj", 197, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemdp.mdj", 198, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffembm.mdj", 199, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemsc.mdj", 200, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefem.mdj", 201, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefeml.mdj", 202, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemc.mdj", 203, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemp.mdj", 204, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemnl.mdj", 205, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemnc.mdj", 206, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemnp.mdj", 207, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemrl.mdj", 208, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemml.mdj", 209, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemsl.mdj", 210, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemmp.mdj", 211, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemxl.mdj", 212, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemsp.mdj", 213, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemdr.mdj", 214, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemop.mdj", 215, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomemr.mdj", 216, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomebc.mdj", 217, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomear.mdj", 218, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemdp.mdj", 219, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefembm.mdj", 220, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemsc.mdj", 221, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somm.mdj", 243, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somml.mdj", 244, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommc.mdj", 245, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommp.mdj", 246, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommnl.mdj", 247, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommnc.mdj", 248, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommnp.mdj", 249, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommrl.mdj", 250, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommml.mdj", 251, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommsl.mdj", 252, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommmp.mdj", 253, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommxl.mdj", 254, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommsp.mdj", 255, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommdr.mdj", 256, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommop.mdj", 257, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodmr.mdj", 258, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodbc.mdj", 259, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodar.mdj", 260, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommdp.mdj", 261, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommbm.mdj", 262, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommsc.mdj", 263, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somf.mdj", 264, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfl.mdj", 265, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfc.mdj", 266, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfp.mdj", 267, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfnl.mdj", 268, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfnc.mdj", 269, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfnp.mdj", 270, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfrl.mdj", 271, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfml.mdj", 272, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfsl.mdj", 273, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfmp.mdj", 274, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfxl.mdj", 275, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfsp.mdj", 276, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfdr.mdj", 277, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfop.mdj", 278, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodmr.mdj", 279, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodbc.mdj", 280, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodar.mdj", 281, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfdp.mdj", 282, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfbm.mdj", 283, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfsc.mdj", 284, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmale.mdj", 285, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalel.mdj", 286, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalec.mdj", 287, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalep.mdj", 288, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalenl.mdj", 289, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalenc.mdj", 290, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalenp.mdj", 291, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalerl.mdj", 292, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmaleml.mdj", 293, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalesl.mdj", 294, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalemp.mdj", 295, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalexl.mdj", 296, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalesp.mdj", 297, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmaledr.mdj", 298, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmaleop.mdj", 299, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodmr.mdj", 300, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodbc.mdj", 301, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodar.mdj", 302, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmaledp.mdj", 303, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalebm.mdj", 304, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmalesc.mdj", 305, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffem.mdj", 306, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffeml.mdj", 307, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemc.mdj", 308, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemp.mdj", 309, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemnl.mdj", 310, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemnc.mdj", 311, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemnp.mdj", 312, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemrl.mdj", 313, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemml.mdj", 314, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemsl.mdj", 315, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemmp.mdj", 316, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemxl.mdj", 317, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemsp.mdj", 318, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemdr.mdj", 319, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemop.mdj", 320, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodmr.mdj", 321, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodbc.mdj", 322, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodar.mdj", 323, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemdp.mdj", 324, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffembm.mdj", 325, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemsc.mdj", 326, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idget.mdj", 327, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetl.mdj", 328, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetc.mdj", 329, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetp.mdj", 330, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetnl.mdj", 331, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetnc.mdj", 332, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetnp.mdj", 333, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetrl.mdj", 334, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetml.mdj", 335, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetsl.mdj", 336, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetmp.mdj", 337, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetxl.mdj", 338, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetsp.mdj", 339, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetdr.mdj", 340, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetop.mdj", 341, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetmr.mdj", 342, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetbc.mdj", 343, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetar.mdj", 344, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetdp.mdj", 345, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetbm.mdj", 346, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetsc.mdj", 347, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforc.mdj", 348, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcl.mdj", 349, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcc.mdj", 350, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcp.mdj", 351, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcnl.mdj", 352, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcnc.mdj", 353, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcnp.mdj", 354, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcrl.mdj", 355, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcml.mdj", 356, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcsl.mdj", 357, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcmp.mdj", 358, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcxl.mdj", 359, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcsp.mdj", 360, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcdr.mdj", 361, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcop.mdj", 362, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcmr.mdj", 363, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcbc.mdj", 364, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcar.mdj", 365, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcdp.mdj", 366, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcbm.mdj", 367, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcsc.mdj", 368, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minion.mdj", 369, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionl.mdj", 370, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionc.mdj", 371, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionp.mdj", 372, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionnl.mdj", 373, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionnc.mdj", 374, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionnp.mdj", 375, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionrl.mdj", 376, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionml.mdj", 377, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionsl.mdj", 378, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionmp.mdj", 379, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionxl.mdj", 380, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionsp.mdj", 381, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("miniondr.mdj", 382, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionop.mdj", 383, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionmr.mdj", 384, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionbc.mdj", 385, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionar.mdj", 386, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("miniondp.mdj", 387, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionbm.mdj", 388, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionsc.mdj", 389, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("demonw.mdj", 390, 0.1500000059604645, 0.1500000059604645, 0.1500000059604645);
	loadcharactermodel("hknt2.mdj", 391, 0.119999997317791, 0.119999997317791, 0.119999997317791);
	loadcharactermodel("imp2.mdj", 392, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femnp.mdj", 393, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregnp.mdj", 394, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallnp.mdj", 395, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomenp.mdj", 396, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodRC.mdj", 397, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregRC.mdj", 398, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallRC.mdj", 399, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomeRC.mdj", 400, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetRC.mdj", 401, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcRC.mdj", 402, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionRC.mdj", 403, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("doodDL.mdj", 404, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femDL.mdj", 405, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregDL.mdj", 406, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("scallDL.mdj", 407, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomeDL.mdj", 408, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("femmagDL.mdj", 409, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("wizardDL.mdj", 410, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("dregfemDL.mdj", 411, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elfmaleDL.mdj", 412, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("elffemDL.mdj", 413, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("gnomefemDL.mdj", 414, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("sommDL.mdj", 415, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("somfDL.mdj", 416, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delfmaleDL.mdj", 417, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("delffemDL.mdj", 418, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("idgetDL.mdj", 419, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("halforcDL.mdj", 420, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);
	loadcharactermodel("minionDL.mdj", 421, 0.1000000014901161, 0.1000000014901161, 0.1000000014901161);

	//END COPIED FROM CLIENT

	setmodelmdjref(10, 0);
	setmodelmdjref(0, 1);
	setmodelmdjref(3, 2);
	setmodelmdjref(4, 3);
	setmodelmdjref(86, 4);
	setmodelmdjref(0, 5);
	setmodelmdjref(5, 6);
	setmodelmdjref(5, 7);
	setmodelmdjref(11, 8);
	setmodelmdjref(0, 9);
	setmodelmdjref(0, 10);
	setmodelmdjref(6, 11);
	setmodelmdjref(6, 12);
	setmodelmdjref(8, 13);
	setmodelmdjref(14, 14);
	setmodelmdjref(132, 15);
	setmodelmdjref(15, 16);
	setmodelmdjref(5, 17);
	setmodelmdjref(85, 18);
	setmodelmdjref(16, 19);
	setmodelmdjref(9, 20);
	setmodelmdjref(13, 21);
	setmodelmdjref(6, 22);
	setmodelmdjref(6, 23);
	setmodelmdjref(12, 24);
	setmodelmdjref(2, 25);
	setmodelmdjref(34, 26);
	setmodelmdjref(14, 27);
	setmodelmdjref(0, 28);
	setmodelmdjref(14, 29);
	setmodelmdjref(19, 30);
	setmodelmdjref(0, 31);
	setmodelmdjref(6, 32);
	setmodelmdjref(7, 33);
	setmodelmdjref(0, 34);
	setmodelmdjref(17, 35);
	setmodelmdjref(18, 36);
	setmodelmdjref(20, 37);
	setmodelmdjref(21, 38);
	setmodelmdjref(6, 39);
	setmodelmdjref(23, 40);
	setmodelmdjref(24, 41);
	setmodelmdjref(25, 42);
	setmodelmdjref(26, 43);
	setmodelmdjref(27, 44);
	setmodelmdjref(28, 45);
	setmodelmdjref(29, 46);
	setmodelmdjref(30, 47);
	setmodelmdjref(31, 48);
	setmodelmdjref(32, 49);
	setmodelmdjref(1, 50);
	setmodelmdjref(22, 51);
	setmodelmdjref(33, 52);
	setmodelmdjref(34, 53);
	setmodelmdjref(22, 54);
	setmodelmdjref(30, 55);
	setmodelmdjref(35, 56);
	setmodelmdjref(36, 57);
	setmodelmdjref(37, 58);
	setmodelmdjref(38, 59);
	setmodelmdjref(39, 60);
	setmodelmdjref(40, 61);
	setmodelmdjref(41, 62);
	setmodelmdjref(42, 63);
	setmodelmdjref(43, 64);
	setmodelmdjref(44, 65);
	setmodelmdjref(45, 66);
	setmodelmdjref(46, 67);
	setmodelmdjref(47, 68);
	setmodelmdjref(48, 69);
	setmodelmdjref(49, 70);
	setmodelmdjref(50, 71);
	setmodelmdjref(51, 72);
	setmodelmdjref(52, 73);
	setmodelmdjref(53, 74);
	setmodelmdjref(54, 75);
	setmodelmdjref(55, 76);
	setmodelmdjref(56, 77);
	setmodelmdjref(57, 78);
	setmodelmdjref(58, 79);
	setmodelmdjref(59, 80);
	setmodelmdjref(60, 81);
	setmodelmdjref(61, 82);
	setmodelmdjref(62, 83);
	setmodelmdjref(63, 84);
	setmodelmdjref(64, 85);
	setmodelmdjref(65, 86);
	setmodelmdjref(66, 87);
	setmodelmdjref(67, 88);
	setmodelmdjref(68, 89);
	setmodelmdjref(69, 90);
	setmodelmdjref(70, 91);
	setmodelmdjref(71, 92);
	setmodelmdjref(72, 93);
	setmodelmdjref(73, 94);
	setmodelmdjref(74, 95);
	setmodelmdjref(75, 96);
	setmodelmdjref(76, 97);
	setmodelmdjref(77, 98);
	setmodelmdjref(78, 99);
	setmodelmdjref(79, 100);
	setmodelmdjref(80, 101);
	setmodelmdjref(81, 102);
	setmodelmdjref(82, 103);
	setmodelmdjref(83, 104);
	setmodelmdjref(84, 105);
	setmodelmdjref(87, 106);
	setmodelmdjref(88, 107);
	setmodelmdjref(89, 108);
	setmodelmdjref(90, 109);
	setmodelmdjref(91, 110);
	setmodelmdjref(92, 111);
	setmodelmdjref(92, 112);
	setmodelmdjref(93, 113);
	setmodelmdjref(94, 114);
	setmodelmdjref(95, 115);
	setmodelmdjref(92, 116);
	setmodelmdjref(92, 117);
	setmodelmdjref(96, 118);
	setmodelmdjref(96, 119);
	setmodelmdjref(97, 120);
	setmodelmdjref(98, 121);
	setmodelmdjref(99, 122);
	setmodelmdjref(96, 123);
	setmodelmdjref(96, 124);
	setmodelmdjref(102, 125);
	setmodelmdjref(103, 126);
	setmodelmdjref(100, 127);
	setmodelmdjref(100, 128);
	setmodelmdjref(101, 129);
	setmodelmdjref(104, 130);
	setmodelmdjref(105, 131);
	setmodelmdjref(106, 132);
	setmodelmdjref(107, 133);
	setmodelmdjref(108, 134);
	setmodelmdjref(109, 135);
	setmodelmdjref(110, 136);
	setmodelmdjref(111, 137);
	setmodelmdjref(112, 138);
	setmodelmdjref(113, 139);
	setmodelmdjref(114, 140);
	setmodelmdjref(115, 141);
	setmodelmdjref(116, 142);
	setmodelmdjref(117, 143);
	setmodelmdjref(118, 144);
	setmodelmdjref(119, 145);
	setmodelmdjref(120, 146);
	setmodelmdjref(121, 147);
	setmodelmdjref(122, 148);
	setmodelmdjref(123, 149);
	setmodelmdjref(124, 150);
	setmodelmdjref(125, 151);
	setmodelmdjref(126, 152);
	setmodelmdjref(127, 153);
	setmodelmdjref(128, 154);
	setmodelmdjref(129, 155);
	setmodelmdjref(130, 156);
	setmodelmdjref(131, 157);
	setmodelmdjref(109, 158);
	setmodelmdjref(8, 159);
	setmodelmdjref(110, 160);
	setmodelmdjref(110, 161);
	setmodelmdjref(133, 162);
	setmodelmdjref(138, 163);
	setmodelmdjref(139, 164);
	setmodelmdjref(140, 165);
	setmodelmdjref(141, 166);
	setmodelmdjref(142, 167);
	setmodelmdjref(143, 168);
	setmodelmdjref(144, 169);
	setmodelmdjref(145, 170);
	setmodelmdjref(146, 171);
	setmodelmdjref(147, 172);
	setmodelmdjref(148, 173);
	setmodelmdjref(149, 174);
	setmodelmdjref(150, 175);
	setmodelmdjref(151, 176);
	setmodelmdjref(152, 177);
	setmodelmdjref(153, 178);
	setmodelmdjref(154, 179);
	setmodelmdjref(155, 180);
	setmodelmdjref(156, 181);
	setmodelmdjref(157, 182);
	setmodelmdjref(158, 183);
	setmodelmdjref(159, 184);
	setmodelmdjref(160, 185);
	setmodelmdjref(161, 186);
	setmodelmdjref(162, 187);
	setmodelmdjref(163, 188);
	setmodelmdjref(164, 189);
	setmodelmdjref(165, 190);
	setmodelmdjref(166, 191);
	setmodelmdjref(167, 192);
	setmodelmdjref(168, 193);
	setmodelmdjref(169, 194);
	setmodelmdjref(170, 195);
	setmodelmdjref(171, 196);
	setmodelmdjref(172, 197);
	setmodelmdjref(173, 198);
	setmodelmdjref(174, 199);
	setmodelmdjref(175, 200);
	setmodelmdjref(176, 201);
	setmodelmdjref(177, 202);
	setmodelmdjref(178, 203);
	setmodelmdjref(179, 204);
	setmodelmdjref(180, 205);
	setmodelmdjref(181, 206);
	setmodelmdjref(182, 207);
	setmodelmdjref(183, 208);
	setmodelmdjref(184, 209);
	setmodelmdjref(185, 210);
	setmodelmdjref(186, 211);
	setmodelmdjref(187, 212);
	setmodelmdjref(188, 213);
	setmodelmdjref(189, 214);
	setmodelmdjref(190, 215);
	setmodelmdjref(191, 216);
	setmodelmdjref(192, 217);
	setmodelmdjref(193, 218);
	setmodelmdjref(194, 219);
	setmodelmdjref(195, 220);
	setmodelmdjref(196, 221);
	setmodelmdjref(197, 222);
	setmodelmdjref(198, 223);
	setmodelmdjref(199, 224);
	setmodelmdjref(200, 225);
	setmodelmdjref(201, 226);
	setmodelmdjref(202, 227);
	setmodelmdjref(203, 228);
	setmodelmdjref(204, 229);
	setmodelmdjref(205, 230);
	setmodelmdjref(206, 231);
	setmodelmdjref(207, 232);
	setmodelmdjref(208, 233);
	setmodelmdjref(209, 234);
	setmodelmdjref(210, 235);
	setmodelmdjref(211, 236);
	setmodelmdjref(212, 237);
	setmodelmdjref(213, 238);
	setmodelmdjref(214, 239);
	setmodelmdjref(215, 240);
	setmodelmdjref(216, 241);
	setmodelmdjref(217, 242);
	setmodelmdjref(218, 243);
	setmodelmdjref(219, 244);
	setmodelmdjref(220, 245);
	setmodelmdjref(221, 246);
	setmodelmdjref(222, 247);
	setmodelmdjref(223, 248);
	setmodelmdjref(224, 249);
	setmodelmdjref(225, 250);
	setmodelmdjref(226, 251);
	setmodelmdjref(227, 252);
	setmodelmdjref(228, 253);
	setmodelmdjref(229, 254);
	setmodelmdjref(230, 255);
	setmodelmdjref(231, 256);
	setmodelmdjref(232, 257);
	setmodelmdjref(233, 258);
	setmodelmdjref(234, 259);
	setmodelmdjref(235, 260);
	setmodelmdjref(236, 261);
	setmodelmdjref(237, 262);
	setmodelmdjref(238, 263);
	setmodelmdjref(239, 264);
	setmodelmdjref(240, 265);
	setmodelmdjref(241, 266);
	setmodelmdjref(242, 267);
	setmodelmdjref(243, 268);
	setmodelmdjref(244, 269);
	setmodelmdjref(245, 270);
	setmodelmdjref(246, 271);
	setmodelmdjref(247, 272);
	setmodelmdjref(248, 273);
	setmodelmdjref(249, 274);
	setmodelmdjref(250, 275);
	setmodelmdjref(251, 276);
	setmodelmdjref(252, 277);
	setmodelmdjref(253, 278);
	setmodelmdjref(254, 279);
	setmodelmdjref(255, 280);
	setmodelmdjref(256, 281);
	setmodelmdjref(257, 282);
	setmodelmdjref(258, 283);
	setmodelmdjref(259, 284);
	setmodelmdjref(260, 285);
	setmodelmdjref(261, 286);
	setmodelmdjref(262, 287);
	setmodelmdjref(263, 288);
	setmodelmdjref(264, 289);
	setmodelmdjref(265, 290);
	setmodelmdjref(266, 291);
	setmodelmdjref(267, 292);
	setmodelmdjref(268, 293);
	setmodelmdjref(269, 294);
	setmodelmdjref(270, 295);
	setmodelmdjref(271, 296);
	setmodelmdjref(272, 297);
	setmodelmdjref(273, 298);
	setmodelmdjref(274, 299);
	setmodelmdjref(275, 300);
	setmodelmdjref(276, 301);
	setmodelmdjref(277, 302);
	setmodelmdjref(278, 303);
	setmodelmdjref(279, 304);
	setmodelmdjref(280, 305);
	setmodelmdjref(281, 306);
	setmodelmdjref(282, 307);
	setmodelmdjref(283, 308);
	setmodelmdjref(284, 309);
	setmodelmdjref(285, 310);
	setmodelmdjref(286, 311);
	setmodelmdjref(287, 312);
	setmodelmdjref(288, 313);
	setmodelmdjref(289, 314);
	setmodelmdjref(290, 315);
	setmodelmdjref(291, 316);
	setmodelmdjref(292, 317);
	setmodelmdjref(293, 318);
	setmodelmdjref(294, 319);
	setmodelmdjref(295, 320);
	setmodelmdjref(296, 321);
	setmodelmdjref(297, 322);
	setmodelmdjref(298, 323);
	setmodelmdjref(299, 324);
	setmodelmdjref(300, 325);
	setmodelmdjref(301, 326);
	setmodelmdjref(302, 327);
	setmodelmdjref(303, 328);
	setmodelmdjref(304, 329);
	setmodelmdjref(305, 330);
	setmodelmdjref(306, 331);
	setmodelmdjref(307, 332);
	setmodelmdjref(308, 333);
	setmodelmdjref(309, 334);
	setmodelmdjref(310, 335);
	setmodelmdjref(311, 336);
	setmodelmdjref(312, 337);
	setmodelmdjref(313, 338);
	setmodelmdjref(314, 339);
	setmodelmdjref(315, 340);
	setmodelmdjref(316, 341);
	setmodelmdjref(317, 342);
	setmodelmdjref(318, 343);
	setmodelmdjref(319, 344);
	setmodelmdjref(320, 345);
	setmodelmdjref(321, 346);
	setmodelmdjref(322, 347);
	setmodelmdjref(323, 348);
	setmodelmdjref(324, 349);
	setmodelmdjref(325, 350);
	setmodelmdjref(326, 351);
	setmodelmdjref(327, 352);
	setmodelmdjref(328, 353);
	setmodelmdjref(329, 354);
	setmodelmdjref(330, 355);
	setmodelmdjref(331, 356);
	setmodelmdjref(332, 357);
	setmodelmdjref(333, 358);
	setmodelmdjref(334, 359);
	setmodelmdjref(335, 360);
	setmodelmdjref(336, 361);
	setmodelmdjref(337, 362);
	setmodelmdjref(338, 363);
	setmodelmdjref(339, 364);
	setmodelmdjref(340, 365);
	setmodelmdjref(341, 366);
	setmodelmdjref(342, 367);
	setmodelmdjref(343, 368);
	setmodelmdjref(344, 369);
	setmodelmdjref(345, 370);
	setmodelmdjref(346, 371);
	setmodelmdjref(347, 372);
	setmodelmdjref(348, 373);
	setmodelmdjref(349, 374);
	setmodelmdjref(350, 375);
	setmodelmdjref(351, 376);
	setmodelmdjref(352, 377);
	setmodelmdjref(353, 378);
	setmodelmdjref(354, 379);
	setmodelmdjref(355, 380);
	setmodelmdjref(356, 381);
	setmodelmdjref(357, 382);
	setmodelmdjref(358, 383);
	setmodelmdjref(359, 384);
	setmodelmdjref(360, 385);
	setmodelmdjref(361, 386);
	setmodelmdjref(362, 387);
	setmodelmdjref(363, 388);
	setmodelmdjref(364, 389);
	setmodelmdjref(365, 390);
	setmodelmdjref(366, 391);
	setmodelmdjref(367, 392);
	setmodelmdjref(368, 393);
	setmodelmdjref(369, 394);
	setmodelmdjref(370, 395);
	setmodelmdjref(371, 396);
	setmodelmdjref(372, 397);
	setmodelmdjref(373, 398);
	setmodelmdjref(374, 399);
	setmodelmdjref(375, 400);
	setmodelmdjref(376, 401);
	setmodelmdjref(377, 402);
	setmodelmdjref(378, 403);
	setmodelmdjref(379, 404);
	setmodelmdjref(380, 405);
	setmodelmdjref(381, 406);
	setmodelmdjref(382, 407);
	setmodelmdjref(383, 408);
	setmodelmdjref(384, 409);
	setmodelmdjref(385, 410);
	setmodelmdjref(386, 411);
	setmodelmdjref(387, 412);
	setmodelmdjref(388, 413);
	setmodelmdjref(389, 414);
	setmodelmdjref(134, 415);
	setmodelmdjref(135, 416);
	setmodelmdjref(136, 417);
	setmodelmdjref(137, 418);
	setmodelmdjref(390, 419);
	setmodelmdjref(391, 420);
	setmodelmdjref(392, 421);
	setmodelmdjref(6, 422);
	setmodelmdjref(6, 423);
	setmodelmdjref(6, 424);
	setmodelmdjref(6, 425);
	setmodelmdjref(6, 426);
	setmodelmdjref(6, 427);
	setmodelmdjref(397, 428);
	setmodelmdjref(397, 429);
	setmodelmdjref(398, 430);
	setmodelmdjref(399, 431);
	setmodelmdjref(400, 432);
	setmodelmdjref(397, 433);
	setmodelmdjref(397, 434);
	setmodelmdjref(398, 435);
	setmodelmdjref(397, 436);
	setmodelmdjref(397, 437);
	setmodelmdjref(400, 438);
	setmodelmdjref(397, 439);
	setmodelmdjref(397, 440);
	setmodelmdjref(397, 441);
	setmodelmdjref(397, 442);
	setmodelmdjref(401, 443);
	setmodelmdjref(402, 444);
	setmodelmdjref(403, 445);
	setmodelmdjref(404, 446);
	setmodelmdjref(405, 447);
	setmodelmdjref(406, 448);
	setmodelmdjref(407, 449);
	setmodelmdjref(408, 450);
	setmodelmdjref(409, 451);
	setmodelmdjref(410, 452);
	setmodelmdjref(411, 453);
	setmodelmdjref(412, 454);
	setmodelmdjref(413, 455);
	setmodelmdjref(414, 456);
	setmodelmdjref(415, 457);
	setmodelmdjref(416, 458);
	setmodelmdjref(417, 459);
	setmodelmdjref(418, 460);
	setmodelmdjref(419, 461);
	setmodelmdjref(420, 462);
	setmodelmdjref(421, 463);
	setmodelmdjref(393, 464);
	setmodelmdjref(394, 465);
	setmodelmdjref(395, 466);
	setmodelmdjref(396, 467);
	setmodelmdjref(0, 468);

}
bool Item::Equip( int32 forcedslot /*= -1*/ )
{
	if (m_data->equiplimit && m_owner->CountEquippedItem(m_data->id) >= m_data->equiplimit)
		return false;

	uint32 slot;
	uint32 numslots;
	if (forcedslot != -1) //when loading from db, dont choose a slot, db contains slot
	{
		slot = forcedslot;
		numslots = 1;
	}
	else
	{
		slot = 0xFFFFFFFF;
		numslots = 0;

		sItemStorage.GetSlotForItem(m_data, slot, numslots);
	}

	if (slot != ITEM_SLOT_AMULET1 && slot != ITEM_SLOT_AMULET2 && slot != ITEM_SLOT_AMULET3 && !(m_owner->GetItemClass() & m_data->propflags))
	{
		if (m_owner->IsPlayer())
		{
			TO_PLAYER(m_owner)->SendActionResult(PLAYER_ACTION_CANT_USE_ITEM);
			TO_PLAYER(m_owner)->SendAllStats();
		}
		return false;
	}

	if (m_owner->IsPlayer() && forcedslot == -1) //need to do stat checks ;)
	{
		Player* p = TO_PLAYER(m_owner);
		if (!CheckRequirements())
		{
			//has to have stats sent to appear in corner, otherwise it appears in tooltip
			p->SendActionResult(PLAYER_ACTION_REQUIREMENTS_NOT_MET);
			p->SendAllStats();
			return false;
		}

		//check for shield and 2h
		//equipping weapon

		bool twohweapon = false;
		bool twohweaponbarb = false;
		bool shield = false;

		sItemStorage.CheckItemType(this, twohweaponbarb, twohweapon, shield);
		sItemStorage.CheckItemType(p->GetItem(ITEM_SLOT_WEAPON1), twohweaponbarb, twohweapon, shield);
		sItemStorage.CheckItemType(p->GetItem(ITEM_SLOT_WEAPON2), twohweaponbarb, twohweapon, shield);
		sItemStorage.CheckItemType(p->GetItem(ITEM_SLOT_SHIELD), twohweaponbarb, twohweapon, shield);

		if (twohweaponbarb && p->m_class != CLASS_BARBARIAN)
			twohweapon = true;

		if (twohweapon && shield)
			return false;
	}

	if (forcedslot != -1 && m_owner->GetItem(forcedslot) != NULL)
		m_owner->GetItem(forcedslot)->Delete();

	if (sItemStorage.IsStackable(m_data->type) && m_stackcount > 1)
	{
		//break us up
		--m_stackcount;
		Item* newit = new Item(m_data->id, m_owner, 1);
		if (!newit->Equip())
		{
			newit->Delete();
			++m_stackcount;
			return false;
		}
		return true;
	}

	if (slot == 0xFFFFFFFF)
		return false;

	bool equipped = false;

	for (uint32 i = slot; i < slot + numslots; ++i)
	{
		if (m_owner->m_items[i] == NULL)
		{
			slot = i;
			m_owner->m_items[i] = this;
			equipped = true;
			break;
		}
	}

	if (!equipped)
		return false;

	//since were equipped now, we have to move us :P
	if (m_currentslot != 0xFFFFFFFF)
	{
		m_owner->m_items[m_currentslot] = NULL;
		if (m_owner->IsPlayer())
			TO_PLAYER(m_owner)->CompactItems();
	}
	m_currentslot = slot;

	ApplyBonuses();

	if (m_owner->IsPlayer())
	{
		TO_PLAYER(m_owner)->Recalculate();
		TO_PLAYER(m_owner)->SendAllStats();
	}

	ApplyItemVisual(slot);

	if (m_itemguid == 0)
		m_itemguid = sInstanceMgr.GenerateItemGUID();

	return true;
}

void Item::UnEquip( bool forced /*= false*/ )
{
	RemoveBonuses();
	if (m_currentslot >= NUM_EQUIPPED_ITEMS) //not equipped
		return;
	uint32 newslot;
	if (!forced && sItemStorage.IsStackable(m_data->type))
		newslot = m_owner->FindFreeInventorySlot(m_data->id, m_stackcount);
	else
		newslot = m_owner->FindFreeInventorySlot();

	if (newslot == 0xFFFFFFFF && !forced) //no free slots, don't do anything, could lose item
		return;

	uint32 curslot = m_currentslot;

	if (!forced && newslot != 0xFFFFFFFF && m_owner->m_items[newslot] != NULL && sItemStorage.IsStackable(m_owner->m_items[newslot]->m_data->type))
	{
		m_owner->m_items[newslot]->m_stackcount += m_stackcount;
		if (m_currentslot != 0xFFFFFFFF)
				m_owner->m_items[m_currentslot] = NULL;

		if (m_owner->IsPlayer())
		{
			TO_PLAYER(m_owner)->Recalculate();
			TO_PLAYER(m_owner)->SendAllStats();
		}
		Delete();
		return;
	}
	else
	{
		if (newslot != 0xFFFFFFFF)
			m_owner->m_items[newslot] = this;
		if (m_currentslot != 0xFFFFFFFF)
			m_owner->m_items[m_currentslot] = NULL;
		m_currentslot = newslot;
	}

	RemoveItemVisual(curslot);

	if (m_owner->IsPlayer())
	{
		TO_PLAYER(m_owner)->Recalculate();
		TO_PLAYER(m_owner)->SendAllStats();
	}
}

bool Item::AddToInventory( int32 forcedslot /*= -1*/ )
{
	uint32 newslot;
	if (forcedslot != -1)
		newslot = forcedslot;
	else
	{
		if (sItemStorage.IsStackable(m_data->type))
			newslot = m_owner->FindFreeInventorySlot(m_data->id, m_stackcount);
		else
			newslot = m_owner->FindFreeInventorySlot();
	}
	
	if (newslot == 0xFFFFFFFF)
	{
		if (m_owner->IsPlayer())
		{
			TO_PLAYER(m_owner)->SendActionResult(PLAYER_ACTION_INVENTORY_FULL);
			TO_PLAYER(m_owner)->SendAllStats();
		}
		return false;
	}

	RemoveEvents(EVENT_OBJECT_DELETE);

	m_mapmgr = NULL; //use new owners instance

	if (m_group != NULL)
		m_group->RemoveItem(this);

	if (m_owner->m_items[newslot] != NULL && sItemStorage.IsStackable(m_owner->m_items[newslot]->m_data->type)) //were adding to a stack
	{
		if (m_owner->m_items[newslot]->m_data->id != m_data->id)
			return false;
		m_owner->m_items[newslot]->m_stackcount += m_stackcount;

		if (m_owner->m_items[newslot]->m_stackcount > 0xFF)
			m_owner->m_items[newslot]->m_stackcount = 0xFF;

		if (m_currentslot != 0xFFFFFFFF)
			m_owner->m_items[m_currentslot] = NULL;
		RemoveBonuses();
		if (m_owner->IsPlayer())
			TO_PLAYER(m_owner)->SendAllStats();
		Delete();
		return true;
	}
	else
	{
		if (m_owner->m_items[newslot] != NULL)
			m_owner->m_items[newslot]->Delete();
		if (m_currentslot != 0xFFFFFFFF)
			m_owner->m_items[m_currentslot] = NULL;
		m_owner->m_items[newslot] = this;
		m_currentslot = newslot;
		RemoveBonuses();
		if (m_owner->IsPlayer())
		{
			TO_PLAYER(m_owner)->SendAllStats();

			if (m_itemguid == 0)
				m_itemguid = sInstanceMgr.GenerateItemGUID();
		}
	}

	Event* ev = GetEvent(EVENT_ITEM_FORCEDELETE);
	if (ev != NULL)
	{
		char timepostfix[256];
		uint32 delay = sAccountMgr.GenerateTimePostfix(ev->m_firedelay - g_mstime, (char*)timepostfix);

		if (forcedslot == -1)
			m_owner->EventChatMsg(0, "You recieve loot: %s (%u %s)", m_data->name, delay, timepostfix);
		else
			m_owner->EventChatMsg(0, "Temporary item: %s (%u %s)", m_data->name, delay, timepostfix);

		ev->DecRef();
	}

	return true;
}

void Item::Drop( int32 amount /*= -1*/ )
{
	UnEquip(true);
	Item* dropitem = this;
	if (sItemStorage.IsStackable(m_data->type) && amount != -1 && (uint32)amount < m_stackcount)
	{
		dropitem = new Item(m_data->id, m_owner);
		dropitem->m_currentslot = 0xFFFFFFFF;
		dropitem->m_stackcount = amount;
		m_stackcount -= amount;
	}
	else
	{
		if (m_currentslot != 0xFFFFFFFF)
		{
			m_owner->m_items[m_currentslot] = NULL;
			if (m_owner->IsPlayer())
				TO_PLAYER(m_owner)->CompactItems();
		}
		m_currentslot = 0xFFFFFFFF;
	}

	int32 cellx = m_owner->m_positionX / 20;
	int32 celly = m_owner->m_positionY / 20;
	MapCell* cell = m_owner->m_cell;
	//try and find a item group where we are
	uint32 key = cellx | (celly << 16);
	ItemGroup* g = NULL;
	std::map<uint32, ItemGroup*>::iterator itr = cell->m_items.find(key);
	if (itr == cell->m_items.end())
	{
		ItemGroup* group = new ItemGroup(cellx, celly, cell);
		g = group;
		group->AddItem(dropitem);
	}
	else
	{
		g = itr->second;
		itr->second->AddItem(dropitem);
	}

	dropitem->m_mapmgr = m_owner->m_mapmgr;
	dropitem->AddEvent(&Item::Delete, EVENT_OBJECT_DELETE, 45000, 1, EVENT_FLAG_NOT_IN_WORLD);

	//push to everyone in owner's range including owner
	m_owner->AddInRangeItemGroup(g);

	for (std::map<Unit*, uint8>::iterator uitr = m_owner->m_inrangeUnits.begin(); uitr != m_owner->m_inrangeUnits.end(); ++uitr)
		uitr->first->AddInRangeItemGroup(g);

	if (m_owner->IsPlayer())
		TO_PLAYER(m_owner)->SendAllStats();
}

void Item::ApplyBonuses()
{
	HandleBonuses(true);
}

void Item::RemoveBonuses()
{
	HandleBonuses(false);
}

void Item::HandleBonuses( bool apply )
{
	//dont apply if applied or remove if removed
	if (bonusesapplied == apply)
		return;

	if (apply && m_owner->IsPlayer())
	{
		switch (m_currentslot)
		{
		case ITEM_SLOT_WEAPON1:
		case ITEM_SLOT_WEAPON2:
			{
				if (TO_PLAYER(m_owner)->m_weaponSwitch != (m_currentslot - ITEM_SLOT_WEAPON1))
					return;
			}
		}
	}
	bonusesapplied = apply;
	
	//apply properties if possible ;)
	uint32 slotbase, numslots;
	sItemStorage.GetSlotForItem(m_data, slotbase, numslots);

	//dont want ppl getting +45 hit atm
	if (slotbase == ITEM_SLOT_AMULET1)
		return;

	if (m_data->type == 65 && m_data->type2 != 8)
	{
		if (apply)
			m_owner->m_bonuses[PROTECTION_ARMOR_BONUS] += m_data->basevalue;
		else
			m_owner->m_bonuses[PROTECTION_ARMOR_BONUS] -= m_data->basevalue;
	}

	for (uint32 i = 0; i < 10; ++i)
	{
		if (m_data->bonuses[i] < NUM_ITEM_BONUSES)
		{
			if (apply)
			{
				if (m_data->bonuses[i] == BONUS_ABILITY_LEVEL)
				{
					if (m_owner->m_bonuses[BONUS_ABILITY_LEVEL] > 0)
						continue;
					bonusabilityapplied = true;
				}
				m_owner->m_bonuses[m_data->bonuses[i]] += m_data->bonusvalues[i];
			}
			else
			{
				if (m_data->bonuses[i] == BONUS_ABILITY_LEVEL)
				{
					if (!bonusabilityapplied)
						continue;
					bonusabilityapplied = false;
				}

				m_owner->m_bonuses[m_data->bonuses[i]] -= m_data->bonusvalues[i];
			}
		}
	}

	switch (slotbase)
	{
	case ITEM_SLOT_HEAD: HandleHelmProperties(apply); break;
	case ITEM_SLOT_WEAPON1: HandleWeaponProperties(apply); break;
	case ITEM_SLOT_BODY: HandleBodyProperties(apply); break;
	case ITEM_SLOT_GLOVES: HandleGloveProperties(apply); break;
	case ITEM_SLOT_RING1: HandleRingProperties(apply); break;
	case ITEM_SLOT_SHIELD: HandleShieldProperties(apply); break;
	}

	//handle hidden props
	/*if (m_stackcount & ITEM_PROP_HIDDEN_HEALTH)
	{
		HANDLE_BONUS(apply, HEALTH_BONUS, 5);
		HANDLE_BONUS(apply, BONUS_MAXIMUS_CHANCE, 10);
	}
	if (m_stackcount & ITEM_PROP_HIDDEN_DOOMBRINGER)
		HANDLE_BONUS(apply, REFLECT_DAMAGE, 5);
	if (m_stackcount & ITEM_PROP_HIDDEN_CHAOTIC)
		HANDLE_BONUS(apply, ON_HIT_DISPEL_CHANCE_CHAOTIC, 5);
	if (m_stackcount & ITEM_PROP_HIDDEN_LEECHING)
		HANDLE_BONUS(apply, ON_HIT_LEECH_CHANCE, 5);
	if (m_stackcount & ITEM_PROP_HIDDEN_ENERGISED)
	{
		HANDLE_BONUS(apply, ON_HIT_ENERGISE_CHANCE, 5);
		HANDLE_BONUS(apply, MANA_BONUS, 5);
	}
	if (m_stackcount & ITEM_PROP_HIDDEN_ANIMUS)
		HANDLE_BONUS(apply, ON_HIT_ANIMUS_CHANCE, 5);*/

	if (m_owner->IsPlayer())
		TO_PLAYER(m_owner)->Recalculate();
}

void Item::HandleBodyProperties( bool apply )
{
		//& 2 1hp reg
		//& 4 1hp reg (requires & 2)
		//& 8 2% reflect damage
		//& 0x10 1mp reg
		//& 0x20 1mp reg (requires & 0x10)
		//& 0x40 5 protection
		//& 0x80 10 protection

	if (m_stackcount & 0x02)
		HANDLE_BONUS(apply, HEALTH_REGENERATION, 1);
	if (m_stackcount & 0x04)
		HANDLE_BONUS(apply, HEALTH_REGENERATION, 1);
	if (m_stackcount & 0x08)
		HANDLE_BONUS(apply, REFLECT_DAMAGE, 2);
	if (m_stackcount & 0x10)
		HANDLE_BONUS(apply, MANA_REGENERATION, 1);
	if (m_stackcount & 0x20)
		HANDLE_BONUS(apply, MANA_REGENERATION, 1);
	if (m_stackcount & 0x40)
		HANDLE_BONUS(apply, PROTECTION_ARMOR_BONUS, 5);
	if (m_stackcount & 0x80)
		HANDLE_BONUS(apply, PROTECTION_ARMOR_BONUS, 5);
	if (m_stackcount & 0x100)
		HANDLE_BONUS(apply, REFLECT_DAMAGE, 2);
	if (m_stackcount & 0x200)
		HANDLE_BONUS(apply, BONUS_MAXIMUS_CHANCE, 1);
	if (m_stackcount & 0x400)
		HANDLE_BONUS(apply, BONUS_MAXIMUS_CHANCE, 1);
	if (m_stackcount & 0x800)
	{
		HANDLE_BONUS(apply, HEALTH_BONUS, 5);
		HANDLE_BONUS(apply, MANA_BONUS, 5);
	}
	if (m_stackcount & 0x1000)
		HANDLE_BONUS(apply, PROTECTION_ALL_MAGIC, 2);
	if (m_stackcount & 0x2000)
		HANDLE_BONUS(apply, PROTECTION_ALL_MAGIC, 2);
}

void Item::HandleHelmProperties( bool apply )
{
	if (m_stackcount & 0x02)
		HANDLE_BONUS(apply, HEALTH_REGENERATION, 1);
	if (m_stackcount & 0x04)
		HANDLE_BONUS(apply, HEALTH_REGENERATION, 1);
	if (m_stackcount & 0x08)
	{
		if (m_owner->HasAura(SPELL_AURA_NIGHT_VISION))
			m_owner->m_auras[SPELL_AURA_NIGHT_VISION]->Remove();

		if (apply)
		{
			//remove old night vision if possible
			Aura* a = new Aura(m_owner, m_owner, 41);
			a->m_dispelblock = true; //CANNOT DISPEL THIS
			//remove event to remove it since its permanent
			a->RemoveEvents(EVENT_AURA_REMOVE);
		}
	}
	if (m_stackcount & 0x10)
		HANDLE_BONUS(apply, MANA_REGENERATION, 1);
	if (m_stackcount & 0x20)
		HANDLE_BONUS(apply, MANA_REGENERATION, 1);
	if (m_stackcount & 0x40)
		HANDLE_BONUS(apply, HEALTH_BONUS, 6);
	if (m_stackcount & 0x80)
		HANDLE_BONUS(apply, MANA_BONUS, 6);
	if (m_stackcount & 0x100)
		HANDLE_BONUS(apply, HEALTH_BONUS, 6);
	if (m_stackcount & 0x200)
		HANDLE_BONUS(apply, MANA_BONUS, 6);
	if (m_stackcount & 0x400)
		HANDLE_BONUS(apply, PROTECTION_ARMOR_BONUS, 5);
	if (m_stackcount & 0x800)
		HANDLE_BONUS(apply, PROTECTION_ARMOR_BONUS, 5);
	if (m_stackcount & 0x1000)
		HANDLE_BONUS(apply, PROTECTION_ALL_MAGIC, 2);
	if (m_stackcount & 0x2000)
		HANDLE_BONUS(apply, PROTECTION_ALL_MAGIC, 2);
}

void Item::HandleGloveProperties( bool apply )
{
	if (m_stackcount & 0x02)
		HANDLE_BONUS(apply, HIT_BONUS, 2);
	if (m_stackcount & 0x04)
		HANDLE_BONUS(apply, HIT_BONUS, 2);
	if (m_stackcount & 0x08)
		HANDLE_BONUS(apply, RAGE_BONUS, 5);
	if (m_stackcount & 0x10)
		HANDLE_BONUS(apply, RAGE_BONUS, 5);
	if (m_stackcount & 0x20)
		HANDLE_BONUS(apply, ATTACK_SPEED, 5);
	if (m_stackcount & 0x40)
		HANDLE_BONUS(apply, ON_HIT_CHANCE_LETHAL, 5);
	if (m_stackcount & 0x80)
		HANDLE_BONUS(apply, ON_HIT_FEROCIAS_ATTACK, 5);
	if (m_stackcount & 0x100)
		HANDLE_BONUS(apply, ATTACK_SPEED, 5);
	if (m_stackcount & 0x200)
		HANDLE_BONUS(apply, ON_HIT_CHANCE_LETHAL, 5);
	if (m_stackcount & 0x400)
		HANDLE_BONUS(apply, ON_HIT_FEROCIAS_ATTACK, 5);
	if (m_stackcount & 0x800)
		HANDLE_BONUS(apply, PROTECTION_ARMOR_BONUS, 3);
	if (m_stackcount & 0x1000)
		HANDLE_BONUS(apply, PROTECTION_ARMOR_BONUS, 3);
	if (m_stackcount & 0x2000)
		HANDLE_BONUS(apply, CAST_SPEED, 1);
}

void Item::HandleWeaponProperties( bool apply )
{
	if (m_data->type == ITEM_TYPE_BOW)
		HandleBowWeaponProperties(apply);
	else if (m_data->propflags2 & (PROP_FLAG_WAND1 | PROP_FLAG_WAND2) || (m_data->type == ITEM_TYPE_POLE_ARM && m_data->propflags & ITEM_CLASS_WIZARD))
		HandleSpellWeaponProperties(apply);
	else
		HandleMeleeWeaponProperties(apply);
}

void Item::HandleSpellWeaponProperties( bool apply )
{
	if (m_stackcount & 0x02)
		HANDLE_BONUS(apply, MANA_REGENERATION, 1);
	if (m_stackcount & 0x04)
		HANDLE_BONUS(apply, MANA_REGENERATION, 1);
	if (m_stackcount & 0x08)
	{
		HANDLE_BONUS(apply, FIRE_MAGIC_BONUS, 5);
		HANDLE_BONUS(apply, SHADOW_MAGIC_BONUS, 5);
	}
	if (m_stackcount & 0x10)
	{
		HANDLE_BONUS(apply, ICE_MAGIC_BONUS, 5);
		HANDLE_BONUS(apply, SPIRIT_MAGIC_BONUS, 5);
	}
	if (m_stackcount & 0x20)
		HANDLE_BONUS(apply, ALL_MAGIC_BONUS, 5);
	if (m_stackcount & 0x40)
		HANDLE_BONUS(apply, HEALTH_BONUS, 6);
	if (m_stackcount & 0x80)
		HANDLE_BONUS(apply, MANA_BONUS, 6);
	if (m_stackcount & 0x100)
		HANDLE_BONUS(apply, ENERGY_MAGIC_BONUS, 7);
	if (m_stackcount & 0x200)
		HANDLE_BONUS(apply, PROTECTION_ALL_MAGIC, 3);
	if (m_stackcount & 0x400)
		HANDLE_BONUS(apply, CAST_SPEED, 2);
	if (m_stackcount & 0x800)
		HANDLE_BONUS(apply, ALL_MAGIC_BONUS, 5);
	if (m_stackcount & 0x1000)
		HANDLE_BONUS(apply, HEALTH_BONUS, 6);
	if (m_stackcount & 0x2000)
		HANDLE_BONUS(apply, MANA_BONUS, 6);
}

void Item::HandleMeleeWeaponProperties( bool apply )
{
	if (m_stackcount & 0x02)
		HANDLE_BONUS(apply, HIT_BONUS, 3);
	if (m_stackcount & 0x04)
		HANDLE_BONUS(apply, ON_HIT_FEROCIAS_ATTACK, 5);
	if (m_stackcount & 0x08)
		HANDLE_BONUS(apply, ATTACK_SPEED, 5);
	if (m_stackcount & 0x10)
		HANDLE_BONUS(apply, BONUS_DAMAGE_PCT, 5);
	if (m_stackcount & 0x20)
		HANDLE_BONUS(apply, ON_HIT_STUPIFY_CHANCE, 10);
	if (m_stackcount & 0x40)
		HANDLE_BONUS(apply, ON_DAMAGE_HEALTH_STEAL, 5);
	if (m_stackcount & 0x80)
		HANDLE_BONUS(apply, ON_DAMAGE_MANA_STEAL, 5);
	if (m_stackcount & 0x100)
		HANDLE_BONUS(apply, HIT_BONUS, 3);
	if (m_stackcount & 0x200)
		HANDLE_BONUS(apply, ON_HIT_FEROCIAS_ATTACK, 5);
	if (m_stackcount & 0x400)
		HANDLE_BONUS(apply, ATTACK_SPEED, 5);
	if (m_stackcount & 0x800)
		HANDLE_BONUS(apply, BONUS_DAMAGE_PCT, 5);
	if (m_stackcount & 0x1000)
		HANDLE_BONUS(apply, ON_HIT_DISPEL_CHANCE, 3);
	if (m_stackcount & 0x2000)
		HANDLE_BONUS(apply, ON_HIT_CHANCE_LETHAL, 4);

}

void Item::HandleRingProperties( bool apply )
{
	if (m_stackcount & 0x02)
		HANDLE_BONUS(apply, MANA_BONUS, 3);
	if (m_stackcount & 0x04)
		HANDLE_BONUS(apply, HEALTH_BONUS, 3);
 	if (m_stackcount & 0x08)
 		HANDLE_BONUS(apply, PROTECTION_ALL_MAGIC, 2);
 	if (m_stackcount & 0x10)
 		HANDLE_BONUS(apply, PROTECTION_ARMOR_BONUS, 10);
 	if (m_stackcount & 0x20)
 		HANDLE_BONUS(apply, BONUS_MAXIMUS_CHANCE, 2);
 	if (m_stackcount & 0x40)
		HANDLE_BONUS(apply, BONUS_REDITUS_CHANCE, 2);
	if (m_stackcount & 0x80)
		HANDLE_BONUS(apply, ALL_MAGIC_BONUS, 2);
	if (m_stackcount & 0x100)
	{
		HANDLE_BONUS(apply, CAST_SPEED, 1);
		HANDLE_BONUS(apply, ATTACK_SPEED, 1);
	}
	if (m_stackcount & 0x200)
	{
		HANDLE_BONUS(apply, CAST_SPEED, 1);
		HANDLE_BONUS(apply, ATTACK_SPEED, 1);
	}
	if (m_stackcount & 0x400)
		HANDLE_BONUS(apply, MANA_BONUS, 3);
	if (m_stackcount & 0x800)
		HANDLE_BONUS(apply, HEALTH_BONUS, 3);
	if (m_stackcount & 0x1000)
		HANDLE_BONUS(apply, PROTECTION_ALL_MAGIC, 2);
	if (m_stackcount & 0x2000)
		HANDLE_BONUS(apply, ALL_MAGIC_BONUS, 2);	
}

void Item::HandleShieldProperties( bool apply )
{
	if (m_stackcount & 0x02)
		HANDLE_BONUS(apply, REFLECT_DAMAGE, 2);
	if (m_stackcount & 0x04)
		HANDLE_BONUS(apply, PROTECTION_ARMOR_BONUS, 3);
	if (m_stackcount & 0x08)
		HANDLE_BONUS(apply, PROTECTION_ARMOR_BONUS, 6);
	if (m_stackcount & 0x10)
		HANDLE_BONUS(apply, DEFEND_BONUS, 4);
	if (m_stackcount & 0x20)
		HANDLE_BONUS(apply, PROTECTION_ALL_MAGIC, 2);
	if (m_stackcount & 0x40)
		HANDLE_BONUS(apply, RESIST_STUPIFY, 20);
	if (m_stackcount & 0x80)
		HANDLE_BONUS(apply, RESIST_DISPEL, 20);
	if (m_stackcount & 0x100)
		HANDLE_BONUS(apply, REFLECT_DAMAGE, 2);
	if (m_stackcount & 0x200)
		HANDLE_BONUS(apply, DEFEND_BONUS, 4);
	if (m_stackcount & 0x400)
		HANDLE_BONUS(apply, PROTECTION_ALL_MAGIC, 2);
	if (m_stackcount & 0x800)
		HANDLE_BONUS(apply, RESIST_STUPIFY, 20);
	if (m_stackcount & 0x1000)
		HANDLE_BONUS(apply, RESIST_DISPEL, 20);
	if (m_stackcount & 0x2000)
		HANDLE_BONUS(apply, BONUS_ARMOR_SPELLS, 10);

}

Item::Item( uint32 itemid, Unit* owner, int32 forcedstack )
{
	m_nodrop = false;
	m_itemflags = 0;
	m_itemversion = CURRENT_ITEM_VERSION;
	m_itemguid = 0;
	m_expiretime = 0;
	m_mapmgr = NULL;
	bonusabilityapplied = false;
	m_group = NULL;
	bonusesapplied = false;
	m_currentslot = 0xFFFFFFFF;
	m_owner = owner;
	m_data = sItemStorage.GetItem(itemid);

	if (forcedstack != -1)
		m_stackcount = forcedstack;
	else
	{
		uint32 slotbase, numslots;
		sItemStorage.GetSlotForItem(m_data, slotbase, numslots);

		if (sItemStorage.IsStackable(m_data->type))
		{
			m_stackcount = RandomUInt(49) + 1;

			if (slotbase == ITEM_SLOT_RING1)
				m_stackcount = 1;
		}
		else
			ApplyProperties();

		if (slotbase == ITEM_SLOT_AMULET1)
			m_stackcount = 1;
	}
}

Item::~Item()
{
}

void Item::Delete()
{
	if (m_owner != NULL)
	{
		HandleBonuses(false);

		uint32 curslot = m_currentslot;

		if (m_currentslot != 0xFFFFFFFF)
			m_owner->m_items[m_currentslot] = NULL;
		RemoveItemVisual(m_currentslot);
	}
	RemoveEvents();
	if (m_group != NULL)
		m_group->RemoveItem(this);

	DecRef();
}

void Item::RemoveItemVisual( uint32 curslot )
{
	if (curslot == 0xFFFFFFFF)
		return;
	switch (curslot)
	{
	case ITEM_SLOT_WEAPON1:
	case ITEM_SLOT_WEAPON2:
		{
			if (m_owner->IsPlayer() && TO_PLAYER(m_owner)->m_weaponSwitch != (curslot - ITEM_SLOT_WEAPON1))
				break;
			m_owner->SetModels(-1, 0);

			//this updates new radius (switching bow to mace, or vice versa)
			if (m_owner->IsAttacking() && m_owner->m_mapmgr != NULL)
			{
				Object* folobj = m_owner->m_mapmgr->GetObject(m_owner->m_POIObject);
				if (folobj != NULL)
					m_owner->Attack(folobj);
			}
		} break;
	case ITEM_SLOT_HEAD: m_owner->SetModels(-1, -1, -1, 0); break;
	case ITEM_SLOT_SHIELD: m_owner->SetModels(-1, -1, 0); break;
	}

	if (m_owner->IsPlayer())
		TO_PLAYER(m_owner)->UpdateBodyArmorVisual();
}

void Item::ApplyItemVisual( uint32 slot )
{
	if (slot == 0xFFFFFFFF)
		return;
	switch (slot)
	{
	case ITEM_SLOT_WEAPON1:
	case ITEM_SLOT_WEAPON2:
		{
			if (m_owner->IsPlayer() && TO_PLAYER(m_owner)->m_weaponSwitch != (slot - ITEM_SLOT_WEAPON1))
				break;
			if (m_owner->m_thrownweapon == m_data->id)
				break;
			m_owner->SetModels(-1, m_data->displayid);

			//this updates new radius (switching bow to mace, or vice versa)
			if (m_owner->IsAttacking() && m_owner->m_mapmgr != NULL)
			{
				Object* folobj = m_owner->m_mapmgr->GetObject(m_owner->m_POIObject);
				if (folobj != NULL)
					m_owner->Attack(folobj);
			}
		} break;
	case ITEM_SLOT_HEAD: m_owner->SetModels(-1, -1, -1, m_data->displayid); break;
	case ITEM_SLOT_SHIELD: m_owner->SetModels(-1, -1, m_data->displayid); break;
	}

	if (m_owner->IsPlayer())
		TO_PLAYER(m_owner)->UpdateBodyArmorVisual();
}

void Item::ApplyProperties()
{
	uint32 slot, numslots;
	sItemStorage.GetSlotForItem(m_data, slot, numslots);
	m_stackcount = 0;

	auto disabled = sItemStorage.ItemDisabled(m_data->id);

	//if (disabled == ITEM_DISABLED_BOSSDROP_TEMP) //super items get no props, sorry
	//	return;

	if (slot == ITEM_SLOT_RING1)
	{
		uint32 numprops = 0;

		while (numprops <= 2 && RandomUInt(100) <= 50)
			numprops++;

		//assign random props
		for (uint32 i = 0; i < numprops; ++i)
		{
			uint32 propbits = RandomUInt(6);

			if (m_stackcount & (2 << propbits)) //this prop already applied
			{
				--i;
				continue;
			}
			m_stackcount |= (2 << propbits);
		}

	}
	else
	{
		m_stackcount = 0;
		uint32 numprops = 0;

		while (numprops <= 8  && RandomUInt(100) <= 90)
			numprops++;

		//assign random props
		for (uint32 i = 0; i < numprops; ++i)
		{
			uint32 propbits = RandomUInt(12);

 			if (m_stackcount & (2 << propbits)) //this prop already applied
 			{
 				--i;
 				continue;
 			}
 			m_stackcount |= (2 << propbits);
		}
	}

	/*uint32 numhiddens = 0;

	uint32 hiddenseed = 1000;

	if (sItemStorage.ItemDisabled(m_data->id) == ITEM_DISABLED_NORMAL)
		hiddenseed = 10;

	while (RandomUInt(hiddenseed) == 0 && numhiddens < 2)
	{
		++numhiddens;
		uint32 hiddenflag = ITEM_PROP_HIDDEN_HEALTH << RandomUInt(5);
		//pick one not chosen so far :)
		while (m_stackcount & hiddenflag)
			hiddenflag = ITEM_PROP_HIDDEN_HEALTH << RandomUInt(5);
		m_stackcount |= hiddenflag;
	}*/
}

void Item::RemoveFromInventory()
{
	UnEquip(true);
	if (m_currentslot != 0xFFFFFFFF)
	{
		m_owner->m_items[m_currentslot] = NULL;
		if (m_owner->IsPlayer())
			TO_PLAYER(m_owner)->CompactItems();
	}
	m_currentslot = 0xFFFFFFFF;

	if (m_owner->IsPlayer())
		TO_PLAYER(m_owner)->SendAllStats();
}

void Item::MoveItem( uint32 numslot, uint32 numtomove )
{
	if (!sItemStorage.IsStackable(m_data->type))
	{
		if (m_currentslot != 0xFFFFFFFF)
			m_owner->m_items[m_currentslot] = NULL;
		m_owner->m_items[numslot] = this;
		m_currentslot = numslot;

		if (m_owner->IsPlayer())
			TO_PLAYER(m_owner)->CompactItems();
	}
	else
	{
		if (numtomove == 0)
			return; //dont create 0 stack items
		if (numtomove > m_stackcount)
			numtomove = m_stackcount;
		//split it up
		m_stackcount -= numtomove;

		if (m_owner->m_items[numslot] == NULL) //splitting stack
		{
			Item* it = new Item(m_data->id, m_owner, numtomove);
			bool added = it->AddToInventory(numslot);

			if (!added)
			{
				m_stackcount += numtomove;
				return;
			}
		}
		else
			m_owner->m_items[numslot]->m_stackcount += numtomove;

		if (m_stackcount == 0)
		{
			Unit* owner = m_owner;
			Delete();
			if (owner->IsPlayer())
			{
				TO_PLAYER(owner)->CompactItems();
				TO_PLAYER(owner)->OpenStorage();
			}
		}
	}
}

void Item::HandleBowWeaponProperties( bool apply )
{
	if (m_stackcount & 0x02)
		HANDLE_BONUS(apply, ATTACK_SPEED, 1);
	if (m_stackcount & 0x04)
		HANDLE_BONUS(apply, ATTACK_SPEED, 1);
	if (m_stackcount & 0x08)
		HANDLE_BONUS(apply, ATTACK_SPEED, 1);
	if (m_stackcount & 0x10)
		HANDLE_BONUS(apply, ON_DAMAGE_ICE_DAMAGE, 5);
	if (m_stackcount & 0x20)
		HANDLE_BONUS(apply, ON_DAMAGE_ACID_DAMAGE, 5);
	if (m_stackcount & 0x40)
		HANDLE_BONUS(apply, ON_DAMAGE_FIRE_DAMAGE, 5);
	if (m_stackcount & 0x80)
		HANDLE_BONUS(apply, BONUS_EXPLODING_ARROWS, 1);
	if (m_stackcount & 0x100)
		HANDLE_BONUS(apply, BONUS_DAMAGE_PCT, 5);
	if (m_stackcount & 0x200)
		HANDLE_BONUS(apply, BONUS_DAMAGE_PCT, 5);
	if (m_stackcount & 0x400)
		HANDLE_BONUS(apply, ON_HIT_CHANCE_LETHAL, 2);
	if (m_stackcount & 0x800)
		HANDLE_BONUS(apply, ON_HIT_CHANCE_LETHAL, 2);
	if (m_stackcount & 0x1000)
		HANDLE_BONUS(apply, ON_DAMAGE_HEALTH_STEAL, 2);
	if (m_stackcount & 0x2000)
		HANDLE_BONUS(apply, ON_DAMAGE_HEALTH_STEAL, 2);
}

ItemGroup::~ItemGroup()
{
	m_cell->m_items.erase(m_key);

	std::set<Object*>::iterator itr = m_inrangeObjects.begin(), itr2;
	while (itr != m_inrangeObjects.end())
	{
		itr2 = itr++;
		(*itr2)->RemoveInRangeItemGroup(this);
	}
}

void ItemGroup::RemoveItem( Item* i )
{
	m_items.remove(i);
	i->m_group = NULL;
	if (m_items.size() == 0)
		delete this;
}

void ItemGroup::AddItem( Item* i )
{
	m_items.push(i);
	i->m_group = this;
}