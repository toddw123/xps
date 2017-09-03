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

#ifndef __CALLBACKS_H
#define __CALLBACKS_H

class CallBackBase
{
public:
	virtual void exe() {};
	virtual ~CallBackBase() {};
};


template <class C> class CallBackP0 : public CallBackBase
{
public:
	typedef void (C::*Method)();
	CallBackP0(C* obj, Method method)
	{
		m_object = obj;
		m_method = method;
	}
	void operator()() { return (m_object->*m_method)(); }
	void exe() { return operator()(); }
private:
	C* m_object;
	Method m_method;
};

template <class C, typename P1> class CallBackP1 : public CallBackBase
{
public:
	typedef void (C::*Method)(P1);
	CallBackP1(C* obj, Method method, P1 p1)
	{
		m_object = obj;
		m_method = method;
		m_p1 = p1;
	}
	void operator()() { return (m_object->*m_method)(m_p1); }
	void exe() { return operator()(); }
private:
	C* m_object;
	Method m_method;
	P1 m_p1;
};

template <class C, typename P1, typename P2> class CallBackP2 : public CallBackBase
{
public:
	typedef void (C::*Method)(P1, P2);
	CallBackP2(C* obj, Method method, P1 p1, P2 p2)
	{
		m_object = obj;
		m_method = method;
		m_p1 = p1;
		m_p2 = p2;
	}
	void operator()() { return (m_object->*m_method)(m_p1, m_p2); }
	void exe() { return operator()(); }

private:
	C* m_object;
	Method m_method;
	P1 m_p1;
	P2 m_p2;
};

template <class C, typename P1, typename P2, typename P3> class CallBackP3 : public CallBackBase
{
public:
	typedef void (C::*Method)(P1, P2, P3);
	CallBackP3(C* obj, Method method, P1 p1, P2 p2, P3 p3)
	{
		m_object = obj;
		m_method = method;
		m_p1 = p1;
		m_p2 = p2;
		m_p3 = p3;
	}
	void operator()() { return (m_object->*m_method)(m_p1, m_p2, m_p3); }
	void exe() { return operator()(); }

private:
	C* m_object;
	Method m_method;
	P1 m_p1;
	P2 m_p2;
	P3 m_p3;
};

template <class C, typename P1, typename P2, typename P3, typename P4> class CallBackP4 : public CallBackBase
{
public:
	typedef void (C::*Method)(P1, P2, P3, P4);
	CallBackP4(C* obj, Method method, P1 p1, P2 p2, P3 p3, P4 p4)
	{
		m_object = obj;
		m_method = method;
		m_p1 = p1;
		m_p2 = p2;
		m_p3 = p3;
		m_p4 = p4;
	}
	void operator()() { return (m_object->*m_method)(m_p1, m_p2, m_p3, m_p4); }
	void exe() { return operator()(); }

private:
	C* m_object;
	Method m_method;
	P1 m_p1;
	P2 m_p2;
	P3 m_p3;
	P4 m_p4;
};

template <class C, typename P1, typename P2, typename P3, typename P4, typename P5> class CallBackP5 : public CallBackBase
{
public:
	typedef void (C::*Method)(P1, P2, P3, P4, P5);
	CallBackP5(C* obj, Method method, P1 p1, P2 p2, P3 p3, P4 p4, P5 p5)
	{
		m_object = obj;
		m_method = method;
		m_p1 = p1;
		m_p2 = p2;
		m_p3 = p3;
		m_p4 = p4;
		m_p5 = p5;
	}
	void operator()() { return (m_object->*m_method)(m_p1, m_p2, m_p3, m_p4, m_p5); }
	void exe() { return operator()(); }

private:
	C* m_object;
	Method m_method;
	P1 m_p1;
	P2 m_p2;
	P3 m_p3;
	P4 m_p4;
	P5 m_p5;
};

#endif
