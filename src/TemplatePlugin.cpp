#include "Template.h"
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
#include <fstream>
#include <regex>
#include "EntityData.h"
#include "PlayersData.h"
#include "prints.h"
#include "tasks.h"

PLUGIN_EXPOSE(Template, TemplatePlugin::g_Template);

CGameEntitySystem* GameEntitySystem()
{
#ifdef WIN32
    static int offset = 88;
#else
    static int offset = 80;
#endif
    return *reinterpret_cast<CGameEntitySystem**>((uintptr_t)(g_pGameResourceServiceServer) + offset);
}

class GameSessionConfiguration_t
{
};

namespace TemplatePlugin
{
    TemplatePlugin g_Template;
    CEntityListener g_EntityListener;

    SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
    SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t&,
                       ISource2WorldSession*, const char*);
    SH_DECL_HOOK2(IGameEventManager2, LoadEventsFromFile, SH_NOATTRIB, 0, int, const char*, bool);
    SH_DECL_HOOK2(IGameEventManager2, FireEvent, SH_NOATTRIB, 0, bool, IGameEvent*, bool);

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

        ShutdownHooks();
        shared::g_pEntitySystem->RemoveListenerEntity(&g_EntityListener);

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
    }

    bool done = false;
    void TemplatePlugin::Hook_StartupServer(const GameSessionConfiguration_t& config,
                                      ISource2WorldSession*, const char*)
    {
        if (!done)
        {
            shared::g_pEntitySystem = GameEntitySystem();
            shared::g_pEntitySystem->AddListenerEntity(&g_EntityListener);
            InitHooks();
            done = true;
        }
    }

    int TemplatePlugin::Hook_LoadEventsFromFile(const char* filename, bool bSearchAll)
    {
        ExecuteOnce(shared::g_pGameEventManager = META_IFACEPTR(IGameEventManager2));
        RETURN_META_VALUE(MRES_IGNORED, 0);
    }

    static std::unordered_map<std::string, std::vector<std::pair<GameEventHandler, HookMode>>> g_eventHandlers;
    static std::vector<IGameEvent*> g_eventStack;

    static HookResult DispatchEvent(IGameEvent* event, HookMode mode, bool& dontBroadcast)
    {
        if (!event) return HookResult::Continue;

        const char* name = event->GetName();
        auto it = g_eventHandlers.find(name);
        if (it == g_eventHandlers.end()) return HookResult::Continue;

        EventOverride override{dontBroadcast};
        HookResult result = HookResult::Continue;

        for (auto& [handler, hmode] : it->second)
        {
            if (hmode != mode)
                continue;

            HookResult r = handler(event, mode, override);
            if (r == HookResult::Stop)
                return HookResult::Stop;
            if (r == HookResult::Handled)
                return HookResult::Handled;
            if (static_cast<int>(r) > static_cast<int>(result))
                result = r;
        }

        dontBroadcast = override.dontBroadcast;
        return result;
    }

    static bool OnFireEvent(IGameEvent* event, bool bDontBroadcast)
    {
        if (!event)
            RETURN_META_VALUE(MRES_IGNORED, false);

        bool localDontBroadcast = bDontBroadcast;
        HookResult res = DispatchEvent(event, HookMode::Pre, localDontBroadcast);

        if (res == HookResult::Stop || res == HookResult::Handled)
            RETURN_META_VALUE(MRES_SUPERCEDE, false);

        if (IGameEvent* copy = shared::g_pGameEventManager->DuplicateEvent(event))
            g_eventStack.push_back(copy);

        if (localDontBroadcast != bDontBroadcast)
            RETURN_META_VALUE_NEWPARAMS(MRES_IGNORED, true, &IGameEventManager2::FireEvent,
                                    (event, localDontBroadcast));

        RETURN_META_VALUE(MRES_IGNORED, true);
    }

    static bool OnFireEventPost(IGameEvent* event, bool bDontBroadcast)
    {
        if (!event)
            RETURN_META_VALUE(MRES_IGNORED, false);

        if (!g_eventStack.empty())
        {
            IGameEvent* copy = g_eventStack.back();
            g_eventStack.pop_back();

            bool dummy = bDontBroadcast;
            DispatchEvent(copy, HookMode::Post, dummy);
            shared::g_pGameEventManager->FreeEvent(copy);
        }

        RETURN_META_VALUE(MRES_IGNORED, true);
    }

    void InitHooks()
    {
        SH_ADD_HOOK(IGameEventManager2, FireEvent, shared::g_pGameEventManager, OnFireEvent, false);
        SH_ADD_HOOK(IGameEventManager2, FireEvent, shared::g_pGameEventManager, OnFireEventPost, true);

        RegisterGameEvent("player_connect", OnPlayerConnectFull, HookMode::Pre);
    }

    void ShutdownHooks()
    {
        SH_REMOVE_HOOK(IGameEventManager2, FireEvent, shared::g_pGameEventManager, OnFireEvent, false);
        SH_REMOVE_HOOK(IGameEventManager2, FireEvent, shared::g_pGameEventManager, OnFireEventPost, true);

        for (auto* ev : g_eventStack)
            shared::g_pGameEventManager->FreeEvent(ev);

        g_eventStack.clear();
        g_eventHandlers.clear();
    }

    void RegisterGameEvent(const std::string& name, GameEventHandler handler, HookMode mode)
    {
        g_eventHandlers[name].push_back({handler, mode});
    }

    HookResult OnPlayerConnectFull(IGameEvent* ev, HookMode mode, EventOverride& override)
    {
        if (!ev)
            return HookResult::Continue;

        CCSPlayerController* player = static_cast<CCSPlayerController*>(ev->GetPlayerController("userid"));
        if (!player || !player->CheckValid())
            return HookResult::Continue;

        META_LOG(&g_Template, "Player %s joined yay!\n", player->GetPlayerName());

        return HookResult::Continue;
    }

    void CEntityListener::OnEntityCreated(CEntityInstance* pEntity)
    {
        if (!pEntity) return;

        auto* baseEntity = static_cast<CBaseEntity*>(pEntity);
        CHandle<CBaseEntity> handle = baseEntity->GetHandle();

        EntityData_t data;
        data.Init();

        EntityData[handle] = std::move(data);
    }

    void CEntityListener::OnEntityDeleted(CEntityInstance* pEntity)
    {
        if (!pEntity) return;

        auto* baseEntity = static_cast<CBaseEntity*>(pEntity);
        CHandle<CBaseEntity> handle = baseEntity->GetHandle();

        EntityData.erase(handle);
    }

    void CEntityListener::OnEntitySpawned(CEntityInstance* pEntity)
    {
        if (!pEntity)
            return;

        auto* baseEntity = static_cast<CCSWeaponBase*>(pEntity);
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
