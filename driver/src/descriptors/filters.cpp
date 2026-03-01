// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// FILTER DESCRIPTORS
// Combined PortCls descriptors for all virtual audio filters.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "descriptors_internal.h"

const PCFILTER_DESCRIPTOR g_WaveRenderFilterDescriptor =
{
    0, &g_WaveFilterAutomationTable,
    sizeof(PCPIN_DESCRIPTOR), SIZEOF_ARRAY(g_WaveRenderPins), g_WaveRenderPins,
    0, 0, nullptr,
    SIZEOF_ARRAY(g_WaveConnections), g_WaveConnections,
    SIZEOF_ARRAY(g_WaveRenderCategories), g_WaveRenderCategories
};

const PCFILTER_DESCRIPTOR g_WaveCaptureFilterDescriptor =
{
    0, &g_WaveFilterAutomationTable,
    sizeof(PCPIN_DESCRIPTOR), SIZEOF_ARRAY(g_WaveCapturePins), g_WaveCapturePins,
    0, 0, nullptr,
    SIZEOF_ARRAY(g_WaveCaptureConnections), g_WaveCaptureConnections,
    SIZEOF_ARRAY(g_WaveCaptureCategories), g_WaveCaptureCategories
};

const PCFILTER_DESCRIPTOR g_TopoRenderFilterDescriptor =
{
    0, &g_TopoFilterAutomationTable,
    sizeof(PCPIN_DESCRIPTOR), SIZEOF_ARRAY(g_TopoRenderPins), g_TopoRenderPins,
    sizeof(PCNODE_DESCRIPTOR), SIZEOF_ARRAY(g_TopoNodes), g_TopoNodes,
    SIZEOF_ARRAY(g_TopoConnections), g_TopoConnections,
    SIZEOF_ARRAY(g_TopoFilterCategories), g_TopoFilterCategories
};

const PCFILTER_DESCRIPTOR g_TopoCaptureFilterDescriptor =
{
    0, &g_TopoFilterAutomationTable,
    sizeof(PCPIN_DESCRIPTOR), SIZEOF_ARRAY(g_TopoCapturePins), g_TopoCapturePins,
    sizeof(PCNODE_DESCRIPTOR), SIZEOF_ARRAY(g_TopoNodes), g_TopoNodes,
    SIZEOF_ARRAY(g_TopoCaptureConnections), g_TopoCaptureConnections,
    SIZEOF_ARRAY(g_TopoFilterCategories), g_TopoFilterCategories
};
