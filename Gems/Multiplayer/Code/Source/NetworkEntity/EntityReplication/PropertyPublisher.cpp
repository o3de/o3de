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

#pragma optimize("", off)

namespace Multiplayer
{
    AZ_CVAR(uint32_t, net_EntityReplicatorRecordsMax, 45, nullptr, AZ::ConsoleFunctorFlags::Null, "Number of allowed outstanding entity records");

    PropertyPublisher::PropertyPublisher(NetEntityRole remoteNetworkRole, OwnsLifetime ownsLifetime, NetBindComponent* netBindComponent, AzNetworking::IConnection& connection)
        : m_ownsLifetime(ownsLifetime)
        , m_netBindComponent(netBindComponent)
        , m_connection(connection)
        , m_pendingRecord(remoteNetworkRole)
        , m_sentRecords(net_EntityReplicatorRecordsMax)
    {
        if ( ownsLifetime == OwnsLifetime::False )
        {
            // This entity is owned by some other authority; this publisher will only be used for updating (not creating).
            // Since this replicator does not own it's lifetime, the remote replicator must exist (otherwise, we would never have created a replicator that doesn't own its lifetime).
            m_remoteReplicatorEstablished = true;
            m_replicatorState = EntityReplicatorState::Updating;
        }

        AZ_Assert(m_netBindComponent, "NetBindComponent is nullptr");
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

    void PropertyPublisher::GenerateRecord()
    {
        AZ_Assert(m_netBindComponent, "NetBindComponent is nullptr");
        m_netBindComponent->FillReplicationRecord(m_pendingRecord);
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

    bool PropertyPublisher::PrepareAddEntityRecord()
    {
        // On an "Add", create a change record that contains all the serialized fields for the entity.
        m_sentRecords.clear();
        m_netBindComponent->FillTotalReplicationRecord(m_pendingRecord);
        m_sentRecords.push_front(m_pendingRecord);
        return true;
    }

    bool PropertyPublisher::PrepareRebaseEntityRecord()
    {
        AZ_Assert(m_netBindComponent, "NetBindComponent is nullptr");

        // This is basically an Add record, but we don't want to send back predictable values
        m_sentRecords.clear();
        m_netBindComponent->FillTotalReplicationRecord(m_pendingRecord);
        // Don't send predictable properties back to the Autonomous unless we correct them
        if (m_pendingRecord.GetRemoteNetworkRole() == NetEntityRole::Autonomous)
        {
            m_pendingRecord.Subtract(m_netBindComponent->GetPredictableRecord());
        }
        m_sentRecords.push_front(m_pendingRecord);
        return true;
    }

    bool PropertyPublisher::PrepareUpdateEntityRecord()
    {
        bool didPrepare = true;
        if (m_sentRecords.size() >= net_EntityReplicatorRecordsMax)
        {
            // If we reach the maximum outstanding records, reset the replication state by creating an "Add" record.
            didPrepare = PrepareAddEntityRecord();
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
            m_pendingRecord.Subtract(m_netBindComponent->GetPredictableRecord());
        }

        return didPrepare;
    }

    bool PropertyPublisher::PrepareDeleteEntityRecord()
    {
        if (IsDeleted())
        {
            // The delete has already been acknowledged, so there's nothing more to do.
            return false;
        }
        if (m_sentRecords.empty() && (!m_remoteReplicatorEstablished))
        {
            // If the entity add has never been sent (no sent records waiting for acknowledgement and
            // no acknowledged sends), then don't bother sending a delete.
            // If there are sent records, there might be an unacknowledged entity add that's been sent,
            // so we'll still need to send a delete record just in case.
            return false;
        }

        GenerateRecord();

        // A delete entity record looks the same as an update but will have an extra deletion flag on it.
        // This ensures that the replicated entity has correct and consistent state at the point of deletion.
        return PrepareUpdateEntityRecord();
    }

    bool PropertyPublisher::SerializeUpdateEntityRecord(AzNetworking::ISerializer &serializer)
    {
        AZ_Assert(m_netBindComponent, "NetBindComponent is nullptr");
        m_pendingRecord.ResetConsumedBits();
        m_pendingRecord.Serialize(serializer);
        m_netBindComponent->SerializeStateDeltaMessage(m_pendingRecord, serializer);
        return serializer.IsValid();
    }

    bool PropertyPublisher::SerializeDeleteEntityRecord(AzNetworking::ISerializer &serializer)
    {
        // On deletion, we still want to serialize any state deltas that exist.
        bool success = SerializeUpdateEntityRecord(serializer);
        return success;
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

    bool PropertyPublisher::PrepareSerialization()
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
                needsUpdate = PrepareAddEntityRecord();
            }
            m_replicatorState = PropertyPublisher::EntityReplicatorState::Updating;
            break;

        case PropertyPublisher::EntityReplicatorState::Rebasing:
            AZ_Assert(m_ownsLifetime == PropertyPublisher::OwnsLifetime::True, "Expected to own our lifetime if we rebase");
            needsUpdate = PrepareRebaseEntityRecord();
            m_replicatorState = PropertyPublisher::EntityReplicatorState::Updating;
            break;

        case PropertyPublisher::EntityReplicatorState::Updating:
            needsUpdate = PrepareUpdateEntityRecord();
            break;

        case PropertyPublisher::EntityReplicatorState::Deleting:
            if (m_ownsLifetime == PropertyPublisher::OwnsLifetime::True)
            {
                needsUpdate = PrepareDeleteEntityRecord();
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


    bool PropertyPublisher::UpdateSerialization(AzNetworking::ISerializer& serializer)
    {
        bool success(true);
        switch (m_replicatorState)
        {
        case PropertyPublisher::EntityReplicatorState::Invalid:
            AZ_Assert(false, "EntityReplicator: Initialize() was not called on this entity replicator");
            break;
        case PropertyPublisher::EntityReplicatorState::Creating:
        case PropertyPublisher::EntityReplicatorState::Updating:
        {
            AZ_Assert(m_serializationPhase == PropertyPublisher::EntityReplicatorSerializationPhase::Prepared, "Unexpected serialization phase");
            success = SerializeUpdateEntityRecord(serializer);
        }
        break;
        case PropertyPublisher::EntityReplicatorState::Deleting:
        {
            AZ_Assert(m_serializationPhase == PropertyPublisher::EntityReplicatorSerializationPhase::Prepared, "Unexpected serialization phase");
            success = SerializeDeleteEntityRecord(serializer);
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

#pragma optimize("", on)
