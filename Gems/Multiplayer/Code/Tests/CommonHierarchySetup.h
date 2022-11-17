/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <IMultiplayerConnectionMock.h>
#include <MockInterfaces.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EventSchedulerSystemComponent.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UnitTest/Mocks/MockITime.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>
#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicationManager.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>
#include <NetworkEntity/NetworkEntityAuthorityTracker.h>
#include <NetworkEntity/NetworkEntityTracker.h>
#include <Tests/TestMultiplayerComponent.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    class NetworkHierarchyCallbacks
    {
    public:
        virtual ~NetworkHierarchyCallbacks() = default;
        virtual void OnNetworkHierarchyLeave() = 0;
        virtual void OnNetworkHierarchyUpdated(const AZ::EntityId& hierarchyRootId) = 0;
    };

    class MockNetworkHierarchyCallbackHandler : public NetworkHierarchyCallbacks
    {
    public:
        MockNetworkHierarchyCallbackHandler()
            : m_leaveHandler([this]() { OnNetworkHierarchyLeave(); })
            , m_changedHandler([this](const AZ::EntityId& rootId) { OnNetworkHierarchyUpdated(rootId); })
        {
        }

        NetworkHierarchyLeaveEvent::Handler m_leaveHandler;
        NetworkHierarchyChangedEvent::Handler m_changedHandler;

        MOCK_METHOD0(OnNetworkHierarchyLeave, void());
        MOCK_METHOD1(OnNetworkHierarchyUpdated, void(const AZ::EntityId&));
    };

    class HierarchyTests
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
            AZ::NameDictionary::Create();

            m_mockComponentApplicationRequests = AZStd::make_unique<NiceMock<MockComponentApplicationRequests>>();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(m_mockComponentApplicationRequests.get());

            ON_CALL(*m_mockComponentApplicationRequests, AddEntity(_)).WillByDefault(Invoke(this, &HierarchyTests::AddEntity));
            ON_CALL(*m_mockComponentApplicationRequests, FindEntity(_)).WillByDefault(Invoke(this, &HierarchyTests::FindEntity));

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

            m_testMultiplayerComponentDescriptor.reset(MultiplayerTest::TestMultiplayerComponent::CreateDescriptor());
            m_testMultiplayerComponentDescriptor->Reflect(m_serializeContext.get());

            m_testInputDriverComponentDescriptor.reset(MultiplayerTest::TestInputDriverComponent::CreateDescriptor());
            m_testInputDriverComponentDescriptor->Reflect(m_serializeContext.get());

            m_mockMultiplayer = AZStd::make_unique<NiceMock<MockMultiplayer>>();
            AZ::Interface<IMultiplayer>::Register(m_mockMultiplayer.get());

            EXPECT_NE(AZ::Interface<IMultiplayer>::Get(), nullptr);

            // Create space for replication stats
            // Without Multiplayer::RegisterMultiplayerComponents() the stats go to invalid id, which is fine for unit tests
            GetMultiplayer()->GetStats().ReserveComponentStats(Multiplayer::InvalidNetComponentId, 50, 0);

            m_mockNetworkEntityManager = AZStd::make_unique<NiceMock<MockNetworkEntityManager>>();
            AZ::Interface<INetworkEntityManager>::Register(m_mockNetworkEntityManager.get());

            ON_CALL(*m_mockNetworkEntityManager, AddEntityToEntityMap(_, _)).WillByDefault(Invoke(this, &HierarchyTests::AddEntityToEntityMap));
            ON_CALL(*m_mockNetworkEntityManager, GetEntity(_)).WillByDefault(Invoke(this, &HierarchyTests::GetEntity));
            ON_CALL(*m_mockNetworkEntityManager, GetNetEntityIdById(_)).WillByDefault(Invoke(this, &HierarchyTests::GetNetEntityIdById));

            m_mockTime = AZStd::make_unique<AZ::NiceTimeSystemMock>();

            m_eventScheduler = AZStd::make_unique<AZ::EventSchedulerSystemComponent>();

            m_mockNetworkTime = AZStd::make_unique<NiceMock<MockNetworkTime>>();
            AZ::Interface<INetworkTime>::Register(m_mockNetworkTime.get());

            ON_CALL(*m_mockMultiplayer, GetNetworkEntityManager()).WillByDefault(Return(m_mockNetworkEntityManager.get()));
            EXPECT_NE(AZ::Interface<IMultiplayer>::Get()->GetNetworkEntityManager(), nullptr);

            const IpAddress address("localhost", 1, ProtocolType::Udp);
            m_mockConnection = AZStd::make_unique<NiceMock<IMultiplayerConnectionMock>>(ConnectionId{ 1 }, address, ConnectionRole::Connector);
            m_mockConnectionListener = AZStd::make_unique<MockConnectionListener>();

            m_networkEntityTracker = AZStd::make_unique<NetworkEntityTracker>();
            ON_CALL(*m_mockNetworkEntityManager, GetNetworkEntityTracker()).WillByDefault(Return(m_networkEntityTracker.get()));

            m_networkEntityAuthorityTracker = AZStd::make_unique<NetworkEntityAuthorityTracker>(*m_mockNetworkEntityManager);
            ON_CALL(*m_mockNetworkEntityManager, GetNetworkEntityAuthorityTracker()).WillByDefault(Return(m_networkEntityAuthorityTracker.get()));

            m_entityReplicationManager = AZStd::make_unique<EntityReplicationManager>(*m_mockConnection, *m_mockConnectionListener, EntityReplicationManager::Mode::LocalClientToRemoteServer);

            m_console.reset(aznew AZ::Console());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());

            m_multiplayerComponentRegistry = AZStd::make_unique<MultiplayerComponentRegistry>();
            ON_CALL(*m_mockNetworkEntityManager, GetMultiplayerComponentRegistry()).WillByDefault(Return(m_multiplayerComponentRegistry.get()));
            RegisterMultiplayerComponents();
            MultiplayerTest::RegisterMultiplayerComponents();
        }

        void TearDown() override
        {
            m_multiplayerComponentRegistry.reset();

            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();

            m_networkEntityMap.clear();
            m_entities.clear();

            m_entityReplicationManager.reset();

            m_mockConnection.reset();
            m_mockConnectionListener.reset();
            m_networkEntityTracker.reset();
            m_networkEntityAuthorityTracker.reset();

            AZ::Interface<INetworkTime>::Unregister(m_mockNetworkTime.get());
            AZ::Interface<INetworkEntityManager>::Unregister(m_mockNetworkEntityManager.get());
            AZ::Interface<IMultiplayer>::Unregister(m_mockMultiplayer.get());
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(m_mockComponentApplicationRequests.get());

            m_eventScheduler.reset();
            m_mockTime.reset();

            m_mockNetworkEntityManager.reset();
            m_mockMultiplayer.reset();

            m_testInputDriverComponentDescriptor.reset();
            m_testMultiplayerComponentDescriptor.reset();
            m_transformDescriptor.reset();
            m_netTransformDescriptor.reset();
            m_hierarchyRootDescriptor.reset();
            m_hierarchyChildDescriptor.reset();
            m_netBindDescriptor.reset();
            m_serializeContext.reset();
            m_mockComponentApplicationRequests.reset();

            AZ::NameDictionary::Destroy();
        }

        AZStd::unique_ptr<AZ::IConsole> m_console;

        AZStd::unique_ptr<NiceMock<MockComponentApplicationRequests>> m_mockComponentApplicationRequests;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netBindDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_hierarchyRootDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_hierarchyChildDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netTransformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_testMultiplayerComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_testInputDriverComponentDescriptor;

        AZStd::unique_ptr<NiceMock<MockMultiplayer>> m_mockMultiplayer;
        AZStd::unique_ptr<MockNetworkEntityManager> m_mockNetworkEntityManager;
        AZStd::unique_ptr<AZ::EventSchedulerSystemComponent> m_eventScheduler;
        AZStd::unique_ptr<AZ::NiceTimeSystemMock> m_mockTime;
        AZStd::unique_ptr<NiceMock<MockNetworkTime>> m_mockNetworkTime;

        AZStd::unique_ptr<NiceMock<IMultiplayerConnectionMock>> m_mockConnection;
        AZStd::unique_ptr<MockConnectionListener> m_mockConnectionListener;
        AZStd::unique_ptr<NetworkEntityTracker> m_networkEntityTracker;
        AZStd::unique_ptr<NetworkEntityAuthorityTracker> m_networkEntityAuthorityTracker;

        AZStd::unique_ptr<EntityReplicationManager> m_entityReplicationManager;

        AZStd::unique_ptr<MultiplayerComponentRegistry> m_multiplayerComponentRegistry;;

        mutable AZStd::map<NetEntityId, AZ::Entity*> m_networkEntityMap;

        NetworkEntityHandle AddEntityToEntityMap(NetEntityId netEntityId, AZ::Entity* entity)
        {
            m_networkEntityMap[netEntityId] = entity;
            return NetworkEntityHandle(entity, m_networkEntityTracker.get());
        }

        ConstNetworkEntityHandle GetEntity(NetEntityId netEntityId) const
        {
            AZ::Entity* entity = m_networkEntityMap[netEntityId];
            return ConstNetworkEntityHandle(entity, m_networkEntityTracker.get());
        }

        NetEntityId GetNetEntityIdById(const AZ::EntityId& entityId) const
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

        AZStd::map<AZ::EntityId, AZ::Entity*> m_entities;

        bool AddEntity(AZ::Entity* entity)
        {
            m_entities[entity->GetId()] = entity;
            return true;
        }

        AZ::Entity* FindEntity(AZ::EntityId entityId)
        {
            const auto iterator = m_entities.find(entityId);
            if (iterator != m_entities.end())
            {
                return iterator->second;
            }

            return nullptr;
        }

        void SetupEntity(const AZStd::unique_ptr<AZ::Entity>& entity, NetEntityId netId, NetEntityRole role)
        {
            if (const auto netBindComponent = entity->FindComponent<Multiplayer::NetBindComponent>())
            {
                netBindComponent->PreInit(entity.get(), PrefabEntityId{ AZ::Name("test"), 1 }, netId, role);
                entity->Init();
            }
        }

        static void StopEntity(const AZStd::unique_ptr<AZ::Entity>& entity)
        {
            if (const auto netBindComponent = entity->FindComponent<Multiplayer::NetBindComponent>())
            {
                netBindComponent->StopEntity();
            }
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

        void SetTranslationOnNetworkTransform(const AZStd::unique_ptr<AZ::Entity>& entity, AZ::Vector3 translation)
        {
            /* Derived from NetworkTransformComponent.AutoComponent.xml */
            constexpr int totalBits = 6 /*NetworkTransformComponentInternal::AuthorityToClientDirtyEnum::Count*/;
            constexpr int translationBit = 1 /*NetworkTransformComponentInternal::AuthorityToClientDirtyEnum::translation_DirtyFlag*/;

            ReplicationRecord currentRecord;
            currentRecord.m_authorityToClient.AddBits(totalBits);
            currentRecord.m_authorityToClient.SetBit(translationBit, true);

            constexpr uint32_t bufferSize = 100;
            AZStd::array<uint8_t, bufferSize> buffer = {};
            NetworkInputSerializer inSerializer(buffer.begin(), bufferSize);
            static_cast<ISerializer*>(&inSerializer)->Serialize(translation,
                "translation" /* Derived from NetworkTransformComponent.AutoComponent.xml */);

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
            entityInfo.m_entity->CreateComponent<MultiplayerTest::TestMultiplayerComponent>();
            entityInfo.m_entity->CreateComponent<MultiplayerTest::TestInputDriverComponent>();

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

        void CreateDeepHierarchy(EntityInfo& root, EntityInfo& child, EntityInfo& childOfChild)
        {
            PopulateHierarchicalEntity(root);
            PopulateHierarchicalEntity(child);
            PopulateHierarchicalEntity(childOfChild);

            SetupEntity(root.m_entity, root.m_netId, NetEntityRole::Authority);
            SetupEntity(child.m_entity, child.m_netId, NetEntityRole::Authority);
            SetupEntity(childOfChild.m_entity, childOfChild.m_netId, NetEntityRole::Authority);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childOfChildHandle(childOfChild.m_entity.get(), m_networkEntityTracker.get());
            childOfChild.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, childOfChildHandle);
            childOfChild.m_replicator->Initialize(childOfChildHandle);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childHandle(child.m_entity.get(), m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, childHandle);
            child.m_replicator->Initialize(childHandle);

            // Create an entity replicator for the root entity
            const NetworkEntityHandle rootHandle(root.m_entity.get(), m_networkEntityTracker.get());
            root.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, rootHandle);
            root.m_replicator->Initialize(rootHandle);

            root.m_entity->Activate();
            child.m_entity->Activate();
            childOfChild.m_entity->Activate();
        }
    };
}
