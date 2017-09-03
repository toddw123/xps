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

#ifndef LOGGER_H
#define LOGGER_H

#define sLog Log::getSingleton()

#ifdef WIN32

#define TRED FOREGROUND_RED | FOREGROUND_INTENSITY
#define TGREEN FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define TYELLOW FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY
#define TNORMAL FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE
#define TWHITE TNORMAL | FOREGROUND_INTENSITY
#define TBLUE FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY

#else

#define TRED 1
#define TGREEN 2
#define TYELLOW 3
#define TNORMAL 4
#define TWHITE 5
#define TBLUE 6

#endif

#define LOG_ACTION(action, info, ...) sDatabase.Execute("insert into log_data VALUES(%u, \"%s\", \"%s\", \""info"\");", (uint32)g_time, "N/A", action, __VA_ARGS__)
#define LOG_ACTION_I(u, info, ...) { char buf[256]; sprintf(buf, "it%llu", u->m_itemguid); sDatabase.Execute("insert into log_data VALUES(%u, \"%s\", \"%s\", \""info"\");", (uint32)g_time, buf, "ILOG", __VA_ARGS__); }
#define LOG_ACTION_U(u, action, info, ...) sDatabase.Execute("insert into log_data VALUES(%u, \"%s\", \"%s\", \""info"\");", (uint32)g_time, u == NULL? "N/A" : TO_UNIT(u)->m_namestr.c_str(), action, __VA_ARGS__)

class Account;
class Log : public Singleton<Log>
{
	Mutex m_lock;
	uint8 m_loglevel;
#ifdef WIN32
	HANDLE m_handle_stdout, m_handle_stderr;
#endif
	FILE* m_chatlog;
	FILE* m_worldlog;
public:
	Log();

	void Color(uint16 color) //windows color attributes use 16bits
	{
#ifndef WIN32
		static const char* colorstrings[TBLUE+1] = {
			"",
			"\033[22;31m",
			"\033[22;32m",
			"\033[01;33m",
			"\033[0m",
			"\033[01;37m",
			"\033[1;34m",
		};
		fputs(colorstrings[color], stdout);
#else
		SetConsoleTextAttribute(m_handle_stdout, (WORD)color);
#endif
	}

	void GenerateTimeString(std::string* dest)
	{
		char c[128];
		sprintf(c, "[%02u:%02u]", g_localtime.tm_hour, g_localtime.tm_min);
		*dest = (char*)c;
	}

	void Notice(const char* source, const char* format, ...);
	void Error(const char* source, const char* format, ...);
	void Warning(const char* source, const char* format, ...);
	void Debug(const char* source, const char* format, ...);

	void File(const char* source, const char* format, ...);
	void FlushChatLog() { fflush(m_chatlog); }
	void LogPacket(ServerPacket* p, Account* acc);

	size_t WriteToFile(FILE *dstFile, const void *pSource, size_t sourceLength)
	{
		fprintf(dstFile, "|------------------------------------------------|----------------|\r\n");
		fprintf(dstFile, "|00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |0123456789ABCDEF|\r\n");
		fprintf(dstFile, "|------------------------------------------------|----------------|\r\n");

		size_t i = 0;
		size_t c = 0;
		size_t start;
		size_t written;
		uint8 byte;
		const unsigned char *pData = (const unsigned char *)pSource;

		for( ; i < sourceLength; )
		{
			start = i;
			fprintf(dstFile, "|");
			for( c = 0; c < 16 && i < sourceLength; )		// write 16 bytes per line
			{
				fprintf(dstFile, "%02X ", (int)pData[i]);
				++i; ++c;
			}

			written = c;
			for( ; c < 16; ++c )							// finish off any incomplete bytes
				fprintf(dstFile, "   ");

			// write the text part
			fprintf(dstFile, "|");
			for( c = 0; c < written; ++c )
			{
				byte = pData[start + c];
				if( isprint((int)byte) )
					fprintf(dstFile, "%c", (int)byte);
				else
					fprintf(dstFile, ".");
			}

			for( ; c < 16; ++c )
				fprintf(dstFile, " ");

			fprintf(dstFile, "|\r\n");
		}

		fprintf(dstFile, "-------------------------------------------------------------------\r\n");
		return 0;
	}

	INLINED void GetLock() { m_lock.Acquire(); }
	INLINED void ReleaseLock() { m_lock.Acquire(); }
};

#endif
