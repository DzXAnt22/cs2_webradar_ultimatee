#include "pch.hpp"

bool i::setup()
{
	const auto [client_base, client_size] = m_memory->get_module_info(CLIENT_DLL);
	if (!client_base.has_value() || !client_size.has_value())
		return false;

	m_client_base = client_base.value();

	refresh_global_vars();
	m_game_entity_system = m_memory->read_t<c_game_entity_system*>(m_client_base + dump_a2x::offsets::dwGameEntitySystem);

	return m_global_vars != nullptr && m_game_entity_system != nullptr;
}

void i::refresh_global_vars()
{
	if (!m_client_base)
	{
		m_global_vars = nullptr;
		return;
	}

	m_global_vars = m_memory->read_t<c_global_vars*>(m_client_base + dump_a2x::offsets::dwGlobalVars);
}
