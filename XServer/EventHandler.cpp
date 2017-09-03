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

initialiseSingleton(EventMgr);

THREAD_LOCAL Event* t_currentEvent;

void EventHolder::AddObject(EventableObject* e)
{
	//move events to us
	e->m_eventLock.Acquire();
	for (std::set<Event*>::iterator itr=e->m_events.begin(); itr!=e->m_events.end(); ++itr)
	{
		Event* ev = (*itr);
		if (!ev->active)
			continue;
		ev->AddRef();
		m_lock.Acquire();
		m_insertpool.insert(ev);
		m_lock.Release();
	}
	e->m_eventLock.Release();
}

void EventHolder::EventUpdate()
{
	m_lock.Acquire();
	std::set<Event*>::iterator itr = m_insertpool.begin(), itr2 = m_insertpool.end();
	while (itr != m_insertpool.end())
	{
		itr2 = itr++;
		Event* ev = *itr2;
		if (ev->deleted)
		{
			m_insertpool.erase(itr2);
			ev->DecRef();
			continue;
		}
		m_events.insert(ev);
		m_insertpool.erase(itr2);
	}
	m_lock.Release();

	//update events
	itr = m_events.begin();

	while (itr != m_events.end())
	{
		itr2 = itr++;
		Event* ev = (*itr2);

		if (ev->deleted || ev->obj == NULL)
		{
			m_events.erase(itr2);
			ev->DecRef();
			continue;
		}

		if (ev->obj->GetInstanceID() != GetInstanceID())
		{
			EventHolder* holder = sEventMgr.GetHolder(ev->obj->GetInstanceID());

			if (holder != NULL && holder->m_lock.AttemptAcquire())
			{
				holder->AddHolderEvent(ev);
				m_events.erase(itr2);
				ev->DecRef();
				holder->m_lock.Release();
			}
			continue;
		}

		if (ev->m_flags & EVENT_FLAG_NOT_IN_WORLD && GetInstanceID() == -1)
			continue;

		//not ready to fire
		if (ev->m_firedelay > g_mstime)
			continue;

		//reset fire delay as were going to fire, PEWPEW
		ev->m_firedelay += ev->m_delay;
		t_currentEvent = ev;
		ev->cb->exe();
		t_currentEvent = NULL;

		if (ev->m_repeats != 0)
		{
			//have we done all our repeats?
			if (!(--ev->m_repeats))
			{
				ev->deleted = true;
				m_events.erase(itr2);
				ev->DecRef();
			}
		}
	}
}

void EventHolder::InitHolder()
{
	sEventMgr.AddHolder(this);
}

void EventHolder::AddHolderEvent( Event* e )
{
	e->AddRef();
	m_lock.Acquire();
	m_insertpool.insert(e);
	m_lock.Release();
}

bool EventableObject::HasEvent( uint32 type )
{
	m_eventLock.Acquire();
	for (std::set<Event*>::iterator itr=m_events.begin(); itr!=m_events.end(); ++itr)
	{
		if ((*itr)->m_eventType == type)
		{
			m_eventLock.Release();
			return true;
		}
	}
	m_eventLock.Release();
	return false;
}

Event* EventableObject::GetEvent( uint32 type )
{
	m_eventLock.Acquire();
	for (std::set<Event*>::iterator itr=m_events.begin(); itr!=m_events.end(); ++itr)
	{
		if ((*itr)->m_eventType == type)
		{
			Event* e = *itr;
			e->AddRef();
			m_eventLock.Release();
			return e;
		}
	}
	m_eventLock.Release();
	return NULL;
}

void EventableObject::GetEvent( uint32 type, std::vector<Event*> & events )
{
	m_eventLock.Acquire();
	for (std::set<Event*>::iterator itr=m_events.begin(); itr!=m_events.end(); ++itr)
	{
		if ((*itr)->m_eventType == type)
		{
			Event* e = *itr;
			e->AddRef();
			events.push_back(e);
		}
	}
	m_eventLock.Release();
}

EventableObject::~EventableObject()
{
	m_eventLock.Acquire();
	for (std::set<Event*>::iterator itr=m_events.begin(); itr!=m_events.end(); ++itr)
	{
		(*itr)->deleted = true;
	}
	m_eventLock.Release();
}

bool EventableObject::AddEvent( Event* e )
{
	m_eventLock.Acquire();

	if (m_destroyed)
	{
		//a delete has been requested and we can't add events now
		AddRef(); //dont allow deletion here ;)
		delete e;
		m_eventLock.Release();
		DecRef();
		return false;
	}

	e->active = true;
	m_events.insert(e);
	m_eventLock.Release();
	EventHolder* holder = sEventMgr.GetHolder(GetInstanceID());

	if (holder == NULL)
		return false;

	holder->AddHolderEvent(e);
	return true;
}

void EventableObject::Destroy()
{
	m_eventLock.Acquire();
	m_destroyed = true;
	RemoveEvents();
	m_eventLock.Release();
}

void EventableObject::RemoveEvents()
{
	m_eventLock.Acquire();
	for (std::set<Event*>::iterator itr = m_events.begin(); itr != m_events.end(); ++itr)
		(*itr)->deleted = true;
	m_eventLock.Release();
}

void EventableObject::RemoveEvents( EventTypes type )
{
	m_eventLock.Acquire();
	for (std::set<Event*>::iterator itr = m_events.begin(); itr != m_events.end(); ++itr)
		if ((*itr)->m_eventType == type)
			(*itr)->deleted = true;
	m_eventLock.Release();
}

void Event::ResetEventTimer()
{
	m_firedelay = g_mstime + m_delay;
}