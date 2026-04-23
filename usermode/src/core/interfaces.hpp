#pragma once

#include <cstdint>

class c_game_entity_system;
class c_global_vars;

namespace i
{
    bool setup();
    void refresh_global_vars();

    inline uintptr_t m_client_base = 0;
    inline c_game_entity_system* m_game_entity_system = nullptr;
    inline c_global_vars* m_global_vars = nullptr;
}