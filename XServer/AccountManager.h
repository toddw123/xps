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

#ifndef __ACCOUNTMANAGER_H
#define __ACCOUNTMANAGER_H

//this is the account data block ;)
#pragma pack(push, 1)
struct AccountData
{
	uint32 Id;
	char* password;
	char* secretword;
	uint32 accountlevel;
	uint32 muteexpiretime;
	uint32 banexpiretime;
	char* banreason;
	uint32 numplayers;
};
#pragma pack(pop)

extern SQLStore<AccountData> sAccountData;

#define sAccountMgr AccountManager::getSingleton()

class PlayerSaveSQLEvent : public SQLEvent
{
public:
	uint32 playerid;
	void OnQueryFinish();
};

class Account : public EventableObject
{
public:
	sockaddr_in netinfo; //required to send data back with no data being sent to us
	uint32 m_onlinePlayer;
	bool online;
	time_t m_lastaction;
	LockedDeque<ServerPacket> m_pendingData;
	LockedDeque<Packet> m_pendingPackets;
	ConcurrentMap<uint32, std::string> m_cachednames;
	bool m_charlistcached;

	ClientSocket* m_sock;
	FastMutex m_socklock;

	AccountData* m_data;

	uint32 m_forcePlayerLoginIDAdmin;

	int32 GetInstanceID() { return -1; }

	Account();

	~Account()
	{
		ASSERT(false);
	}

	void Save()
	{
		sAccountData.Save(m_data);
	}

	void LoadPlayers();

	void SendActionResult(uint16 res);

	void Logout(bool force);

	void DeletePlayerComplete(SQLEvent* ev, uint32 plrid);

	void SendCharList();

	void CacheCharList();

	void CacheCharListComplete(SQLEvent* ev);

	void EventExpireCharCache()
	{
		m_cachednames.clear();
		m_charlistcached = false;
	}

	void PlayerLoadComplete(SQLEvent* ev);

	void SetSocket(ClientSocket* s)
	{
		m_socklock.Acquire();
		m_sock = s;
		s->AddRef();
		m_socklock.Release();
	}

	void ClearSocket() { m_socklock.Acquire(); if (m_sock != NULL) m_sock->DecRef(); m_sock = NULL; m_socklock.Release(); }

	ClientSocket* GetSocket()
	{
		m_socklock.Acquire();
		if (m_sock != NULL)
			m_sock->AddRef();
		auto retval = m_sock;
		m_socklock.Release();
		return retval;
	}

	void WritePacket(ServerPacket & pack) { WritePacket(&pack); }
	void WritePacket(ServerPacket* pack)
	{
		auto sock = GetSocket();

		if (sock != NULL)
		{
			sock->WritePacket(pack);
			sock->DecRef();
		}
	}

	void WritePacket(Packet & pack) { WritePacket(&pack); }
	void WritePacket(Packet* pack)
	{
		auto sock = GetSocket();

		if (sock != NULL)
		{
			sock->WritePacket(pack);
			sock->DecRef();
		}
	}

	void Send(Packet & pkt)
	{
		auto sock = GetSocket();
		if (sock != NULL)
		{
			sock->WritePacket(pkt);
			sock->DecRef();
		}
	}
};

enum AccountMessages
{
	ACCOUNT_MSG_PLAYER_WAS_SAVED_ON_SERVER = 0x0D,
	ACCOUNT_MSG_NAME_ALREADY_EXIST = 0x0E,
	ACCOUNT_MSG_LOGOFF_SUCCESSFULL = 0x14,
};

enum PlayerLockFlags
{
	PLAYER_LOCK_PENDING_DB_WRITE	= 0x01,
	PLAYER_LOCK_PENDING_SAVE_CALL	= 0x02,
};

