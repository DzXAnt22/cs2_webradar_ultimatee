#include "driver.h"
#include "../shared/Ioctl.h"
#include <ntimage.h>

// Global device object pointer
PDEVICE_OBJECT g_device_object = nullptr;

// Driver entry point
extern "C" NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    NTSTATUS status;

    // Create the device
    status = IoCreateDevice(
        DriverObject,
        0,
        nullptr,
        FILE_DEVICE_CS2_WEBRADAR,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_device_object
    );

    if (!NT_SUCCESS(status)) {
        return status;
    }

    // Create symbolic link for user-mode access
    status = IoCreateSymbolicLink(
        const_cast<PUNICODE_STRING>(reinterpret_cast<const UNICODE_STRING*>(DEVICE_SYMBOLIC_LINK)),
        const_cast<PUNICODE_STRING>(reinterpret_cast<const UNICODE_STRING*>(DEVICE_NAME))
    );

    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(g_device_object);
        g_device_object = nullptr;
        return status;
    }

    // Set up dispatch entry points
    DriverObject->MajorFunction[IRP_MJ_CREATE] = EvtDeviceCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = EvtDeviceClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = EvtIoDeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    // Set up device characteristics
    g_device_object->Flags |= DO_DIRECT_IO;
    g_device_object->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

// Driver unload routine
void DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    if (g_device_object) {
        IoDeleteSymbolicLink(
            const_cast<PUNICODE_STRING>(reinterpret_cast<const UNICODE_STRING*>(DEVICE_SYMBOLIC_LINK))
        );
        IoDeleteDevice(g_device_object);
        g_device_object = nullptr;
    }
}

// Create dispatch handler
NTSTATUS EvtDeviceCreate(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// Close dispatch handler
NTSTATUS EvtDeviceClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// Helper function to open a process by PID
static PEPROCESS OpenProcessById(HANDLE process_id)
{
    PEPROCESS process = nullptr;
    if (!NT_SUCCESS(PsLookupProcessByProcessId(process_id, &process))) {
        return nullptr;
    }
    return process;
}

// Helper function to read process memory
static NTSTATUS ReadProcessMemoryKernel(HANDLE process_id, PVOID source_address, PVOID target_buffer, SIZE_T size, SIZE_T* bytes_read)
{
    PEPROCESS target_process = nullptr;
    NTSTATUS status = PsLookupProcessByProcessId(process_id, &target_process);
    if (!NT_SUCCESS(status)) {
        return STATUS_INVALID_PROCESS;
    }

    status = MmCopyVirtualMemory(
        target_process,
        source_address,
        PsGetCurrentProcess(),
        target_buffer,
        size,
        UserMode,
        bytes_read
    );

    ObDereferenceObject(target_process);
    return status;
}

// Helper function to enumerate modules in a process
static NTSTATUS GetModuleInfoByName(HANDLE process_id, PCHAR module_name, PVOID* base_address, PULONG size)
{
    PEPROCESS target_process = nullptr;
    NTSTATUS status = PsLookupProcessByProcessId(process_id, &target_process);
    if (!NT_SUCCESS(status)) {
        return STATUS_INVALID_PROCESS;
    }

    // Use PEB to find modules
    PEPROCESS cur_process = target_process;
    PPEB peb = PsGetProcessPeb(cur_process);
    if (!peb) {
        ObDereferenceObject(target_process);
        return STATUS_NO_MEMORY;
    }

    // Read PEB_LDR_DATA
    PPEB_LDR_DATA ldr_data = nullptr;
    status = MmCopyVirtualMemory(
        target_process,
        &peb->Ldr,
        PsGetCurrentProcess(),
        &ldr_data,
        sizeof(PVOID),
        UserMode,
        nullptr
    );

    if (!NT_SUCCESS(status) || !ldr_data) {
        ObDereferenceObject(target_process);
        return STATUS_NO_MEMORY;
    }

    // Walk the module list
    PLDR_DATA_TABLE_ENTRY module_entry = nullptr;
    PLIST_ENTRY list_head = nullptr;
    PLIST_ENTRY current_entry = nullptr;

    status = MmCopyVirtualMemory(
        target_process,
        &ldr_data->InMemoryOrderModuleList,
        PsGetCurrentProcess(),
        &list_head,
        sizeof(PLIST_ENTRY),
        UserMode,
        nullptr
    );

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(target_process);
        return STATUS_NO_MEMORY;
    }

    current_entry = list_head->Flink;
    ANSI_STRING module_name_ansi;
    ANSI_STRING target_name_ansi;

    RtlInitAnsiString(&target_name_ansi, module_name);

    while (current_entry != list_head) {
        LDR_DATA_TABLE_ENTRY temp_entry = {0};

        status = MmCopyVirtualMemory(
            target_process,
            CONTAINING_RECORD(current_entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks),
            PsGetCurrentProcess(),
            &temp_entry,
            sizeof(LDR_DATA_TABLE_ENTRY),
            UserMode,
            nullptr
        );

        if (!NT_SUCCESS(status)) {
            current_entry = current_entry->Flink;
            continue;
        }

        module_entry = CONTAINING_RECORD(current_entry, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);

        // Get the full DLL name
        WCHAR full_dll_name_buffer[MAX_PATH] = {0};
        UNICODE_STRING full_dll_name_ansi = {0};

        status = MmCopyVirtualMemory(
            target_process,
            &module_entry->FullDllName,
            PsGetCurrentProcess(),
            &full_dll_name_ansi,
            sizeof(UNICODE_STRING),
            UserMode,
            nullptr
        );

        if (NT_SUCCESS(status) && full_dll_name_ansi.Length > 0 && full_dll_name_ansi.Length < sizeof(full_dll_name_buffer)) {
            status = MmCopyVirtualMemory(
                target_process,
                full_dll_name_ansi.Buffer,
                PsGetCurrentProcess(),
                full_dll_name_buffer,
                full_dll_name_ansi.Length,
                UserMode,
                nullptr
            );

            if (NT_SUCCESS(status)) {
                full_dll_name_buffer[full_dll_name_ansi.Length / sizeof(WCHAR)] = L'\0';

                // Convert to ANSI for comparison
                ANSI_STRING ansi_dll_name = {0};
                RtlUnicodeStringToAnsiString(&ansi_dll_name, &full_dll_name_ansi, TRUE);

                if (NT_SUCCESS(status)) {
                    // Check if this module matches (case insensitive)
                    if (RtlCompareUnicodeString(&full_dll_name_ansi, &target_name_ansi, TRUE) == 0 ||
                        ansi_dll_name.Length > 0) {
                        *base_address = temp_entry.DllBase;
                        *size = temp_entry.SizeOfImage;
                        RtlFreeAnsiString(&ansi_dll_name);
                        ObDereferenceObject(target_process);
                        return STATUS_SUCCESS;
                    }
                    RtlFreeAnsiString(&ansi_dll_name);
                }
            }
        }

        current_entry = current_entry->Flink;
    }

    ObDereferenceObject(target_process);
    return STATUS_NOT_FOUND;
}

