#include "pch.hpp"
#include "memory.hpp"
#include "memory/win32_provider.hpp"
#include "memory/kernel_provider.hpp"

c_memory::~c_memory()
{
    m_provider.reset();
}

void c_memory::set_provider(memory::provider_type type)
{
    if (type == m_provider_type && m_provider)
        return;

    m_provider.reset();

    switch (type) {
    case memory::provider_type::win32:
        m_provider = std::make_unique<c_win32_provider>();
        break;
    case memory::provider_type::kernel:
        m_provider = std::make_unique<c_kernel_provider>();
        break;
    default:
        m_provider = std::make_unique<c_win32_provider>();
        break;
    }

    m_provider_type = type;
}

memory::provider_type c_memory::get_provider_type() const
{
    return m_provider_type;
}

int c_memory::setup()
{
    if (is_anticheat_running()) {
        LOG_WARNING("Detected running Anti-Cheat software.");
        return 1;
    }

    const auto process_id = get_process_id("cs2.exe");
    if (!process_id.has_value()) {
        LOG_WARNING("No CS2.exe process found.");
        return 2;
    }

    this->m_id = process_id.value();

    if (!m_provider) {
        m_provider = std::make_unique<c_win32_provider>();
        m_provider_type = memory::provider_type::win32;
    }

    if (!m_provider->initialize(m_id)) {
        LOG_ERROR("Failed to initialize memory provider.");
        m_provider.reset();
        return 3;
    }

    return 0;
}

std::optional<uint32_t> c_memory::get_process_id(const std::string_view& process_name)
{
    if (m_provider && m_provider->is_initialized()) {
        if (auto win32_provider = dynamic_cast<c_win32_provider*>(m_provider.get())) {
            return win32_provider->get_process_id(process_name);
        }
    }

    const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return {};

    PROCESSENTRY32 process_entry = {0};
    process_entry.dwSize = sizeof(process_entry);

    for (Process32First(snapshot, &process_entry); Process32Next(snapshot, &process_entry);) {
        if (std::string_view(process_entry.szExeFile) == process_name) {
            CloseHandle(snapshot);
            return process_entry.th32ProcessID;
        }
    }

    CloseHandle(snapshot);
    return {};
}

std::optional<c_address> c_memory::find_pattern(const std::string_view& module_name, const std::string_view& pattern)
{
    if (!m_provider || !m_provider->is_initialized())
        return {};

    constexpr auto pattern_to_bytes = [](const std::string_view& pattern) {
        std::vector<int32_t> bytes;

        for (uint32_t idx = 0; idx < pattern.size(); ++idx) {
            switch (pattern[idx]) {
            case '?':
                bytes.push_back(-1);
                break;

            case ' ':
                break;

            default: {
                if (idx + 1 < pattern.size()) {
                    uint32_t value = 0;

                    if (const auto [ptr, ec] = std::from_chars(pattern.data() + idx,
                                                                pattern.data() + idx + 2,
                                                                value,
                                                                16);
                        ec == std::errc()) {
                        bytes.push_back(static_cast<int32_t>(value));
                        ++idx;
                    }
                }

                break;
            }
            }
        }

        return bytes;
    };

    auto [module_base, module_size] = m_provider->get_module_info(module_name);
    if (!module_base.has_value() || !module_size.has_value())
        return {};

    const auto module_data = std::make_unique<uint8_t[]>(module_size.value());
    if (!m_provider->read_raw(module_base.value(), module_data.get(), module_size.value()))
        return {};

    const auto pattern_bytes = pattern_to_bytes(pattern);
    for (uint32_t idx = 0; idx < module_size.value() - pattern.size(); ++idx) {
        bool found = true;

        for (uint32_t b_idx = 0; b_idx < pattern_bytes.size(); ++b_idx) {
            if (module_data[idx + b_idx] != pattern_bytes[b_idx] && pattern_bytes[b_idx] != -1) {
                found = false;
                break;
            }
        }

        if (found)
            return c_address(module_base.value() + idx);
    }

    return {};
}

bool c_memory::is_anticheat_running()
{
    constexpr std::array<std::string_view, 7> m_process_list = {
        "faceitclient.exe",  // faceit client
        "faceitservice.exe", // faceit service
        "faceit.exe",        // faceit process
        "esportal.exe",      // esportal client
        "perfectworld.exe"   // perfect world (?)
    };

    for (const auto& process_name : m_process_list) {
        const auto process_id = get_process_id(process_name);
        if (!process_id.has_value())
            continue;

        LOG_INFO("Possible anti-cheat process detected ('%s')", process_name.data());
        return true;
    }

    return false;
}

std::pair<std::optional<uintptr_t>, std::optional<size_t>> c_memory::get_module_info(
    const std::string_view& module_name)
{
    if (!m_provider || !m_provider->is_initialized())
        return {std::nullopt, std::nullopt};

    return m_provider->get_module_info(module_name);
}