// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LEYLINE CLIENT LIBRARY IMPLEMENTATION
// Connects to the WDM Control Device Object via DeviceIoControl.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "leyline_client.h"

// Bring in the IOCTL definitions explicitly bridging the kernel boundary
#define FILE_DEVICE_LEYLINE     FILE_DEVICE_UNKNOWN
#define LEYLINE_IOCTL_BASE      0x800

#define IOCTL_LEYLINE_MAP_BUFFER \
    CTL_CODE(FILE_DEVICE_LEYLINE, LEYLINE_IOCTL_BASE + 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_LEYLINE_MAP_PARAMS \
    CTL_CODE(FILE_DEVICE_LEYLINE, LEYLINE_IOCTL_BASE + 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

namespace Leyline
{
    AudioDriverClient::AudioDriverClient()
        : m_DeviceHandle(INVALID_HANDLE_VALUE)
        , m_MappedVirtualBuffer(nullptr)
        , m_MappedParams(nullptr)
    {
        // Require symbolic link \\.\LeylineAudio to be exposed by the driver
        m_DeviceHandle = CreateFileW(
            L"\\\\.\\LeylineAudio",
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,         // Default security
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
    }

    AudioDriverClient::~AudioDriverClient()
    {
        // Releasing the file handle triggers IRP_MJ_CLEANUP in the driver,
        // which could be configured to tear down process-specific MDL mappings.
        // Currently, MmMapLockedPages outlives the handle unless explicitly freed,
        // but modern OS behavior generally sweeps upon process exit.
        if (m_DeviceHandle != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_DeviceHandle);
            m_DeviceHandle = INVALID_HANDLE_VALUE;
        }
    }

    bool AudioDriverClient::IsConnected() const
    {
        return m_DeviceHandle != INVALID_HANDLE_VALUE;
    }

    void* AudioDriverClient::MapLoopbackBuffer(size_t& outSize)
    {
        if (!IsConnected()) return nullptr;
        if (m_MappedVirtualBuffer) return m_MappedVirtualBuffer;

        void* kernelMappedPtr = nullptr;
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            m_DeviceHandle,
            IOCTL_LEYLINE_MAP_BUFFER,
            nullptr, 0,
            &kernelMappedPtr, sizeof(kernelMappedPtr),
            &bytesReturned,
            nullptr);

        if (success && bytesReturned == sizeof(void*))
        {
            m_MappedVirtualBuffer = kernelMappedPtr;
            outSize = 128 * 1024; // Hardcoded driver loopback allocation size
        }
        else
        {
            outSize = 0;
        }

        return m_MappedVirtualBuffer;
    }

    LeylineSharedParameters* AudioDriverClient::MapSharedParameters()
    {
        if (!IsConnected()) return nullptr;
        if (m_MappedParams) return m_MappedParams;

        void* kernelMappedPtr = nullptr;
        DWORD bytesReturned = 0;

        BOOL success = DeviceIoControl(
            m_DeviceHandle,
            IOCTL_LEYLINE_MAP_PARAMS,
            nullptr, 0,
            &kernelMappedPtr, sizeof(kernelMappedPtr),
            &bytesReturned,
            nullptr);

        if (success && bytesReturned == sizeof(void*))
        {
            // Cast the raw mapped pointer to our structured telemetry block
            m_MappedParams = reinterpret_cast<LeylineSharedParameters*>(kernelMappedPtr);
        }

        return m_MappedParams;
    }
}
