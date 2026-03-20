// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LEYLINE MINIPORT DECLARATIONS
// Main interface for WaveRT and Topology miniport implementations.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include "leyline_common.h"
#include "leyline_guids.h"
#include "leyline_descriptors.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DEVICE EXTENSION
// Appended past the PortCls-reserved region of DeviceExtension.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class CMiniportWaveRT;
class CMiniportWaveRTStream;
class CMiniportTopology;

struct DeviceExtension
{
    PDEVICE_OBJECT  ControlDeviceObject;
    LeylineSharedParameters* SharedParams;
    PMDL            SharedParamsMdl;
    PVOID           SharedParamsUserMapping;
    PMDL            LoopbackMdl;
    PUCHAR          LoopbackBuffer;
    SIZE_T          LoopbackSize;
    PVOID           UserMapping;
    CMiniportWaveRT* RenderMiniport;
    CMiniportWaveRT* CaptureMiniport;
    CMiniportTopology* RenderTopoMiniport;
    CMiniportTopology* CaptureTopoMiniport;

    // Loopback engine
    KSPIN_LOCK          StreamLock;
    LIST_ENTRY          RenderStreams;
    LIST_ENTRY          CaptureStreams;
    KTIMER              LoopbackTimer;
    KDPC                LoopbackDpc;
    BOOLEAN             TimerRunning;
    ULONGLONG           LastCopiedByte;
    ULONG               GlitchCount;

    // Volume / Mute (shared between property handlers and DPC)
    LONG                VolumeLevel;      // 1/65536 dB, range [-96*0x10000, 0]
    LONG                MuteState;        // 0 = unmuted, nonzero = muted
    ULONG               GainLinear16;     // Precomputed 16.16 fixed-point linear gain
};

