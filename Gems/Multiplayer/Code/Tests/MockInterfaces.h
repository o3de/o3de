/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Time/ITime.h>
#include <AzNetworking/ConnectionLayer/IConnectionListener.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/NetworkEntity/NetworkEntityRpcMessage.h>

namespace UnitTest
{
    class MockMultiplayer : public Multiplayer::IMultiplayer
    {
    public:
        MOCK_CONST_METHOD0(GetAgentType, Multiplayer::MultiplayerAgentType());
        MOCK_METHOD1(InitializeMultiplayer, void(Multiplayer::MultiplayerAgentType));
        MOCK_METHOD2(StartHosting, bool(uint16_t, bool));
        MOCK_METHOD2(Connect, bool(const AZStd::string&, uint16_t));
        MOCK_METHOD1(Terminate, void(AzNetworking::DisconnectReason));
        MOCK_METHOD1(AddClientMigrationStartEventHandler, void(Multiplayer::ClientMigrationStartEvent::Handler&));
        MOCK_METHOD1(AddClientMigrationEndEventHandler, void(Multiplayer::ClientMigrationEndEvent::Handler&));
        MOCK_METHOD1(AddClientDisconnectedHandler, void(AZ::Event<>::Handler&));
        MOCK_METHOD1(AddNotifyClientMigrationHandler, void(Multiplayer::NotifyClientMigrationEvent::Handler&));
        MOCK_METHOD1(AddNotifyEntityMigrationEventHandler, void(Multiplayer::NotifyEntityMigrationEvent::Handler&));
        MOCK_METHOD1(AddConnectionAcquiredHandler, void(Multiplayer::ConnectionAcquiredEvent::Handler&));
        MOCK_METHOD1(AddServerAcceptanceReceivedHandler, void(Multiplayer::ServerAcceptanceReceivedEvent::Handler&));
        MOCK_METHOD1(AddSessionInitHandler, void(Multiplayer::SessionInitEvent::Handler&));
        MOCK_METHOD1(AddSessionShutdownHandler, void(Multiplayer::SessionShutdownEvent::Handler&));
        MOCK_METHOD5(SendNotifyClientMigrationEvent, void(AzNetworking::ConnectionId, const Multiplayer::HostId&, uint64_t, Multiplayer::ClientInputId, Multiplayer::NetEntityId));
        MOCK_METHOD2(SendNotifyEntityMigrationEvent, void(const Multiplayer::ConstNetworkEntityHandle&, const Multiplayer::HostId&));
        MOCK_METHOD1(SendReadyForEntityUpdates, void(bool));
        MOCK_CONST_METHOD0(GetCurrentHostTimeMs, AZ::TimeMs());
        MOCK_CONST_METHOD0(GetCurrentBlendFactor, float());
        MOCK_METHOD0(GetNetworkTime, Multiplayer::INetworkTime* ());
        MOCK_METHOD0(GetNetworkEntityManager, Multiplayer::INetworkEntityManager* ());
        MOCK_METHOD1(SetFilterEntityManager, void(Multiplayer::IFilterEntityManager*));
        MOCK_METHOD0(GetFilterEntityManager, Multiplayer::IFilterEntityManager* ());
        MOCK_METHOD2(RegisterPlayerIdentifierForRejoin, void(uint64_t, Multiplayer::NetEntityId));
        MOCK_METHOD4(CompleteClientMigration, void(uint64_t, AzNetworking::ConnectionId, const Multiplayer::HostId&, Multiplayer::ClientInputId));
        MOCK_METHOD1(SetShouldSpawnNetworkEntities, void(bool));
        MOCK_CONST_METHOD0(GetShouldSpawnNetworkEntities, bool());
    };

