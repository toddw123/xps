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

initialiseSingleton(World);

bool World::run()
{
	while (THREAD_LOOP_CONDS)
	{
		EventUpdate();
		for (auto pack = m_pendingPacks.pop_front(); pack != NULL; pack = m_pendingPacks.pop_front())
		{
			Packet & p = *pack->pack;
			p.readpos = 0;
			uint8 opcode;
			p >> opcode;

			switch (opcode)
			{
			case CMSG_AUTH_DATA:
				{
					std::string passwordhash;
					p >> passwordhash;
					pack->sock->m_passwordhash = passwordhash;
				} break;
			case CMSG_AUTH_CHANGE:
				{
					std::string passwordhash;
					p >> passwordhash;
					pack->sock->m_changehash = passwordhash;
				} break;
			case CMSG_AUTH_SECRET:
				{
					std::string passwordhash;
					p >> passwordhash;
					pack->sock->m_secrethash = passwordhash;
				} break;
			case CMSG_REDIRECT_DATA:
				{
					p >> opcode; //redirected opcode
					uint8 encryptionIV;
					p >> encryptionIV; //not used anymore

					switch (opcode)
					{
					case MSG_SWITCH_ACCOUNT: //switch account
						HandleSwitchAccount(pack); break;
					case MSG_REQUEST_INFO: //online count
						HandleOnlineCount(pack); break;
					case MSG_NEW_ACCOUNT: //request new account
						HandleNewAccount(pack); break;
					case MSG_NEW_ACCOUNT_2: //new account set pw/sw
						HandleNewAccountP2(pack); break;
					case MSG_ACCOUNT_LOGIN:
						HandleLoginChallenge(pack); break;
					case MSG_CREATE_CHARACTER:
						HandleNewCharacter(pack); break;
					case MSG_DELETE_CHARACTER:
						HandleDeleteCharacter(pack); break;
					case MSG_PLAYER_LOGIN:
						HandlePlayerLogin(pack); break;
					case MSG_ACCOUNT_CHANGEPW:
						HandleChangePassword(pack); break;
					}
				} break;
			}
			delete pack;
		}

		Sleep(100);
	}

	return false;
}

void World::HandleSwitchAccount( NoAuthPackets* data )
{
	uint32 accid;
	uint8 buffer = 0xFF; //so it doesnt trigger null check

	*data->pack >> accid;

	std::string password;

	*data->pack >> password;

	Account* a = sAccountMgr.GetAccount(accid);

	if (a == NULL)
		return;

	if (data->sock->m_passwordhash == a->m_data->password)
	{
		//dont move this to packet, its constant, quicker to push to stack
		ServerPacket p(MSG_REQUEST_INFO, 13);
		p << uint32(7);
		p << uint32(0);
		p << uint32(0);
		data->sock->WritePacket(p);
	}

	a->DecRef();
}

void World::HandleOnlineCount( NoAuthPackets* data )
{
	sLog.Debug("Network", "Recieved online request");

	ServerPacket p(MSG_REQUEST_INFO);
	p << uint16(6); //were sending online response
	p << uint32(0); //unk
	p << sAccountMgr.GetOnlineCount();
	p << uint32(0x13BA0064); //unk
	data->sock->WritePacket(p);

	sLog.Debug("Network", "Responded with %u online to online request", sAccountMgr.GetOnlineCount());
}

void World::HandleNewAccount( NoAuthPackets* data )
{
	uint32 accid = sAccountMgr.GenerateAccountNum();

	sAccountMgr.CreateAccount(accid);
	Account* acc = sAccountMgr.GetAccount(accid);
	//acc->m_data->banexpiretime = 0xFFFFFFFF;
	//acc->m_data->banreason = strdup("Account not authorised to play, contact admins");
	acc->Save();

	ServerPacket p(MSG_NEW_ACCOUNT);
	p << accid;
	data->sock->WritePacket(p);

	sLog.Debug("Network", "Responded with account %u to new account request", accid);
}