// The PortCls reference driver reserves this many pointer-sized slots
// in the DeviceExtension before our own fields begin.
static const SIZE_T LEYLINE_PORT_CLASS_DEVICE_EXTENSION_SIZE = 64 * sizeof(PVOID);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WAVE RT STREAM
// Manages a single audio stream (render or capture).
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class CMiniportWaveRTStream : public IMiniportWaveRTStream,
                              public IMiniportWaveRTStreamNotification,
                              public CUnknown
{
public:
    DECLARE_STD_UNKNOWN();

    CMiniportWaveRTStream(PUNKNOWN OuterUnknown, DeviceExtension* DevExt);
    virtual ~CMiniportWaveRTStream();

    // IMiniportWaveRTStream
    STDMETHODIMP SetFormat(PKSDATAFORMAT DataFormat) override;
    STDMETHODIMP SetState(KSSTATE State) override;
    STDMETHODIMP GetPosition(PKSAUDIO_POSITION Position) override;
    STDMETHODIMP AllocateAudioBuffer(ULONG RequestedSize, PMDL* AudioBufferMdl,
                                     ULONG* ActualSize, ULONG* OffsetFromFirstPage,
                                     MEMORY_CACHING_TYPE* CacheType) override;
    STDMETHODIMP_(void) FreeAudioBuffer(PMDL AudioBufferMdl, ULONG BufferSize) override;
    STDMETHODIMP_(void) GetHWLatency(KSRTAUDIO_HWLATENCY* Latency) override;
    STDMETHODIMP GetPositionRegister(KSRTAUDIO_HWREGISTER* Register) override;
    STDMETHODIMP GetClockRegister(KSRTAUDIO_HWREGISTER* Register) override;

    // IMiniportWaveRTStreamNotification
    STDMETHODIMP AllocateBufferWithNotification(ULONG NotificationCount,
                                                ULONG RequestedSize, PMDL* AudioBufferMdl,
                                                ULONG* ActualSize, ULONG* OffsetFromFirstPage,
                                                MEMORY_CACHING_TYPE* CacheType) override;
    STDMETHODIMP_(void) FreeBufferWithNotification(PMDL AudioBufferMdl, ULONG BufferSize) override;
    STDMETHODIMP RegisterNotificationEvent(PKEVENT NotificationEvent) override;
    STDMETHODIMP UnregisterNotificationEvent(PKEVENT NotificationEvent) override;

    // Initialization helper.
    NTSTATUS Init(ULONG PinId, BOOLEAN Capture, PKSDATAFORMAT Format);

    // Event signaling helper
    void CheckAndSignalEvents(ULONGLONG lastPosBytes, ULONGLONG currentPosBytes);

    // Public accessors for the loopback engine.
    PUCHAR   GetBufferBase()     const { return m_Buffer.GetBaseAddress(); }
    SIZE_T   GetBufferSize()     const { return m_Buffer.GetSize(); }
    BOOLEAN  IsStreamCapture()   const { return m_IsCapture; }
    KSSTATE  GetStreamState()    const { return m_State; }
    ULONG    GetStreamByteRate() const { return m_ByteRate; }
    LONGLONG GetStartTime()      const { return m_StartTime; }
    LONGLONG GetFrequency()      const { return m_Frequency; }
    ULONG    GetBitsPerSample()  const { return m_BitsPerSample; }
    ULONG    GetChannels()       const { return m_Channels; }
    BOOLEAN  IsFloat()           const { return m_IsFloat; }

private:
    RingBuffer         m_Buffer;
    KSSTATE            m_State;
    PMDL               m_Mdl;
    PVOID              m_Mapping;
    BOOLEAN            m_IsCapture;
    BOOLEAN            m_OwnsMdl;
    LONGLONG           m_StartTime;
    ULONG              m_ByteRate;
    LONGLONG           m_Frequency;
    DeviceExtension*   m_DevExt;
    ULONG              m_BitsPerSample;
    ULONG              m_Channels;
    BOOLEAN            m_IsFloat;

    // Notification events
    PKEVENT            m_NotificationEvents[8];
    ULONG              m_NotificationBytes;

    LIST_ENTRY         m_ListEntry;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WAVE RT MINIPORT
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class CMiniportWaveRT : public IMiniportWaveRT,
                        public CUnknown
{
public:
    DECLARE_STD_UNKNOWN();

    CMiniportWaveRT(PUNKNOWN OuterUnknown, BOOLEAN IsCapture, DeviceExtension* DevExt);
    virtual ~CMiniportWaveRT();

    // IMiniport
    STDMETHODIMP GetDescription(PPCFILTER_DESCRIPTOR* Description) override;
    STDMETHODIMP DataRangeIntersection(ULONG PinId, PKSDATARANGE DataRange,
                                       PKSDATARANGE MatchingDataRange,
                                       ULONG DataFormatSize, PVOID DataFormat,
                                       PULONG ResultantFormatSize) override;

    // IMiniportWaveRT
    STDMETHODIMP Init(PUNKNOWN UnknownAdapter, PRESOURCELIST ResourceList,
                      IPortWaveRT* Port) override;
    STDMETHODIMP GetDeviceDescription(PDEVICE_DESCRIPTION DeviceDescription) override;
    STDMETHODIMP NewStream(PMINIPORTWAVERTSTREAM* Stream,
                           PPORTWAVERTSTREAM PortStream,
                           ULONG PinId, BOOLEAN Capture,
                           PKSDATAFORMAT DataFormat) override;

private:
    BOOLEAN          m_IsCapture;
    BOOLEAN          m_IsInitialized;
    DeviceExtension* m_DevExt;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TOPOLOGY MINIPORT
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class CMiniportTopology : public IMiniportTopology,
                          public CUnknown
{
public:
    DECLARE_STD_UNKNOWN();

    CMiniportTopology(PUNKNOWN OuterUnknown, BOOLEAN IsCapture, DeviceExtension* DevExt);
    virtual ~CMiniportTopology();
    DeviceExtension* GetDevExt() const { return m_DevExt; }

    // IMiniport
    STDMETHODIMP GetDescription(PPCFILTER_DESCRIPTOR* Description) override;
    STDMETHODIMP DataRangeIntersection(ULONG PinId, PKSDATARANGE DataRange,
                                       PKSDATARANGE MatchingDataRange,
                                       ULONG DataFormatSize, PVOID DataFormat,
                                       PULONG ResultantFormatSize) override;

    // IMiniportTopology
    STDMETHODIMP Init(PUNKNOWN UnknownAdapter, PRESOURCELIST ResourceList,
                      IPortTopology* Port) override;

private:
    BOOLEAN          m_IsCapture;
    BOOLEAN          m_IsInitialized;
    PVOID            m_Port;
    DeviceExtension* m_DevExt;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HELPER: Recover DeviceExtension from a PortCls device object.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

inline DeviceExtension* GetDeviceExtension(PDEVICE_OBJECT DeviceObject)
{
    PUCHAR base = reinterpret_cast<PUCHAR>(DeviceObject->DeviceExtension);
    return reinterpret_cast<DeviceExtension*>(base + LEYLINE_PORT_CLASS_DEVICE_EXTENSION_SIZE);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LOOPBACK DPC
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern "C" void LoopbackDpcRoutine(PKDPC Dpc, PVOID DeferredContext,
                                   PVOID SystemArgument1, PVOID SystemArgument2);
