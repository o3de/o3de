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

#pragma once

#include <Multiplayer/NetworkTime/INetworkTime.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Console/IConsole.h>

namespace Multiplayer
{
    //! Implementation of the INetworkTime interface.
    class NetworkTime
        : public INetworkTime
    {
    public:
        NetworkTime();
        virtual ~NetworkTime();

        //! INetworkTime overrides.
        //! @{
        bool IsTimeRewound() const override;
        HostFrameId GetHostFrameId() const override;
        HostFrameId GetUnalteredHostFrameId() const override;
        void IncrementHostFrameId() override;
        AZ::TimeMs GetHostTimeMs() const override;
        AzNetworking::ConnectionId GetRewindingConnectionId() const override;
        HostFrameId GetHostFrameIdForRewindingConnection(AzNetworking::ConnectionId rewindConnectionId) const override;
        void AlterTime(HostFrameId frameId, AZ::TimeMs timeMs, AzNetworking::ConnectionId rewindConnectionId) override;
        void SyncEntitiesToRewindState(const AZ::Aabb& rewindVolume) override;
        void ClearRewoundEntities() override;
        //! @}

    private:

        HostFrameId m_hostFrameId = HostFrameId{ 0 };
        HostFrameId m_unalteredFrameId = HostFrameId{ 0 };
        AZ::TimeMs m_hostTimeMs = AZ::TimeMs{ 0 };
        AzNetworking::ConnectionId m_rewindingConnectionId = AzNetworking::InvalidConnectionId;
    };
}
