/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/PlatformIncl.h>
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
    AZStd::mutex g_cPerforceValues;
}

////////////////////////////////////////////////////////////
ULONG STDMETHODCALLTYPE CPerforceSourceControl::Release()
{
    if ((--m_ref) == 0)
    {
        g_cPerforceValues.lock();
        delete this;
        g_cPerforceValues.unlock();
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
    AZStd::scoped_lock lock(g_cPerforceValues);

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
