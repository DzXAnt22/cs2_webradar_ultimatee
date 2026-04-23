#include "pch.hpp"

bool f::run()
{
    const auto local_team = sdk::m_local_controller->m_iTeamNum();
    if (local_team == e_team::none || local_team == e_team::spec)
        return false;

    m_data.clear();
    m_player_data.clear();

    m_data["m_local_team"] = local_team;

    get_map();
    get_player_info();
    return true;
}

void f::get_map()
{
    const auto map_name = i::m_global_vars->m_map_name();
    if (map_name.empty() || map_name.find("<empty>") != std::string::npos || map_name == "")
    {
        m_data["m_map"] = "invalid";

        LOG_WARNING("Failed to get map name! Updating m_global_vars!");
        i::refresh_global_vars();
    }

    if (f::features_vars::map_name != map_name) {
        f::bomb::update_bomb_dmg_info(map_name);
        f::features_vars::map_name = map_name;
    }

    m_data["m_map"] = map_name;
}

void f::get_player_info()
{
    m_data["m_players"].clear();
    m_data["m_grenades"]["landed"].clear();
    m_data["m_grenades"]["thrown"].clear();
    m_data["m_dropped_weapons"].clear();

    auto* entity_system = i::m_game_entity_system;
    if (!entity_system)
        return;

    const auto highest_entity_index = m_memory->read_t<int32_t>(reinterpret_cast<uintptr_t>(entity_system) + dump_a2x::offsets::dwGameEntitySystem_highestEntityIndex);
    if (highest_entity_index <= 0)
        return;

    const int32_t highest_idx = std::min(highest_entity_index + 1, ENT_MAX_NETWORKED_ENTRY);

    for (int32_t idx = 0; idx < highest_idx; idx++)
    {
        const auto entity = entity_system->get(idx);
        if (!entity) continue;

        const auto entity_handle = entity->get_ref_e_handle();
        if (!entity_handle.is_valid()) continue;

        const auto class_name = entity->get_schema_class_name();
        const auto hashed_class_name = class_name.empty() ? 0 : fnv1a::hash(class_name);

        const auto designer_name = entity->m_pEntity()->m_designerName();
        const auto hashed_designer_name = designer_name.empty() ? 0 : fnv1a::hash(designer_name);

        auto is_target = [hashed_class_name, hashed_designer_name](const fnv1a_t class_hash, const fnv1a_t designer_hash)
        {
            return hashed_class_name == class_hash || hashed_designer_name == designer_hash;
        };

        if (is_target(hashes::PLAYER_CONTROLLER, hashes::PLAYER_CONTROLLER_DESIGNER)) {
            const auto player = i::m_game_entity_system->get<c_cs_player_controller*>(entity_handle);
            if (!player)
                continue;

            const auto player_pawn = player->get_player_pawn();
            if (!player_pawn)
                continue;

            if (!f::players::get_data(idx, player, player_pawn))
                continue;

            f::players::get_weapons(player_pawn);
            f::players::get_active_weapon(player_pawn);

            m_data["m_players"].push_back(m_player_data);
            continue;
        }

        if (is_target(hashes::C4, hashes::C4_DESIGNER)) {
            f::bomb::get_carried_bomb(entity);
            continue;
        }

        if (is_target(hashes::PLANTED_C4, hashes::PLANTED_C4_DESIGNER)) {
            f::bomb::get_planted_bomb(reinterpret_cast<c_planted_c4*>(entity));
            continue;
        }

        if (is_target(hashes::SMOKE, hashes::SMOKE_DESIGNER)) {
            m_grenade_data.clear();
            m_grenade_thrown_data.clear();

            if (!f::grenades::get_smoke(reinterpret_cast<c_smoke_grenade*>(entity))) {
                if (f::grenades::get_thrown(reinterpret_cast<c_base_grenade*>(entity))) {
                    m_grenade_thrown_data["m_idx"] = idx;
                    m_data["m_grenades"]["thrown"].push_back(m_grenade_thrown_data);
                }
                continue;
            }

            m_grenade_data["m_idx"] = idx;
            m_data["m_grenades"]["landed"].push_back(m_grenade_data);
            continue;
        }

        if (is_target(hashes::INFERNO, hashes::INFERNO_DESIGNER)) {
            m_grenade_data.clear();

            if (!f::grenades::get_molo(reinterpret_cast<c_molo_grenade*>(entity)))
                continue;

            m_grenade_data["m_idx"] = idx;
            m_data["m_grenades"]["landed"].push_back(m_grenade_data);
            continue;
        }

        if (is_target(hashes::HE, hashes::HE_DESIGNER)
            || is_target(hashes::FLASH, hashes::FLASH_DESIGNER)
            || is_target(hashes::DECOY, hashes::DECOY_DESIGNER)
            || is_target(hashes::MOLOTOV, hashes::MOLOTOV_DESIGNER)) {
            m_grenade_thrown_data.clear();

            if (!f::grenades::get_thrown(reinterpret_cast<c_base_grenade*>(entity)))
                continue;

            m_grenade_thrown_data["m_idx"] = idx;
            m_data["m_grenades"]["thrown"].push_back(m_grenade_thrown_data);
            continue;
        }

        if (f::dropped_weapons::is_weapon(designer_name))
        {
            m_dropped_weapon_data.clear();

            if (!f::dropped_weapons::get_weapon(reinterpret_cast<c_base_entity*>(entity)))
                continue;

            m_dropped_weapon_data["m_idx"] = idx;
            m_data["m_dropped_weapons"].push_back(m_dropped_weapon_data);
        }
    }
}