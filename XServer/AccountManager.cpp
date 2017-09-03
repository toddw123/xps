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

initialiseSingleton(AccountManager); //needed
SQLStore<AccountData> sAccountData;

void AccountManager::CreateAccount(unsigned int AccNum)
{
	if (AccNum < 1000000 || AccNum >= 10000000)
	{
		sLog.Debug("AccountMgr", "Attempted to create account %u, out of range", AccNum);
		return;
	}

	//is there an account number there already?
	if (AccountExists(AccNum))
	{
		sLog.Debug("AccountMgr", "Attempted to create account %u, already exists", AccNum);
		return;
	}

	AccountData* ad = new AccountData;
	memset(ad, 0, sizeof(AccountData));
	ad->Id = AccNum;
	

	Account* a = new Account;
	a->m_data = ad;
	a->m_onlinePlayer = 0;
	a->m_data->Id = AccNum;
	a->online = false;

	AddAccount(a);
	m_accounts.insert(a);
	a->Save();
}

bool AccountManager::AccountExists(uint32 AccNum)
{
	AccountData* data = sAccountData.GetEntry(AccNum);
	return (data != NULL);
}

uint32 AccountManager::GenerateAccountNum()
{
	uint32 Num=RandomUInt(10000000);

	while (AccountExists(Num) || Num < 1000000 || Num >= 10000000)
		Num = RandomUInt(10000000);

	return Num;
}

Account* AccountManager::GetAccount(uint32 AccNum)
{
	Account* a;
	for (ConcurrentSet<Account*>::iterator i = m_accounts.begin(); i != m_accounts.end(); ++i)
	{
		a = *(i.m_itr);
		if (a->m_data->Id == AccNum)
		{
			a->AddRef();
			return a;
		}
	}

	return false;
}

Account* AccountManager::GetAccount(sockaddr_in sender)
{
	ConcurrentUnorderedMap<uint64, Account*>::iterator itr = m_onlineaccounts.find((uint64)sender.sin_addr.IN_ADDR_IP << 32 | sender.sin_port);
	if (itr != m_onlineaccounts.end())
	{
		Account * a = itr.m_itr->second;
		a->AddRef();
		return a;
	}
	return NULL;
}

uint16 AccountManager::GetOnlineCount()
{
	uint16 count = 0;
	ConcurrentUnorderedMap<uint64, Account*>::iterator itr = m_onlineaccounts.begin();
	for (; itr != m_onlineaccounts.end(); ++itr)
	{
		if (itr.m_itr->second->online)
			++count;
	}
	return count;
}

bool AccountManager::IsOnline(Account* a)
{
	return a->online;
}

void AccountManager::SetPassword(Account* a, char* password)
{
	if (a == NULL || (a->m_data->password != NULL && strlen(a->m_data->password) > 0) || a->online)
		return;

	//free old
	free(a->m_data->password);
	a->m_data->password = strdup(password);
	a->Save();
}

void AccountManager::SetSecretWord(Account* a, char* secretword)
{
	if (a==NULL || (a->m_data->secretword != NULL && strlen(a->m_data->secretword) > 0) || a->online)
		return;

	//free old
	free(a->m_data->secretword);
	a->m_data->secretword = strdup(secretword);
	a->Save();
}

void AccountManager::LoadAllAccounts()
{
	sAccountData.m_lock.AcquireReadLock();
	int i = 0;
	for (unordered_map<uint32, AccountData*>::iterator itr = sAccountData.m_data.begin(); itr != sAccountData.m_data.end(); ++itr)
	{
		++i;
		Account* a = new Account;
		a->m_data = itr->second;
		a->online = false;
		a->m_onlinePlayer = 0;
		m_accounts.insert(a);
	}

	LoadAllPlayers();
	sLog.Debug("AccountMgr", "Cached %u accounts", sAccountData.m_data.size());
	sAccountData.m_lock.ReleaseReadLock();
}

void AccountManager::SendCharList(Account* a)
{
	if (!a->online) //we cant send info to nothing
		return;

	
}

bool AccountManager::PlayerExists(std::string Name)
{
	return m_usednames.find(Name) != m_usednames.end();
}

