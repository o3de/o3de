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
#include <Multiplayer/IMultiplayer.h>

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

    void PropertyPublisher::UpdatePendingRecord(NetBindComponent* netBindComponent)
    {
        // Only update the pending record if we don't have a cached delete message already.
        if (!m_cachedDeleteMessage.GetIsDelete())
        {
            AZ_Assert(netBindComponent, "NetBindComponent is nullptr");
            netBindComponent->FillReplicationRecord(m_pendingRecord);
        }
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

    void PropertyPublisher::PrepareFullReplicationEntityRecord(NetBindComponent* netBindComponent)
    {
        AZ_Assert(netBindComponent, "NetBindComponent is nullptr");

        // On initial entity replication or after too many unacknowledged updates,
        // create a change record that contains all the serialized fields for the entity.
        m_sentRecords.clear();
        netBindComponent->FillTotalReplicationRecord(m_pendingRecord);
        m_sentRecords.push_front(m_pendingRecord);
    }

    void PropertyPublisher::PrepareRebaseEntityRecord(NetBindComponent* netBindComponent)
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
    }

    void PropertyPublisher::PrepareUpdateEntityRecord(NetBindComponent* netBindComponent)
    {
        if (m_sentRecords.size() >= net_EntityReplicatorRecordsMax)
        {
            // If we reach the maximum outstanding records, reset the replication state by creating a change record
            // that serializes the state of *all* the networked properties. This is necessary because we'll no longer
            // have accurate bookkeeping on which sent changes have been acknowledged.
            PrepareFullReplicationEntityRecord(netBindComponent);
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
    }

    void PropertyPublisher::PrepareDeleteEntityRecord(NetBindComponent* netBindComponent)
    {
        // Once the delete message is cached, there's nothing more that needs to be prepared.
        if (!m_cachedDeleteMessage.GetIsDelete())
        {
            UpdatePendingRecord(netBindComponent);

            // A delete entity record looks the same as an update but will have an extra deletion flag on it.
            // This ensures that the replicated entity has correct and consistent state at the point of deletion.
            PrepareUpdateEntityRecord(netBindComponent);
        }
    }

    bool PropertyPublisher::SerializeEntityRecord(AzNetworking::ISerializer& serializer, NetBindComponent* netBindComponent)
    {
        AZ_Assert(
            m_replicatorState != PropertyPublisher::EntityReplicatorState::Invalid,
            "EntityReplicator: Initialize() was not called on this entity replicator");
        AZ_Assert(netBindComponent, "NetBindComponent is nullptr");
        m_pendingRecord.ResetConsumedBits();
        m_pendingRecord.Serialize(serializer);
        netBindComponent->SerializeStateDeltaMessage(m_pendingRecord, serializer);
        if (!serializer.IsValid())
        {
            AZLOG_ERROR("EntityReplicator: Serialization failed");
            AZ_Assert(false, "EntityReplicator: Serialization failed");
        }
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
                    return false;
                }

                return true;
            }

            // Never send a delete for an entity that we don't own the lifetime for.
            return false;

        default:
            AZ_Assert(false, "EntityReplicator: Unexpected state");
            return false;
        }
    }

    bool PropertyPublisher::PrepareSerialization(NetBindComponent* netBindComponent)
    {
        // The publisher should always be in the "Ready" phase at the point that we prepare for serialization.
        AZ_Assert(m_serializationPhase == PropertyPublisher::EntityReplicatorSerializationPhase::Ready, "Unexpected serialization phase");

        // If there are no unacknowledged changes, there's nothing to do.
        if (RequiresSerialization() == false)
        {
            // Nothing was prepared.
            return false;
        }

        // There are unacknowledged changes, so prepare the proper type of entity record.
        switch (m_replicatorState)
        {
        case PropertyPublisher::EntityReplicatorState::Invalid:
            AZ_Assert(false, "EntityReplicator: Initialize() was not called on this entity replicator");
            break;

        case PropertyPublisher::EntityReplicatorState::Creating:
            if (m_ownsLifetime == PropertyPublisher::OwnsLifetime::True)
            {
                PrepareFullReplicationEntityRecord(netBindComponent);
            }
            // After the first create, transition to updating.
            m_replicatorState = PropertyPublisher::EntityReplicatorState::Updating;
            break;

        case PropertyPublisher::EntityReplicatorState::Rebasing:
            AZ_Assert(m_ownsLifetime == PropertyPublisher::OwnsLifetime::True, "Expected to own our lifetime if we rebase");
            PrepareRebaseEntityRecord(netBindComponent);
            // After a rebase, transition to updating.
            m_replicatorState = PropertyPublisher::EntityReplicatorState::Updating;
            break;

        case PropertyPublisher::EntityReplicatorState::Updating:
            PrepareUpdateEntityRecord(netBindComponent);
            break;

        case PropertyPublisher::EntityReplicatorState::Deleting:
            AZ_Assert(m_ownsLifetime == PropertyPublisher::OwnsLifetime::True, "Expected to own our lifetime if we delete");
            PrepareDeleteEntityRecord(netBindComponent);
            break;

        default:
            AZ_Assert(false, "EntityReplicator: Unexpected state");
            break;
        }

        // We've prepared the record, so change our phrase and return true that we have changes to send.
        m_serializationPhase = PropertyPublisher::EntityReplicatorSerializationPhase::Prepared;
        return true;
    }

    bool PropertyPublisher::CacheDeletePacket(NetBindComponent* netBindComponent, bool wasMigrated)
    {
        bool cacheDelete = PrepareSerialization(netBindComponent);
        if (cacheDelete)
        {
            AZ_Assert(!m_cachedDeleteMessage.GetIsDelete(), "Double-creating the cached delete message.");
            m_cachedDeleteMessage = GenerateUpdatePacket(netBindComponent, wasMigrated);
            AZ_Assert(m_cachedDeleteMessage.GetIsDelete(), "Cached delete message wasn't created successfully.");

            // We can't call "Finalize" because no packet was sent, so we'll just manually set the phase back to "Ready".
            m_serializationPhase = PropertyPublisher::EntityReplicatorSerializationPhase::Ready;
        }
        return cacheDelete;
    }

    NetworkEntityUpdateMessage PropertyPublisher::GenerateUpdatePacket(NetBindComponent* netBindComponent, bool wasMigrated)
    {
        const bool sendPrefabId = !IsRemoteReplicatorEstablished();

        // If the remote replicator is not established, we need to take ownership of the entity
        const bool isDeleted = IsDeleting() && (m_ownsLifetime == PropertyPublisher::OwnsLifetime::True);

        if (isDeleted && m_cachedDeleteMessage.GetIsDelete())
        {
            return m_cachedDeleteMessage;
        }

        NetworkEntityUpdateMessage updateMessage(
            m_pendingRecord.GetRemoteNetworkRole(), netBindComponent->GetNetEntityId(), isDeleted, wasMigrated);

        // Only set the prefab id if the remote replicator hasn't been established yet. Once the remote replicator has been established
        // it has received a copy of the prefab id. Sending it again would be redundant and wasted bandwidth since it doesn't change
        // over the entity's lifetime.
        if (sendPrefabId)
        {
            updateMessage.SetPrefabEntityId(netBindComponent->GetPrefabEntityId());
        }

        InputSerializer inputSerializer(
            updateMessage.ModifyData().GetBuffer(), static_cast<uint32_t>(updateMessage.ModifyData().GetCapacity()));
        SerializeEntityRecord(inputSerializer, netBindComponent);
        updateMessage.ModifyData().Resize(inputSerializer.GetSize());

        return updateMessage;
    }

    EntityMigrationMessage PropertyPublisher::GenerateMigrationPacket(NetBindComponent* netBindComponent)
    {
        AZ_Assert(netBindComponent, "Trying to migrate when NetBindComponent is null.");
        AZ_Assert(!IsDeleting(), "Trying to migrate a deleted entity");

        EntityMigrationMessage message;
        message.m_netEntityId = netBindComponent->GetNetEntityId();
        message.m_prefabEntityId = netBindComponent->GetPrefabEntityId();

        // Send an update packet if it needs one
        UpdatePendingRecord(netBindComponent);
        bool needsNetworkPropertyUpdate = PrepareSerialization(netBindComponent);
        InputSerializer inputSerializer(
            message.m_propertyUpdateData.GetBuffer(), static_cast<uint32_t>(message.m_propertyUpdateData.GetCapacity()));
        if (needsNetworkPropertyUpdate)
        {
            // Write out entity state into the buffer
            SerializeEntityRecord(inputSerializer, netBindComponent);
        }
        AZ_Assert(inputSerializer.IsValid(), "Failed to migrate entity from server");
        message.m_propertyUpdateData.Resize(inputSerializer.GetSize());

        return message;
    }

    void PropertyPublisher::FinalizeSerialization(AzNetworking::PacketId sentId)
    {
        AZ_Assert(
            m_serializationPhase == PropertyPublisher::EntityReplicatorSerializationPhase::Prepared, "Unexpected serialization phase");

        switch (m_replicatorState)
        {
        case PropertyPublisher::EntityReplicatorState::Invalid:
            AZ_Assert(false, "EntityReplicator: Initialize() was not called on this entity replicator");
            break;
        case PropertyPublisher::EntityReplicatorState::Creating:
        case PropertyPublisher::EntityReplicatorState::Updating:
        {
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
