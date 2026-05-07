#include "pch.hpp"
#include "kernel_provider.hpp"
#include "memory.hpp"
#include "../driver/kernel_bridge.h"

c_kernel_provider::~c_kernel_provider()
{
    if (m_bridge) {
        m_bridge->cleanup();
        delete m_bridge;
        m_bridge = nullptr;
    }
}

bool c_kernel_provider::initialize(uint32_t process_id)
{
    if (m_initialized) {
        return true;
    }

    m_process_id = process_id;

    // Create kernel bridge instance
    m_bridge = new kernel_bridge();
    if (!m_bridge) {
        LOG_WARNING("Failed to allocate kernel bridge.");
        return false;
    }

    // Initialize the bridge (opens driver device)
    if (!m_bridge->initialize()) {
        LOG_WARNING("Failed to initialize kernel bridge.");
        delete m_bridge;
        m_bridge = nullptr;
        return false;
    }

    // Verify driver is responding
    if (!m_bridge->verify_driver()) {
        LOG_WARNING("Kernel driver verification failed.");
        m_bridge->cleanup();
        delete m_bridge;
        m_bridge = nullptr;
        return false;
    }

    LOG_INFO("Kernel driver initialized successfully for PID %u.", process_id);
    m_initialized = true;
    return true;
}

bool c_kernel_provider::read_raw(uintptr_t address, void* buffer, size_t size)
{
    if (!m_initialized || !m_bridge) {
        return false;
    }

    if (!buffer || size == 0) {
        return false;
    }

    const size_t bytes_read = m_bridge->read_memory(m_process_id, address, buffer, size);
    return bytes_read == size;
}

std::pair<std::optional<uintptr_t>, std::optional<size_t>> c_kernel_provider::get_module_info(
    const std::string_view& module_name)
{
    if (!m_initialized || !m_bridge) {
        return {std::nullopt, std::nullopt};
    }

    uintptr_t base_address = m_bridge->get_module_base(m_process_id, module_name);
    if (base_address == 0) {
        return {std::nullopt, std::nullopt};
    }

    size_t size = m_bridge->get_module_size(m_process_id, module_name);
    return {base_address, size};
}

void* c_kernel_provider::get_handle() const
{
    // Return the bridge handle if available
    if (m_bridge && m_bridge->is_initialized()) {
        return m_bridge;
    }
    return nullptr;
}

bool c_kernel_provider::is_initialized() const
{
    return m_initialized && m_bridge && m_bridge->is_initialized();
}

bool c_kernel_provider::open_device()
{
    // This method is now a no-op since kernel_bridge handles device opening
    // Kept for interface compatibility
    return is_initialized();
}