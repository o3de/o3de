/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicationManager.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>
#include <Source/NetworkEntity/EntityReplication/PropertyPublisher.h>
#include <Source/NetworkEntity/EntityReplication/PropertySubscriber.h>
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
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Math/Transform.h>

AZ_DECLARE_BUDGET(MULTIPLAYER);

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
        , m_notifyEntityMigrationHandler([this](const ConstNetworkEntityHandle& entityHandle, const HostId& remoteHostId) { OnPostEntityMigration(entityHandle, remoteHostId); })
    {
        // Set up our remote host identifier, by default we use the IP address of the remote host
        m_remoteHostId = connection.GetRemoteAddress();

        // Our max payload size is whatever is passed in, minus room for a udp packetheader
        m_maxPayloadSize = connection.GetConnectionMtu() - UdpPacketHeaderSerializeSize - ReplicationManagerPacketOverhead;

        // Schedule ClearRemovedReplicators()
        m_clearRemovedReplicators.Enqueue(AZ::Time::ZeroTimeMs, true);

        // Start window update events
        m_updateWindow.Enqueue(AZ::Time::ZeroTimeMs, true);

        INetworkEntityManager* networkEntityManager = GetNetworkEntityManager();
        if (networkEntityManager != nullptr)
        {
            networkEntityManager->AddEntityExitDomainHandler(m_entityExitDomainEventHandler);
        }

        if (m_updateMode == Mode::LocalServerToRemoteServer)
        {
            GetMultiplayer()->AddNotifyEntityMigrationEventHandler(m_notifyEntityMigrationHandler);
        }
    }

    const HostId& EntityReplicationManager::GetRemoteHostId() const
    {
        return m_remoteHostId;
    }

    void EntityReplicationManager::ActivatePendingEntities()
    {
        AZ_PROFILE_SCOPE(MULTIPLAYER, "EntityReplicationManager: ActivatePendingEntities");

        AZStd::vector<NetEntityId> notReadyEntities;

        const AZ::TimeMs endTimeMs = AZ::GetElapsedTimeMs() + m_entityActivationTimeSliceMs;
        while (!m_entitiesPendingActivation.empty())
        {
            NetEntityId entityId = m_entitiesPendingActivation.front();
            m_entitiesPendingActivation.pop_front();
            EntityReplicator* entityReplicator = GetEntityReplicator(entityId);
            if (entityReplicator && !entityReplicator->IsMarkedForRemoval())
            {
                if (entityReplicator->IsReadyToActivate())
                {
                    entityReplicator->ActivateNetworkEntity();
                }
                else
                {
                    notReadyEntities.push_back(entityId);
                }
            }
            if (m_entityActivationTimeSliceMs > AZ::Time::ZeroTimeMs && AZ::GetElapsedTimeMs() > endTimeMs)
            {
                // If we go over our timeslice, break out the loop
                break;
            }
        }

        for (NetEntityId netEntityId : notReadyEntities)
        {
            m_entitiesPendingActivation.push_back(netEntityId);
        }
    }

    void EntityReplicationManager::SendUpdates()
    {
        m_frameTimeMs = AZ::GetElapsedTimeMs();

        {
            EntityReplicatorList toSendList = GenerateEntityUpdateList();

            AZLOG
            (
                NET_ReplicationInfo,
                "Sending %zd updates from %s to %s",
                toSendList.size(),
                GetNetworkEntityManager()->GetHostId().GetString().c_str(),
                GetRemoteHostId().GetString().c_str()
            );

            {
                AZ_PROFILE_SCOPE(MULTIPLAYER, "EntityReplicationManager: SendUpdates - PrepareSerialization");
                // Prep a replication record for send, at this point, everything needs to be sent
                for (EntityReplicator* replicator : toSendList)
                {
                    replicator->GetPropertyPublisher()->PrepareSerialization();
                }
            }

            {
                AZ_PROFILE_SCOPE(MULTIPLAYER, "EntityReplicationManager: SendUpdates - SendEntityUpdateMessages");
                // While our to send list is not empty, build up another packet to send
                do
                {
                    SendEntityUpdateMessages(toSendList);
                } while (!toSendList.empty());
            }
        }

        SendEntityRpcs(m_deferredRpcMessagesReliable, true);
        SendEntityRpcs(m_deferredRpcMessagesUnreliable, false);

        m_orphanedEntityRpcs.Update();

        SendEntityResets();

        AZLOG
        (
            NET_ReplicationInfo,
            "Sending from %s to %s, replicator count %u orphan count %u deferred reliable count %u deferred unreliable count %u",
            GetNetworkEntityManager()->GetHostId().GetString().c_str(),
            GetRemoteHostId().GetString().c_str(),
            aznumeric_cast<uint32_t>(m_entityReplicatorMap.size()),
            aznumeric_cast<uint32_t>(m_orphanedEntityRpcs.Size()),
            aznumeric_cast<uint32_t>(m_deferredRpcMessagesReliable.size()),
            aznumeric_cast<uint32_t>(m_deferredRpcMessagesUnreliable.size())
        );
    }

    EntityReplicationManager::EntityReplicatorList EntityReplicationManager::GenerateEntityUpdateList()
    {
        if (m_replicationWindow == nullptr)
        {
            return EntityReplicatorList();
        }

        AZ_PROFILE_SCOPE(MULTIPLAYER, "EntityReplicationManager: GenerateEntityUpdateList");

        // Generate a list of all our entities that need updates
        EntityReplicatorList toSendList;

        uint32_t proxySendCount = 0;
        for (auto iter = m_replicatorsPendingSend.begin(); iter != m_replicatorsPendingSend.end();)
        {
            bool clearPendingSend = true;
            if (EntityReplicator* replicator = GetEntityReplicator(*iter))
            {
                NetEntityId entityId = replicator->GetEntityHandle().GetNetEntityId();
                if (PropertyPublisher* propPublisher = replicator->GetPropertyPublisher())
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

                        if (replicator->GetRemoteNetworkRole() == NetEntityRole::Autonomous ||
                            replicator->GetBoundLocalNetworkRole() == NetEntityRole::Autonomous)
                        {
                            toSendList.push_back(replicator);
                        }
                        else if (proxySendCount < m_replicationWindow->GetMaxProxyEntityReplicatorSendCount())
                        {
                            toSendList.push_back(replicator);
                            ++proxySendCount;
                        }
                    }
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

    void EntityReplicationManager::SendEntityUpdateMessages(EntityReplicatorList& replicatorList)
    {
        uint32_t pendingPacketSize = 0;
        EntityReplicatorList replicatorUpdatedList;
        NetworkEntityUpdateVector entityUpdates;
        // Serialize everything
        while (!replicatorList.empty())
        {
            EntityReplicator* replicator = replicatorList.front();
            NetworkEntityUpdateMessage updateMessage(replicator->GenerateUpdatePacket());

            const uint32_t nextMessageSize = updateMessage.GetEstimatedSerializeSize();

            // Check if we are over our limits
            const bool payloadFull = (pendingPacketSize + nextMessageSize > m_maxPayloadSize);
            const bool capacityReached = (entityUpdates.size() >= entityUpdates.capacity());
            const bool largeEntityDetected = (payloadFull && replicatorUpdatedList.empty());
            if (capacityReached || (payloadFull && !largeEntityDetected))
            {
                break;
            }

            pendingPacketSize += nextMessageSize;
            entityUpdates.push_back(updateMessage);
            replicatorUpdatedList.push_back(replicator);
            replicatorList.pop_front();

            if (largeEntityDetected)
            {
                AZLOG_WARN
                (
                    "Serializing extremely large entity (%llu) - MaxPayload: %d NeededSize %d",
                    aznumeric_cast<AZ::u64>(replicator->GetEntityHandle().GetNetEntityId()),
                    m_maxPayloadSize,
                    nextMessageSize
                );
                break;
            }
        }

        if (m_replicationWindow)
        {
            const AzNetworking::PacketId sentId = m_replicationWindow->SendEntityUpdateMessages(entityUpdates);

            // Update the sent things with the packet id
            for (EntityReplicator* replicator : replicatorUpdatedList)
            {
                replicator->FinalizeSerialization(sentId);
            }
        }
        else
        {
            AZ_Assert(false, "Failed to send entity update message, replication window does not exist");
        }
    }

    void EntityReplicationManager::SendEntityRpcs(RpcMessages& rpcMessages, bool reliable)
    {
        while (!rpcMessages.empty())
        {
            NetworkEntityRpcVector entityRpcs;
            uint32_t pendingPacketSize = 0;

            while (!rpcMessages.empty())
            {
                NetworkEntityRpcMessage& message = rpcMessages.front();

                const uint32_t nextRpcSize = message.GetEstimatedSerializeSize();

                if ((pendingPacketSize + nextRpcSize) > m_maxPayloadSize)
                {
                    // We're over our limit, break and send an Rpc packet
                    if (entityRpcs.size() == 0)
                    {
                        AZLOG(NET_Replicator, "Encountered an RPC that is above our MTU, message will be segmented (object size %u, max allowed size %u)", nextRpcSize, m_maxPayloadSize);
                        entityRpcs.push_back(message);
                        rpcMessages.pop_front();
                    }
                    break;
                }

                pendingPacketSize += nextRpcSize;
                if (entityRpcs.full())
                {
                    // Packet was full, send what we've accumulated so far
                    AZLOG(NET_Replicator, "We've hit our RPC message limit (RPC count %u, packet size %u)", aznumeric_cast<uint32_t>(entityRpcs.size()), pendingPacketSize);
                    break;
                }
                entityRpcs.push_back(message);
                rpcMessages.pop_front();
            }

            if (m_replicationWindow)
            {
                m_replicationWindow->SendEntityRpcs(entityRpcs, reliable);
            }
            else
            {
                AZ_Assert(false, "Failed to send entity rpc, replication window does not exist");
            }
        }
    }

    void EntityReplicationManager::SendEntityResets()
    {
        if (m_replicationWindow)
        {
            m_replicationWindow->SendEntityResets(m_replicatorsPendingReset);
        }
        m_replicatorsPendingReset.clear();
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
            m_replicatorsPendingReset.clear();
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
        if (entityHandle.GetEntity())
        {
            entityReplicator = GetEntityReplicator(entityHandle);
            if (entityReplicator)
            {
                // Check if we changed our remote role - this can happen during server entity migration.
                // Retain our replicator after migration until we are sure the other side has received all the packets (and we haven't had to do resends).
                // At this point, the remote host should inform us we've migrated prior to the timeout and cleanup of the old replicator
                const bool changedRemoteRole = (remoteNetworkRole != entityReplicator->GetRemoteNetworkRole());
                // Check if we've changed our bound local role - this can occur when we gain Autonomous or lose Autonomous on a client
                bool changedLocalRole(false);
                NetBindComponent* netBindComponent = entityReplicator->GetEntityHandle().GetNetBindComponent();
                if (netBindComponent != nullptr)
                {
                    changedLocalRole = (netBindComponent->GetNetEntityRole() != entityReplicator->GetBoundLocalNetworkRole());
                }

                if (changedRemoteRole || changedLocalRole)
                {
                    const AZ::u64 intEntityId = static_cast<AZ::u64>(netBindComponent->GetNetEntityId());
                    const char* entityName = entityReplicator->GetEntityHandle().GetEntity()->GetName().c_str();
                    if (changedLocalRole)
                    {
                        const char* oldRoleString = GetEnumString(entityReplicator->GetRemoteNetworkRole());
                        const char* newRoleString = GetEnumString(remoteNetworkRole);
                        AZLOG(NET_ReplicatorRoles, "Replicator %s(%llu) changed local role, old role = %s, new role = %s", entityName, intEntityId, oldRoleString, newRoleString);
                    }
                    if (changedRemoteRole)
                    {
                        const char* oldRoleString = GetEnumString(entityReplicator->GetBoundLocalNetworkRole());
                        const char* newRoleString = GetEnumString(netBindComponent->GetNetEntityRole());
                        AZLOG(NET_ReplicatorRoles, "Replicator %s(%llu) changed remote role, old role = %s, new role = %s", entityName, intEntityId, oldRoleString, newRoleString);
                    }

                    // If we changed roles, we need to reset everything
                    if (!entityReplicator->IsMarkedForRemoval())
                    {
                        // Clear our ownership
                        entityReplicator->MarkForRemoval();
                    }
                    // Reset our replicator, we are establishing a new one
                    entityReplicator->Reset(remoteNetworkRole);
                }
                // Else case is when an entity had left relevancy and come back (but it was still pending a removal)
                entityReplicator->Initialize(entityHandle);
                AZLOG
                (
                    NET_RepDeletes,
                    "Reinited replicator for netEntityId %llu from remote host %s role %d",
                    static_cast<AZ::u64>(entityHandle.GetNetEntityId()),
                    GetRemoteHostId().GetString().c_str(),
                    aznumeric_cast<int32_t>(remoteNetworkRole)
                );
            }
            else
            {
                // Haven't seen him before, let's add him
                AZ_Assert(entityHandle.GetNetBindComponent(), "No NetBindComponent");
                AZStd::unique_ptr<EntityReplicator> newEntityReplicator = AZStd::make_unique<EntityReplicator>(*this, &m_connection, remoteNetworkRole, entityHandle);
                newEntityReplicator->Initialize(entityHandle);
                entityReplicator = newEntityReplicator.get();
                m_entityReplicatorMap.emplace(entityHandle.GetNetEntityId(), AZStd::move(newEntityReplicator));
                AZLOG
                (
                    NET_RepDeletes,
                    "Added replicator for netEntityId %llu from remote host %s role %d",
                    static_cast<AZ::u64>(entityHandle.GetNetEntityId()),
                    GetRemoteHostId().GetString().c_str(),
                    aznumeric_cast<int32_t>(remoteNetworkRole)
                );
            }
        }
        else
        {
            AZLOG_ERROR("Failed to add entity replicator, entity does not exist, netEntityId %llu", static_cast<AZ::u64>(entityHandle.GetNetEntityId()));
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
    void EntityReplicationManager::AddAutonomousEntityReplicatorCreatedHandler(AZ::Event<NetEntityId>::Handler& handler)
    {
        handler.Connect(m_autonomousEntityReplicatorCreated);
    }

    void EntityReplicationManager::AddSendMigrateEntityEventHandler(SendMigrateEntityEvent::Handler& handler)
    {
        handler.Connect(m_sendMigrateEntityEvent);
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
                AZLOG_WARN("Entity replicator for id %llu is already marked for deletion on remote host %s", static_cast<AZ::u64>(updateMessage.GetEntityId()), GetRemoteHostId().GetString().c_str());
                return true;
            }
            else if (entityReplicator->OwnsReplicatorLifetime())
            {
                // This can occur if we migrate entities quickly - if this is a replicator from C to A, A migrates to B, B then migrates to C, and A's delete replicator has not arrived at C
                AZLOG(NET_RepDeletes, "Got a replicator delete message for a replicator we own id %llu remote host %s", static_cast<AZ::u64>(updateMessage.GetEntityId()), GetRemoteHostId().GetString().c_str());
            }
            else
            {
                shouldDeleteEntity = true;
                entityReplicator->MarkForRemoval();
                AZLOG(NET_RepDeletes, "Deleting replicater for entity id %llu remote host %s", static_cast<AZ::u64>(updateMessage.GetEntityId()), GetRemoteHostId().GetString().c_str());
            }
        }
        else
        {
            // Replicators are cleared on the server via ScheduledEvent. It's possible for redundant delete messages to be sent before the event fires.
            AZLOG(
                NET_RepDeletes,
                "Replicator for id %llu is null on remote host %s. It likely has already been deleted.",
                static_cast<AZ::u64>(updateMessage.GetEntityId()),
                GetRemoteHostId().GetString().c_str());
            return true;
        }

        // Handle entity cleanup
        if (shouldDeleteEntity)
        {
            ConstNetworkEntityHandle entity = GetNetworkEntityManager()->GetEntity(updateMessage.GetEntityId());
            if (entity)
            {
                if (updateMessage.GetWasMigrated())
                {
                    AZLOG(NET_RepDeletes, "Leaving id %llu using timeout remote host %s", static_cast<AZ::u64>(entity.GetNetEntityId()), GetRemoteHostId().GetString().c_str());
                }
                else
                {
                    AZLOG(NET_RepDeletes, "Deleting entity id %llu remote host %s", static_cast<AZ::u64>(entity.GetNetEntityId()), GetRemoteHostId().GetString().c_str());
                    GetNetworkEntityManager()->MarkForRemoval(entity);
                }
            }
            else
            {
                AZLOG(NET_RepDeletes, "Trying to delete entity id %llu remote host %s, but it has been removed", static_cast<AZ::u64>(entity.GetNetEntityId()), GetRemoteHostId().GetString().c_str());
            }
        }

        return shouldDeleteEntity;
    }

    bool EntityReplicationManager::HandlePropertyChangeMessage
    (
        AzNetworking::IConnection* invokingConnection,
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

        if (netBindComponent->GetOwningConnectionId() != invokingConnection->GetConnectionId())
        {
            // Always ensure our owning connectionId is correct for correct rewind behaviour
            netBindComponent->SetOwningConnectionId(invokingConnection->GetConnectionId());
        }

        const bool changeNetworkRole = (netBindComponent->GetNetEntityRole() != localNetworkRole);
        if (changeNetworkRole)
        {
            AZ_Assert(localNetworkRole != NetEntityRole::Authority, "UpdateMessage trying to set local role to Authority, this should only happen via migration");
            AZLOG_INFO
            (
                "EntityReplicationManager: Changing network role on entity %s(%llu), old role %s new role %s",
                replicatorEntity.GetEntity()->GetName().c_str(),
                aznumeric_cast<AZ::u64>(netEntityId),
                GetEnumString(netBindComponent->GetNetEntityRole()),
                GetEnumString(localNetworkRole)
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
                        "LocalShard=%s EntityId=%llu RemoteNetworkRole=%u BoundLocalNetworkRole=%u ActualNetworkRole=%u IsMarkedForRemoval=%s",
                        GetNetworkEntityManager()->GetHostId().GetString().c_str(),
                        aznumeric_cast<AZ::u64>(entityReplicator->GetEntityHandle().GetNetEntityId()),
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
                    else if (entityReplicator->GetRemoteNetworkRole() != NetEntityRole::Authority) // We expect the remote role to be NetEntityRole::Authority
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
                    AZLOG(NET_RepDeletes, "EntityReplicationManager: Received old DeleteProxy message for entity id %llu, sequence %d latest sequence %d from remote host %s",
                        (AZ::u64)updateMessage.GetEntityId(), (uint32_t)packetId, (uint32_t)propSubscriber->GetLastReceivedPacketId(), GetRemoteHostId().GetString().c_str());
                }
                else
                {
                    AZLOG(NET_RepUpdate, "EntityReplicationManager: Received old PropertyChangeMessage message for entity id %llu, sequence %d latest sequence %d from remote host %s",
                        (AZ::u64)updateMessage.GetEntityId(), (uint32_t)packetId, (uint32_t)propSubscriber->GetLastReceivedPacketId(), GetRemoteHostId().GetString().c_str());
                }
            }
        }
        return result;
    }

    bool EntityReplicationManager::HandleEntityUpdateMessage
    (
        AzNetworking::IConnection* invokingConnection,
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

        OutputSerializer outputSerializer(updateMessage.GetData()->GetBuffer(), static_cast<uint32_t>(updateMessage.GetData()->GetSize()));

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
                m_replicatorsPendingReset.emplace(updateMessage.GetEntityId());
                return true;
            }

            // Use the cached slice entry data from the entity replicator
            prefabEntityId = entityReplicator->GetPrefabEntityId();
        }

        // This may implicitly create a replicator for us
        bool handled = HandlePropertyChangeMessage(invokingConnection, entityReplicator, packetHeader.GetPacketId(), updateMessage.GetEntityId(), updateMessage.GetNetworkRole(), outputSerializer, prefabEntityId);
        AZ_Assert(handled, "Failed to handle NetworkEntityUpdateMessage message");

        return handled;
    }

    bool EntityReplicationManager::HandleEntityRpcMessages(AzNetworking::IConnection* invokingConnection, NetworkEntityRpcVector& rpcVector)
    {
        for (NetworkEntityRpcMessage& rpcMessage : rpcVector)
        {
            EntityReplicator* entityReplicator = GetEntityReplicator(rpcMessage.GetEntityId());
            const bool isReplicatorValid = (entityReplicator != nullptr) && !entityReplicator->IsMarkedForRemoval();
            const bool isEntityActivated = isReplicatorValid && entityReplicator->GetEntityHandle() && (entityReplicator->GetEntityHandle().GetEntity()->GetState() == AZ::Entity::State::Active);
            if (!isReplicatorValid || !isEntityActivated)
            {
                m_orphanedEntityRpcs.AddOrphanedRpc(rpcMessage.GetEntityId(), rpcMessage);
            }
            else
            {
                if (!entityReplicator->HandleRpcMessage(invokingConnection, rpcMessage))
                {
                    AZ_Assert(false, "Failed processing RPC messages, disconnecting");
                    return false;
                }
            }
        }
        return true;
    }

    bool EntityReplicationManager::HandleEntityResetMessages([[maybe_unused]] AzNetworking::IConnection* invokingConnection, const NetEntityIdsForReset& resetIds)
    {
        for (NetEntityId netEntityId : resetIds)
        {
            EntityReplicator* entityReplicator = GetEntityReplicator(netEntityId);
            if (entityReplicator != nullptr)
            {
                // Don't reset the remote role, we want to reset the publisher/subscriber
                entityReplicator->Reset(entityReplicator->GetRemoteNetworkRole());
            }
        }
        return true;
    }

    bool EntityReplicationManager::DispatchOrphanedRpc(NetworkEntityRpcMessage& message, EntityReplicator* entityReplicator)
    {
        if (entityReplicator == nullptr)
        {
            AZLOG_INFO
            (
                "EntityReplicationManager: Dropping remote RPC message for component %s of rpc index %s, entityId %llu has already been deleted",
                GetMultiplayerComponentRegistry()->GetComponentName(message.GetComponentId()),
                GetMultiplayerComponentRegistry()->GetComponentRpcName(message.GetComponentId(), message.GetRpcIndex()),
                static_cast<AZ::u64>(message.GetEntityId())
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

    void EntityReplicationManager::OrphanedEntityRpcs::Update()
    {
        m_timeoutQueue.UpdateTimeouts([this](AzNetworking::TimeoutQueue::TimeoutItem& item)
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
        });
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

    void EntityReplicationManager::SetRemoteEntityDomain(AZStd::unique_ptr<IEntityDomain> entityDomain)
    {
        m_remoteEntityDomain = AZStd::move(entityDomain);
    }

    IEntityDomain* EntityReplicationManager::GetRemoteEntityDomain()
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
                netBindComponent->NotifyServerMigration(GetRemoteHostId());
            }

            [[maybe_unused]] bool didSucceed = true;
            EntityMigrationMessage message;
            message.m_netEntityId = replicator->GetEntityHandle().GetNetEntityId();
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
                InputSerializer inputSerializer(message.m_propertyUpdateData.GetBuffer(), static_cast<uint32_t>(message.m_propertyUpdateData.GetCapacity()));
                if (needsNetworkPropertyUpdate)
                {
                    // Write out entity state into the buffer
                    propPublisher->UpdateSerialization(inputSerializer);
                }
                didSucceed &= inputSerializer.IsValid();
                message.m_propertyUpdateData.Resize(inputSerializer.GetSize());
            }
            AZ_Assert(didSucceed, "Failed to migrate entity from server");

            m_sendMigrateEntityEvent.Signal(m_connection, message);
            AZLOG(NET_RepDeletes, "Migration packet sent %llu to remote host %s", static_cast<AZ::u64>(netEntityId), GetRemoteHostId().GetString().c_str());

            // Notify all other EntityReplicationManagers that this entity has migrated so they can adjust their own replicators given our new proxy status
            GetMultiplayer()->SendNotifyEntityMigrationEvent(entityHandle, GetRemoteHostId());

            // Immediately add a new replicator so that we catch RPC invocations, the remote side will make us a new one, and then remove us if needs be
            AddEntityReplicator(entityHandle, NetEntityRole::Authority);
        }
    }

    bool EntityReplicationManager::HandleEntityMigration(AzNetworking::IConnection* invokingConnection, EntityMigrationMessage& message)
    {
        EntityReplicator* replicator = GetEntityReplicator(message.m_netEntityId);
        {
            if (message.m_propertyUpdateData.GetSize() > 0)
            {
                OutputSerializer outputSerializer(message.m_propertyUpdateData.GetBuffer(), static_cast<uint32_t>(message.m_propertyUpdateData.GetSize()));
                if (!HandlePropertyChangeMessage
                (
                    invokingConnection,
                    replicator,
                    AzNetworking::InvalidPacketId,
                    message.m_netEntityId,
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
            replicator = GetEntityReplicator(message.m_netEntityId);
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

        AZLOG(NET_RepDeletes, "Handle Migration %llu new authority from remote host %s", static_cast<AZ::u64>(entityHandle.GetNetEntityId()), GetRemoteHostId().GetString().c_str());
        return true;
    }

    void EntityReplicationManager::OnEntityExitDomain(const ConstNetworkEntityHandle& entityHandle)
    {
        if (CanMigrateEntity(entityHandle))
        {
            MigrateEntity(entityHandle.GetNetEntityId());
        }
    }

    void EntityReplicationManager::OnPostEntityMigration(const ConstNetworkEntityHandle& entityHandle, const HostId& remoteHostId)
    {
        if (remoteHostId == GetRemoteHostId())
        {
            // Don't handle self sent messages
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
        m_replicatorsPendingRemoval.emplace(replicator.GetEntityHandle().GetNetEntityId());
    }

    void EntityReplicationManager::AddReplicatorToPendingSend(const EntityReplicator& replicator)
    {
        m_replicatorsPendingSend.emplace(replicator.GetEntityHandle().GetNetEntityId());
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
