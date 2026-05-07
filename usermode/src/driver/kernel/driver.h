#pragma once

#include <ntifs.h>

// Driver entry points
extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
extern "C" NTSTATUS RealDriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);

// Undocumented function to create a driver object
extern "C" NTSTATUS IoCreateDriver(PUNICODE_STRING DriverName, PDRIVER_INITIALIZE InitializationFunction);

// Driver unload routine
void DriverUnload(_In_ PDRIVER_OBJECT DriverObject);

// Device name and symbolic link
constexpr const wchar_t* DEVICE_NAME = L"\\Device\\Cs2WebRadar";
constexpr const wchar_t* DEVICE_SYMBOLIC_LINK = L"\\DosDevices\\Cs2WebRadar";

// Global device object pointer
extern PDEVICE_OBJECT g_device_object;

// IOCTL dispatch handler
NTSTATUS EvtIoDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
);

// Create/Close dispatch handlers
NTSTATUS EvtDeviceCreate(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
);

NTSTATUS EvtDeviceClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp
);