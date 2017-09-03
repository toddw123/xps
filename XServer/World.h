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

#ifndef __WORLD_H
#define __WORLD_H

class ClientSocket;
class NoAuthPackets
{
public:
	ClientSocket* sock;
	Packet* pack;

	NoAuthPackets(ClientSocket* s, Packet* p);
	~NoAuthPackets();
};

class World : public ThreadBase, public Singleton<World>, public EventHolder, public EventableObject
{
public:
	unordered_map<uint32, uint32> m_allowedIPs;
	FastMutex m_allowedIPLock;

	LockedDeque<ServerPacket> m_pendingPackets;
	LockedDeque<NoAuthPackets> m_pendingPacks;
	bool run();
	int32 GetInstanceID() { return -1; }

	void PushPendingPacket(ServerPacket* p) { m_pendingPackets.push_back(p); }

	void AddAllowedIP(uint32 ip)
	{
		m_allowedIPLock.Acquire();
		unordered_map<uint32, uint32>::iterator itr = m_allowedIPs.find(ip);
		if (itr == m_allowedIPs.end())
			m_allowedIPs.insert(std::make_pair(ip, 1));
		else
			itr->second += 1;
		m_allowedIPLock.Release();
	}

	void RemoveAllowedIP(uint32 ip)
	{
		m_allowedIPLock.Acquire();
		unordered_map<uint32, uint32>::iterator itr = m_allowedIPs.find(ip);
		if (itr != m_allowedIPs.end())
		{
			if (itr->second == 1)
				m_allowedIPs.erase(itr);
			else
				itr->second -= 1;
		}
		m_allowedIPLock.Release();
	}

	bool IsAllowedIP(uint32 ip)
	{
		bool retval = false;
		m_allowedIPLock.Acquire();
		unordered_map<uint32, uint32>::iterator itr = m_allowedIPs.find(ip);
		if (itr != m_allowedIPs.end())
			retval = true;
		m_allowedIPLock.Release();
		return retval;
	}

	void StartLogCleaner()
	{
		LogCleanup();
		AddEvent(&World::LogCleanup, EVENT_UNKNOWN, 3600000, 0, 0);
	}

	void LogCleanup()
	{
		int32 tlol = time(NULL);
		tlol -= 60 * 60 * 24 * 7; //clear anything older then 7 days

		//cleanup logs
		SQLEvent* ev = new SQLEvent;
		ev->AddQuery("insert into `log_data_old` (select * from `log_data` where `timestamp` <= %u);", tlol);
		ev->AddQuery("delete from `log_data` where `timestamp` <= %u;", tlol);
		sDatabase.Execute(ev);
	}

	//handlers
	void HandleSwitchAccount(NoAuthPackets* data);
	void HandleOnlineCount(NoAuthPackets* data);
	void HandleNewAccount(NoAuthPackets* data);
	void HandleNewAccountP2(NoAuthPackets* data);
	void HandleNewCharacter(NoAuthPackets* data);
	void HandleDeleteCharacter(NoAuthPackets* data);
	void HandleLoginChallenge(NoAuthPackets* data);
	void HandlePlayerLogin(NoAuthPackets* data);
	void HandleChangePassword(NoAuthPackets* data);

	void LoadPendingDBData();

	void LoadPendingDBDataComplete(SQLEvent* ev);

	void HandlePendingChange(const char* change, const char* extra, const char* extra2, const char* extra3, const char* extra4, const char* extra5);

	void HandlePacket(ClientSocket* sock, Packet* pkt);
};

#define sWorld World::getSingleton()

#endif