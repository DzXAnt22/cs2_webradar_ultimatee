#include "pch.hpp"

namespace
{
    std::string to_lower_copy(std::string_view value)
    {
        std::string lowered{ value };
        for (auto& ch : lowered)
        {
            if (ch >= 'A' && ch <= 'Z')
                ch = static_cast<char>(ch - 'A' + 'a');
            else if (ch == '-')
                ch = '_';
        }

        return lowered;
    }

    bool contains_any(std::string_view value, std::initializer_list<std::string_view> needles)
    {
        for (const auto needle : needles)
        {
            if (value.find(needle) != std::string::npos)
                return true;
        }

        return false;
    }

    bool is_player_controller_entity(const fnv1a_t hashed_class_name, const fnv1a_t hashed_designer_name, std::string_view class_name, std::string_view designer_name)
    {
        if (hashed_class_name == hashes::PLAYER_CONTROLLER || hashed_designer_name == hashes::PLAYER_CONTROLLER_DESIGNER)
            return true;

        const auto class_lower = to_lower_copy(class_name);
        const auto designer_lower = to_lower_copy(designer_name);
        return class_lower.find("playercontroller") != std::string::npos || designer_lower.find("player_controller") != std::string::npos;
    }

    bool is_carried_c4_entity(const fnv1a_t hashed_class_name, const fnv1a_t hashed_designer_name, std::string_view class_name, std::string_view designer_name)
    {
        if (hashed_class_name == hashes::C4 || hashed_designer_name == hashes::C4_DESIGNER)
            return true;

        const auto class_lower = to_lower_copy(class_name);
        const auto designer_lower = to_lower_copy(designer_name);

        const auto class_mentions_c4 = contains_any(class_lower, { "c4", "weaponc4" }) && class_lower.find("planted") == std::string::npos;
        const auto designer_mentions_c4 = contains_any(designer_lower, { "weapon_c4", "c4" }) && designer_lower.find("planted") == std::string::npos;

        return class_mentions_c4 || designer_mentions_c4;
    }

    bool is_planted_c4_entity(const fnv1a_t hashed_class_name, const fnv1a_t hashed_designer_name, std::string_view class_name, std::string_view designer_name)
    {
        if (hashed_class_name == hashes::PLANTED_C4 || hashed_designer_name == hashes::PLANTED_C4_DESIGNER)
            return true;

        const auto class_lower = to_lower_copy(class_name);
        const auto designer_lower = to_lower_copy(designer_name);
        return contains_any(class_lower, { "plantedc4", "planted_c4" }) || contains_any(designer_lower, { "planted_c4", "plantedc4" });
    }

    enum class grenade_entity_kind
    {
        none,
        smoke,
        inferno,
        thrown,
    };

    grenade_entity_kind classify_grenade_entity(const fnv1a_t hashed_class_name, const fnv1a_t hashed_designer_name, std::string_view class_name, std::string_view designer_name)
    {
        const auto is_hash_match = [hashed_class_name, hashed_designer_name](const fnv1a_t class_hash, const fnv1a_t designer_hash)
        {
            return hashed_class_name == class_hash || hashed_designer_name == designer_hash;
        };

        if (is_hash_match(hashes::SMOKE, hashes::SMOKE_DESIGNER))
            return grenade_entity_kind::smoke;

        if (is_hash_match(hashes::INFERNO, hashes::INFERNO_DESIGNER))
            return grenade_entity_kind::inferno;

        if (is_hash_match(hashes::HE, hashes::HE_DESIGNER)
            || is_hash_match(hashes::FLASH, hashes::FLASH_DESIGNER)
            || is_hash_match(hashes::DECOY, hashes::DECOY_DESIGNER)
            || is_hash_match(hashes::MOLOTOV, hashes::MOLOTOV_DESIGNER)
            || is_hash_match(hashes::INCENDIARY, hashes::INCENDIARY_DESIGNER))
            return grenade_entity_kind::thrown;

        const auto class_lower = to_lower_copy(class_name);
        const auto designer_lower = to_lower_copy(designer_name);

        const auto projectile_like = contains_any(class_lower, { "projectile", "grenadeprojectile", "basecsgrenade", "basegrenade", "inferno" })
            || designer_lower.find("projectile") != std::string::npos
            || designer_lower == "inferno";

        if (projectile_like && (contains_any(class_lower, { "smokegrenade", "smoke_grenade", "smoke" }) || contains_any(designer_lower, { "smokegrenade", "smoke_grenade", "smoke" })))
            return grenade_entity_kind::smoke;

        if (contains_any(class_lower, { "inferno", "incgrenade", "incendiary" }) || contains_any(designer_lower, { "inferno", "incgrenade", "incendiary" }))
            return grenade_entity_kind::inferno;

        const auto known_grenade_name = contains_any(class_lower, { "hegrenade", "flashbang", "decoy", "molotov", "incgrenade", "incendiary", "frag" })
            || contains_any(designer_lower, { "hegrenade", "flashbang", "decoy", "molotov", "incgrenade", "smokegrenade", "incendiary", "frag" });

        if (projectile_like && known_grenade_name)
            return grenade_entity_kind::thrown;

        return grenade_entity_kind::none;
    }
}