bool AccountManager::PlayerExists(uint32 playerid)
{
	return (m_usedplayerids.find(playerid) != m_usedplayerids.end());
}

void AccountManager::SavePlayer( Account* a, Player* p, bool newslot /*= true*/ )
{
}

void AccountManager::DeletePlayer( Account* a, uint32 playerid )
{
	sLog.Debug("AccountMgr", "Deleting player %u from account %u", playerid, a->m_data->Id);

	if (a == NULL || a->m_onlinePlayer != 0)
		return;

	//dont delete a player with pending writes or save calls (could do a delete then a replace, which would be bad)
	if (GetPlayerLockFlags(playerid) != 0)
		return;

	AddPlayerLock(playerid);

	//delete all players because we need to reindex
	SQLEvent* sqlev = new SQLEvent;
	Event* ev = a->MakeEvent(&Account::DeletePlayerComplete, sqlev, playerid, EVENT_ACCOUNTMGR_REMOVELOCK, 0, 1, 0);
	sqlev->m_event = ev;

	LOG_ACTION("DELETECHAR", "Account %u deleting playerid %u", a->m_data->Id, playerid);

	//this lets us clean up used names
	sqlev->AddQuery("select `name` from `players` where `id` = %u;", playerid);
	sqlev->AddQuery("update `players` set `id` = `id` | 0x80000000 where `id` = %u;", playerid);
	//sqlev->AddQuery("delete from `player_marks` where `playerid` = %u;", playerid);
	//sqlev->AddQuery("delete from `player_items` where `playerid` = %u;", playerid);
	sDatabase.Execute(sqlev);
}

bool AccountManager::CreatePlayer( Account* a, std::string Name, uint16 Str, uint16 Agi, uint16 Cons, uint16 Int, uint16 Wis, uint16 Path, uint16 Gender, uint16 Race, uint16 Class, NoAuthPackets* pack )
{
	if (a->m_data->numplayers >= 5)
		return false;

	sAccountMgr.FixName(Name);

	if (PlayerExists(Name))
	{
		SendAccountMessage(a, ACCOUNT_MSG_NAME_ALREADY_EXIST, pack); //name already exists
		return false;
	}

	uint32 ID = RandomUInt(HIGHID_PLAYER - LOWID_PLAYER) + LOWID_PLAYER;
	while (PlayerExists(ID))
		ID = RandomUInt(HIGHID_PLAYER - LOWID_PLAYER) + LOWID_PLAYER;

	sLog.Debug("AccountMgr", "Creating player %s (IP %s)", Name.c_str(), inet_ntoa(a->netinfo.sin_addr));

	Player* p = new Player(ID);
	p->m_accountId = a->m_data->Id;
	p->m_account = a;
	p->m_namestr = Name;
	p->m_strength = Str;
	p->m_agility = Agi;
	p->m_constitution = Cons;
	p->m_intellect = Int;
	p->m_wisdom = Wis;
	p->m_gender = Gender;
	p->m_race = Race;
	p->m_path = p->GetPath();
	p->m_class = Class;
	p->m_level = 1;
	p->m_experience = 0;
	p->m_points = 0;
	p->Recalculate();
	p->Respawn();
	p->UpdateFaction();
	p->UpdateBodyArmorVisual();
	p->SaveToDB();
	sAccountMgr.RemovePlayerLock(p->m_playerId, 0xFFFFFFFF);
	p->RemoveEvents();
	p->Delete();

	m_usednames.insert(Name);
	AddExistingPlayerId(ID);

	SendAccountMessage(a, ACCOUNT_MSG_PLAYER_WAS_SAVED_ON_SERVER, pack);
	a->RemoveEvents(EVENT_ACCOUNT_CHARLIST_CACHE_DELETE);
	a->EventExpireCharCache();


	a->m_data->numplayers += 1;
	a->Save();

	return true;
}

void AccountManager::Logout(Account* a, bool force)
{
	if (a == NULL)
		return;
	a->Logout(force);
}

void AccountManager::SaveAllAccounts()
{
	sLog.Debug("AccountMgr", "Saving all accounts");
	Account* a;
	for (ConcurrentSet<Account*>::iterator itr = m_accounts.begin(); itr != m_accounts.end(); ++itr)
	{
		a = *(itr.m_itr);
		a->Save();
	}
}

