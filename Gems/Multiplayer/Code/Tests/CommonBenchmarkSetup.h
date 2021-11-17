/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#ifdef HAVE_BENCHMARK
#include <CommonHierarchySetup.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/Mocks/MockITime.h>
#include <AzFramework/Components/TransformComponent.h>
#include <benchmark/benchmark.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>

namespace Multiplayer
{
    class BenchmarkComponentApplicationRequests : public AZ::ComponentApplicationRequests
    {
    public:
        void RegisterComponentDescriptor([[maybe_unused]] const AZ::ComponentDescriptor* descriptor) override {}
        void UnregisterComponentDescriptor([[maybe_unused]] const AZ::ComponentDescriptor* descriptor) override {}
        AZ::ComponentApplication* GetApplication() override { return {}; }
        void RegisterEntityAddedEventHandler([[maybe_unused]] AZ::Event<AZ::Entity*>::Handler& handler) override {}
        void RegisterEntityRemovedEventHandler([[maybe_unused]] AZ::Event<AZ::Entity*>::Handler& handler) override {}
        void RegisterEntityActivatedEventHandler([[maybe_unused]] AZ::Event<AZ::Entity*>::Handler& handler) override {}
        void RegisterEntityDeactivatedEventHandler([[maybe_unused]] AZ::Event<AZ::Entity*>::Handler& handler) override {}
        void SignalEntityActivated([[maybe_unused]] AZ::Entity* entity) override {}
        void SignalEntityDeactivated([[maybe_unused]] AZ::Entity* entity) override {}
        bool RemoveEntity([[maybe_unused]] AZ::Entity* entity) override { return {}; }
        bool DeleteEntity([[maybe_unused]] const AZ::EntityId& id) override { return {}; }
        void EnumerateEntities([[maybe_unused]] const EntityCallback& callback) override {}
        AZ::SerializeContext* GetSerializeContext() override { return {}; }
        AZ::BehaviorContext* GetBehaviorContext() override { return {}; }
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return {}; }
        const char* GetEngineRoot() const override { return {}; }
        const char* GetExecutableFolder() const override { return {}; }
        void QueryApplicationType([[maybe_unused]] AZ::ApplicationTypeQuery& appType) const override {}

        AZStd::map<AZ::EntityId, AZ::Entity*> m_entities;

        bool AddEntity(AZ::Entity* entity) override
        {
            m_entities[entity->GetId()] = entity;
            return true;
        }

