//
// Created by Michal Přikryl on 26.06.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#pragma once
#include "CBasePlayerController.h"
#include "serversideclient.h"
#include "services.h"
#include "Shared.h"

namespace Template {
    extern CServerSideClient *GetClientBySlot(CPlayerSlot slot);

    class CCSPlayerController : public CBasePlayerController {
    public:
        DECLARE_SCHEMA_CLASS(CCSPlayerController);

        SCHEMA_FIELD(CCSPlayerController_InGameMoneyServices*, m_pInGameMoneyServices);

        SCHEMA_FIELD(CCSPlayerController_ActionTrackingServices*, m_pActionTrackingServices);

        SCHEMA_FIELD(CCSPlayerController_InventoryServices*, m_pInventoryServices);

        SCHEMA_FIELD(uint32_t, m_iPing);

        SCHEMA_FIELD(CUtlSymbolLarge, m_szClan);

        SCHEMA_FIELD_POINTER(char, m_szClanName) // char m_szClanName[32]
        SCHEMA_FIELD(bool, m_bEverFullyConnected);

        SCHEMA_FIELD(bool, m_bPawnIsAlive);

        SCHEMA_FIELD(int32_t, m_nDisconnectionTick);

        SCHEMA_FIELD(CHandle<CCSPlayerPawn>, m_hPlayerPawn);

        SCHEMA_FIELD(int32, m_DesiredObserverMode);

        SCHEMA_FIELD(int16_t, m_nPawnCharacterDefIndex);

        SCHEMA_FIELD(CHandle<CCSPlayerPawnBase>, m_hObserverPawn);

        SCHEMA_FIELD(CHandle<CCSPlayerController>, m_hOriginalControllerOfCurrentPawn);

        SCHEMA_FIELD(uint32_t, m_iPawnHealth);

        SCHEMA_FIELD(int32_t, m_iPawnArmor);

        SCHEMA_FIELD(int32_t, m_iScore);

        SCHEMA_FIELD(int32_t, m_iRoundScore);

        SCHEMA_FIELD(int32_t, m_iRoundsWon);

        SCHEMA_FIELD(bool, m_bMvpNoMusic);

        SCHEMA_FIELD(int32_t, m_iMusicKitID);

        SCHEMA_FIELD(int32_t, m_iMVPs);

        SCHEMA_FIELD(float, m_flSmoothedPing);

        SCHEMA_FIELD(GameTime_t, m_flForceTeamTime);

        SCHEMA_FIELD(int32_t, m_iCompetitiveRanking);

        SCHEMA_FIELD(int8_t, m_iCompetitiveRankType);

        SCHEMA_FIELD(int32_t, m_iCompetitiveWins);

        SCHEMA_FIELD(byte, m_iPendingTeamNum);

        SCHEMA_FIELD(byte, m_bTeamChanged);

        SCHEMA_FIELD(byte, m_bInSwitchTeam);

        static CCSPlayerController *FromPawn(CCSPlayerPawn *pawn) {
            return reinterpret_cast<CCSPlayerController *>(pawn->m_hController().Get());
        }

        static CCSPlayerController *FromIndex(int iIndex) {
            return static_cast<CCSPlayerController *>(shared::g_pEntitySystem->
                GetEntityInstance(CEntityIndex(iIndex)));
        }

        static CCSPlayerController *FromSlot(int iSlot) {
            return static_cast<CCSPlayerController *>(shared::g_pEntitySystem->
                GetEntityInstance(CEntityIndex(iSlot + 1)));
        }

        static CCSPlayerController *FromSteamID(uint64_t steamid) {
            for (int i = 0; i < shared::getGlobalVars()->maxClients; ++i) {
                CCSPlayerController *controller = FromSlot(i);
                if (!controller || controller->GetSteamID() != steamid) continue;

                return controller;
            }
            return nullptr;
        }

        CServerSideClient *GetServerSideClient() {
            return GetClientBySlot(GetPlayerSlot());
        }

        CCSPlayerPawn *GetPlayerPawn() {
            auto handle = m_hPlayerPawn();
            if (!handle.IsValid())
                return nullptr;

            void *rawEnt = shared::g_pEntitySystem->GetEntityInstance(CEntityIndex(handle.GetEntryIndex()));
            if (!rawEnt)
                return nullptr;

            return static_cast<CCSPlayerPawn *>(rawEnt);
        }

        CCSPlayerPawn *GetObserverPawn() {
            auto handle = m_hObserverPawn();
            if (!handle.IsValid())
                return nullptr;

            void *rawEnt = shared::g_pEntitySystem->GetEntityInstance(CEntityIndex(handle.GetEntryIndex()));
            if (!rawEnt)
                return nullptr;

            return static_cast<CCSPlayerPawn *>(rawEnt);
        }

        bool IsBot() { return GetSteamID() == 0; }

        static std::string CsTeamToString(CsTeam team) {
            switch (team) {
                case CsTeam::None: return "None";
                case CsTeam::Spectator: return "Spectator";
                case CsTeam::Terrorist: return "Terrorist";
                case CsTeam::CounterTerrorist: return "CounterTerrorist";
                default: return "Unknown";
            }
        }

