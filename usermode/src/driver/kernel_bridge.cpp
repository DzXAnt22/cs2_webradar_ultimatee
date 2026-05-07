#include "pch.hpp"
#include "kernel_bridge.h"
#include "shared/Ioctl.h"

#include <stdexcept>
#include <cstring>

namespace {
    constexpr const char* DEVICE_PATH = "\\\\.\\Cs2WebRadar";
    constexpr uint32_t EXPECTED_SIGNATURE = 0xCS2WR0xD;
}

// Internal context structure for each bridge instance
struct kernel_bridge_context_t {
    void* device_handle = nullptr;
    uint32_t last_error = STATUS_SUCCESS;
    bool valid = false;
};

// C API Implementation

extern "C" KERNEL_BRIDGE_API kernel_bridge_handle __cdecl kernel_bridge_load()
{
    // Allocate context
    auto* context = new kernel_bridge_context_t{};
    if (!context) {
        return nullptr;
    }

    // Open handle to kernel driver
    context->device_handle = CreateFileA(
        DEVICE_PATH,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (context->device_handle == INVALID_HANDLE_VALUE) {
        context->device_handle = nullptr;
        context->last_error = GetLastError();
        delete context;
        return nullptr;
    }

    // Verify driver is loaded and responding
    verify_driver_response_t response = {};
    DWORD bytes_returned = 0;

    BOOL result = DeviceIoControl(
        context->device_handle,
        IOCTL_VERIFY_DRIVER,
        nullptr,
        0,
        &response,
        sizeof(response),
        &bytes_returned,
        nullptr
    );

    if (!result || response.signature != DRIVER_SIGNATURE || response.status != STATUS_SUCCESS) {
        CloseHandle(context->device_handle);
        context->device_handle = nullptr;
        context->last_error = STATUS_UNSUCCESSFUL;
        delete context;
        return nullptr;
    }

    context->valid = true;
    return static_cast<kernel_bridge_handle>(context);
}

extern "C" KERNEL_BRIDGE_API bool __cdecl kernel_bridge_unload(kernel_bridge_handle handle)
{
    if (!handle) {
        return false;
    }

    auto* context = static_cast<kernel_bridge_context_t*>(handle);
    if (context->device_handle && context->device_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(context->device_handle);
    }
    delete context;
    return true;
}

extern "C" KERNEL_BRIDGE_API size_t __cdecl kernel_bridge_read_memory(
    kernel_bridge_handle handle,
    uint32_t process_id,
    uintptr_t address,
    void* buffer,
    size_t size)
{
    if (!handle || !buffer || size == 0) {
        return 0;
    }

    auto* context = static_cast<kernel_bridge_context_t*>(handle);
    if (!context->valid || !context->device_handle) {
        context->last_error = STATUS_INVALID_HANDLE;
        return 0;
    }

    // Validate size
    if (size > MAX_READ_SIZE) {
        context->last_error = STATUS_BUFFER_TOO_SMALL;
        return 0;
    }

    // Prepare request
    read_memory_request_t request = {};
    request.process_id = process_id;
    request.address = address;
    request.size = size;

    // Calculate output buffer size: response header + data
    const size_t output_size = sizeof(read_memory_response_t) + size;
    std::vector<uint8_t> output_buffer(output_size);

    DWORD bytes_returned = 0;
    BOOL result = DeviceIoControl(
        context->device_handle,
        IOCTL_READ_PROCESS_MEMORY,
        &request,
        sizeof(request),
        output_buffer.data(),
        static_cast<DWORD>(output_size),
        &bytes_returned,
        nullptr
    );

    if (!result) {
        context->last_error = GetLastError();
        return 0;
    }

    auto* response = reinterpret_cast<read_memory_response_t*>(output_buffer.data());
    context->last_error = response->status;

    if (!NT_SUCCESS(response->status)) {
        return 0;
    }

    // Copy data to output buffer
    const size_t data_size = bytes_returned - sizeof(read_memory_response_t);
    if (data_size > 0) {
        std::memcpy(
            buffer,
            output_buffer.data() + sizeof(read_memory_response_t),
            std::min(data_size, size)
        );
    }

    return static_cast<size_t>(response->bytes_read);
}

extern "C" KERNEL_BRIDGE_API bool __cdecl kernel_bridge_get_module_info(
    kernel_bridge_handle handle,
    uint32_t process_id,
    const char* module_name,
    uintptr_t* base_address,
    size_t* size)
{
    if (!handle || !module_name || !base_address || !size) {
        return false;
    }

    auto* context = static_cast<kernel_bridge_context_t*>(handle);
    if (!context->valid || !context->device_handle) {
        context->last_error = STATUS_INVALID_HANDLE;
        return false;
    }

    // Prepare request
    get_module_info_request_t request = {};
    request.process_id = process_id;
    std::strncpy(request.module_name, module_name, MAX_MODULE_NAME_LENGTH - 1);
    request.module_name[MAX_MODULE_NAME_LENGTH - 1] = '\0';

    get_module_info_response_t response = {};
    DWORD bytes_returned = 0;

    BOOL result = DeviceIoControl(
        context->device_handle,
        IOCTL_GET_MODULE_INFO,
        &request,
        sizeof(request),
        &response,
        sizeof(response),
        &bytes_returned,
        nullptr
    );

    if (!result) {
        context->last_error = GetLastError();
        return false;
    }

    context->last_error = response.status;

    if (!NT_SUCCESS(response.status) || !response.found) {
        return false;
    }

    *base_address = response.base_address;
    *size = response.size;
    return true;
}

extern "C" KERNEL_BRIDGE_API bool __cdecl kernel_bridge_verify(kernel_bridge_handle handle)
{
    if (!handle) {
        return false;
    }

    auto* context = static_cast<kernel_bridge_context_t*>(handle);
    if (!context->valid || !context->device_handle) {
        return false;
    }

    verify_driver_response_t response = {};
    DWORD bytes_returned = 0;

    BOOL result = DeviceIoControl(
        context->device_handle,
        IOCTL_VERIFY_DRIVER,
        nullptr,
        0,
        &response,
        sizeof(response),
        &bytes_returned,
        nullptr
    );

    return result && response.signature == DRIVER_SIGNATURE && NT_SUCCESS(response.status);
}

extern "C" KERNEL_BRIDGE_API uint32_t __cdecl kernel_bridge_get_last_error(kernel_bridge_handle handle)
{
    if (!handle) {
        return STATUS_INVALID_HANDLE;
    }

    auto* context = static_cast<kernel_bridge_context_t*>(handle);
    return context->last_error;
}

// Helper macro for NT success check
#define NT_SUCCESS(status) (static_cast<long>(status) >= 0)

// C++ Class Implementation

kernel_bridge::kernel_bridge() = default;

kernel_bridge::~kernel_bridge()
{
    cleanup();
}

bool kernel_bridge::initialize()
{
    if (m_initialized) {
        return true;
    }

    m_handle = kernel_bridge_load();
    if (!m_handle) {
        return false;
    }

    m_initialized = true;
    return true;
}

void kernel_bridge::cleanup()
{
    if (m_handle) {
        kernel_bridge_unload(m_handle);
        m_handle = nullptr;
    }
    m_initialized = false;
}

bool kernel_bridge::is_initialized() const
{
    return m_initialized && m_handle != nullptr;
}

size_t kernel_bridge::read_memory(uint32_t process_id, uintptr_t address, void* buffer, size_t size)
{
    if (!is_initialized()) {
        return 0;
    }
    return kernel_bridge_read_memory(m_handle, process_id, address, buffer, size);
}

uintptr_t kernel_bridge::get_module_base(uint32_t process_id, const std::string_view& module_name)
{
    if (!is_initialized()) {
        return 0;
    }

    uintptr_t base_address = 0;
    size_t size = 0;

    // Convert string_view to null-terminated string
    std::string module_name_str(module_name.data(), module_name.size());

    if (kernel_bridge_get_module_info(m_handle, process_id, module_name_str.c_str(), &base_address, &size)) {
        return base_address;
    }

    return 0;
}

size_t kernel_bridge::get_module_size(uint32_t process_id, const std::string_view& module_name)
{
    if (!is_initialized()) {
        return 0;
    }

    uintptr_t base_address = 0;
    size_t size = 0;

    std::string module_name_str(module_name.data(), module_name.size());

    if (kernel_bridge_get_module_info(m_handle, process_id, module_name_str.c_str(), &base_address, &size)) {
        return size;
    }

    return 0;
}

bool kernel_bridge::verify_driver() const
{
    if (!is_initialized()) {
        return false;
    }
    return kernel_bridge_verify(m_handle);
}