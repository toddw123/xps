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

#ifndef __STACKBUFFER_H
#define __STACKBUFFER_H

#define CREATE_STACK_BUFFER(name, size) uint8 buffer__##name[size]; PacketBuffer name((uint8*)&buffer__##name, size);

#define PACKET_DEFAULT_SIZE 1500

class PacketBuffer
{
	uint8* m_buffer;
	uint32 m_pos;
	uint32 m_size;
	bool heaped;

public:
	PacketBuffer(uint8* buf, size_t size)
	{
		m_buffer = buf;
		m_pos = 0;
		heaped = false;
		m_size = size;

		if (buf == NULL)
			InitHeap(size);
	}

	PacketBuffer()
	{
		m_pos = 0;
		heaped = false;
		m_size = PACKET_DEFAULT_SIZE;
		InitHeap(m_size);
	}

	~PacketBuffer() { if (heaped) free(m_buffer); }

	INLINED void InitHeap(uint32 size)
	{
		if (!heaped)
			m_buffer = (uint8*)malloc(size);
		else
			m_buffer = (uint8*)realloc(m_buffer, size);
		heaped = true;
		m_size = size;
	}

	INLINED void MoveStackToHeap()
	{
		uint8* stackbuffer = m_buffer;
		InitHeap(m_pos);
		memcpy(m_buffer, stackbuffer, m_pos);
	}

	INLINED void CheckSize(size_t newsize)
	{
		if (m_pos + newsize > m_size)
		{
			if (!heaped)
			{
				sLog.Warning("StackBuffer", "Not enough space on stack, reallocating on heap.");
				MoveStackToHeap();
			}
			sLog.Warning("StackBuffer", "Not enough space in buffer, allocating %u bytes.", m_size - m_pos + newsize);
			InitHeap(m_pos + newsize);
		}
	}
	
	INLINED uint8* DetachBuffer()
	{
		//dont detach stack
		if (!heaped)
			return NULL;
		m_pos = 0;
		m_size = 0;
		uint8* buf = m_buffer;
		m_buffer = NULL;
		return buf;
	}

	INLINED void ResetPointer() { m_pos = 0; }

	INLINED PacketBuffer& operator << (const std::string & str)
	{
		CheckSize(str.size());
		memcpy(&m_buffer[m_pos], (void*)str.c_str(), str.size());
		m_pos += str.size();
		return *this;
	}

	INLINED PacketBuffer& operator << (const char* c)
	{
		CheckSize(strlen(c));
		memcpy(&m_buffer[m_pos], (void*)c, strlen(c));
		m_pos += strlen(c);
		return *this;
	}

	INLINED PacketBuffer& operator << (char* c)
	{
		CheckSize(strlen(c));
		memcpy(&m_buffer[m_pos], (void*)c, strlen(c));
		m_pos += strlen(c);
		return *this;
	}

#define DEFINE_QUICK_COPY(thetype) INLINED PacketBuffer & operator << (const thetype v)\
	{\
		CheckSize(sizeof(v));\
		*(thetype*)&m_buffer[m_pos] = v;\
		m_pos += sizeof(v);\
		return *this;\
	}

	DEFINE_QUICK_COPY(uint8);
	DEFINE_QUICK_COPY(uint16);
	DEFINE_QUICK_COPY(uint32);

	INLINED PacketBuffer& operator << (PacketBuffer & oldbuffer)
	{
		CheckSize(oldbuffer.GetSize());
		memcpy(&m_buffer[m_pos], oldbuffer.GetBuffer(), oldbuffer.GetSize());
		m_pos += oldbuffer.GetSize();
		return *this;
	}

	INLINED PacketBuffer& operator << (PacketBuffer* oldbuffer)
	{
		CheckSize(oldbuffer->GetSize());
		memcpy(&m_buffer[m_pos], oldbuffer->GetBuffer(), oldbuffer->GetSize());
		m_pos += oldbuffer->GetSize();
		return *this;
	}

	INLINED uint32 GetSize() { return m_pos; }
	INLINED uint8* GetBuffer() { return m_buffer; }
	INLINED char* GetCBuffer() { return (char*)m_buffer; }

	void AddVar(char* format, ...)
	{
		char stuff[16384];

		va_list vlist;
		va_start(vlist, format);
		vsnprintf(stuff, 16384, format, vlist);
		va_end(vlist);

		*this << (char*)&stuff;
	}
};

#endif