    class MockNetworkEntityManager : public Multiplayer::INetworkEntityManager
    {
    public:
        MOCK_METHOD2(Initialize, void(const Multiplayer::HostId&, AZStd::unique_ptr<Multiplayer::IEntityDomain>));
        MOCK_CONST_METHOD0(IsInitialized, bool());
        MOCK_CONST_METHOD0(GetEntityDomain, Multiplayer::IEntityDomain*());
        MOCK_METHOD0(GetNetworkEntityTracker, Multiplayer::NetworkEntityTracker* ());
        MOCK_METHOD0(GetNetworkEntityAuthorityTracker, Multiplayer::NetworkEntityAuthorityTracker* ());
        MOCK_METHOD0(GetMultiplayerComponentRegistry, Multiplayer::MultiplayerComponentRegistry* ());
        MOCK_CONST_METHOD0(GetHostId, const Multiplayer::HostId&());
        MOCK_CONST_METHOD1(GetEntity, Multiplayer::ConstNetworkEntityHandle(Multiplayer::NetEntityId));
        MOCK_CONST_METHOD1(GetNetEntityIdById, Multiplayer::NetEntityId(const AZ::EntityId&));
        MOCK_METHOD3(CreateEntitiesImmediate, EntityList(const Multiplayer::PrefabEntityId&, Multiplayer::NetEntityRole, const AZ::
            Transform&));
        MOCK_METHOD4(
            CreateEntitiesImmediate,
            EntityList(const Multiplayer::PrefabEntityId&, Multiplayer::NetEntityRole, const AZ::Transform&, Multiplayer::AutoActivate));
        MOCK_METHOD5(CreateEntitiesImmediate, EntityList(const Multiplayer::PrefabEntityId&, Multiplayer::NetEntityId, Multiplayer::
            NetEntityRole, Multiplayer::AutoActivate, const AZ::Transform&));
        MOCK_METHOD2(RequestNetSpawnableInstantiation, AZStd::unique_ptr<AzFramework::EntitySpawnTicket>(const AZ::Data::Asset<AzFramework::Spawnable>&, const AZ::Transform&));
        MOCK_METHOD3(SetupNetEntity, void(AZ::Entity*, Multiplayer::PrefabEntityId, Multiplayer::NetEntityRole));
        MOCK_CONST_METHOD0(GetEntityCount, uint32_t());
        MOCK_METHOD2(AddEntityToEntityMap, Multiplayer::NetworkEntityHandle(Multiplayer::NetEntityId, AZ::Entity*));
        MOCK_METHOD1(MarkForRemoval, void(const Multiplayer::ConstNetworkEntityHandle&));
        MOCK_CONST_METHOD1(IsMarkedForRemoval, bool(const Multiplayer::ConstNetworkEntityHandle&));
        MOCK_METHOD1(ClearEntityFromRemovalList, void(const Multiplayer::ConstNetworkEntityHandle&));
        MOCK_METHOD0(ClearAllEntities, void());
        MOCK_METHOD1(AddEntityMarkedDirtyHandler, void(AZ::Event<>::Handler&));
        MOCK_METHOD1(AddEntityNotifyChangesHandler, void(AZ::Event<>::Handler&));
        MOCK_METHOD1(AddEntityExitDomainHandler, void(AZ::Event<const Multiplayer::ConstNetworkEntityHandle&>::Handler&));
        MOCK_METHOD1(AddControllersActivatedHandler, void(AZ::Event<const Multiplayer::ConstNetworkEntityHandle&, Multiplayer::
            EntityIsMigrating>::Handler&));
        MOCK_METHOD1(AddControllersDeactivatedHandler, void(AZ::Event<const Multiplayer::ConstNetworkEntityHandle&, Multiplayer::
            EntityIsMigrating>::Handler&));
        MOCK_METHOD0(NotifyEntitiesDirtied, void());
        MOCK_METHOD0(NotifyEntitiesChanged, void());
        MOCK_METHOD2(NotifyControllersActivated, void(const Multiplayer::ConstNetworkEntityHandle&, Multiplayer::EntityIsMigrating));
        MOCK_METHOD2(NotifyControllersDeactivated, void(const Multiplayer::ConstNetworkEntityHandle&, Multiplayer::EntityIsMigrating));
        MOCK_METHOD1(HandleLocalRpcMessage, void(Multiplayer::NetworkEntityRpcMessage&));
        MOCK_CONST_METHOD0(DebugDraw, void());
    };

    class MockConnectionListener : public AzNetworking::IConnectionListener
    {
    public:
        MOCK_METHOD3(ValidateConnect, ConnectResult(const IpAddress&, const IPacketHeader&, ISerializer&));
        MOCK_METHOD1(OnConnect, void(IConnection*));
        MOCK_METHOD3(OnPacketReceived, PacketDispatchResult (IConnection*, const IPacketHeader&, ISerializer&));
        MOCK_METHOD2(OnPacketLost, void(IConnection*, PacketId));
        MOCK_METHOD3(OnDisconnect, void(IConnection*, DisconnectReason, TerminationEndpoint));
    };

    class MockTime : public AZ::ITime
    {
    public:
        MOCK_CONST_METHOD0(GetElapsedTimeUs, AZ::TimeUs());
        MOCK_CONST_METHOD0(GetElapsedTimeMs, AZ::TimeMs());
    };

    class MockNetworkTime : public Multiplayer::INetworkTime
    {
    public:
        MOCK_METHOD2(ForceSetTime, void (Multiplayer::HostFrameId, AZ::TimeMs));
        MOCK_CONST_METHOD0(GetHostBlendFactor, float ());
        MOCK_METHOD1(AlterBlendFactor, void (float));
        MOCK_CONST_METHOD0(IsTimeRewound, bool());
        MOCK_CONST_METHOD0(GetHostFrameId, Multiplayer::HostFrameId());
        MOCK_CONST_METHOD0(GetUnalteredHostFrameId, Multiplayer::HostFrameId());
        MOCK_METHOD0(IncrementHostFrameId, void());
        MOCK_CONST_METHOD0(GetHostTimeMs, AZ::TimeMs());
        MOCK_CONST_METHOD0(GetRewindingConnectionId, AzNetworking::ConnectionId());
        MOCK_CONST_METHOD1(GetHostFrameIdForRewindingConnection, Multiplayer::HostFrameId(AzNetworking::ConnectionId));
        MOCK_METHOD4(AlterTime, void (Multiplayer::HostFrameId, AZ::TimeMs, float, AzNetworking::ConnectionId));
        MOCK_METHOD1(SyncEntitiesToRewindState, void(const AZ::Aabb&));
        MOCK_METHOD0(ClearRewoundEntities, void());
    };

