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
#include <Multiplayer/NetworkEntity/NetworkEntityUpdateMessage.h>

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
        //! Rebase records send the full replication state for Autonomous entities minus the predictable properties.
        //! These can be useful for client migrations.
        void SetRebasing();

        //! Set the publishing state to "deleting". The next record sent will be a delete record.
        //! Delete records will also send the final changed property states at the point of deletion.
        void SetDeleting();

        //! Returns true if the entity should be deleted, false if not. It will return true whether or not the delete has been acknowledged.
        bool IsDeleting() const;
        //! Returns true only if the entity delete has been acknlowledged.
        bool IsDeleted() const;

        //! Returns true if the receiver has acknowledged at least one serialized record, which means that it exists and has created
        //! the entity and an entity replicator.
        bool IsRemoteReplicatorEstablished() const;

        //! Generate and cache a "delete" update packet for this entity.
        //! For deletes, the entity data will no longer be available by the time GenerateUpdatePacket() is called,
        //! so the update packet needs to get generated before the local entity delete is completed.
        //! @return true if a delete packet was cached, false if it wasn't
        //! If the delete packet wasn't cached, it's typically because it is unnecessary, since the add was never sent.
        bool CacheDeletePacket(NetBindComponent* netBindComponent, bool wasMigrated);

        //! Generate a migration packet for this entity.
        //! Unlike GenerateUpdatePacket, this method expects that you have *not* called PrepareSerialization first.
        EntityMigrationMessage GenerateMigrationPacket(NetBindComponent* netBindComponent);

        //! Append an updated list of changed fields to the pending record.
        //! This can get called multiple times in a frame without harm, the changes will just keep accumulating.
        void UpdatePendingRecord(NetBindComponent* netBindComponent);

        //! Returns true if there are any changes pending to send, false if not.
        bool RequiresSerialization();

        //! Based on the current publishing state, append an updated list of changed fields to the pending record and
        //! add it to the sentRecords list.
        bool PrepareSerialization(NetBindComponent* netBindComponent);

        //! Generate an add/update/delete packet for this entity.
        //! This method expects that UpdatePendingRecord and PrepareSerialization have been called prior to this.
        NetworkEntityUpdateMessage GenerateUpdatePacket(NetBindComponent* netBindComponent, bool wasMigrated);

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

        //! Check if we have data to send.
        //! This will return true if there are any unacknowledged changes, even if they aren't new for this frame.
        bool HasEntityChangesToSend();

        //! Phase 1, setup of the record
        void PrepareFullReplicationEntityRecord(NetBindComponent* netBindComponent);
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

        // In the case of deletes, we need to produce our update message at the point of deletion
        // and then keep it around until it's requested. By the time the message is requested, the entity
        // is likely already deleted, so the data to serialize from it would no longer be available.
        NetworkEntityUpdateMessage m_cachedDeleteMessage;
    };
}
