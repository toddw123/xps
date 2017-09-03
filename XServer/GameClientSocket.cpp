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

initialiseSingleton(GameClientSocket);

void GameClientSocket::PushMap( ServerPacket* pack )
{
	Account* acc = sAccountMgr.GetAccount(m_sender);
	if (acc == NULL)
	{
		delete pack;
		return;
	}
	acc->m_lastaction = g_time;
	acc->m_pendingData.push_back(pack);
	acc->DecRef();
}

void GameClientSocket::PushWorld( ServerPacket* pack )
{
	sWorld.PushPendingPacket(pack);
}

bool GameClientSocket::OnDataRead()
{
#ifndef DEBUG
   	if (!sWorld.IsAllowedIP(m_sender.sin_addr.s_addr))
   	{
   		ResetRead();
   		return true;
   	}
#endif

	ServerPacket* pack = new ServerPacket;
	pack->m_addr = m_sender;
	pack->Write(m_readbuffer, m_readpos);

	m_crypt.Decrypt(pack->GetBuffer(), pack->GetSize());

	switch (pack->GetBuffer()[0])
	{
	case MSG_SWITCH_ACCOUNT:
	case MSG_NEW_ACCOUNT:
	case MSG_NEW_ACCOUNT_2:
	case MSG_ACCOUNT_LOGIN:
	case MSG_CREATE_CHARACTER:
	case MSG_DELETE_CHARACTER:
	case MSG_PLAYER_LOGIN:
	case MSG_ACCOUNT_CHANGEPW:
		PushWorld(pack); break;
	case MSG_REQUEST_INFO: //online count
			{
				uint16 charid;
				*pack >> charid;
				pack->readpos -= 2;
				if (charid == 0)
					PushWorld(pack);
				else
					PushMap(pack);
			} break;

		default:
			PushMap(pack);
	}

	ResetRead();
	return true;
}

void GameClientSocket::SendPacket( ServerPacket* p )
{
	m_crypt.Encrypt(p->GetBuffer(), p->GetSize());
	SendTo(p->GetBuffer(), p->GetSize() + 1, p->m_addr);
}