    class MockComponentApplicationRequests : public AZ::ComponentApplicationRequests
    {
    public:
        MOCK_METHOD1(RegisterComponentDescriptor, void(const AZ::ComponentDescriptor*));
        MOCK_METHOD1(UnregisterComponentDescriptor, void(const AZ::ComponentDescriptor*));
        MOCK_METHOD0(GetApplication, AZ::ComponentApplication* ());
        MOCK_METHOD1(RegisterEntityAddedEventHandler, void(AZ::Event<AZ::Entity*>::Handler&));
        MOCK_METHOD1(RegisterEntityRemovedEventHandler, void(AZ::Event<AZ::Entity*>::Handler&));
        MOCK_METHOD1(RegisterEntityActivatedEventHandler, void(AZ::Event<AZ::Entity*>::Handler&));
        MOCK_METHOD1(RegisterEntityDeactivatedEventHandler, void(AZ::Event<AZ::Entity*>::Handler&));
        MOCK_METHOD1(SignalEntityActivated, void(AZ::Entity*));
        MOCK_METHOD1(SignalEntityDeactivated, void(AZ::Entity*));
        MOCK_METHOD1(AddEntity, bool(AZ::Entity*));
        MOCK_METHOD1(RemoveEntity, bool(AZ::Entity*));
        MOCK_METHOD1(DeleteEntity, bool(const AZ::EntityId&));
        MOCK_METHOD1(FindEntity, AZ::Entity* (const AZ::EntityId&));
        MOCK_METHOD1(GetEntityName, AZStd::string(const AZ::EntityId&));
        MOCK_METHOD1(EnumerateEntities, void(const EntityCallback&));
        MOCK_METHOD0(GetSerializeContext, AZ::SerializeContext* ());
        MOCK_METHOD0(GetBehaviorContext, AZ::BehaviorContext* ());
        MOCK_METHOD0(GetJsonRegistrationContext, AZ::JsonRegistrationContext* ());
        MOCK_CONST_METHOD0(GetAppRoot, const char* ());
        MOCK_CONST_METHOD0(GetEngineRoot, const char* ());
        MOCK_CONST_METHOD0(GetExecutableFolder, const char* ());
        MOCK_METHOD0(GetDrillerManager, AZ::Debug::DrillerManager* ());
        MOCK_METHOD1(ResolveModulePath, void(AZ::OSString&));
        MOCK_METHOD0(GetAzCommandLine, AZ::CommandLine* ());
        MOCK_CONST_METHOD1(QueryApplicationType, void(AZ::ApplicationTypeQuery&));
    };

    class MockSerializer : public ISerializer
    {
    public:
        MOCK_CONST_METHOD0(IsValid, bool ());
        MOCK_CONST_METHOD0(GetSerializerMode, SerializerMode ());
        MOCK_METHOD2(Serialize, bool (bool&, const char*));
        MOCK_METHOD4(Serialize, bool (char&, const char*, char, char));
        MOCK_METHOD4(Serialize, bool (int8_t&, const char*, int8_t, int8_t));
        MOCK_METHOD4(Serialize, bool (int16_t&, const char*, int16_t, int16_t));
        MOCK_METHOD4(Serialize, bool (int32_t&, const char*, int32_t, int32_t));
        MOCK_METHOD4(Serialize, bool (int64_t&, const char*, int64_t, int64_t));
        MOCK_METHOD4(Serialize, bool (uint8_t&, const char*, uint8_t, uint8_t));
        MOCK_METHOD4(Serialize, bool (uint16_t&, const char*, uint16_t, uint16_t));
        MOCK_METHOD4(Serialize, bool (uint32_t&, const char*, uint32_t, uint32_t));
        MOCK_METHOD4(Serialize, bool (uint64_t&, const char*, uint64_t, uint64_t));
        MOCK_METHOD4(Serialize, bool (float&, const char*, float, float));
        MOCK_METHOD4(Serialize, bool (double&, const char*, double, double));
        MOCK_METHOD5(SerializeBytes, bool (uint8_t*, uint32_t, bool, uint32_t&, const char*));
        MOCK_METHOD2(BeginObject, bool (const char*, const char*));
        MOCK_METHOD2(EndObject, bool (const char*, const char*));
        MOCK_CONST_METHOD0(GetBuffer, const uint8_t* ());
        MOCK_CONST_METHOD0(GetCapacity, uint32_t ());
        MOCK_CONST_METHOD0(GetSize, uint32_t ());
        MOCK_METHOD0(ClearTrackedChangesFlag, void ());
        MOCK_CONST_METHOD0(GetTrackedChangesFlag, bool ());
    };
}