void AccountManager::SaveAllPlayers()
{
	sInstanceMgr.SaveAllPlayers();
}

void AccountManager::SendAccountMessage( Account* a, uint32 message, NoAuthPackets* pack /*= NULL*/ )
{
	ClientSocket* sock = NULL;
	if (a != NULL)
		sock = a->GetSocket();
	if (pack != NULL)
	{
		sock = pack->sock;
		sock->AddRef();
	}

	if (sock == NULL)
		return;

	ServerPacket p(MSG_CLIENT_INFO);

	p << message;
	p << (uint8)16 << (uint32)0 << (uint16)0; //wtf is this?

	sock->WritePacket(p);

	sock->DecRef();
}

Account* AccountManager::GetOnlineAccount( uint64 key )
{
	ConcurrentUnorderedMap<uint64, Account*>::iterator itr = m_onlineaccounts.find(key);
	if (itr != m_onlineaccounts.end())
	{
		Account* a = itr.m_itr->second;
		a->AddRef();
		return a;
	}
	return NULL;
}

void AccountManager::LogoffAllPlayers()
{
	Account* a;
	for (ConcurrentSet<Account*>::iterator itr = m_accounts.begin(); itr != m_accounts.end(); ++itr)
	{
		a = *(itr.m_itr);
		Logout(a, false);
	}
}

void AccountManager::FixName( std::string & name )
{
	if (name.size() == 0)
		return;
	//ends in space, remove
	while (name.size() > 0 && name[name.size() - 1] == 0x20)
		name = name.substr(0, name.size() - 1);
	if (name.size() == 0)
		return;
	//space at front
	while (name[0] == 0x20)
		name = name.substr(1);
	//lowercase it all
	for (uint32 i = 0; i < name.size(); ++i)
		name[i] = tolower(name[i]);
	name[0] = toupper(name[0]);
	size_t pos = name.find(" ");
	while (pos != std::string::npos && pos + 1 < name.size())
	{
		name[pos + 1] = toupper(name[pos + 1]);
		pos = name.find(" ", pos + 1);
	}

	if (name == "Noname" || name.size() > 19)
		name = "NONAME";
}

void AccountManager::LoadAllPlayers()
{
	MYSQL_RES* resource = sDatabase.Query("select * from players");
	if (resource == NULL || mysql_num_rows(resource) == 0)
		return;
	for (MYSQL_ROW info = sDatabase.Fetch(resource); info != NULL; info = sDatabase.Fetch(resource))
	{
		std::string name = info[2];
		FixName(name);	
		m_usednames.insert(name);
		uint32 id = atoul(info[0]);
		id &= ~0x80000000;
		AddExistingPlayerId(id);
	}

	sDatabase.Free(resource);
}

void AccountManager::LoadUsedNPCNames()
{
	for (unordered_map<uint32, CreatureProto*>::iterator itr = sCreatureProto.m_data.begin(); itr != sCreatureProto.m_data.end(); ++itr)
	{
		m_usednames.insert(itr->second->name);
	}
}

void AccountManager::LoadIPBans()
{
	MYSQL_RES* resource = sDatabase.Query("select * from `ip_bans`");
	if (resource == NULL || mysql_num_rows(resource) == 0)
		return;

	uint8 slot = 0;
	for (MYSQL_ROW info = sDatabase.Fetch(resource); info != NULL; info = sDatabase.Fetch(resource))
	{
		uint32 ipaddr = atoul(info[0]);
		uint32 banmask = atoul(info[1]);
		m_bannedIPs.insert(std::make_pair(ipaddr, banmask));
	}

	sDatabase.Free(resource);
}
void Account::LoadPlayers()
{
}

void Account::SendActionResult( uint16 res )
{
	ServerPacket data(MSG_PLAYER_ACTION, 2);
	data << res;
	WritePacket(data);
}

