/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>
#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <Multiplayer/NetworkEntity/NetworkEntityRpcMessage.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicationManager.h>
#include <Source/NetworkEntity/NetworkEntityAuthorityTracker.h>
#include <Source/NetworkEntity/NetworkEntityTracker.h>
#include <Source/NetworkEntity/EntityReplication/PropertyPublisher.h>
#include <Source/NetworkEntity/EntityReplication/PropertySubscriber.h>

#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <AzNetworking/PacketLayer/IPacket.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

#include <AzFramework/Components/TransformComponent.h>

namespace Multiplayer
{
    void SyncActivatingEntityTransform(AZ::Entity* entity)
    {
        NetworkTransformComponent* netTransform = entity->FindComponent<NetworkTransformComponent>();
        AzFramework::TransformComponent* transformComponent = entity->FindComponent<AzFramework::TransformComponent>();

        if (netTransform && transformComponent)
        {
            AZ::Transform transform;
            transform.SetRotation(netTransform->GetRotation());
            transform.SetTranslation(netTransform->GetTranslation());
            transform.SetUniformScale(netTransform->GetScale());

            if (netTransform->GetParentEntityId() == InvalidNetEntityId)
            {
                transformComponent->SetWorldTM(transform);
            }
            else
            {
                transformComponent->SetLocalTM(transform);
            }
        }
    }

    EntityReplicator::EntityReplicator
    (
        EntityReplicationManager& replicationManager,
        AzNetworking::IConnection* connection,
        NetEntityRole remoteNetworkRole,
        const ConstNetworkEntityHandle& entityHandle
    )
        : m_replicationManager(replicationManager)
        , m_connection(connection)
        , m_entityHandle(entityHandle)
        , m_remoteNetworkRole(remoteNetworkRole)
        , m_onEntityDirtiedHandler([this]() { OnEntityDirtiedEvent(); })
        , m_onSendRpcHandler([this](NetworkEntityRpcMessage& entityRpcMessage) { OnSendRpcEvent(entityRpcMessage); })
        , m_onForwardRpcHandler([this](NetworkEntityRpcMessage& entityRpcMessage) { OnSendRpcEvent(entityRpcMessage); })
        , m_onSendAutonomousRpcHandler([this](NetworkEntityRpcMessage& entityRpcMessage) { OnSendRpcEvent(entityRpcMessage); })
        , m_onForwardAutonomousRpcHandler([this](NetworkEntityRpcMessage& entityRpcMessage) { OnSendRpcEvent(entityRpcMessage); })
        , m_onEntityStopHandler([this](const ConstNetworkEntityHandle&) { OnEntityRemovedEvent(); })
        , m_proxyRemovalEvent([this] { OnProxyRemovalTimedEvent(); }, AZ::Name("ProxyRemovalTimedEvent"))
    {
        if (m_entityHandle.GetEntity())
        {
            m_netBindComponent = m_entityHandle.GetNetBindComponent();
            m_boundLocalNetworkRole = m_netBindComponent->GetNetEntityRole();
        }
    }

    EntityReplicator::~EntityReplicator()
    {
        AZ::EntityBus::Handler::BusDisconnect();
    }

    void EntityReplicator::SetPrefabEntityId(const PrefabEntityId& prefabEntityId)
    {
        m_prefabEntityId = prefabEntityId;
        m_prefabEntityIdSet = true;
    }

    void EntityReplicator::Reset(NetEntityRole remoteNetworkRole)
    {
        AZ::EntityBus::Handler::BusDisconnect();

        m_remoteNetworkRole = remoteNetworkRole;

        m_propertyPublisher = nullptr;
        m_propertySubscriber = nullptr;

        m_wasMigrated = false;

        m_onSendRpcHandler.Disconnect();
        m_onForwardRpcHandler.Disconnect();
        m_onSendAutonomousRpcHandler.Disconnect();
        m_onForwardAutonomousRpcHandler.Disconnect();
        m_onEntityStopHandler.Disconnect();
    }

