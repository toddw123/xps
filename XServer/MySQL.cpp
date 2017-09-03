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

initialiseSingleton(MySQLInterface);

MySQLInterface::MySQLInterface()
{

	hostname = m_config.GetStringDefault("Database", "Hostname", "localhost");
	port = (uint16)m_config.GetIntDefault("Database", "Port", 3306);
	username = m_config.GetStringDefault("Database", "User", "root");
	password = m_config.GetStringDefault("Database", "Password", "replace");
	database = m_config.GetStringDefault("Database", "Database", "xen");

	mysql_library_init(-1, NULL, NULL);
	for (int i=0; i<MAX_CONNECTIONS; i++)
		Init(i);

	InitBuffer();

	QueryBufferLock=false;
}

MySQLInterface::~MySQLInterface()
{
	for (int i=0; i<MAX_CONNECTIONS; i++)
		mysql_close(pool[i].mysql);
	mysql_library_end();
}

bool MySQLInterface::Init(int i)
{
	sLog.Debug("Database", "Connecting...");

	pool[i].mysql=mysql_init(NULL);

	if (!pool[i].mysql)
		DoExitMessage(10, (char*)mysql_error(pool[i].mysql));

	if (!mysql_real_connect(pool[i].mysql, hostname.c_str(), username.c_str(), password.c_str(), NULL, this->port, NULL, 0))
		DoExitMessage(10, (char*)mysql_error(pool[i].mysql));

	//This is called after connect to fix a reset bug prior to 5.0.19
	my_bool option = true;
	if (mysql_options(pool[i].mysql, MYSQL_OPT_RECONNECT, &option))
		sLog.Error("MySQL", "Cannot set reconnect option");

	if (mysql_select_db(this->pool[i].mysql, this->database.c_str()) != 0)
		DoExitMessage(10, (char*)mysql_error(pool[i].mysql));

	return true;
}

bool MySQLInterface::InitBuffer()
{
	sLog.Debug("Database", "Buffer connecting...");

	this->buffer.mysql=mysql_init(NULL);

	if (!this->buffer.mysql)
		DoExitMessage(10, (char*)mysql_error(buffer.mysql));

	if (!mysql_real_connect(buffer.mysql, hostname.c_str(), username.c_str(), password.c_str(), NULL, port, NULL, 0))
		DoExitMessage(10, (char*)mysql_error(buffer.mysql));

	//This is called after connect to fix a reset bug prior to 5.0.19
	my_bool option = true;
	if (mysql_options(buffer.mysql, MYSQL_OPT_RECONNECT, &option))
		sLog.Error("MySQL", "Cannot set reconnect option");

	if (mysql_select_db(buffer.mysql, database.c_str()) != 0)
		DoExitMessage(10, (char*)mysql_error(buffer.mysql));

	return true;
}

void MySQLInterface::Close(SQLPool sqlpool)
{
	mysql_close(sqlpool.mysql);
}

int MySQLInterface::GetFreeConnection()
{
	for (int i=0; i<MAX_CONNECTIONS; i++)
	{
		if (this->pool[i].mutex.AttemptAcquire())
			return i;
	}
	return MAX_CONNECTIONS; //1 higher then possible
}

MYSQL_RES* MySQLInterface::Query(char* q, ...)
{
	char query[16384];

	va_list vlist;
	va_start(vlist, q);
	vsnprintf(query, 16384, q, vlist);
	va_end(vlist);


	int poolindex = GetFreeConnection();
	while (poolindex == MAX_CONNECTIONS)
	{
		Sleep(10);
		poolindex = GetFreeConnection();
	}

	if (mysql_query(pool[poolindex].mysql, query) != 0)
		sLog.Error("Database", "Error, %s\nQuery: %s", mysql_error(pool[poolindex].mysql), query);


	MYSQL_RES* result = mysql_store_result(this->pool[poolindex].mysql);

	this->pool[poolindex].mutex.Release();

	return result;
}

void MySQLInterface::DoBufferedQuery(char* query)
{
	if (mysql_query(buffer.mysql, query) != 0)
		sLog.Error("Database", "Error, %s\nQuery: %s", mysql_error(this->buffer.mysql), query);
}

