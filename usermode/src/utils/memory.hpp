#pragma once

#define NT_SUCCESS(status) (static_cast<long>(status) >= 0)

struct __system_handle_t
{
    unsigned long m_process_id;
    uint8_t m_object_type_number;
    uint8_t m_flags;
    uint16_t m_handle;
    void* m_object;
    ACCESS_MASK m_granted_access;
};

struct system_handle_info_t
{
    unsigned long m_handle_count;
    __system_handle_t m_handles[1];
};

namespace memory {
    class i_memory_provider;

    enum class provider_type : uint8_t {
        win32,
        kernel
    };
}

class c_memory
{
public:
    ~c_memory();

    void set_provider(memory::provider_type type);
    memory::provider_type get_provider_type() const;

    int setup();
    std::optional<uint32_t> get_process_id(const std::string_view& process_name);
    std::optional<c_address> find_pattern(const std::string_view& module_name, const std::string_view& pattern);
    std::pair<std::optional<uintptr_t>, std::optional<size_t>> get_module_info(const std::string_view& module_name);
    bool is_anticheat_running();

    bool read_t(const uintptr_t address, void* buffer, size_t size)
    {
        if (!m_provider || !m_provider->is_initialized())
            return false;
        return m_provider->read_raw(address, buffer, size);
    }

    template <typename T>
    T read_t(const uintptr_t address) noexcept
    {
        T buffer{};
        if (m_provider && m_provider->is_initialized()) {
            m_provider->read_raw(address, &buffer, sizeof(T));
        }
        return buffer;
    }

    template <typename T>
    T read_t(void* address)
    {
        return read_t<T>(reinterpret_cast<uintptr_t>(address));
    }

    template <>
    std::string read_t<std::string>(const uintptr_t address)
    {
        static constexpr size_t length = 64;
        std::vector<char> buffer(length);

        if (m_provider && m_provider->is_initialized()) {
            m_provider->read_raw(address, buffer.data(), length);
        }

        const auto& it = find(buffer.begin(), buffer.end(), '\0');

        if (it != buffer.end())
            buffer.resize(distance(buffer.begin(), it));

        return std::string(buffer.begin(), buffer.end());
    }

private:
    std::unique_ptr<i_memory_provider> m_provider;
    memory::provider_type m_provider_type = memory::provider_type::win32;
    uint32_t m_id = 0;

    bool read_memory(void* address, void* buffer, size_t size)
    {
        if (!m_provider || !m_provider->is_initialized())
            return false;
        return m_provider->read_raw(reinterpret_cast<uintptr_t>(address), buffer, size);
    }
};

inline const std::unique_ptr<c_memory> m_memory{ new c_memory() };