bool f::run()
{
    m_data.clear();
    m_player_data.clear();

    const auto local_team = sdk::m_local_controller->m_iTeamNum();
    m_data["m_local_team"] = local_team;
    m_data["local_team"] = local_team;

    if (local_team == e_team::none || local_team == e_team::spec)
    {
        m_data["m_map"] = "invalid";
        m_data["map"] = "invalid";
        m_data["m_players"] = nlohmann::json::array();
        m_data["players"] = nlohmann::json::array();
        m_data["m_grenades"]["landed"] = nlohmann::json::array();
        m_data["m_grenades"]["thrown"] = nlohmann::json::array();
        m_data["grenades"] = m_data["m_grenades"];
        m_data["grenade_data"] = m_data["m_grenades"];
        m_data["m_dropped_weapons"] = nlohmann::json::array();
        m_data["dropped_weapons"] = nlohmann::json::array();
        m_data.erase("m_bomb");
        m_data.erase("bomb");
        m_data.erase("m_bomb_data");
        return false;
    }

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
        m_data["map"] = "invalid";

        LOG_WARNING("Failed to get map name! Updating m_global_vars!");
        i::refresh_global_vars();
        return;
    }

    if (f::features_vars::map_name != map_name) {
        f::bomb::update_bomb_dmg_info(map_name);
        f::features_vars::map_name = map_name;
    }

    m_data["m_map"] = map_name;
    m_data["map"] = map_name;
}

void f::get_player_info()
{
    m_bomb_idx = 0;
    f::features_vars::bomb_blow_time = 0.f;
    f::features_vars::bomb_vec = {};

    m_data["m_players"] = nlohmann::json::array();
    m_data["m_grenades"]["landed"] = nlohmann::json::array();
    m_data["m_grenades"]["thrown"] = nlohmann::json::array();
    m_data["m_dropped_weapons"] = nlohmann::json::array();

    auto* entity_system = i::m_game_entity_system;
    if (entity_system)
    {
        const auto highest_entity_index = m_memory->read_t<int32_t>(reinterpret_cast<uintptr_t>(entity_system) + dump_a2x::offsets::dwGameEntitySystem_highestEntityIndex);
        if (highest_entity_index > 0)
        {
            const int32_t highest_idx = std::min(highest_entity_index + 1, ENT_MAX_NETWORKED_ENTRY);

            for (int32_t idx = 0; idx < highest_idx; idx++)
            {
                const auto entity = entity_system->get(idx);
                if (!entity)
                    continue;

                const auto identity = entity->m_pEntity();
                if (!identity)
                    continue;

                const auto entity_handle = entity->get_ref_e_handle();
                const auto class_name = entity->get_schema_class_name();
                const auto hashed_class_name = class_name.empty() ? 0 : fnv1a::hash(class_name);

                const auto designer_name = identity->m_designerName();
                const auto hashed_designer_name = designer_name.empty() ? 0 : fnv1a::hash(designer_name);

                if (is_player_controller_entity(hashed_class_name, hashed_designer_name, class_name, designer_name)) {
                    if (!entity_handle.is_valid())
                        continue;

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

                if (is_carried_c4_entity(hashed_class_name, hashed_designer_name, class_name, designer_name)) {
                    f::bomb::get_carried_bomb(entity);
                    continue;
                }

                if (is_planted_c4_entity(hashed_class_name, hashed_designer_name, class_name, designer_name)) {
                    f::bomb::get_planted_bomb(reinterpret_cast<c_planted_c4*>(entity));
                    continue;
                }

                const auto grenade_kind = classify_grenade_entity(hashed_class_name, hashed_designer_name, class_name, designer_name);
                if (grenade_kind == grenade_entity_kind::smoke) {
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

                if (grenade_kind == grenade_entity_kind::inferno) {
                    m_grenade_data.clear();
                    m_grenade_thrown_data.clear();

                    if (f::grenades::get_molo(reinterpret_cast<c_molo_grenade*>(entity))) {
                        m_grenade_data["m_idx"] = idx;
                        m_data["m_grenades"]["landed"].push_back(m_grenade_data);
                        continue;
                    }

                    if (f::grenades::get_thrown(reinterpret_cast<c_base_grenade*>(entity))) {
                        m_grenade_thrown_data["m_idx"] = idx;
                        m_data["m_grenades"]["thrown"].push_back(m_grenade_thrown_data);
                        continue;
                    }
                }

                if (grenade_kind == grenade_entity_kind::thrown) {
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
    }

    m_data["players"] = m_data["m_players"];
    m_data["grenades"] = m_data["m_grenades"];
    m_data["grenade_data"] = m_data["m_grenades"];
    m_data["dropped_weapons"] = m_data["m_dropped_weapons"];

    if (m_data.contains("m_bomb")) {
        m_data["bomb"] = m_data["m_bomb"];
        m_data["m_bomb_data"] = m_data["m_bomb"];
    }
    else {
        m_data.erase("bomb");
        m_data.erase("m_bomb_data");
    }
}