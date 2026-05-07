# CS2 WebRadar Kernel Bridge DLL

This DLL provides a user-mode interface to the CS2 WebRadar kernel driver.

## Files

- `kernel_bridge.h` - DLL interface declarations
- `kernel_bridge.cpp` - DLL implementation

## Features

- **C API**: Functions for loading, reading memory, and getting module info
- **C++ Wrapper**: RAII-style kernel_bridge class for convenience
- **Thread-safe**: Each bridge instance manages its own state

## API Functions

### kernel_bridge_load()
Load the kernel driver and initialize communication.
```cpp
kernel_bridge_handle handle = kernel_bridge_load();
if (handle) {
    // Driver loaded successfully
}
```

### kernel_bridge_unload(handle)
Unload and cleanup.
```cpp
kernel_bridge_unload(handle);
```

### kernel_bridge_read_memory(handle, process_id, address, buffer, size)
Read memory from target process.
```cpp
uint8_t buffer[256];
size_t bytes_read = kernel_bridge_read_memory(handle, pid, 0x1000, buffer, sizeof(buffer));
```

### kernel_bridge_get_module_info(handle, process_id, module_name, base_address, size)
Get module base address and size.
```cpp
uintptr_t base;
size_t module_size;
if (kernel_bridge_get_module_info(handle, pid, "client.dll", &base, &module_size)) {
    // Module found
}
```

### kernel_bridge_verify(handle)
Verify driver is loaded and responding.
```cpp
if (kernel_bridge_verify(handle)) {
    // Driver is verified
}
```

## C++ Wrapper Class

```cpp
kernel_bridge bridge;
if (bridge.initialize()) {
    auto bytes = bridge.read_memory(pid, address, buffer, size);
    auto base = bridge.get_module_base(pid, "client.dll");
    bridge.cleanup();
}
```

## Building

This DLL is built as part of the main usermode project. Add `kernel_bridge.cpp` to your project and include `kernel_bridge.h`.

## Dependencies

- Windows SDK
- Ioctl.h (from shared directory)