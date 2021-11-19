/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzNetworking/Serialization/ISerializer.h>
#include <AzNetworking/ConnectionLayer/IConnection.h>
#include <Multiplayer/NetworkEntity/EntityReplication/ReplicationRecord.h>
#include <Multiplayer/NetworkEntity/NetworkEntityHandle.h>
#include <Multiplayer/NetworkInput/IMultiplayerComponentInput.h>
#include <Multiplayer/NetworkTime/INetworkTime.h>
#include <Multiplayer/MultiplayerTypes.h>
#include <AzCore/EBus/Event.h>

namespace Multiplayer
{
    class NetworkInput;
    class ReplicationRecord;
    class MultiplayerComponent;

    using EntityStopEvent = AZ::Event<const ConstNetworkEntityHandle&>;
    using EntityDirtiedEvent = AZ::Event<>;
    using EntitySyncRewindEvent = AZ::Event<>;
    using EntityServerMigrationEvent = AZ::Event<const ConstNetworkEntityHandle&, const HostId&>;
    using EntityPreRenderEvent = AZ::Event<float>;
    using EntityCorrectionEvent = AZ::Event<>;

    //! @class NetBindComponent
    //! @brief Component that provides net-binding to a networked entity.
    class NetBindComponent final
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(NetBindComponent, "{DAA076B3-1A1C-4FEF-8583-1DF696971604}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        NetBindComponent();
        ~NetBindComponent() override = default;

        //! AZ::Component overrides.
        //! @{
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //! @}

        NetEntityRole GetNetEntityRole() const;
        
        //! IsNetEntityRoleAuthority
        //! @return true if this network entity is an authoritative proxy on a server (full authority); otherwise false.
        bool IsNetEntityRoleAuthority() const;
        
        //! IsNetEntityRoleAutonomous
        //! @return true if this network entity is an autonomous proxy on a client (can execute local prediction) or if this network entity is an authoritative proxy on a server but has autonomous privileges (ie: a host who is also a player); otherwise false.   
        bool IsNetEntityRoleAutonomous() const;

        //! IsNetEntityRoleServer
        //! @return true if this network entity is a simulated proxy on a server (ie: a different server may have authority for this entity, but the entity has been replicated on this server; otherwise false.
        bool IsNetEntityRoleServer() const;

        //! IsNetEntityRoleClient
        //! @return true if this network entity is a simulated proxy on a client; otherwise false.
        bool IsNetEntityRoleClient() const;
        
        bool HasController() const;
        NetEntityId GetNetEntityId() const;
        const PrefabEntityId& GetPrefabEntityId() const;
        ConstNetworkEntityHandle GetEntityHandle() const;
        NetworkEntityHandle GetEntityHandle();

        void SetOwningConnectionId(AzNetworking::ConnectionId connectionId);
        AzNetworking::ConnectionId GetOwningConnectionId() const;
        void SetAllowAutonomy(bool value);
        MultiplayerComponentInputVector AllocateComponentInputs();

        //! Return true if we're currently processing inputs.
        //! @return true if we're within ProcessInput scope and writing to predictive state
        bool IsProcessingInput() const;

        //! Return true if we're currently replaying inputs after a correction.
        //! If this value returns true, effects, audio, and other cosmetic triggers should be suppressed
        //! @return true if we're within correction scope and replaying inputs
        bool IsReprocessingInput() const;

        void CreateInput(NetworkInput& networkInput, float deltaTime);
        void ProcessInput(NetworkInput& networkInput, float deltaTime);
        void ReprocessInput(NetworkInput& networkInput, float deltaTime);

        bool HandleRpcMessage(AzNetworking::IConnection* invokingConnection, NetEntityRole remoteRole, NetworkEntityRpcMessage& message);
        bool HandlePropertyChangeMessage(AzNetworking::ISerializer& serializer, bool notifyChanges = true);

        RpcSendEvent& GetSendAuthorityToClientRpcEvent();
        RpcSendEvent& GetSendAuthorityToAutonomousRpcEvent();
        RpcSendEvent& GetSendServerToAuthorityRpcEvent();
        RpcSendEvent& GetSendAutonomousToAuthorityRpcEvent();

        const ReplicationRecord& GetPredictableRecord() const;

        void MarkDirty();
        void NotifyLocalChanges();
        void NotifySyncRewindState();
        void NotifyServerMigration(const HostId& remoteHostId);
        void NotifyPreRender(float deltaTime);
        void NotifyCorrection();

        void AddEntityStopEventHandler(EntityStopEvent::Handler& eventHandler);
        void AddEntityDirtiedEventHandler(EntityDirtiedEvent::Handler& eventHandler);
        void AddEntitySyncRewindEventHandler(EntitySyncRewindEvent::Handler& eventHandler);
        void AddEntityServerMigrationEventHandler(EntityServerMigrationEvent::Handler& eventHandler);
        void AddEntityPreRenderEventHandler(EntityPreRenderEvent::Handler& eventHandler);
        void AddEntityCorrectionEventHandler(EntityCorrectionEvent::Handler& handler);

        bool SerializeEntityCorrection(AzNetworking::ISerializer& serializer);

        bool SerializeStateDeltaMessage(ReplicationRecord& replicationRecord, AzNetworking::ISerializer& serializer);
        void NotifyStateDeltaChanges(ReplicationRecord& replicationRecord);

        void FillReplicationRecord(ReplicationRecord& replicationRecord) const;
        void FillTotalReplicationRecord(ReplicationRecord& replicationRecord) const;

    private:
        void PreInit(AZ::Entity* entity, const PrefabEntityId& prefabEntityId, NetEntityId netEntityId, NetEntityRole netEntityRole);

        void ConstructControllers();
        void DestructControllers();
        void ActivateControllers(EntityIsMigrating entityIsMigrating);
        void DeactivateControllers(EntityIsMigrating entityIsMigrating);

        void OnEntityStateEvent(AZ::Entity::State oldState, AZ::Entity::State newState);

        void NetworkAttach();

        void HandleMarkedDirty();
        void HandleLocalServerRpcMessage(NetworkEntityRpcMessage& message);

        void DetermineInputOrdering();

        void StopEntity();

        ReplicationRecord m_currentRecord = NetEntityRole::InvalidRole;
        ReplicationRecord m_totalRecord = NetEntityRole::InvalidRole;
        ReplicationRecord m_predictableRecord = NetEntityRole::Autonomous;
        ReplicationRecord m_localNotificationRecord = NetEntityRole::InvalidRole;
        PrefabEntityId    m_prefabEntityId;
        AZStd::unordered_map<NetComponentId, MultiplayerComponent*> m_multiplayerComponentMap;
        AZStd::vector<MultiplayerComponent*> m_multiplayerSerializationComponentVector;
        AZStd::vector<MultiplayerComponent*> m_multiplayerInputComponentVector;

        RpcSendEvent m_sendAuthorityToClientRpcEvent;
        RpcSendEvent m_sendAuthorityToAutonomousRpcEvent;
        RpcSendEvent m_sendServertoAuthorityRpcEvent;
        RpcSendEvent m_sendAutonomousToAuthorityRpcEvent;

        EntityStopEvent       m_entityStopEvent;
        EntityDirtiedEvent    m_dirtiedEvent;
        EntitySyncRewindEvent m_syncRewindEvent;
        EntityServerMigrationEvent m_entityServerMigrationEvent;
        EntityPreRenderEvent  m_entityPreRenderEvent;
        EntityCorrectionEvent m_entityCorrectionEvent;
        AZ::Event<>           m_onRemove;
        RpcSendEvent::Handler m_handleLocalServerRpcMessageEventHandle;
        AZ::Event<>::Handler  m_handleMarkedDirty;
        AZ::Event<>::Handler  m_handleNotifyChanges;
        AZ::Entity::EntityStateEvent::Handler m_handleEntityStateEvent;

        NetworkEntityHandle   m_netEntityHandle;
        NetEntityRole         m_netEntityRole   = NetEntityRole::InvalidRole;
        NetEntityId           m_netEntityId     = InvalidNetEntityId;

        AzNetworking::ConnectionId m_owningConnectionId = AzNetworking::InvalidConnectionId;

        bool m_isProcessingInput    = false; // Set to true when we are processing input
        bool m_isReprocessingInput  = false; // Set to true when we are reprocessing input (during a correction)
        bool m_isMigrationDataValid = false;
        bool m_needsToBeStopped     = false;
        bool m_allowAutonomy        = false; // Set to true for the hosts controlled entity

        friend class NetworkEntityManager;
        friend class EntityReplicationManager;

        friend class HierarchyTests;
        friend class HierarchyBenchmarkBase;
    };

    bool NetworkRoleHasController(NetEntityRole networkRole);
}
