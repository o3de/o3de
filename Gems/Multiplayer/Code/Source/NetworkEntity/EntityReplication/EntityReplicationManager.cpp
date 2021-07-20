/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/NetworkEntity/EntityReplication/EntityReplicationManager.h>
#include <Source/NetworkEntity/EntityReplication/EntityReplicator.h>
#include <Source/NetworkEntity/EntityReplication/PropertyPublisher.h>
#include <Source/NetworkEntity/EntityReplication/PropertySubscriber.h>
#include <Source/AutoGen/Multiplayer.AutoPackets.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/EntityDomains/IEntityDomain.h>
#include <Multiplayer/NetworkEntity/INetworkEntityManager.h>
#include <Multiplayer/NetworkEntity/NetworkEntityUpdateMessage.h>
#include <Multiplayer/NetworkEntity/NetworkEntityRpcMessage.h>
#include <Multiplayer/ReplicationWindows/IReplicationWindow.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <AzNetworking/PacketLayer/IPacketHeader.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzNetworking/Serialization/TrackChangedSerializer.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Math/Transform.h>

namespace Multiplayer
{
    // Current max size for a UdpPacketHeader is 11 bytes
    constexpr uint32_t UdpPacketHeaderSerializeSize = 12;
    // Take out a few extra bytes for special headers, we currently only use 1 byte for the count of entity updates
    constexpr uint32_t ReplicationManagerPacketOverhead = 16;

