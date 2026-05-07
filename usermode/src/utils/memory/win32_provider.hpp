#pragma once

#include "memory.hpp"
#include "i_memory_provider.hpp"

#include <windows.h>
#include <tlhelp32.h>
#include <optional>
#include <string_view>
#include <utility>

class c_win32_provider final : public i_memory_provider {
public:
    ~c_win32_provider() override;

    bool initialize(uint32_t process_id) override;
    bool read_raw(uintptr_t address, void* buffer, size_t size) override;
    std::pair<std::optional<uintptr_t>, std::optional<size_t>> get_module_info(
        const std::string_view& module_name) override;
    void* get_handle() const override;
    bool is_initialized() const override;

    std::optional<void*> hijack_handle();
    std::optional<uint32_t> get_process_id(const std::string_view& process_name);

private:
    bool m_initialized = false;
    void* m_handle = nullptr;
    uint32_t m_process_id = 0;

    bool attempt_handle_hijack();
    bool attempt_standard_open();
};