    void EntityReplicator::Initialize(const ConstNetworkEntityHandle& entityHandle)
    {
        AZ_Assert(entityHandle, "Empty handle passed to Initialize");

        m_entityHandle = entityHandle;
        if (m_entityHandle.GetEntity())
        {
            m_netBindComponent = m_entityHandle.GetNetBindComponent();
            AZ_Assert(m_netBindComponent, "No Multiplayer::NetBindComponent");
            m_boundLocalNetworkRole = m_netBindComponent->GetNetEntityRole();
            SetPrefabEntityId(m_netBindComponent->GetPrefabEntityId());
        }

        AZ_Assert
        (
            m_boundLocalNetworkRole != m_remoteNetworkRole,
            "Invalid configuration detected, bound local role must differ from remote network role: %s",
            GetEnumString(m_boundLocalNetworkRole)
        );

        if (RemoteManagerOwnsEntityLifetime())
        {
            // Make sure we don't have any outstanding entity migration timeouts since we now have a new replicator
            GetNetworkEntityAuthorityTracker()->AddEntityAuthorityManager(entityHandle, m_replicationManager.GetRemoteHostId());
        }

        // We got re-added
        m_proxyRemovalEvent.RemoveFromQueue();

        if (CanSendUpdates())
        {
            AZ_Assert(m_propertyPublisher == nullptr,
                "Initializing twice without a Reset() in-between, internal state might not have been cleared fully.");

            m_replicationManager.AddReplicatorToPendingSend(*this);
            m_propertyPublisher = AZStd::make_unique<PropertyPublisher>
                (
                    GetRemoteNetworkRole(),
                    !RemoteManagerOwnsEntityLifetime() ? PropertyPublisher::OwnsLifetime::True : PropertyPublisher::OwnsLifetime::False,
                    *m_connection
                );
            m_onEntityDirtiedHandler.Disconnect();
            m_netBindComponent->AddEntityDirtiedEventHandler(m_onEntityDirtiedHandler);
        }
        else
        {
            m_propertyPublisher = nullptr;
        }

        if (m_remoteNetworkRole == NetEntityRole::Authority ||
            m_remoteNetworkRole == NetEntityRole::Autonomous)
        {
            m_propertySubscriber = AZStd::make_unique<PropertySubscriber>(m_replicationManager, m_netBindComponent);
        }
        else
        {
            m_propertySubscriber = nullptr;
        }

        // Prepare event handlers
        if (m_entityHandle.GetEntity())
        {
            NetBindComponent* netBindComponent = m_entityHandle.GetNetBindComponent();
            AZ_Assert(netBindComponent, "No Multiplayer::NetBindComponent");
            m_onEntityStopHandler.Disconnect();
            netBindComponent->AddEntityStopEventHandler(m_onEntityStopHandler);
            AttachRPCHandlers();
            netBindComponent->NetworkActivated();
        }

        AZ_Assert(m_remoteNetworkRole != NetEntityRole::InvalidRole, "Trying to add an entity replicator with the remote role as invalid");
        AZ_Assert(m_boundLocalNetworkRole != NetEntityRole::InvalidRole, "Trying to add an entity replicator with the bound local role as invalid");

        m_wasMigrated = false;
    }

    void EntityReplicator::AttachRPCHandlers()
    {
        // Make sure all handlers are detached first
        m_onSendRpcHandler.Disconnect();
        m_onSendAutonomousRpcHandler.Disconnect();
        m_onForwardRpcHandler.Disconnect();
        m_onForwardAutonomousRpcHandler.Disconnect();

        if (m_entityHandle.GetEntity())
        {
            NetBindComponent* netBindComponent = m_entityHandle.GetNetBindComponent();
            AZ_Assert(netBindComponent, "No Multiplayer::NetBindComponent");

            switch (GetBoundLocalNetworkRole())
            {
            case NetEntityRole::Authority:
                if (GetRemoteNetworkRole() == NetEntityRole::Client || GetRemoteNetworkRole() == NetEntityRole::Autonomous)
                {
                    m_onSendRpcHandler.Connect(netBindComponent->GetSendAuthorityToClientRpcEvent());
                    if (GetRemoteNetworkRole() == NetEntityRole::Autonomous)
                    {
                        m_onSendAutonomousRpcHandler.Connect(netBindComponent->GetSendAuthorityToAutonomousRpcEvent());
                    }
                }
                else if (GetRemoteNetworkRole() == NetEntityRole::Server)
                {
                    m_onForwardRpcHandler.Connect(netBindComponent->GetSendAuthorityToClientRpcEvent());
                }
                break;
            case NetEntityRole::Server:
                if (GetRemoteNetworkRole() == NetEntityRole::Authority)
                {
                    m_onSendRpcHandler.Connect(netBindComponent->GetSendServerToAuthorityRpcEvent());
                    m_onForwardRpcHandler.Connect(netBindComponent->GetSendAuthorityToClientRpcEvent());
                    m_onForwardAutonomousRpcHandler.Connect(netBindComponent->GetSendAuthorityToAutonomousRpcEvent());
                }
                else if (GetRemoteNetworkRole() == NetEntityRole::Client)
                {
                    // Listen for these to forward the rpc along to the other Client replicators
                    m_onSendRpcHandler.Connect(netBindComponent->GetSendAuthorityToClientRpcEvent());
                }
                else if (GetRemoteNetworkRole() == NetEntityRole::Autonomous)
                {
                    // NOTE: Autonomous is not connected to ServerProxy, it is always connected to an Authority
                    AZ_Assert(false, "Unexpected autonomous remote role")
                }
                break;
            case NetEntityRole::Client:
                // Nothing allowed, no Client to Server communication
                break;
            case NetEntityRole::Autonomous:
                if (GetRemoteNetworkRole() == NetEntityRole::Authority)
                {
                    m_onSendRpcHandler.Connect(netBindComponent->GetSendAutonomousToAuthorityRpcEvent());
                }
                break;
            default:
                AZ_Assert(false, "Unexpected network role");
            }
        }
    }

