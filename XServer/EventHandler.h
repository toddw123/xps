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

#ifndef __EVENT_H
#define __EVENT_H

#include "EventMgr.h"

class Event;
class EventableObject;

extern THREAD_LOCAL Event* t_currentEvent;

enum EventFlags
{
	EVENT_FLAG_NONE,
	EVENT_FLAG_NOT_IN_WORLD,
};

class EventHolder
{
public:
	std::set<Event*> m_events;
	std::set<Event*> m_insertpool;
	Mutex m_lock;
	void InitHolder();

	void AddObject(EventableObject* e);
	void AddHolderEvent(Event* e);
	virtual int32 GetInstanceID() { return -1; }

	void EventUpdate();
};

enum EventTypes;
class EventableObject : public MemoryBlock, public RefCounterBase
{
public:
	std::set<Event*> m_events;
	bool m_destroyed;

	EventableObject()
	{
		m_destroyed = false;
	}
	~EventableObject();

	FastMutex m_eventLock;
	bool AddEvent(Event* e);

	bool HasEvent(uint32 type);
	Event* GetEvent(uint32 type);

	void GetEvent(uint32 type, std::vector<Event*> & events);

	void Destroy();

	virtual int32 GetInstanceID() { return -1; }


	//stuff for adding events ;)
	template<class C>
	void AddEvent(void(C::*method)(), EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		AddEvent(MakeEvent(method, type, delay, repeats, flags));
	}
	template<class C, typename P1>
	void AddEvent(void(C::*method)(P1), P1 p1, EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		AddEvent(MakeEvent(method, p1, type, delay, repeats, flags));
	}
	template<class C, typename P1, typename P2>
	void AddEvent(void(C::*method)(P1, P2), P1 p1, P2 p2, EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		AddEvent(MakeEvent(method, p1, p2, type, delay, repeats, flags));
	}
	template<class C, typename P1, typename P2,  typename P3>
	void AddEvent(void(C::*method)(P1, P2, P3), P1 p1, P2 p2, P3 p3, EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		AddEvent(MakeEvent(method, p1, p2, p3, type, delay, repeats, flags));
	}

	template<class C, typename P1, typename P2,  typename P3, typename P4>
	void AddEvent(void(C::*method)(P1, P2, P3, P4), P1 p1, P2 p2, P3 p3, P4 p4, EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		AddEvent(MakeEvent(method, p1, p2, p3, p4, type, delay, repeats, flags));
	}

	template<class C, typename P1, typename P2,  typename P3, typename P4, typename P5>
	void AddEvent(void(C::*method)(P1, P2, P3, P4), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		AddEvent(MakeEvent(method, p1, p2, p3, p4, p5, type, delay, repeats, flags));
	}

	template<class C>
	Event* MakeEvent(void(C::*method)(), EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		CallBackBase* cb = new CallBackP0<C>((C*)this, method);
		Event* ev = new Event((C*)this, cb, type, delay, repeats, flags);
		return ev;
	}
	template<class C, typename P1>
	Event* MakeEvent(void(C::*method)(P1), P1 p1, EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		CallBackBase* cb = new CallBackP1<C, P1>((C*)this, method, p1);
		Event* ev = new Event((C*)this, cb, type, delay, repeats, flags);
		return ev;
	}
	template<class C, typename P1, typename P2>
	Event* MakeEvent(void(C::*method)(P1, P2), P1 p1, P2 p2, EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		CallBackBase* cb = new CallBackP2<C, P1, P2>((C*)this, method, p1, p2);
		Event* ev = new Event((C*)this, cb, type, delay, repeats, flags);
		return ev;
	}
	template<class C, typename P1, typename P2,  typename P3>
	Event* MakeEvent(void(C::*method)(P1, P2, P3), P1 p1, P2 p2, P3 p3, EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		CallBackBase* cb = new CallBackP3<C, P1, P2, P3>((C*)this, method, p1, p2, p3);
		Event* ev = new Event((C*)this, cb, type, delay, repeats, flags);
		return ev;
	}
	template<class C, typename P1, typename P2,  typename P3, typename P4>
	Event* MakeEvent(void(C::*method)(P1, P2, P3, P4), P1 p1, P2 p2, P3 p3, P4 p4, EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		CallBackBase* cb = new CallBackP4<C, P1, P2, P3, P4>((C*)this, method, p1, p2, p3, p4);
		Event* ev = new Event((C*)this, cb, type, delay, repeats, flags);
		return ev;
	}
	template<class C, typename P1, typename P2,  typename P3, typename P4, typename P5>
	Event* MakeEvent(void(C::*method)(P1, P2, P3, P4), P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, EventTypes type, uint32 delay, uint32 repeats, uint32 flags)
	{
		CallBackBase* cb = new CallBackP5<C, P1, P2, P3, P4, P5>((C*)this, method, p1, p2, p3, p4, p5);
		Event* ev = new Event((C*)this, cb, type, delay, repeats, flags);
		return ev;
	}

	void RemoveEvents();
	void RemoveEvents(EventTypes type);
};

enum EventTypes;
class Event
{
public:
	bool deleted;
	bool active;
	volatile long m_references;
	EventableObject* obj;
	CallBackBase* cb;

	uint32 m_delay;
	uint32 m_repeats;
	uint32 m_flags;
	uint32 m_firedelay;
	EventTypes m_eventType;
	Event(EventableObject* o, CallBackBase* c, EventTypes type, uint32 delay, uint32 repeats, uint32 flags) : obj(o), cb(c), m_delay(delay), m_repeats(repeats), m_flags(flags), m_references(0), m_firedelay(g_mstime + delay), m_eventType(type), deleted(false)
	{
		active = false;
		o->m_eventLock.Acquire();
		o->m_events.insert(this);
		o->m_eventLock.Release();
		o->AddRef();
	}

	INLINED void AddRef() { Sync_Add(&m_references); }
	INLINED void DecRef()
	{
		if (Sync_Sub(&m_references) == 0)
		{
			deleted = true;
			delete cb;
			delete this;
		}
	}

	void ResetEventTimer();

	void Remove() { deleted = true; }

	~Event()
	{
		obj->m_eventLock.Acquire();
		obj->m_events.erase(this);
		obj->m_eventLock.Release();
		obj->DecRef();
	}
};

#endif
