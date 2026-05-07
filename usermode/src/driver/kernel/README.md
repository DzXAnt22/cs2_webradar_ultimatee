# CS2 WebRadar Kernel Driver

This directory contains the Windows kernel driver implementation for CS2 WebRadar.

## Files

- `driver.h` - Driver declarations
- `driver.cpp` - Driver implementation with IOCTL handlers
- `driver.inf` - Driver installation information file
- `sources` - WDK build configuration file

## Building the Driver

### Prerequisites
- Windows Driver Kit (WDK) for Windows 10/11
- Visual Studio with WDK integration

### Build Steps

1. Open a Developer Command Prompt for VS2022
2. Navigate to this directory
3. Run the WDK build command:
   ```
   msbuild driver.vcxproj /p:Configuration=Release /p:Platform=x64
   ```

Or use the WDK build environment:
```
build -ceZ
```

### Installation (requires admin)
```
sc create Cs2WebRadar binPath= C:\path\to\Cs2WebRadar.sys type= kernel
sc start Cs2WebRadar
```

Or using the INF file:
```
devcon install driver.inf "Cs2WebRadar"
```

### Uninstallation
```
sc stop Cs2WebRadar
sc delete Cs2WebRadar
```

## Driver Features

1. **IOCTL_VERIFY_DRIVER** - Verify driver is loaded and responding
2. **IOCTL_READ_PROCESS_MEMORY** - Read memory from any process
3. **IOCTL_GET_MODULE_INFO** - Get module base address and size
4. **IOCTL_GET_PROCESS_ID** - Get process ID by name

## IOCTL Codes

The IOCTL codes are defined in `../shared/Ioctl.h` and must match between kernel and user-mode:

```cpp
#define IOCTL_READ_PROCESS_MEMORY  CTL_CODE(0x9000, 0x100, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_GET_MODULE_INFO      CTL_CODE(0x9000, 0x101, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)
#define IOCTL_GET_PROCESS_ID       CTL_CODE(0x9000, 0x102, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_VERIFY_DRIVER        CTL_CODE(0x9000, 0x103, METHOD_BUFFERED, FILE_ANY_ACCESS)
```

## Device Path

User-mode applications communicate with the driver via:
```
\\.\Cs2WebRadar
```

## Security Notes

- The driver requires administrator privileges to install
- The driver should be signed with a valid certificate for production use
- For development, test signing can be enabled with:
  ```
  bcdedit /set testsigning on
  ```