    void EntityReplicator::ActivateNetworkEntity()
    {
        ActivateNetworkEntityInternal();
    }

    void EntityReplicator::OnEntityActivated(const AZ::EntityId&)
    {
        ActivateNetworkEntityInternal();
        AZ::EntityBus::Handler::BusDisconnect();
    }

    void EntityReplicator::OnEntityDestroyed(const AZ::EntityId&)
    {
        AZ::EntityBus::Handler::BusDisconnect();
    }

    void EntityReplicator::ActivateNetworkEntityInternal()
    {
        AZ_Assert(!m_propertyPublisher || !m_propertyPublisher->IsDeleting(), "Unexpectedly activating a deleted entity.");

        AZ::EntityBus::Handler::BusDisconnect();

        AZ::Entity* entity = GetEntityHandle().GetEntity();
        AZ_Assert(entity, "Entity replicator entity unexpectedly missing");

        if (entity->GetState() != AZ::Entity::State::Init)
        {
            AZLOG_WARN("Trying to activate an entity that is not in the Init state (%llu)", static_cast<AZ::u64>(GetEntityHandle().GetNetEntityId()));
        }

        SyncActivatingEntityTransform(entity);

        entity->Activate();

        m_replicationManager.m_orphanedEntityRpcs.DispatchOrphanedRpcs(*this);
    }

    bool EntityReplicator::CanSendUpdates()
    {
        bool ret(false);
        if (GetEntityHandle().GetEntity())
        {
            NetBindComponent* netBindComponent = m_netBindComponent;
            AZ_Assert(netBindComponent, "No Multiplayer::NetBindComponent");

            bool isAuthority = (GetBoundLocalNetworkRole() == NetEntityRole::Authority) && (GetBoundLocalNetworkRole() == netBindComponent->GetNetEntityRole());
            bool isClient = GetRemoteNetworkRole() == NetEntityRole::Client;
            bool isAutonomous = GetBoundLocalNetworkRole() == NetEntityRole::Autonomous;
            if (isAuthority || isClient || isAutonomous)
            {
                ret = true;
            }
        }
        return ret;
    }

    bool EntityReplicator::OwnsReplicatorLifetime() const
    {
        bool ret(false);
        if (GetBoundLocalNetworkRole() == NetEntityRole::Authority  // Authority always owns lifetime
            || (GetBoundLocalNetworkRole() == NetEntityRole::Server // Server also owns lifetime if the remote endpoint is a client of some form
                && (GetRemoteNetworkRole() == NetEntityRole::Client
                ||  GetRemoteNetworkRole() == NetEntityRole::Autonomous)))
        {
            ret = true;
        }
        return ret;
    }

    bool EntityReplicator::RemoteManagerOwnsEntityLifetime() const
    {
        bool isServer = (GetBoundLocalNetworkRole() == NetEntityRole::Server)
                     && (GetRemoteNetworkRole()     == NetEntityRole::Authority);
        bool isClient = (GetBoundLocalNetworkRole() == NetEntityRole::Client)
                     || (GetBoundLocalNetworkRole() == NetEntityRole::Autonomous);
        return isServer || isClient;
    }

