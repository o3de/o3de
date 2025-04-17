/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/MultiplayerTypes.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <Multiplayer/NetworkEntity/NetworkEntityRpcMessage.h>
#include <Multiplayer/NetworkEntity/NetworkEntityUpdateMessage.h>
#include <AzCore/std/containers/map.h>

namespace Multiplayer
{
    class EntityReplicator;

    struct EntityReplicationData 
    {
        EntityReplicationData() = default;
        NetEntityRole m_netEntityRole = NetEntityRole::InvalidRole;
        float m_priority = 0.0f;
    };
    using ReplicationSet = AZStd::map<ConstNetworkEntityHandle, EntityReplicationData>;
    using RpcMessages = AZStd::list<NetworkEntityRpcMessage>;
    using EntityReplicatorList = AZStd::deque<EntityReplicator*>;

    class IReplicationWindow
    {
    public:
        virtual ~IReplicationWindow() = default;

        //! Queries whether or not the replication window is in a state capable of sending entity update messages.
        //! @return boolean true if the replication window is ready and in a valid state, false otherwise
        virtual bool ReplicationSetUpdateReady() = 0;

        //! Returns the set of entities, roles, and priorities marked for replication by this replication window.
        //! @return const reference to the replication windows replication set
        virtual const ReplicationSet& GetReplicationSet() const = 0;

        //! Max number of entities we can send updates for in one frame.
        //! @return the max number of entities we can send updates for in one frame
        virtual uint32_t GetMaxProxyEntityReplicatorSendCount() const = 0;

        //! Returns true if the provided network entity is within this replication window.
        //! @param entityPtr the handle of the entity to test for inclusion
        //! @param outNetworkRole output containing the network role of the requested entity if found
        virtual bool IsInWindow(const ConstNetworkEntityHandle& entityPtr, NetEntityRole& outNetworkRole) const = 0;

        //! Adds an entity to the replication window's set
        //! @param entity The entity to try adding
        //! @return Whether the entity was able to be added
        virtual bool AddEntity(AZ::Entity* entity) = 0;

        //! Removes an entity from the replication window's set, if present
        //! @param entity The entity to remove
        virtual void RemoveEntity(AZ::Entity* entity) = 0;

        //! This updates the replication set, ensuring all relevant entities are included.
        virtual void UpdateWindow() = 0;

        //! This sends an EntityUpdate message on the associated network interface and connection.
        //! @param entityUpdateVector set of entity updates
        //! @return the packetId of the sent update message, or InvalidPacketId in the case of failure
        virtual AzNetworking::PacketId SendEntityUpdateMessages(NetworkEntityUpdateVector& entityUpdateVector) = 0;

        //! This sends an EntityRpcs message on the associated network interface and connection.
        //! @param entityRpcVector the rpc data set to transmit
        //! @param reliable if a value of true is passed, the rpc message will be sent reliably, unreliably if false
        virtual void SendEntityRpcs(NetworkEntityRpcVector& entityRpcVector, bool reliable) = 0;

        //! This sends an EntityReset message on the associated network interface and connection.
        //! This will reset the replicators on the remote endpoint and cause a full refresh of the specified entities
        //! @param resetIds the set of netEntityIds to refresh
        virtual void SendEntityResets(const NetEntityIdSet& resetIds) = 0;

        //! This causes the replication window to perform debug-draw overlays.
        virtual void DebugDraw() const = 0;
    };
}
