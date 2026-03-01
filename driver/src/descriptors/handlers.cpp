// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PROPERTY HANDLERS
// Implementation of KS property handlers for volume, mute, and jack info.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "descriptors_internal.h"

NTSTATUS ComponentIdHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest) return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        return HandleBasicSupportFull(PropertyRequest,
               KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, VT_I4);

    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = sizeof(KSCOMPONENTID);
        return STATUS_BUFFER_OVERFLOW;
    }
    if (PropertyRequest->ValueSize < sizeof(KSCOMPONENTID))
        return STATUS_BUFFER_TOO_SMALL;

    auto *id = reinterpret_cast<KSCOMPONENTID*>(PropertyRequest->Value);
    if (id)
    {
        id->Manufacturer = { 0x534C, 0x4154, 0x4547, { 0x52,0x41,0x59,0x44,0x45,0x56,0x31,0x31 } };
        id->Product      = { 0x4C45, 0x594C, 0x494E, { 0x45,0x41,0x55,0x44,0x49,0x4F,0x31,0x31 } };
        id->Component    = { 0xDEADBEEF, 0xCAFE, 0xFEED, { 0x4C,0x45,0x59,0x4C,0x49,0x4E,0x45,0x31 } };
        id->Name         = GUID_NULL;
        id->Version      = 1;
        id->Revision     = 0;
    }
    return STATUS_SUCCESS;
}

NTSTATUS JackDescriptionHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest) return STATUS_INVALID_PARAMETER;
    if (!PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;

    ULONG propId = PropertyRequest->PropertyItem->Id;
    ULONG pinId  = 0xFFFFFFFF;
    if (PropertyRequest->InstanceSize >= sizeof(ULONG))
        pinId = *reinterpret_cast<ULONG*>(PropertyRequest->Instance);

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        return HandleBasicSupportFull(PropertyRequest,
               KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, VT_I4);

    if (propId == KSPROPERTY_JACK_DESCRIPTION)
    {
        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = sizeof(KSJACK_DESCRIPTION);
            return STATUS_BUFFER_OVERFLOW;
        }
        if (PropertyRequest->ValueSize < sizeof(KSJACK_DESCRIPTION))
            return STATUS_BUFFER_TOO_SMALL;

        auto *j = reinterpret_cast<KSJACK_DESCRIPTION*>(PropertyRequest->Value);
        if (j)
        {
            j->ChannelMapping  = 0x3;   // KSAUDIO_SPEAKER_STEREO
            j->Color           = 0;
            j->ConnectionType  = (EPcxConnectionType)1;     // eConnType3Point5mm
            j->GeoLocation     = (EPcxGeoLocation)1;        // eGeoLocRear
            j->GenLocation     = (EPcxGenLocation)0;
            j->PortConnection  = (EPxcPortConnection)0;
            j->IsConnected     = TRUE;
        }
        return STATUS_SUCCESS;
    }
    else if (propId == KSPROPERTY_JACK_DESCRIPTION2)
    {
        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = sizeof(KSJACK_DESCRIPTION2);
            return STATUS_BUFFER_OVERFLOW;
        }
        if (PropertyRequest->ValueSize < sizeof(KSJACK_DESCRIPTION2))
            return STATUS_BUFFER_TOO_SMALL;

        auto *j2 = reinterpret_cast<KSJACK_DESCRIPTION2*>(PropertyRequest->Value);
        if (j2) { j2->DeviceStateInfo = 0; j2->JackCapabilities = 0; }
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS VolumeHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest) return STATUS_INVALID_PARAMETER;

    struct VOL_BASIC
    {
        KSPROPERTY_DESCRIPTION  desc;
        KSPROPERTY_MEMBERSHEADER hdr;
        KSPROPERTY_STEPPING_LONG stepping;
    };

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        const ULONG fullSize  = sizeof(VOL_BASIC);
        const ULONG ulongSize = sizeof(ULONG);

        if (PropertyRequest->ValueSize == 0)
        {
            PropertyRequest->ValueSize = fullSize;
            return STATUS_BUFFER_OVERFLOW;
        }
        if (PropertyRequest->ValueSize >= fullSize)
        {
            auto *v = reinterpret_cast<VOL_BASIC*>(PropertyRequest->Value);
            if (v)
            {
                v->desc.AccessFlags      = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;
                v->desc.DescriptionSize  = fullSize;
                v->desc.PropTypeSet.Set  = KSPROPTYPESETID_General;
                v->desc.PropTypeSet.Id   = VT_I4;
                v->desc.PropTypeSet.Flags = 0;
                v->desc.MembersListCount = 1;
                v->desc.Reserved         = 0;
                v->hdr.MembersFlags  = KSPROPERTY_MEMBER_STEPPEDRANGES;
                v->hdr.MembersSize   = sizeof(KSPROPERTY_STEPPING_LONG);
                v->hdr.MembersCount  = 1;
                v->hdr.Flags         = 0;
                v->stepping.Bounds.SignedMinimum  = -96 * 0x10000;
                v->stepping.Bounds.SignedMaximum  = 0;
                v->stepping.SteppingDelta  = 0x10000;
                v->stepping.Reserved       = 0;
            }
            PropertyRequest->ValueSize = fullSize;
            return STATUS_SUCCESS;
        }
        if (PropertyRequest->ValueSize >= ulongSize)
        {
            auto *f = reinterpret_cast<ULONG*>(PropertyRequest->Value);
            if (f) *f = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;
            PropertyRequest->ValueSize = ulongSize;
            return STATUS_SUCCESS;
        }
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = sizeof(LONG);
        return STATUS_BUFFER_OVERFLOW;
    }
    if (PropertyRequest->ValueSize < sizeof(LONG))
        return STATUS_BUFFER_TOO_SMALL;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        auto *val = reinterpret_cast<LONG*>(PropertyRequest->Value);
        if (val) *val = 0;  // 0 dB default
    }
    return STATUS_SUCCESS;
}

