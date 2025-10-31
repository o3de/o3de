/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
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
        ~NetBindComponent() override;

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

        //! Sets whether or not a netbound entity is allowed to migrate between hosts.
        //! Use this feature carefully, as replication is spatially based. If migration is disabled chances are you want to mark the entity as always persistent as well.
        //! See INetworkEntityManager::MarkAlwaysRelevantToClients and INetworkEntityManager::MarkAlwaysRelevantToServers
        //! @param value whether to enable or disable host migrations for this entity
        void SetAllowEntityMigration(EntityMigration value);

        //! Retrieves whether or not the netbound entity is allowed to migrate between hosts.
        //! @return EntityMigration::Enabled if the entity is allowed to migrate, EntityMigration::Disabled otherwise
        EntityMigration GetAllowEntityMigration() const;

        //! This is a helper that validates the owning entity is in the correct role to read from a network property that matches the relicateFrom and replicateTo parameters.
        //! @param propertyName  the name of the property, for logging and debugging purposes
        //! @param replicateFrom the network entity role that the property replicates from
        //! @param replicateTo   the network entity role that the property replicates to
        //! @return boolean true if the read is valid, false if nothing is replicated to the target and invalid data will be read
        bool ValidatePropertyRead(const char* propertyName, NetEntityRole replicateFrom, NetEntityRole replicateTo) const;

        //! This is a helper that validates the owning entity is in the correct role to write to a network property that matches the relicateFrom, replicateTo parameters, and isPredictable parameters.
        //! @param propertyName  the name of the property, for logging and debugging purposes
        //! @param replicateFrom the network entity role that the property replicates from
        //! @param replicateTo   the network entity role that the property replicates to
        //! @param isPredictable true if the property is predictable, false otherwise
        //! @return boolean true if the write is valid, false if the write will desync the network property
        bool ValidatePropertyWrite(const char* propertyName, NetEntityRole replicateFrom, NetEntityRole replicateTo, bool isPredictable) const;

        //! Returns whether or not a controller exists for the bound network entity.
        //! Warning, this function is dangerous to use in game code as it makes it easy to write logic that will function incorrectly within multihost environments. Use carefully.
        //! The recommended solution for communicating from proxy level to a controller is to use a Server to Authority RPC, as the network layer can route the RPC appropriately.
        //! @return boolean true if a controller exists, false otherwise
        bool HasController() const;

        //! Returns the bound NetworkEntityId that represents this entity.
        //! @return the bound NetworkEntityId that represents this entity
        NetEntityId GetNetEntityId() const;

        //! Returns the PrefabEntityId that this entity was loaded from.
        //! @return the PrefabEntityId that this entity was loaded from
        const PrefabEntityId& GetPrefabEntityId() const;

        //! Sets the PrefabEntityId that this entity was loaded from.
        //! @param prefabEntityId the PrefabEntityId to mark as bound to this entity
        void SetPrefabEntityId(const PrefabEntityId& prefabEntityId);

        //! Returns the PrefabAssetId of the prefab this entity was loaded from.
        //! @return the PrefabAssetId of the prefab this entity was loaded from
        const AZ::Data::AssetId& GetPrefabAssetId() const;

        //! Sets the PrefabAssetId of the prefab that this entity was loaded from.
        //! @param val the PrefabAssetId to mark as bound to this entity
        void SetPrefabAssetId(const AZ::Data::AssetId& val);

        //! Returns a const network entity handle to this entity.
        //! @return a const network entity handle to this entity
        ConstNetworkEntityHandle GetEntityHandle() const;

        //! Returns a non-const network entity handle for this entity, this allows controller access so use it with great caution.
        //! Warning, this function is dangerous to use in game code as it makes it easy to write logic that will function incorrectly within multihost environments. Use carefully.
        //! @return a non-const network entity handle to this entity
        NetworkEntityHandle GetEntityHandle();

        //! Sets the AzNetworking::ConnectionId that 'owns' this entity from a local prediction standpoint.
        //! This is important for correct rewind operation during backward reconciliation, as we shouldn't rewind anything owned by the autonomous entity itself.
        //! @param connectionId the AzNetworking::ConnectionId to mark as the owner of this entity
        void SetOwningConnectionId(AzNetworking::ConnectionId connectionId);

        //! Returns the AzNetworking::ConnectionId of the connection that owns this entity form a local prediction standpoint.
        //! @return the AzNetworking::ConnectionId of the connection that owns this entity form a local prediction standpoint
        AzNetworking::ConnectionId GetOwningConnectionId() const;

        //! Allows a player host to autonomously control their player entity, even though the entity is in an authority role.
        //! Note: If this entity is already activated this will reactivate all of the multiplayer component controllers in order for them to reactivate under autonomous control.
        void EnablePlayerHostAutonomy(bool enabled);

        //! This allocates and returns an appropriate MultiplayerComponentInputVector for the components bound to this entity.
        //! @return a MultiplayerComponentInputVector suitable for this entity
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
        void NetworkActivated();

        void AddEntityStopEventHandler(EntityStopEvent::Handler& eventHandler);
        void AddEntityDirtiedEventHandler(EntityDirtiedEvent::Handler& eventHandler);
        void AddEntitySyncRewindEventHandler(EntitySyncRewindEvent::Handler& eventHandler);
        void AddEntityServerMigrationEventHandler(EntityServerMigrationEvent::Handler& eventHandler);
        void AddEntityPreRenderEventHandler(EntityPreRenderEvent::Handler& eventHandler);
        void AddEntityCorrectionEventHandler(EntityCorrectionEvent::Handler& handler);
        void AddNetworkActivatedEventHandler(AZ::Event<>::Handler& handler);

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
        void HandleLocalAutonomousToAuthorityRpcMessage(NetworkEntityRpcMessage& message);
        void HandleLocalAuthorityToAutonomousRpcMessage(NetworkEntityRpcMessage& message);
        void HandleLocalAuthorityToClientRpcMessage(NetworkEntityRpcMessage& message);

        void DetermineInputOrdering();

        void StopEntity();

        void Register(AZ::Entity* entity);
        void Unregister();

        ReplicationRecord m_currentRecord = NetEntityRole::InvalidRole;
        ReplicationRecord m_totalRecord = NetEntityRole::InvalidRole;
        ReplicationRecord m_predictableRecord = NetEntityRole::Autonomous;
        ReplicationRecord m_localNotificationRecord = NetEntityRole::InvalidRole;
        PrefabEntityId    m_prefabEntityId;
        AZ::Data::AssetId m_prefabAssetId;
        // It is important that this component map be ordered, as we walk it to generate serialization ordering
        AZStd::map<NetComponentId, MultiplayerComponent*> m_multiplayerComponentMap;
        AZStd::vector<MultiplayerComponent*> m_multiplayerSerializationComponentVector;
        AZStd::vector<MultiplayerComponent*> m_multiplayerInputComponentVector;

        RpcSendEvent m_sendAuthorityToClientRpcEvent;
        RpcSendEvent m_sendAuthorityToAutonomousRpcEvent;
        RpcSendEvent m_sendServerToAuthorityRpcEvent;
        RpcSendEvent m_sendAutonomousToAuthorityRpcEvent;

        EntityStopEvent       m_entityStopEvent;
        EntityDirtiedEvent    m_dirtiedEvent;
        EntitySyncRewindEvent m_syncRewindEvent;
        EntityServerMigrationEvent m_entityServerMigrationEvent;
        EntityPreRenderEvent  m_entityPreRenderEvent;
        EntityCorrectionEvent m_entityCorrectionEvent;
        AZ::Event<>           m_onRemove;
        AZ::Event<>           m_onNetworkActivated;
        RpcSendEvent::Handler m_handleLocalServerRpcMessageEventHandle;
        RpcSendEvent::Handler m_handleLocalAutonomousToAuthorityRpcMessageEventHandle;
        RpcSendEvent::Handler m_handleLocalAuthorityToAutonomousRpcMessageEventHandle;
        RpcSendEvent::Handler m_handleLocalAuthorityToClientRpcMessageEventHandle;
        AZ::Event<>::Handler  m_handleMarkedDirty;
        AZ::Event<>::Handler  m_handleNotifyChanges;
        AZ::Entity::EntityStateEvent::Handler m_handleEntityStateEvent;

        NetworkEntityHandle   m_netEntityHandle;
        NetEntityRole         m_netEntityRole   = NetEntityRole::InvalidRole;
        NetEntityId           m_netEntityId     = InvalidNetEntityId;
        EntityMigration       m_netEntityMigration = EntityMigration::Enabled;

        AzNetworking::ConnectionId m_owningConnectionId = AzNetworking::InvalidConnectionId;

        bool m_isProcessingInput    = false; // Set to true when we are processing input
        bool m_isReprocessingInput  = false; // Set to true when we are reprocessing input (during a correction)
        bool m_isMigrationDataValid = false;
        bool m_needsToBeStopped     = false;
        bool m_playerHostAutonomyEnabled = false; // Set to true for the host's controlled entity
        bool m_isRegistered = false;

        friend class NetworkEntityManager;
        friend class EntityReplicationManager;

        friend class HierarchyTests;
        friend class HierarchyBenchmarkBase;
        friend class MultiplayerSystemTests;
        friend class NetworkEntityTests;
        friend class LocalPredictionPlayerInputTests;
    };

    bool NetworkRoleHasController(NetEntityRole networkRole);
}