        CsTeam Team() {
            byte teamNum = GetTeam();
            if (teamNum < static_cast<byte>(CsTeam::None) || teamNum > static_cast<byte>(CsTeam::CounterTerrorist))
                return CsTeam::None;

            return static_cast<CsTeam>(teamNum);
        }

        PlayerConnectedState GetConnectedState() const { return (m_iConnected_prop) m_iConnected; }

        std::string PlayerName() {
            return std::string(GetPlayerName());
        }

        bool CheckValid(bool alive = false) {
            CCSPlayerPawn *pawn = GetPlayerPawn();

            if (IsBot() || GetIsHLTV() || GetSteamID() <= 0 || !IsConnected() || !pawn)
                return false;

            if (alive && !m_bPawnIsAlive()) {
                return false;
            }

            return true;
        }

        CSPlayerState GetPawnState() {
            CCSPlayerPawnBase *pPawn = static_cast<CCSPlayerPawnBase *>(GetPawn());
            if (!pPawn)
                return STATE_WELCOME;

            return pPawn->m_iPlayerState();
        }

        CSPlayerState GetPlayerPawnState() {
            CCSPlayerPawn *pPawn = GetPlayerPawn();
            if (!pPawn)
                return STATE_WELCOME;

            return pPawn->m_iPlayerState();
        }

        CBaseEntity *GetObserverTarget() {
            auto pPawn = GetPawn();

            if (!pPawn)
                return nullptr;

            return pPawn->m_pObserverServices->m_hObserverTarget().Get();
        }

        // --- HEALTH & ARMOR ---
        void SetHealth(int health) {
            if (!CheckValid(true) || health <= 0) return;
            auto pawn = GetPlayerPawn();
            pawn->m_iHealth = health;
            m_iPawnHealth = health;
            if (health > pawn->m_iMaxHealth)
                pawn->m_iMaxHealth = health;
        }

        void AddHealth(int delta) {
            if (!CheckValid(true) || delta <= 0) return;
            auto pawn = GetPlayerPawn();
            pawn->m_iHealth += delta;
            m_iPawnHealth += delta;
        }

        void SetMaxHealth(int maxHp) {
            if (!CheckValid(true) || maxHp <= 0) return;
            auto pawn = GetPlayerPawn();
            pawn->m_iMaxHealth = maxHp;
        }

        void SetArmor(int armor, bool helmet = false) {
            if (!CheckValid(true) || armor < 0) return;
            auto pawn = GetPlayerPawn();
            pawn->m_ArmorValue = armor;
            m_iPawnArmor = armor;
            if (helmet && pawn->m_pItemServices)
                pawn->m_pItemServices->m_bHasHelmet = true;
        }

        void AddArmor(int delta) {
            if (!CheckValid(true) || delta <= 0) return;
            auto pawn = GetPlayerPawn();
            pawn->m_ArmorValue() += delta;
            m_iPawnArmor() += delta;
        }

        // --- WEAPONS ---
        std::vector<CCSWeaponBase *> GetWeapons() {
            std::vector<CCSWeaponBase *> weapons;
            auto pawn = GetPlayerPawn();
            if (!pawn || !pawn->m_pWeaponServices() || !pawn->m_pWeaponServices()->m_hMyWeapons())
                return weapons;

            const auto &myWeapons = *pawn->m_pWeaponServices()->m_hMyWeapons();

            for (int i = 0; i < myWeapons.Count(); ++i) {
                const auto &handle = myWeapons[i];
                if (!handle.IsValid()) continue;

                if (auto *weapon = static_cast<CCSWeaponBase *>(handle.Get())) {
                    weapons.push_back(weapon);
                }
            }

            return weapons;
        }

        CCSWeaponBase *GetWeaponBySlot(gear_slot_t slot) {
            for (auto *base: GetWeapons()) {
                if (base->GetWeaponVData() && base->GetWeaponVData()->m_GearSlot() == slot)
                    return base;
            }
            return nullptr;
        }

        CCSWeaponBase *GetWeaponByName(const std::string &weaponName) {
            for (auto *base: GetWeapons()) {
                if (base->GetDesignerName() == weaponName)
                    return base;
            }
            return nullptr;
        }

        bool HasWeaponByName(const std::string &weaponName) {
            return GetWeaponByName(weaponName) != nullptr;
        }

        bool HasWeaponInSlot(gear_slot_t slot) {
            return GetWeaponBySlot(slot) != nullptr;
        }

        std::string GetWeaponNameBySlot(gear_slot_t slot) {
            if (auto *w = GetWeaponBySlot(slot)) {
                return w->GetDesignerName();
            }
            return {};
        }

        void DropWeaponByName(const std::string &weaponName) {
            auto pawn = GetPlayerPawn();
            if (!pawn || !pawn->m_pWeaponServices()) return;

            if (auto *weapon = GetWeaponByName(weaponName)) {
                pawn->m_pWeaponServices()->m_hActiveWeapon() = CHandle<CBaseEntity>(weapon);
                pawn->m_pItemServices()->DropActiveWeapon(weapon);
            }
        }
    };
}
