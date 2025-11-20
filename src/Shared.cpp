//
// Created by Michal Přikryl on 20.09.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#include "Shared.h"
#include <icvar.h>
#include <iserver.h>
#include <schemasystem.h>

namespace TemplatePlugin::shared
{
    ICvar* g_pCVar = nullptr;
    IServerGameDLL* g_pServer = nullptr;
    ISource2Server* g_pSource2Server = nullptr;
    IVEngineServer* g_pEngine = nullptr;
    CSchemaSystem* g_pSchemaSystem = nullptr;
    IGameEventManager2* g_pGameEventManager = nullptr;
    IGameEventSystem* g_pGameEventSystem = nullptr;
    ISource2GameEntities* g_pGameEntities = nullptr;
    INetworkMessages* g_pNetworkMessages = nullptr;
    INetworkServerService* g_pNetworkServerService = nullptr;
    CGameEntitySystem* g_pEntitySystem = nullptr;
    IServerGameClients* g_pGameClients = nullptr;
    CGlobalVars *g_pGlobalVars = nullptr;
    CGameResourceService *g_pGameResourceServiceServer = nullptr;
    CGameConfig *g_pGameConfig = nullptr;

    CGlobalVars *getGlobalVars() {
        INetworkGameServer *server = g_pNetworkServerService->GetIGameServer();
        if (!server) return nullptr;
        if (!g_pGlobalVars) g_pGlobalVars = server->GetGlobals();
        return g_pNetworkServerService->GetIGameServer()->GetGlobals();
    }
    constexpr float engine_fixed_tick_interval = 0.015625f;
    const char* GetMapName()
    {
        if (getGlobalVars() == nullptr) return nullptr;

        return getGlobalVars()->mapname.ToCStr();
    }
    void ServerCommand(const char* command)
    {
        auto clean_command = std::string(command);
        clean_command.append("\n\0");
        g_pEngine->ServerCommand(clean_command.c_str());
    }
    double GetEngineTime() { return Plat_FloatTime(); }
    float GetTickInterval() { return engine_fixed_tick_interval; }
    float GetCurrentTime() { return getGlobalVars()->curtime; }
    int GetTickCount() { return getGlobalVars()->tickcount; }
    float GetGameFrameTime() { return getGlobalVars()->frametime; }

    double g_flUniversalTime = 0.0;
    float g_flLastTickedTime = 0.0f;
    bool g_bHasTicked = false;
}