void World::HandleNewAccountP2( NoAuthPackets* data )
{
	sLog.Debug("Network", "Recieved password/secret word set request");

	uint32 accid;
	*data->pack >> accid;

	std::string password;
	
	*data->pack >> password;


	std::string secretword;
	data->pack->readpos = 26 + 2;

	*data->pack >> secretword;

	Account* a = sAccountMgr.GetAccount(accid);

	if (a == NULL)
		return;

	sAccountMgr.SetPassword(a, (char*)data->sock->m_passwordhash.c_str());
	sAccountMgr.SetSecretWord(a, (char*)data->sock->m_secrethash.c_str());

	a->DecRef();
}

void World::HandleNewCharacter( NoAuthPackets* data )
{
	static byte chars[]="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ ";
	//
	
	//get account by sender
	Account* a = data->sock->m_account;

	if (a == NULL)
		return;

	if (a->m_data->banexpiretime >= time(NULL))
	{
		return;
	}

	std::string name;
	*data->pack >> name;

	if (name.size() == 0)
	{
		return;
	}

	uint32 spaces = 0;
	for (std::string::iterator itr=name.begin(); itr!=name.end(); ++itr)
	{
		char c = (*itr);
		if (c == 0x20)
			spaces++;

		//check the char is valid
		for (int j=0; j < sizeof(chars)/sizeof(byte); j++)
		{
			if (chars[j] == c)
				goto next;
		}
		
		return;

next:
		continue;
	}

	uint16 Str, Agi, Cons, Int, Wis, Path, Gender, myrace, myclass;
	data->pack->readpos = 0x37 + 2;
	*data->pack >> Str >> Agi >> Cons >> Int >> Wis;
	data->pack->readpos = 0x5B + 2;
	*data->pack >> Path >> Gender >> myrace >> myclass;

	if (spaces > 2) //can't have more then 1 space in name
	{
		return;
	}

	if (Str + Agi + Cons + Int + Wis != 75) //havn't placed all points, or tried to hack higher stats
	{
		return;
	}

	static uint8 racedata[NUM_RACES][NUM_CLASSES] =
	{
		{ 0x58, 0x20, 0x58, 0x58, 0x20, 0x20, 0x20, 0x58, 0x20, 0x58, 0x20, 0x20, 0x20 },
		{ 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x20, 0x20, 0x20, 0x20 },
		{ 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x20, 0x20, 0x20, 0x20 },
		{ 0x58, 0x58, 0x20, 0x20, 0x58, 0x58, 0x58, 0x20, 0x58, 0x20, 0x20, 0x20, 0x20 },
		{ 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x20, 0x20, 0x20 },
		{ 0x58, 0x20, 0x58, 0x58, 0x20, 0x20, 0x20, 0x58, 0x20, 0x58, 0x20, 0x20, 0x20 },
		{ 0x58, 0x58, 0x20, 0x20, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58, 0x58 },
		{ 0x20, 0x58, 0x20, 0x20, 0x58, 0x58, 0x58, 0x58, 0x58, 0x20, 0x20, 0x58, 0x58 },
		{ 0x20, 0x58, 0x20, 0x20, 0x58, 0x58, 0x58, 0x20, 0x58, 0x20, 0x20, 0x20, 0x58 },
		{ 0x58, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x58, 0x58, 0x58, 0x20, 0x58, 0x58 },
		{ 0x58, 0x58, 0x20, 0x20, 0x58, 0x58, 0x58, 0x20, 0x58, 0x58, 0x20, 0x58, 0x58 },
	};

	if (myrace >= NUM_RACES || myclass >= NUM_CLASSES || myclass == CLASS_ADEPT || racedata[myrace][myclass] == 0x20) //no class chosen
	{
		return;
	}

	sAccountMgr.CreatePlayer(a, name, Str, Agi, Cons, Int, Wis, Path, Gender, myrace, myclass, data);

	a->DecRef();
}