NTSTATUS MuteHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest) return STATUS_INVALID_PARAMETER;

    ULONG rwFlags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        return HandleBasicSupportFull(PropertyRequest, rwFlags, VT_BOOL);

    if (PropertyRequest->ValueSize == 0) { PropertyRequest->ValueSize = sizeof(LONG); return STATUS_BUFFER_OVERFLOW; }
    if (PropertyRequest->ValueSize < sizeof(LONG)) return STATUS_BUFFER_TOO_SMALL;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        auto *val = reinterpret_cast<LONG*>(PropertyRequest->Value);
        if (val) *val = 0;  // Unmuted by default.
    }
    return STATUS_SUCCESS;
}

NTSTATUS PinCategoryHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest || !PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        return HandleBasicSupportFull(PropertyRequest, KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, VT_CLSID);

    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = sizeof(GUID);
        return STATUS_BUFFER_OVERFLOW;
    }
    if (PropertyRequest->ValueSize < sizeof(GUID))
        return STATUS_BUFFER_TOO_SMALL;

    GUID *category = reinterpret_cast<GUID*>(PropertyRequest->Value);
    if (!category) return STATUS_INVALID_PARAMETER;

    *category = KSCATEGORY_AUDIO;
    PropertyRequest->ValueSize = sizeof(GUID);
    return STATUS_SUCCESS;
}

NTSTATUS PinNameHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest || !PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        return HandleBasicSupportFull(PropertyRequest, KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT, VT_CLSID);

    if (PropertyRequest->ValueSize == 0)
    {
        PropertyRequest->ValueSize = sizeof(GUID);
        return STATUS_BUFFER_OVERFLOW;
    }
    if (PropertyRequest->ValueSize < sizeof(GUID))
        return STATUS_BUFFER_TOO_SMALL;

    GUID *name = reinterpret_cast<GUID*>(PropertyRequest->Value);
    if (!name) return STATUS_INVALID_PARAMETER;

    *name = GUID_NULL;
    PropertyRequest->ValueSize = sizeof(GUID);
    return STATUS_SUCCESS;
}

