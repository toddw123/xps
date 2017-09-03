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

#ifndef __CONCURRENT_CONTAINER_H
#define __CONCURRENT_CONTAINER_H

template <class C> class ConncurentContainerIterator
{
private:
	RWLock* m_holdinglock;
public:
	typename C::iterator m_itr;


	ConncurentContainerIterator(RWLock* locktohold, typename C::iterator & itr)
	{
		m_itr = itr;
		m_holdinglock = locktohold;
		if (locktohold != NULL)
			locktohold->AcquireReadLock();
	}

	~ConncurentContainerIterator()
	{
		if (m_holdinglock != NULL)
			m_holdinglock->ReleaseReadLock();
	}

	bool operator ==(const ConncurentContainerIterator<C> & rhs)
	{
		return m_itr == rhs.m_itr;
	}
	bool operator !=(const ConncurentContainerIterator<C> & rhs)
	{
		return m_itr != rhs.m_itr;
	}

	ConncurentContainerIterator<C> & operator ++ ()
	{
		++m_itr;
		return *this;
	}

	ConncurentContainerIterator<C> & operator ++ (int)
	{
		ConncurentContainerIterator<C> ret(NULL, m_itr);
		ret.m_itr++;
		return ret;
	}
};

template <typename A, typename B> class ConcurrentMap
{
private:
	std::map<A, B> m_container;
	RWLock m_lock;
public:
	typedef ConncurentContainerIterator<std::map<A, B>> iterator;

	iterator begin() { return iterator(&m_lock, m_container.begin()); }
	iterator end() { return iterator(NULL, m_container.end()); }
	iterator find(const A & key) { return iterator(&m_lock, m_container.find(key)); }
	void insert(const typename std::map<A, B>::value_type & val) { m_lock.AcquireReadLock(); m_container.insert(val); m_lock.ReleaseReadLock(); }
	void erase(const A & key) { m_lock.AcquireWriteLock(); m_container.erase(key); m_lock.ReleaseWriteLock(); }
	void erase(iterator & itr) { m_lock.ReadToWrite(); m_container.erase(itr.m_itr); m_lock.WriteToRead(); }
	size_t size() { return m_container.size(); }
	void clear() { m_lock.AcquireWriteLock(); m_container.clear(); m_lock.ReleaseWriteLock(); }
};

template <typename A, typename B> class ConcurrentUnorderedMap
{
private:
	unordered_map<A, B> m_container;
	RWLock m_lock;
public:
	typedef ConncurentContainerIterator<unordered_map<A, B>> iterator;
	typedef typename unordered_map<A, B>::value_type value_type;

	iterator begin() { return iterator(&m_lock, m_container.begin()); }
	iterator end() { return iterator(NULL, m_container.end()); }
	iterator find(const A & key) { return iterator(&m_lock, m_container.find(key)); }
	//NOTE: unlike maps, a concurrent insert and read on a hash map gives an undefined result
	void insert(const value_type & val) { m_lock.AcquireWriteLock(); m_container.insert(val); m_lock.ReleaseWriteLock(); }
	void erase(const A & key) { m_lock.AcquireWriteLock(); m_container.erase(key); m_lock.ReleaseWriteLock(); }
	//itr holds a read lock so we can do funky stuff here and avoid deadlocks :P
	void erase(iterator & itr) { m_lock.ReadToWrite(); m_container.erase(itr.m_itr); m_lock.WriteToRead(); }
	size_t size() { return m_container.size(); }
	void clear() { m_lock.AcquireWriteLock(); m_container.clear(); m_lock.ReleaseWriteLock(); }
};

template <typename A> class ConcurrentSet
{
private:
	std::set<A> m_container;
	RWLock m_lock;
public:
	typedef ConncurentContainerIterator<std::set<A>> iterator;
	typedef typename std::set<A>::value_type value_type;

	iterator begin() { return iterator(&m_lock, m_container.begin()); }
	iterator end() { return iterator(NULL, m_container.end()); }
	iterator find(const A & key) { return iterator(&m_lock, m_container.find(key)); }
	//NOTE: unlike maps, a concurrent insert and read on a hash map gives an undefined result
	void insert(const value_type & val) { m_lock.AcquireWriteLock(); m_container.insert(val); m_lock.ReleaseWriteLock(); }
	void erase(const A & key) { m_lock.AcquireWriteLock(); m_container.erase(key); m_lock.ReleaseWriteLock(); }
	//itr holds a read lock so we can do funky stuff here and avoid deadlocks :P
	void erase(iterator & itr) { m_lock.ReadToWrite(); m_container.erase(itr.m_itr); m_lock.WriteToRead(); }
	size_t size() { return m_container.size(); }
};

template<typename A> class GrowableArray
{
public:
	A* buffer;
	uint32 numalloc;
	uint32 numused;

	GrowableArray(uint32 defaultsize = 1) { buffer = (A*)malloc(sizeof(A) * defaultsize); numalloc = defaultsize; numused = 0; }
	~GrowableArray() { free(buffer); }
	uint32 size() { return numused; }
	uint32 allocsize() { return numalloc; }

	INLINED void Alloc(uint32 newsize) { buffer = (A*)realloc(buffer, sizeof(A) * newsize); numalloc = newsize; }

	INLINED void push(const A & el)
	{
		if (numused >= numalloc)
			Alloc(numalloc < 1024? numalloc * 4 : numalloc * 2);
		buffer[numused++] = el;
	}

	INLINED void remove(A & el)
	{
		for (uint32 i=0; i<numused; ++i)
		{
			if (buffer[i] == el)
			{
				//ugh i hate this part
				--numused; //this saves a decrement on the loop
				for (uint32 j=i; j<numused; ++j)
					buffer[j] = buffer[j + 1];
				return;
			}
		}
	}

	INLINED const A & pop_back()
	{
		//lets be cheap, we don't have to free or destroy anything here
		return buffer[--numused];
	}

	INLINED void clear()
	{
		numused = 0;
	}

	INLINED void recreate(uint32 size)
	{
		clear();
		numalloc = size;
		buffer = (A*)realloc(buffer, sizeof(A) * size);
	}

	INLINED A & operator [](uint32 index) { ASSERT(index < numused); return buffer[index]; }
};

class RefCounterBase
{
protected:
	volatile long _refs;
public:
	RefCounterBase() { _refs = 1; }
	virtual ~RefCounterBase() {}
	void AddRef() { Sync_Add(&_refs); }
	void DecRef() { if (Sync_Sub(&_refs) == 0) delete this; }
};

template<typename T> class ref
{
protected:
	T* ptr;
public:
	ref(T* p) { ptr = p; if (ptr != NULL) ptr->AddRef(); }
	~ref() { if (ptr != NULL) ptr->DecRef(); }
	T* get() { return ptr; }
};

#endif
