/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include "IEditorClassFactory.h"
#include "Include/IEditorClassFactory.h"

#include "Include/ISourceControl.h"


class CPerforceSourceControl
    : public ISourceControl
    , public IClassDesc
{
public:
    // constructor
    CPerforceSourceControl() = default;
    virtual ~CPerforceSourceControl() = default;

    void Init();

    // ISourceControl
    void SetSourceControlState(SourceControlState state) override;
    ConnectivityState GetConnectivityState() override { return m_connectionState; };
    void ShowSettings() override;

    // from IClassDesc
    virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_SCM_PROVIDER; };
    REFGUID ClassID()
    {
        // {3c209e66-0728-4d43-897d-168962d5c8b5}
        static const GUID guid = {
            0x3c209e66, 0x0728, 0x4d43, { 0x89, 0x7d, 0x16, 0x89, 0x62, 0xd5, 0xc8, 0xb5 }
        };
        return guid;
    }
    virtual QString ClassName() { return "Perforce source control"; };
    virtual QString Category() { return "SourceControl"; };
    virtual void ShowAbout() {};

    // from IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj)
    {
        if (riid == __uuidof(ISourceControl) /* && m_pIntegrator*/)
        {
            *ppvObj = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() { return ++m_ref; };
    ULONG STDMETHODCALLTYPE Release();

private:
    void UpdateSourceControlState();

    ULONG m_ref{ 0 };
    ConnectivityState m_connectionState{ ConnectivityState::Disconnected };
};
