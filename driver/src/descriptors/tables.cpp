// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PIN, NODE, AND CONNECTION TABLES
// Topology graphs for Render and Capture devices.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "descriptors_internal.h"

const PCNODE_DESCRIPTOR g_TopoNodes[] =
{
    { 0, &g_VolumeAutomationTable, &KSNODETYPE_VOLUME,     &KSAUDFNAME_MASTER_VOLUME },
    { 0, &g_MuteAutomationTable,   &KSNODETYPE_MUTE,       &KSAUDFNAME_MASTER_MUTE   },
};

const PCPIN_DESCRIPTOR g_WaveRenderPins[] =
{
    {
        4, 4, 1,
        &g_PinAutomationTable,
        {
            SIZEOF_ARRAY(g_KsInterfaces), g_KsInterfaces,
            0, nullptr,
            SIZEOF_ARRAY(g_WaveDataRanges), (PKSDATARANGE*)g_WaveDataRanges,
            KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr,
            0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
};

const PCPIN_DESCRIPTOR g_WaveCapturePins[] =
{
    {
        4, 4, 1,
        &g_PinAutomationTable,
        {
            SIZEOF_ARRAY(g_KsInterfaces), g_KsInterfaces,
            0, nullptr,
            SIZEOF_ARRAY(g_WaveDataRanges), (PKSDATARANGE*)g_WaveDataRanges,
            KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_SINK,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr,
            0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
};

const PCPIN_DESCRIPTOR g_TopoRenderPins[] =
{
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr, 0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr, 0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_SPEAKER, nullptr, 0
        }
    },
};

const PCPIN_DESCRIPTOR g_TopoCapturePins[] =
{
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr, 0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_NONE,
            &KSNODETYPE_MICROPHONE, nullptr, 0
        }
    },
    {
        1, 1, 1,
        &g_PinAutomationTable,
        {
            0, nullptr, 0, nullptr,
            SIZEOF_ARRAY(g_BridgeDataRanges), (PKSDATARANGE*)g_BridgeDataRanges,
            KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_NONE,
            &KSCATEGORY_AUDIO, nullptr, 0
        }
    },
};

const PCCONNECTION_DESCRIPTOR g_WaveConnections[] =
{
    { PCFILTER_NODE, KSPIN_WAVE_SINK, PCFILTER_NODE, KSPIN_WAVE_BRIDGE }
};

const PCCONNECTION_DESCRIPTOR g_WaveCaptureConnections[] =
{
    { PCFILTER_NODE, KSPIN_WAVE_BRIDGE, PCFILTER_NODE, KSPIN_WAVE_SINK }
};

const PCCONNECTION_DESCRIPTOR g_TopoConnections[] =
{
    { PCFILTER_NODE, KSPIN_TOPO_BRIDGE,  0,             0 },
    { 0,             1,                  1,             0 },
    { 1,             1,                  PCFILTER_NODE, KSPIN_TOPO_LINEOUT }
};

const PCCONNECTION_DESCRIPTOR g_TopoCaptureConnections[] =
{
    { PCFILTER_NODE, 0, PCFILTER_NODE, 1 }
};

const GUID g_TopoFilterCategories[]      = { STATICGUIDOF(KSCATEGORY_AUDIO), STATICGUIDOF(KSCATEGORY_TOPOLOGY) };
const GUID g_WaveRenderCategories[]       = { STATICGUIDOF(KSCATEGORY_AUDIO), STATICGUIDOF(KSCATEGORY_RENDER), STATICGUIDOF(KSCATEGORY_REALTIME) };
const GUID g_WaveCaptureCategories[]      = { STATICGUIDOF(KSCATEGORY_AUDIO), STATICGUIDOF(KSCATEGORY_CAPTURE), STATICGUIDOF(KSCATEGORY_REALTIME) };
