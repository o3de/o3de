/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Multiplayer/Components/NetBindComponent.h>
#include <AzCore/std/containers/ring_buffer.h>

namespace AzNetworking
{
    class IConnection;
}

namespace Multiplayer
{
    class PropertyPublisher
    {
    public:
        enum class OwnsLifetime
        {
            True,
            False,
        };

        PropertyPublisher(NetEntityRole remoteNetworkRole, OwnsLifetime ownsLifetime, NetBindComponent* netBindComponent, AzNetworking::IConnection& connection);

        void SetRebasing();

        bool IsDeleting() const;
        bool IsDeleted() const;
        void SetDeleting();

        bool IsRemoteReplicatorEstablished() const;

        void GenerateRecord();

        //! Interface for ReplicationManager to manage serialization of entities
        //! @{
        bool RequiresSerialization();
        bool PrepareSerialization();
        bool UpdateSerialization(AzNetworking::ISerializer& serializer);
        void FinalizeSerialization(AzNetworking::PacketId sentId);
        //! @}

    private:
        enum class EntityReplicatorState
        {
            Invalid,  // Invalid state - do not use
            Creating, // Create an initial replication record, transitions to updating after that first packet
            Rebasing, // Create an initial replication record, without predictable values, transitions to updating after that first packet - used for client migrations
            Updating, // Create delta update packets based off what the remote endpoint has received
            Deleting, // Awaiting confirmation that the delete packet was received
        };

        enum class EntityReplicatorSerializationPhase
        {
            Ready,
            Prepared,
        };

        EntityReplicatorState GetReplicatorState() const;

        //! Check if we have data to send
        bool HasUpdateEntityRecord();

        //! Phase 1, setup of the record
        bool PrepareAddEntityRecord();
        bool PrepareRebaseEntityRecord();
        bool PrepareUpdateEntityRecord();
        bool PrepareDeleteEntityRecord();

        //! Phase 2, serialize the record
        //! No add, they share the update path
        bool SerializeUpdateEntityRecord(AzNetworking::ISerializer& serializer);
        bool SerializeDeleteEntityRecord(AzNetworking::ISerializer& serializer);

        //! Phase 3, finalize with the packet id
        void FinalizeUpdateEntityRecord(AzNetworking::PacketId packetId);
        void FinalizeDeleteEntityRecord(AzNetworking::PacketId packetId);

        EntityReplicatorState m_replicatorState = EntityReplicatorState::Creating;
        EntityReplicatorSerializationPhase m_serializationPhase = EntityReplicatorSerializationPhase::Ready;
        OwnsLifetime m_ownsLifetime = OwnsLifetime::False;
        AzNetworking::IConnection& m_connection;
        NetBindComponent* m_netBindComponent = nullptr;

        //! Aggregate changes that we need to serialize (m_currentRecord + outstanding m_sentRecords)
        ReplicationRecord m_pendingRecord;

        //! List of sent records (history of m_currentRecord)
        AZStd::ring_buffer<ReplicationRecord> m_sentRecords;
        AZStd::vector<AzNetworking::PacketId> m_deletePacketIds;
        bool m_remoteReplicatorEstablished = false;
    };
}