    void EntityReplicator::MarkForRemoval()
    {
        // NOTE: It's possible that heading into this function, m_entityHandle is already invalid if this was triggered
        // via OnEntityRemoved.

        AZLOG(NET_RepDeletes, "Marking entity %llu for removal.", aznumeric_cast<AZ::u64>(m_entityHandle.GetNetEntityId()));

        AZ::EntityBus::Handler::BusDisconnect();

        if (RemoteManagerOwnsEntityLifetime())
        {
            GetNetworkEntityAuthorityTracker()->RemoveEntityAuthorityManager(m_entityHandle, m_replicationManager.GetRemoteHostId());
        }

        ClearPendingRemoval();

        if (m_propertyPublisher)
        {
            // We don't need to remove the same entity multiple times.
            if (m_propertyPublisher->IsDeleting())
            {
                AZLOG_WARN("Trying to delete entity %llu multiple times", static_cast<AZ::u64>(GetEntityHandle().GetNetEntityId()));
                return;
            }

            m_propertyPublisher->SetDeleting();
            m_onEntityDirtiedHandler.Disconnect();

            // Cache the delete packet here, since the data will no longer be available after this point.
            if (m_propertyPublisher->CacheDeletePacket(m_netBindComponent, WasMigrated()))
            {
                AZLOG(NET_RepDeletes, "Caching delete message for entity %llu.", aznumeric_cast<AZ::u64>(m_entityHandle.GetNetEntityId()));

                // Only send the delete message if one was cached. Otherwise, no delete message is necessary, the client
                // never added the entity.
                m_replicationManager.AddReplicatorToPendingSend(*this);
                m_replicationManager.AddReplicatorToPendingRemoval(*this);
            }
            else
            {
                if (m_propertyPublisher->IsDeleted())
                {
                    AZLOG(NET_RepDeletes, "Trying to cache second delete message for entity %llu.",
                        aznumeric_cast<AZ::u64>(m_entityHandle.GetNetEntityId()));
                }
                else
                {
                    AZLOG(
                        NET_RepDeletes, "Dropping delete message, never sent add for entity %llu.",
                        aznumeric_cast<AZ::u64>(m_entityHandle.GetNetEntityId()));

                    // It's possible that adds/updates have already added this replicator to the pending send list.
                    // If we've also got a delete before we've sent anything, then remove the replicator from the send queue.
                    m_replicationManager.RemoveReplicatorFromPendingSend(*this);
                }
            }
        }
        else if (m_propertySubscriber)
        {
            // We don't need to remove the same entity multiple times.
            if (m_propertySubscriber->IsDeleting())
            {
                AZLOG_WARN("Trying to delete entity %llu multiple times", static_cast<AZ::u64>(GetEntityHandle().GetNetEntityId()));
                return;
            }

            m_propertySubscriber->SetDeleting();
            m_replicationManager.AddReplicatorToPendingRemoval(*this);
        }

        m_onForwardRpcHandler.Disconnect();
        m_onForwardAutonomousRpcHandler.Disconnect();

        m_onEntityStopHandler.Disconnect();
    }

    bool EntityReplicator::IsMarkedForRemoval() const
    {
        if (m_propertyPublisher)
        {
            return m_propertyPublisher->IsDeleting();
        }
        else if (m_propertySubscriber)
        {
            return m_propertySubscriber->IsDeleting();
        }
        AZLOG_WARN("Encountered netentity marked for removal that is not properly bound");
        return true;
    }

    void EntityReplicator::SetPendingRemoval(AZ::TimeMs pendingRemovalTimeMs)
    {
        AZ_Assert(m_propertyPublisher, "Only valid if we are publishing updates");

        // Don't set a pending removal on an entity that's already been removed.
        if (m_propertyPublisher->IsDeleting())
        {
            return;
        }

        if (pendingRemovalTimeMs > AZ::Time::ZeroTimeMs)
        {
            AZLOG(
                NET_RepDeletes,
                "Setting pending removal for entity %llu with time %lld ms.",
                aznumeric_cast<AZ::u64>(m_entityHandle.GetNetEntityId()),
                aznumeric_cast<AZ::s64>(pendingRemovalTimeMs));

            if (!IsPendingRemoval())
            {
                m_proxyRemovalEvent.Enqueue(pendingRemovalTimeMs);
            }
        }
        else
        {
            MarkForRemoval();
        }
    }

    bool EntityReplicator::IsPendingRemoval() const
    {
        return m_proxyRemovalEvent.IsScheduled();
    }

    void EntityReplicator::ClearPendingRemoval()
    {
        if (IsPendingRemoval())
        {
            AZLOG(NET_RepDeletes, "Clearing pending removal for entity %llu.", aznumeric_cast<AZ::u64>(m_entityHandle.GetNetEntityId()));
        }

        m_proxyRemovalEvent.RemoveFromQueue();
    }

