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

#include <Source/NetworkTime/NetworkTime.h>

namespace Multiplayer
{
    NetworkTime::NetworkTime()
    {
        AZ::Interface<INetworkTime>::Register(this);
    }

    NetworkTime::~NetworkTime()
    {
        AZ::Interface<INetworkTime>::Unregister(this);
    }

    AZ::TimeMs NetworkTime::ConvertFrameIdToTimeMs([[maybe_unused]] ApplicationFrameId frameId) const
    {
        return AZ::TimeMs{0};
    }

    ApplicationFrameId NetworkTime::ConvertTimeMsToFrameId([[maybe_unused]] AZ::TimeMs timeMs) const
    {
        return ApplicationFrameId{0};
    }

    bool NetworkTime::IsApplicationFrameIdRewound() const
    {
        return m_rewindingConnectionId != AzNetworking::InvalidConnectionId;
    }

    ApplicationFrameId NetworkTime::GetApplicationFrameId() const
    {
        return m_applicationFrameId;
    }

    ApplicationFrameId NetworkTime::GetUnalteredApplicationFrameId() const
    {
        return m_unalteredFrameId;
    }

    void NetworkTime::IncrementApplicationFrameId()
    {
        AZ_Assert(!IsApplicationFrameIdRewound(), "Incrementing the global application frameId is unsupported under a rewound time scope");
        ++m_unalteredFrameId;
        m_applicationFrameId = m_unalteredFrameId;
    }

    void NetworkTime::SyncRewindableEntityState()
    {

    }

    AzNetworking::ConnectionId NetworkTime::GetRewindingConnectionId() const
    {
        return m_rewindingConnectionId;
    }

    ApplicationFrameId NetworkTime::GetApplicationFrameIdForRewindingConnection(AzNetworking::ConnectionId rewindConnectionId) const
    {
        return (IsApplicationFrameIdRewound() && (rewindConnectionId == m_rewindingConnectionId)) ? m_unalteredFrameId : m_applicationFrameId;
    }

    void NetworkTime::AlterApplicationFrameId(ApplicationFrameId frameId, AzNetworking::ConnectionId rewindConnectionId)
    {
        m_applicationFrameId = frameId;
        m_rewindingConnectionId = rewindConnectionId;
    }
}
