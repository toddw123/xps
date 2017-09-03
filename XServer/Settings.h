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

#ifndef __SETTINGS_H
#define __SETTINGS_H

#pragma pack(push, 1)
struct ServerSetting
{
	uint32 index;
	char* setting;
	char* value;
};
#pragma pack(pop)

extern SQLStore<ServerSetting> sServerSetting;


class ServerLoadSQLEvent : public SQLEvent
{
public:
	uint32 playerid;
	void OnQueryFinish();
};

class ServerSettingValue
{
public:
	std::string setting;

	std::string val_string;
	uint32 val_u32;
	int32 val_i32;
	float val_float;

	ServerSettingValue() { setting= "NOT_SET"; val_string = "NOT_SET"; val_u32 = 0; val_i32 = 0; val_float = 1.0f; }

	std::string & Setting() { return setting; }
	std::string & AsString() { return val_string; }
	uint32 AsUInt() { return val_u32; }
	uint32 AsInt() { return val_i32; }
	float AsFloat() { return val_float; }
};

class ServerSettings : public EventableObject
{
	Mutex m_lock;
	std::unordered_map<std::string, ServerSettingValue> m_settingMap;
	std::string m_notSetString;

public:
	void Load();
	void ReloadEvent(SQLEvent* ev);

	std::string & GetString(char* setting);
	int32 GetInt(char* setting);
	uint32 GetUInt(char* setting);
	float GetFloat(char* setting);

	void UpdateSetting(std::string setting, std::string val);
	void UpdateSetting(std::string setting, uint32 val) { char temp[1024]; sprintf(temp, "%u", val); UpdateSetting(setting, temp); }


	void CreateNewSetting(std::string setting, std::string val);

	void BuildInfoMessage(InfoMessage & msg);

	void Lock() { m_lock.Acquire(); }
	void Unlock() { m_lock.Release(); }
};

extern ServerSettings g_ServerSettings;

#endif
