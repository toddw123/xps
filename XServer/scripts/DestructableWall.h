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

#ifndef __DESTRUCTABLEWALL_H
#define __DESTRUCTABLEWALL_H


class DestructableWallAI : public CreatureAIScript
{
public:
	void OnSetUnit() { m_unit->SetUnitFlag(UNIT_FLAG_CANNOT_MOVE | UNIT_FLAG_CANNOT_ATTACK); }

	void OnRespawn() { CloseDoors(); }
	void OnDeath() { OpenDoors(); }

	void OpenDoors()
	{
		TimedObject* obj1 = m_unit->m_mapmgr->GetTimedObject(m_unit->m_positionX / 20, m_unit->m_positionY / 20);
		if (obj1 == NULL)
			return;
		obj1->SetOpen();
	}

	void CloseDoors()
	{
		TimedObject* obj1 = m_unit->m_mapmgr->GetTimedObject(m_unit->m_positionX / 20, m_unit->m_positionY / 20);
		if (obj1 == NULL)
			return;
		obj1->SetClosed();
	}

	static CreatureAIScript* Create() { return new DestructableWallAI; }
};


class RickAstleyAI : public CreatureAIScript
{
public:
	void OnKilledPlayer(Player* p)
	{
		m_unit->EventChatMsg(0, "We're no strangers to love");
		m_unit->EventChatMsg(0, "You know the rules and so do I");
		m_unit->EventChatMsg(0, "A full commitment's what I'm thinking of");
		m_unit->EventChatMsg(0, "You wouldn't get this from any other guy");

		m_unit->EventChatMsg(0, "I just wanna tell you how I'm feeling");
		m_unit->EventChatMsg(0, "Gotta make you understand");

		m_unit->EventChatMsg(0, "Never gonna give you up");
		m_unit->EventChatMsg(0, "Never gonna let you down");
		m_unit->EventChatMsg(0, "Never gonna run around and desert you");
		m_unit->EventChatMsg(0, "Never gonna make you cry");
		m_unit->EventChatMsg(0, "Never gonna say goodbye");
		m_unit->EventChatMsg(0, "Never gonna tell a lie and hurt you");

		m_unit->EventChatMsg(0, "We've know each other for so long");
		m_unit->EventChatMsg(0, "Your heart's been aching");
		m_unit->EventChatMsg(0, "But you're too shy to say it");
		m_unit->EventChatMsg(0, "Inside we both know what's been going on");
		m_unit->EventChatMsg(0, "We know the game and we're gonna play it");


		m_unit->EventChatMsg(0, "And if you ask me how I'm feeling");
		m_unit->EventChatMsg(0, "Don't tell me you're too blind to see");

		m_unit->EventChatMsg(0, "Never gonna give you up");
		m_unit->EventChatMsg(0, "Never gonna let you down");
		m_unit->EventChatMsg(0, "Never gonna run around and desert you");
		m_unit->EventChatMsg(0, "Never gonna make you cry");
		m_unit->EventChatMsg(0, "Never gonna say goodbye");
		m_unit->EventChatMsg(0, "Never gonna tell a lie and hurt you");

		m_unit->EventChatMsg(0, "Never gonna give you up");
		m_unit->EventChatMsg(0, "Never gonna let you down");
		m_unit->EventChatMsg(0, "Never gonna run around and desert you");
		m_unit->EventChatMsg(0, "Never gonna make you cry");
		m_unit->EventChatMsg(0, "Never gonna say goodbye");
		m_unit->EventChatMsg(0, "Never gonna tell a lie and hurt you");

		m_unit->EventChatMsg(0, "Give you up, give you up");
		m_unit->EventChatMsg(0, "Give you up, give you up");
		m_unit->EventChatMsg(0, "Never gonna give,");
		m_unit->EventChatMsg(0, "Never gonna give, give you up");
		m_unit->EventChatMsg(0, "Never gonna give,");
		m_unit->EventChatMsg(0, "Never gonna give, give you up");

	}

	static CreatureAIScript* Create() { return new RickAstleyAI; }
};

#endif
