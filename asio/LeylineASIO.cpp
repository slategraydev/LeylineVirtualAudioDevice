// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// LEYLINE ASIO PROXY DRIVER (SKELETON)
// User-mode ASIO DLL that interfaces directly with the Leyline Virtual Audio Device.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include <windows.h>
#include <stdio.h>

// Note: A complete ASIO implementation requires the proprietary Steinberg ASIO SDK 
// headers (iasiodrv.h). This file provides the COM DLL wrapper structure that routes
// standard ASIO requests down to the Leyline kernel driver via DeviceIoControl.

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// Exported standard COM DLL entry points for DAW registration

STDAPI DllCanUnloadNow(void)
{
    return S_OK;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    // A full implementation provides an IClassFactory that instantiates 
    // an object derived from the Steinberg IASIO COM interface.
    // That instance will handle ASIO lifecycle methods like:
    // - init()
    // - getChannels()
    // - getBufferSize()
    // - createBuffers() -> Which we will link to IOCTL_LEYLINE_MAP_BUFFER
    // - start()
    // - stop()
    
    *ppv = nullptr;
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllRegisterServer(void)
{
    // Register the Leyline ASIO driver under the industry standard key:
    // HKEY_LOCAL_MACHINE\SOFTWARE\ASIO\Leyline Virtual Audio Device
    // so that Cubase, Ableton, ProTools will automatically discover it.
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    // Unregister the Leyline ASIO driver
    return S_OK;
}
