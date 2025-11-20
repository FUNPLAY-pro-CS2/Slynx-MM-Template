//
// Created by Michal Přikryl on 23.09.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#pragma once
#include <vector.h>
#include "ehandle.h"
#include "schemasystem.h"
#include "globaltypes.h"

namespace TemplatePlugin {
    class CTakeDamageInfo;

#pragma pack(push, 1)
    struct CAttackerInfo
    {
        bool NeedInit = true;             // 0x0
        bool IsPawn = false;              // 0x1
        bool IsWorld = false;             // 0x2
        std::uint8_t pad_3[1] = {};       // vyrovnání na 4 bajty
        std::uint32_t Attacker = 0;       // 0x4
        std::uint16_t AttackerUserId = 0; // 0x8
        std::uint8_t pad_A[2] = {};       // vyrovnání na 0xC
        int TeamChecked = -1;             // 0x0C
        int TeamNum = -1;                 // 0x10

        CAttackerInfo() = default;
        explicit CAttackerInfo(CEntityInstance* attacker);
    };
#pragma pack(pop)

    struct CTakeDamageInfoContainer
    {
        CTakeDamageInfo* pInfo;
    };

    class CTakeDamageInfo
    {
    public:
        DECLARE_SCHEMA_CLASS(CTakeDamageInfo)

    	SCHEMA_FIELD(Vector, m_vecDamageForce);
    	SCHEMA_FIELD(Vector, m_vecDamagePosition);
    	SCHEMA_FIELD(Vector, m_vecReportedPosition);
		SCHEMA_FIELD(Vector, m_vecDamageDirection);
    	SCHEMA_FIELD(CHandle<CBaseEntity>, m_hInflictor);
    	SCHEMA_FIELD(CHandle<CBaseEntity>, m_hAttacker);
    	SCHEMA_FIELD(CHandle<CBaseEntity>, m_hAbility);
    	SCHEMA_FIELD(float, m_flDamage);
    	SCHEMA_FIELD(float, m_flTotalledDamage);
    	SCHEMA_FIELD(int32_t, m_bitsDamageType);
    	SCHEMA_FIELD(int32_t, m_iDamageCustom);
    	SCHEMA_FIELD(int8_t, m_iAmmoType);
    	SCHEMA_FIELD(float, m_flOriginalDamage);
    	SCHEMA_FIELD(bool, m_bShouldBleed);
    	SCHEMA_FIELD(bool, m_bShouldSpark);
		SCHEMA_FIELD(TakeDamageFlags_t, m_nDamageFlags);
		SCHEMA_FIELD_POINTER(char, m_sDamageSourceName);
        SCHEMA_FIELD(HitGroup_t, m_iHitGroupId);
        SCHEMA_FIELD(int32_t, m_nNumObjectsPenetrated);
        SCHEMA_FIELD(float, m_flFriendlyFireDamageReductionRatio);
        SCHEMA_FIELD(bool, m_bInTakeDamageFlow);

        HitGroup_t GetHitGroup() const;
    };

    class CTakeDamageInfoEx {
    private:
        [[maybe_unused]] uint8_t __pad0000[0x8];

    public:Vector m_vecDamageForce; // 0x8  |  8
        Vector m_vecDamagePosition; // 0x14 | 20
        Vector m_vecReportedPosition; // 0x20 | 32
        Vector m_vecDamageDirection; // 0x2c | 44
        CBaseHandle m_hInflictor; // 0x38 | 56
        CBaseHandle m_hAttacker; // 0x3c | 60
        CBaseHandle m_hAbility; // 0x40 | 64
        float m_flDamage; // 0x44 | 68
        float m_flTotalledDamage; // 0x48 | 72
        int32_t m_bitsDamageType; // 0x4c | 76
        int32_t m_iDamageCustom; // 0x50 | 80
        int8_t m_iAmmoType; // 0x54 | 84

    private:
        [[maybe_unused]] uint8_t m_nUnknown0[0xb]; // 0x55 | 85

    public:
        float m_flOriginalDamage; // 0x60 | 96
        bool m_bShouldBleed; // 0x64 | 100
        bool m_bShouldSpark; // 0x65 | 101

    private:
        [[maybe_unused]] uint8_t m_nUnknown1[0x2]; // 0x66

    public:
        CGameTrace *m_pTrace; // 0x68 | 104
        TakeDamageFlags_t m_nDamageFlags; // 0x70 | 112

    private:
        [[maybe_unused]] uint8_t m_sDamageSourceName[0x8]; // 0x78 | 120

    public:
        HitGroup_t m_iHitGroupId; // 0x80 | 128
        int32_t m_nNumObjectsPenetrated; // 0x84 | 132
        float m_flFriendlyFireDamageReductionRatio; // 0x88 | 136
    private:
        uint8_t m_nUnknown2[0x5C]; // 0x9c | 140
    public:
        void *m_hScriptInstance; // 0xe8 | 232
        CTakeDamageInfo m_AttackerInfo; // 0xf0 | 240
        bool m_bInTakeDamageFlow; // 0x104 | 260

    private:
        [[maybe_unused]] int32_t m_nUnknown3; // 0x108 | 264
    public:
        HitGroup_t GetHitGroup() const;
    };

    static_assert(sizeof(CTakeDamageInfoEx) == 272);
}
