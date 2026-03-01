// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// INTERNAL LEYLINE DESCRIPTOR DECLARATIONS
// These are only shared within the descriptors/ folder.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include "leyline_descriptors.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HANDLERS (Internal)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NTSTATUS HandleBasicSupportFull(PPCPROPERTY_REQUEST Req, ULONG AccessFlags, ULONG TypeId);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// AUTOMATION TABLES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern const PCAUTOMATION_TABLE g_ComponentAutomationTable;
extern const PCAUTOMATION_TABLE g_WaveFilterAutomationTable;
extern const PCAUTOMATION_TABLE g_TopoFilterAutomationTable;
extern const PCAUTOMATION_TABLE g_PinAutomationTable;
extern const PCAUTOMATION_TABLE g_VolumeAutomationTable;
extern const PCAUTOMATION_TABLE g_MuteAutomationTable;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// PINS & NODES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern const PCNODE_DESCRIPTOR      g_TopoNodes[];
extern const PCPIN_DESCRIPTOR       g_WaveRenderPins[];
extern const PCPIN_DESCRIPTOR       g_WaveCapturePins[];
extern const PCPIN_DESCRIPTOR       g_TopoRenderPins[];
extern const PCPIN_DESCRIPTOR       g_TopoCapturePins[];

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CONNECTIONS & CATEGORIES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern const PCCONNECTION_DESCRIPTOR g_WaveConnections[];
extern const PCCONNECTION_DESCRIPTOR g_WaveCaptureConnections[];
extern const PCCONNECTION_DESCRIPTOR g_TopoConnections[];
extern const PCCONNECTION_DESCRIPTOR g_TopoCaptureConnections[];

extern const GUID g_TopoFilterCategories[];
extern const GUID g_WaveRenderCategories[];
extern const GUID g_WaveCaptureCategories[];

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// COMMON DATA
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern const KSIDENTIFIER g_KsInterfaces[];
