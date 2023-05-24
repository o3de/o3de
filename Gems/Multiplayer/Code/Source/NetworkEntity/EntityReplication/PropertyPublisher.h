/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    //! @class PropertyPublisher
    //! @brief Private helper class for the EntityReplicator to serialize and track entity adds/updates/deletes.
    //! The EntityReplicator owns the actual sending of the records.
    //! This class tracks the acknowledgement state of all the published records so that it can aggregate and reserialize
    //! changes for any updates or deletes that occur before previous records have been acknowledged.
    //! This also tracks whether or not it has *ever* received an acknowledgement and stores it in IsRemoteReplicatorEstablished
    //! as a way to know if the receiver has created the replicated entity.
    class PropertyPublisher
    {
    public:
        enum class OwnsLifetime
        {
            True,
            False,
        };

        PropertyPublisher(NetEntityRole remoteNetworkRole, OwnsLifetime ownsLifetime, AzNetworking::IConnection& connection);

        //! Set the publishing state to "rebasing". The next record sent will be a rebase record.
        void SetRebasing();
        //! Set the publishing state to "deleting". The next record sent will be a delete record.
        void SetDeleting();

        //! Returns true if the entity should be deleted, false if not. It will return true whether or not the delete has been acknowledged.
        bool IsDeleting() const;
        //! Returns true only if the entity delete has been acknlowledged.
        bool IsDeleted() const;

        //! Returns true if the receiver has acknowledged at least one serialized record, which means that it exists and has created
        //! the entity and an entity replicator.
        bool IsRemoteReplicatorEstablished() const;

        //! Append an updated list of changed fields to the pending record.
        void GenerateRecord(NetBindComponent* netBindComponent);

        //! Returns true if there are any changes pending to send, false if not.
        bool RequiresSerialization();

        //! Based on the current publishing state, append an updated list of changed fields to the pending record and
        //! add it to the sentRecords list.
        bool PrepareSerialization(NetBindComponent* netBindComponent);

        //! Serialize the entity state into the given serializer based on the pending record's list of changed fields.
        bool UpdateSerialization(AzNetworking::ISerializer& serializer, NetBindComponent* netBindComponent);

        //! Track the given packet id so that we can continue to send any fields currently changed until this packet
        //! (or later) has been acknowledged.
        void FinalizeSerialization(AzNetworking::PacketId sentId);

    private:
        enum class EntityReplicatorState
        {
            Invalid,  // Invalid state - do not use
            Creating, // Create an initial replication record, transitions to updating after that first packet
            Rebasing, // Create an initial replication record, without predictable values, transitions to updating after that first packet - used for client migrations
            Updating, // Create delta update packets based off what the remote endpoint has received
            Deleting, // Create delete packets containing the final delta update
        };

        enum class EntityReplicatorSerializationPhase
        {
            Ready,
            Prepared,
        };

        EntityReplicatorState GetReplicatorState() const;

        //! Check if we have data to send
        bool HasEntityChangesToSend();

        //! Phase 1, setup of the record
        void PrepareAddEntityRecord(NetBindComponent* netBindComponent);
        void PrepareRebaseEntityRecord(NetBindComponent* netBindComponent);
        void PrepareUpdateEntityRecord(NetBindComponent* netBindComponent);
        void PrepareDeleteEntityRecord(NetBindComponent* netBindComponent);

        //! Phase 2, serialize the record
        //! Add/update/delete all use the same serialization path.
        bool SerializeEntityRecord(AzNetworking::ISerializer& serializer, NetBindComponent* netBindComponent);

        //! Phase 3, finalize with the packet id
        void FinalizeUpdateEntityRecord(AzNetworking::PacketId packetId);
        void FinalizeDeleteEntityRecord(AzNetworking::PacketId packetId);

        //! The current state of entity replication - add / rebase / update / delete
        EntityReplicatorState m_replicatorState = EntityReplicatorState::Creating;
        //! Tracks whether the record needs to be prepared, serialized, or finalized.
        EntityReplicatorSerializationPhase m_serializationPhase = EntityReplicatorSerializationPhase::Ready;
        //! True if this process owns the entity lifetime, false if not.
        OwnsLifetime m_ownsLifetime = OwnsLifetime::False;
        //! Reference to the connection, used for checking for packet acknowledgements.
        AzNetworking::IConnection& m_connection;

        //! Aggregate changes that we need to serialize (currentRecord + outstanding m_sentRecords)
        ReplicationRecord m_pendingRecord;

        //! List of sent records
        AZStd::ring_buffer<ReplicationRecord> m_sentRecords;
        //! List of sent delete packets, tracked separately as a way to look for acknowledged deletes.
        //! (This could potentially get merged into m_sentRecords as an optimization)
        AZStd::vector<AzNetworking::PacketId> m_deletePacketIds;
        //! True if the remote replicator has acknowledged at least one packet, which means that it exists and created the entity.
        bool m_remoteReplicatorEstablished = false;
    };
}
