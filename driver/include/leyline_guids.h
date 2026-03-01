// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LEYLINE GUIDS & CONSTANTS
// All KS, PortCls, and Leyline-specific GUIDs in one place.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once
#include <guiddef.h>
#include <ks.h>
#include <ksmedia.h>

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ETW PROVIDER
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// {71549463-5E1E-4B7E-9F93-A65606E50D64}
DEFINE_GUID(ETW_PROVIDER_GUID,
    0x71549463, 0x5E1E, 0x4B7E, 0x9F, 0x93, 0xA6, 0x56, 0x06, 0xE5, 0x0D, 0x64);

// {b31122c0-6d13-11d1-afad-00a0c9223196}
#ifndef KSDATAFORMAT_SPECIFIER_WAVEFORMATEXTENSIBLE
DEFINE_GUID(KSDATAFORMAT_SPECIFIER_WAVEFORMATEXTENSIBLE,
    0xb31122c0, 0x6d13, 0x11d1, 0xaf, 0xad, 0x00, 0xa0, 0xc9, 0x22, 0x31, 0x96);
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ADDITIONAL KS FORMAT GUIDS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#ifndef KSPROPSETID_AudioEffectsDiscovery
DEFINE_GUID(KSPROPSETID_AudioEffectsDiscovery,
    0xB49EEC73, 0xC88F, 0x40E1, 0x88, 0x48, 0xD3, 0xCE, 0x2C, 0x4B, 0x00, 0x51);
#endif

#ifndef KSPROPERTY_AUDIOEFFECTSDISCOVERY_EFFECTSLIST
#define KSPROPERTY_AUDIOEFFECTSDISCOVERY_EFFECTSLIST 1
#endif

#ifndef KSPROPSETID_AudioModule
DEFINE_GUID(KSPROPSETID_AudioModule,
    0xC034FDB0, 0xFF4C, 0x4788, 0xB3, 0xB6, 0xBF, 0x3E, 0x15, 0xCD, 0xC3, 0xE9);
#endif

#ifndef KSPROPERTY_AUDIOMODULE_DESCRIPTORS
#define KSPROPERTY_AUDIOMODULE_DESCRIPTORS 1
#endif
#ifndef KSPROPERTY_AUDIOMODULE_COMMAND
#define KSPROPERTY_AUDIOMODULE_COMMAND 2
#endif
#ifndef KSPROPERTY_AUDIOMODULE_NOTIFICATION_DEVICE_ID
#define KSPROPERTY_AUDIOMODULE_NOTIFICATION_DEVICE_ID 3
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PROPERTY ID CONSTANTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// VT_I4 / VT_BOOL from windef or propvariant.
#ifndef VT_I4
#define VT_I4   3
#endif
#ifndef VT_BOOL
#define VT_BOOL 11
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PIN INDEX CONSTANTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define KSPIN_WAVE_SINK     0
#define KSPIN_WAVE_BRIDGE   1
#define KSPIN_TOPO_BRIDGE   0
#define KSPIN_TOPO_LINEOUT  1

// KS constants not always in older headers
#ifndef MM_ALLOCATE_FULLY_REQUIRED
#define MM_ALLOCATE_FULLY_REQUIRED 0x00000004
#endif
