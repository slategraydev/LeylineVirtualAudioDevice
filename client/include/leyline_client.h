// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LEYLINE CLIENT LIBRARY
// User-mode wrapper for reading/writing the driver's Control Device Object (CDO).
// Provide a safe, ergonomic C++ interface to map shared memory and read telemetry.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <windows.h>
#include <winioctl.h>
#include <memory>

// Forward declarations of kernel-shared structures
struct LeylineSharedParameters;

namespace Leyline
{
    // High-level wrapper for traversing to the Leyline driver.
    class AudioDriverClient
    {
    public:
        // Attempt to open a handle to the kernel-mode CDO.
        AudioDriverClient();
        ~AudioDriverClient();

        // Check if the link to the driver is established.
        bool IsConnected() const;

        // Map the driver's raw loopback PCM buffer into user-mode space.
        // Returns the size of the buffer via outSize.
        void* MapLoopbackBuffer(size_t& outSize);

        // Map the driver's high-frequency telemetry struct into user-mode space.
        // (Gain amounts, exact sample positions, QPC clocks, peak values).
        LeylineSharedParameters* MapSharedParameters();

    private:
        HANDLE m_DeviceHandle;
        
        // Pointers provided directly by the Windows Kernel Memory Manager
        // These are mapped into the calling process's virtual address space.
        void* m_MappedVirtualBuffer;
        LeylineSharedParameters* m_MappedParams;

        // Deny copies since this holds system resources
        AudioDriverClient(const AudioDriverClient&) = delete;
        AudioDriverClient& operator=(const AudioDriverClient&) = delete;
    };
}
