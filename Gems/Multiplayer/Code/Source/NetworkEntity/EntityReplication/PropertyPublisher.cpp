/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/NetworkEntity/EntityReplication/PropertyPublisher.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

namespace Multiplayer
{
    AZ_CVAR(uint32_t, net_EntityReplicatorRecordsMax, 45, nullptr, AZ::ConsoleFunctorFlags::Null, "Number of allowed outstanding entity records");

    PropertyPublisher::PropertyPublisher(NetEntityRole remoteNetworkRole, OwnsLifetime ownsLifetime, AzNetworking::IConnection& connection)
        : m_ownsLifetime(ownsLifetime)
        , m_connection(connection)
        , m_pendingRecord(remoteNetworkRole)
        , m_sentRecords(net_EntityReplicatorRecordsMax)
    {
        if ( ownsLifetime == OwnsLifetime::False )
        {
            // This entity is owned by some other authority; this publisher will only be used for updating (not creating).
            // Since this replicator does not own its lifetime, the remote replicator must exist
            // (otherwise, we would never have created a replicator that doesn't own its lifetime).
            m_remoteReplicatorEstablished = true;
            m_replicatorState = EntityReplicatorState::Updating;
        }

        m_pendingRecord.SetRemoteNetworkRole(remoteNetworkRole);
    }

    bool PropertyPublisher::IsDeleting() const
    {
        // This will return true once the delete message has been generated, both before and after acknowledgement.
        return (PropertyPublisher::EntityReplicatorState::Deleting == m_replicatorState);
    }

    bool PropertyPublisher::IsDeleted() const
    {
        bool result = false;
        for (AzNetworking::PacketId deletePacket : m_deletePacketIds)
        {
            if (m_connection.WasPacketAcked(deletePacket))
            {
                result = true;
                break;
            }
        }
        return result;
    }

    void PropertyPublisher::SetDeleting()
    {
        AZ_Assert(m_replicatorState != EntityReplicatorState::Deleting, "Attempting to delete the same entity twice.");
        m_replicatorState = EntityReplicatorState::Deleting;
    }

    bool PropertyPublisher::IsRemoteReplicatorEstablished() const
    {
        return m_remoteReplicatorEstablished;
    }

    PropertyPublisher::EntityReplicatorState PropertyPublisher::GetReplicatorState() const
    {
        return m_replicatorState;
    }

    void PropertyPublisher::SetRebasing()
    {
        AZ_Assert(m_pendingRecord.GetRemoteNetworkRole() == NetEntityRole::Autonomous, "Expected to be rebasing on a Autonomous entity");
        m_replicatorState = EntityReplicatorState::Rebasing;
    }

    void PropertyPublisher::GenerateRecord(NetBindComponent* netBindComponent)
    {
        AZ_Assert(netBindComponent, "NetBindComponent is nullptr");
        netBindComponent->FillReplicationRecord(m_pendingRecord);
    }

    bool PropertyPublisher::HasEntityChangesToSend()
    {
        auto mostRecentAckedIter = m_sentRecords.end();
        for (auto iter = m_sentRecords.begin(); iter != m_sentRecords.end(); ++iter)
        {
            // m_sentRecords is sorted from the most to the least recent sent changes, so once
            // the first acknowledged record is found, everything from that record and beyond in the list
            // is no longer necessary and can be deleted.
            if (m_connection.WasPacketAcked(iter->m_sentPacketId))
            {
                mostRecentAckedIter = iter;
                m_remoteReplicatorEstablished = true;
                break;
            }
        }

        // Delete all of the acknowledged records.
        m_sentRecords.erase(mostRecentAckedIter, m_sentRecords.end());

        // Nothing to send
        if (!m_pendingRecord.HasChanges() && m_sentRecords.empty() && m_remoteReplicatorEstablished)
        {
            return false;
        }

        // Still need to send a change record if there are pending changes or
        // if the remote replicator hasn't acknowledged a connection yet.
        return true;
    }

    bool PropertyPublisher::PrepareAddEntityRecord(NetBindComponent* netBindComponent)
    {
        AZ_Assert(netBindComponent, "NetBindComponent is nullptr");

        // On an "Add", create a change record that contains all the serialized fields for the entity.
        m_sentRecords.clear();
        netBindComponent->FillTotalReplicationRecord(m_pendingRecord);
        m_sentRecords.push_front(m_pendingRecord);
        return true;
    }

    bool PropertyPublisher::PrepareRebaseEntityRecord(NetBindComponent* netBindComponent)
    {
        AZ_Assert(netBindComponent, "NetBindComponent is nullptr");

        // This is basically an Add record, but we don't want to send back predictable values
        m_sentRecords.clear();
        netBindComponent->FillTotalReplicationRecord(m_pendingRecord);
        // Don't send predictable properties back to the Autonomous unless we correct them
        if (m_pendingRecord.GetRemoteNetworkRole() == NetEntityRole::Autonomous)
        {
            m_pendingRecord.Subtract(netBindComponent->GetPredictableRecord());
        }
        m_sentRecords.push_front(m_pendingRecord);
        return true;
    }

