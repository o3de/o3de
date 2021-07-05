/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Time/ITime.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <Multiplayer/MultiplayerTypes.h>

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

        //! Syncs all entities contained within a volume to the current rewind state.
        //! @param rewindVolume the volume to rewind entities within (needed for physics entities)
        virtual void SyncEntitiesToRewindState(const AZ::Aabb& rewindVolume) = 0;

        //! Restores all rewound entities to the current application time.
        virtual void ClearRewoundEntities() = 0;

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

    // Convenience helpers
    inline INetworkTime* GetNetworkTime()
    {
        return AZ::Interface<INetworkTime>::Get();
    }
}
