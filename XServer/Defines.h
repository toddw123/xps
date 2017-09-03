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

#ifndef __DEFINES_H
#define __DEFINES_H

class ConfigFile;

void OnExit(int signal);

extern int g_serverstatus;
#define ServerStatus g_serverstatus
extern time_t g_time;
extern tm g_localtime;
extern unsigned int g_mstime;
extern ConfigFile m_config;
extern bool g_restartEvent;
extern unsigned int g_restartTimer;

#define Socket SocketHandler2::getSingleton()

#endif