    bool PropertyPublisher::PrepareUpdateEntityRecord(NetBindComponent* netBindComponent)
    {
        bool didPrepare = true;
        if (m_sentRecords.size() >= net_EntityReplicatorRecordsMax)
        {
            // If we reach the maximum outstanding records, reset the replication state by creating an "Add" record.
            didPrepare = PrepareAddEntityRecord(netBindComponent);
        }
        else
        {
            // The update record consists of the pending record (new changes) merged together with everything else that has changed
            // since the last acked record (old changes). That way, the client can ignore any out-of-sequence records because the
            // later ones will always contain the necessary and latest information.
            m_sentRecords.push_front(m_pendingRecord);
            auto iter = m_sentRecords.begin();
            ++iter; // Consider everything after the record we are going to send
            for (; iter != m_sentRecords.end(); ++iter)
            {
                // Sequence wasn't acked, so we need to send these bits again
                m_pendingRecord.Append(*iter);
            }
        }

        // Don't send predictable properties back to the Autonomous unless we correct them
        if (m_pendingRecord.GetRemoteNetworkRole() == NetEntityRole::Autonomous)
        {
            m_pendingRecord.Subtract(netBindComponent->GetPredictableRecord());
        }

        return didPrepare;
    }

    bool PropertyPublisher::PrepareDeleteEntityRecord(NetBindComponent* netBindComponent)
    {
        if (IsDeleted())
        {
            // The delete has already been acknowledged, so there's nothing more to do.
            return false;
        }
        if (m_sentRecords.empty() && (!m_remoteReplicatorEstablished))
        {
            // If the entity add has never been sent (no sent records waiting for acknowledgement and
            // no acknowledged sends), then don't send a delete. It would cause the receiving end to create and delete
            // the entity in the same frame, which would be unnecessarily wasteful for both bandwidth and processing.

            // Note that if at least one record has been sent, even if it hasn't been acknowledged yet,
            // the entity creation might still get received, so in that case we'll still send a delete record.
            AZLOG(
                NET_RepDeletes,
                "Skipping delete replication for entity %llu because no create has been sent yet.",
                (AZ::u64)netBindComponent->GetNetEntityId());
            return false;
        }

        GenerateRecord(netBindComponent);

        // A delete entity record looks the same as an update but will have an extra deletion flag on it.
        // This ensures that the replicated entity has correct and consistent state at the point of deletion.
        return PrepareUpdateEntityRecord(netBindComponent);
    }

    bool PropertyPublisher::SerializeEntityRecord(AzNetworking::ISerializer& serializer, NetBindComponent* netBindComponent)
    {
        AZ_Assert(netBindComponent, "NetBindComponent is nullptr");
        m_pendingRecord.ResetConsumedBits();
        m_pendingRecord.Serialize(serializer);
        netBindComponent->SerializeStateDeltaMessage(m_pendingRecord, serializer);
        return serializer.IsValid();
    }

    void PropertyPublisher::FinalizeUpdateEntityRecord(AzNetworking::PacketId packetId)
    {
        // Fill in the packet id for the last sent update
        ReplicationRecord& lastSentRecord = m_sentRecords.front();
        AZ_Assert(lastSentRecord.m_sentPacketId == AzNetworking::InvalidPacketId, "Assumed we pushed on a packet in UpdateSerialization");
        lastSentRecord.m_sentPacketId = packetId;
        AZ_Assert(lastSentRecord.m_sentPacketId != AzNetworking::InvalidPacketId, "Got a bad packet id");
        if (lastSentRecord.m_sentPacketId == AzNetworking::InvalidPacketId)
        {
            // The packet failed to be generated, pop off the failed sent record
            m_sentRecords.pop_front();
            return;
        }
        m_pendingRecord.Clear();
    }

    void PropertyPublisher::FinalizeDeleteEntityRecord(AzNetworking::PacketId packetId)
    {
        // If we have more than our max records, just clear it and restart tracking again
        if (m_deletePacketIds.size() >= net_EntityReplicatorRecordsMax)
        {
            m_deletePacketIds.clear();
        }
        m_deletePacketIds.push_back(packetId);
    }

    bool PropertyPublisher::RequiresSerialization()
    {
        // Send our entity replication update

        switch (m_replicatorState)
        {
        case PropertyPublisher::EntityReplicatorState::Invalid:
            AZ_Assert(false, "EntityReplicator: Initialize() was not called on this entity replicator");
            return false;

        case PropertyPublisher::EntityReplicatorState::Creating:
        case PropertyPublisher::EntityReplicatorState::Rebasing:
            AZ_Assert(
                m_serializationPhase == PropertyPublisher::EntityReplicatorSerializationPhase::Ready, "Unexpected serialization phase");
            return true;

        case PropertyPublisher::EntityReplicatorState::Updating:
            AZ_Assert(
                m_serializationPhase == PropertyPublisher::EntityReplicatorSerializationPhase::Ready, "Unexpected serialization phase");
            return HasEntityChangesToSend();

        case PropertyPublisher::EntityReplicatorState::Deleting:
            if (m_ownsLifetime == PropertyPublisher::OwnsLifetime::True)
            {
                return !IsDeleted();
            }
            return false;

        default:
            AZ_Assert(false, "EntityReplicator: Unexpected state");
            return false;
        }
    }

