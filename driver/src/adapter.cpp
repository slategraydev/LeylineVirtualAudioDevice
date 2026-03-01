// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ADAPTER MANAGEMENT & PORTCLS ORCHESTRATION
// Registers subdevices, manages physical connections, creates the CDO.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "leyline_miniport.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// GLOBALS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

PDEVICE_OBJECT g_ControlDeviceObject    = nullptr;
extern PDEVICE_OBJECT g_FunctionalDeviceObject;
ULONGLONG      g_EtwRegHandle           = 0;

static PDRIVER_DISPATCH s_OriginalDispatchCreate  = nullptr;
static PDRIVER_DISPATCH s_OriginalDispatchClose   = nullptr;
static PDRIVER_DISPATCH s_OriginalDispatchControl = nullptr;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// IRP DISPATCH ROUTINES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    if (DeviceObject != g_ControlDeviceObject)
    {
        if (s_OriginalDispatchCreate)
            return s_OriginalDispatchCreate(DeviceObject, Irp);
        return STATUS_DEVICE_NOT_READY;
    }
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    if (DeviceObject != g_ControlDeviceObject)
    {
        if (s_OriginalDispatchClose)
            return s_OriginalDispatchClose(DeviceObject, Irp);
        return STATUS_DEVICE_NOT_READY;
    }
    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    if (DeviceObject != g_ControlDeviceObject)
    {
        if (s_OriginalDispatchControl)
            return s_OriginalDispatchControl(DeviceObject, Irp);
        return STATUS_DEVICE_NOT_READY;
    }

    PIO_STACK_LOCATION stack    = IoGetCurrentIrpStackLocation(Irp);
    ULONG              ioctl    = stack->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS           status   = STATUS_SUCCESS;
    ULONG_PTR          info     = 0;

    switch (ioctl)
    {
    case IOCTL_LEYLINE_GET_STATUS:
        if (stack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(ULONG))
        {
            *reinterpret_cast<ULONG*>(Irp->AssociatedIrp.SystemBuffer) = 0x1337BEEF;
            info = sizeof(ULONG);
        }
        else status = STATUS_BUFFER_TOO_SMALL;
        break;

    case IOCTL_LEYLINE_MAP_BUFFER:
        if (g_FunctionalDeviceObject)
        {
            DeviceExtension *ext = GetDeviceExtension(g_FunctionalDeviceObject);
            if (ext->LoopbackMdl)
            {
                PVOID userAddr = MmMapLockedPagesSpecifyCache(ext->LoopbackMdl, UserMode, MmCached, nullptr, FALSE, NormalPagePriority);
                if (userAddr)
                {
                    *reinterpret_cast<PVOID*>(Irp->AssociatedIrp.SystemBuffer) = userAddr;
                    info = sizeof(PVOID);
                }
                else status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else status = STATUS_DEVICE_NOT_READY;
        }
        else status = STATUS_DEVICE_NOT_READY;
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status      = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// StartDevice - PORTCLS CALLBACK
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern "C" NTSTATUS NTAPI StartDevice(PDEVICE_OBJECT DeviceObject, PIRP Irp, PRESOURCELIST ResourceList)
{
    NTSTATUS status;
    DeviceExtension *devExt = GetDeviceExtension(DeviceObject);

    if (!devExt->LoopbackMdl)
    {
        devExt->LoopbackSize = 128 * 1024;
        PHYSICAL_ADDRESS low = {0}, high = {0}, skip = {0};
        high.LowPart = 0xFFFFFFFF;
        devExt->LoopbackMdl = MmAllocatePagesForMdlEx(low, high, skip, devExt->LoopbackSize, MmCached, MM_ALLOCATE_FULLY_REQUIRED);
        if (devExt->LoopbackMdl)
        {
            devExt->LoopbackBuffer = (PUCHAR)MmMapLockedPagesSpecifyCache(devExt->LoopbackMdl, KernelMode, MmCached, nullptr, FALSE, NormalPagePriority);
            if (devExt->LoopbackBuffer) RtlZeroMemory(devExt->LoopbackBuffer, devExt->LoopbackSize);
        }
    }

    if (!devExt->SharedParamsMdl)
    {
        PHYSICAL_ADDRESS low = {0}, high = {0}, skip = {0};
        high.LowPart = 0xFFFFFFFF;
        devExt->SharedParamsMdl = MmAllocatePagesForMdlEx(low, high, skip, sizeof(LeylineSharedParameters), MmCached, MM_ALLOCATE_FULLY_REQUIRED);
        if (devExt->SharedParamsMdl)
        {
            devExt->SharedParams = (LeylineSharedParameters*)MmMapLockedPagesSpecifyCache(devExt->SharedParamsMdl, KernelMode, MmCached, nullptr, FALSE, NormalPagePriority);
            if (devExt->SharedParams)
            {
                RtlZeroMemory(devExt->SharedParams, sizeof(LeylineSharedParameters));
                devExt->SharedParams->BufferSize = (ULONG)devExt->LoopbackSize;
                devExt->SharedParams->ByteRate   = 48000 * 4;
                LARGE_INTEGER freq;
                KeQueryPerformanceCounter(&freq);
                devExt->SharedParams->QpcFrequency = freq.QuadPart;
            }
        }
    }

    PPORT renderPort = nullptr;
    PPORT capturePort = nullptr;
    PPORT renderTopoPort = nullptr;
    PPORT captureTopoPort = nullptr;

    // ---- WaveRender ----
    status = PcNewPort(&renderPort, CLSID_PortWaveRT);
    if (NT_SUCCESS(status))
    {
        CMiniportWaveRT *renderMiniport = new (NonPagedPool, 'LLWR') CMiniportWaveRT(nullptr, FALSE, devExt);
        if (renderMiniport)
        {
            renderMiniport->AddRef();
            devExt->RenderMiniport = renderMiniport;
            status = renderPort->Init(DeviceObject, Irp, renderMiniport, nullptr, ResourceList);
            if (NT_SUCCESS(status))
            {
                status = PcRegisterSubdevice(DeviceObject, L"WaveRender", renderPort);
            }
            renderMiniport->Release();
        }
        else status = STATUS_INSUFFICIENT_RESOURCES;
    }
    if (!NT_SUCCESS(status)) goto Cleanup;

    // ---- WaveCapture ----
    status = PcNewPort(&capturePort, CLSID_PortWaveRT);
    if (NT_SUCCESS(status))
    {
        CMiniportWaveRT *captureMiniport = new (NonPagedPool, 'LLWC') CMiniportWaveRT(nullptr, TRUE, devExt);
        if (captureMiniport)
        {
            captureMiniport->AddRef();
            devExt->CaptureMiniport = captureMiniport;
            status = capturePort->Init(DeviceObject, Irp, captureMiniport, nullptr, ResourceList);
            if (NT_SUCCESS(status))
            {
                status = PcRegisterSubdevice(DeviceObject, L"WaveCapture", capturePort);
            }
            captureMiniport->Release();
        }
        else status = STATUS_INSUFFICIENT_RESOURCES;
    }
    if (!NT_SUCCESS(status)) goto Cleanup;

    // ---- TopologyRender ----
    status = PcNewPort(&renderTopoPort, CLSID_PortTopology);
    if (NT_SUCCESS(status))
    {
        CMiniportTopology *renderTopoMiniport = new (NonPagedPool, 'LLTR') CMiniportTopology(nullptr, FALSE);
        if (renderTopoMiniport)
        {
            renderTopoMiniport->AddRef();
            devExt->RenderTopoMiniport = renderTopoMiniport;
            status = renderTopoPort->Init(DeviceObject, Irp, renderTopoMiniport, nullptr, ResourceList);
            if (NT_SUCCESS(status))
            {
                status = PcRegisterSubdevice(DeviceObject, L"TopologyRender", renderTopoPort);
            }
            renderTopoMiniport->Release();
        }
        else status = STATUS_INSUFFICIENT_RESOURCES;
    }
    if (!NT_SUCCESS(status)) goto Cleanup;

    // ---- TopologyCapture ----
    status = PcNewPort(&captureTopoPort, CLSID_PortTopology);
    if (NT_SUCCESS(status))
    {
        CMiniportTopology *captureTopoMiniport = new (NonPagedPool, 'LLTC') CMiniportTopology(nullptr, TRUE);
        if (captureTopoMiniport)
        {
            captureTopoMiniport->AddRef();
            devExt->CaptureTopoMiniport = captureTopoMiniport;
            status = captureTopoPort->Init(DeviceObject, Irp, captureTopoMiniport, nullptr, ResourceList);
            if (NT_SUCCESS(status))
            {
                status = PcRegisterSubdevice(DeviceObject, L"TopologyCapture", captureTopoPort);
            }
            captureTopoMiniport->Release();
        }
        else status = STATUS_INSUFFICIENT_RESOURCES;
    }
    if (!NT_SUCCESS(status)) goto Cleanup;

    // Physical connections
    PUNKNOWN renderPortUnk = nullptr;
    status = renderPort->QueryInterface(IID_IUnknown, (PVOID*)&renderPortUnk);
    if (NT_SUCCESS(status))
    {
        PUNKNOWN renderTopoPortUnk = nullptr;
        status = renderTopoPort->QueryInterface(IID_IUnknown, (PVOID*)&renderTopoPortUnk);
        if (NT_SUCCESS(status))
        {
            PcRegisterPhysicalConnection(DeviceObject, renderPortUnk, 1, renderTopoPortUnk, 0);
            renderTopoPortUnk->Release();
        }
        renderPortUnk->Release();
    }

    PUNKNOWN capturePortUnk = nullptr;
    status = capturePort->QueryInterface(IID_IUnknown, (PVOID*)&capturePortUnk);
    if (NT_SUCCESS(status))
    {
        PUNKNOWN captureTopoPortUnk = nullptr;
        status = captureTopoPort->QueryInterface(IID_IUnknown, (PVOID*)&captureTopoPortUnk);
        if (NT_SUCCESS(status))
        {
            PcRegisterPhysicalConnection(DeviceObject, captureTopoPortUnk, 1, capturePortUnk, 1);
            captureTopoPortUnk->Release();
        }
        capturePortUnk->Release();
    }

    // CDO Creation
    g_FunctionalDeviceObject = DeviceObject;
    if (!g_ControlDeviceObject)
    {
        UNICODE_STRING deviceName;
        RtlInitUnicodeString(&deviceName, L"\\Device\\LeylineAudio");
        status = IoCreateDevice(DeviceObject->DriverObject, sizeof(PVOID), &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &g_ControlDeviceObject);
        if (NT_SUCCESS(status))
        {
            UNICODE_STRING linkName;
            RtlInitUnicodeString(&linkName, L"\\DosDevices\\LeylineAudio");
            IoCreateSymbolicLink(&linkName, &deviceName);

            // Hook dispatch routines
            s_OriginalDispatchCreate  = DeviceObject->DriverObject->MajorFunction[IRP_MJ_CREATE];
            s_OriginalDispatchClose   = DeviceObject->DriverObject->MajorFunction[IRP_MJ_CLOSE];
            s_OriginalDispatchControl = DeviceObject->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL];

            DeviceObject->DriverObject->MajorFunction[IRP_MJ_CREATE]         = DispatchCreate;
            DeviceObject->DriverObject->MajorFunction[IRP_MJ_CLOSE]          = DispatchClose;
            DeviceObject->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
        }
    }

Cleanup:
    if (renderPort) renderPort->Release();
    if (capturePort) capturePort->Release();
    if (renderTopoPort) renderTopoPort->Release();
    if (captureTopoPort) captureTopoPort->Release();

    return status;
}

extern "C" NTSTATUS NTAPI AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT PhysicalDeviceObject)
{
    ULONG extensionSize = (ULONG)(PORT_CLASS_DEVICE_EXTENSION_SIZE + sizeof(DeviceExtension));
    return PcAddAdapterDevice(DriverObject, PhysicalDeviceObject, StartDevice, 10, extensionSize);
}
