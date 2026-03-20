// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// INTERNAL LEYLINE DESCRIPTOR DECLARATIONS
// These are only shared within the descriptors/ folder.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#pragma once

#include "leyline_miniport.h"

// FDO global, needed by property handlers to recover DeviceExtension.
extern PDEVICE_OBJECT g_FunctionalDeviceObject;

extern PDEVICE_OBJECT g_FunctionalDeviceObject;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HANDLERS (Internal)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NTSTATUS HandleBasicSupportFull(PPCPROPERTY_REQUEST Req, ULONG AccessFlags, ULONG TypeId);
NTSTATUS SignalProcessingModesHandler(PPCPROPERTY_REQUEST PropertyRequest);

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

extern const PCNODE_DESCRIPTOR      g_TopoNodes[2];
extern const PCPIN_DESCRIPTOR       g_WaveRenderPins[2];
extern const PCPIN_DESCRIPTOR       g_WaveCapturePins[2];
extern const PCPIN_DESCRIPTOR       g_TopoRenderPins[2];
extern const PCPIN_DESCRIPTOR       g_TopoCapturePins[2];

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// CONNECTIONS & CATEGORIES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern const PCCONNECTION_DESCRIPTOR g_WaveConnections[1];
extern const PCCONNECTION_DESCRIPTOR g_WaveCaptureConnections[1];
extern const PCCONNECTION_DESCRIPTOR g_TopoConnections[3];
extern const PCCONNECTION_DESCRIPTOR g_TopoCaptureConnections[1];

extern const GUID g_TopoFilterCategories[2];
extern const GUID g_WaveRenderCategories[3];
extern const GUID g_WaveCaptureCategories[3];

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// COMMON DATA
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern const KSIDENTIFIER g_KsInterfaces[1];
