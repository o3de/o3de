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

#include "PerforcePlugin_precompiled.h"
#include "CryFile.h"
#include "PerforceSourceControl.h"
#include "PasswordDlg.h"

#include <QSettings>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QApplication>
#include <QTimer>

#include <Util/PathUtil.h>
#include <AzCore/base.h>

namespace
{
    CryCriticalSection g_cPerforceValues;
}

void CMyClientUser::Init()
{
    m_e.Clear();
}

////////////////////////////////////////////////////////////
void CMyClientApi::Run(const char* func)
{
    // The "enableStreams" var has to be set prior to any Run call in order to be able to support Perforce streams
    ClientApi::SetVar("enableStreams");
    ClientApi::Run(func);
}

void CMyClientApi::Run(const char* func, ClientUser* clientUserUI)
{
    ClientApi::SetVar("enableStreams");
    ClientApi::Run(func, clientUserUI);
}

////////////////////////////////////////////////////////////
CPerforceSourceControl::CPerforceSourceControl()
    : m_ref(0)
{
    m_bIsWorkOffline = true;
    m_bIsWorkOfflineBecauseOfConnectionLoss = true;
    m_bIsFailConnectionLogged = false;
    m_configurationInvalid = false;
    m_dwLastAccessTime = 0;
    m_client = nullptr;
    m_nextHandle = 0;
    m_lastWorkOfflineResult = true;
    m_lastWorkOfflineBecauseOfConnectionLossResult = true;
    m_clientInitialized = false;
}

ULONG STDMETHODCALLTYPE CPerforceSourceControl::Release()
{
    if ((--m_ref) == 0)
    {
        g_cPerforceValues.Lock();
        FreeData();
        delete this;
        g_cPerforceValues.Unlock();
        return 0;
    }
    else
    {
        return m_ref;
    }
}

CPerforceSourceControl::~CPerforceSourceControl()
{
    delete m_client;
}

bool CPerforceSourceControl::Connect()
{
    // before we call Init we must set up the connection!
    // the user may have overridden the connection on a per-project basis.
    // check the config file.
    AUTO_LOCK(g_cPerforceValues);

    FreeData();

    m_client = new CMyClientApi();

    // Ensure P4PORT is set before calling ClientApi::Init
    const bool p4PortIsSet = nullptr != m_client->GetEnviro()->Get("P4PORT");
    if (p4PortIsSet)
    {
        m_client->Init(&m_e);
        m_clientInitialized = true;
    }

    if (!p4PortIsSet || m_e.Test())
    {
        if (!m_bIsWorkOffline)
        {
            m_bIsWorkOfflineBecauseOfConnectionLoss = true;
        }

        m_bIsWorkOffline = true;
        if (!m_bIsFailConnectionLogged)
        {
            GetIEditor()->GetSystem()->GetILog()->Log(p4PortIsSet ? "\nPerforce plugin: Failed to connect." : "\nPerforce plugin: P4PORT must be set to connect.");
            m_bIsFailConnectionLogged = true;
        }
        if (GetIEditor()->IsSourceControlAvailable())
        {
            //Only Check Connection and than Notify Listeners if SourceControl is available at that instance
            CheckConnectionAndNotifyListeners();
        }
        return false;
    }
    else
    {
        GetIEditor()->GetSystem()->GetILog()->Log("\nPerforce plugin: Connected.");
        m_bIsFailConnectionLogged = false;
        if (m_bIsWorkOfflineBecauseOfConnectionLoss)
        {
            m_bIsWorkOffline = false;
            m_bIsWorkOfflineBecauseOfConnectionLoss = false;
        }
    }

    return true;
}

bool CPerforceSourceControl::Reconnect()
{
    AUTO_LOCK(g_cPerforceValues);

    if ((m_client && m_client->Dropped()) || m_bIsWorkOfflineBecauseOfConnectionLoss)
    {
        if (!m_bIsFailConnectionLogged)
        {
            GetIEditor()->GetSystem()->GetILog()->Log("\nPerforce connection dropped: attempting reconnect");
        }

        FreeData();
        if (!Connect())
        {
            if (!m_bIsWorkOffline)
            {
                m_bIsWorkOfflineBecauseOfConnectionLoss = true;
            }
            m_bIsWorkOffline = true;
            return false;
        }
        else
        {
            CheckConnectionAndNotifyListeners();
        }
    }
    return true;
}

void CPerforceSourceControl::FreeData()
{
    AUTO_LOCK(g_cPerforceValues);

    if (m_client)
    {
        if (m_clientInitialized)
        {
            m_client->Final(&m_e);
            m_clientInitialized = false;
        }
        delete m_client;
        m_client = nullptr;
    }
    m_e.Clear();
}

void CPerforceSourceControl::Init()
{
    Connect();

    {
        AzToolsFramework::SourceControlState state = AzToolsFramework::SourceControlState::Disabled;
        using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
        SCRequestBus::BroadcastResult(state, &SCRequestBus::Events::GetSourceControlState);
        SetSourceControlState(state);
    }
}

void CPerforceSourceControl::ShowSettings()
{
    AUTO_LOCK(g_cPerforceValues);
    bool workOffline = (m_bIsWorkOffline && !m_bIsWorkOfflineBecauseOfConnectionLoss);

    if (PerforceConnection::OpenPasswordDlg())
    {
        m_bIsWorkOfflineBecauseOfConnectionLoss = false;
        bool onlineMode = false;
        {
            using SCRequestBus = AzToolsFramework::SourceControlConnectionRequestBus;
            SCRequestBus::BroadcastResult(onlineMode, &SCRequestBus::Events::IsActive);
        }
        m_bIsWorkOffline = !onlineMode;

        // reset the connection!
        if ((!m_bIsWorkOfflineBecauseOfConnectionLoss) && onlineMode)
        {
            Connect();
        }

        CheckConnectionAndNotifyListeners();
    }
}

