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

#ifndef __PACKETHANDLER_H
#define __PACKETHANDLER_H

#define CLIENT_KEY_SIZE 0x100

class Crypto
{
public:
	uint8 m_key[CLIENT_KEY_SIZE];
	Crypto();
	void Encrypt(uint8* packet, size_t len);
	void Decrypt(uint8* packet, size_t len);

	//this is used for encit.dat and encsp.dat
	void Decrypt( uint8* packet, size_t len, uint32 keystart );

};

#endif