void Account::Logout( bool force )
{
	RemoveEvents(EVENT_ACCOUNT_LOGOFF_TIMEOUT);

	if (!force)
	{
		sAccountMgr.SendAccountMessage(this, ACCOUNT_MSG_LOGOFF_SUCCESSFULL);
		//add event for forced logout
		AddEvent(&Account::Logout, true, EVENT_ACCOUNT_LOGOFF_TIMEOUT, 120000, 1, 0);
	}
	else
	{
		sAccountMgr.RemoveOnlineAccount(this);
		sDatabase.Execute("delete from `account_locks` where `accid` = %u and `locktype` = 0;", m_data->Id);
	}

	online = false;

	if (m_onlinePlayer != 0)
		sInstanceMgr.RemovePlayerByAccount(this);

	m_socklock.Acquire();
	if (m_sock != NULL)
		m_sock->SetAccount(NULL);
	m_socklock.Release();
}

void Account::DeletePlayerComplete( SQLEvent* ev, uint32 plrid )
{
	sAccountMgr.RemovePlayerLock(plrid, PLAYER_LOCK_PENDING_DB_WRITE);
	m_data->numplayers -= 1;
	Save();
	RemoveEvents(EVENT_ACCOUNT_CHARLIST_CACHE_DELETE);
	EventExpireCharCache();
	SendCharList();
	delete ev;
}

Account::Account()
{
	m_sock = NULL;
	m_charlistcached = false;
	m_onlinePlayer = 0;
	online = false;
	m_lastaction = time(NULL);
	AddRef();
}

void Account::SendCharList()
{
	if (!m_charlistcached)
	{
		CacheCharList();
		return;
	}

	ServerPacket p(MSG_ACCOUNT_LOGIN, 145);

	static uint8 cpos[5]={0x01, 0x1d, 0x39, 0x55, 0x71};

	uint32 cposind = 0;

	ConcurrentMap<uint32, std::string>::iterator itr = m_cachednames.begin();

	while (itr != m_cachednames.end())
	{
		if (cposind >= 5)
			break;
		p.WriteTo(cpos[cposind++]);
		p << itr.m_itr->first;
		p << itr.m_itr->second;
		++itr;
	}

	for (; cposind < 5; ++cposind)
	{
		p.WriteTo(cpos[cposind]);
		p << uint32(0xFFFFFFFF);
	}

	p.WriteTo(137);
	//15 B8 64 DD
	p << uint32(0xDD64B815);
	p << uint32(0x7C45A546);

	WritePacket(p);

	if (m_data->banexpiretime >= time(NULL))
	{
		//this account is banned
		uint32 bantimeleft = m_data->banexpiretime - time(NULL);
		ServerPacket p2(SMSG_SERVER_MESSAGE);
		p2 << uint32(0x0B); //banned thing
		p2 << bantimeleft;
		p2.WriteTo(21);
		if (m_data->banreason != NULL)
			p2.Write((uint8*)m_data->banreason, strlen(m_data->banreason));
		p2.WriteTo(200);
		WritePacket(p2);

		Logout(true);
	}

	if (sAccountMgr.BannedIP(netinfo.sin_addr.s_addr))
	{
		//this account is banned
		uint32 bantimeleft = 9001;
		ServerPacket p2(SMSG_SERVER_MESSAGE);
		p2 << uint32(0x0C); //banned thing
		p2 << bantimeleft;
		p2.WriteTo(21);
		const char* banreason = "Your IP is banned from this server.";
		p2.Write((uint8*)banreason, strlen(banreason));
		p2.WriteTo(200);
		WritePacket(p2);
		Logout(true);
	}
}

void Account::CacheCharList()
{
	SQLEvent* sqlev = new SQLEvent;
	Event* ev = MakeEvent(&Account::CacheCharListComplete, sqlev, EVENT_ACCOUNT_CHARLIST_CACHE, 0, 1, 0);
	sqlev->m_event = ev;

	sqlev->AddQuery("select `id`, `name` from `players` where `accountid` = %u AND (`id` & 0x80000000) = 0;", m_data->Id);

	sDatabase.Execute(sqlev);
}

void Account::CacheCharListComplete( SQLEvent* ev )
{
	AddEvent(&Account::EventExpireCharCache, EVENT_ACCOUNT_CHARLIST_CACHE_DELETE, 300000, 1, 0);

	if (ev->m_queryresult[0] != NULL)
	{
		for (MYSQL_ROW info = sDatabase.Fetch(ev->m_queryresult[0]); info != NULL; info = sDatabase.Fetch(ev->m_queryresult[0]))
		{
			uint32 plrid = atol(info[0]);
			char* charname = info[1];
			m_cachednames.insert(std::make_pair(plrid, charname));
		}
	}

	m_charlistcached = true;

	SendCharList();
}

