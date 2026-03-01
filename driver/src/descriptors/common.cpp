// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// COMMON DATA FOR DESCRIPTORS
// Shared data ranges and basic support handlers.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "descriptors_internal.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DATA RANGES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const KSDATARANGE_AUDIO_CUSTOM g_PcmDataRange =
{
    {
        sizeof(KSDATARANGE_AUDIO_CUSTOM), 0, 0, 0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    2, 8, 32, 8000, 192000
};

const KSDATARANGE_AUDIO_CUSTOM g_FloatDataRange =
{
    {
        sizeof(KSDATARANGE_AUDIO_CUSTOM), 0, 0, 0,
        STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
        STATICGUIDOF(KSDATAFORMAT_SUBTYPE_IEEE_FLOAT),
        STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)
    },
    2, 8, 32, 8000, 192000
};

const KSDATARANGE g_BridgeDataRange =
{
    sizeof(KSDATARANGE), 0, 0, 0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
};

const KSDATARANGE * const g_WaveDataRanges[2]   = { (PKSDATARANGE)&g_PcmDataRange, (PKSDATARANGE)&g_FloatDataRange };
const KSDATARANGE * const g_BridgeDataRanges[1] = { &g_BridgeDataRange };

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// INTERFACES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

const KSIDENTIFIER g_KsInterfaces[] =
{
    { STATICGUIDOF(KSINTERFACESETID_Standard), KSINTERFACE_STANDARD_STREAMING, 0 },
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PROPERTY HANDLER HELPERS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NTSTATUS HandleBasicSupportFull(PPCPROPERTY_REQUEST Req, ULONG AccessFlags, ULONG TypeId)
{
    const ULONG fullSize = sizeof(KSPROPERTY_DESCRIPTION);
    const ULONG ulongSize = sizeof(ULONG);

    if (Req->ValueSize == 0)
    {
        Req->ValueSize = fullSize;
        return STATUS_BUFFER_OVERFLOW;
    }
    if (Req->ValueSize >= fullSize)
    {
        auto *desc = reinterpret_cast<KSPROPERTY_DESCRIPTION*>(Req->Value);
        if (desc)
        {
            desc->AccessFlags      = AccessFlags;
            desc->DescriptionSize  = fullSize;
            desc->PropTypeSet.Set  = KSPROPTYPESETID_General;
            desc->PropTypeSet.Id   = TypeId;
            desc->PropTypeSet.Flags = 0;
            desc->MembersListCount = 0;
            desc->Reserved         = 0;
        }
        Req->ValueSize = fullSize;
        return STATUS_SUCCESS;
    }
    if (Req->ValueSize >= ulongSize)
    {
        auto *flags = reinterpret_cast<ULONG*>(Req->Value);
        if (flags) *flags = AccessFlags;
        Req->ValueSize = ulongSize;
        return STATUS_SUCCESS;
    }
    return STATUS_BUFFER_TOO_SMALL;
}
