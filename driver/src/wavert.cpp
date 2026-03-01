// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WAVERT MINIPORT & STREAM IMPLEMENTATION
// Handles audio format negotiation, streaming, and buffer lifecycle.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "leyline_miniport.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CMiniportWaveRTStream
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CMiniportWaveRTStream::CMiniportWaveRTStream(PUNKNOWN OuterUnknown, DeviceExtension* DevExt)
    : CUnknown(OuterUnknown)
    , m_State(KSSTATE_STOP)
    , m_Mdl(nullptr)
    , m_Mapping(nullptr)
    , m_IsCapture(FALSE)
    , m_OwnsMdl(FALSE)
    , m_StartTime(0)
    , m_ByteRate(48000 * 4)
    , m_Frequency(0)
    , m_DevExt(DevExt)
{
    LARGE_INTEGER freq = {};
    KeQueryPerformanceCounter(&freq);
    m_Frequency = freq.QuadPart;
}

CMiniportWaveRTStream::~CMiniportWaveRTStream()
{
    if (m_OwnsMdl && m_Mdl)
    {
        if (m_Mapping)
            MmUnmapLockedPages(m_Mapping, m_Mdl);
        MmFreePagesFromMdl(m_Mdl);
        IoFreeMdl(m_Mdl);
        m_Mdl     = nullptr;
        m_Mapping = nullptr;
    }
}

