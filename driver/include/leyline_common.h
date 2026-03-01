// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LEYLINE COMMON DEFINITIONS
// Shared types, GUIDs, constants, and IOCTLs for the Leyline audio driver suite.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include <portcls.h>
#include <stdunk.h>
#include <stdarg.h>
#include <intrin.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// IOCTL DEFINITIONS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define FILE_DEVICE_LEYLINE     FILE_DEVICE_UNKNOWN
#define LEYLINE_IOCTL_BASE      0x800

#define IOCTL_LEYLINE_GET_STATUS \
    CTL_CODE(FILE_DEVICE_LEYLINE, LEYLINE_IOCTL_BASE + 1, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_LEYLINE_MAP_BUFFER \
    CTL_CODE(FILE_DEVICE_LEYLINE, LEYLINE_IOCTL_BASE + 2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_LEYLINE_MAP_PARAMS \
    CTL_CODE(FILE_DEVICE_LEYLINE, LEYLINE_IOCTL_BASE + 3, METHOD_BUFFERED, FILE_ANY_ACCESS)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// SHARED PARAMETER BLOCK
// Layout must be identical between kernel, APO, and HSA.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma pack(push, 1)
struct LeylineSharedParameters
{
    ULONG   MasterGainBits;     // IEEE 754 float bits for master gain
    ULONG   PeakLBits;          // IEEE 754 float bits for left peak
    ULONG   PeakRBits;          // IEEE 754 float bits for right peak
    LONGLONG QpcFrequency;
    LONGLONG RenderStartQpc;
    LONGLONG CaptureStartQpc;
    ULONG   BufferSize;
    ULONG   ByteRate;
    ULONG   WritePos;           // Current render position (byte offset)
    ULONG   ReadPos;            // Current capture position (byte offset)
};
#pragma pack(pop)

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// RING BUFFER
// A simple, lock-free ring buffer for audio samples.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class RingBuffer
{
public:
    RingBuffer() : m_Buffer(nullptr), m_Size(0), m_WritePos(0), m_ReadPos(0) {}

    void Init(PUCHAR buffer, SIZE_T size)
    {
        m_Buffer   = buffer;
        m_Size     = size;
        m_WritePos = 0;
        m_ReadPos  = 0;
    }

    PUCHAR GetBaseAddress() const { return m_Buffer; }
    SIZE_T GetSize()        const { return m_Size; }

    SIZE_T AvailableWrite() const
    {
        if (m_Size == 0) return 0;
        if (m_WritePos >= m_ReadPos)
            return m_Size - (m_WritePos - m_ReadPos) - 1;
        return m_ReadPos - m_WritePos - 1;
    }

    SIZE_T AvailableRead() const
    {
        if (m_Size == 0) return 0;
        if (m_WritePos >= m_ReadPos)
            return m_WritePos - m_ReadPos;
        return m_Size - (m_ReadPos - m_WritePos);
    }

    SIZE_T Write(const PUCHAR data, SIZE_T len)
    {
        SIZE_T available = AvailableWrite();
        SIZE_T toWrite   = min(len, available);
        if (toWrite == 0) return 0;

        SIZE_T firstPart = min(toWrite, m_Size - m_WritePos);
        RtlCopyMemory(m_Buffer + m_WritePos, data, firstPart);

        if (firstPart < toWrite)
            RtlCopyMemory(m_Buffer, data + firstPart, toWrite - firstPart);

        m_WritePos = (m_WritePos + toWrite) % m_Size;
        return toWrite;
    }

    SIZE_T Read(PUCHAR data, SIZE_T len)
    {
        SIZE_T available = AvailableRead();
        SIZE_T toRead    = min(len, available);
        if (toRead == 0) return 0;

        SIZE_T firstPart = min(toRead, m_Size - m_ReadPos);
        RtlCopyMemory(data, m_Buffer + m_ReadPos, firstPart);

        if (firstPart < toRead)
            RtlCopyMemory(data + firstPart, m_Buffer, toRead - firstPart);

        m_ReadPos = (m_ReadPos + toRead) % m_Size;
        return toRead;
    }

    void Reset() { m_WritePos = m_ReadPos = 0; }

private:
    PUCHAR  m_Buffer;
    SIZE_T  m_Size;
    SIZE_T  m_WritePos;
    SIZE_T  m_ReadPos;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AUDIO MATH UTILITIES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace WaveRTMath
{
    // Convert elapsed QPC ticks to an absolute byte offset.
    inline ULONGLONG TicksToBytes(LONGLONG elapsedTicks, ULONG byteRate, LONGLONG frequency)
    {
        if (frequency <= 0) return 0;
        // Standard 64-bit math is safe for >100 days of continuous playback at 192kHz/24bit.
        return (ULONGLONG)((elapsedTicks * (unsigned __int64)byteRate) / (unsigned __int64)frequency);
    }

    // Clamp a byte offset into a ring buffer.
    inline ULONGLONG CalculatePosition(LONGLONG elapsedTicks, ULONG byteRate, LONGLONG frequency, SIZE_T bufferSize)
    {
        ULONGLONG bytes = TicksToBytes(elapsedTicks, byteRate, frequency);
        if (bufferSize > 0) bytes %= (ULONGLONG)bufferSize;
        return bytes;
    }
}