    bool EntityReplicator::IsDeletionAcknowledged() const
    {
        // We sent the delete message, make sure it gets there
        if (m_propertyPublisher)
        {
            return m_propertyPublisher->IsDeleted();
        }
        else if (m_propertySubscriber)
        {
            return m_propertySubscriber->IsDeleted();
        }
        AZLOG_WARN("Encountered netentity marked for removal that is not properly bound");
        return true;
    }

    AZ::TimeMs EntityReplicator::GetResendTimeoutTimeMs() const
    {
        return m_replicationManager.GetResendTimeoutTimeMs();
    }

    bool EntityReplicator::IsReadyToActivate() const
    {
        const AZ::Entity* entity = m_entityHandle.GetEntity();
        AZ_Assert(entity, "Entity replicator entity unexpectedly missing");

        const NetworkHierarchyChildComponent* hierarchyChildComponent = entity->FindComponent<NetworkHierarchyChildComponent>();
        const NetworkHierarchyRootComponent* hierarchyRootComponent = nullptr;

        if (hierarchyChildComponent == nullptr)
        {
            // Child and root hierarchy components are mutually exclusive
            hierarchyRootComponent = entity->FindComponent<NetworkHierarchyRootComponent>();
        }

        if ((hierarchyChildComponent && hierarchyChildComponent->IsHierarchicalChild())
         || (hierarchyRootComponent && hierarchyRootComponent->IsHierarchicalChild()))
        {
            // If hierarchy is enabled for the entity, check if the parent is available
            if (const NetworkTransformComponent* networkTransform = entity->FindComponent<NetworkTransformComponent>())
            {
                const NetEntityId parentId = networkTransform->GetParentEntityId();
                // For root entities attached to a level, a network parent won't be set.
                // In this case, this entity is the root entity of the hierarchy and it will be activated first.
                if (parentId != InvalidNetEntityId)
                {
                    ConstNetworkEntityHandle parentHandle = GetNetworkEntityManager()->GetEntity(parentId);

                    const AZ::Entity* parentEntity = parentHandle.GetEntity();
                    if (parentEntity && parentEntity->GetState() == AZ::Entity::State::Active)
                    {
                        AZLOG
                        (
                            NET_HierarchyActivationInfo,
                            "Hierchical entity %s asking for activation - granted",
                            entity->GetName().c_str()
                        );
                        return true;
                    }

                    AZLOG
                    (
                        NET_HierarchyActivationInfo,
                        "Hierchical entity %s asking for activation - waiting on the parent %llu",
                        entity->GetName().c_str(),
                        aznumeric_cast<AZ::u64>(parentId)
                    );
                    return false;
                }
            }
        }

        return true;
    }

    NetworkEntityUpdateMessage EntityReplicator::GenerateUpdatePacket()
    {
        AZ_Assert(m_propertyPublisher, "Expected to have a property publisher");
        if (!m_propertyPublisher)
        {
            return {};
        }

        auto message = m_propertyPublisher->GenerateUpdatePacket(m_netBindComponent, WasMigrated());
        if (message.GetIsDelete())
        {
            AZLOG(
                NET_RepDeletes,
                "Sending delete replicator id %llu migrated=%s to remote host %s",
                aznumeric_cast<AZ::u64>(GetEntityHandle().GetNetEntityId()),
                WasMigrated() ? "true" : "false",
                m_replicationManager.GetRemoteHostId().GetString().c_str());
        }
        return message;
    }

    EntityMigrationMessage EntityReplicator::GenerateMigrationPacket()
    {
        AZ_Assert(m_propertyPublisher, "Expected to have a property publisher");
        return m_propertyPublisher ? m_propertyPublisher->GenerateMigrationPacket(m_netBindComponent) : EntityMigrationMessage();
    }

    bool EntityReplicator::IsReadyToPublish() const
    {
        return (m_propertyPublisher != nullptr);
    }

    bool EntityReplicator::IsRemoteReplicatorEstablished() const
    {
        AZ_Assert(m_propertyPublisher, "Expected to have a property publisher");
        return m_propertyPublisher ? m_propertyPublisher->IsRemoteReplicatorEstablished() : false;
    }

    bool EntityReplicator::HasChangesToPublish()
    {
        AZ_Assert(m_propertyPublisher, "Expected to have a property publisher");
        return m_propertyPublisher ? m_propertyPublisher->RequiresSerialization() : false;
    }

    void EntityReplicator::SetRebasing()
    {
        AZ_Assert(m_propertyPublisher, "Expected to have a property publisher");
        if (m_propertyPublisher)
        {
            m_propertyPublisher->SetRebasing();
        }
    }

