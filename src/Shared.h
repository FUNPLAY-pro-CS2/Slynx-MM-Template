//
// Created by Michal Přikryl on 20.09.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#pragma once
#include <icvar.h>
#include <memory>
#include <schemasystem.h>
#include <vector>
#include <eiface.h>
#include <igameeventsystem.h>
#include "schema/cgameresourceserviceserver.h"

class CGameEntitySystem;

namespace Template::shared
{
    extern ICvar* g_pCVar;
    extern IServerGameDLL* g_pServer;
    extern ISource2Server* g_pSource2Server;
    extern IVEngineServer* g_pEngine;
    extern CSchemaSystem* g_pSchemaSystem;
    extern IGameEventManager2* g_pGameEventManager;
    extern IGameEventSystem* g_pGameEventSystem;
    extern ISource2GameEntities* g_pGameEntities;
    extern INetworkMessages* g_pNetworkMessages;
    extern INetworkServerService* g_pNetworkServerService;
    extern CGameEntitySystem* g_pEntitySystem;
    extern IServerGameClients* g_pGameClients;
    extern CGlobalVars* g_pGlobalVars;
    extern CGameResourceService* g_pGameResourceServiceServer;

    CGlobalVars* getGlobalVars();
}
