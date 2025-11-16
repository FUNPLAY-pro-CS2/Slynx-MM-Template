//
// Created by Michal Přikryl on 21.09.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#include "Shared.h"
#include "game_system.h"

#include <gamedata.h>
#include <tier0/vprof.h>
#include "dynlibutils/module.h"

CBaseGameSystemFactory** CBaseGameSystemFactory::sm_pFirst = nullptr;

CGameSystem g_GameSystem;
IGameSystemFactory* CGameSystem::sm_Factory = nullptr;

IEntityResourceManifest* m_exportResourceManifest = nullptr;

// This mess is needed to get the pointer to sm_pFirst so we can insert game systems
bool InitGameSystems()
{
    DynLibUtils::CModule libserver(Template::shared::g_pSource2Server);

    auto result = libserver.FindPattern(Template::gamedata::signatures::IGameSystem_InitAllSystems_pFirst);
    if (!result)
    {
        META_LOG(&Template::g_Template, "[Template] <<< Failed to find IGameSystem_InitAllSystems_pFirst!\n");
        return false;
    }

    uint8_t* ptr = reinterpret_cast<uint8_t*>(result.GetPtr()) + 3;
    uint32 offset = *(uint32*)ptr;
    ptr += 4;

    CBaseGameSystemFactory::sm_pFirst = (CBaseGameSystemFactory**)(ptr + offset);
    CGameSystem::sm_Factory = new CGameSystemStaticFactory<CGameSystem>("Template_GameSystem", &g_GameSystem);

    return true;
}

GS_EVENT_MEMBER(CGameSystem, BuildGameSessionManifest)
{
    IEntityResourceManifest* pResourceManifest = msg->m_pResourceManifest;

    m_exportResourceManifest = pResourceManifest;
}
