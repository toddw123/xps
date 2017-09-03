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

void ServerPacket::Send()
{
	//sGameClientSocket.SendPacket(this);
}

void ServerPacket::Send( ServerPacket* original )
{
	m_addr = original->m_addr;
	Send();
}

ServerPacket::ServerPacket()
{
	m_creationsize = 0;
	m_buffer = m_stackbuffer;
	m_usingstack = true;
	readpos = 2;
	writepos = 0;
}

ServerPacket::ServerPacket(uint8 opcode)
{
	m_creationsize = 0;
	m_buffer = m_stackbuffer;
	m_usingstack = true;
	readpos = 2;
	writepos = 0;
	Write(&opcode, 1);
}

ServerPacket::ServerPacket(uint8 opcode, uint32 size)
{
	m_creationsize = size;
	m_buffer = m_stackbuffer;
	m_usingstack = true;
	readpos = 1;
	writepos = 0;
	WriteTo(size + 1);
	writepos = 0;
	Write(&opcode, 1);
	memset(&m_buffer[1], 0, size);
}