    bool EntityReplicator::IsPacketIdValid(AzNetworking::PacketId packetId) const
    {
        AZ_Assert(m_propertySubscriber, "Expected to have a property subscriber.");
        return m_propertySubscriber ? m_propertySubscriber->IsPacketIdValid(packetId) : false;
    }

    AzNetworking::PacketId EntityReplicator::GetLastReceivedPacketId() const
    {
        AZ_Assert(m_propertySubscriber, "Expected to have a property subscriber.");
        return m_propertySubscriber ? m_propertySubscriber->GetLastReceivedPacketId() : AzNetworking::InvalidPacketId;
    }

    bool EntityReplicator::HandlePropertyChangeMessage(
        AzNetworking::PacketId packetId, AzNetworking::ISerializer* serializer, bool notifyChanges)
    {
        AZ_Assert(m_propertySubscriber, "Expected to have a property subscriber.");
        return m_propertySubscriber ? m_propertySubscriber->HandlePropertyChangeMessage(packetId, serializer, notifyChanges) : false;
    }

    bool EntityReplicator::PrepareToGenerateUpdatePacket()
    {
        AZ_Assert(m_propertyPublisher, "Expected to have a property publisher");

        // Otherwise, let the property publisher determine if any data should be sent.
        if (m_propertyPublisher)
        {
            m_propertyPublisher->UpdatePendingRecord(m_netBindComponent);
            return m_propertyPublisher->PrepareSerialization(m_netBindComponent);
        }

        return false;
    }

    void EntityReplicator::RecordSentPacketId(AzNetworking::PacketId sentId)
    {
        AZ_Assert(m_propertyPublisher, "Expected to have a property publisher");
        if (m_propertyPublisher)
        {
            m_propertyPublisher->FinalizeSerialization(sentId);
        }
    }

    void EntityReplicator::DeferRpcMessage(NetworkEntityRpcMessage& entityRpcMessage)
    {
        // Received rpc metrics, log rpc sent, number of bytes, and the componentId/rpcId for bandwidth metrics
        MultiplayerStats& stats = GetMultiplayer()->GetStats();
        stats.RecordRpcSent(GetEntityHandle().GetEntity()->GetId(), GetEntityHandle().GetEntity()->GetName().c_str(),
            entityRpcMessage.GetComponentId(), entityRpcMessage.GetRpcIndex(), entityRpcMessage.GetEstimatedSerializeSize());

        m_replicationManager.AddDeferredRpcMessage(entityRpcMessage);
    }

    void EntityReplicator::OnSendRpcEvent(NetworkEntityRpcMessage& entityRpcMessage)
    {
        if (IsMarkedForRemoval() && GetNetworkEntityAuthorityTracker()->DoesEntityHaveOwner(GetEntityHandle()))
        {
            // The remote end no longer owns this entity, so don't try and send to it (let another replicator send to it)
            return;
        }
        if (m_isForwardingRpc)
        {
            return;
        }

        if (m_entityHandle.GetEntity())
        {
            DeferRpcMessage(entityRpcMessage);
        }
    }

    void EntityReplicator::OnEntityDirtiedEvent()
    {
        AZ_Assert(m_propertyPublisher, "Expected to have a publisher, did we forget to disconnect?");
        if (m_propertyPublisher != nullptr)
        {
            m_propertyPublisher->UpdatePendingRecord(m_netBindComponent);
            m_replicationManager.AddReplicatorToPendingSend(*this);
        }
    }

    void EntityReplicator::OnEntityRemovedEvent()
    {
        MarkForRemoval();

        // The net bind component won't be valid after this event completes, so clear it out.
        m_netBindComponent = nullptr;
    }

    void EntityReplicator::OnProxyRemovalTimedEvent()
    {
        MarkForRemoval();
    }

