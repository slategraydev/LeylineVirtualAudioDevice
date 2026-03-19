// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// WAVERT MINIPORT & STREAM IMPLEMENTATION
// Handles audio format negotiation, streaming, and buffer lifecycle.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "leyline_miniport.h"

// Loopback timer period: 1ms relative interval in 100ns units (negative = relative).
static const LONGLONG LOOPBACK_PERIOD_100NS = -10000LL;
static const LONG     LOOPBACK_PERIOD_MS    = 1;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LOOPBACK DPC ROUTINE
// Copies audio from the active render buffer to the active capture buffer.
// Applies volume and mute during the copy using integer math.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern "C" void LoopbackDpcRoutine(PKDPC /*Dpc*/, PVOID DeferredContext,
                                   PVOID /*SystemArgument1*/, PVOID /*SystemArgument2*/)
{
    DeviceExtension* devExt = reinterpret_cast<DeviceExtension*>(DeferredContext);
    if (!devExt) return;

    KIRQL oldIrql;
    KeAcquireSpinLock(&devExt->StreamLock, &oldIrql);

    if (IsListEmpty(&devExt->RenderStreams) || IsListEmpty(&devExt->CaptureStreams))
    {
        KeReleaseSpinLock(&devExt->StreamLock, oldIrql);
        return;
    }

    // For now, kernel-mixing relies on the first render stream as the master clock/source
    CMiniportWaveRTStream* renderStream = CONTAINING_RECORD(devExt->RenderStreams.Flink, CMiniportWaveRTStream, m_ListEntry);

    if (renderStream->GetStreamState() != KSSTATE_RUN)
    {
        KeReleaseSpinLock(&devExt->StreamLock, oldIrql);
        return;
    }

    PUCHAR renderBase  = renderStream->GetBufferBase();
    SIZE_T renderSize  = renderStream->GetBufferSize();

    if (!renderBase || renderSize == 0)
    {
        KeReleaseSpinLock(&devExt->StreamLock, oldIrql);
        return;
    }

    LONGLONG now = KeQueryPerformanceCounter(nullptr).QuadPart;
    LONGLONG renderElapsed = now - renderStream->GetStartTime();
    ULONGLONG currentByte = WaveRTMath::TicksToBytes(
        renderElapsed, renderStream->GetStreamByteRate(), renderStream->GetFrequency());

    ULONGLONG lastByte = devExt->LastCopiedByte;
    if (currentByte <= lastByte)
    {
        KeReleaseSpinLock(&devExt->StreamLock, oldIrql);
        return;
    }

    renderStream->CheckAndSignalEvents(lastByte, currentByte);

    ULONGLONG bytesToCopy = currentByte - lastByte;

    // Distribute to all capture streams
    for (PLIST_ENTRY entry = devExt->CaptureStreams.Flink; entry != &devExt->CaptureStreams; entry = entry->Flink)
    {
        CMiniportWaveRTStream* captureStream = CONTAINING_RECORD(entry, CMiniportWaveRTStream, m_ListEntry);
        
        if (captureStream->GetStreamState() != KSSTATE_RUN) continue;

        PUCHAR captureBase = captureStream->GetBufferBase();
        SIZE_T captureSize = captureStream->GetBufferSize();

        if (!captureBase || captureSize == 0) continue;

        LONGLONG captureElapsed = now - captureStream->GetStartTime();
        ULONGLONG currentCapByte = WaveRTMath::TicksToBytes(
            captureElapsed, captureStream->GetStreamByteRate(), captureStream->GetFrequency());
        ULONGLONG lastCapByte = currentCapByte - bytesToCopy; // Tightly bound to render length

        captureStream->CheckAndSignalEvents(lastCapByte, currentCapByte);

        SIZE_T maxCopy = min(renderSize, captureSize);
        ULONGLONG toCopy = (bytesToCopy > (ULONGLONG)maxCopy) ? maxCopy : bytesToCopy;

        SIZE_T srcOff = (SIZE_T)(lastByte % renderSize);
        SIZE_T dstOff = (SIZE_T)(lastCapByte % captureSize);
        SIZE_T remaining = (SIZE_T)toCopy;

        BOOLEAN muted   = (devExt->MuteState != 0);
        ULONG gainFixed = devExt->GainLinear16;

    while (remaining > 0)
    {
        SIZE_T srcAvail = renderSize  - srcOff;
        SIZE_T dstAvail = captureSize - dstOff;
        SIZE_T chunk    = min(remaining, min(srcAvail, dstAvail));

        LONG peakL = 0;
        LONG peakR = 0;

        if (muted)
        {
            RtlZeroMemory(captureBase + dstOff, chunk);
        }
        else
        {
            // Copy then scale in-place on the capture buffer.
            RtlCopyMemory(captureBase + dstOff, renderBase + srcOff, chunk);

            // 16-bit PCM volume scaling and peak calculation.
            if (renderStream->GetBitsPerSample() == 16)
            {
                const SIZE_T bytesPerSample = 2;
                SIZE_T aligned = chunk - (chunk % bytesPerSample);
                for (SIZE_T s = 0; s < aligned; s += bytesPerSample)
                {
                    INT16* sample = reinterpret_cast<INT16*>(captureBase + dstOff + s);
                    
                    if (gainFixed != 0x10000)
                    {
                        INT32 scaled = ((INT32)*sample * (INT32)gainFixed) >> 16;
                        if (scaled >  32767) scaled =  32767;
                        if (scaled < -32768) scaled = -32768;
                        *sample = (INT16)scaled;
                    }

                    LONG absVal = (*sample >= 0) ? *sample : -*sample;
                    if ((s / bytesPerSample) % 2 == 0) // L
                    {
                        if (absVal > peakL) peakL = absVal;
                    }
                    else // R
                    {
                        if (absVal > peakR) peakR = absVal;
                    }
                }
            }
        }

        // Decay the old peak level over time slightly, and push the new instantaneous peak.
        // We use a small decay to keep the peak smoothed for the UI. (approx -5% per ms).
        LONG newPeakL = peakL * 65535;
        LONG newPeakR = peakR * 65535;
        
        LONG decayL = devExt->PeakLevel[0] - (devExt->PeakLevel[0] / 20);
        LONG decayR = devExt->PeakLevel[1] - (devExt->PeakLevel[1] / 20);

        devExt->PeakLevel[0] = max(decayL, newPeakL);
        devExt->PeakLevel[1] = max(decayR, newPeakR);

        srcOff    = (srcOff + chunk) % renderSize;
        dstOff    = (dstOff + chunk) % captureSize;
        remaining -= chunk;
    }

    } // End loop over capture streams

    devExt->LastCopiedByte = currentByte;
    KeReleaseSpinLock(&devExt->StreamLock, oldIrql);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HELPER: Register or unregister a stream with the loopback engine.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

static void RegisterStreamForLoopback(DeviceExtension* devExt, CMiniportWaveRTStream* stream, BOOLEAN capture)
{
    if (!devExt) return;

    KIRQL oldIrql;
    KeAcquireSpinLock(&devExt->StreamLock, &oldIrql);

    if (capture)
        InsertTailList(&devExt->CaptureStreams, &stream->m_ListEntry);
    else
        InsertTailList(&devExt->RenderStreams, &stream->m_ListEntry);

    // Start timer when both lists become populated
    if (!IsListEmpty(&devExt->RenderStreams) && !IsListEmpty(&devExt->CaptureStreams) && !devExt->TimerRunning)
    {
        CMiniportWaveRTStream* masterRender = CONTAINING_RECORD(devExt->RenderStreams.Flink, CMiniportWaveRTStream, m_ListEntry);

        LONGLONG now          = KeQueryPerformanceCounter(nullptr).QuadPart;
        LONGLONG renderStart  = masterRender->GetStartTime();
        LONGLONG renderElapsed = now - renderStart;
        devExt->LastCopiedByte = WaveRTMath::TicksToBytes(
            renderElapsed,
            masterRender->GetStreamByteRate(),
            masterRender->GetFrequency());

        LARGE_INTEGER dueTime;
        dueTime.QuadPart = LOOPBACK_PERIOD_100NS;
        KeSetTimerEx(&devExt->LoopbackTimer, dueTime, LOOPBACK_PERIOD_MS, &devExt->LoopbackDpc);
        devExt->TimerRunning = TRUE;
    }

    KeReleaseSpinLock(&devExt->StreamLock, oldIrql);
}

static void UnregisterStreamFromLoopback(DeviceExtension* devExt, CMiniportWaveRTStream* stream, BOOLEAN capture)
{
    if (!devExt) return;

    KIRQL oldIrql;
    KeAcquireSpinLock(&devExt->StreamLock, &oldIrql);

    RemoveEntryList(&stream->m_ListEntry);
    InitializeListHead(&stream->m_ListEntry);

    if ((IsListEmpty(&devExt->RenderStreams) || IsListEmpty(&devExt->CaptureStreams)) && devExt->TimerRunning)
    {
        KeCancelTimer(&devExt->LoopbackTimer);
        devExt->TimerRunning = FALSE;
        devExt->LastCopiedByte = 0;
    }

    KeReleaseSpinLock(&devExt->StreamLock, oldIrql);
}

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
    , m_BitsPerSample(16)
    , m_Channels(2)
    , m_IsFloat(FALSE)
    , m_NotificationBytes(0)
{
    InitializeListHead(&m_ListEntry);
    RtlZeroMemory(m_NotificationEvents, sizeof(m_NotificationEvents));
    LARGE_INTEGER freq = {};
    KeQueryPerformanceCounter(&freq);
    m_Frequency = freq.QuadPart;
}

CMiniportWaveRTStream::~CMiniportWaveRTStream()
{
    // Unregister from loopback engine before resource cleanup.
    UnregisterStreamFromLoopback(m_DevExt, this, m_IsCapture);

    for (int i = 0; i < 8; i++)
    {
        if (m_NotificationEvents[i])
        {
            ObDereferenceObject(m_NotificationEvents[i]);
            m_NotificationEvents[i] = nullptr;
        }
    }

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
    m_IsCapture     = Capture;
    m_BitsPerSample = 16;
    m_Channels      = 2;
    m_IsFloat       = FALSE;

    if (Format)
    {
        auto *wfx  = reinterpret_cast<KSDATAFORMAT*>(Format);
        auto *wave = reinterpret_cast<WAVEFORMATEX*>(wfx + 1);
        m_ByteRate      = wave->nAvgBytesPerSec;
        m_BitsPerSample = wave->wBitsPerSample;
        m_Channels      = wave->nChannels;
        m_IsFloat       = (wave->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);

        if (wave->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            auto *wfext = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(wave);
            m_IsFloat   = !!IsEqualGUID(wfext->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
        }
    }

    DbgPrint("LeylineWaveRT: Stream Init (capture=%d, byteRate=%u, bits=%u, ch=%u, float=%d)\n",
             (int)m_IsCapture, m_ByteRate, m_BitsPerSample, m_Channels, (int)m_IsFloat);
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
    else if (IsEqualGUID(riid, IID_IMiniportWaveRTStreamNotification))
    {
        *ppvObject = reinterpret_cast<PVOID>(static_cast<IMiniportWaveRTStreamNotification*>(this));
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
    KSSTATE prevState = m_State;
    m_State = State;

    if (State == KSSTATE_STOP)
    {
        m_StartTime = 0;
        UnregisterStreamFromLoopback(m_DevExt, this, m_IsCapture);
    }
    else if (State == KSSTATE_RUN && prevState != KSSTATE_RUN)
    {
        m_StartTime = KeQueryPerformanceCounter(nullptr).QuadPart;
        RegisterStreamForLoopback(m_DevExt, this, m_IsCapture);
    }

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
    if (m_OwnsMdl && m_Mdl)
    {
        if (m_Mapping)
        {
            MmUnmapLockedPages(m_Mapping, m_Mdl);
            m_Mapping = nullptr;
        }
        MmFreePagesFromMdl(m_Mdl);
        IoFreeMdl(m_Mdl);
        m_Mdl     = nullptr;
        m_OwnsMdl = FALSE;
    }
    m_Buffer.Reset();
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
// IMiniportWaveRTStreamNotification Implementation
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

STDMETHODIMP CMiniportWaveRTStream::AllocateBufferWithNotification(
    ULONG NotificationCount,
    ULONG RequestedSize, PMDL* AudioBufferMdl,
    ULONG* ActualSize, ULONG* OffsetFromFirstPage,
    MEMORY_CACHING_TYPE* CacheType)
{
    NTSTATUS status = AllocateAudioBuffer(RequestedSize, AudioBufferMdl, ActualSize, OffsetFromFirstPage, CacheType);
    if (NT_SUCCESS(status))
    {
        if (NotificationCount > 0 && ActualSize && *ActualSize > 0)
        {
            m_NotificationBytes = *ActualSize / NotificationCount;
        }
        else
        {
            m_NotificationBytes = 0;
        }
    }
    return status;
}

STDMETHODIMP_(void) CMiniportWaveRTStream::FreeBufferWithNotification(PMDL AudioBufferMdl, ULONG BufferSize)
{
    FreeAudioBuffer(AudioBufferMdl, BufferSize);
    m_NotificationBytes = 0;
}

STDMETHODIMP CMiniportWaveRTStream::RegisterNotificationEvent(PKEVENT NotificationEvent)
{
    if (!NotificationEvent) return STATUS_INVALID_PARAMETER;

    for (int i = 0; i < 8; i++)
    {
        if (m_NotificationEvents[i] == nullptr)
        {
            ObReferenceObject(NotificationEvent);
            m_NotificationEvents[i] = NotificationEvent;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_INSUFFICIENT_RESOURCES;
}

STDMETHODIMP CMiniportWaveRTStream::UnregisterNotificationEvent(PKEVENT NotificationEvent)
{
    if (!NotificationEvent) return STATUS_INVALID_PARAMETER;

    for (int i = 0; i < 8; i++)
    {
        if (m_NotificationEvents[i] == NotificationEvent)
        {
            m_NotificationEvents[i] = nullptr;
            ObDereferenceObject(NotificationEvent);
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}

void CMiniportWaveRTStream::CheckAndSignalEvents(ULONGLONG lastPosBytes, ULONGLONG currentPosBytes)
{
    if (m_NotificationBytes == 0) return;

    ULONGLONG lastBoundary = lastPosBytes / m_NotificationBytes;
    ULONGLONG currentBoundary = currentPosBytes / m_NotificationBytes;

    if (currentBoundary > lastBoundary)
    {
        for (int i = 0; i < 8; i++)
        {
            if (m_NotificationEvents[i])
            {
                KeSetEvent(m_NotificationEvents[i], 0, FALSE);
            }
        }
    }
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

    ULONG sampleRate = 48000;
    ULONG bitsPerSample = isPCM ? 16 : 32;
    ULONG channels = 2;

    if (MatchingDataRange && MatchingDataRange->FormatSize >= sizeof(KSDATARANGE_AUDIO))
    {
        auto* audioRange = reinterpret_cast<PKSDATARANGE_AUDIO>(MatchingDataRange);
        if (audioRange->MaximumSampleFrequency == audioRange->MinimumSampleFrequency)
            sampleRate = audioRange->MaximumSampleFrequency;
        if (audioRange->MaximumBitsPerSample == audioRange->MinimumBitsPerSample)
            bitsPerSample = audioRange->MaximumBitsPerSample;
        if (audioRange->MaximumChannels == audioRange->MinimumChannels)
            channels = audioRange->MaximumChannels;
    }

    ULONG blockAlign = channels * (bitsPerSample / 8);
    ULONG avgBytesPerSec = sampleRate * blockAlign;

    if (isExt)
    {
        auto *r = reinterpret_cast<KSDATAFORMAT_WAVEFORMATEXTENSIBLE*>(DataFormat);
        r->DataFormat.FormatSize  = fmtSize;
        r->DataFormat.Flags       = 0;
        r->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
        r->DataFormat.SubFormat   = DataRange->SubFormat;
        r->DataFormat.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEXTENSIBLE;
        r->WaveFormatExt.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
        r->WaveFormatExt.Format.nChannels       = (WORD)channels;
        r->WaveFormatExt.Format.nSamplesPerSec  = sampleRate;
        r->WaveFormatExt.Format.wBitsPerSample  = (WORD)bitsPerSample;
        r->WaveFormatExt.Format.nBlockAlign     = (WORD)blockAlign;
        r->WaveFormatExt.Format.nAvgBytesPerSec = avgBytesPerSec;
        r->WaveFormatExt.Format.cbSize          = 22;
        r->WaveFormatExt.Samples.wValidBitsPerSample = r->WaveFormatExt.Format.wBitsPerSample;
        r->WaveFormatExt.dwChannelMask          = (channels == 1) ? KSAUDIO_SPEAKER_MONO : KSAUDIO_SPEAKER_STEREO; // Simplified mask
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
        r->WaveFormatEx.nChannels       = (WORD)channels;
        r->WaveFormatEx.nSamplesPerSec  = sampleRate;
        r->WaveFormatEx.wBitsPerSample  = (WORD)bitsPerSample;
        r->WaveFormatEx.nBlockAlign     = (WORD)blockAlign;
        r->WaveFormatEx.nAvgBytesPerSec = avgBytesPerSec;
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