const char* CPerforceSourceControl::GetErrorByGenericCode(int nGeneric)
{
    switch (nGeneric)
    {
    case EV_USAGE:
        return "Request is not consistent with documentation or cannot support a server version";
    case EV_UNKNOWN:
        return "Using unknown entity";
    case EV_CONTEXT:
        return "Using entity in wrong context";
    case EV_ILLEGAL:
        return "Trying to do something you can't";
    case EV_NOTYET:
        return "Something must be corrected first";
    case EV_PROTECT:
        return "Operation was prevented by protection level";

    // No fault at all
    case EV_EMPTY:
        return "Action returned empty results";

    // not the fault of the user
    case EV_FAULT:
        return "Inexplicable program fault";
    case EV_CLIENT:
        return "Client side program errors";
    case EV_ADMIN:
        return "Server administrative action required";
    case EV_CONFIG:
        return "Client configuration is inadequate";
    case EV_UPGRADE:
        return "Client or server is too old to interact";
    case EV_COMM:
        return "Communication error";
    case EV_TOOBIG:
        return "File is too big";
    }

    return "Undefined";
}

bool CPerforceSourceControl::Run(const char* func, int nArgs, char* argv[], bool bOnlyFatal)
{
    for (int x = 0; x < nArgs; ++x)
    {
        if (argv[x][0] == '\0')
        {
            return false;
        }
    }

    if (!Reconnect())
    {
        return false;
    }

    bool bRet = false;

    AUTO_LOCK(g_cPerforceValues);

    m_ui.Init();
    m_client->SetArgv(nArgs, argv);
    m_client->Run(func, &m_ui);
    m_client->WaitTag();

#if 0 // connection debug
    for (int argid = 0; argid < nArgs; argid++)
    {
        g_pSystem->GetILog()->Log("\n arg %d : %s", argid, argv[argid]);
    }
#endif // connection debug

    if (bOnlyFatal)
    {
        if (!m_ui.m_e.IsFatal())
        {
            bRet = true;
        }
    }
    else
    {
        if (!m_ui.m_e.Test())
        {
            bRet = true;
        }
    }

    if (m_ui.m_e.GetSeverity() == E_FAILED || m_ui.m_e.GetSeverity() == E_FATAL)
    {
        static int nGenericPrev = 0;
        const int nGeneric = m_ui.m_e.GetGeneric();

        if (IsSomeTimePassed())
        {
            nGenericPrev = 0;
        }

        if (nGenericPrev != nGeneric)
        {
            if ((!bOnlyFatal) || (m_ui.m_e.GetSeverity() == E_FATAL))
            {
                GetIEditor()->GetSystem()->GetILog()->LogError("Perforce plugin: %s", GetErrorByGenericCode(nGeneric));

                StrBuf  m;
                m_ui.m_e.Fmt(&m);
                GetIEditor()->GetSystem()->GetILog()->LogError("Perforce plugin: %s", m.Text());
            }

            nGenericPrev = nGeneric;
        }
    }

    m_ui.m_e.Clear();
    return bRet;
}

void CPerforceSourceControl::SetSourceControlState(SourceControlState state)
{
    AUTO_LOCK(g_cPerforceValues);
    switch (state)
    {
    case SourceControlState::Disabled:
    {
        m_bIsWorkOffline = true;
        m_bIsWorkOfflineBecauseOfConnectionLoss = true;
        m_configurationInvalid = false;
        break;
    }
    case SourceControlState::Active:
        m_bIsWorkOffline = false;
        m_bIsWorkOfflineBecauseOfConnectionLoss = false;
        m_configurationInvalid = false;
        CheckConnectionAndNotifyListeners();
        break;
        
    case SourceControlState::ConfigurationInvalid:
        m_bIsWorkOffline = false;
        m_bIsWorkOfflineBecauseOfConnectionLoss = false;
        m_configurationInvalid = true;
        break;

    default:
        break;
    }
}

CPerforceSourceControl::ConnectivityState CPerforceSourceControl::GetConnectivityState()
{
    ConnectivityState state = ConnectivityState::Disconnected;
    if (!m_bIsWorkOffline)
    {
        state = ConnectivityState::Connected;
    }
    else if (m_configurationInvalid)
    {
        state = ConnectivityState::BadConfiguration;
    }

    return state;
}

bool CPerforceSourceControl::CheckConnectionAndNotifyListeners()
{
    AUTO_LOCK(g_cPerforceValues);

    bool bRet = false;

    bool wasWorkOffline = m_bIsWorkOffline;
    if (!m_bIsWorkOffline)
    {
        bRet = Run("info", 0, 0);
    }

    if (m_bIsWorkOffline || m_configurationInvalid)
    {
        if (!wasWorkOffline)
        {
            // only log once when transitioning offline.
            GetIEditor()->GetSystem()->GetILog()->LogError("Perforce plugin: Perforce is offline");
        }
        return false;
    }

    return bRet;
}

bool CPerforceSourceControl::IsSomeTimePassed()
{
    AUTO_LOCK(g_cPerforceValues);
    const DWORD dwSomeTime = 10000; // 10 sec
    static DWORD dwLastTime = 0;
    DWORD dwCurTime = GetTickCount();

    if (dwCurTime - dwLastTime > dwSomeTime)
    {
        dwLastTime = dwCurTime;
        return true;
    }

    return false;
}

