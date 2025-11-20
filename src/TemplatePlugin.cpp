#include "TemplatePlugin.h"
#include "path.h"
#include "Shared.h"
#include "dynlibutils/module.h"
#include <entitysystem.h>
#include "igameevents.h"
#include <iserver.h>
#include "game_system.h"
#include "schemasystem/schemasystem.h"
#include "schema/CCSPlayerController.h"
#include "schema/cgameresourceserviceserver.h"
#include "schema/plat.h"
#include <filesystem>
#include <cstdio>
#include <detours.h>
#include <fstream>
#include <gameconfig.h>
#include <regex>
#include "EntityData.h"
#include "PlayersData.h"
#include "tasks.h"
#include "schema/CGameRules.h"

PLUGIN_EXPOSE(Template, TemplatePlugin::g_Template);

CGameEntitySystem* GameEntitySystem()
{
    return *reinterpret_cast<CGameEntitySystem**>((uintptr_t)(g_pGameResourceServiceServer) +
        TemplatePlugin::shared::g_pGameConfig->GetOffset("GameEntitySystem"));
}

class GameSessionConfiguration_t
{
};

namespace TemplatePlugin
{
    TemplatePlugin g_Template;

    SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
    SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t&,
                       ISource2WorldSession*, const char*);
    SH_DECL_HOOK2(IGameEventManager2, LoadEventsFromFile, SH_NOATTRIB, 0, int, const char*, bool);

    int g_iLoadEventsFromFileId = -1;

    bool TemplatePlugin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
    {
        PLUGIN_SAVEVARS();

        GET_V_IFACE_CURRENT(GetEngineFactory, shared::g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
        GET_V_IFACE_ANY(GetServerFactory, shared::g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
        GET_V_IFACE_ANY(GetServerFactory, shared::g_pServer, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
        GET_V_IFACE_CURRENT(GetEngineFactory, shared::g_pEngine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
        GET_V_IFACE_ANY(GetEngineFactory, shared::g_pSchemaSystem, CSchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
        GET_V_IFACE_ANY(GetEngineFactory, shared::g_pGameEventSystem, IGameEventSystem,
                        GAMEEVENTSYSTEM_INTERFACE_VERSION);
        GET_V_IFACE_ANY(GetServerFactory, shared::g_pGameEntities, ISource2GameEntities,
                        SOURCE2GAMEENTITIES_INTERFACE_VERSION);
        GET_V_IFACE_ANY(GetServerFactory, shared::g_pGameClients, IServerGameClients,
                        SOURCE2GAMECLIENTS_INTERFACE_VERSION);
        GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceServiceServer, IGameResourceService,
                            GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
        GET_V_IFACE_ANY(GetEngineFactory, shared::g_pNetworkMessages, INetworkMessages,
                        NETWORKMESSAGES_INTERFACE_VERSION);
        GET_V_IFACE_ANY(GetEngineFactory, shared::g_pNetworkServerService, INetworkServerService,
                        NETWORKSERVERSERVICE_INTERFACE_VERSION);

        g_pCVar = shared::g_pCVar;
        g_pSource2GameEntities = shared::g_pGameEntities;
        shared::g_pGameResourceServiceServer = (CGameResourceService*)g_pGameResourceServiceServer;
        if (!shared::g_pGameResourceServiceServer)
            return false;

        if (!InitGameSystems())
            return false;

        Tasks::Init();
        Detours::InitHooks();
        RayTrace::Initialize();
        auto gamedata_path = std::string(Paths::GetRootDirectory() + "/gamedata.json");
        shared::g_pGameConfig = new CGameConfig(gamedata_path);
        char conf_error[255] = "";

        if (!shared::g_pGameConfig->Init(conf_error, sizeof(conf_error)))
        {
            META_LOG(&g_Template, "Could not read '%s'. Error: %s", gamedata_path.c_str(), conf_error);
            return false;
        }

        g_SMAPI->AddListener(this, this);

        SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, shared::g_pServer, this,
                            &TemplatePlugin::Hook_GameFrame, false);
        SH_ADD_HOOK(INetworkServerService, StartupServer, shared::g_pNetworkServerService,
                    SH_MEMBER(this, &TemplatePlugin::Hook_StartupServer), true);
        auto pCGameEventManagerVTable = DynLibUtils::CModule(shared::g_pServer).
                                        GetVirtualTableByName("CGameEventManager").RCast<IGameEventManager2*>();
        g_iLoadEventsFromFileId = SH_ADD_DVPHOOK(IGameEventManager2, LoadEventsFromFile, pCGameEventManagerVTable,
                                                 SH_MEMBER(this, &TemplatePlugin::Hook_LoadEventsFromFile), false);

        g_pCVar = shared::g_pCVar;
        ConVar_Register(FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL);

        META_LOG(&g_Template, "[Template] <<< Load() success!\n");
        return true;
    }

    bool TemplatePlugin::Unload(char* error, size_t maxlen)
    {
        SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, shared::g_pServer, this,
                               &TemplatePlugin::Hook_GameFrame, false);
        SH_REMOVE_HOOK(INetworkServerService, StartupServer, shared::g_pNetworkServerService,
                       SH_MEMBER(this, &TemplatePlugin::Hook_StartupServer), true);
        SH_REMOVE_HOOK_ID(g_iLoadEventsFromFileId);

        Detours::ShutdownHooks();
        Detours::Shutdown();
        shared::g_pEntitySystem->RemoveListenerEntity(&Detours::entityListener);
        Tasks::Shutdown();

        META_LOG(&g_Template, "[Template] <<< Unload() success!\n");

        return true;
    }

    void TemplatePlugin::OnLevelShutdown()
    {
        Tasks::RemoveMapChangeTimers();
    }

    void TemplatePlugin::Hook_GameFrame(bool simulating, bool bFirstTick, bool bLastTick)
    {
        Tasks::Tick();
        if (!shared::getGlobalVars())
            return;

        if (simulating && shared::g_bHasTicked)
            shared::g_flUniversalTime += shared::getGlobalVars()->curtime - shared::g_flLastTickedTime;

        shared::g_flLastTickedTime = shared::getGlobalVars()->curtime;
        shared::g_bHasTicked = true;

        if (CCSGameRules::FindGameRules())
            CCSGameRules::FindGameRules()->m_bGameRestart =
                    CCSGameRules::FindGameRules()->m_flRestartRoundTime < shared::GetCurrentTime();
    }

    static bool done = false;
    void TemplatePlugin::Hook_StartupServer(const GameSessionConfiguration_t& config,
                                            ISource2WorldSession*, const char*)
    {
        if (!done)
        {
            shared::g_pEntitySystem = GameEntitySystem();
            shared::g_pEntitySystem->AddListenerEntity(&Detours::entityListener);
            done = true;
        }
    }

    int TemplatePlugin::Hook_LoadEventsFromFile(const char* filename, bool bSearchAll)
    {
        ExecuteOnce(shared::g_pGameEventManager = META_IFACEPTR(IGameEventManager2));
        RETURN_META_VALUE(MRES_IGNORED, 0);
    }

    const char* TemplatePlugin::GetAuthor() { return "Slynx"; }
    const char* TemplatePlugin::GetName() { return "Template"; }
    const char* TemplatePlugin::GetDescription() { return "Template Metamod plugin for CS2 servers."; }
    const char* TemplatePlugin::GetURL() { return "https://slynxdev.cz"; }
    const char* TemplatePlugin::GetLicense() { return "GPLv3"; }
    const char* TemplatePlugin::GetVersion() { return TEMPLATEPLUGIN_VERSION; }
    const char* TemplatePlugin::GetDate() { return __DATE__; }
    const char* TemplatePlugin::GetLogTag() { return "Template"; }
}