        AZ::Entity* FindEntity(const AZ::EntityId& id) override
        {
            const auto iterator = m_entities.find(id);
            if (iterator != m_entities.end())
            {
                return iterator->second;
            }

            return nullptr;
        }
    };


    class BenchmarkConnectionListener : public AzNetworking::IConnectionListener
    {
    public:
        ConnectResult ValidateConnect([[maybe_unused]] const IpAddress& remoteAddress, [[maybe_unused]] const IPacketHeader& packetHeader, [[maybe_unused]] ISerializer& serializer) override
        {
            return {};
        }

        void OnConnect([[maybe_unused]] IConnection* connection) override
        {
        }

        PacketDispatchResult OnPacketReceived([[maybe_unused]] IConnection* connection, [[maybe_unused]] const IPacketHeader& packetHeader, [[maybe_unused]] ISerializer& serializer) override
        {
            return {};
        }

        void OnPacketLost([[maybe_unused]] IConnection* connection, [[maybe_unused]] PacketId packetId) override
        {
        }

        void OnDisconnect([[maybe_unused]] IConnection* connection, [[maybe_unused]] DisconnectReason reason, [[maybe_unused]] TerminationEndpoint endpoint) override
        {
        }
    };

    class BenchmarkNetworkTime : public Multiplayer::INetworkTime
    {
    public:
        bool IsTimeRewound() const override
        {
            return false;
        }

        HostFrameId GetHostFrameId() const override
        {
            return {};
        }

        HostFrameId GetUnalteredHostFrameId() const override
        {
            return {};
        }

        void IncrementHostFrameId() override
        {
        }

        AZ::TimeMs GetHostTimeMs() const override
        {
            return {};
        }

        float GetHostBlendFactor() const override
        {
            return 0;
        }

        AzNetworking::ConnectionId GetRewindingConnectionId() const override
        {
            return {};
        }

        void ForceSetTime([[maybe_unused]] HostFrameId frameId, [[maybe_unused]] AZ::TimeMs timeMs) override
        {
        }

        void SyncEntitiesToRewindState([[maybe_unused]] const AZ::Aabb& rewindVolume) override
        {
        }

        void ClearRewoundEntities() override
        {
        }

        void AlterTime([[maybe_unused]] HostFrameId frameId, [[maybe_unused]] AZ::TimeMs timeMs, [[maybe_unused]] float blendFactor, [[maybe_unused]] AzNetworking::ConnectionId rewindConnectionId) override
        {
        }
    };

    class BenchmarkMultiplayerConnection : public IConnection
    {
    public:
        BenchmarkMultiplayerConnection(ConnectionId connectionId, const IpAddress& address, [[maybe_unused]] ConnectionRole connectionRole)
            : IConnection(connectionId, address)
        {
            ;
        }

        ~BenchmarkMultiplayerConnection() override = default;

        bool SendReliablePacket([[maybe_unused]] const IPacket& packet) override
        {
            return false;
        }

        PacketId SendUnreliablePacket([[maybe_unused]] const IPacket& packet) override
        {
            return {};
        }

        bool WasPacketAcked([[maybe_unused]] PacketId packetId) const override
        {
            return false;
        }

        ConnectionState GetConnectionState() const override
        {
            return {};
        }

        ConnectionRole GetConnectionRole() const override
        {
            return {};
        }

        bool Disconnect([[maybe_unused]] DisconnectReason reason, [[maybe_unused]] TerminationEndpoint endpoint) override
        {
            return false;
        }

        void SetConnectionMtu([[maybe_unused]] uint32_t connectionMtu) override
        {
        }

        uint32_t GetConnectionMtu() const override
        {
            return 0;
        }
    };

    class BenchmarkNetworkEntityManager : public MockNetworkEntityManager
    {
    public:
        BenchmarkNetworkEntityManager() : m_authorityTracker(*this) {}

        NetworkEntityTracker* GetNetworkEntityTracker() override { return &m_tracker; }
        NetworkEntityAuthorityTracker* GetNetworkEntityAuthorityTracker() override { return &m_authorityTracker; }
        MultiplayerComponentRegistry* GetMultiplayerComponentRegistry() override { return &m_multiplayerComponentRegistry; }
        const HostId& GetHostId() const override { return m_hostId; }

        mutable AZStd::map<NetEntityId, AZ::Entity*> m_networkEntityMap;

        NetworkEntityHandle AddEntityToEntityMap(NetEntityId netEntityId, AZ::Entity* entity) override
        {
            m_networkEntityMap[netEntityId] = entity;
            return NetworkEntityHandle(entity, &m_tracker);
        }

        ConstNetworkEntityHandle GetEntity(NetEntityId netEntityId) const override
        {
            AZ::Entity* entity = m_networkEntityMap[netEntityId];
            return ConstNetworkEntityHandle(entity, &m_tracker);
        }

        NetEntityId GetNetEntityIdById(const AZ::EntityId& entityId) const override
        {
            for (const auto& pair : m_networkEntityMap)
            {
                if (pair.second->GetId() == entityId)
                {
                    return pair.first;
                }
            }

            return InvalidNetEntityId;
        }

        NetworkEntityTracker m_tracker;
        NetworkEntityAuthorityTracker m_authorityTracker;
        MultiplayerComponentRegistry m_multiplayerComponentRegistry;
        HostId m_hostId;
    };

    class BenchmarkMultiplayer : public Multiplayer::IMultiplayer
    {
    public:
        BenchmarkMultiplayer(BenchmarkNetworkEntityManager& manager) : m_manager(manager) {}

        MultiplayerAgentType GetAgentType() const override { return {}; }
        void InitializeMultiplayer([[maybe_unused]] MultiplayerAgentType state) override {}
        bool StartHosting([[maybe_unused]] uint16_t port, [[maybe_unused]] bool isDedicated) override { return {}; }
        bool Connect([[maybe_unused]] const AZStd::string& remoteAddress, [[maybe_unused]] uint16_t port) override { return {}; }
        void Terminate([[maybe_unused]] AzNetworking::DisconnectReason reason) override {}
        void AddClientDisconnectedHandler([[maybe_unused]] ClientDisconnectedEvent::Handler& handler) override {}
        void AddConnectionAcquiredHandler([[maybe_unused]] ConnectionAcquiredEvent::Handler& handler) override {}
        void AddServerAcceptanceReceivedHandler([[maybe_unused]] ServerAcceptanceReceivedEvent::Handler& handler) override {}
        void AddSessionInitHandler([[maybe_unused]] SessionInitEvent::Handler& handler) override {}
        void AddSessionShutdownHandler([[maybe_unused]] SessionShutdownEvent::Handler& handler) override {}
        void SendReadyForEntityUpdates([[maybe_unused]] bool readyForEntityUpdates) override {}
        AZ::TimeMs GetCurrentHostTimeMs() const override { return {}; }
        float GetCurrentBlendFactor() const override { return {}; }
        INetworkTime* GetNetworkTime() override { return {}; }
        INetworkEntityManager* GetNetworkEntityManager() override { return &m_manager; }
        void SetFilterEntityManager([[maybe_unused]] IFilterEntityManager* entityFilter) override {}
        IFilterEntityManager* GetFilterEntityManager() override { return {}; }
        void AddClientMigrationStartEventHandler([[maybe_unused]] ClientMigrationStartEvent::Handler& handler) override {}
        void AddClientMigrationEndEventHandler([[maybe_unused]] ClientMigrationEndEvent::Handler& handler) override {}
        void AddNotifyClientMigrationHandler([[maybe_unused]] NotifyClientMigrationEvent::Handler& handler) override {}
        void AddNotifyEntityMigrationEventHandler([[maybe_unused]] NotifyEntityMigrationEvent::Handler& handler) override {}
        void SendNotifyClientMigrationEvent([[maybe_unused]] AzNetworking::ConnectionId connectionId, [[maybe_unused]] const HostId& hostId,
            [[maybe_unused]] uint64_t userIdentifier, [[maybe_unused]] ClientInputId lastClientInputId, [[maybe_unused]] NetEntityId netEntityId) override {}
        void SendNotifyEntityMigrationEvent([[maybe_unused]] const ConstNetworkEntityHandle& entityHandle, [[maybe_unused]] const HostId& remoteHostId) override {}
        void RegisterPlayerIdentifierForRejoin(uint64_t, NetEntityId) override {}
        void CompleteClientMigration(uint64_t, AzNetworking::ConnectionId, const HostId&, ClientInputId) override {}
        void SetShouldSpawnNetworkEntities([[maybe_unused]] bool value) override {}
        bool GetShouldSpawnNetworkEntities() const override { return true; }

        BenchmarkNetworkEntityManager& m_manager;
    };

    class HierarchyBenchmarkBase
        : public benchmark::Fixture
        , public AllocatorsBase
    {
    public:
        void SetUp(const benchmark::State&) override
        {
            internalSetUp();
        }
        void SetUp(benchmark::State&) override
        {
            internalSetUp();
        }

        void TearDown(const benchmark::State&) override
        {
            internalTearDown();
        }
        void TearDown(benchmark::State&) override
        {
            internalTearDown();
        }

        virtual void internalSetUp()
        {
            SetupAllocator();
            AZ::NameDictionary::Create();

            m_ComponentApplicationRequests = AZStd::make_unique<BenchmarkComponentApplicationRequests>();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(m_ComponentApplicationRequests.get());

            // register components involved in testing
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformDescriptor->Reflect(m_serializeContext.get());

            m_netBindDescriptor.reset(NetBindComponent::CreateDescriptor());
            m_netBindDescriptor->Reflect(m_serializeContext.get());

            m_hierarchyRootDescriptor.reset(NetworkHierarchyRootComponent::CreateDescriptor());
            m_hierarchyRootDescriptor->Reflect(m_serializeContext.get());

            m_hierarchyChildDescriptor.reset(NetworkHierarchyChildComponent::CreateDescriptor());
            m_hierarchyChildDescriptor->Reflect(m_serializeContext.get());

            m_netTransformDescriptor.reset(NetworkTransformComponent::CreateDescriptor());
            m_netTransformDescriptor->Reflect(m_serializeContext.get());

            m_NetworkEntityManager = AZStd::make_unique<BenchmarkNetworkEntityManager>();

            m_Multiplayer = AZStd::make_unique<BenchmarkMultiplayer>(*m_NetworkEntityManager);
            AZ::Interface<IMultiplayer>::Register(m_Multiplayer.get());

            // Create space for replication stats
            // Without Multiplayer::RegisterMultiplayerComponents() the stats go to invalid id, which is fine for unit tests
            GetMultiplayer()->GetStats().ReserveComponentStats(Multiplayer::InvalidNetComponentId, 50, 0);

            m_Time = AZStd::make_unique<AZ::StubTimeSystem>();

            m_NetworkTime = AZStd::make_unique<BenchmarkNetworkTime>();
            AZ::Interface<INetworkTime>::Register(m_NetworkTime.get());

            EXPECT_NE(AZ::Interface<IMultiplayer>::Get()->GetNetworkEntityManager(), nullptr);

            const IpAddress address("localhost", 1, ProtocolType::Udp);
            m_Connection = AZStd::make_unique<BenchmarkMultiplayerConnection>(ConnectionId{ 1 }, address, ConnectionRole::Connector);
            m_ConnectionListener = AZStd::make_unique<BenchmarkConnectionListener>();

            m_entityReplicationManager = AZStd::make_unique<EntityReplicationManager>(*m_Connection, *m_ConnectionListener, EntityReplicationManager::Mode::LocalClientToRemoteServer);

            m_console.reset(aznew AZ::Console());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());

            RegisterMultiplayerComponents();
        }

        virtual void internalTearDown()
        {
            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();

            m_entityReplicationManager.reset();

            m_Connection.reset();
            m_ConnectionListener.reset();

            AZ::Interface<INetworkTime>::Unregister(m_NetworkTime.get());
            AZ::Interface<IMultiplayer>::Unregister(m_Multiplayer.get());
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(m_ComponentApplicationRequests.get());

            m_Time.reset();

            m_NetworkEntityManager.reset();
            m_Multiplayer.reset();

            m_transformDescriptor.reset();
            m_netTransformDescriptor.reset();
            m_hierarchyRootDescriptor.reset();
            m_hierarchyChildDescriptor.reset();
            m_netBindDescriptor.reset();
            m_serializeContext.reset();
            m_ComponentApplicationRequests.reset();

            AZ::NameDictionary::Destroy();
            TeardownAllocator();
        }

        AZStd::unique_ptr<AZ::IConsole> m_console;

        AZStd::unique_ptr<BenchmarkComponentApplicationRequests> m_ComponentApplicationRequests;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netBindDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_hierarchyRootDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_hierarchyChildDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netTransformDescriptor;

        AZStd::unique_ptr<BenchmarkMultiplayer> m_Multiplayer;
        AZStd::unique_ptr<BenchmarkNetworkEntityManager> m_NetworkEntityManager;
        AZStd::unique_ptr<AZ::StubTimeSystem> m_Time;
        AZStd::unique_ptr<BenchmarkNetworkTime> m_NetworkTime;

        AZStd::unique_ptr<BenchmarkMultiplayerConnection> m_Connection;
        AZStd::unique_ptr<BenchmarkConnectionListener> m_ConnectionListener;

        AZStd::unique_ptr<EntityReplicationManager> m_entityReplicationManager;

        void SetupEntity(const AZStd::unique_ptr<AZ::Entity>& entity, NetEntityId netId, NetEntityRole role)
        {
            const auto netBindComponent = entity->FindComponent<Multiplayer::NetBindComponent>();
            EXPECT_NE(netBindComponent, nullptr);
            netBindComponent->PreInit(entity.get(), PrefabEntityId{ AZ::Name("test"), 1 }, netId, role);
            entity->Init();
        }

        static void StopEntity(const AZStd::unique_ptr<AZ::Entity>& entity)
        {
            const auto netBindComponent = entity->FindComponent<Multiplayer::NetBindComponent>();
            EXPECT_NE(netBindComponent, nullptr);
            netBindComponent->StopEntity();
        }

        static void StopAndDeactivateEntity(AZStd::unique_ptr<AZ::Entity>& entity)
        {
            if (entity)
            {
                StopEntity(entity);
                entity->Deactivate();
                entity.reset();
            }
        }

        void CreateEntityWithRootHierarchy(AZStd::unique_ptr<AZ::Entity>& rootEntity)
        {
            rootEntity->CreateComponent<AzFramework::TransformComponent>();
            rootEntity->CreateComponent<NetBindComponent>();
            rootEntity->CreateComponent<NetworkTransformComponent>();
            rootEntity->CreateComponent<NetworkHierarchyRootComponent>();
        }

        void CreateEntityWithChildHierarchy(AZStd::unique_ptr<AZ::Entity>& childEntity)
        {
            childEntity->CreateComponent<AzFramework::TransformComponent>();
            childEntity->CreateComponent<NetBindComponent>();
            childEntity->CreateComponent<NetworkTransformComponent>();
            childEntity->CreateComponent<NetworkHierarchyChildComponent>();
        }

        void SetParentIdOnNetworkTransform(const AZStd::unique_ptr<AZ::Entity>& entity, NetEntityId netParentId)
        {
            /* Derived from NetworkTransformComponent.AutoComponent.xml */
            constexpr int totalBits = 6 /*NetworkTransformComponentInternal::AuthorityToClientDirtyEnum::Count*/;
            constexpr int parentIdBit = 4 /*NetworkTransformComponentInternal::AuthorityToClientDirtyEnum::parentEntityId_DirtyFlag*/;

            ReplicationRecord currentRecord;
            currentRecord.m_authorityToClient.AddBits(totalBits);
            currentRecord.m_authorityToClient.SetBit(parentIdBit, true);

            constexpr uint32_t bufferSize = 100;
            AZStd::array<uint8_t, bufferSize> buffer = {};
            NetworkInputSerializer inSerializer(buffer.begin(), bufferSize);
            ISerializer& serializer = inSerializer;
            serializer.Serialize(netParentId, "parentEntityId"); // Derived from NetworkTransformComponent.AutoComponent.xml

            NetworkOutputSerializer outSerializer(buffer.begin(), bufferSize);

            ReplicationRecord notifyRecord = currentRecord;
            entity->FindComponent<NetworkTransformComponent>()->SerializeStateDeltaMessage(currentRecord, outSerializer);
            entity->FindComponent<NetworkTransformComponent>()->NotifyStateDeltaChanges(notifyRecord);
        }

        template <typename Component>
        void SetHierarchyRootFieldOnNetworkHierarchyChild(const AZStd::unique_ptr<AZ::Entity>& entity, NetEntityId value)
        {
            /* Derived from NetworkHierarchyChildComponent.AutoComponent.xml */
            constexpr int totalBits = 1 /*NetworkHierarchyChildComponentInternal::AuthorityToClientDirtyEnum::Count*/;
            constexpr int inHierarchyBit = 0 /*NetworkHierarchyChildComponentInternal::AuthorityToClientDirtyEnum::hierarchyRoot_DirtyFlag*/;

            ReplicationRecord currentRecord;
            currentRecord.m_authorityToClient.AddBits(totalBits);
            currentRecord.m_authorityToClient.SetBit(inHierarchyBit, true);

            constexpr uint32_t bufferSize = 100;
            AZStd::array<uint8_t, bufferSize> buffer = {};
            NetworkInputSerializer inSerializer(buffer.begin(), bufferSize);
            ISerializer& serializer = inSerializer;
            serializer.Serialize(value, "hierarchyRoot"); // Derived from NetworkHierarchyChildComponent.AutoComponent.xml

            NetworkOutputSerializer outSerializer(buffer.begin(), bufferSize);

            ReplicationRecord notifyRecord = currentRecord;
            entity->FindComponent<Component>()->SerializeStateDeltaMessage(currentRecord, outSerializer);
            entity->FindComponent<Component>()->NotifyStateDeltaChanges(notifyRecord);
        }

        struct EntityInfo
        {
            enum class Role
            {
                Root,
                Child,
                None
            };

            EntityInfo(AZ::u64 entityId, const char* entityName, NetEntityId netId, Role role)
                : m_entity(AZStd::make_unique<AZ::Entity>(AZ::EntityId(entityId), entityName))
                , m_netId(netId)
                , m_role(role)
            {
            }

            ~EntityInfo()
            {
                StopAndDeactivateEntity(m_entity);
            }

            AZStd::unique_ptr<AZ::Entity> m_entity;
            NetEntityId m_netId;
            AZStd::unique_ptr<EntityReplicator> m_replicator;
            Role m_role = Role::None;
        };

        void PopulateHierarchicalEntity(const EntityInfo& entityInfo)
        {
            entityInfo.m_entity->CreateComponent<AzFramework::TransformComponent>();
            entityInfo.m_entity->CreateComponent<NetBindComponent>();
            entityInfo.m_entity->CreateComponent<NetworkTransformComponent>();
            switch (entityInfo.m_role)
            {
            case EntityInfo::Role::Root:
                entityInfo.m_entity->CreateComponent<NetworkHierarchyRootComponent>();
                break;
            case EntityInfo::Role::Child:
                entityInfo.m_entity->CreateComponent<NetworkHierarchyChildComponent>();
                break;
            case EntityInfo::Role::None:
                break;
            }
        }

        void CreateParent(EntityInfo& parent)
        {
            PopulateHierarchicalEntity(parent);

            SetupEntity(parent.m_entity, parent.m_netId, NetEntityRole::Authority);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childHandle(parent.m_entity.get(), m_NetworkEntityManager->GetNetworkEntityTracker());
            parent.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_Connection.get(), NetEntityRole::Client, childHandle);
            parent.m_replicator->Initialize(childHandle);

            parent.m_entity->Activate();
        }

        void CreateChildForParent(EntityInfo& child, EntityInfo& parent)
        {
            PopulateHierarchicalEntity(child);

            SetupEntity(child.m_entity, child.m_netId, NetEntityRole::Authority);

            // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
            SetParentIdOnNetworkTransform(child.m_entity, parent.m_netId);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childHandle(child.m_entity.get(), m_NetworkEntityManager->GetNetworkEntityTracker());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_Connection.get(), NetEntityRole::Client, childHandle);
            child.m_replicator->Initialize(childHandle);

            child.m_entity->Activate();
        }

        void ForceRebuildHierarchy(const AZStd::unique_ptr<AZ::Entity>& rootEntity)
        {
            if (NetworkHierarchyRootComponent* root = rootEntity->FindComponent<NetworkHierarchyRootComponent>())
            {
                root->RebuildHierarchy();
            }
        }
    };
}

#endif
