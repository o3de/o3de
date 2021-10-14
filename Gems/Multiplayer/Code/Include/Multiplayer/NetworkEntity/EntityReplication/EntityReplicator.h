/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/Event.h>
#include <AzCore/EBus/ScheduledEvent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/ring_buffer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/NetworkEntity/NetworkEntityUpdateMessage.h>

namespace AzNetworking
{
    class IConnection;
}

namespace Multiplayer
{
    class EntityReplicationManager;
    class NetworkEntityRpcMessage;
    class NetBindComponent;

    class PropertyPublisher;
    class PropertySubscriber;

    class EntityReplicator final
        : public AZ::EntityBus::Handler
    {
    public:
        EntityReplicator(EntityReplicationManager& replicationManager, AzNetworking::IConnection* connection, NetEntityRole remoteNetworkRole, const ConstNetworkEntityHandle& entityHandle);
        ~EntityReplicator() override;

        NetEntityRole GetBoundLocalNetworkRole() const;
        NetEntityRole GetRemoteNetworkRole() const;
        ConstNetworkEntityHandle GetEntityHandle() const;
        NetBindComponent* GetNetBindComponent();

        void ActivateNetworkEntity();

        const PrefabEntityId& GetPrefabEntityId() const;
        bool IsPrefabEntityIdSet() const;

        bool OwnsReplicatorLifetime() const;
        bool RemoteManagerOwnsEntityLifetime() const;

        // Interface for ReplicationManager to modify state of replication
        void Initialize(const ConstNetworkEntityHandle& entityHandle);
        void Reset(NetEntityRole remoteNetworkRole);
        void MarkForRemoval();
        bool IsMarkedForRemoval() const;
        void SetPendingRemoval(AZ::TimeMs pendingRemovalTimeMs);
        bool IsPendingRemoval() const;
        void ClearPendingRemoval();
        bool IsDeletionAcknowledged() const;
        bool WasMigrated() const;
        void SetWasMigrated(bool wasMigrated);
        // If an entity is part of a network hierarchy then it is only ready to activate when its direct parent entity is active.
        bool IsReadyToActivate() const;

        NetworkEntityUpdateMessage GenerateUpdatePacket();
        void FinalizeSerialization(AzNetworking::PacketId sentId);

        AZ::TimeMs GetResendTimeoutTimeMs() const;

        PropertyPublisher* GetPropertyPublisher();
        const PropertyPublisher* GetPropertyPublisher() const;
        PropertySubscriber* GetPropertySubscriber();

        // Handlers for Rpc messages
        bool HandleRpcMessage(AzNetworking::IConnection* invokingConnection, NetworkEntityRpcMessage& entityRpcMessage);

        //! AZ::EntityBus overrides
        //! @{
        void OnEntityActivated(const AZ::EntityId&) override;
        void OnEntityDestroyed(const AZ::EntityId&) override;
        //! @}

    private:
        enum class RpcValidationResult
        {
            HandleRpc,              // Handle Rpc message
            DropRpc,                // Do not handle Rpc
            DropRpcAndDisconnect,   // Do not handle the Rpc, it is disallowed from this endpoint we should disconnect the connection
            ForwardToClient,        // Forward this message to the Client
            ForwardToAutonomous,    // Forward this message to the Autonomous
            ForwardToAuthority,     // Forward this message to the Authority
        };
        RpcValidationResult ValidateRpcMessage(const NetworkEntityRpcMessage& entityRpcMessage) const;

        // Internal state tracking
        bool CanSendUpdates();

        void SetPrefabEntityId(const PrefabEntityId& prefabEntityId); // cache assetId so authority doesn't need to keep sending it

        // Event processing
        void OnSendRpcEvent(NetworkEntityRpcMessage& message);
        void OnForwardRpcEvent(NetworkEntityRpcMessage& message);
        void OnEntityDirtiedEvent();
        void OnEntityRemovedEvent();
        void OnProxyRemovalTimedEvent();

        void ActivateNetworkEntityInternal();

        void AttachRPCHandlers();

        void DeferRpcMessage(NetworkEntityRpcMessage& message);

        AZ_DISABLE_COPY_MOVE(EntityReplicator);

        // Events
        RpcSendEvent::Handler m_onSendRpcHandler;
        RpcSendEvent::Handler m_onForwardRpcHandler;
        RpcSendEvent::Handler m_onSendAutonomousRpcHandler;
        RpcSendEvent::Handler m_onForwardAutonomousRpcHandler;
        EntityDirtiedEvent::Handler m_onEntityDirtiedHandler;
        EntityStopEvent::Handler m_onEntityStopHandler;
        AZ::ScheduledEvent m_proxyRemovalEvent;

        ConstNetworkEntityHandle m_entityHandle;
        PrefabEntityId m_prefabEntityId;

        AZStd::unique_ptr<PropertyPublisher> m_propertyPublisher;
        AZStd::unique_ptr<PropertySubscriber> m_propertySubscriber;

        NetBindComponent* m_netBindComponent = nullptr;
        EntityReplicationManager& m_replicationManager;
        AzNetworking::IConnection* m_connection;
        NetEntityRole m_boundLocalNetworkRole;
        NetEntityRole m_remoteNetworkRole;

        bool m_wasMigrated = false;
        bool m_isForwardingRpc = false;
        bool m_prefabEntityIdSet = false;
    };
}

#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.inl>
