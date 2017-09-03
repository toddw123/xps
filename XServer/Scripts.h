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

#ifndef __SCRIPTS_H
#define __SCRIPTS_H

enum TileScriptProcFlags
{
	TILE_PROC_FLAG_STANDING_ON		= 0x01,
	TILE_PROC_BUMP_INTO				= 0x02,
	TILE_PROC_FLAG_CHAT				= 0x04,
};


class CreatureAIScript : public EventableObject
{
public:
	Creature* m_unit;
	void SetUnit(Creature* c) { m_unit = c; m_unit->AddRef(); OnSetUnit(); }
	~CreatureAIScript() { if (m_unit != NULL) m_unit->DecRef(); m_unit = NULL; }
	int32 GetInstanceID() { if (m_unit == NULL) return -1; return m_unit->GetInstanceID(); }

	virtual void OnSetUnit() {}
	virtual void OnCombatStart() {}
	virtual void OnCombatStop() {}
	virtual void OnCombatEnd() {}
	virtual void OnDeath() {}
	virtual void OnRespawn() {}
	virtual void OnAddInRangeObject(Object* o) {}
	virtual void OnRemoveInRangeObject(Object* o) {}
	virtual void OnKilledPlayer(Player* p) {}
	virtual void OnReachWayPoint() {}
	virtual void OnActive() {}
	virtual void OnInactive() {}
	virtual void OnAddToWorld() {}
	virtual void OnDealDamage(int32 damage, SpellEntry* spell) {}
	virtual void OnTakeDamage(Unit* from, int32 damage, SpellEntry* spell) {}
	virtual uint32 GetReaction(Unit* other) { return REACTION_SCRIPT_NOT_SET_USE_SERVER_DEFAULT; }
};

class MapScript : public EventableObject
{
public:
	MapMgr* m_mapmgr;
	MapScript() { m_mapmgr = NULL; }
	void SetMapMgr(MapMgr* m) { m_mapmgr = m; }
	int32 GetInstanceID() { if (m_mapmgr == NULL) return -1; return m_mapmgr->GetInstanceID(); }

	virtual void OnPlayerAddToWorld(Player* p) {}
	virtual void OnPlayerRemoveFromWorld(Player* p) {}
	virtual void OnPlayerBreakLink(Player* p) {}
	virtual void OnPlayerJoinLink(Player* o, Player* p) {}
};

class TileScript : public EventableObject
{
public:
	MapMgr* m_mapmgr;
	TileScript() { m_mapmgr = NULL; }
	void SetMapMgr(MapMgr* mgr) { m_mapmgr = mgr; }
	int32 GetInstanceID() { if (m_mapmgr == NULL) return -1; return m_mapmgr->GetInstanceID(); }

	uint32 m_procflags;
	uint32 m_tilex;
	uint32 m_tiley;
	bool HasProcFlag(uint32 flag) { if (m_procflags & flag) return true; return false; }
	virtual uint32 GetProcFlags() { return 0; }

	virtual void Proc(Object* u) { }
};

class ScriptMgr : public Singleton<ScriptMgr>
{
public:
	typedef CreatureAIScript*(*CreatureAIScriptCreate)(void);
	typedef MapScript*(*MapScriptCreate)(void);
	typedef TileScript*(*TileScriptCreate)(void);
	unordered_map<uint32, CreatureAIScriptCreate> m_creatureaiScripts;
	unordered_map<uint32, MapScriptCreate> m_mapScripts;
	unordered_map<std::string, TileScriptCreate> m_tileScripts;

	void RegisterAIScript(uint32 id, CreatureAIScriptCreate script) { m_creatureaiScripts.insert(std::make_pair(id, script)); }

	void RegisterMapScript(uint32 id, MapScriptCreate script) { m_mapScripts.insert(std::make_pair(id, script)); }
	void RegisterTileScript(std::string str, TileScriptCreate script) { m_tileScripts.insert(std::make_pair(str, script)); }

	CreatureAIScript* CreateCreatureAIScript(Creature* cre, uint32 scriptid)
	{
		unordered_map<uint32, CreatureAIScriptCreate>::iterator itr = m_creatureaiScripts.find(scriptid);
		if (itr == m_creatureaiScripts.end())
			return NULL;
		CreatureAIScript* script = itr->second();
		script->SetUnit(cre);
		return script;
	}

	void CreateMapScripts(MapMgr* m)
	{
		for (unordered_map<uint32, MapScriptCreate>::iterator itr = m_mapScripts.begin(); itr != m_mapScripts.end(); ++itr)
		{
			MapScript* s = itr->second();
			s->SetMapMgr(m);
			m->m_mapscripts.insert(std::make_pair(itr->first, s));
		}
	}

	void CreateTileScripts(MapMgr* m)
	{
		for (auto x = 0; x < 3220; ++x)
		{
			for (auto y = 0; y < 3220; ++y)
			{
				m->m_tilescripts[x][y].procflags = 0;
				m->m_tilescripts[x][y].script = NULL;
			}
		}

		LoadTileScripts(m);
	}

	void LoadTileScripts(MapMgr* m);
	void CreateTileScript(MapMgr* m, uint32 x, uint32 y, std::string scriptname);

	ScriptMgr();
};

#define sScriptMgr ScriptMgr::getSingleton()

#endif
