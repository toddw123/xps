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

#ifndef __LAUNCHERSOCKET_H
#define __LAUNCHERSOCKET_H

#define LAUNCHER_VERSION 27

enum LauncherOpCodes
{
	CMSG_PING,
	SMSG_PONG,
	CMSG_REDIRECT_DATA,
	SMSG_REDIRECT_DATA,
	SMSG_UPDATE_TARGET,
	SMSG_UPDATE_TARGET_NAME,
	SMSG_CLEAR_TARGET,
	CMSG_AUTH_DATA,
	CMSG_AUTH_CHANGE,
	CMSG_AUTH_SECRET,
	SMSG_DAMAGE_INFO,
	SMSG_HEALTH_CHANGE_SELF,
	SMSG_HEALTH_UPDATE,
	SMSG_REMOVE_OBJECT,
	SMSG_QUEST_UI_UPDATE,
	NUM_PACKET_TYPES,
};

class Account;
class ServerPacket;
class ClientSocket : public IOCPSocket
{
public:
	ClientSocket() { m_account = NULL; m_passwordhash = "INVALID"; m_changehash = "INVALID"; m_secrethash = "INVALID"; }

	Account* m_account;
	FastMutex m_accountlock;
	std::string m_passwordhash;
	std::string m_changehash;
	std::string m_secrethash;

	void SetAccount(Account* a);
	//DO NOT EDIT this may look dumb but it's to do with internal concurrency :)
	void ClearAccount();

	void WritePacket(Packet & p) { WritePacket(&p); }
	void WritePacket(Packet* p)
	{
		uint32 headerlen = p->GetSize();
		Send((uint8*)&headerlen, 4, p->GetBuffer(), p->GetSize());
	}
	void WritePacket(ServerPacket & p) { WritePacket(&p); }
	void WritePacket(ServerPacket* p);

	void OnConnect();
	bool OnDataRead();
	void OnRemove();
	void OnSocketError() { Remove(); }
};

struct PatchData
{
	uint32 x;
	uint32 y;
	uint8 layer;
	uint8 val;
	uint8 oval;
};

struct ConnectedLauncher
{
	ClientSocket* sock;
	uint32 flags;
};

class ClientListener : public ListenerSocket<ClientSocket>, public Singleton<ClientListener>, public ThreadBase
{
public:
	unordered_multimap<uint64, ConnectedLauncher> m_connectedsockets;
	std::multimap<uint32, PatchData> m_patchdata;
	FastMutex m_lock;
	
	void Start()
	{
		CreateSocket(IPPROTO_TCP);
		Bind(INADDR_ANY, 9876);

		Listen(SOMAXCONN);
		Accept();
	}

	bool run();

	uint64 GetSocketKey(ClientSocket* s) { return ((uint64)s->m_sender.sin_addr.s_addr | ((uint64)s->m_sender.sin_port << 32)); }

	void AddSocket(ClientSocket* s)
	{
		GETFGUARD(m_lock);
		s->AddRef();
		ConnectedLauncher d;
		d.sock = s;
		d.flags = 0;
		m_connectedsockets.insert(std::make_pair(GetSocketKey(s), d));
	}

	void RemoveSocket(ClientSocket* s)
	{
		GETFGUARD(m_lock);
		m_connectedsockets.erase(GetSocketKey(s));
		s->DecRef();
	}
};

#define sLauncherListener ClientListener::getSingleton()


struct MappedFile
{
	uint8* filebuf;
	uint32 filesize;
};

class LauncherFileInfo : public Singleton<LauncherFileInfo>
{
public:
	unordered_map<std::string, uint32> m_crcinfo;
	unordered_map<std::string, MappedFile> m_filedata;
	std::set<std::string> m_filenames;

	LauncherFileInfo()
	{
		std::vector<std::string> filenames;
		std::vector<std::string> dirnames;

		Directory dir("client");
		dir.EnumFiles(FILE_ENUM_DIRECTORIES, dirnames);
		dir.EnumFiles(FILE_ENUM_FILES, filenames);

		for (uint32 i = 0; i < dirnames.size(); ++i)
		{
			Directory tmp(dirnames[i].c_str());
			tmp.EnumFiles(FILE_ENUM_DIRECTORIES, dirnames);
			tmp.EnumFiles(FILE_ENUM_FILES, filenames);
		}

		for (std::vector<std::string>::iterator itr = filenames.begin(); itr != filenames.end(); ++itr)
		{
			sLog.Debug("Launcher", "Loading data for %s", (*itr).c_str());

			std::string filename = (*itr).c_str();
			replace(filename, "client\\", "", 0);

			LoadInfo(filename.c_str());


			//DONT PUT LAUNCHER.NEW INTO THE FILE CHECKS
			if (strcmp("Launcher.new", filename.c_str()) == 0)
				continue;
			m_filenames.insert(filename);
		}
	}

	void LoadInfo(const char* filename)
	{
		char tmpfilename[1024];
		sprintf(tmpfilename, "client/%s", filename);
		FILE* f = fopen(tmpfilename, "rb");
		if (f == NULL)
			return;
		fseek(f, 0, SEEK_END);
		uint32 flen = ftell(f);
		uint8* buf = new uint8[flen];
		fseek(f, 0, SEEK_SET);
		fread(buf, 1, flen, f);
		fclose(f);

		uint32 crc = crc32(buf, flen);

		m_crcinfo.insert(std::make_pair(filename, crc));
		MappedFile data;
		data.filebuf = buf;
		data.filesize = flen;
		m_filedata.insert(std::make_pair(filename, data));
	}

	uint32 GetFileCRC(std::string filename)
	{
		unordered_map<std::string, uint32>::iterator itr = m_crcinfo.find(filename);
		if (itr == m_crcinfo.end())
			return 0;
		return itr->second;
	}

	MappedFile* GetMappedFile(std::string filename)
	{
		unordered_map<std::string, MappedFile>::iterator itr = m_filedata.find(filename);
		if (itr == m_filedata.end())
			return NULL;
		return &itr->second;
	}
};

#define sLauncherFileInfo LauncherFileInfo::getSingleton()

#endif
