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

initialiseSingleton(LauncherFileInfo);
initialiseSingleton(ClientListener);

bool ClientSocket::OnDataRead()
{
	while (true)
	{
		if (m_readpos < 4)
			return true;
		uint32 pktsize = *(uint32*)m_readbuffer;
		if (m_readpos < 4 + pktsize)
			return true;


		//where do we push this?
		if (m_account == NULL || m_account->m_onlinePlayer == 0) //hasn't logged in yet
		{
			//ok it's read
			Packet* pkt = new Packet;
			pkt->Write(m_readbuffer + 4, pktsize);
			sWorld.HandlePacket(this, pkt);
		}
		else
		{
			Packet* heappacket = new Packet;
			heappacket->Write(m_readbuffer + 4, pktsize);
			m_account->m_pendingPackets.push_back(heappacket);
			m_account->m_lastaction = g_time;
		}

		if (m_readpos > 4 + pktsize)
		{
			//mov buffer
			memmove(m_readbuffer, &m_readbuffer[4 + pktsize], m_readpos - 4 - pktsize);
			m_readpos -= 4 + pktsize;
			continue;
		}

		ResetRead();
		return true;
	}
}

void ClientSocket::OnConnect()
{
	DisableNagle();
	sLauncherListener.AddSocket(this);
}

void ClientSocket::OnRemove()
{
	sLauncherListener.RemoveSocket(this);
}

void ClientSocket::ClearAccount()
{
	m_accountlock.Acquire();
	if (m_account == NULL)
	{
		m_accountlock.Release();
		return;
	}
	Account* a = m_account;
	m_account = NULL;

	//We must hold a reference here to ensure we are not deleted memory returning from Account::ClearSocket
	AddRef();
	a->ClearSocket();
	m_accountlock.Release();
	DecRef();
}

void ClientSocket::SetAccount( Account* a )
{
	m_accountlock.Acquire();
	if (m_account != NULL)
		ClearAccount();
	m_account = a;
	if (a != NULL)
		a->SetSocket(this);
	m_accountlock.Release();
}

void ClientSocket::WritePacket( ServerPacket* p )
{
	uint8 opcode = p->GetBuffer()[0];
	Packet data(SMSG_REDIRECT_DATA);
	data << opcode;
	data.Write(p->GetBuffer(), p->GetSize());
	WritePacket(data);
}

bool ClientListener::run()
{
	while (THREAD_LOOP_CONDS)
	{
		m_lock.Acquire();

		unordered_multimap<uint64, ConnectedLauncher>::iterator itr, itr2;

		for (itr = m_connectedsockets.begin(); itr != m_connectedsockets.end();)
		{
			itr2 = itr++;

			ClientSocket* s = itr2->second.sock;

			if (s->deleted)
			{
				s->DecRef();
				m_connectedsockets.erase(itr2);
				continue;
			}
		}

		m_lock.Release();
		Sleep(1000);
	}
	return true;
}