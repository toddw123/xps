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


void CStackWalker::OnSymInit(LPCSTR szSearchPath, DWORD symOptions, LPCSTR szUserName)
{

}

void CStackWalker::OnLoadModule(LPCSTR img, LPCSTR mod, DWORD64 baseAddr, DWORD size, DWORD result, LPCSTR symType, LPCSTR pdbName, ULONGLONG fileVersion)
{

}

void CStackWalker::OnDbgHelpErr(LPCSTR szFuncName, DWORD gle, DWORD64 addr)
{

}

void CStackWalker::OnCallstackEntry(CallstackEntryType eType, CallstackEntry &entry)
{
	CHAR buffer[STACKWALK_MAX_NAMELEN];
	if ( (eType != lastEntry) && (entry.offset != 0) )
	{
		if (entry.name[0] == 0)
			strcpy(entry.name, "(function-name not available)");
		if (entry.undName[0] != 0)
			strcpy(entry.name, entry.undName);
		if (entry.undFullName[0] != 0)
			strcpy(entry.name, entry.undFullName);
		/*		if(!stricmp(entry.symTypeString, "-exported-"))
		strcpy(entry.symTypeString, "dll");
		for(uint32 i = 0; i < strlen(entry.symTypeString); ++i)
		entry.symTypeString[i] = tolower(entry.symTypeString);*/

		char * p = strrchr(entry.loadedImageName, '\\');
		if(!p)
			p = entry.loadedImageName;
		else
			++p;

		if (entry.lineFileName[0] == 0)
		{
			//strcpy(entry.lineFileName, "(filename not available)");
			//if (entry.moduleName[0] == 0)
			//strcpy(entry.moduleName, "(module-name not available)");
			//sprintf(buffer, "%s): %s: %s\n", (LPVOID) entry.offset, entry.moduleName, entry.lineFileName, entry.name);
			//sprintf(buffer, "%s.
			if(entry.name[0] == 0)
				sprintf(entry.name, "%p", entry.offset);

			sprintf(buffer, "%s!%s Line %u\n", p, entry.name, entry.lineNumber );
		}
		else
			sprintf(buffer, "%s!%s Line %u\n", p, entry.name, entry.lineNumber);
		//OnOutput(buffer);

		/*if(p)
		OnOutput(p);
		else*/
		OnOutput(buffer);
	}
}

void CStackWalker::OnOutput(LPCSTR szText)
{
	CallBack+=szText;
	printf("   %s", szText);
}