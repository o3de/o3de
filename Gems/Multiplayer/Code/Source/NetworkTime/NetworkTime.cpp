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

    bool NetworkTime::IsTimeRewound() const
    {
        return m_rewindingConnectionId != AzNetworking::InvalidConnectionId;
    }

    HostFrameId NetworkTime::GetHostFrameId() const
    {
        return m_hostFrameId;
    }

    HostFrameId NetworkTime::GetUnalteredHostFrameId() const
    {
        return m_unalteredFrameId;
    }

    void NetworkTime::IncrementHostFrameId()
    {
        AZ_Assert(!IsTimeRewound(), "Incrementing the global application frameId is unsupported under a rewound time scope");
        ++m_unalteredFrameId;
        m_hostFrameId = m_unalteredFrameId;
    }

    AZ::TimeMs NetworkTime::GetHostTimeMs() const
    {
        return m_hostTimeMs;
    }

    void NetworkTime::SyncRewindableEntityState()
    {

    }

    AzNetworking::ConnectionId NetworkTime::GetRewindingConnectionId() const
    {
        return m_rewindingConnectionId;
    }

    HostFrameId NetworkTime::GetHostFrameIdForRewindingConnection(AzNetworking::ConnectionId rewindConnectionId) const
    {
        return (IsTimeRewound() && (rewindConnectionId == m_rewindingConnectionId)) ? m_unalteredFrameId : m_hostFrameId;
    }

    void NetworkTime::AlterTime(HostFrameId frameId, AZ::TimeMs timeMs, AzNetworking::ConnectionId rewindConnectionId)
    {
        m_hostFrameId = frameId;
        m_hostTimeMs = timeMs;
        m_rewindingConnectionId = rewindConnectionId;
    }
}