    bool PropertyPublisher::PrepareSerialization(NetBindComponent* netBindComponent)
    {
        if (m_serializationPhase == PropertyPublisher::EntityReplicatorSerializationPhase::Prepared)
        {
            AZ_Assert(IsDeleting(), "We should only be in the Prepared phase for an entity that's being deleted.");
            return true;
        }

        // Send our entity replication update
        AZ_Assert(m_serializationPhase == PropertyPublisher::EntityReplicatorSerializationPhase::Ready, "Unexpected serialization phase");

        bool needsUpdate(false);
        switch (m_replicatorState)
        {
        case PropertyPublisher::EntityReplicatorState::Invalid:
            AZ_Assert(false, "EntityReplicator: Initialize() was not called on this entity replicator");
            break;

        case PropertyPublisher::EntityReplicatorState::Creating:
            if (m_ownsLifetime == PropertyPublisher::OwnsLifetime::True)
            {
                needsUpdate = PrepareAddEntityRecord(netBindComponent);
            }
            m_replicatorState = PropertyPublisher::EntityReplicatorState::Updating;
            break;

        case PropertyPublisher::EntityReplicatorState::Rebasing:
            AZ_Assert(m_ownsLifetime == PropertyPublisher::OwnsLifetime::True, "Expected to own our lifetime if we rebase");
            needsUpdate = PrepareRebaseEntityRecord(netBindComponent);
            m_replicatorState = PropertyPublisher::EntityReplicatorState::Updating;
            break;

        case PropertyPublisher::EntityReplicatorState::Updating:
            needsUpdate = PrepareUpdateEntityRecord(netBindComponent);
            break;

        case PropertyPublisher::EntityReplicatorState::Deleting:
            if (m_ownsLifetime == PropertyPublisher::OwnsLifetime::True)
            {
                needsUpdate = PrepareDeleteEntityRecord(netBindComponent);
            }
            break;

        default:
            AZ_Assert(false, "EntityReplicator: Unexpected state");
            break;
        }
        m_serializationPhase = needsUpdate ? PropertyPublisher::EntityReplicatorSerializationPhase::Prepared
                                           : PropertyPublisher::EntityReplicatorSerializationPhase::Ready;
        return needsUpdate;
    }


    bool PropertyPublisher::UpdateSerialization(AzNetworking::ISerializer& serializer, NetBindComponent* netBindComponent)
    {
        bool success(true);
        switch (m_replicatorState)
        {
        case PropertyPublisher::EntityReplicatorState::Invalid:
            AZ_Assert(false, "EntityReplicator: Initialize() was not called on this entity replicator");
            break;
        case PropertyPublisher::EntityReplicatorState::Creating:
        case PropertyPublisher::EntityReplicatorState::Updating:
        case PropertyPublisher::EntityReplicatorState::Deleting:
        {
            AZ_Assert(m_serializationPhase == PropertyPublisher::EntityReplicatorSerializationPhase::Prepared, "Unexpected serialization phase");
            success = SerializeEntityRecord(serializer, netBindComponent);
        }
        break;
        default:
            AZ_Assert(false, "EntityReplicator: Unexpected state");
            break;
        }
        if (!success)
        {
            AZLOG_ERROR("EntityReplicator: Serialization failed");
        }
        AZ_Assert(success, "EntityReplicator: Serialization failed");
        return success;
    }

    void PropertyPublisher::FinalizeSerialization(AzNetworking::PacketId sentId)
    {
        switch (m_replicatorState)
        {
        case PropertyPublisher::EntityReplicatorState::Invalid:
            AZ_Assert(false, "EntityReplicator: Initialize() was not called on this entity replicator");
            break;
        case PropertyPublisher::EntityReplicatorState::Creating:
        case PropertyPublisher::EntityReplicatorState::Updating:
        {
            AZ_Assert(m_serializationPhase == PropertyPublisher::EntityReplicatorSerializationPhase::Prepared, "Unexpected serialization phase");
            FinalizeUpdateEntityRecord(sentId);
            m_replicatorState = PropertyPublisher::EntityReplicatorState::Updating;
        }
        break;
        case PropertyPublisher::EntityReplicatorState::Deleting:
        {
            FinalizeDeleteEntityRecord(sentId);
        }
        break;
        default:
            AZ_Assert(false, "EntityReplicator: Unexpected state");
            break;
        }
        // Reset our state for the next frame
        m_serializationPhase = PropertyPublisher::EntityReplicatorSerializationPhase::Ready;
    }
}