// IOCTL dispatch handler
NTSTATUS EvtIoDeviceControl(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    NTSTATUS status = STATUS_NOT_SUPPORTED;
    PIO_STACK_LOCATION irp_sp = IoGetCurrentIrpStackLocation(Irp);
    ULONG output_length = irp_sp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG input_length = irp_sp->Parameters.DeviceIoControl.InputBufferLength;

    Irp->IoStatus.Information = 0;

    switch (irp_sp->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_VERIFY_DRIVER: {
            if (output_length >= sizeof(verify_driver_response_t)) {
                auto response = static_cast<verify_driver_response_t*>(Irp->AssociatedIrp.SystemBuffer);
                response->signature = DRIVER_SIGNATURE;
                response->version = 1;
                response->status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(verify_driver_response_t);
                status = STATUS_SUCCESS;
            }
            break;
        }

        case IOCTL_READ_PROCESS_MEMORY: {
            if (input_length >= sizeof(read_memory_request_t) && output_length > sizeof(read_memory_response_t)) {
                auto request = static_cast<read_memory_request_t*>(Irp->AssociatedIrp.SystemBuffer);
                auto response = static_cast<read_memory_response_t*>(Irp->AssociatedIrp.SystemBuffer);

                if (request->size > MAX_READ_SIZE) {
                    response->status = STATUS_INVALID_ADDRESS;
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }

                SIZE_T bytes_read = 0;
                PVOID target_buffer = static_cast<PVOID>(static_cast<uint8_t*>(Irp->AssociatedIrp.SystemBuffer) + sizeof(read_memory_response_t));

                status = ReadProcessMemoryKernel(
                    reinterpret_cast<HANDLE>(request->process_id),
                    reinterpret_cast<PVOID>(request->address),
                    target_buffer,
                    request->size,
                    &bytes_read
                );

                response->bytes_read = static_cast<uint32_t>(bytes_read);
                response->status = static_cast<uint32_t>(status);
                Irp->IoStatus.Information = sizeof(read_memory_response_t) + bytes_read;
            }
            break;
        }

        case IOCTL_GET_MODULE_INFO: {
            if (input_length >= sizeof(get_module_info_request_t) && output_length >= sizeof(get_module_info_response_t)) {
                auto request = static_cast<get_module_info_request_t*>(Irp->AssociatedIrp.SystemBuffer);
                auto response = static_cast<get_module_info_response_t*>(Irp->AssociatedIrp.SystemBuffer);

                PVOID base_address = nullptr;
                ULONG size = 0;

                status = GetModuleInfoByName(
                    reinterpret_cast<HANDLE>(request->process_id),
                    request->module_name,
                    &base_address,
                    &size
                );

                response->base_address = reinterpret_cast<uintptr_t>(base_address);
                response->size = size;
                response->found = NT_SUCCESS(status) ? 1 : 0;
                response->status = static_cast<uint32_t>(status);
                Irp->IoStatus.Information = sizeof(get_module_info_response_t);
            }
            break;
        }

        case IOCTL_GET_PROCESS_ID: {
            if (input_length >= sizeof(get_process_id_request_t) && output_length >= sizeof(get_process_id_response_t)) {
                auto request = static_cast<get_process_id_request_t*>(Irp->AssociatedIrp.SystemBuffer);
                auto response = static_cast<get_process_id_response_t*>(Irp->AssociatedIrp.SystemBuffer);

                // Implement process enumeration
                // Note: This is simplified - a full implementation would walk the process list
                response->process_id = 0;
                response->status = static_cast<uint32_t>(STATUS_NOT_FOUND);
                status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(get_process_id_response_t);
            }
            break;
        }

        default:
            status = STATUS_NOT_SUPPORTED;
            break;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}