class AccountManager: public Singleton <AccountManager>, public EventableObject
{
public:
	unordered_set<std::string> m_usednames;
	ConcurrentSet<Account*> m_accounts;
	ConcurrentSet<uint32> m_usedplayerids;
	//Fucking UDP, lets key it by port
	ConcurrentUnorderedMap<uint64, Account*> m_onlineaccounts;
	unordered_map<uint32, uint32> m_onlineIPs;
	std::map<uint32, uint32> m_bannedIPs;
	FastMutex m_bannedIPLock;
	FastMutex m_onlineIPLock;

	std::map<uint32, uint32> m_playerlocks;
	FastMutex m_playerlockmutex;

	uint16 m_online;

	AccountManager() : m_online(0), m_hiPlayerId(0), m_hiAccountId(0) {}

	void AddAccount(Account* a) { a->AddRef(); m_accounts.insert(a); }
	void RemoveAccount(Account* a) { m_accounts.erase(a); a->DecRef(); }

	void AddPlayerLock(uint32 playerid, uint32 lockflag = 1)
	{
		FastGuard g(m_playerlockmutex);
		std::map<uint32, uint32>::iterator itr = m_playerlocks.find(playerid);
		if (itr == m_playerlocks.end())
			m_playerlocks.insert(std::make_pair(playerid, lockflag));
		else
			itr->second |= lockflag;
	}

	void RemovePlayerLock(uint32 playerid, uint32 lockflag = 1)
	{
		FastGuard g(m_playerlockmutex);
		std::map<uint32, uint32>::iterator itr = m_playerlocks.find(playerid);
		if (itr == m_playerlocks.end())
			return;
		else
		{
			itr->second &= ~lockflag;

			if (itr->second == 0)
				m_playerlocks.erase(playerid);
		}
	}

	bool PlayerIsLocked(uint32 playerid)
	{
		FastGuard g(m_playerlockmutex);
		return (m_playerlocks.find(playerid) != m_playerlocks.end());
	}

	uint32 GetPlayerLockFlags(uint32 playerid)
	{
		FastGuard g(m_playerlockmutex);

		std::map<uint32, uint32>::iterator itr = m_playerlocks.find(playerid);

		if (itr == m_playerlocks.end())
			return 0;
		return itr->second;
	}

	void EventPlayerSaveComplete(SQLEvent* ev, uint32 playerid)
	{
		RemovePlayerLock(playerid);
		delete ev;
	}

	void FixName(std::string & name);

	uint32 m_hiAccountId;
	uint32 m_hiPlayerId;

	void AddOnlineIP(uint32 ip)
	{
		m_onlineIPLock.Acquire();
		unordered_map<uint32, uint32>::iterator itr = m_onlineIPs.find(ip);
		if (itr == m_onlineIPs.end())
			m_onlineIPs.insert(std::make_pair(ip, 1));
		else
			itr->second += 1;
		m_onlineIPLock.Release();
	}

	void RemoveOnlineIP(uint32 ip)
	{
		m_onlineIPLock.Acquire();
		unordered_map<uint32, uint32>::iterator itr = m_onlineIPs.find(ip);
		if (itr != m_onlineIPs.end())
		{
			if (itr->second == 1)
				m_onlineIPs.erase(itr);
			else
				itr->second -= 1;
		}
		m_onlineIPLock.Release();
	}

	uint32 GetOnlineIPCount(uint32 ip)
	{
		m_onlineIPLock.Acquire();
		unordered_map<uint32, uint32>::iterator itr = m_onlineIPs.find(ip);
		if (itr == m_onlineIPs.end())
		{
			m_onlineIPLock.Release();
			return 0;
		}
		uint32 count = itr->second;
		m_onlineIPLock.Release();
		return count;
	}

	uint32 GetIP(sockaddr_in & addr)
	{
		return addr.sin_addr.IN_ADDR_IP;
	}

	void AddOnlineAccount(Account* a)
	{
		a->AddRef();
		m_onlineaccounts.insert(std::make_pair((uint64)a->netinfo.sin_addr.IN_ADDR_IP << 32 | a->netinfo.sin_port, a));
		m_online = m_onlineaccounts.size();
	}