    EntityReplicator::RpcValidationResult EntityReplicator::ValidateRpcMessage(const NetworkEntityRpcMessage& entityRpcMessage) const
    {
        RpcValidationResult result = RpcValidationResult::DropRpcAndDisconnect;
        switch (entityRpcMessage.GetRpcDeliveryType())
        {
        case RpcDeliveryType::AuthorityToClient:
            if (((GetBoundLocalNetworkRole() == NetEntityRole::Client) || (GetBoundLocalNetworkRole() == NetEntityRole::Autonomous))
                && (GetRemoteNetworkRole() == NetEntityRole::Authority))
            {
                // We are a local client, and we are connected to server, aka AuthorityToClient
                result = RpcValidationResult::HandleRpc;
            }
            if ((GetBoundLocalNetworkRole() == NetEntityRole::Server) && (GetRemoteNetworkRole() == NetEntityRole::Authority))
            {
                // We are on a server, and we received this message from another server, therefore we should forward this to any connected clients
                result = RpcValidationResult::ForwardToClient;
            }
            break;
        case RpcDeliveryType::AuthorityToAutonomous:
            if ((GetBoundLocalNetworkRole() == NetEntityRole::Autonomous) && (GetRemoteNetworkRole() == NetEntityRole::Authority))
            {
                // We are an autonomous client, and we are connected to server, aka AuthorityToAutonomous
                result = RpcValidationResult::HandleRpc;
            }
            if ((GetBoundLocalNetworkRole() == NetEntityRole::Authority) && (GetRemoteNetworkRole() == NetEntityRole::Server))
            {
                // We are on a server, and we received this message from another server, therefore we should forward this to our autonomous player
                // This can occur if we've recently migrated
                result = RpcValidationResult::ForwardToAutonomous;
            }
            break;
        case RpcDeliveryType::AutonomousToAuthority:
            if ((GetBoundLocalNetworkRole() == NetEntityRole::Authority) && (GetRemoteNetworkRole() == NetEntityRole::Autonomous))
            {
                if (IsMarkedForRemoval())
                {
                    // we've likely migrated, forward if the message is reliable
                    if (entityRpcMessage.GetReliability() == ReliabilityType::Reliable)
                    {
                        // We only forward messages that should be reliable
                        result = RpcValidationResult::ForwardToAuthority;
                    }
                    else
                    {
                        // this isn't reliable, so we can just drop it
                        result = RpcValidationResult::DropRpc;
                    }
                }
                else
                {
                    // We are on a server, and we got a message from the autonomous, aka AutonomousToAuthority, so handle
                    result = RpcValidationResult::HandleRpc;
                }
            }
            break;
        case RpcDeliveryType::ServerToAuthority:
            if ((GetBoundLocalNetworkRole() == NetEntityRole::Authority) && (GetRemoteNetworkRole() == NetEntityRole::Server))
            {
                // if we're marked for removal, then we should forward to whomever now owns this entity
                if (IsMarkedForRemoval())
                {
                    // we've likely migrated, forward if the message is reliable
                    if (entityRpcMessage.GetReliability() == ReliabilityType::Reliable)
                    {
                        // We only forward messages that should be reliable
                        result = RpcValidationResult::ForwardToAuthority;
                    }
                    else
                    {
                        // this isn't reliable, so we can just drop it
                        result = RpcValidationResult::DropRpc;
                    }
                }
                else
                {
                    // We are the authority, and we got this message from a server proxy, aka ServerToAuthority, so handle
                    result = RpcValidationResult::HandleRpc;
                }
            }
            break;
        }

        if (result == RpcValidationResult::DropRpcAndDisconnect)
        {
            bool isLocalServer = (GetBoundLocalNetworkRole() == NetEntityRole::Authority) || (GetBoundLocalNetworkRole() == NetEntityRole::Server);
            bool isRemoteServer = (GetRemoteNetworkRole() == NetEntityRole::Authority) || (GetRemoteNetworkRole() == NetEntityRole::Server);
            if (isLocalServer && isRemoteServer)
            {
                // Demote this to just a drop message, we didn't want to handle the message, but we don't want to drop the connection
                result = EntityReplicator::RpcValidationResult::DropRpc;
            }
            else
            {
                AZLOG_ERROR
                (
                    "Dropping RPC and Connection EntityId=%llu LocalRole=%s RemoteRole=%s RpcDeliveryType=%u RpcName=%s IsReliable=%s IsMarkedForRemoval=%s",
                    aznumeric_cast<AZ::u64>(m_entityHandle.GetNetEntityId()),
                    GetEnumString(GetBoundLocalNetworkRole()),
                    GetEnumString(GetRemoteNetworkRole()),
                    aznumeric_cast<uint32_t>(entityRpcMessage.GetRpcDeliveryType()),
                    GetMultiplayerComponentRegistry()->GetComponentRpcName(entityRpcMessage.GetComponentId(), entityRpcMessage.GetRpcIndex()),
                    entityRpcMessage.GetReliability() == ReliabilityType::Reliable ? "true" : "false",
                    IsMarkedForRemoval() ? "true" : "false"
                );
            }
        }

        if (result == RpcValidationResult::DropRpc)
        {
            AZLOG
            (
                NET_Rpc,
                "Dropping RPC EntityId=%llu LocalRole=%s RemoteRole=%s RpcDeliveryType=%u RpcName=%s IsReliable=%s IsMarkedForRemoval=%s",
                aznumeric_cast<AZ::u64>(m_entityHandle.GetNetEntityId()),
                GetEnumString(GetBoundLocalNetworkRole()),
                GetEnumString(GetRemoteNetworkRole()),
                aznumeric_cast<uint32_t>(entityRpcMessage.GetRpcDeliveryType()),
                GetMultiplayerComponentRegistry()->GetComponentRpcName(entityRpcMessage.GetComponentId(), entityRpcMessage.GetRpcIndex()),
                entityRpcMessage.GetReliability() == ReliabilityType::Reliable ? "true" : "false",
                IsMarkedForRemoval() ? "true" : "false"
            );
        }
        return result;
    }

