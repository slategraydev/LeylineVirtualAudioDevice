// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DRIVER ENTRY & INITIALIZATION
// Implements `DriverEntry` and manages the WDM driver lifecycle.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define INITGUID
#include "leyline_miniport.h"

PDEVICE_OBJECT g_FunctionalDeviceObject = nullptr;
extern PDEVICE_OBJECT g_ControlDeviceObject;
extern ULONGLONG      g_EtwRegHandle;
extern "C" NTSTATUS NTAPI AddDevice(PDRIVER_OBJECT, PDEVICE_OBJECT);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DriverUnload
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static void NTAPI DriverUnload(PDRIVER_OBJECT /*DriverObject*/)
{
    DbgPrint("Leyline: DriverUnload\n");

    if (g_FunctionalDeviceObject)
    {
        DeviceExtension *ext = GetDeviceExtension(g_FunctionalDeviceObject);
        if (ext)
        {
            if (ext->LoopbackMdl)
            {
                if (ext->LoopbackBuffer) MmUnmapLockedPages(ext->LoopbackBuffer, ext->LoopbackMdl);
                MmFreePagesFromMdl(ext->LoopbackMdl);
                IoFreeMdl(ext->LoopbackMdl);
                ext->LoopbackMdl = nullptr;
            }
            if (ext->SharedParamsMdl)
            {
                if (ext->SharedParams) MmUnmapLockedPages(ext->SharedParams, ext->SharedParamsMdl);
                MmFreePagesFromMdl(ext->SharedParamsMdl);
                IoFreeMdl(ext->SharedParamsMdl);
                ext->SharedParamsMdl = nullptr;
            }
        }
    }

    if (g_ControlDeviceObject)
    {
        UNICODE_STRING linkName;
        RtlInitUnicodeString(&linkName, L"\\DosDevices\\LeylineAudio");
        IoDeleteSymbolicLink(&linkName);
        IoDeleteDevice(g_ControlDeviceObject);
        g_ControlDeviceObject = nullptr;
        DbgPrint("Leyline: CDO and Symbolic Link Cleaned Up\n");
    }

    if (g_EtwRegHandle)
    {
        EtwUnregister(g_EtwRegHandle);
        g_EtwRegHandle = 0;
    }
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DriverEntry
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    DbgPrint("Leyline: DriverEntry v0.1.0\n");

    EtwRegister(&ETW_PROVIDER_GUID, nullptr, nullptr, &g_EtwRegHandle);

    DriverObject->DriverUnload = DriverUnload;

    NTSTATUS status = PcInitializeAdapterDriver(DriverObject, RegistryPath, AddDevice);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("Leyline: PcInitializeAdapterDriver FAILED 0x%X\n", status);
        return status;
    }

    return status;
}
