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

#ifndef __SESSION_H
#define __SESSION_H

enum PACKET_TYPES
{
	MSG_REQUEST_INFO		= 0x25, //1.179 whatever new
	MSG_NEW_ACCOUNT			= 0x0D, //1.179
	MSG_NEW_ACCOUNT_2		= 0x23, //1.179
	MSG_ACCOUNT_LOGIN		= 0x0F, //1.179
	MSG_SWITCH_ACCOUNT		= 0x28, //1.179
	SMSG_SWITCH_RESPONSE	= 0x21, //1.138 REDUNDANT NOW USES MSG_REQUEST_INFO
	MSG_CREATE_CHARACTER	= 0x0E, //1.179
	MSG_CLIENT_INFO			= 0x0C, //1.179
	MSG_DELETE_CHARACTER	= 0x24, //1.179
	MSG_ACCOUNT_CHANGEPW	= 0x22, //1.179
	MSG_PLAYER_LOGIN		= 0x10,	//1.179
	SMSG_UPDATE_DATA		= 0x03, //1.179
	MSG_PLAYER_UPDATE		= 0x06, //1.179
	SMSG_INITIAL_LOGIN_DATA	= 0x1F, //1.179
	MSG_UPDATE_LOCAL_MODELS	= 0x04, //1.179
	MSG_PLAYER_ACTION		= 0x01, //1.179
	MSG_USE_ITEM			= 0x07, //1.179
	MSG_CAST_SPELL			= 0x08, //1.179
	SMSG_PLAYER_STATS		= 0x14, //1.138 MAJOR CHANGE in 1.179 REQUIRES CODE CHANGES
	SMSG_PLAYER_EXPERIENCE	= 0x15, //1.179
	SMSG_PLAYER_ITEMS		= 0x12, //1.179
	SMSG_PLAYER_SKILLS		= 0x13, //1.179
	MSG_PLAYER_CHAT			= 0x0A, //1.179
	SMSG_CHAT_INFO_MESSAGE	= 0x1C, //1.179
	MSG_QUERY_ITEM			= 0x34, //1.179
	SMSG_QUERY_ITEM_RESPONSE= 0x35, //1.179
	SMSG_INITIATE_TRADE		= 0x10, //1.138
	SMSG_TRADE_UPDATE		= 0x1A, //1.138
	SMSG_MERCHANT_DATA		= 0x17, //1.179
	SMSG_STORAGE_DATA		= 0x1B, //1.179
	SMSG_SERVER_MESSAGE		= 0x25, //1.138
	SMSG_NAME_RESPONSE		= 0x19, //1.179
	MSG_EVID_REQNAME		= 0x90,
	SMSG_EVID_NAME_RESPONSE	= 0x91,
	NUM_MSG_TYPES			= 0x100,
};

class Session;
typedef void(Session::*PacketHandler)(ServerPacket &data);
extern PacketHandler PacketHandlers[NUM_MSG_TYPES];

enum CommandFlags
{
	COMMAND_FLAG_DELETE_CREATURE		= 0x01,
	COMMAND_FLAG_SET_CREATURE_LEVEL		= 0x02,
	COMMAND_FLAG_SET_CREATURE_FLAG		= 0x04,
	COMMAND_FLAG_SET_CREATURE_FACTION	= 0x08,
	COMMAND_FLAG_GET_CREATURE_INFO		= 0x10,
};

class Session
{
public:
	Player* m_player;
	uint16 m_lastcryptseed;
	uint8 m_lastcryptcounter;
	uint8 m_lastmovecounter;
	uint32 m_commandflags;
	uint32 m_commandarg;
	uint32 m_pastex;
	uint32 m_pastey;
	void HandleAttack(ServerPacket & data);
	void HandlePlayerUpdate(ServerPacket & data);
	void HandleInterface(ServerPacket & data);
	void HandleMovement(ServerPacket & data);
	void HandleChat(ServerPacket & data);
	void HandleCastSpell(ServerPacket & data);
	void HandleUseItem(ServerPacket & data);
	void HandleChatCommand(std::string &str);
	void HandleKeyPress(ServerPacket & data);

	//Evidyon doesn't do names the same way as Xenimus, names can show
	//directly on top of the characters and thus multiple name requests
	//might be made. We need to include id in the response
	void HandleEvidyonNameRequest(ServerPacket & data);

	void HandleItemQuery(ServerPacket & data);

	Session(Player* p)
	{
		m_pastex = 0;
		m_pastey = 0;
		m_commandflags = 0;
		m_lastmovecounter = 0;
		m_player = p;
		PacketHandlers[MSG_CLIENT_INFO] = &Session::HandleInterface;
		PacketHandlers[MSG_PLAYER_UPDATE] = &Session::HandlePlayerUpdate;
		PacketHandlers[MSG_PLAYER_CHAT] = &Session::HandleChat;
		PacketHandlers[MSG_CAST_SPELL] = &Session::HandleCastSpell;
		PacketHandlers[MSG_USE_ITEM] = &Session::HandleUseItem;
		PacketHandlers[MSG_PLAYER_ACTION] = &Session::HandleMovement;
		PacketHandlers[MSG_REQUEST_INFO] = &Session::HandleKeyPress;
		PacketHandlers[MSG_QUERY_ITEM] = &Session::HandleItemQuery;
		PacketHandlers[MSG_EVID_REQNAME] = &Session::HandleEvidyonNameRequest;

		m_lastcryptseed = 0xFFFF;
		m_lastcryptcounter = 0;
	}
};

#endif