void World::HandleLoginChallenge( NoAuthPackets* data )
{
	sLog.Debug("Network", "Recieved logon challenge");

	if (g_restartEvent)
		return;
	
	uint32 accid, unk;

	std::string password;

	*data->pack >> accid >> unk >> password;

	sLog.Debug("Network", "Account number %u requesting to login", accid);

	Account* account = sAccountMgr.GetAccount(accid);

	if (account == NULL)
	{
		//try and create the account
		sAccountMgr.CreateAccount(accid);
		account = sAccountMgr.GetAccount(accid);

		if (account == NULL) //account creation failed
			return;

		//account->m_data->banexpiretime = 0xFFFFFFFF;
		//account->m_data->banreason = strdup("Account not authorised to play, contact admins");
		sAccountMgr.SetPassword(account, (char*)data->sock->m_passwordhash.c_str()); //set password for new account
		account->Save();
	}

	if (account->m_data->password != NULL && account->m_data->password == data->sock->m_passwordhash)
	{
		//remove old accounts from same client
		Account* old = data->sock->m_account;
		if (old != NULL && old != account)
		{
			old->Logout(false);
			old->Logout(true);
			old->RemoveEvents(EVENT_ACCOUNT_LOGOFF_TIMEOUT);
		}

		if (account->m_onlinePlayer != 0)
			account->Logout(false);

		sLog.Debug("Network", "Account number %u has logged in", accid);
		//put the send back data on the account
		data->sock->SetAccount(account);
		account->online = true;
		account->m_lastaction = g_time;
		
		sAccountMgr.AddOnlineAccount(account);
		account->SendCharList();

		//event a logoff
		account->RemoveEvents(EVENT_ACCOUNT_LOGOFF_TIMEOUT);
		account->AddEvent(&Account::Logout, true, EVENT_ACCOUNT_LOGOFF_TIMEOUT, 120000, 1, 0);
		
		if (account->m_data->banexpiretime < time(NULL))
			sDatabase.Execute("replace into `account_locks` values (%u, 0);", account->m_data->Id);
	}
}

void World::HandleDeleteCharacter( NoAuthPackets* data )
{
	//unsigned int ID=buffer[1]+(buffer[2]<<8);
	Account* a = data->sock->m_account;
	sLog.Debug("Network", "Delete character request");

	uint32 ID;

	*data->pack >> ID;

	if (a == NULL)
		return;

	//password
	//std::string password;
	//data->pack->readpos = 9 + 2;
	
	//*data->pack >> password;

	//if (password == data->sock->m_passwordhash)
		sAccountMgr.DeletePlayer(a, ID);
}

void World::HandlePlayerLogin( NoAuthPackets* data )
{
	//get account
	Account* a = data->sock->m_account;
	if (a == NULL || g_restartEvent || a->HasEvent(EVENT_ACCOUNT_PLAYER_LOAD))
		return;

	if (a->m_onlinePlayer != 0)
	{
		a->Logout(false);
		return;
	}

	bool banned = false;
	if (a->m_data->banexpiretime >= time(NULL))
		banned = true;

	if (banned)
		return;

	uint32 ipcount = sAccountMgr.GetOnlineIPCount(sAccountMgr.GetIP(a->netinfo));
	/*if (ipcount >= 4)
	{
		sLog.Debug("Login", "Disallowing %u to login, %s has %u references.", a->m_data->Id, inet_ntoa(a->netinfo.sin_addr), ipcount);
		return;
	}*/

	//int i=13;
	uint32 ID;
	uint32 morezeros;
	
	*data->pack >> ID >> morezeros;

	if (a->m_forcePlayerLoginIDAdmin != 0xFFFFFFFF && a->m_data->accountlevel > 0)
	{
		ID = a->m_forcePlayerLoginIDAdmin;
		a->m_forcePlayerLoginIDAdmin = 0xFFFFFFFF;
	}

	if (sAccountMgr.PlayerIsLocked(ID))
		return;

	if (data->sock->m_passwordhash != a->m_data->password)
		return;
 	SQLEvent* sqlev = new SQLEvent;
 	Event* ev = a->MakeEvent(&Account::PlayerLoadComplete, sqlev, EVENT_ACCOUNT_PLAYER_LOAD, 800, 1, 0);
 	sqlev->m_event = ev;
 	sqlev->AddQuery("select * from `players` where `id` = %u;", ID);
	sqlev->AddQuery("select * from `player_items` where `playerid` = %u and (`itemflags` = 0 or (`itemflags` & %u)) order by slot asc;", ID, ITEM_FLAG_REINSERT);
 	sqlev->AddQuery("select * from `player_marks` where `playerid` = %u;", ID);
  	sqlev->AddQuery("select * from `player_skills` where `playerid` = %u;", ID);
	sqlev->AddQuery("select * from `player_deathtokens` where `playerid` = %u;", ID);
	sqlev->AddQuery("select * from `player_skillcooldowns` where `playerid` = %u;", ID);
	sqlev->AddQuery("select * from `player_completedquests` where `playerid` = %u;", ID);
	sqlev->AddQuery("select * from `player_currency` where `playerid` = %u;", ID);
	sDatabase.Execute(sqlev);

	//remove logout event
	a->RemoveEvents(EVENT_ACCOUNT_LOGOFF_TIMEOUT);
}