void MySQLInterface::Execute(const char* q, ...)
{
	if (QueryBufferLock)
		return;

	char query[16384];

	va_list vlist;
	va_start(vlist, q);
	vsnprintf(query, 16384, q, vlist);
	va_end(vlist);

	std::string strq = query;

	m_querybufferlock.Acquire();
	QueryBuffer.push_back(strq);
	m_querybufferlock.Release();
}

void MySQLInterface::Execute( SQLEvent* queryev )
{
	m_querybufferlock.Acquire();
	m_eventQueryBuffer.push_back(queryev);
	m_querybufferlock.Release();
}

MYSQL_ROW MySQLInterface::Fetch(MYSQL_RES* result)
{
	return mysql_fetch_row(result);
}

int MySQLInterface::NumRows(char* query)
{
	int poolindex = GetFreeConnection();

	while (poolindex == MAX_CONNECTIONS)
	{
		sLog.Warning("Database", "Cannot find unlocked mysql connection...\nAttempting again in 1 second...");
		Sleep(1000);
		poolindex = GetFreeConnection();
	}

	mysql_query(pool[poolindex].mysql, query);

	MYSQL_RES* query_res = mysql_store_result(this->pool[poolindex].mysql);

	int result = (int)mysql_affected_rows(this->pool[poolindex].mysql);

	this->Free(query_res);

	this->pool[poolindex].mutex.Release();

	return result;
}

void MySQLInterface::Free(MYSQL_RES* result)
{
	mysql_free_result(result);
}

void MySQLInterface::DoBufferedRound()
{
	while (QueryBuffer.size() > 0)
	{
		std::string query = QueryBuffer.front();
		QueryBuffer.pop_front();
		DoBufferedQuery((char*)query.c_str());
	}
}

void MySQLInterface::DoCleanUp()
{
#define CLEANUPQUERY(q, ...) sLog.Notice("Database", "Running query: "q, __VA_ARGS__); Query(q, __VA_ARGS__)

	CLEANUPQUERY("delete from `npc_spawns` where entry = 0;");
	CLEANUPQUERY("update `players` set `bosskilledid` = 0;");
	CLEANUPQUERY("truncate table `player_online`;");
	CLEANUPQUERY("truncate table `account_locks`;");
	CLEANUPQUERY("update `accounts` set `numplayers` = (select count(*) from `players` where `accountid` = `AccNum` and (`id` & 0x80000000) = 0);");

#undef CLEANUPQUERY
}

void MySQLInterface::StartWorkers( uint32 numworkers )
{
	for (uint32 i = 0; i < numworkers; ++i)
		sThreadPool.AddTask(new MySQLWorkerThread);
}

void SQLEvent::OnQueryFinish()
{
	if (m_event == NULL)
	{
		delete this;
		return;
	}
	if (!m_event->obj->AddEvent(m_event))
		delete this;
}

bool MySQLWorkerThread::run()
{
	while (THREAD_LOOP_CONDS)
	{
		while (sDatabase.m_eventQueryBuffer.size() > 0)
		{
			sDatabase.m_querybufferlock.Acquire();
			if (sDatabase.m_eventQueryBuffer.size() == 0)
			{
				sDatabase.m_querybufferlock.Release();
				break;
			}
			SQLEvent* ev = sDatabase.m_eventQueryBuffer.front();
			sDatabase.m_eventQueryBuffer.pop_front();
			sDatabase.m_querybufferlock.Release();
			for (std::vector<std::string>::iterator itr = ev->m_queries.begin(); itr != ev->m_queries.end(); ++itr)
			{
				ev->m_queryresult.push_back(sDatabase.Query("%s", (*itr).c_str()));
			}
			ev->OnQueryFinish();
		}
		while (sDatabase.QueryBuffer.size() > 0)
		{
			sDatabase.m_querybufferlock.Acquire();
			if (sDatabase.QueryBuffer.size() == 0)
			{
				sDatabase.m_querybufferlock.Release();
				break;
			}
			std::string query = sDatabase.QueryBuffer.front();
			sDatabase.QueryBuffer.pop_front();
			sDatabase.m_querybufferlock.Release();
			MYSQL_RES* res = sDatabase.Query("%s", query.c_str());

			if (res != NULL)
				sDatabase.Free(res);
		}
		Sleep(100);
	}

	return false;
}