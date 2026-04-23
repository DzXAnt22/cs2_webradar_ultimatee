#include "pch.hpp"

struct schema_data_t
{
	fnv1a_t m_hashed_field_name = 0;
	uint32_t m_offset = 0;
};
static std::vector<schema_data_t> m_schema_data = {};

bool schema::setup()
{
	m_schema_data.clear();
	m_schema_data.reserve(40);

	auto add_field = [](const char* field_name, uint32_t offset)
	{
		m_schema_data.push_back({ fnv1a::hash(field_name), offset });
	};

	add_field("CEntityInstance->m_pEntity", dump_a2x::schema::CEntityInstance__m_pEntity);
	add_field("CEntityIdentity->m_designerName", dump_a2x::schema::CEntityIdentity__m_designerName);
	add_field("CEntityIdentity->m_flags", dump_a2x::schema::CEntityIdentity__m_flags);
	add_field("CGameSceneNode->m_vecAbsOrigin", dump_a2x::schema::CGameSceneNode__m_vecAbsOrigin);
	add_field("CGameSceneNode->m_vecOrigin", dump_a2x::schema::CGameSceneNode__m_vecOrigin);
	add_field("C_BaseEntity->m_pGameSceneNode", dump_a2x::schema::C_BaseEntity__m_pGameSceneNode);
	add_field("C_BaseEntity->m_iHealth", dump_a2x::schema::C_BaseEntity__m_iHealth);
	add_field("C_BaseEntity->m_iTeamNum", dump_a2x::schema::C_BaseEntity__m_iTeamNum);
	add_field("C_BaseEntity->m_hOwnerEntity", dump_a2x::schema::C_BaseEntity__m_hOwnerEntity);
	add_field("C_BaseEntity->m_nSubclassID", dump_a2x::schema::C_BaseEntity__m_nSubclassID);
	add_field("C_BasePlayerPawn->m_pWeaponServices", dump_a2x::schema::C_BasePlayerPawn__m_pWeaponServices);
	add_field("C_BasePlayerPawn->m_pItemServices", dump_a2x::schema::C_BasePlayerPawn__m_pItemServices);
	add_field("CPlayer_WeaponServices->m_hActiveWeapon", dump_a2x::schema::CPlayer_WeaponServices__m_hActiveWeapon);
	add_field("CPlayer_WeaponServices->m_hMyWeapons", dump_a2x::schema::CPlayer_WeaponServices__m_hMyWeapons);
	add_field("CCSPlayer_ItemServices->m_bHasDefuser", dump_a2x::schema::CCSPlayer_ItemServices__m_bHasDefuser);
	add_field("CCSPlayer_ItemServices->m_bHasHelmet", dump_a2x::schema::CCSPlayer_ItemServices__m_bHasHelmet);
	add_field("C_CSPlayerPawn->m_ArmorValue", dump_a2x::schema::C_CSPlayerPawn__m_ArmorValue);
	add_field("C_CSPlayerPawn->m_angEyeAngles", dump_a2x::schema::C_CSPlayerPawn__m_angEyeAngles);
	add_field("C_CSPlayerPawnBase->m_flFlashOverlayAlpha", dump_a2x::schema::C_CSPlayerPawnBase__m_flFlashOverlayAlpha);
	add_field("C_CSPlayerPawn->m_bIsScoped", dump_a2x::schema::C_CSPlayerPawn__m_bIsScoped);
	add_field("CBasePlayerController->m_hPawn", dump_a2x::schema::CBasePlayerController__m_hPawn);
	add_field("CBasePlayerController->m_steamID", dump_a2x::schema::CBasePlayerController__m_steamID);
	add_field("CCSPlayerController_InGameMoneyServices->m_iAccount", dump_a2x::schema::CCSPlayerController_InGameMoneyServices__m_iAccount);
	add_field("CCSPlayerController->m_pInGameMoneyServices", dump_a2x::schema::CCSPlayerController__m_pInGameMoneyServices);
	add_field("CCSPlayerController->m_iCompTeammateColor", dump_a2x::schema::CCSPlayerController__m_iCompTeammateColor);
	add_field("CCSPlayerController->m_sSanitizedPlayerName", dump_a2x::schema::CCSPlayerController__m_sSanitizedPlayerName);
	add_field("C_PlantedC4->m_bBombTicking", dump_a2x::schema::C_PlantedC4__m_bBombTicking);
	add_field("C_PlantedC4->m_flC4Blow", dump_a2x::schema::C_PlantedC4__m_flC4Blow);
	add_field("C_PlantedC4->m_bBombDefused", dump_a2x::schema::C_PlantedC4__m_bBombDefused);
	add_field("C_PlantedC4->m_bBeingDefused", dump_a2x::schema::C_PlantedC4__m_bBeingDefused);
	add_field("C_PlantedC4->m_flDefuseCountDown", dump_a2x::schema::C_PlantedC4__m_flDefuseCountDown);
	add_field("CCSWeaponBaseVData->m_WeaponType", dump_a2x::schema::CCSWeaponBaseVData__m_WeaponType);
	add_field("CCSWeaponBaseVData->m_szName", dump_a2x::schema::CCSWeaponBaseVData__m_szName);
	add_field("C_SmokeGrenadeProjectile->m_nSmokeEffectTickBegin", dump_a2x::schema::C_SmokeGrenadeProjectile__m_nSmokeEffectTickBegin);
	add_field("C_SmokeGrenadeProjectile->m_vSmokeDetonationPos", dump_a2x::schema::C_SmokeGrenadeProjectile__m_vSmokeDetonationPos);
	add_field("C_Inferno->m_bFireIsBurning", dump_a2x::schema::C_Inferno__m_bFireIsBurning);
	add_field("C_Inferno->m_firePositions", dump_a2x::schema::C_Inferno__m_firePositions);
	add_field("C_Inferno->m_fireCount", dump_a2x::schema::C_Inferno__m_fireCount);
	add_field("C_Inferno->m_nFireEffectTickBegin", dump_a2x::schema::C_Inferno__m_nFireEffectTickBegin);
	add_field("CSkeletonInstance->m_modelState", dump_a2x::schema::CSkeletonInstance__m_modelState);
	add_field("CModelState->m_ModelName", dump_a2x::schema::CModelState__m_ModelName);

	return !m_schema_data.empty();
}

uint32_t schema::get_offset(const fnv1a_t hashed_field_name)
{
	if (const auto it = std::ranges::find_if(m_schema_data, [hashed_field_name](const schema_data_t& data)
	{
		return data.m_hashed_field_name == hashed_field_name;
	});

	it != m_schema_data.end())
		return it->m_offset;

	LOG_ERROR("failed to find an offset for the field with the hash value '%d'", hashed_field_name);
	return {};
}
