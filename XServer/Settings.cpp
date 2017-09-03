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

ServerSettings g_ServerSettings;
SQLStore<ServerSetting> sServerSetting;

void ServerLoadSQLEvent::OnQueryFinish()
{

}

void ServerSettings::Load()
{
	GETGUARD(m_lock);

	sServerSetting.Reload();
	m_settingMap.clear();

	for (auto itr = sServerSetting.m_data.begin(); itr != sServerSetting.m_data.end(); ++itr)
	{
		auto set = itr->second->setting;
		auto val = itr->second->value;

		ServerSettingValue stval;
		stval.setting = set;
		stval.val_string = val;
		stval.val_float = atof(val);
		stval.val_i32 = atol(val);
		stval.val_u32 = atoul(val);

		m_settingMap.insert(std::make_pair(set, stval));
	}
}

void ServerSettings::CreateNewSetting(std::string setting, std::string val)
{
	SQLEvent* ev = new SQLEvent;
	ev->m_event = MakeEvent(&ServerSettings::ReloadEvent, ev, EVENT_UNKNOWN, 0, 1, 0);

	std::string escaped_setting;
	std::string escaped_val;

	sDatabase.EscapeStr(setting, escaped_setting);
	sDatabase.EscapeStr(val, escaped_val);

	ev->AddQuery("delete from settings where setting = \"%s\";", escaped_setting.c_str()); //this should fail just make sure someone hasn't been editing manually which could conflict a future load
	ev->AddQuery("insert into settings (`setting`, `value`) VALUES (\"%s\", \"%s\");", escaped_setting.c_str(), escaped_val.c_str());
	sDatabase.Execute(ev);
}

void ServerSettings::UpdateSetting(std::string setting, std::string val)
{
	GETGUARD(m_lock);

	auto itr = m_settingMap.find(setting);

	if (itr == m_settingMap.end())
		CreateNewSetting(setting, val);
	else
	{		
		SQLEvent* ev = new SQLEvent;
		ev->m_event = MakeEvent(&ServerSettings::ReloadEvent, ev, EVENT_UNKNOWN, 0, 1, 0);

		std::string escaped_setting;
		std::string escaped_val;

		sDatabase.EscapeStr(setting, escaped_setting);
		sDatabase.EscapeStr(val, escaped_val);

		ev->AddQuery("update settings set value = \"%s\" where setting = \"%s\";", escaped_val.c_str(), escaped_setting.c_str());
		sDatabase.Execute(ev);
	}
}

void ServerSettings::ReloadEvent(SQLEvent* ev)
{
	Load();
	delete ev;
}

std::string & ServerSettings::GetString(char* setting)
{
	GETGUARD(m_lock);
	auto itr = m_settingMap.find(setting);

	if (itr == m_settingMap.end())
		return m_notSetString;
	return itr->second.AsString();
}

int32 ServerSettings::GetInt(char* setting)
{
	GETGUARD(m_lock);
	auto itr = m_settingMap.find(setting);

	if (itr == m_settingMap.end())
		return 0;
	return itr->second.AsInt();
}

uint32 ServerSettings::GetUInt(char* setting)
{
	GETGUARD(m_lock);
	auto itr = m_settingMap.find(setting);

	if (itr == m_settingMap.end())
		return 0;
	return itr->second.AsUInt();
}

float ServerSettings::GetFloat(char* setting)
{
	GETGUARD(m_lock);
	auto itr = m_settingMap.find(setting);

	if (itr == m_settingMap.end())
		return 1.0f;
	return itr->second.AsFloat();
}

void ServerSettings::BuildInfoMessage(InfoMessage & msg)
{
	GETGUARD(m_lock);

	//every one starts with a space, each line is 0x28 bytes, null terminated (but still uses that amount of bytes, gay)
	char line[0x28];

	for (auto itr = m_settingMap.begin(); itr != m_settingMap.end(); ++itr)
	{
		sprintf(line, "%s: %s", itr->second.Setting().c_str(), itr->second.AsString().c_str());

		msg.messages.push_back(line);
	}
}
