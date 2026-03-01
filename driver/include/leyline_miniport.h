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
};

// The PortCls reference driver reserves this many pointer-sized slots
// in the DeviceExtension before our own fields begin.
static const SIZE_T LEYLINE_PORT_CLASS_DEVICE_EXTENSION_SIZE = 64 * sizeof(PVOID);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WAVE RT STREAM
// Manages a single audio stream (render or capture).
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class CMiniportWaveRTStream : public IMiniportWaveRTStream,
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

    // Initialization helper.
    NTSTATUS Init(ULONG PinId, BOOLEAN Capture, PKSDATAFORMAT Format);

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

    CMiniportTopology(PUNKNOWN OuterUnknown, BOOLEAN IsCapture);
    virtual ~CMiniportTopology();

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
    BOOLEAN  m_IsCapture;
    BOOLEAN  m_IsInitialized;
    PVOID    m_Port;
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HELPER: Recover DeviceExtension from a PortCls device object.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

inline DeviceExtension* GetDeviceExtension(PDEVICE_OBJECT DeviceObject)
{
    PUCHAR base = reinterpret_cast<PUCHAR>(DeviceObject->DeviceExtension);
    return reinterpret_cast<DeviceExtension*>(base + LEYLINE_PORT_CLASS_DEVICE_EXTENSION_SIZE);
}
