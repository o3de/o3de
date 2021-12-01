/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "MainWindowSavedState.h"
#include <AzCore/Serialization/SerializeContext.h>
#include <QtCore/QByteArray>

namespace AzToolsFramework
{
    void MainWindowSavedState::Init(const QByteArray& windowState, const QByteArray& windowGeom)
    {
        m_serializableWindowState.clear();

        m_windowState.assign((AZ::u8*)windowState.begin(), (AZ::u8*)windowState.end());
        m_windowGeometry.assign((AZ::u8*)windowGeom.begin(), (AZ::u8*)windowGeom.end());

        // save 2k at a time :( need a better way to do this.
        AZStd::size_t pos = 0;
        AZStd::size_t remaining = m_windowState.size();

        while (remaining > 0)
        {
            AZStd::size_t bytes_this_gulp = AZStd::min((AZStd::size_t)2000, remaining);
            m_serializableWindowState.push_back();
            m_serializableWindowState.back().assign((AZ::u8*)windowState.begin() + pos, (AZ::u8*)windowState.begin() + pos + bytes_this_gulp);
            pos += bytes_this_gulp;
            remaining -= bytes_this_gulp;
        }
    }

    const AZStd::vector<AZ::u8>&  MainWindowSavedState::GetWindowState()
    {
        m_windowState.clear();
        for (auto it = m_serializableWindowState.begin(); it != m_serializableWindowState.end(); ++it)
        {
            m_windowState.insert(m_windowState.end(), it->begin(), it->end());
        }
        return m_windowState;
    }

    void MainWindowSavedState::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<MainWindowSavedState>()
                ->Version(2)
                ->Field("m_windowGeometry", &MainWindowSavedState::m_windowGeometry)
                ->Field("m_serializableWindowState", &MainWindowSavedState::m_serializableWindowState);
        }
    }
}
