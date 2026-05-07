#pragma once

#include "i_memory_provider.hpp"
#include "memory.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>

class c_kernel_provider final : public i_memory_provider {
public:
    ~c_kernel_provider() override;

    bool initialize(uint32_t process_id) override;
    bool read_raw(uintptr_t address, void* buffer, size_t size) override;
    std::pair<std::optional<uintptr_t>, std::optional<size_t>> get_module_info(
        const std::string_view& module_name) override;
    void* get_handle() const override;
    bool is_initialized() const override;

private:
    bool open_device();

    bool m_initialized = false;
    uint32_t m_process_id = 0;
    void* m_device_handle = nullptr;
};