NTSTATUS CMiniportWaveRTStream::Init(ULONG /*PinId*/, BOOLEAN Capture, PKSDATAFORMAT Format)
{
    m_IsCapture = Capture;
    if (Format)
    {
        auto *wfx = reinterpret_cast<KSDATAFORMAT*>(Format);
        auto *wave = reinterpret_cast<WAVEFORMATEX*>(wfx + 1);
        m_ByteRate = wave->nAvgBytesPerSec;
    }
    DbgPrint("LeylineWaveRT: Stream Init (capture=%d, byteRate=%u)\n", (int)m_IsCapture, m_ByteRate);
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::NonDelegatingQueryInterface(REFIID riid, PVOID* ppvObject)
{
    if (IsEqualGUID(riid, IID_IMiniportWaveRTStream) || IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObject = reinterpret_cast<PVOID>(static_cast<IMiniportWaveRTStream*>(this));
        AddRef();
        return STATUS_SUCCESS;
    }
    return STATUS_NOINTERFACE;
}

STDMETHODIMP CMiniportWaveRTStream::SetFormat(PKSDATAFORMAT /*DataFormat*/)
{
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::SetState(KSSTATE State)
{
    m_State = State;
    if (State == KSSTATE_STOP)
        m_StartTime = 0;
    else if (State == KSSTATE_RUN)
        m_StartTime = KeQueryPerformanceCounter(nullptr).QuadPart;
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::GetPosition(PKSAUDIO_POSITION Position)
{
    if (!Position) return STATUS_INVALID_PARAMETER;

    if (m_State != KSSTATE_RUN || m_StartTime == 0)
    {
        Position->PlayOffset = 0;
        Position->WriteOffset = 0;
        return STATUS_SUCCESS;
    }

    LONGLONG now     = KeQueryPerformanceCounter(nullptr).QuadPart;
    LONGLONG elapsed = now - m_StartTime;
    ULONGLONG bytes  = WaveRTMath::TicksToBytes(elapsed, m_ByteRate, m_Frequency);

    SIZE_T size = m_Buffer.GetSize();
    ULONGLONG pos = (size > 0) ? (bytes % (ULONGLONG)size) : 0;
    
    Position->PlayOffset = pos;
    Position->WriteOffset = pos;

    if (m_DevExt && m_DevExt->SharedParams)
    {
        if (!m_IsCapture) m_DevExt->SharedParams->WritePos = (ULONG)pos;
        else m_DevExt->SharedParams->ReadPos = (ULONG)pos;
    }

    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRTStream::AllocateAudioBuffer(
    ULONG RequestedSize, PMDL* AudioBufferMdl,
    ULONG* ActualSize, ULONG* OffsetFromFirstPage,
    MEMORY_CACHING_TYPE* CacheType)
{
    if (m_Mdl) return STATUS_ALREADY_COMMITTED;

    PHYSICAL_ADDRESS low  = { 0 }, high = { 0 }, skip = { 0 };
    high.LowPart = 0xFFFFFFFF;

    PMDL mdl = MmAllocatePagesForMdlEx(low, high, skip, RequestedSize, MmCached, MM_ALLOCATE_FULLY_REQUIRED);
    if (!mdl)
    {
        if (m_DevExt && m_DevExt->LoopbackMdl)
        {
            m_Mdl     = m_DevExt->LoopbackMdl;
            m_Mapping = m_DevExt->LoopbackBuffer;
            m_Buffer.Init(m_DevExt->LoopbackBuffer, m_DevExt->LoopbackSize);
            m_OwnsMdl = FALSE;
            if (AudioBufferMdl)     *AudioBufferMdl     = m_Mdl;
            if (ActualSize)         *ActualSize         = (ULONG)m_DevExt->LoopbackSize;
            if (OffsetFromFirstPage) *OffsetFromFirstPage = 0;
            if (CacheType)          *CacheType          = MmCached;
            return STATUS_SUCCESS;
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_Mapping = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, nullptr, FALSE, NormalPagePriority);
    if (!m_Mapping)
    {
        IoFreeMdl(mdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_Mdl     = mdl;
    m_OwnsMdl = TRUE;
    m_Buffer.Init(reinterpret_cast<PUCHAR>(m_Mapping), RequestedSize);

    if (AudioBufferMdl)      *AudioBufferMdl      = m_Mdl;
    if (ActualSize)          *ActualSize          = RequestedSize;
    if (OffsetFromFirstPage) *OffsetFromFirstPage = 0;
    if (CacheType)           *CacheType           = MmCached;
    return STATUS_SUCCESS;
}

STDMETHODIMP_(void) CMiniportWaveRTStream::FreeAudioBuffer(PMDL /*AudioBufferMdl*/, ULONG /*BufferSize*/)
{
    return;
}

STDMETHODIMP_(void) CMiniportWaveRTStream::GetHWLatency(KSRTAUDIO_HWLATENCY* Latency)
{
    if (Latency)
    {
        Latency->FifoSize     = 0;
        Latency->ChipsetDelay = 0;
        Latency->CodecDelay   = 0;
    }
}

STDMETHODIMP CMiniportWaveRTStream::GetPositionRegister(KSRTAUDIO_HWREGISTER* /*Register*/)
{
    return STATUS_UNSUCCESSFUL;
}

STDMETHODIMP CMiniportWaveRTStream::GetClockRegister(KSRTAUDIO_HWREGISTER* /*Register*/)
{
    return STATUS_UNSUCCESSFUL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CMiniportWaveRT
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CMiniportWaveRT::CMiniportWaveRT(PUNKNOWN OuterUnknown, BOOLEAN IsCapture, DeviceExtension* DevExt)
    : CUnknown(OuterUnknown)
    , m_IsCapture(IsCapture)
    , m_IsInitialized(FALSE)
    , m_DevExt(DevExt)
{}

CMiniportWaveRT::~CMiniportWaveRT() {}

STDMETHODIMP CMiniportWaveRT::NonDelegatingQueryInterface(REFIID riid, PVOID* ppvObject)
{
    if (IsEqualGUID(riid, IID_IMiniportWaveRT) || IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, IID_IMiniport))
    {
        *ppvObject = reinterpret_cast<PVOID>(static_cast<IMiniportWaveRT*>(this));
        AddRef();
        return STATUS_SUCCESS;
    }
    *ppvObject = nullptr;
    return STATUS_NOINTERFACE;
}

STDMETHODIMP CMiniportWaveRT::GetDescription(PPCFILTER_DESCRIPTOR* Description)
{
    if (!Description) return STATUS_INVALID_PARAMETER;
    *Description = const_cast<PPCFILTER_DESCRIPTOR>(m_IsCapture ? &g_WaveCaptureFilterDescriptor : &g_WaveRenderFilterDescriptor);
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRT::DataRangeIntersection(
    ULONG PinId, PKSDATARANGE DataRange, PKSDATARANGE /*MatchingDataRange*/,
    ULONG DataFormatSize, PVOID DataFormat, PULONG ResultantFormatSize)
{
    UNREFERENCED_PARAMETER(PinId);
    if (!DataRange) return STATUS_INVALID_PARAMETER;

    if (!IsEqualGUID(DataRange->MajorFormat, KSDATAFORMAT_TYPE_AUDIO))
        return STATUS_NO_MATCH;

    BOOLEAN isEx      = !!IsEqualGUID(DataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX);
    BOOLEAN isExt     = !!IsEqualGUID(DataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEXTENSIBLE);
    if (!isEx && !isExt) return STATUS_NO_MATCH;

    BOOLEAN isPCM   = !!IsEqualGUID(DataRange->SubFormat, KSDATAFORMAT_SUBTYPE_PCM);
    BOOLEAN isFloat = !!IsEqualGUID(DataRange->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
    if (!isPCM && !isFloat) return STATUS_NO_MATCH;

    ULONG fmtSize = isExt ? sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE) : sizeof(KSDATAFORMAT_WAVEFORMATEX);

    if (DataFormatSize == 0) { if (ResultantFormatSize) *ResultantFormatSize = fmtSize; return STATUS_BUFFER_TOO_SMALL; }
    if (DataFormatSize < fmtSize) return STATUS_BUFFER_TOO_SMALL;

    if (isExt)
    {
        auto *r = reinterpret_cast<KSDATAFORMAT_WAVEFORMATEXTENSIBLE*>(DataFormat);
        r->DataFormat.FormatSize  = fmtSize;
        r->DataFormat.Flags       = 0;
        r->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
        r->DataFormat.SubFormat   = DataRange->SubFormat;
        r->DataFormat.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEXTENSIBLE;
        r->WaveFormatExt.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
        r->WaveFormatExt.Format.nChannels       = 2;
        r->WaveFormatExt.Format.nSamplesPerSec  = 48000;
        r->WaveFormatExt.Format.wBitsPerSample  = isPCM ? 16 : 32;
        r->WaveFormatExt.Format.nBlockAlign     = r->WaveFormatExt.Format.nChannels * r->WaveFormatExt.Format.wBitsPerSample / 8;
        r->WaveFormatExt.Format.nAvgBytesPerSec = r->WaveFormatExt.Format.nSamplesPerSec * r->WaveFormatExt.Format.nBlockAlign;
        r->WaveFormatExt.Format.cbSize          = 22;
        r->WaveFormatExt.Samples.wValidBitsPerSample = r->WaveFormatExt.Format.wBitsPerSample;
        r->WaveFormatExt.dwChannelMask          = KSAUDIO_SPEAKER_STEREO;
        r->WaveFormatExt.SubFormat              = DataRange->SubFormat;
        r->DataFormat.SampleSize                = r->WaveFormatExt.Format.nBlockAlign;
    }
    else
    {
        auto *r = reinterpret_cast<KSDATAFORMAT_WAVEFORMATEX*>(DataFormat);
        r->DataFormat.FormatSize  = fmtSize;
        r->DataFormat.Flags       = 0;
        r->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
        r->DataFormat.SubFormat   = DataRange->SubFormat;
        r->DataFormat.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
        r->WaveFormatEx.wFormatTag      = isPCM ? WAVE_FORMAT_PCM : WAVE_FORMAT_IEEE_FLOAT;
        r->WaveFormatEx.nChannels       = 2;
        r->WaveFormatEx.nSamplesPerSec  = 48000;
        r->WaveFormatEx.wBitsPerSample  = isPCM ? 16 : 32;
        r->WaveFormatEx.nBlockAlign     = r->WaveFormatEx.nChannels * r->WaveFormatEx.wBitsPerSample / 8;
        r->WaveFormatEx.nAvgBytesPerSec = r->WaveFormatEx.nSamplesPerSec * r->WaveFormatEx.nBlockAlign;
        r->WaveFormatEx.cbSize          = 0;
        r->DataFormat.SampleSize        = r->WaveFormatEx.nBlockAlign;
    }

    if (ResultantFormatSize) *ResultantFormatSize = fmtSize;
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRT::Init(PUNKNOWN /*UnknownAdapter*/, PRESOURCELIST /*ResourceList*/, IPortWaveRT* /*Port*/)
{
    m_IsInitialized = TRUE;
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRT::GetDeviceDescription(PDEVICE_DESCRIPTION /*DeviceDescription*/)
{
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportWaveRT::NewStream(
    PMINIPORTWAVERTSTREAM* Stream, PPORTWAVERTSTREAM /*PortStream*/,
    ULONG PinId, BOOLEAN Capture, PKSDATAFORMAT DataFormat)
{
    if (!Stream) return STATUS_INVALID_PARAMETER;
    if (!m_IsInitialized) return STATUS_DEVICE_NOT_READY;

    CMiniportWaveRTStream *stream = new (NonPagedPool, 'LLWS') CMiniportWaveRTStream(nullptr, m_DevExt);
    if (!stream) return STATUS_INSUFFICIENT_RESOURCES;

    NTSTATUS status = stream->Init(PinId, Capture, DataFormat);
    if (!NT_SUCCESS(status)) { delete stream; return status; }

    stream->AddRef();
    *Stream = stream;
    return STATUS_SUCCESS;
}