    AZ_CVAR(bool, bg_replicationWindowImmediateAddRemove, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Update replication windows immediately on visibility Add/Removes.");

    EntityReplicationManager::EntityReplicationManager(AzNetworking::IConnection& connection, AzNetworking::IConnectionListener& connectionListener, Mode updateMode)
        : m_updateMode(updateMode)
        , m_connection(connection)
        , m_connectionListener(connectionListener)
        , m_orphanedEntityRpcs(*this)
        , m_clearRemovedReplicators([this]() { ClearRemovedReplicators(); }, AZ::Name("EntityReplicationManager::ClearRemovedReplicators"))
        , m_updateWindow([this]() { UpdateWindow(); }, AZ::Name("EntityReplicationManager::UpdateWindow"))
        , m_entityExitDomainEventHandler([this](const ConstNetworkEntityHandle& entityHandle) { OnEntityExitDomain(entityHandle); })
    {
        // Our max payload size is whatever is passed in, minus room for a udp packetheader
        m_maxPayloadSize = connection.GetConnectionMtu() - UdpPacketHeaderSerializeSize - ReplicationManagerPacketOverhead;

        // Schedule ClearRemovedReplicators()
        m_clearRemovedReplicators.Enqueue(AZ::TimeMs{ 0 }, true);

        // Start window update events
        m_updateWindow.Enqueue(AZ::TimeMs{ 0 }, true);

        INetworkEntityManager* networkEntityManager = GetNetworkEntityManager();
        if (networkEntityManager != nullptr)
        {
            networkEntityManager->AddEntityExitDomainHandler(m_entityExitDomainEventHandler);
        }
    }

    void EntityReplicationManager::SetRemoteHostId(HostId hostId)
    {
        m_remoteHostId = hostId;
    }

    HostId EntityReplicationManager::GetRemoteHostId() const
    {
        return m_remoteHostId;
    }

    void EntityReplicationManager::ActivatePendingEntities()
    {
        const AZ::TimeMs endTimeMs = AZ::GetElapsedTimeMs() + m_entityActivationTimeSliceMs;
        while (!m_entitiesPendingActivation.empty())
        {
            NetEntityId entityId = m_entitiesPendingActivation.front();
            m_entitiesPendingActivation.pop_front();
            EntityReplicator* entityReplicator = GetEntityReplicator(entityId);
            if (entityReplicator && !entityReplicator->IsMarkedForRemoval())
            {
                entityReplicator->ActivateNetworkEntity();
            }
            if (m_entityActivationTimeSliceMs > AZ::TimeMs{ 0 } && AZ::GetElapsedTimeMs() > endTimeMs)
            {
                // If we go over our timeslice, break out the loop
                break;
            }
        }
    }

    void EntityReplicationManager::SendUpdates(AZ::TimeMs hostTimeMs)
    {
        m_frameTimeMs = AZ::GetElapsedTimeMs();
        SendEntityUpdates(hostTimeMs);

        SendEntityRpcs(m_deferredRpcMessagesReliable, true);
        SendEntityRpcs(m_deferredRpcMessagesUnreliable, false);

        m_orphanedEntityRpcs.Update();

        AZLOG
        (
            NET_ReplicationInfo,
            "Sending from %u to %u, replicator count %u orphan count %u deferred reliable count %u deferred unreliable count %u",
            aznumeric_cast<uint32_t>(GetNetworkEntityManager()->GetHostId()),
            aznumeric_cast<uint32_t>(GetRemoteHostId()),
            aznumeric_cast<uint32_t>(m_entityReplicatorMap.size()),
            aznumeric_cast<uint32_t>(m_orphanedEntityRpcs.Size()),
            aznumeric_cast<uint32_t>(m_deferredRpcMessagesReliable.size()),
            aznumeric_cast<uint32_t>(m_deferredRpcMessagesUnreliable.size())
        );
    }

    void EntityReplicationManager::SendEntityUpdatesPacketHelper
    (
        AZ::TimeMs hostTimeMs,
        EntityReplicatorList& toSendList,
        uint32_t maxPayloadSize,
        AzNetworking::IConnection& connection
    )
    {
        uint32_t pendingPacketSize = 0;
        EntityReplicatorList replicatorUpdatedList;
        MultiplayerPackets::EntityUpdates entityUpdatePacket;
        entityUpdatePacket.SetHostTimeMs(hostTimeMs);
        entityUpdatePacket.SetHostFrameId(GetNetworkTime()->GetHostFrameId());
        // Serialize everything
        while (!toSendList.empty())
        {
            EntityReplicator* replicator = toSendList.front();
            NetworkEntityUpdateMessage updateMessage(replicator->GenerateUpdatePacket());

            const uint32_t nextMessageSize = updateMessage.GetEstimatedSerializeSize();

            // Check if we are over our limits
            const bool payloadFull = (pendingPacketSize + nextMessageSize > maxPayloadSize);
            const bool capacityReached = (entityUpdatePacket.GetEntityMessages().size() >= entityUpdatePacket.GetEntityMessages().capacity());
            const bool largeEntityDetected = (payloadFull && replicatorUpdatedList.empty());
            if (capacityReached || (payloadFull && !largeEntityDetected))
            {
                break;
            }

            pendingPacketSize += nextMessageSize;
            entityUpdatePacket.ModifyEntityMessages().push_back(updateMessage);
            replicatorUpdatedList.push_back(replicator);
            toSendList.pop_front();

            if (largeEntityDetected)
            {
                AZLOG_WARN("\n\n*******************************");
                AZLOG_WARN
                (
                    "Serializing extremely large entity (%u) - MaxPayload: %d NeededSize %d",
                    aznumeric_cast<uint32_t>(replicator->GetEntityHandle().GetNetEntityId()),
                    maxPayloadSize,
                    nextMessageSize
                );
                AZLOG_WARN("*******************************");
                break;
            }
        }

        const AzNetworking::PacketId sentId = connection.SendUnreliablePacket(entityUpdatePacket);

        // Update the sent things with the packet id
        for (EntityReplicator* replicator : replicatorUpdatedList)
        {
            replicator->GetPropertyPublisher()->FinalizeSerialization(sentId);
        }
    }

    EntityReplicationManager::EntityReplicatorList EntityReplicationManager::GenerateEntityUpdateList()
    {
        if (m_replicationWindow == nullptr)
        {
            return EntityReplicatorList();
        }

        // Generate a list of all our entities that need updates
        EntityReplicatorList toSendList;

        uint32_t elementsAdded = 0;
        for (auto iter = m_replicatorsPendingSend.begin(); iter != m_replicatorsPendingSend.end() && elementsAdded < m_replicationWindow->GetMaxEntityReplicatorSendCount(); )
        {
            EntityReplicator* replicator = GetEntityReplicator(*iter);
            bool clearPendingSend = true;
            if (replicator)
            {
                NetEntityId entityId = replicator->GetEntityHandle().GetNetEntityId();
                PropertyPublisher* propPublisher = replicator->GetPropertyPublisher();
                if (propPublisher)
                {
                    // don't have too many replicators pending creation outstanding at a time
                    bool canSend = true;
                    if (!propPublisher->IsRemoteReplicatorEstablished())
                    {
                        // If we have our maximum set of entities pending creation, and this entity isn't in that set, then skip it
                        if ((m_remoteEntitiesPendingCreation.size() >= m_maxRemoteEntitiesPendingCreationCount) && (m_remoteEntitiesPendingCreation.find(entityId) == m_remoteEntitiesPendingCreation.end()))
                        {
                            canSend = false; // don't send this
                            clearPendingSend = false;  // there might be outstanding data here, but we won't check, so we shouldn't clear it
                        }
                    }
                    else
                    {
                        m_remoteEntitiesPendingCreation.erase(*iter);
                    }

                    if (canSend && propPublisher->RequiresSerialization())
                    {
                        clearPendingSend = false;
                        if (!propPublisher->IsRemoteReplicatorEstablished())
                        {
                            m_remoteEntitiesPendingCreation.insert(entityId);
                        }

                        if (replicator->GetRemoteNetworkRole() == NetEntityRole::Autonomous)
                        {
                            toSendList.push_back(replicator);
                        }
                        else
                        {
                            if (elementsAdded < m_replicationWindow->GetMaxEntityReplicatorSendCount())
                            {
                                toSendList.push_back(replicator);
                            }
                        }
                    }
                    ++elementsAdded;
                }
            }

            if (clearPendingSend)
            {
                m_remoteEntitiesPendingCreation.erase(*iter);
                iter = m_replicatorsPendingSend.erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        return toSendList;
    }

    void EntityReplicationManager::SendEntityUpdates(AZ::TimeMs hostTimeMs)
    {
        EntityReplicatorList toSendList = GenerateEntityUpdateList();
    
        AZLOG(NET_ReplicationInfo, "Sending %zd updates from %d to %d", toSendList.size(), (uint8_t)GetNetworkEntityManager()->GetHostId(), (uint8_t)GetRemoteHostId());
    
        // prep a replication record for send, at this point, everything needs to be sent
        for (EntityReplicator* replicator : toSendList)
        {
            replicator->GetPropertyPublisher()->PrepareSerialization();
        }
    
        // While our to send list is not empty, build up another packet to send
        do
        {
            SendEntityUpdatesPacketHelper(hostTimeMs, toSendList, m_maxPayloadSize, m_connection);
        } while (!toSendList.empty());
    }

    void EntityReplicationManager::SendEntityRpcs(RpcMessages& deferredRpcs, bool reliable)
    {
        while (!deferredRpcs.empty())
        {
            MultiplayerPackets::EntityRpcs entityRpcsPacket;
            uint32_t pendingPacketSize = 0;

            while (!deferredRpcs.empty())
            {
                NetworkEntityRpcMessage& message = deferredRpcs.front();

                const uint32_t nextRpcSize = message.GetEstimatedSerializeSize();

                if ((pendingPacketSize + nextRpcSize) > m_maxPayloadSize)
                {
                    // We're over our limit, break and send an Rpc packet
                    if (entityRpcsPacket.GetEntityRpcs().size() == 0)
                    {
                        AZLOG(NET_Replicator, "Encountered an RPC that is above our MTU, message will be segmented (object size %u, max allowed size %u)", nextRpcSize, m_maxPayloadSize);
                        entityRpcsPacket.ModifyEntityRpcs().push_back(message);
                        deferredRpcs.pop_front();
                    }
                    break;
                }

                pendingPacketSize += nextRpcSize;
                if (entityRpcsPacket.GetEntityRpcs().full())
                {
                    // Packet was full, send what we've accumulated so far
                    AZLOG(NET_Replicator, "We've hit our RPC message limit (RPC count %u, packet size %u)", aznumeric_cast<uint32_t>(entityRpcsPacket.GetEntityRpcs().size()), pendingPacketSize);
                    break;
                }
                entityRpcsPacket.ModifyEntityRpcs().push_back(message);
                deferredRpcs.pop_front();
            }

            if (reliable)
            {
                m_connection.SendReliablePacket(entityRpcsPacket);
            }
            else
            {
                m_connection.SendUnreliablePacket(entityRpcsPacket);
            }
        }
    }

    void EntityReplicationManager::Clear(bool forMigration)
    {
        if (forMigration)
        {
            for (auto& replicatorPair : m_entityReplicatorMap)
            {
                if (!replicatorPair.second->IsMarkedForRemoval())
                {
                    replicatorPair.second->MarkForRemoval();
                }
            }
        }
        else
        {
            m_replicatorsPendingRemoval.clear();
            m_replicatorsPendingSend.clear();
        }

        m_entityReplicatorMap.clear();
    }

    bool EntityReplicationManager::SetEntityRebasing(NetworkEntityHandle& entityHandle)
    {
        EntityReplicator* entityReplicator = GetEntityReplicator(entityHandle.GetNetEntityId());
        if (entityReplicator)
        {
            PropertyPublisher* propPublisher = entityReplicator->GetPropertyPublisher();
            AZ_Assert(propPublisher, "Expected to have a property publisher");
            propPublisher->SetRebasing();
            return true;
        }
        return false;
    }

    EntityReplicator* EntityReplicationManager::AddEntityReplicator(const ConstNetworkEntityHandle& entityHandle, NetEntityRole remoteNetworkRole)
    {
        EntityReplicator* entityReplicator(nullptr);
        if (const AZ::Entity* entity = entityHandle.GetEntity())
        {
            entityReplicator = GetEntityReplicator(entityHandle);
            if (entityReplicator)
            {
                // Check if we changed our remote role - this can happen during server entity migration.  After we migrate ownership to the new server, we hold onto our entity replicator until we are sure
                //      the other side has received all the packets (and we haven't had to do resends).  At this point, it is possible hear back from the remote side we migrated to on the old replicator prior to the timeout and cleanup on the old one
                const bool changedRemoteRole = (remoteNetworkRole != entityReplicator->GetRemoteNetworkRole());
                // check if we've changed our bound local role - this can occur when we gain Autonomous or lose Autonomous on a client
                bool changedLocalRole(false);
                if (AZ::Entity* localEnt = entityReplicator->GetEntityHandle().GetEntity())
                {
                    NetBindComponent* netBindComponent = entityReplicator->GetEntityHandle().GetNetBindComponent();
                    AZ_Assert(netBindComponent != nullptr, "No NetBindComponent");
                    changedLocalRole = (netBindComponent->GetNetEntityRole() != entityReplicator->GetBoundLocalNetworkRole());
                }

                if (changedRemoteRole || changedLocalRole)
                {
                    // If we changed roles, we need to reset everything
                    if (!entityReplicator->IsMarkedForRemoval())
                    {
                        // Clear our ownership
                        entityReplicator->MarkForRemoval();
                    }
                    // Reset our replicator, we are establishing a new one
                    entityReplicator->Reset(remoteNetworkRole);
                }
                // else case is when an entity had left relevancy and come back (but it was still pending a removal)
                entityReplicator->Initialize(entityHandle);
                AZLOG(NET_RepDeletes, "Reinited replicator for %u from remote manager id %d role %d", entityHandle.GetNetEntityId(), aznumeric_cast<int32_t>(GetRemoteHostId()), aznumeric_cast<int32_t>(remoteNetworkRole));
            }
            else
            {
                // haven't seen him before, let's add him
                AZ_Assert(entityHandle.GetNetBindComponent(), "No NetBindComponent");
                AZStd::unique_ptr<EntityReplicator> newEntityReplicator = AZStd::make_unique<EntityReplicator>(*this, &m_connection, remoteNetworkRole, entityHandle);
                newEntityReplicator->Initialize(entityHandle);
                entityReplicator = newEntityReplicator.get();
                m_entityReplicatorMap.emplace(entityHandle.GetNetEntityId(), AZStd::move(newEntityReplicator));
                AZLOG(NET_RepDeletes, "Added replicator for %u from remote manager id %d role %d", entityHandle.GetNetEntityId(), aznumeric_cast<int32_t>(GetRemoteHostId()), aznumeric_cast<int32_t>(remoteNetworkRole));
            }
        }
        else
        {
            AZLOG_ERROR("Failed to add entity replicator, entity does not exist, entity id %u", entityHandle.GetNetEntityId());
            AZ_Assert(false, "Failed to add entity replicator, entity does not exist");
        }
        return entityReplicator;
    }

    EntityReplicator* EntityReplicationManager::GetEntityReplicator(const ConstNetworkEntityHandle& entityHandle)
    {
        return GetEntityReplicator(entityHandle.GetNetEntityId());
    }

    void EntityReplicationManager::GetEntityReplicatorIdList(AZStd::list<NetEntityId>& outList)
    {
        for (const auto& pair : m_entityReplicatorMap)
        {
            outList.push_back(pair.second->GetEntityHandle().GetNetEntityId());
        }
    }

    uint32_t EntityReplicationManager::GetEntityReplicatorCount(NetEntityRole localNetworkRole)
    {
        uint32_t count = 0;
        for (auto &entityReplicatorPair : m_entityReplicatorMap)
        {
            if (entityReplicatorPair.second->GetBoundLocalNetworkRole() == localNetworkRole)
            {
                ++count;
            }
        }
        return count;
    }

    void EntityReplicationManager::AddDeferredRpcMessage(NetworkEntityRpcMessage& message)
    {
        if (message.GetReliability() == ReliabilityType::Reliable)
        {
            m_deferredRpcMessagesReliable.emplace_back(message);
        }
        else
        {
            m_deferredRpcMessagesUnreliable.emplace_back(message);
        }
    }

    // @nt: TODO - delete once dropped RPC problem fixed
    void EntityReplicationManager::AddAutonomousEntityReplicatorCreatedHandle(AZ::Event<NetEntityId>::Handler& handler)
    {
        handler.Connect(m_autonomousEntityReplicatorCreated);
    }

    const EntityReplicator* EntityReplicationManager::GetEntityReplicator(NetEntityId netEntityId) const
    {
        auto it = m_entityReplicatorMap.find(netEntityId);

        if (it != m_entityReplicatorMap.end())
        {
            return it->second.get();
        }
        else
        {
            return nullptr;
        }
    }

    EntityReplicator* EntityReplicationManager::GetEntityReplicator(NetEntityId netEntityId)
    {
        const EntityReplicationManager* constThis = this;
        return const_cast<EntityReplicator*>(constThis->GetEntityReplicator(netEntityId));
    }

    bool EntityReplicationManager::HandleEntityDeleteMessage
    (
        EntityReplicator* entityReplicator,
        [[maybe_unused]] const AzNetworking::IPacketHeader& packetHeader,
        const NetworkEntityUpdateMessage& updateMessage
    )
    {
        bool shouldDeleteEntity = false;

        // Handle Replicator cleanup
        if (entityReplicator)
        {
            if (entityReplicator->IsMarkedForRemoval())
            {
                AZLOG(NET_RepDeletes, "Got a replicator delete message that is a duplicate id %u remote manager id %d", updateMessage.GetEntityId(), aznumeric_cast<int32_t>(GetRemoteHostId()));
            }
            else if (entityReplicator->OwnsReplicatorLifetime())
            {
                // This can occur if we migrate entities quickly - if this is a replicator from C to A, A migrates to B, B then migrates to C, and A's delete replicator has not arrived at C
                AZLOG(NET_RepDeletes, "Got a replicator delete message for a replicator we own id %u remote manager id %d", updateMessage.GetEntityId(), aznumeric_cast<int32_t>(GetRemoteHostId()));
            }
            else
            {
                shouldDeleteEntity = true;
                entityReplicator->MarkForRemoval();
                AZLOG(NET_RepDeletes, "Deleting replicater for entity id %u remote manager id %d", updateMessage.GetEntityId(), aznumeric_cast<int32_t>(GetRemoteHostId()));
            }
        }
        else
        {
            shouldDeleteEntity = updateMessage.GetTakeOwnership();
        }

        // Handle entity cleanup
        if (shouldDeleteEntity)
        {
            ConstNetworkEntityHandle entity = GetNetworkEntityManager()->GetEntity(updateMessage.GetEntityId());
            if (entity)
            {
                if (updateMessage.GetWasMigrated())
                {
                    AZLOG(NET_RepDeletes, "Leaving id %u using timeout remote manager id %d", entity.GetNetEntityId(), aznumeric_cast<int32_t>(GetRemoteHostId()));
                }
                else
                {
                    AZLOG(NET_RepDeletes, "Deleting entity id %u remote manager id %d", entity.GetNetEntityId(), aznumeric_cast<int32_t>(GetRemoteHostId()));
                    GetNetworkEntityManager()->MarkForRemoval(entity);
                }
            }
            else
            {
                AZLOG(NET_RepDeletes, "Trying to delete entity id %u remote manager id %d, but it has been removed", entity.GetNetEntityId(), aznumeric_cast<int32_t>(GetRemoteHostId()));
            }
        }

        return true;
    }

    bool EntityReplicationManager::HandlePropertyChangeMessage
    (
        EntityReplicator* entityReplicator,
        AzNetworking::PacketId packetId,
        NetEntityId netEntityId,
        NetEntityRole localNetworkRole,
        AzNetworking::ISerializer& serializer,
        const PrefabEntityId& prefabEntityId
    )
    {
        ConstNetworkEntityHandle replicatorEntity = GetNetworkEntityManager()->GetEntity(netEntityId);

        const bool createEntity = (replicatorEntity == nullptr);
        const bool notifySerializationChanges = (replicatorEntity && replicatorEntity.GetEntity()->GetState() == AZ::Entity::State::Active);

        // Create an entity if we don't have one
        if (createEntity)
        {
            INetworkEntityManager::EntityList entityList = GetNetworkEntityManager()->CreateEntitiesImmediate(
                prefabEntityId, netEntityId, localNetworkRole, AutoActivate::DoNotActivate, AZ::Transform::Identity());

            if (entityList.size() == 1)
            {
                replicatorEntity = entityList[0];
            }
            else
            {
                AZ_Assert(false, "There should be exactly one created entity out of prefab %s, index %d. Got: %d",
                    prefabEntityId.m_prefabName.GetCStr(), prefabEntityId.m_entityOffset, entityList.size());
                return false;
            }
        }

        NetBindComponent* netBindComponent = replicatorEntity.GetNetBindComponent();
        AZ_Assert(netBindComponent != nullptr, "No NetBindComponent");

        const bool changeNetworkRole = (netBindComponent->GetNetEntityRole() != localNetworkRole);
        if (changeNetworkRole)
        {
            AZ_Assert(localNetworkRole != NetEntityRole::Authority, "UpdateMessage trying to set local role to Authority, this should only happen via migration");
            AZLOG_INFO
            (
                "EntityReplicationManager: Changing network role on entity %u, old role %u new role %u",
                aznumeric_cast<uint32_t>(netEntityId),
                aznumeric_cast<uint32_t>(netBindComponent->GetNetEntityRole()),
                aznumeric_cast<uint32_t>(localNetworkRole)
            );

            if (NetworkRoleHasController(localNetworkRole))
            {
                // We defer activation until after the data has been deserialized into our entity.
                // The packet may contain additional data that might be required for a component's proper activation.
                netBindComponent->ConstructControllers();
            }
            else
            {
                // We have lost control, deactivate and destroy the controllers
                netBindComponent->DeactivateControllers(EntityIsMigrating::False);
                netBindComponent->DestructControllers();
            }
        }

        const bool createReplicator = (entityReplicator == nullptr)
                                    || entityReplicator->IsMarkedForRemoval()
                                    || entityReplicator->GetBoundLocalNetworkRole() != localNetworkRole;
        if (createReplicator)
        {
            // Make sure this entity that we're getting a packet on hasn't been marked for removal by someone else
            // This can occur on a 3 server case where an entity has migrated from A->B and we are on server C, observing the migration.
            // A will tell us to set a timer to delete that entity (since it no longer owns it, and has been handed off), and B will tell us to create it.
            // This covers an edge case where the timer has popped, but the entity is pending removal when we are told by B to create the entity.
            GetNetworkEntityManager()->ClearEntityFromRemovalList(replicatorEntity);
            entityReplicator = AddEntityReplicator(replicatorEntity, NetEntityRole::Authority);
            //AZLOG(NET_RepUpdate, "EntityReplicationManager: Created from update entity id %u for type %s role %d", netEntityId, prefabEntityId.GetString(), localNetworkRole);
        }

        // @nt: TODO - delete once dropped RPC problem fixed
        // This code is temporary to work around to the problem that RPC messages are silently lost during migration
        // Once this problem is solved, we can remove this code and associated event
        if (createReplicator && localNetworkRole == NetEntityRole::Autonomous)
        {
            m_autonomousEntityReplicatorCreated.Signal(netEntityId);
        }

        //AZLOG(NET_RepUpdate, "EntityReplicationManager: Received PropertyChangeMessage message for entity id %u for type %s role %d", netEntityId, prefabEntityId.GetString(), localNetworkRole);

        bool didSucceed = entityReplicator->GetPropertySubscriber()->HandlePropertyChangeMessage(packetId, &serializer, notifySerializationChanges);

        if (changeNetworkRole)
        {
            if (NetworkRoleHasController(localNetworkRole))
            {
                // Activate the controllers since the entity had previously been activated
                netBindComponent->ActivateControllers(EntityIsMigrating::False);
            }
        }

        if (createEntity)
        {
            // We defer activation until after the packet has been deserialized (this will also implicitly activate controllers if they exist)
            // The actual entity activate could be deferred further, in cases where entity dependencies are not met
            m_entitiesPendingActivation.push_back(netEntityId);
        }

        if (createReplicator && !createEntity)
        {
            // See if we have any outstanding RPCs that came in prior to creating the entity
            didSucceed &= m_orphanedEntityRpcs.DispatchOrphanedRpcs(*entityReplicator);
        }

        return didSucceed;
    }

    EntityReplicationManager::UpdateValidationResult EntityReplicationManager::ValidateUpdate
    (
        const NetworkEntityUpdateMessage& updateMessage,
        AzNetworking::PacketId packetId,
        EntityReplicator* entityReplicator
    )
    {
        UpdateValidationResult result = UpdateValidationResult::HandleMessage;

        switch (m_updateMode)
        {
        case Mode::LocalServerToRemoteClient:
            {
                // Don't trust the client by default
                result = UpdateValidationResult::DropMessageAndDisconnect;
                // Clients sending data must have a replicator and be sending in the correct mode, further, they must have a replicator and can never delete a replicator
                if (updateMessage.GetNetworkRole() == NetEntityRole::Authority && entityReplicator && !updateMessage.GetIsDelete())
                {
                    // Make sure we our replicator is in the expected configuration
                    if ((entityReplicator->GetRemoteNetworkRole() == NetEntityRole::Autonomous) && (entityReplicator->GetBoundLocalNetworkRole() == NetEntityRole::Authority))
                    {
                        // If we're marked for removal, just drop the message - migration message is likely in flight
                        if (entityReplicator->IsMarkedForRemoval())
                        {
                            result = UpdateValidationResult::DropMessage;
                        }
                        else
                        {
                            // We can process this
                            result = UpdateValidationResult::HandleMessage;
                        }
                    }  // If we've migrated the entity away from the server, but we get this late, just drop it
                    else if ((entityReplicator->GetRemoteNetworkRole() == NetEntityRole::Client) && (entityReplicator->GetBoundLocalNetworkRole() == NetEntityRole::Server))
                    {
                        result = UpdateValidationResult::DropMessage;
                    }
                }
                if (result == UpdateValidationResult::DropMessageAndDisconnect)
                {
                    AZLOG_WARN
                    (
                        "Dropping Packet and LocalServerToRemoteClient connection, unexpected packet "
                        "LocalShard=%u EntityId=%u RemoteNetworkRole=%u BoundLocalNetworkRole=%u ActualNetworkRole=%u IsMarkedForRemoval=%s",
                        aznumeric_cast<uint32_t>(GetNetworkEntityManager()->GetHostId()),
                        aznumeric_cast<uint32_t>(entityReplicator->GetEntityHandle().GetNetEntityId()),
                        aznumeric_cast<uint32_t>(entityReplicator->GetRemoteNetworkRole()),
                        aznumeric_cast<uint32_t>(entityReplicator->GetBoundLocalNetworkRole()),
                        aznumeric_cast<uint32_t>(entityReplicator->GetNetBindComponent()->GetNetEntityRole()),
                        entityReplicator->IsMarkedForRemoval() ? "true" : "false"
                    );
                }
            }
            break;
        case Mode::LocalServerToRemoteServer:
            {
                AZ_Assert(updateMessage.GetNetworkRole() == NetEntityRole::Server || updateMessage.GetIsDelete(), "Unexpected update type coming from peer server");
                // Trust messages from a peer server by default
                result = UpdateValidationResult::HandleMessage;
                // If we have a replicator, make sure we're in the correct state
                if (entityReplicator)
                {
                    if (!entityReplicator->IsMarkedForRemoval() && (entityReplicator->GetBoundLocalNetworkRole() == NetEntityRole::Authority))
                    {
                        // Likely an old message from a previous owner trying to delete the replicator it had, while we've received ownership
                        // This can happen when Shard A migrates an entity to Shard B, then shard B migrates the entity to Shard C, and Shard A tries to delete a replicator it had to Shard C (which has already made a new replicator for Shard A)
                        result = UpdateValidationResult::DropMessage;
                    }
                    else if (entityReplicator->GetRemoteNetworkRole() != NetEntityRole::Authority)// we expect to the remote role to be e_Authority
                    {
                        // This entity has migrated previously, and we haven't heard back that the remove was successful, so we can accept the message
                        AZ_Assert(entityReplicator->IsMarkedForRemoval() && entityReplicator->GetRemoteNetworkRole() == NetEntityRole::Server, "Unexpected server message is not Authority or Server");
                    }
                }
            }
            break;
        case Mode::LocalClientToRemoteServer:
            {
                // Trust everything from the server
                result = UpdateValidationResult::HandleMessage;
            }
            break;
        }

        // Make sure if everything else looks good, that we don't have an old out of order message
        if (result == UpdateValidationResult::HandleMessage && entityReplicator && !entityReplicator->IsMarkedForRemoval())
        {
            PropertySubscriber* propSubscriber = entityReplicator->GetPropertySubscriber();
            AZ_Assert(propSubscriber, "Expected to have a property subscriber if we are handling a message");
            if (!propSubscriber->IsPacketIdValid(packetId))
            {
                // Got an old message
                result = UpdateValidationResult::DropMessage;
                if (updateMessage.GetIsDelete())
                {
                    AZLOG(NET_RepDeletes, "EntityReplicationManager: Received old DeleteProxy message for entity id %u, sequence %d latest sequence %d from remote manager id %d",
                        updateMessage.GetEntityId(), (uint32_t)packetId, (uint32_t)propSubscriber->GetLastReceivedPacketId(), aznumeric_cast<int32_t>(GetRemoteHostId()));
                }
                else
                {
                    AZLOG(NET_RepUpdate, "EntityReplicationManager: Received old PropertyChangeMessage message for entity id %u, sequence %d latest sequence %d from remote manager id %d",
                        updateMessage.GetEntityId(), (uint32_t)packetId, (uint32_t)propSubscriber->GetLastReceivedPacketId(), aznumeric_cast<int32_t>(GetRemoteHostId()));
                }
            }
        }
        return result;
    }

    bool EntityReplicationManager::HandleEntityUpdateMessage
    (
        [[maybe_unused]] AzNetworking::IConnection* invokingConnection,
        const AzNetworking::IPacketHeader& packetHeader,
        const NetworkEntityUpdateMessage& updateMessage
    )
    {
        // May still be nullptr
        EntityReplicator* entityReplicator = GetEntityReplicator(updateMessage.GetEntityId());
        UpdateValidationResult result = ValidateUpdate(updateMessage, packetHeader.GetPacketId(), entityReplicator);
        switch (result)
        {
        case UpdateValidationResult::HandleMessage:
            break;
        case UpdateValidationResult::DropMessage:
            return true;
        case UpdateValidationResult::DropMessageAndDisconnect:
            return false;
        default:
            AZ_Assert(false, "Unhandled case");
        }

        if (updateMessage.GetIsDelete())
        {
            return HandleEntityDeleteMessage(entityReplicator, packetHeader, updateMessage);
        }

        AzNetworking::TrackChangedSerializer<AzNetworking::NetworkOutputSerializer> outputSerializer(updateMessage.GetData()->GetBuffer(), updateMessage.GetData()->GetSize());

        PrefabEntityId prefabEntityId;
        if (updateMessage.GetHasValidPrefabId())
        {
            // If the update packet contained a PrefabEntityId, use that directly
            prefabEntityId = updateMessage.GetPrefabEntityId();
        }
        else
        {
            // No PrefabEntityId was provided, so the remote endpoint assumed we already have a replicator set up
            // Validate that our replicator actually exists and that it contains a valid PrefabEntityId
            if ((entityReplicator == nullptr) || !entityReplicator->IsPrefabEntityIdSet())
            {
                // Note that we need to make sure the replicator is not marked for removal if we're server authority
                // If a client migrates and we receive a property update message out-of-order, this would re-create a replicator which would be bad
                AZLOG_ERROR("Unable to process NetworkEntityUpdateMessage without a prefabEntityId, our local EntityReplicator is not set up or is configured incorrectly");
                return true;
            }

            // Use the cached slice entry data from the entity replicator
            prefabEntityId = entityReplicator->GetPrefabEntityId();
        }

        // This may implicitly create a replicator for us
        bool handled = HandlePropertyChangeMessage(entityReplicator, packetHeader.GetPacketId(), updateMessage.GetEntityId(), updateMessage.GetNetworkRole(), outputSerializer, prefabEntityId);
        AZ_Assert(handled, "Failed to handle NetworkEntityUpdateMessage message");

        return handled;
    }

    bool EntityReplicationManager::HandleEntityRpcMessage(AzNetworking::IConnection* invokingConnection, NetworkEntityRpcMessage& message)
    {
        EntityReplicator* entityReplicator = GetEntityReplicator(message.GetEntityId());
        const bool isReplicatorValid = (entityReplicator != nullptr) && !entityReplicator->IsMarkedForRemoval();
        const bool isEntityActivated = isReplicatorValid && entityReplicator->GetEntityHandle() && (entityReplicator->GetEntityHandle().GetEntity()->GetState() == AZ::Entity::State::Active);
        if (!isReplicatorValid || !isEntityActivated)
        {
            m_orphanedEntityRpcs.AddOrphanedRpc(message.GetEntityId(), message);
            return true;
        }
        else
        {
            return entityReplicator->HandleRpcMessage(invokingConnection, message);
        }
    }

    bool EntityReplicationManager::DispatchOrphanedRpc(NetworkEntityRpcMessage& message, EntityReplicator* entityReplicator)
    {
        if (entityReplicator == nullptr)
        {
            AZLOG_INFO
            (
                "EntityReplicationManager: Dropping remote RPC message for component %s of rpc index %s, entityId %u has already been deleted",
                GetMultiplayerComponentRegistry()->GetComponentName(message.GetComponentId()),
                GetMultiplayerComponentRegistry()->GetComponentRpcName(message.GetComponentId(), message.GetRpcIndex()),
                message.GetEntityId()
            );
            return false;
        }
        return entityReplicator->HandleRpcMessage(nullptr, message);
    }

    AZ::TimeMs EntityReplicationManager::GetResendTimeoutTimeMs() const
    {
        return aznumeric_cast<AZ::TimeMs>(aznumeric_cast<uint32_t>(m_connection.GetMetrics().m_connectionRtt.GetRoundTripTimeSeconds()) * 1000 * 2);
    }

    void EntityReplicationManager::SetMaxRemoteEntitiesPendingCreationCount(uint32_t maxPendingEntities)
    {
        m_maxRemoteEntitiesPendingCreationCount = maxPendingEntities;
    }

    void EntityReplicationManager::SetEntityActivationTimeSliceMs(AZ::TimeMs timeSliceMs)
    {
        m_entityActivationTimeSliceMs = timeSliceMs;
    }

    void EntityReplicationManager::SetEntityPendingRemovalMs(AZ::TimeMs entityPendingRemovalMs)
    {
        m_entityPendingRemovalMs = entityPendingRemovalMs;
    }

    AzNetworking::IConnection& EntityReplicationManager::GetConnection()
    {
        return m_connection;
    }

    AZ::TimeMs EntityReplicationManager::GetFrameTimeMs()
    {
        return m_frameTimeMs;
    }


    EntityReplicationManager::OrphanedEntityRpcs::OrphanedEntityRpcs(EntityReplicationManager& replicationManager)
        : m_replicationManager(replicationManager)
    {
        ;
    }

    AzNetworking::TimeoutResult EntityReplicationManager::OrphanedEntityRpcs::HandleTimeout(AzNetworking::TimeoutQueue::TimeoutItem& item)
    {
        NetEntityId timedOutEntityId = aznumeric_cast<NetEntityId>(item.m_userData);
        auto entityRpcsIter = m_entityRpcMap.find(timedOutEntityId);
        if (entityRpcsIter != m_entityRpcMap.end())
        {
            for (NetworkEntityRpcMessage& rpcMessage : entityRpcsIter->second.m_rpcMessages)
            {
                m_replicationManager.DispatchOrphanedRpc(rpcMessage, nullptr);
            }
            m_entityRpcMap.erase(entityRpcsIter);
        }
        return AzNetworking::TimeoutResult::Delete;
    }

    void EntityReplicationManager::OrphanedEntityRpcs::Update()
    {
        m_timeoutQueue.UpdateTimeouts(*this);
    }

    bool EntityReplicationManager::OrphanedEntityRpcs::DispatchOrphanedRpcs(EntityReplicator& entityReplicator)
    {
        auto orphanedRpcsIter = m_entityRpcMap.find(entityReplicator.GetEntityHandle().GetNetEntityId());
        if (orphanedRpcsIter != m_entityRpcMap.end())
        {
            bool dispatchedAll = true;
            for (NetworkEntityRpcMessage& rpcMessage : orphanedRpcsIter->second.m_rpcMessages)
            {
                dispatchedAll &= m_replicationManager.DispatchOrphanedRpc(rpcMessage, &entityReplicator);
            }

            m_timeoutQueue.RemoveItem(orphanedRpcsIter->second.m_timeoutId);
            m_entityRpcMap.erase(orphanedRpcsIter);
            return dispatchedAll;
        }

        return true;
    }

    void EntityReplicationManager::OrphanedEntityRpcs::AddOrphanedRpc(NetEntityId netEntityId, NetworkEntityRpcMessage& message)
    {
        auto orphanedRpcsIter = m_entityRpcMap.find(netEntityId);
        if (orphanedRpcsIter == m_entityRpcMap.end())
        {
            OrphanedRpcs& orphanedRpcs = m_entityRpcMap[netEntityId];
            orphanedRpcs.m_timeoutId = m_timeoutQueue.RegisterItem(aznumeric_cast<uint64_t>(netEntityId), m_replicationManager.GetResendTimeoutTimeMs());
            orphanedRpcsIter = m_entityRpcMap.find(netEntityId);
        }
        orphanedRpcsIter->second.m_rpcMessages.emplace_back(AZStd::move(message));
    }

    void EntityReplicationManager::UpdateWindow()
    {
        if (!m_replicationWindow)
        {
            // No window setup, this will occur during connection
            return;
        }

        if (m_replicationWindow->ReplicationSetUpdateReady())
        {
            const ReplicationSet& newWindow = m_replicationWindow->GetReplicationSet();

            // Walk both for adds and removals
            auto newWindowIter = newWindow.begin();
            auto currWindowIter = m_entityReplicatorMap.begin();
            while (newWindowIter != newWindow.end() && currWindowIter != m_entityReplicatorMap.end())
            {
                if (newWindowIter->first && (newWindowIter->first.GetNetEntityId() < currWindowIter->first))
                {
                    AddEntityReplicator(newWindowIter->first, newWindowIter->second.m_netEntityRole);
                    ++newWindowIter;
                }
                else if (newWindowIter->first.GetNetEntityId() > currWindowIter->first)
                {
                    EntityReplicator* currReplicator = currWindowIter->second.get();
                    if (currReplicator->OwnsReplicatorLifetime())
                    {
                        currReplicator->SetPendingRemoval(m_entityPendingRemovalMs);
                    }
                    ++currWindowIter;
                }
                else // Same entity
                {
                    // Check if we changed modes
                    EntityReplicator* currReplicator = currWindowIter->second.get();
                    if (currReplicator->GetRemoteNetworkRole() != newWindowIter->second.m_netEntityRole)
                    {
                        currReplicator = AddEntityReplicator(newWindowIter->first, newWindowIter->second.m_netEntityRole);
                    }
                    currReplicator->ClearPendingRemoval();
                    ++newWindowIter;
                    ++currWindowIter;
                }
            }

            // Do remaining adds
            while (newWindowIter != newWindow.end())
            {
                AddEntityReplicator(newWindowIter->first, newWindowIter->second.m_netEntityRole);
                ++newWindowIter;
            }

            // Do remaining removes
            while (currWindowIter != m_entityReplicatorMap.end())
            {
                EntityReplicator* currReplicator = currWindowIter->second.get();
                if (currReplicator->OwnsReplicatorLifetime())
                {
                    currReplicator->SetPendingRemoval(m_entityPendingRemovalMs);
                }
                ++currWindowIter;
            }
        }
    }

    void EntityReplicationManager::MigrateAllEntities()
    {
        AZStd::list<NetEntityId> replicatorList;
        GetEntityReplicatorIdList(replicatorList);
        for (auto iter = replicatorList.begin(); iter != replicatorList.end(); ++iter)
        {
            auto replicator = GetEntityReplicator(*iter);
            if (replicator && replicator->OwnsReplicatorLifetime())
            {
                MigrateEntityInternal(*iter);
            }
            else
            {
                ++iter;
            }
        }
    }

    void EntityReplicationManager::MigrateEntity(NetEntityId netEntityId)
    {
        MigrateEntityInternal(netEntityId);
    }

    bool EntityReplicationManager::CanMigrateEntity(const ConstNetworkEntityHandle& entityHandle) const
    {
        bool hasAuthority{ false };
        bool isInDomain{ false };
        bool isMarkedForRemoval{ true };
        bool isRemoteReplicatorEstablished{ false };

        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
        AZ_Assert(netBindComponent, "No NetBindComponent");

        const EntityReplicator* entityReplicator = GetEntityReplicator(entityHandle.GetNetEntityId());
        hasAuthority = (netBindComponent->GetNetEntityRole() == NetEntityRole::Authority); // Make sure someone hasn't migrated this already
        isInDomain = (m_remoteEntityDomain && m_remoteEntityDomain->IsInDomain(entityHandle)); // Make sure the remote side would want it
        if (entityReplicator && entityReplicator->GetBoundLocalNetworkRole() == NetEntityRole::Authority)
        {
            isMarkedForRemoval = entityReplicator->IsMarkedForRemoval(); // Make sure we aren't telling the other side to remove the replicator
            const PropertyPublisher* propertyPublisher = entityReplicator->GetPropertyPublisher();
            AZ_Assert(propertyPublisher, "Expected to have a property publisher");
            isRemoteReplicatorEstablished = propertyPublisher->IsRemoteReplicatorEstablished(); // Make sure they are setup to receive the replicator
        }

        return hasAuthority && isInDomain && !isMarkedForRemoval && isRemoteReplicatorEstablished;
    }

    bool EntityReplicationManager::HasRemoteAuthority(const ConstNetworkEntityHandle& entityHandle) const
    {
        if (const EntityReplicator* replicator = GetEntityReplicator(entityHandle.GetNetEntityId()))
        {
            return replicator->GetRemoteNetworkRole() == NetEntityRole::Authority;
        }
        return false;
    }

    void EntityReplicationManager::SetEntityDomain(AZStd::unique_ptr<IEntityDomain> entityDomain)
    {
        m_remoteEntityDomain = AZStd::move(entityDomain);
    }

    IEntityDomain* EntityReplicationManager::GetEntityDomain()
    {
        return m_remoteEntityDomain.get();
    }

    void EntityReplicationManager::SetReplicationWindow(AZStd::unique_ptr<IReplicationWindow> replicationWindow)
    {
        m_replicationWindow = AZStd::move(replicationWindow);
        UpdateWindow();
    }

    IReplicationWindow* EntityReplicationManager::GetReplicationWindow()
    {
        return m_replicationWindow.get();
    }

    void EntityReplicationManager::MigrateEntityInternal(NetEntityId netEntityId)
    {
        ConstNetworkEntityHandle entityHandle = GetNetworkEntityManager()->GetEntity(netEntityId);
        AZ::Entity* localEnt = entityHandle.GetEntity();
        if (!localEnt)
        {
            return;
        }

        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
        AZ_Assert(netBindComponent, "No NetBindComponent");

        if (netBindComponent && netBindComponent->GetNetEntityRole() == NetEntityRole::Authority)
        {
            EntityReplicator* replicator = AddEntityReplicator(entityHandle, NetEntityRole::Server);
            PropertyPublisher* propPublisher = replicator->GetPropertyPublisher();
            AZ_Assert(propPublisher, "Assumed we have a property publisher");

            if (m_updateMode == EntityReplicationManager::Mode::LocalServerToRemoteServer)
            {
                netBindComponent->NotifyServerMigration(GetRemoteHostId(), GetConnection().GetConnectionId());
            }

            bool didSucceed = true;
            EntityMigrationMessage message;
            message.m_entityId = replicator->GetEntityHandle().GetNetEntityId();
            message.m_prefabEntityId = netBindComponent->GetPrefabEntityId();

            if (localEnt->GetState() == AZ::Entity::State::Active)
            {
                netBindComponent->DeactivateControllers(EntityIsMigrating::True);
            }

            netBindComponent->DestructControllers();

            // Gather the most recent network property state, including authoritative only network properties for migration
            {
                // Send an update packet if it needs one
                propPublisher->GenerateRecord();
                bool needsNetworkPropertyUpdate = propPublisher->PrepareSerialization();
                AzNetworking::NetworkInputSerializer inputSerializer(message.m_propertyUpdateData.GetBuffer(), message.m_propertyUpdateData.GetCapacity());
                if (needsNetworkPropertyUpdate)
                {
                    // Write out entity state into the buffer
                    propPublisher->UpdateSerialization(inputSerializer);
                }
                didSucceed &= inputSerializer.IsValid();
                message.m_propertyUpdateData.Resize(inputSerializer.GetSize());
            }
            AZ_Assert(didSucceed, "Failed to migrate entity from server");
            // TODO: Move this to an event
            //m_connection.SendReliablePacket(message);
            AZLOG(NET_RepDeletes, "Migration packet sent %u to remote manager id %d", netEntityId, aznumeric_cast<int32_t>(GetRemoteHostId()));

            // Immediately add a new replicator so that we catch RPC invocations, the remote side will make us a new one, and then remove us if needs be
            AddEntityReplicator(entityHandle, NetEntityRole::Authority);
        }
    }

    bool EntityReplicationManager::HandleEntityMigration([[maybe_unused]] AzNetworking::IConnection* invokingConnection, EntityMigrationMessage& message)
    {
        EntityReplicator* replicator = GetEntityReplicator(message.m_entityId);
        {
            if (message.m_propertyUpdateData.GetSize() > 0)
            {
                AzNetworking::TrackChangedSerializer<AzNetworking::NetworkOutputSerializer> outputSerializer(message.m_propertyUpdateData.GetBuffer(), message.m_propertyUpdateData.GetSize());
                if (!HandlePropertyChangeMessage
                (
                    replicator,
                    AzNetworking::InvalidPacketId,
                    message.m_entityId,
                    NetEntityRole::Server,
                    outputSerializer,
                    message.m_prefabEntityId
                ))
                {
                    AZ_Assert(false, "Unable to process network properties during server entity migration");
                    return false;
                }
            }
        }
        // The HandlePropertyChangeMessage will have made a replicator if we didn't have one already
        if (!replicator)
        {
            replicator = GetEntityReplicator(message.m_entityId);
        }
        AZ_Assert(replicator, "Do not have replicator after handling migration message");

        ConstNetworkEntityHandle entityHandle = replicator->GetEntityHandle();
        NetBindComponent* netBindComponent = entityHandle.GetNetBindComponent();
        AZ_Assert(netBindComponent, "No NetBindComponent");

        // Stop listening to the OnEntityNetworkRoleChange, since we are about to change it and we don't want that callback
        netBindComponent->ConstructControllers();

        if (entityHandle.GetEntity()->GetState() == AZ::Entity::State::Active)
        {
            // Only activate controllers if the entity was previously activated, otherwise, wait for the normal entity activation flow
            netBindComponent->ActivateControllers(EntityIsMigrating::True);
        }

        // Change the role on the replicator
        AddEntityReplicator(entityHandle, NetEntityRole::Server);

        AZLOG(NET_RepDeletes, "Handle Migration %u new authority from remote manager id %d", entityHandle.GetNetEntityId(), aznumeric_cast<int32_t>(GetRemoteHostId()));
        return true;
    }

    void EntityReplicationManager::OnEntityExitDomain(const ConstNetworkEntityHandle& entityHandle)
    {
        if (CanMigrateEntity(entityHandle))
        {
            MigrateEntity(entityHandle.GetNetEntityId());
        }
    }

    void EntityReplicationManager::OnPostEntityMigration(const ConstNetworkEntityHandle& entityHandle, HostId remoteHostId, [[maybe_unused]] AzNetworking::ConnectionId connectionId)
    {
        if (remoteHostId == GetRemoteHostId())
        {
            // don't handle self sent messages
            return;
        }

        NetEntityRole remoteRole = NetEntityRole::InvalidRole;
        // TODO: Rethink the IsInWindow call here, this is an IReplicationWindow concern - should we need this at all?
        if (m_replicationWindow && m_replicationWindow->IsInWindow(entityHandle, remoteRole))
        {
            AddEntityReplicator(entityHandle, remoteRole);
        }
        else
        {
            EntityReplicator* replicator = GetEntityReplicator(entityHandle);
            if (replicator)
            {
                replicator->SetWasMigrated(true);
                replicator->MarkForRemoval();
            }
        }
    }

    void EntityReplicationManager::AddReplicatorToPendingRemoval(const EntityReplicator& replicator)
    {
        m_replicatorsPendingRemoval.insert(replicator.GetEntityHandle().GetNetEntityId());
    }

    void EntityReplicationManager::AddReplicatorToPendingSend(const EntityReplicator& replicator)
    {
        m_replicatorsPendingSend.insert(replicator.GetEntityHandle().GetNetEntityId());
    }

    bool EntityReplicationManager::IsUpdateModeToServerClient()
    {
        return (m_updateMode != Mode::LocalServerToRemoteServer);
    }

    void EntityReplicationManager::ClearRemovedReplicators()
    {
        for (auto iter = m_replicatorsPendingRemoval.begin(); iter != m_replicatorsPendingRemoval.end();)
        {
            EntityReplicator* replicator = GetEntityReplicator(*iter);
            AZ_Assert(replicator, "Replicator deleted unexpectedly");
            if (replicator->IsMarkedForRemoval())
            {
                if (replicator->IsDeletionAcknowledged())
                {
                    m_remoteEntitiesPendingCreation.erase(replicator->GetEntityHandle().GetNetEntityId());
                    m_entityReplicatorMap.erase(*iter);
                    iter = m_replicatorsPendingRemoval.erase(iter);
                }
                else
                {
                    ++iter;
                }
            }
            else
            {
                // no longer marked for removal, remove it from the set
                iter = m_replicatorsPendingRemoval.erase(iter);
            }
        }
    }
}
