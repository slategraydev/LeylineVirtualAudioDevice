// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// TOPOLOGY MINIPORT IMPLEMENTATION
// Manages volume, mute, and endpoint topology.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "leyline_miniport.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CMiniportTopology
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CMiniportTopology::CMiniportTopology(PUNKNOWN OuterUnknown, BOOLEAN IsCapture)
    : CUnknown(OuterUnknown)
    , m_IsCapture(IsCapture)
    , m_IsInitialized(FALSE)
    , m_Port(nullptr)
{}

CMiniportTopology::~CMiniportTopology() {}

STDMETHODIMP CMiniportTopology::NonDelegatingQueryInterface(REFIID riid, PVOID* ppvObject)
{
    if (IsEqualGUID(riid, IID_IMiniportTopology) || IsEqualGUID(riid, IID_IUnknown) || IsEqualGUID(riid, IID_IMiniport))
    {
        *ppvObject = reinterpret_cast<PVOID>(static_cast<IMiniportTopology*>(this));
        AddRef();
        return STATUS_SUCCESS;
    }
    *ppvObject = nullptr;
    return STATUS_NOINTERFACE;
}

STDMETHODIMP CMiniportTopology::GetDescription(PPCFILTER_DESCRIPTOR* Description)
{
    if (!Description) return STATUS_INVALID_PARAMETER;
    DbgPrint("LeylineTopo: GetDescription (capture=%d)\n", (int)m_IsCapture);
    *Description = const_cast<PPCFILTER_DESCRIPTOR>(m_IsCapture ? &g_TopoCaptureFilterDescriptor : &g_TopoRenderFilterDescriptor);
    return STATUS_SUCCESS;
}

STDMETHODIMP CMiniportTopology::DataRangeIntersection(
    ULONG PinId, PKSDATARANGE DataRange, PKSDATARANGE /*MatchingDataRange*/,
    ULONG /*DataFormatSize*/, PVOID /*DataFormat*/, PULONG /*ResultantFormatSize*/)
{
    UNREFERENCED_PARAMETER(DataRange);
    DbgPrint("LeylineTopo: DataRangeIntersection Pin=%u\n", PinId);
    return STATUS_NOT_IMPLEMENTED;
}

STDMETHODIMP CMiniportTopology::Init(PUNKNOWN /*UnknownAdapter*/, PRESOURCELIST /*ResourceList*/, IPortTopology* Port)
{
    DbgPrint("LeylineTopo: Init (capture=%d)\n", (int)m_IsCapture);
    m_Port = Port;
    m_IsInitialized = TRUE;
    return STATUS_SUCCESS;
}
