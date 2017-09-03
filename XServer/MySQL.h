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

#ifndef __MYSQL_H
#define __MYSQL_H

#define sDatabase MySQLInterface::getSingleton()

typedef struct SQLPool
{
	MYSQL* mysql;
	Mutex mutex;
} SQLPool;

class Event;
class SQLEvent
{
public:
	Event* m_event;
	std::vector<MYSQL_RES*> m_queryresult;
	std::vector<std::string> m_queries;
	SQLEvent() { m_event = NULL; }
	
	~SQLEvent()
	{
		for (std::vector<MYSQL_RES*>::iterator itr = m_queryresult.begin(); itr != m_queryresult.end(); ++itr)
			mysql_free_result(*itr);
	}

	virtual void OnQueryFinish();
	void AddQuery(char* q, ...)
	{
		char querybuf[16384];
		va_list vlist;
		va_start(vlist, q);
		vsnprintf(querybuf, 16384, q, vlist);
		va_end(vlist);
		m_queries.push_back(querybuf);
	}
};

class MySQLInterface : public Singleton<MySQLInterface>
{
public:
	SQLPool pool[MAX_CONNECTIONS];
	SQLPool buffer;
	Mutex m_querybufferlock;
	std::deque<SQLEvent*> m_eventQueryBuffer;
	std::deque<std::string> QueryBuffer;
	bool QueryBufferLock;

	//config
	std::string hostname;
	std::string username;
	std::string password;
	std::string database;
	uint16 port;
	//end config

	MySQLInterface();
	~MySQLInterface();
	bool Init(int i);
	bool InitBuffer();
	void Close(SQLPool sqlpool);
	int GetFreeConnection();
	MYSQL_RES* Query(char* query, ...);
	void Execute(SQLEvent* queryev);
	void Execute(const char* query, ...);
	void DoBufferedQuery(char* query);
	MYSQL_ROW Fetch(MYSQL_RES* result);
	int NumRows(char* query);
	void Free(MYSQL_RES* result);
	void DoBufferedRound();
	void EscapeStr(std::string & s, std::string & out) { Escape(s.c_str(), out); }
	void Escape(const char* s, std::string & out)
	{
		uint32 lens = strlen(s);
		out.resize(lens * 2);
		mysql_real_escape_string(this->buffer.mysql, (char*)out.data(), s, lens);
	}

	void StartWorkers(uint32 numworkers);

	void DoCleanUp();
};


class MySQLWorkerThread : public ThreadBase
{
public:
	bool run();
};


#endif
