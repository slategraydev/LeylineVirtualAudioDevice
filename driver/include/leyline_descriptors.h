// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LEYLINE KS DESCRIPTOR TABLES
// Statically-initialized descriptors for Wave and Topology filters.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include "leyline_common.h"
#include "leyline_guids.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PROPERTY HANDLERS (Forward Declarations)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NTSTATUS ComponentIdHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS JackDescriptionHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS VolumeHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS MuteHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS PinCategoryHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS PinNameHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS ProposedFormatHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS AudioEffectsDiscoveryHandler(PPCPROPERTY_REQUEST PropertyRequest);
NTSTATUS AudioModuleHandler(PPCPROPERTY_REQUEST PropertyRequest);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// DESCRIPTOR TABLE DECLARATIONS
// Definitions are in descriptors.cpp; extern refs for other translation units.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// Custom data range structure for audio format specification.
// Since KSDATAFORMAT is a union in some environments, we use composition.
typedef struct {
    KSDATARANGE DataRange;
    ULONG MaximumChannels;
    ULONG MinimumBitsPerSample;
    ULONG MaximumBitsPerSample;
    ULONG MinimumSampleFrequency;
    ULONG MaximumSampleFrequency;
} KSDATARANGE_AUDIO_CUSTOM;

extern const KSDATARANGE_AUDIO_CUSTOM   g_PcmDataRange;
extern const KSDATARANGE_AUDIO_CUSTOM   g_FloatDataRange;
extern const KSDATARANGE                g_BridgeDataRange;
extern const KSDATARANGE * const        g_WaveDataRanges[2];
extern const KSDATARANGE * const        g_BridgeDataRanges[1];

extern const PCFILTER_DESCRIPTOR        g_WaveRenderFilterDescriptor;
extern const PCFILTER_DESCRIPTOR        g_WaveCaptureFilterDescriptor;
extern const PCFILTER_DESCRIPTOR        g_TopoRenderFilterDescriptor;
extern const PCFILTER_DESCRIPTOR        g_TopoCaptureFilterDescriptor;