void Account::PlayerLoadComplete( SQLEvent* ev )
{
	if (ev->m_queryresult[0] == NULL) //player not found
	{
		delete ev;
		return;
	}

	MYSQL_ROW info = sDatabase.Fetch(ev->m_queryresult[0]);

	if (info == NULL) //player deleted by third party during query
	{
		delete ev;
		return;
	}

	Player* player = new Player(atoi(info[0]));
	player->m_account = this;
	player->m_accountId = atoi(info[1]);
	player->m_namestr = info[2];

	sAccountMgr.FixName(player->m_namestr);	
	uint32 row = 4;
	player->m_strength = atoi(info[row++]);
	player->m_agility = atoi(info[row++]);
	player->m_constitution = atoi(info[row++]);
	player->m_intellect = atoi(info[row++]);
	player->m_wisdom = atoi(info[row++]);
	player->m_path = atoi(info[row++]);
	player->m_gender = atoi(info[row++]);
	player->m_race = atoi(info[row++]);
	player->m_class = atoi(info[row++]);
	player->m_oldclass = atoi(info[row++]);
	player->m_level = atoul(info[row++]);
	player->m_experience = atoul(info[row++]);
	player->m_points = atoi(info[row++]);
	player->m_positionX = atoi(info[row++]);
	player->m_positionY = atoi(info[row++]);
	player->m_alignment = atoi(info[row++]);
	player->m_pvplevel = atoul(info[row++]);
	player->m_pvpexp = atoul(info[row++]);
	player->m_mapid = atoul(info[row++]);
	player->m_bosskilledid = atoul(info[row++]);
	player->m_color1 = atoul(info[row++]);
	player->m_color2 = atoul(info[row++]);
	player->m_alignlock = atoul(info[row++]);
	player->m_temppvpexp = atoul(info[row++]);

	if (player->m_bosskilledid != 0)
	{
		//player should be dead
		if (RandomUInt(1) == 0)
			player->m_unitAnimation = 21;
		else
			player->m_unitAnimation = 45;
		player->SetUpdateBlock(UPDATETYPE_ANIMATION);
		player->m_deathstate = STATE_DEAD;
		player->AddEvent(&Player::Respawn, EVENT_PLAYER_RESPAWN, 8000, 1, EVENT_FLAG_NOT_IN_WORLD);
	}

	player->m_path = player->GetPath();
	player->Recalculate();
	player->SetHealth(player->m_maxhealth);
	player->SetMana(player->m_maxmana);
	player->UpdateFaction();
	player->UpdateBodyArmorVisual();

	player->m_skillpoints = player->m_level;
	player->m_pvppoints = player->m_pvplevel;

	uint32 r = 1;

	player->OnItemLoad(ev->m_queryresult[r++]);
	player->OnMarksLoad(ev->m_queryresult[r++]);
	player->OnSkillsLoad(ev->m_queryresult[r++]);
	player->OnDeathTokensLoad(ev->m_queryresult[r++]);
	player->OnSkillCooldownLoad(ev->m_queryresult[r++]);
	player->OnCompletedQuestsLoad(ev->m_queryresult[r++]);
	player->OnCurrencyLoad(ev->m_queryresult[r++]);

	player->m_pendingmaptransfer.playerportal = true; //push out of no mark areas
	MapMgr* maptopush = sInstanceMgr.GetMap(player->m_mapid);

	if (maptopush != NULL)
	{
		m_onlinePlayer = player->m_playerId;
		RemoveEvents(EVENT_ACCOUNT_LOGOFF_TIMEOUT);

		maptopush->AddObject(player);
	}
	else
		player->Delete();

	delete ev;
}

void PlayerSaveSQLEvent::OnQueryFinish()
{
	sAccountMgr.RemovePlayerLock(playerid, PLAYER_LOCK_PENDING_DB_WRITE);
	delete this;
}