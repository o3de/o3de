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

////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE CPerforceSourceControl::Release()
{
    if ((--m_ref) == 0)
    {
        g_cPerforceValues.Lock();
        delete this;
        g_cPerforceValues.Unlock();
        return 0;
    }
    else
    {
        return m_ref;
    }
}

void CPerforceSourceControl::Init()
{
    UpdateSourceControlState();
}

void CPerforceSourceControl::ShowSettings()
{
    if (PerforceConnection::OpenPasswordDlg())
    {
        UpdateSourceControlState();
    }
}

void CPerforceSourceControl::SetSourceControlState(SourceControlState state)
{
    AUTO_LOCK(g_cPerforceValues);

    switch (state)
    {
        case SourceControlState::Disabled:
            m_connectionState = ConnectivityState::Disconnected;
            break;

        case SourceControlState::Active:
            m_connectionState = ConnectivityState::Connected;
            break;

        case SourceControlState::ConfigurationInvalid:
            m_connectionState = ConnectivityState::BadConfiguration;
            break;

        default:
            break;
    }
}

void CPerforceSourceControl::UpdateSourceControlState()
{
    using namespace AzToolsFramework;

    SourceControlState state = SourceControlState::Disabled;
    SourceControlConnectionRequestBus::BroadcastResult(state,
        &SourceControlConnectionRequestBus::Events::GetSourceControlState);

    SetSourceControlState(state);
}