    bool EntityReplicator::HandleRpcMessage(AzNetworking::IConnection* invokingConnection, NetworkEntityRpcMessage& entityRpcMessage)
    {
        // Received rpc metrics, log rpc received, time spent, number of bytes, and the componentId/rpcId for bandwidth metrics
        MultiplayerStats& stats = GetMultiplayer()->GetStats();
        stats.RecordRpcReceived(GetEntityHandle().GetEntity()->GetId(), GetEntityHandle().GetEntity()->GetName().c_str(),
            entityRpcMessage.GetComponentId(), entityRpcMessage.GetRpcIndex(), entityRpcMessage.GetEstimatedSerializeSize());

        if (!m_netBindComponent)
        {
            AZLOG_WARN
            (
                "Dropping RPC since entity deleted EntityId=%llu LocalRole=%s RemoteRole=%s RpcDeliveryType=%u RpcName=%s IsReliable=%s IsMarkedForRemoval=%s",
                aznumeric_cast<AZ::u64>(m_entityHandle.GetNetEntityId()),
                GetEnumString(GetBoundLocalNetworkRole()),
                GetEnumString(GetRemoteNetworkRole()),
                aznumeric_cast<uint32_t>(entityRpcMessage.GetRpcDeliveryType()),
                GetMultiplayerComponentRegistry()->GetComponentRpcName(entityRpcMessage.GetComponentId(), entityRpcMessage.GetRpcIndex()),
                entityRpcMessage.GetReliability() == ReliabilityType::Reliable ? "true" : "false",
                IsMarkedForRemoval() ? "true" : "false"
            );
            return false;
        }

        // When we forward a message, we'll likely hit the this entity replicator again (since it's already listening on the RPC events)
        // Therefore, we need to ignore the re-entrant case.
        class ScopedForwardingMessage
        {
        public:
            ScopedForwardingMessage(EntityReplicator& replicator)
                : m_replicator(replicator)
            {
                m_isForwardingCache = m_replicator.m_isForwardingRpc;
                m_replicator.m_isForwardingRpc = true;
            }
            ~ScopedForwardingMessage()
            {
                m_replicator.m_isForwardingRpc = m_isForwardingCache;
            }
            bool m_isForwardingCache = false;
            EntityReplicator& m_replicator;
        };

        // First validate the message with local & remote roles
        RpcValidationResult result = ValidateRpcMessage(entityRpcMessage);

        switch (result)
        {
        case RpcValidationResult::HandleRpc:
            return m_netBindComponent->HandleRpcMessage(invokingConnection, GetRemoteNetworkRole(), entityRpcMessage);
        case RpcValidationResult::DropRpc:
            return true;
        case RpcValidationResult::DropRpcAndDisconnect:
            return false;
        case RpcValidationResult::ForwardToClient:
            {
                ScopedForwardingMessage forwarding(*this);
                m_netBindComponent->GetSendAuthorityToClientRpcEvent().Signal(entityRpcMessage);
            }
            return true;
        case RpcValidationResult::ForwardToAutonomous:
            {
                ScopedForwardingMessage forwarding(*this);
                m_netBindComponent->GetSendAuthorityToAutonomousRpcEvent().Signal(entityRpcMessage);
            }
            return true;
        case RpcValidationResult::ForwardToAuthority:
            {
                ScopedForwardingMessage forwarding(*this);
                m_netBindComponent->GetSendServerToAuthorityRpcEvent().Signal(entityRpcMessage);
            }
            return true;
        default:
            break;
        }

        AZ_Assert(false, "Unhandled RpcValidationResult %d", result);
        return false;
    }
} // namespace Multiplayer
