#pragma once

#include <cstdint>

namespace dump_a2x
{
    namespace offsets
    {
        inline constexpr uintptr_t dwEntityList = 0x24CED50;
        inline constexpr uintptr_t dwGameEntitySystem = 0x24CED50;
        inline constexpr uintptr_t dwGameEntitySystem_highestEntityIndex = 0x2090;
        inline constexpr uintptr_t dwGlobalVars = 0x20496A0;
        inline constexpr uintptr_t dwLocalPlayerController = 0x2308520;
    }

    namespace schema
    {
        inline constexpr uint32_t CEntityInstance__m_pEntity = 0x10;
        inline constexpr uint32_t CEntityIdentity__m_designerName = 0x20;
        inline constexpr uint32_t CEntityIdentity__m_flags = 0x30;
        inline constexpr uint32_t CGameSceneNode__m_vecAbsOrigin = 0xC8;
        inline constexpr uint32_t CGameSceneNode__m_vecOrigin = 0x80;
        inline constexpr uint32_t C_BaseEntity__m_pGameSceneNode = 0x330;
        inline constexpr uint32_t C_BaseEntity__m_iHealth = 0x34C;
        inline constexpr uint32_t C_BaseEntity__m_iTeamNum = 0x3EB;
        inline constexpr uint32_t C_BaseEntity__m_nSubclassID = 0x380;
        inline constexpr uint32_t C_BaseEntity__m_hOwnerEntity = 0x520;
        inline constexpr uint32_t C_BasePlayerPawn__m_pWeaponServices = 0x11E0;
        inline constexpr uint32_t C_BasePlayerPawn__m_pItemServices = 0x11E8;
        inline constexpr uint32_t CPlayer_WeaponServices__m_hMyWeapons = 0x48;
        inline constexpr uint32_t CPlayer_WeaponServices__m_hActiveWeapon = 0x60;
        inline constexpr uint32_t CCSPlayer_ItemServices__m_bHasDefuser = 0x48;
        inline constexpr uint32_t CCSPlayer_ItemServices__m_bHasHelmet = 0x49;
        inline constexpr uint32_t C_CSPlayerPawn__m_bIsScoped = 0x1C48;
        inline constexpr uint32_t C_CSPlayerPawn__m_ArmorValue = 0x1C74;
        inline constexpr uint32_t C_CSPlayerPawnBase__m_flFlashOverlayAlpha = 0x13F4;
        inline constexpr uint32_t C_CSPlayerPawn__m_angEyeAngles = 0x3300;
        inline constexpr uint32_t CBasePlayerController__m_hPawn = 0x6BC;
        inline constexpr uint32_t CBasePlayerController__m_steamID = 0x778;
        inline constexpr uint32_t CCSPlayerController_InGameMoneyServices__m_iAccount = 0x40;
        inline constexpr uint32_t CCSPlayerController__m_pInGameMoneyServices = 0x800;
        inline constexpr uint32_t CCSPlayerController__m_iCompTeammateColor = 0x840;
        inline constexpr uint32_t CCSPlayerController__m_sSanitizedPlayerName = 0x858;
        inline constexpr uint32_t C_PlantedC4__m_bBombTicking = 0x1160;
        inline constexpr uint32_t C_PlantedC4__m_flC4Blow = 0x1190;
        inline constexpr uint32_t C_PlantedC4__m_bBeingDefused = 0x119C;
        inline constexpr uint32_t C_PlantedC4__m_flDefuseCountDown = 0x11B0;
        inline constexpr uint32_t C_PlantedC4__m_bBombDefused = 0x11B4;
        inline constexpr uint32_t CCSWeaponBaseVData__m_WeaponType = 0x520;
        inline constexpr uint32_t CCSWeaponBaseVData__m_szName = 0x720;
        inline constexpr uint32_t C_SmokeGrenadeProjectile__m_nSmokeEffectTickBegin = 0x1250;
        inline constexpr uint32_t C_SmokeGrenadeProjectile__m_vSmokeDetonationPos = 0x1268;
        inline constexpr uint32_t C_Inferno__m_firePositions = 0x1018;
        inline constexpr uint32_t C_Inferno__m_bFireIsBurning = 0x1618;
        inline constexpr uint32_t C_Inferno__m_fireCount = 0x1958;
        inline constexpr uint32_t C_Inferno__m_nFireEffectTickBegin = 0x196C;
        inline constexpr uint32_t CSkeletonInstance__m_modelState = 0x150;
        inline constexpr uint32_t CModelState__m_ModelName = 0xA8;
    }
}