	void RemoveOnlineAccount(Account* a)
	{
		m_onlineaccounts.erase((uint64)a->netinfo.sin_addr.IN_ADDR_IP << 32 | a->netinfo.sin_port);
		m_online = m_onlineaccounts.size();
		a->DecRef();
	}

	//This holds a reference!
	Account* GetOnlineAccount(uint64 key);

	void CreateAccount(uint32 AccNum);
	uint32 GenerateAccountNum();
	bool AccountExists(uint32 AccNum);
	Account* GetAccount(uint32 AccNum);
	Account* GetAccount(sockaddr_in sender);
	uint16 GetOnlineCount();
	bool IsOnline(Account* a);
	void SetPassword(Account* a, char* password);
	void SetSecretWord(Account* a, char* secretword);
	void SendCharList(Account* a);
	void LoadAllAccounts();
	void LoadAllPlayers();
	void LoadUsedNPCNames();
	void LoadIPBans();
	void SaveAllAccounts();
	bool CreatePlayer(Account* a, std::string Name, uint16 Str, uint16 Agi, uint16 Cons, uint16 Int, uint16 Wis, uint16 Path, uint16 Gender, uint16 Race, uint16 Class, NoAuthPackets* pack);
	void SavePlayer(Account* a, Player* p, bool newslot = true);
	bool PlayerExists(std::string Name);
	bool PlayerExists(uint32 playerid);
	void AddExistingPlayerId(uint32 playerid) { m_usedplayerids.insert(playerid); }
	void DeletePlayer(Account* a, uint32 playerid);
	void SaveAllPlayers();
	void LogoffAllPlayers();
	void Logout(Account* a, bool force);
	void SendAccountMessage(Account* a, uint32 message, NoAuthPackets* pack = NULL);

	void ReleaseUsedName(const char* name) { m_usednames.erase(name); }
	void ReleaseUsedID(uint32 plrid) { m_usedplayerids.erase(plrid); }

	bool BannedIP(uint32 ipaddr)
	{
		FastGuard g(m_bannedIPLock);
		for (std::map<uint32, uint32>::iterator itr = m_bannedIPs.begin(); itr != m_bannedIPs.end(); ++itr)
		{
			if ((ipaddr & itr->second) == (itr->first & itr->second))
				return true;
		}
		return false;
	}

	void AddBannedIPEntry(uint32 ipaddr, uint32 cidrmask)
	{
		FastGuard g(m_bannedIPLock);
		m_bannedIPs.insert(std::make_pair(ipaddr, cidrmask));
		sDatabase.Execute("insert into `ip_bans` VALUES (%u, %u);", ipaddr, cidrmask);
	}

	void RemoveIPBan(uint32 ipaddr, uint32 cidrmask)
	{
		FastGuard g(m_bannedIPLock);
		std::map<uint32, uint32>::iterator itr2;
		for (std::map<uint32, uint32>::iterator itr = m_bannedIPs.begin(); itr != m_bannedIPs.end();)
		{
			std::map<uint32, uint32>::iterator itr2 = itr++;
			if (ipaddr = itr2->first && cidrmask == itr2->second)
			{
				m_bannedIPs.erase(itr2);
			}
		}

		sDatabase.Execute("delete from `ip_bans` where `ip` = %u AND `cidrmask` = %u;", ipaddr, cidrmask);
	}

	uint32 GenerateTimePostfix(uint32 timems, char* postbuffer)
	{
		char* postfix = "unknown";
		timems /= 1000; //seconds
		postfix = timems == 1? "second" : "seconds";
		if (timems >= 60)
		{
			timems /= 60;
			postfix = timems == 1? "minute" : "minutes";
			if (timems >= 60)
			{
				timems /= 60;
				postfix = timems == 1? "hour" : "hours";
				if (timems >= 24)
				{
					timems /= 24;
					postfix = timems == 1? "day" : "days";
				}
			}
		}

		strcpy(postbuffer, postfix);

		return timems;
	}
};


#endif