NTSTATUS ProposedFormatHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest || !PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;

    ULONG rwFlags = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
        return HandleBasicSupportFull(PropertyRequest, rwFlags, VT_I4);

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_SET)
        return STATUS_SUCCESS;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_GET)
    {
        struct KSDATAFORMAT_WAVEFORMATEXTENSIBLE_LOCAL
        {
            KSDATAFORMAT        DataFormat;
            WAVEFORMATEXTENSIBLE WaveFormatExt;
        };

        ULONG fmtSize = sizeof(KSDATAFORMAT_WAVEFORMATEXTENSIBLE_LOCAL);
        if (PropertyRequest->ValueSize == 0) { PropertyRequest->ValueSize = fmtSize; return STATUS_BUFFER_OVERFLOW; }
        if (PropertyRequest->ValueSize < fmtSize) return STATUS_BUFFER_TOO_SMALL;

        auto *r = reinterpret_cast<KSDATAFORMAT_WAVEFORMATEXTENSIBLE_LOCAL*>(PropertyRequest->Value);
        if (r)
        {
            r->DataFormat.FormatSize  = fmtSize;
            r->DataFormat.Flags       = 0;
            r->DataFormat.SampleSize  = 4;
            r->DataFormat.Reserved    = 0;
            r->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
            r->DataFormat.SubFormat   = KSDATAFORMAT_SUBTYPE_PCM;
            r->DataFormat.Specifier   = KSDATAFORMAT_SPECIFIER_WAVEFORMATEXTENSIBLE;

            r->WaveFormatExt.Format.wFormatTag      = WAVE_FORMAT_EXTENSIBLE;
            r->WaveFormatExt.Format.nChannels        = 2;
            r->WaveFormatExt.Format.nSamplesPerSec   = 48000;
            r->WaveFormatExt.Format.wBitsPerSample   = 16;
            r->WaveFormatExt.Format.nBlockAlign      = 4;
            r->WaveFormatExt.Format.nAvgBytesPerSec  = 48000 * 4;
            r->WaveFormatExt.Format.cbSize            = 22;
            r->WaveFormatExt.Samples.wValidBitsPerSample = 16;
            r->WaveFormatExt.dwChannelMask            = KSAUDIO_SPEAKER_STEREO;
            r->WaveFormatExt.SubFormat               = KSDATAFORMAT_SUBTYPE_PCM;
        }
        PropertyRequest->ValueSize = fmtSize;
        return STATUS_SUCCESS;
    }
    return STATUS_SUCCESS;
}

NTSTATUS AudioEffectsDiscoveryHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest || !PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        if (PropertyRequest->ValueSize < sizeof(ULONG)) { PropertyRequest->ValueSize = sizeof(ULONG); return STATUS_BUFFER_OVERFLOW; }
        auto *f = reinterpret_cast<ULONG*>(PropertyRequest->Value);
        if (f) *f = KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
        PropertyRequest->ValueSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS AudioModuleHandler(PPCPROPERTY_REQUEST PropertyRequest)
{
    if (!PropertyRequest || !PropertyRequest->PropertyItem) return STATUS_INVALID_PARAMETER;
    ULONG propId = PropertyRequest->PropertyItem->Id;

    if (PropertyRequest->Verb & KSPROPERTY_TYPE_BASICSUPPORT)
    {
        if (PropertyRequest->ValueSize < sizeof(ULONG)) { PropertyRequest->ValueSize = sizeof(ULONG); return STATUS_BUFFER_OVERFLOW; }
        auto *f = reinterpret_cast<ULONG*>(PropertyRequest->Value);
        if (f)
        {
            *f = (propId == KSPROPERTY_AUDIOMODULE_COMMAND)
                 ? KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT
                 : KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_BASICSUPPORT;
        }
        PropertyRequest->ValueSize = sizeof(ULONG);
        return STATUS_SUCCESS;
    }
    return STATUS_NOT_IMPLEMENTED;
}
