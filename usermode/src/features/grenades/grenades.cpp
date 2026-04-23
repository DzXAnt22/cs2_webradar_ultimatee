#include "pch.hpp"
#include <algorithm>

namespace
{
    constexpr float TICK_INTERVAL = 0.015625f;
    constexpr auto TICKS_TO_TIME(int ticks)
    {
        return TICK_INTERVAL * ticks;
    }

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

    void erase_prefix_if_present(std::string& value, std::string_view prefix)
    {
        if (value.starts_with(prefix))
            value.erase(0, prefix.size());
    }

    void erase_suffix_if_present(std::string& value, std::string_view suffix)
    {
        if (value.ends_with(suffix))
            value.erase(value.size() - suffix.size());
    }

    std::string normalize_grenade_type(std::string value)
    {
        if (value.empty())
            return {};

        value = to_lower_copy(value);
        erase_prefix_if_present(value, "weapon_");
        erase_prefix_if_present(value, "c_");
        erase_suffix_if_present(value, "_projectile");
        erase_suffix_if_present(value, "projectile");

        if (value.find("smoke") != std::string::npos)
            return "smokegrenade";

        if (value.find("flash") != std::string::npos)
            return "flashbang";

        if (value.find("decoy") != std::string::npos)
            return "decoy";

        if (value.find("molotov") != std::string::npos)
            return "molotov";

        if (value.find("incgrenade") != std::string::npos || value.find("incendiary") != std::string::npos || value.find("inferno") != std::string::npos)
            return "incgrenade";

        if (value.find("hegrenade") != std::string::npos || value == "he" || value.find("frag") != std::string::npos)
            return "hegrenade";

        return {};
    }

    f_vector get_entity_origin(c_base_entity* entity)
    {
        if (!entity)
            return {};

        const auto scene_node = entity->m_pGameSceneNode();
        if (!scene_node)
            return {};

        auto origin = scene_node->m_vecOrigin();
        if (origin.is_zero())
            origin = scene_node->m_vecAbsOrigin();

        return origin;
    }

    std::string resolve_grenade_type(c_base_grenade* nade)
    {
        if (!nade)
            return {};

        if (const auto weapon_data = nade->m_WeaponData(); weapon_data)
        {
            if (const auto normalized = normalize_grenade_type(weapon_data->m_szName()); !normalized.empty())
                return normalized;
        }

        if (const auto identity = nade->m_pEntity(); identity)
        {
            if (const auto normalized = normalize_grenade_type(identity->m_designerName()); !normalized.empty())
                return normalized;
        }

        return normalize_grenade_type(nade->get_schema_class_name());
    }

    void log_unknown_grenade_type_once(std::string_view class_name, std::string_view designer_name)
    {
        static std::set<std::string> logged_identifiers{};

        std::string key{ class_name };
        key += "|";
        key += designer_name;
        if (!logged_identifiers.insert(key).second)
            return;

        LOG_WARNING("Unable to normalize grenade entity (class='%s', designer='%s')", std::string(class_name).c_str(), std::string(designer_name).c_str());
    }
}

bool f::grenades::get_smoke(c_smoke_grenade* smoke)
{
    const auto begin_tick = smoke->m_nSmokeEffectTickBegin();
    if (begin_tick <= 0)
        return false;

    const auto curtime = i::m_global_vars->m_curtime();
    const auto dis_time = 21.5f - (curtime - TICKS_TO_TIME(begin_tick));
    if (dis_time <= 0.f)
        return false;

    auto det_pos = smoke->m_vSmokeDetonationPos();
    if (det_pos.is_zero())
        det_pos = get_entity_origin(smoke);

    if (det_pos.is_zero())
        return false;

    m_grenade_data = {
        {"m_type", "smoke"},
        {"m_timeleft", dis_time},
        {"m_x", det_pos.m_x},
        {"m_y", det_pos.m_y}
    };

    return true;
}

bool f::grenades::get_molo(c_molo_grenade* molo)
{
    const auto begin_tick = molo->m_nFireEffectTickBegin();
    if (begin_tick <= 0)
        return false;

    const auto curtime = i::m_global_vars->m_curtime();
    const auto dis_time = 7.f - (curtime - TICKS_TO_TIME(begin_tick));
    if (dis_time <= 0.f)
        return false;

    const auto vec_origin = get_entity_origin(molo);
    if (vec_origin.is_zero())
        return false;

    auto firePosLocal = nlohmann::json::array();

    const auto fireBurning = molo->m_bFireIsBurning();
    const auto firePositions = molo->m_firePositions();
    const auto fireCount = std::clamp(molo->m_fireCount(), 0, 64);

    for (int i = 0; i < fireCount; i++)
    {
        if (!fireBurning[i])
            continue;

        firePosLocal.push_back({ firePositions[i].m_x, firePositions[i].m_y });
    }

    if (firePosLocal.empty())
        firePosLocal.push_back({ vec_origin.m_x, vec_origin.m_y });

    m_grenade_data = {
        {"m_type", "molo"},
        {"m_timeleft", dis_time},
        {"m_x", vec_origin.m_x},
        {"m_y", vec_origin.m_y},
        {"m_firePositions", std::move(firePosLocal)}
    };

    return true;
}

bool f::grenades::get_thrown(c_base_grenade* nade)
{
    const auto nadePos = get_entity_origin(nade);
    if (nadePos.is_zero())
        return false;

    const auto grenade_type = resolve_grenade_type(nade);
    if (grenade_type.empty())
    {
        std::string class_name = nade->get_schema_class_name();
        std::string designer_name = {};
        if (const auto identity = nade->m_pEntity(); identity)
            designer_name = identity->m_designerName();

        log_unknown_grenade_type_once(class_name, designer_name);
        return false;
    }

    m_grenade_thrown_data = {
        {"m_type", grenade_type},
        {"m_x", nadePos.m_x},
        {"m_y", nadePos.m_y}
    };

    return true;
}