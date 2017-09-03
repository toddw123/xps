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

#ifndef __SHARED_H
#define __SHARED_H

#define MAX_CONNECTIONS 15
#define HIGHID_PLAYER 0x7FFFFFFF
#define LOWID_PLAYER 0xFFFF

#define PACKET_BUFFER_SIZE 500 //amount of packets that can be buffered

#ifdef WIN32
	#define _WIN32_WINNT 0x500
	#define IOCP
	#include <winsock2.h>
	#include <Winsock.h>
	#include <mstcpip.h>
	#include <mswsock.h>
	#include <windows.h>
	#include <psapi.h>
	#include <intrin.h>
	#include <unordered_map>
	#include <unordered_set>
	#include <array>
	#include <DbgHelp.h>
	#include <direct.h>
	#define socklen_t int
	LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* ExceptionInfo);
	#define THREAD_LOCAL __declspec(thread)
#else
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <pthread.h>
	#include <tr1/unordered_map>
	#include <tr1/unordered_set>
	//fucking gcc
	#define strnicmp strncasecmp
	#define THREAD_LOCAL __thread
#endif

typedef unsigned char byte;
using namespace std;

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <deque>
#include <mysql/mysql.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
#include <sstream>
#include <algorithm>
#include <math.h>

#include "Defines.h"
#include "Stackwalker.h"
#include "CStackWalker.h"

//This is to remove GetObject winapi, conflicts with MapMgr::GetObject
#undef GetObject
//#include <system.h>

#include "shared_include.h"

#include "Recast.h"
#include "DetourCrowd.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshBuilder.h"
#include "DetourNavMeshQuery.h"
#include "DetourSeekBehavior.h"


#include "sha1.h"

#include "micropather/micropather.h"
#include "MemoryBlock.h"
#include "G3D/KDTree.h"

#include "MersenneTwister.h"
#include "MersenneTwister/randomc.h"

#include "PacketHandler.h"
#include "Config.h"
#include "Mutex.h"
#include "LockedDeque.h"
#include "ConcurrentContainer.h"
#include "ServerPacket.h"
#include "Logs.h"
#include "ThreadFuncs.h"
#include "ThreadPool.h"
#include "FileCrypt.h"
#include "Logger.h"
#include "Util.h"
#include "SQLStorage.h"
#include "EventMgr.h"
#include "World.h"
#include "InstanceMgr.h"
#include "MeshBuilder.h"
#include "WorldQuest.h"
#include "MapMgr.h"
#include "CallBacks.h"
#include "EventHandler.h"
#include "Launcher.h"
#include "GameClientSocket.h"
#include "Object.h"
#include "Session.h"
#include "Spell.h"
#include "SpellAura.h"
#include "TimedObject.h"
#include "Skills.h"
#include "Unit.h"
#include "Player.h"
#include "Creature.h"
#include "Item.h"
#include "Settings.h"
#include "Scripts.h"
#include "scripts/ScriptInclude.h"
#include "AccountManager.h"
#include "MySQL.h"

#include "Test.h"

#include "MapCell.h"

#include "SocketWorker.h"

#endif
