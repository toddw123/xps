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

initialiseSingleton(ScriptMgr);

ScriptMgr::ScriptMgr()
{
	RegisterAIScript(1, &YanBlackAIScript::Create);
	RegisterAIScript(2, &OsmosAIScript::Create);
	RegisterAIScript(3, &DestructableWallAI::Create);
	RegisterAIScript(4, &JelocAIScript::Create);
	RegisterAIScript(5, &ChargedLurkerAIScript::Create);
	RegisterAIScript(6, &MagrothCrystalAIScript::Create);
	RegisterAIScript(7, &MagrothGuardAIScript::Create);
	RegisterAIScript(8, &MagrothHeraldAIScript::Create);
	RegisterAIScript(9, &JelocVelBossAIScript::Create);
	RegisterAIScript(10, &VoidZoneAIScript::Create);
	RegisterAIScript(11, &ReflectionBossAIScript::Create);
	RegisterAIScript(12, &ReflectionAIScript::Create);
	RegisterAIScript(13, &GeneralThoramAIScript::Create);
	RegisterAIScript(14, &BeamCrystalAIScript::Create);
	RegisterAIScript(15, &IngorAI::Create);
	RegisterAIScript(16, &RaelekAI::Create);
	RegisterAIScript(17, &VaeltarAI::Create);
	RegisterAIScript(18, &VaeltarGuardianAI::Create);
	RegisterAIScript(19, &GanarAI::Create);
	RegisterAIScript(20, &ToxicBugAI::Create);
	RegisterAIScript(21, &QueenRissaAI::Create);
	RegisterAIScript(22, &SkoreAI::Create);
	RegisterAIScript(23, &LokiAI::Create);
	RegisterAIScript(24, &CaptainAI::Create);
	RegisterAIScript(25, &SorcererAI::Create);
	RegisterAIScript(26, &ArachnidQueenAI::Create);
	RegisterAIScript(27, &DummyFireballEmitterAI::Create);
	RegisterAIScript(28, &RickAstleyAI::Create);
	RegisterAIScript(29, &DreadBarawAIScript::Create);
	RegisterAIScript(30, &BarawVoidZoneAIScript::Create);
	
	RegisterMapScript(1, &MagrothMapScript::Create);

	RegisterTileScript("CaraSaWall", &CaraSaWall::Create);
	RegisterTileScript("EquarWall", &EquarWall::Create);
	RegisterTileScript("LokiCaptainSummoner", &LokiCaptainSummoner::Create);
	RegisterTileScript("ManaPool", &ManaPool::Create);
	RegisterTileScript("FierceShrine", &FierceShrine::Create);
}

void ScriptMgr::LoadTileScripts( MapMgr* m )
{
	MYSQL_RES* res = sDatabase.Query("select * from `map_scripts` where mapid = %u;", m->m_mapid);
	if (res != NULL && mysql_num_rows(res) != 0)
	{
		MYSQL_ROW row = sDatabase.Fetch(res);
		while (row != NULL)
		{
			uint32 r = 1;
			uint32 x = atol(row[r++]);
			uint32 y = atol(row[r++]);
			std::string scriptname = row[r++];

			CreateTileScript(m, x, y, scriptname);

			row = sDatabase.Fetch(res);
		}
	}
	sDatabase.Free(res);
}

void ScriptMgr::CreateTileScript( MapMgr* m, uint32 x, uint32 y, std::string scriptname )
{
	auto itr = m_tileScripts.find(scriptname);

	if (itr == m_tileScripts.end())
		return;

	if (x >= 3220 || y >= 3220)
		return;

	TileScript* script = itr->second();
	script->SetMapMgr(m);
	script->m_tilex = x;
	script->m_tiley = y;
	
	if (script == NULL)
		return;

	uint32 procflag = script->GetProcFlags();
	script->m_procflags = procflag;
	script->m_mapmgr = m;

	m->m_tilescripts[x][y].procflags = procflag;
	m->m_tilescripts[x][y].script = script;
}
