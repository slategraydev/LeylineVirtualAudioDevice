// Copyright (c) 2026 Randall Rosas (Slategray).
// All rights reserved.

#include "leyline_miniport.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Implementation of the standard `CUnknown` base class. 
// PortCls requires this for COM-like interface handling.
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

CUnknown::CUnknown(PUNKNOWN pUnknownOuter)
    : m_lRefCount(1)
{
    // Use the outer unknown if aggregated. Otherwise, we are our own outer unknown.
    m_pUnknownOuter = pUnknownOuter ? pUnknownOuter : reinterpret_cast<PUNKNOWN>(static_cast<INonDelegatingUnknown*>(this));
}

CUnknown::~CUnknown()
{
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// COM Reference Management
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

STDMETHODIMP_(ULONG) CUnknown::NonDelegatingAddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG) CUnknown::NonDelegatingRelease()
{
    ULONG lRefCount = InterlockedDecrement(&m_lRefCount);
    if (lRefCount == 0)
    {
        delete this;
    }
    return lRefCount;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Interface Discovery
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

STDMETHODIMP_(NTSTATUS) CUnknown::NonDelegatingQueryInterface(REFIID riid, PVOID* ppvObject)
{
    if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppvObject = reinterpret_cast<PVOID>(static_cast<INonDelegatingUnknown*>(this));
        NonDelegatingAddRef();
        return STATUS_SUCCESS;
    }
    *ppvObject = nullptr;
    return STATUS_NOINTERFACE;
}
