/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/NetworkTime/INetworkTime.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
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
        float GetHostBlendFactor() const override;
        AzNetworking::ConnectionId GetRewindingConnectionId() const override;
        void ForceSetTime(HostFrameId frameId, AZ::TimeMs timeMs) override;
        void AlterTime(HostFrameId frameId, AZ::TimeMs timeMs, float blendFactor, AzNetworking::ConnectionId rewindConnectionId) override;
        void SyncEntitiesToRewindState(const AZ::Aabb& rewindVolume) override;
        void ClearRewoundEntities() override;
        //! @}

    private:

        AZStd::vector<NetworkEntityHandle> m_rewoundEntities;

        HostFrameId m_hostFrameId = HostFrameId{ 0 };
        HostFrameId m_unalteredFrameId = HostFrameId{ 0 };
        AZ::TimeMs m_hostTimeMs = AZ::TimeMs{ 0 };
        float m_hostBlendFactor = DefaultBlendFactor;
        AzNetworking::ConnectionId m_rewindingConnectionId = AzNetworking::InvalidConnectionId;
    };
}
