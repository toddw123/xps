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

initialiseSingleton(Log);

Log::Log()
{
	// Create the date/time string
	time_t curtime = time(NULL);
	tm * pTime = localtime(&curtime);
	char filename[MAX_PATH];

	sprintf(filename, "logs\\ChatLog-%u-%u-%u-%u-%u-%u-%u.txt",
		pTime->tm_year+1900, pTime->tm_mon+1, pTime->tm_mday,
		pTime->tm_hour, pTime->tm_min, pTime->tm_sec, GetCurrentThreadId());

	m_chatlog = fopen(filename, "wb");

	sprintf(filename, "logs\\WorldLog-%u-%u-%u-%u-%u-%u-%u.txt",
		pTime->tm_year+1900, pTime->tm_mon+1, pTime->tm_mday,
		pTime->tm_hour, pTime->tm_min, pTime->tm_sec, GetCurrentThreadId());

	m_worldlog = fopen(filename, "wb");

	m_loglevel = m_config.GetIntDefault("Log", "Level", 0);

#ifdef WIN32
	m_handle_stderr = GetStdHandle(STD_ERROR_HANDLE);
	m_handle_stdout = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
}

void Log::Notice(const char* source, const char* format, ...)
{
	m_lock.Acquire();
	va_list ap;
	va_start(ap, format);
	std::string time;
	GenerateTimeString(&time);
	fputs(time.c_str(), stdout);
	fputs(" N ", stdout);
	if(*source)
	{
		Color(TWHITE);
		fputs(source, stdout);
		putchar(':');
		putchar(' ');
		Color(TNORMAL);
	}

	vprintf(format, ap);
	putchar('\n');
	va_end(ap);
	Color(TNORMAL);

	m_lock.Release();
}

void Log::Error(const char* source, const char* format, ...)
{
	if (m_loglevel < 1)
		return;

	m_lock.Acquire();
	va_list ap;
	va_start(ap, format);
	std::string time;
	GenerateTimeString(&time);
	fputs(time.c_str(), stdout);
	Color(TRED);
	fputs(" E ", stdout);
	if(*source)
	{
		Color(TWHITE);
		fputs(source, stdout);
		putchar(':');
		putchar(' ');
		Color(TRED);
	}

	vprintf(format, ap);
	putchar('\n');
	va_end(ap);
	Color(TNORMAL);

	m_lock.Release();
}
void Log::Warning(const char* source, const char* format, ...)
{
	if (m_loglevel < 1)
		return;
	m_lock.Acquire();
	va_list ap;
	va_start(ap, format);
	std::string time;
	GenerateTimeString(&time);
	fputs(time.c_str(), stdout);
	Color(TYELLOW);
	fputs(" W ", stdout);
	if(*source)
	{
		Color(TWHITE);
		fputs(source, stdout);
		putchar(':');
		putchar(' ');
		Color(TYELLOW);
	}

	vprintf(format, ap);
	putchar('\n');
	va_end(ap);
	Color(TNORMAL);

	m_lock.Release();
}

void Log::Debug(const char* source, const char* format, ...)
{
	if (m_loglevel < 2)
		return;
	m_lock.Acquire();
	va_list ap;
	va_start(ap, format);
	std::string time;
	GenerateTimeString(&time);
	fputs(time.c_str(), stdout);
	Color(TBLUE);
	fputs(" D ", stdout);
	if(*source)
	{
		Color(TWHITE);
		fputs(source, stdout);
		putchar(':');
		putchar(' ');
		Color(TBLUE);
	}

	vprintf(format, ap);
	putchar('\n');
	va_end(ap);
	Color(TNORMAL);

	m_lock.Release();
}

void Log::File( const char* source, const char* format, ... )
{
	if (m_loglevel < 2)
		return;
	m_lock.Acquire();
	va_list ap;
	va_start(ap, format);
	std::string time;
	GenerateTimeString(&time);

	fprintf(m_chatlog, "%s %s: ", time.c_str(), source);
	vfprintf(m_chatlog, format, ap);
	fprintf(m_chatlog, "\r\n");
	va_end(ap);
	m_lock.Release();
}

void Log::LogPacket( ServerPacket* p, Account* acc )
{
	uint8* buffer = p->GetBuffer() + 1; //ignore opcode
	size_t len = p->GetSize() - 1; //ignore opcode...
	fprintf(m_worldlog, "\r\nOPCODE: %02X Account: %u\r\n", p->GetBuffer()[0], acc? acc->m_data->Id : 0);
	WriteToFile(m_worldlog, buffer, len);
}
