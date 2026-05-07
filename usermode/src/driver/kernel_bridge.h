#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

// Kernel bridge DLL handle type
using kernel_bridge_handle = void*;

// DLL export functions
#ifdef KERNEL_BRIDGE_EXPORTS
#define KERNEL_BRIDGE_API __declspec(dllexport)
#else
#define KERNEL_BRIDGE_API __declspec(dllimport)
#endif

// Load the kernel driver and initialize communication
// Returns: Valid handle on success, nullptr on failure
// Note: Requires administrator privileges
extern "C" KERNEL_BRIDGE_API kernel_bridge_handle __cdecl kernel_bridge_load();

// Unload and cleanup
// handle: Handle returned from kernel_bridge_load
// Returns: true on success
extern "C" KERNEL_BRIDGE_API bool __cdecl kernel_bridge_unload(kernel_bridge_handle handle);

// Read memory from a target process via kernel driver
// handle: Handle from kernel_bridge_load
// process_id: Target process ID
// address: Address to read from
// buffer: Buffer to store read data
// size: Number of bytes to read
// Returns: Number of bytes read, or 0 on failure
extern "C" KERNEL_BRIDGE_API size_t __cdecl kernel_bridge_read_memory(
    kernel_bridge_handle handle,
    uint32_t process_id,
    uintptr_t address,
    void* buffer,
    size_t size
);

// Get module information for a process
// handle: Handle from kernel_bridge_load
// process_id: Target process ID
// module_name: Name of the module (e.g., "client.dll")
// Returns: Tuple of (base_address, size), or nullopt on failure
// Note: module_name should NOT include path, just the DLL name
extern "C" KERNEL_BRIDGE_API bool __cdecl kernel_bridge_get_module_info(
    kernel_bridge_handle handle,
    uint32_t process_id,
    const char* module_name,
    uintptr_t* base_address,
    size_t* size
);

// Verify the driver is loaded and responding
// handle: Handle from kernel_bridge_load
// Returns: true if driver is loaded and verified
extern "C" KERNEL_BRIDGE_API bool __cdecl kernel_bridge_verify(kernel_bridge_handle handle);

// Get the last error code
// Returns: Last error code from driver operations
extern "C" KERNEL_BRIDGE_API uint32_t __cdecl kernel_bridge_get_last_error(kernel_bridge_handle handle);

// Helper class for RAII-style resource management
class kernel_bridge {
public:
    kernel_bridge();
    ~kernel_bridge();

    kernel_bridge(const kernel_bridge&) = delete;
    kernel_bridge& operator=(const kernel_bridge&) = delete;
    kernel_bridge(kernel_bridge&&) = delete;
    kernel_bridge& operator=(kernel_bridge&&) = delete;

    // Initialize the kernel bridge
    // Returns: true on success
    bool initialize();

    // Cleanup resources
    void cleanup();

    // Check if initialized
    bool is_initialized() const;

    // Read memory from target process
    // Returns: Number of bytes read
    size_t read_memory(uint32_t process_id, uintptr_t address, void* buffer, size_t size);

    // Get module information
    // Returns: Base address of module, or 0 on failure
    uintptr_t get_module_base(uint32_t process_id, const std::string_view& module_name);

    // Get module size
    size_t get_module_size(uint32_t process_id, const std::string_view& module_name);

    // Verify driver is loaded
    bool verify_driver() const;

private:
    kernel_bridge_handle m_handle = nullptr;
    bool m_initialized = false;
};