#pragma once

#include <cstdint>
#include <cstddef>

#ifdef _KERNEL_MODE
#include <ntifs.h>
#else
#include <windows.h>
#endif

// Device type for our driver
constexpr uint16_t FILE_DEVICE_CS2_WEBRADAR = 0x9000;

// IOCTL codes using CTL_CODE macro
// Format: (DeviceType << 16) | (Access << 14) | (Function << 2) | (Method)
#define IOCTL_READ_PROCESS_MEMORY  CTL_CODE(FILE_DEVICE_CS2_WEBRADAR, 0x100, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_GET_MODULE_INFO      CTL_CODE(FILE_DEVICE_CS2_WEBRADAR, 0x101, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_GET_PROCESS_ID       CTL_CODE(FILE_DEVICE_CS2_WEBRADAR, 0x102, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VERIFY_DRIVER        CTL_CODE(FILE_DEVICE_CS2_WEBRADAR, 0x103, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Maximum module name length
constexpr size_t MAX_MODULE_NAME_LENGTH = 64;

// Maximum number of modules to enumerate
constexpr size_t MAX_MODULE_COUNT = 128;

// Maximum read size (4MB)
constexpr size_t MAX_READ_SIZE = 0x400000;

// Signature for driver verification
constexpr uint32_t DRIVER_SIGNATURE = 0xCS2WR0xD;

// Request structure for reading process memory
struct read_memory_request_t {
    uint32_t process_id;
    uintptr_t address;
    size_t size;
};

// Response structure for read memory (data follows this header)
// The actual read data starts immediately after this structure
struct read_memory_response_t {
    uint32_t bytes_read;
    uint32_t status;
};

// Request structure for getting module info
struct get_module_info_request_t {
    uint32_t process_id;
    char module_name[MAX_MODULE_NAME_LENGTH];
};

// Response structure for module info query
struct get_module_info_response_t {
    uintptr_t base_address;
    size_t size;
    uint32_t found;
    uint32_t status;
};

// Structure for returning multiple module info entries
struct module_entry_t {
    char name[MAX_MODULE_NAME_LENGTH];
    uintptr_t base_address;
    size_t size;
};

struct get_all_modules_request_t {
    uint32_t process_id;
    uint32_t buffer_size;
};

struct get_all_modules_response_t {
    uint32_t module_count;
    uint32_t status;
    // module_entry_t entries follow
};

// Request structure for getting process ID by name
struct get_process_id_request_t {
    char process_name[MAX_MODULE_NAME_LENGTH];
};

struct get_process_id_response_t {
    uint32_t process_id;
    uint32_t status;
};

// Driver verification response
struct verify_driver_response_t {
    uint32_t signature;
    uint32_t version;
    uint32_t status;
};

// Helper to get pointer to module entries from response
inline module_entry_t* get_module_entries(get_all_modules_response_t* response) {
    return reinterpret_cast<module_entry_t*>(response + 1);
}

// Status codes returned by the driver
constexpr uint32_t STATUS_SUCCESS = 0x00000000;
constexpr uint32_t STATUS_INVALID_PROCESS = 0xC0000001;
constexpr uint32_t STATUS_ACCESS_DENIED = 0xC0000022;
constexpr uint32_t STATUS_INVALID_ADDRESS = 0xC0000006;
constexpr uint32_t STATUS_BUFFER_TOO_SMALL = 0xC0000023;
constexpr uint32_t STATUS_NO_MEMORY = 0xC0000017;
constexpr uint32_t STATUS_NOT_FOUND = 0xC0000225;