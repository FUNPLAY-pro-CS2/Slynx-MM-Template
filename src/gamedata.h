//
// Created by Michal Přikryl on 16.11.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#pragma once
#include <icvar.h>
#include <memory>
#include <schemasystem.h>
#include <vector>
#include <eiface.h>

namespace Template::gamedata
{
    namespace signatures
    {
#ifdef WIN32
        static inline const char* IGameSystem_InitAllSystems_pFirst = "48 8B 1D ? ? ? ? 48 85 DB 0F 84 ? ? ? ? BD";
#else
        static inline const char* IGameSystem_InitAllSystems_pFirst = "4C 8B 35 ? ? ? ? 4D 85 F6 75";
#endif
    }

    namespace offsets
    {
#ifdef WIN32
        static inline int CBaseEntity_Teleport = 168
        static inline int CBaseEntity_CollisionRulesChanged = 190;
#else
        static inline int CBaseEntity_Teleport = 167;
        static inline int CBaseEntity_CollisionRulesChanged = 190;
#endif
    }
}