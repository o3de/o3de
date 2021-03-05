/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#define USERNAME_LENGTH 64

#include "IEditorClassFactory.h"
#include "Include/IEditorClassFactory.h"

#include "Include/ISourceControl.h"

class CMyClientUser
    : public ClientUser
{
public:
    CMyClientUser()
    {
        Init();
    }
    void Init();

    Error m_e;
};

class CMyClientApi
    : public ClientApi
{
public:
    void Run(const char* func);
    void Run(const char* func, ClientUser* ui);
};

class CPerforceSourceControl
    : public ISourceControl
    , public IClassDesc
{
public:
    // constructor
    CPerforceSourceControl();
    virtual ~CPerforceSourceControl();

    bool Connect();
    bool Reconnect();
    void FreeData();
    void Init();
    bool CheckConnectionAndNotifyListeners();

    virtual void SetSourceControlState(SourceControlState state) override;
    virtual ConnectivityState GetConnectivityState() override;

    virtual void ShowSettings() override;

    bool Run(const char* func, int nArgs, char* argv[], bool bOnlyFatal = false);

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


protected:
    bool IsSomeTimePassed();

    static const char* GetErrorByGenericCode(int nGeneric);

private:
    CMyClientUser m_ui;
    CMyClientApi* m_client;
    Error m_e;

    bool m_bIsWorkOffline;
    bool m_bIsWorkOfflineBecauseOfConnectionLoss;
    bool m_bIsFailConnectionLogged;
    bool m_configurationInvalid;

    DWORD m_dwLastAccessTime;

    ULONG m_ref;
    uint m_nextHandle;

    bool m_lastWorkOfflineResult;
    bool m_lastWorkOfflineBecauseOfConnectionLossResult;

    bool m_clientInitialized;
};
