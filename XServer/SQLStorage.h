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

#ifndef __SQLSTORAGE_H
#define __SQLSTORAGE_H

#include <vector>
#include "MySQL.h"
#include "StackBuffer.h"

template <typename T> class SQLStore
{
public:
	unordered_map<uint32, T*> m_data;
	RWLock m_lock;
	uint32 m_maxentry;
	std::string m_tablename;

	std::vector<uint32> m_fieldtypes;

	SQLStore()
	{
		m_maxentry = 0;
	}

	INLINED uint32 GetMaxEntry() { return m_maxentry; }
	
	void Reload()
	{
		Load(m_tablename.c_str());
	}

	void Insert(uint32 index, T* data)
	{
		m_lock.AcquireWriteLock();
		if (index > m_maxentry)
			m_maxentry = index;
		m_data.insert(std::make_pair(index, data));
		m_lock.ReleaseWriteLock();
	}

	void Erase(uint32 index)
	{
		m_lock.AcquireWriteLock();
		m_data.erase(index);
		m_lock.ReleaseWriteLock();
	}

	~SQLStore()
	{
		for (typename unordered_map<uint32, T*>::iterator itr=m_data.begin(); itr!=m_data.end(); ++itr)
		{
			char* structpointer = (char*)itr->second;
			uint32 offset = 0;

			for (std::vector<uint32>::iterator itr2 = m_fieldtypes.begin(); itr2 != m_fieldtypes.end(); ++itr2)
			{
				switch ((*itr2) & 0x00FFFFFF)
				{
				case MYSQL_TYPE_STRING:
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_BLOB:
					free(*(char**)&structpointer[offset]);
					offset += sizeof(char*);
					break;
				case MYSQL_TYPE_LONG:
					{
						uint32 flags = (*itr2) >> 24;
						if (flags & UNSIGNED_FLAG)
						{
							offset += sizeof(uint32);
						}
						else
						{
							offset += sizeof(int32);
						}
						break;
					}
				case MYSQL_TYPE_FLOAT:
					offset += sizeof(float);
					break;
				default:
					ASSERT(false); break;
				}
			}
		}
	}

	void Save(T* data)
	{
		char* structpointer = (char*)data;
		//yes im so fucking lazy im using my stack buffer
		CREATE_STACK_BUFFER(buffer, 15000);
		buffer.AddVar("replace into `%s` VALUES(", m_tablename.c_str());

		uint32 offset = 0;
		uint32 valuecounter = 0;

		for (std::vector<uint32>::iterator itr2 = m_fieldtypes.begin(); itr2 != m_fieldtypes.end(); ++itr2)
		{
			if (valuecounter > 0)
				buffer << ",";
			++valuecounter;
			switch ((*itr2) & 0x00FFFFFF)
			{
			case MYSQL_TYPE_STRING:
			case MYSQL_TYPE_VAR_STRING:
			case MYSQL_TYPE_BLOB:
				{
					char* strbuf = *(char**)&structpointer[offset];
					if (strbuf == NULL) //eek
						buffer.AddVar("\"\"");
					else
						buffer.AddVar("\"%s\"", *(char**)&structpointer[offset]);
					offset += sizeof(char*);
				}
				break;
			case MYSQL_TYPE_LONG:
				{
					uint32 flags = (*itr2) >> 24;
					if (flags & UNSIGNED_FLAG)
					{
						buffer.AddVar("%u", *(uint32*)&structpointer[offset]);
						offset += sizeof(uint32);
					}
					else
					{
						buffer.AddVar("%d", *(uint32*)&structpointer[offset]);
						offset += sizeof(int32);
					}
					break;
				}
			case MYSQL_TYPE_FLOAT:
				buffer.AddVar("%f", *(float*)&structpointer[offset]);
				offset += sizeof(float);
				break;
			default:
				ASSERT(false); break;
			}
		}
		buffer << ");";
		buffer << uint8(0);
		sDatabase.Execute(buffer.GetCBuffer());
		sLog.Debug("TEST", "%s", buffer.GetCBuffer());
	}

	void Save( uint32 entry )
	{
		unordered_map<uint32, T*>::iterator itr = m_data.find(entry);
		if (itr == m_data.end())
			return;
		Save(itr->second);
	}

	void Load(const char* table)
	{
		char* query = new char[2048];
		std::string escapedtable;
		sDatabase.Escape(table, escapedtable);
		sprintf(query, "SELECT * from `%s`;", escapedtable.c_str());
		m_tablename = escapedtable;
		MYSQL_RES* res = sDatabase.Query(query);

		delete[] query;

		if (res == NULL)
			return;

		sLog.Debug("SQLStore", "Loading table %s", table);

		MYSQL_FIELD* fields = mysql_fetch_fields(res);
		uint32 numfields = mysql_num_fields(res);

		ASSERT(numfields > 0);

		size_t expectedsize = 0;

		m_fieldtypes.clear();
		for (uint32 i=0; i<numfields; ++i)
		{
			m_fieldtypes.push_back(fields[i].type | fields[i].flags << 24);

			switch (fields[i].type)
			{
			case MYSQL_TYPE_FLOAT:
				{
					expectedsize += sizeof(long); break;
				}
			case MYSQL_TYPE_LONG:
				{
					if (fields[i].flags & UNSIGNED_FLAG)
						expectedsize += sizeof(uint32);
					else
						expectedsize += sizeof(int32);
					break;
				}
			case MYSQL_TYPE_STRING:
			case MYSQL_TYPE_VAR_STRING:
			case MYSQL_TYPE_BLOB:
				{
					expectedsize += sizeof(char*); break;
				}
			}
		}

		ASSERTVA(sizeof(T) >= expectedsize, "sizeof(T): %u, expectedsize: %u", sizeof(T), expectedsize);

		uint32 num_rows = 0;

		while (MYSQL_ROW row = sDatabase.Fetch(res))
		{
			++num_rows;
			uint32 col = 0;
			uint32 key = atol(row[0]);

			if (m_maxentry < key)
				m_maxentry = key;

			unordered_map<uint32, T*>::iterator lookupitr = m_data.find(key);
			char* structpointer = NULL;
			if (lookupitr != m_data.end())
				structpointer = (char*)lookupitr->second;
			else
			{
				T* obj = new T;
				structpointer = (char*)obj;
				m_data.insert(std::make_pair(key, obj));
			}

			uint32 offset = 0;

			for (std::vector<uint32>::iterator itr = m_fieldtypes.begin(); itr != m_fieldtypes.end(); ++itr)
			{
				switch ((*itr) & 0x00FFFFFF)
				{
				case MYSQL_TYPE_STRING:
				case MYSQL_TYPE_VAR_STRING:
				case MYSQL_TYPE_BLOB:
					*(char**)&structpointer[offset] = strdup(row[col]);
					offset += sizeof(char*);
					break;
				case MYSQL_TYPE_LONG:
					{
						uint32 flags = (*itr) >> 24;
						if (flags & UNSIGNED_FLAG)
						{
							*(uint32*)&structpointer[offset] = atol(row[col]);
							offset += sizeof(uint32);
						}
						else
						{
							*(int32*)&structpointer[offset] = atol(row[col]);
							offset += sizeof(int32);
						}
						break;
					}
				case MYSQL_TYPE_FLOAT:
					*(float*)&structpointer[offset] = (float)atof(row[col]);
					offset += sizeof(float);
					break;
				default:
					ASSERT(false); break;
				}
				++col;
			}
		}

		sLog.Debug("SQLStore", "Loaded %u rows from %s", num_rows, table);
	}

	T* GetEntry(uint32 entry)
	{
		m_lock.AcquireReadLock();
		typename unordered_map<uint32, T*>::iterator itr = m_data.find(entry);
		if (itr != m_data.end())
		{
			T* ret = itr->second;
			m_lock.ReleaseReadLock();
			return ret;
		}
		m_lock.ReleaseReadLock();
		return NULL;
	}
};

#endif
