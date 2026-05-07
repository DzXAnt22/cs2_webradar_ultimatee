#include "pch.hpp"
#include "kernel_provider.hpp"
#include "memory.hpp"

// TODO: Define IOCTL codes for kernel communication
// These will match the IOCTL codes defined in the kernel driver
// #define IOCTL_READ_MEMORY ...

c_kernel_provider::~c_kernel_provider()
{
    if (m_device_handle != nullptr && m_device_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_device_handle);
        m_device_handle = nullptr;
    }
}

bool c_kernel_provider::initialize(uint32_t process_id)
{
    // TODO: Implement kernel driver initialization
    // 1. Open handle to kernel driver device using CreateFile
    // 2. Store process ID for subsequent operations
    // 3. Optionally verify driver is loaded and responding

    m_process_id = process_id;

    if (!open_device()) {
        LOG_WARNING("Failed to open kernel driver device.");
        return false;
    }

    m_initialized = true;
    return true;
}

bool c_kernel_provider::read_raw(uintptr_t address, void* buffer, size_t size)
{
    // TODO: Implement kernel-mode memory reading via IOCTL
    // This requires sending an IOCTL request to the kernel driver
    // with the address and size, then receiving the data back.
    //
    // Example structure:
    // struct read_request_t {
    //     uint32_t process_id;
    //     uintptr_t address;
    //     size_t size;
    // };
    //
    // struct read_response_t {
    //     uint8_t data[...];
    // };
    //
    // 1. Build read_request_t with m_process_id, address, and size
    // 2. Send IOCTL to driver via DeviceIoControl
    // 3. Copy response data to buffer
    // 4. Return success/failure

    if (!m_initialized) {
        return false;
    }

    // Placeholder: Not yet implemented
    return false;
}

std::pair<std::optional<uintptr_t>, std::optional<size_t>> c_kernel_provider::get_module_info(
    const std::string_view& module_name)
{
    // TODO: Implement kernel-mode module info retrieval
    // The kernel driver can enumerate modules more reliably
    // than user-mode tools (especially under anti-cheat).
    //
    // Implementation approach:
    // 1. Send IOCTL request to enumerate modules in target process
    // 2. Receive module list from driver
    // 3. Match module_name and return base address and size

    if (!m_initialized) {
        return {std::nullopt, std::nullopt};
    }

    // Placeholder: Not yet implemented
    return {std::nullopt, std::nullopt};
}

void* c_kernel_provider::get_handle() const
{
    return m_device_handle;
}

bool c_kernel_provider::is_initialized() const
{
    return m_initialized;
}

bool c_kernel_provider::open_device()
{
    // TODO: Open handle to kernel driver device
    // This will typically be a named device like:
    // "\\Device\\Cs2WebRadar" or "\\??\\Cs2WebRadar"
    //
    // The driver should create a symbolic link for easier access.
    // Example device path: "\\\\.\\Cs2WebRadar"

    // Placeholder: Device path to be defined when driver is created
    constexpr const char* device_path = "\\\\.\\Cs2WebRadar";

    m_device_handle = CreateFileA(
        device_path,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (m_device_handle == INVALID_HANDLE_VALUE) {
        LOG_WARNING("Failed to open device '%s'. Error: %lu", device_path, GetLastError());
        m_device_handle = nullptr;
        return false;
    }

    return true;
}