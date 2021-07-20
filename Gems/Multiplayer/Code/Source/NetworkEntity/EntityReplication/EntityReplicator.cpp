/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/NetworkEntity/EntityReplication/EntityReplicator.h>
#include <Source/NetworkEntity/EntityReplication/EntityReplicationManager.h>
#include <Source/NetworkEntity/EntityReplication/PropertyPublisher.h>
#include <Source/NetworkEntity/EntityReplication/PropertySubscriber.h>
#include <Source/NetworkEntity/NetworkEntityAuthorityTracker.h>
#include <Source/NetworkEntity/NetworkEntityTracker.h>
#include <Source/AutoGen/Multiplayer.AutoPackets.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <Multiplayer/NetworkEntity/NetworkEntityRpcMessage.h>

#include <AzNetworking/PacketLayer/IPacket.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/ILogger.h>

#include <AzFramework/Components/TransformComponent.h>

namespace Multiplayer
{
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
        , m_onEntityStopHandler([this](const ConstNetworkEntityHandle &) { OnEntityRemovedEvent(); })
        , m_proxyRemovalEvent([this] { OnProxyRemovalTimedEvent(); }, AZ::Name("ProxyRemovalTimedEvent"))
    {
        if (auto localEnt = m_entityHandle.GetEntity())
        {
            m_netBindComponent = localEnt->FindComponent<NetBindComponent>();
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
        if (auto localEntity = m_entityHandle.GetEntity())
        {
            m_netBindComponent = localEntity->FindComponent<NetBindComponent>();
            AZ_Assert(m_netBindComponent, "No Multiplayer::NetBindComponent");
            m_boundLocalNetworkRole = m_netBindComponent->GetNetEntityRole();
            SetPrefabEntityId(m_netBindComponent->GetPrefabEntityId());
        }

        AZ_Assert
        (
            m_boundLocalNetworkRole != m_remoteNetworkRole,
            "Invalid configuration detected, bound local role must differ from remote network role Role: %d",
            aznumeric_cast<int32_t>(m_boundLocalNetworkRole)
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
            m_replicationManager.AddReplicatorToPendingSend(*this);
            m_propertyPublisher = AZStd::make_unique<PropertyPublisher>
            (
                GetRemoteNetworkRole(),
                !RemoteManagerOwnsEntityLifetime() ? PropertyPublisher::OwnsLifetime::True : PropertyPublisher::OwnsLifetime::False,
                m_netBindComponent,
                *m_connection
            );
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
        if (auto localEntity = m_entityHandle.GetEntity())
        {
            NetBindComponent* netBindComponent = localEntity->FindComponent<NetBindComponent>();
            AZ_Assert(netBindComponent, "No Multiplayer::NetBindComponent");
            netBindComponent->AddEntityStopEventHandler(m_onEntityStopHandler);
            AttachRPCHandlers();
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

        if (auto localEntity = m_entityHandle.GetEntity())
        {
            NetBindComponent* netBindComponent = localEntity->FindComponent<NetBindComponent>();
            AZ_Assert(netBindComponent, "No Multiplayer::NetBindComponent");

            switch (GetBoundLocalNetworkRole())
            {
            case NetEntityRole::Authority:
            {
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
            }
            break;
            case NetEntityRole::Server:
            {
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
                // NOTE: e_Autonomous is not connected to e_ServerProxy, it is always connected to an e_Authority
                AZ_Assert(GetRemoteNetworkRole() != NetEntityRole::Autonomous, "Unexpected autonomous remote role")
            }
            break;
            case NetEntityRole::Client:
            {
                // Nothing allowed, no Client to Server communication
            }
            break;
            case NetEntityRole::Autonomous:
            {
                if (GetRemoteNetworkRole() == NetEntityRole::Authority)
                {
                    m_onSendRpcHandler.Connect(netBindComponent->GetSendAutonomousToAuthorityRpcEvent());
                }
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
        AZ::EntityBus::Handler::BusDisconnect();

        AZ::Entity* entity = GetEntityHandle().GetEntity();
        AZ_Assert(entity, "Entity replicator entity unexpectedly missing");

        if (entity->GetState() != AZ::Entity::State::Init)
        {
            AZLOG_WARN("Trying to activate an entity that is not in the Init state (%u)", GetEntityHandle().GetNetEntityId());
        }

        // First we need to make sure the transform component has been updated with the correct value prior to activation
        // This is because vanilla az components may only depend on the transform component, not the multiplayer transform component
        //if (auto* locationComponent = FindCommonComponent<LocationComponent::Common>(GetEntityHandle()))
        //{
        //    AZ::Transform newTransform = locationComponent->GetTransform();
        //    auto* transformComponent = entity->FindComponent<AzFramework::TransformComponent>();
        //    if (transformComponent)
        //    {
        //        // We can't use EBus here since the TransFormBus does not get connected until the activate call below
        //        transformComponent->SetWorldTM(newTransform);
        //    }
        //}
        // Ugly, but this is the only time we need to call a non-const function on this entity
        entity->Activate();

        m_replicationManager.m_orphanedEntityRpcs.DispatchOrphanedRpcs(*this);
    }

    bool EntityReplicator::CanSendUpdates()
    {
        bool ret(false);
        if (auto localEnt = GetEntityHandle().GetEntity())
        {
            NetBindComponent* netBindComponent = m_netBindComponent;
            AZ_Assert(netBindComponent, "No Multiplayer::NetBindComponent");

            bool isAuthority = (GetBoundLocalNetworkRole() == NetEntityRole::Authority)
                            && (GetBoundLocalNetworkRole() == netBindComponent->GetNetEntityRole());
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
        if (GetBoundLocalNetworkRole() == NetEntityRole::Authority
            || (GetBoundLocalNetworkRole() == NetEntityRole::Server
                && (GetRemoteNetworkRole() == NetEntityRole::Client
                    || GetRemoteNetworkRole() == NetEntityRole::Autonomous)))
        {
            ret = true;
        }
        return ret;
    }

    bool EntityReplicator::RemoteManagerOwnsEntityLifetime() const
    {
        bool ret(false);
        bool isServer = (GetBoundLocalNetworkRole() == NetEntityRole::Server)
                     && (GetRemoteNetworkRole() == NetEntityRole::Authority);
        bool isClient = (GetBoundLocalNetworkRole() == NetEntityRole::Client)
                     || (GetBoundLocalNetworkRole() == NetEntityRole::Autonomous);
        if (isServer || isClient)
        {
            ret = true;
        }
        return ret;
    }

    void EntityReplicator::MarkForRemoval()
    {
        AZ::EntityBus::Handler::BusDisconnect();

        if (RemoteManagerOwnsEntityLifetime())
        {
            GetNetworkEntityAuthorityTracker()->RemoveEntityAuthorityManager(m_entityHandle, m_replicationManager.GetRemoteHostId());
        }

        ClearPendingRemoval();

        if (m_propertyPublisher)
        {
            m_propertyPublisher->SetDeleting();
            m_replicationManager.AddReplicatorToPendingSend(*this);
            m_onEntityDirtiedHandler.Disconnect();
        }
        else if (m_propertySubscriber)
        {
            m_propertySubscriber->SetDeleting();
        }

        m_replicationManager.AddReplicatorToPendingRemoval(*this);

        m_onForwardRpcHandler.Disconnect();
        m_onForwardAutonomousRpcHandler.Disconnect();

        m_onEntityStopHandler.Disconnect();
    }

    bool EntityReplicator::IsMarkedForRemoval() const
    {
        bool ret(true);
        if (m_propertyPublisher)
        {
            ret = m_propertyPublisher->IsDeleting();
        }
        else
        {
            AZ_Assert(m_propertySubscriber, "Expected to have at least a subscriber when deleting");
            ret = m_propertySubscriber->IsDeleting();
        }
        return ret;
    }

    void EntityReplicator::SetPendingRemoval(AZ::TimeMs pendingRemovalTimeMs)
    {
        AZ_Assert(m_propertyPublisher, "Only valid if we are publishing updates");
        if (pendingRemovalTimeMs > AZ::TimeMs{ 0 })
        {
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
        m_proxyRemovalEvent.RemoveFromQueue();
    }

    bool EntityReplicator::IsDeletionAcknowledged() const
    {
        bool ret(true);
        // we sent the delete message, make sure it gets there
        if (m_propertyPublisher)
        {
            ret = m_propertyPublisher->IsDeleted();
        }
        else
        {
            AZ_Assert(m_propertySubscriber, "Expected to have at least a subscriber when deleting");
            ret = m_propertySubscriber->IsDeleted();
        }
        return ret;
    }

    AZ::TimeMs EntityReplicator::GetResendTimeoutTimeMs() const
    {
        return m_replicationManager.GetResendTimeoutTimeMs();
    }

    NetworkEntityUpdateMessage EntityReplicator::GenerateUpdatePacket()
    {
        if (IsMarkedForRemoval() && OwnsReplicatorLifetime()) // TODO: clean this up
        {
            // If the remote replicator is not established, we need to take ownership of the entity
            AZLOG
            (
                NET_RepDeletes,
                "Sending delete replicator id %u migrated %d to remote manager id %d",
                aznumeric_cast<uint32_t>(GetEntityHandle().GetNetEntityId()),
                WasMigrated() ? 1 : 0,
                aznumeric_cast<int32_t>(m_replicationManager.GetRemoteHostId())
            );
            return NetworkEntityUpdateMessage(GetEntityHandle().GetNetEntityId(), WasMigrated(), m_propertyPublisher->IsRemoteReplicatorEstablished());
        }

        NetBindComponent* netBindComponent = GetNetBindComponent();
        const bool sendSliceName = !m_propertyPublisher->IsRemoteReplicatorEstablished();

        NetworkEntityUpdateMessage updateMessage(GetRemoteNetworkRole(), GetEntityHandle().GetNetEntityId());
        if (sendSliceName)
        {
            updateMessage.SetPrefabEntityId(netBindComponent->GetPrefabEntityId());
        }

        AzNetworking::NetworkInputSerializer inputSerializer(updateMessage.ModifyData().GetBuffer(), updateMessage.ModifyData().GetCapacity());
        m_propertyPublisher->UpdateSerialization(inputSerializer);
        updateMessage.ModifyData().Resize(inputSerializer.GetSize());

        return updateMessage;
    }

    void EntityReplicator::DeferRpcMessage(NetworkEntityRpcMessage& entityRpcMessage)
    {
        // Received rpc metrics, log rpc sent, number of bytes, and the componentId/rpcId for bandwidth metrics
        MultiplayerStats& stats = GetMultiplayer()->GetStats();
        stats.RecordRpcSent(entityRpcMessage.GetComponentId(), entityRpcMessage.GetRpcIndex(), entityRpcMessage.GetEstimatedSerializeSize());

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

        if (auto localEntity = m_entityHandle.GetEntity())
        {
            DeferRpcMessage(entityRpcMessage);
        }
    }

    void EntityReplicator::OnEntityDirtiedEvent()
    {
        AZ_Assert(m_propertyPublisher, "Expected to have a publisher, did we forget to disconnect?");
        m_propertyPublisher->GenerateRecord();
        m_replicationManager.AddReplicatorToPendingSend(*this);
    }

    void EntityReplicator::OnEntityRemovedEvent()
    {
        m_netBindComponent = nullptr;
        MarkForRemoval();
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
        {
            if (((GetBoundLocalNetworkRole() == NetEntityRole::Client) || (GetBoundLocalNetworkRole() == NetEntityRole::Autonomous))
                && (GetRemoteNetworkRole() == NetEntityRole::Authority))
            {
                // We are a local client, and we are connected to server, aka AuthorityToClient
                result = RpcValidationResult::HandleRpc;
            }
            if ((GetBoundLocalNetworkRole() == NetEntityRole::Server)
                && (GetRemoteNetworkRole() == NetEntityRole::Authority))
            {
                // We are on a server, and we received this message from another server, therefore we should forward this to any connected clients
                result = RpcValidationResult::ForwardToClient;
            }
        }
        break;
        case RpcDeliveryType::AuthorityToAutonomous:
        {
            if ((GetBoundLocalNetworkRole() == NetEntityRole::Autonomous)
                && (GetRemoteNetworkRole() == NetEntityRole::Authority))
            {
                // We are an autonomous client, and we are connected to server, aka AuthorityToAutonomous
                result = RpcValidationResult::HandleRpc;
            }
            if ((GetBoundLocalNetworkRole() == NetEntityRole::Authority)
                && (GetRemoteNetworkRole() == NetEntityRole::Server))
            {
                // We are on a server, and we received this message from another server, therefore we should forward this to our autonomous player
                // This can occur if we've recently migrated 
                result = RpcValidationResult::ForwardToAutonomous;
            }
        }
        break;
        case RpcDeliveryType::AutonomousToAuthority:
        {
            if ((GetBoundLocalNetworkRole() == NetEntityRole::Authority)
                && (GetRemoteNetworkRole() == NetEntityRole::Autonomous))
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
        }
        break;
        case RpcDeliveryType::ServerToAuthority:
        {
            if ((GetBoundLocalNetworkRole() == NetEntityRole::Authority)
                && (GetRemoteNetworkRole() == NetEntityRole::Server))
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
                    "Dropping RPC and Connection EntityId=%u LocalRole=%u RemoteRole=%u RpcDeliveryType=%u ComponentId=%u RpcType=%u IsReliable=%s IsMarkedForRemoval=%s",
                    aznumeric_cast<uint32_t>(m_entityHandle.GetNetEntityId()),
                    aznumeric_cast<uint32_t>(GetBoundLocalNetworkRole()),
                    aznumeric_cast<uint32_t>(GetRemoteNetworkRole()),
                    aznumeric_cast<uint32_t>(entityRpcMessage.GetRpcDeliveryType()),
                    aznumeric_cast<uint32_t>(entityRpcMessage.GetComponentId()),
                    aznumeric_cast<uint32_t>(entityRpcMessage.GetRpcIndex()),
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
                "Dropping RPC EntityId=%u LocalRole=%u RemoteRole=%u RpcDeliveryType=%u ComponentId=%u RpcType=%u IsReliable=%s IsMarkedForRemoval=%s",
                aznumeric_cast<uint32_t>(m_entityHandle.GetNetEntityId()),
                aznumeric_cast<uint32_t>(GetBoundLocalNetworkRole()),
                aznumeric_cast<uint32_t>(GetRemoteNetworkRole()),
                aznumeric_cast<uint32_t>(entityRpcMessage.GetRpcDeliveryType()),
                aznumeric_cast<uint32_t>(entityRpcMessage.GetComponentId()),
                aznumeric_cast<uint32_t>(entityRpcMessage.GetRpcIndex()),
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
        stats.RecordRpcReceived(entityRpcMessage.GetComponentId(), entityRpcMessage.GetRpcIndex(), entityRpcMessage.GetEstimatedSerializeSize());

        if (!m_netBindComponent)
        {
            AZLOG_WARN
            (
                "Dropping RPC since entity deleted EntityId=%u LocalRole=%u RemoteRole=%u RpcDeliveryType=%u ComponentId=%u RpcType=%u IsReliable=%s IsMarkedForRemoval=%s",
                aznumeric_cast<uint32_t>(m_entityHandle.GetNetEntityId()),
                aznumeric_cast<uint32_t>(GetBoundLocalNetworkRole()),
                aznumeric_cast<uint32_t>(GetRemoteNetworkRole()),
                aznumeric_cast<uint32_t>(entityRpcMessage.GetRpcDeliveryType()),
                aznumeric_cast<uint32_t>(entityRpcMessage.GetComponentId()),
                aznumeric_cast<uint32_t>(entityRpcMessage.GetRpcIndex()),
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
            return true;
        }
        case RpcValidationResult::ForwardToAutonomous:
        {
            ScopedForwardingMessage forwarding(*this);
            m_netBindComponent->GetSendAuthorityToAutonomousRpcEvent().Signal(entityRpcMessage);
            return true;
        }
        case RpcValidationResult::ForwardToAuthority:
        {
            ScopedForwardingMessage forwarding(*this);
            m_netBindComponent->GetSendServerToAuthorityRpcEvent().Signal(entityRpcMessage);
            return true;
        }
        default:
            break;
        }

        AZ_Assert(false, "Unhandled RpcValidationResult %d", result);
        return false;
    }
}
