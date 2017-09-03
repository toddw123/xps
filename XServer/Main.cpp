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

#define SOCKET_ACCEPT_COUNT 1
int g_serverstatus = 0;
time_t g_time = time(NULL);
tm g_localtime = *localtime(&g_time);
uint32 g_mstime = GetMSTime();
ConfigFile m_config;
bool g_restartEvent = false;
uint32 g_restartTimer = 0xFFFFFFFF;
uint32 g_restartTimerLastShow = 0xFFFFFFFF;


void OnExit(int signal)
{
	//ONLY ON SIGINT WILL WE DO THIS, OTHER SIGNALS ARE DANGEROUS
	if (signal != SIGINT)
		return;

	ServerStatus = SERVER_SHUTDOWN;
	g_restartEvent = true;
}

#ifdef WIN32
LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo)
{
	// Create the date/time string
	time_t curtime = time(NULL);
	tm * pTime = localtime(&curtime);
	char filename[MAX_PATH];
	TCHAR modname[MAX_PATH*2];
	ZeroMemory(modname, sizeof(modname));
	if(GetModuleFileName(0, modname, MAX_PATH*2-2) <= 0)
		strcpy(modname, "UNKNOWN");

	char * mname = strrchr(modname, '\\');
	(void*)mname++;

	sprintf(filename, "dumps\\dump-%s-%u-%u-%u-%u-%u-%u-%u.dmp",
		mname, pTime->tm_year+1900, pTime->tm_mon+1, pTime->tm_mday,
		pTime->tm_hour, pTime->tm_min, pTime->tm_sec, GetCurrentThreadId());

	char filename2[MAX_PATH];

	sprintf(filename2, "dumps\\quickdump-%s-%u-%u-%u-%u-%u-%u-%u.dmp",
		mname, pTime->tm_year+1900, pTime->tm_mon+1, pTime->tm_mday,
		pTime->tm_hour, pTime->tm_min, pTime->tm_sec, GetCurrentThreadId());


	HANDLE hDump = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);
	HANDLE hDump2 = CreateFile(filename2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);

	if(hDump == INVALID_HANDLE_VALUE || hDump2 == INVALID_HANDLE_VALUE)
	{
		// Create the directory first
		CreateDirectory("dumps", 0);
		hDump = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);
		hDump2 = CreateFile(filename2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);
	}

	printf("\nCreating crash dump file %s\n", filename);

	if(hDump == INVALID_HANDLE_VALUE || hDump2 == INVALID_HANDLE_VALUE)
	{
		MessageBox(0, "Could not open crash dump file.", "Crash dump error.", MB_OK);
	}
	else
	{
		MINIDUMP_EXCEPTION_INFORMATION info;

		info.ClientPointers = FALSE;
		info.ExceptionPointers = ExceptionInfo;
		info.ThreadId = GetCurrentThreadId();


		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			hDump2, MiniDumpWithIndirectlyReferencedMemory, ExceptionInfo == NULL? NULL : &info, 0, 0);

		CloseHandle(hDump2);

		info.ClientPointers = FALSE;
		info.ExceptionPointers = ExceptionInfo;
		info.ThreadId = GetCurrentThreadId();


		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
			hDump, MiniDumpWithFullMemory, ExceptionInfo == NULL? NULL : &info, 0, 0);

		CloseHandle(hDump);
	}

	__try
	{

		if (SINGLETON_EXISTS(MySQLInterface))
		{
			sDatabase.QueryBufferLock = true;
			//wait for the query buffer to clean
			while (sDatabase.m_eventQueryBuffer.size() != 0)
			{
				sLog.Debug("Database", "Waiting for event query buffer to empty, %u queries left", sDatabase.m_eventQueryBuffer.size());
				Sleep(1000);
			}

			while (sDatabase.QueryBuffer.size() != 0)
			{
				sLog.Debug("Database", "Waiting for query buffer to empty, %u queries left", sDatabase.QueryBuffer.size());
				Sleep(1000);
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		ExitProcess(0);
	}

	ExitProcess(0);
	return EXCEPTION_EXECUTE_HANDLER;
}

void InvalidParameterHandler(const wchar_t * expression, const wchar_t * function, const wchar_t * file, unsigned int line,uintptr_t pReserved)
{
	//what this is: purposfully generate an exception so the exception handler steps in with a valid stream
	int* lol = NULL;
	*lol = 0;
}

#endif



int main()
{
	//dirs we need
	_mkdir("map");
	_mkdir("maps");
	_mkdir("dumps");
	_mkdir("client");
	_mkdir("logs");


#ifdef WIN32
	SetUnhandledExceptionFilter(UnhandledExceptionHandler);
	_CrtSetReportMode(_CRT_ASSERT, 0);
	_set_invalid_parameter_handler(InvalidParameterHandler);
#endif
	m_config.SetSource("Xen.conf");
	new Log;
	ServerStatus = SERVER_STARTUP;
	InitRandomNumberGenerators();
	InitSkillLimits();

	memset(&PacketHandlers, 0, sizeof(PacketHandler) * NUM_MSG_TYPES);

	signal(SIGINT, OnExit);
	sLog.Notice("Main", "Starting XenServer...");

	new EventMgr;
	new ThreadPool;

	new LauncherFileInfo;

	SocketHandler* sockhandler = new SocketHandler;

	World* w = new World;
	w->InitHolder();
	w->AddEvent(&World::LoadPendingDBData, EVENT_WORLD_LOADPENDINGDATA, 1000, 0, 0);
	sThreadPool.AddTask(w);

	new MySQLInterface;
	sDatabase.StartWorkers(10);

	//cleanup time!
	sDatabase.DoCleanUp();
	w->StartLogCleaner();

	sServerSetting.Load("settings");
	sCreatureProto.Load("npc_proto");
	sCreatureSpells.Load("npc_spells");
	sCreatureEquipment.Load("npc_equip");
	sCreatureSpawn.Load("npc_spawns");
	sCreatureQuestGivers.Load("npc_questgivers");
	sQuests.Load("quests");
	sQuestText.Load("quest_text");
	sQuestLocs.Load("quest_locs");
	sSpellData.Load("spell_data");
	sSpellAuras.Load("spell_auras");
	sSpellLifetime.Load("spell_lifetime");
	sItemVisuals.Load("item_visuals");
	sItemSpells.Load("item_spells");
	sSkillStore.Load("skill_data");

	sAccountData.Load("accounts");

	g_ServerSettings.Load();

	new AccountManager;

	sLog.Debug("SizeInfo", "ThreadPool size: %u bytes", sizeof(ThreadPool));
	sLog.Debug("SizeInfo", "Thread size: %u bytes", sizeof(Thread));
	sLog.Debug("SizeInfo", "MySQLInterface size: %u bytes", sizeof(MySQLInterface));
	sLog.Debug("SizeInfo", "Player size: %u bytes", sizeof(Player));
	sLog.Debug("SizeInfo", "Unit size: %u bytes", sizeof(Unit));
	sLog.Debug("SizeInfo", "MapMgr size: %u bytes", sizeof(MapMgr) + (sizeof(MapCell) * CELL_X_COUNT * CELL_Y_COUNT));

	new ItemStorage;

	sAccountMgr.LoadUsedNPCNames();
	sAccountMgr.LoadAllAccounts();
	sAccountMgr.LoadIPBans();

	new SpellStorage;
	new ScriptMgr;
	new MeshBuilder;
	new InstanceMgr;
#ifdef DEBUG
	sMeshBuilder.StartWorkers(10);
#else
	sMeshBuilder.StartWorkers(10);
#endif
	sInstanceMgr.UpdateMaxItemGUID();

	sockhandler->StartWorkers(2);

	ClientListener* launchersock = new ClientListener;

	launchersock->Start();
	sThreadPool.AddTask(launchersock);

	GameClientSocket* sock = new GameClientSocket;

	//set this up
	sock->SetHandler(sockhandler); //set handler
	sock->Bind(INADDR_ANY, 5050);

	sock->ReadFrom(); //create read event

	sInstanceMgr.CreateMap(0);
	sInstanceMgr.CreateMap(1);
	ServerStatus = SERVER_RUNNING;

	uint64 loopcounter = 0;

	while (!g_restartEvent)
	{
		++loopcounter;

		g_time = time(NULL);
		g_localtime = *localtime(&g_time);
		g_mstime = GetMSTime();

		if (!(loopcounter % 3000)) //30secs
		{
			sThreadPool.Gobble();
		}

		if (g_restartTimer != 0xFFFFFFFF)
		{
			if (g_restartTimer <= g_time)
				g_restartEvent = true;
			else
			{
				uint32 timeleft = g_restartTimer - g_time;

				bool display = (timeleft % 60) == 0;

				if (timeleft >= 3600)
					display = (timeleft % 3600) == 0;
				if (timeleft <= 30)
					display = true;
				if (g_restartTimerLastShow == timeleft)
					display = false;

				if (display)
				{
					g_restartTimerLastShow = timeleft;

					char postfix[2048];

					uint32 timeleftlol = sAccountMgr.GenerateTimePostfix(timeleft * 1000, postfix);

					char msg[2048];
					sprintf(msg, "Restart in %u %s!", timeleftlol, postfix);

					GlobalSay psay;
					psay.message = msg;
					psay.name = "SYSTEM";
					psay.to_link = 0;
					psay.to_player = 0;
					sInstanceMgr.HandleGlobalMessage(psay);
				}
			}
		}

		Sleep(10);
	}

	if (SINGLETON_EXISTS(AccountManager))
	{
		// 		for (std::set<Account*>::iterator itr=sAccountMgr.m_accounts.begin(); itr!=sAccountMgr.m_accounts.end(); ++itr)
		// 		{
		// 			if (sAccountMgr.IsOnline(*itr))
		// 			{
		// 				sAccountMgr.SendAccountMessage((*itr), ACCOUNT_MSG_LOGOFF_SUCCESSFULL);
		// 				sAccountMgr.Logout((*itr), true);
		// 			}
		// 		}
		sAccountMgr.LogoffAllPlayers();
		Sleep(5000);
	}

	if (SINGLETON_EXISTS(MySQLInterface))
	{
		//wait for the query buffer to clean
		while (sDatabase.m_eventQueryBuffer.size() != 0)
		{
			sLog.Debug("Database", "Waiting for event query buffer to empty, %u queries left", sDatabase.m_eventQueryBuffer.size());
			Sleep(1000);
		}

		while (sDatabase.QueryBuffer.size() != 0)
		{
			sLog.Debug("Database", "Waiting for query buffer to empty, %u queries left", sDatabase.QueryBuffer.size());
			Sleep(1000);
		}
	}

#ifdef WIN32
	WSACleanup();
#endif

	if (SINGLETON_EXISTS(ThreadPool))
	{
		sThreadPool.Shutdown();
		delete ThreadPool::getSingletonPtr();
	}

	if (SINGLETON_EXISTS(Log))
		delete Log::getSingletonPtr();

	return 0;
}