void World::HandleChangePassword( NoAuthPackets* data )
{
	uint32 accid;
	std::string currentpw, newpw;
	*data->pack >> accid;
	*data->pack >> currentpw;

	AccountData* dat = sAccountData.GetEntry(accid);
	if (dat == NULL)
		return;
	if (data->sock->m_passwordhash != dat->password)
		return;
	data->pack->readpos = 0x1A + 2;
	*data->pack >> newpw;

	char* pw = strdup(data->sock->m_changehash.c_str());
	char* oldpw = dat->password;
	dat->password = pw;
	free(oldpw);
	sAccountData.Save(dat);
}

void World::LoadPendingDBData()
{
	SQLEvent* sqlev = new SQLEvent;
	Event* ev = MakeEvent(&World::LoadPendingDBDataComplete, sqlev, EVENT_WORLD_LOADPENDINGDATA, 0, 1, 0);
	sqlev->m_event = ev;
	sqlev->AddQuery("select * from `pending_changes`;");
	sDatabase.Execute(sqlev);
}

void World::LoadPendingDBDataComplete( SQLEvent* ev )
{
	if (ev->m_queryresult[0] != NULL)
	{
		for (MYSQL_ROW info = sDatabase.Fetch(ev->m_queryresult[0]); info != NULL; info = sDatabase.Fetch(ev->m_queryresult[0]))
		{
			uint32 r = 0;
			uint32 key = atol(info[r++]);
			const char* changetype = info[r++];
			const char* extra = info[r++];
			const char* extra2 = info[r++];
			const char* extra3 = info[r++];
			const char* extra4 = info[r++];
			const char* extra5 = info[r++];

			HandlePendingChange(changetype, extra, extra2, extra3, extra4, extra5);
			sDatabase.Execute("delete from `pending_changes` where `key` = %u;", key);
		}
	}

	delete ev;
}

void World::HandlePendingChange( const char* change, const char* extra, const char* extra2, const char* extra3, const char* extra4, const char* extra5 )
{
	std::string cha = change;

	if (cha == "UNBAN")
	{
		uint32 accountid = atol(extra);

		Account* acc = sAccountMgr.GetAccount(accountid);

		if (acc == NULL)
			return;

		acc->m_data->banexpiretime = 0;
		acc->Save();
		acc->DecRef();
	}

	if (cha == "BAN")
	{
		uint32 accountid = atol(extra);
		uint32 howlong = atol(extra2);
		uint32 multiplier = atol(extra3);
		const char* reason = extra4;

		Account* acc = sAccountMgr.GetAccount(accountid);

		if (acc == NULL)
			return;
		acc->m_data->banexpiretime = g_time + (howlong * multiplier);

		//handle reason
		if (reason != NULL)
		{
			char* oldreason = acc->m_data->banreason;
			acc->m_data->banreason = strdup(reason);
			free(oldreason);
		}

		acc->Save();

		acc->Logout(false);
		acc->Logout(true);

		acc->DecRef();
	}

	if (cha == "REMOVEIPBAN")
	{
		uint32 ipaddr = atoul(extra);
		uint32 cidrmask = atoul(extra2);

		sAccountMgr.RemoveIPBan(ipaddr, cidrmask);
	}

	if (cha == "UPDATE")
	{
		FILE* f = fopen("update", "wb");
		if (f != NULL)
			fclose(f);
		g_restartEvent = true;
	}

	if (cha == "RESTART")
	{
		g_restartEvent = true;
	}
}

void World::HandlePacket( ClientSocket* sock, Packet* pkt )
{
	NoAuthPackets* p = new NoAuthPackets(sock, pkt);
	m_pendingPacks.push_back(p);
}

NoAuthPackets::~NoAuthPackets()
{
	delete pack; sock->DecRef();
}

NoAuthPackets::NoAuthPackets( ClientSocket* s, Packet* p )
{
	sock = s; s->AddRef(); pack = p;
}
