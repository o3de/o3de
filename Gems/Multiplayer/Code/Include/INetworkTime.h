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

#include <AzCore/Time/ITime.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <Include/MultiplayerTypes.h>

namespace Multiplayer
{
    //! @class INetworkTime
    //! @brief This is an AZ::Interface<> for managing multiplayer specific time related operations.
    class INetworkTime
    {
    public:
        AZ_RTTI(INetworkTime, "{7D468063-255B-4FEE-86E1-6D750EEDD42A}");

        INetworkTime() = default;
        virtual ~INetworkTime() = default;

        //! Returns true if the host timeMs and frameId has been temporarily altered.
        //! @return true if the host timeMs and frameId has been altered, false otherwise
        virtual bool IsTimeRewound() const = 0;

        //! Retrieves the hosts current frameId (may be rewound on the server during backward reconciliation).
        //! @return the hosts current frameId
        virtual HostFrameId GetHostFrameId() const = 0;

        //! Retrieves the unaltered hosts current frameId.
        //! @return the hosts current frameId, unaltered by any scoped time instance
        virtual HostFrameId GetUnalteredHostFrameId() const = 0;

        //! Increments the hosts current frameId.
        virtual void IncrementHostFrameId() = 0;

        //! Retrieves the hosts current timeMs (may be rewound on the server during backward reconciliation).
        //! @return the hosts current timeMs
        virtual AZ::TimeMs GetHostTimeMs() const = 0;

        //! Synchronizes rewindable entity state for the current application time.
        virtual void SyncRewindableEntityState() = 0;

        //! Get the controlling connection that may be currently altering global game time.
        //! Note this abstraction is required at a relatively high level to allow for 'don't rewind the shooter' semantics
        //! @return the ConnectionId of the connection requesting the rewind operation
        virtual AzNetworking::ConnectionId GetRewindingConnectionId() const = 0;

        //! Get the controlling connection that may be currently altering global game time.
        //! Note this abstraction is required at a relatively high level to allow for 'don't rewind the shooter' semantics
        //! @param rewindConnectionId if this parameter matches the current rewindConnectionId, it will return the unaltered hostFrameId
        //! @return the HostFrameId taking into account the provided rewinding connectionId
        virtual HostFrameId GetHostFrameIdForRewindingConnection(AzNetworking::ConnectionId rewindConnectionId) const = 0;

        //! Alters the current HostFrameId and binds that alteration to the provided ConnectionId.
        //! @param frameId the new HostFrameId to use
        //! @param timeMs the new HostTimeMs to use
        //! @param rewindConnectionId the rewinding ConnectionId 
        virtual void AlterTime(HostFrameId frameId, AZ::TimeMs timeMs, AzNetworking::ConnectionId rewindConnectionId) = 0;

        AZ_DISABLE_COPY_MOVE(INetworkTime);
    };

    // EBus wrapper for ScriptCanvas
    class INetworkTimeRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
    };
    using INetworkTimeRequestBus = AZ::EBus<INetworkTime, INetworkTimeRequests>;

    //! @class ScopedAlterTime
    //! @brief This is a wrapper that temporarily adjusts global program time for backward reconciliation purposes.
    class ScopedAlterTime final
    {
    public:
        inline ScopedAlterTime(HostFrameId frameId, AZ::TimeMs timeMs, AzNetworking::ConnectionId connectionId)
        {
            INetworkTime* time = AZ::Interface<INetworkTime>::Get();
            m_previousHostFrameId = time->GetHostFrameId();
            m_previousHostTimeMs = time->GetHostTimeMs();
            m_previousRewindConnectionId = time->GetRewindingConnectionId();
            time->AlterTime(frameId, timeMs, connectionId);
        }
        inline ~ScopedAlterTime()
        {
            INetworkTime* time = AZ::Interface<INetworkTime>::Get();
            time->AlterTime(m_previousHostFrameId, m_previousHostTimeMs, m_previousRewindConnectionId);
        }
    private:
        HostFrameId m_previousHostFrameId = InvalidHostFrameId;
        AZ::TimeMs m_previousHostTimeMs = AZ::TimeMs{ 0 };
        AzNetworking::ConnectionId m_previousRewindConnectionId = AzNetworking::InvalidConnectionId;
    };
}
