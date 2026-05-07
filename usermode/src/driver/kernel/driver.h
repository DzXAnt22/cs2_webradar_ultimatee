#pragma once

#include <ntifs.h>

// Driver entry point
extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);

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