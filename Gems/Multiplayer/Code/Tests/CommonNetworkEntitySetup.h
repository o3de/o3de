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
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzCore/UnitTest/Mocks/MockITime.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Visibility/EntityVisibilityBoundsUnionSystem.h>
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
#include <Source/NetworkEntity/NetworkEntityManager.h>
#include <NetworkEntity/NetworkEntityAuthorityTracker.h>
#include <NetworkEntity/NetworkEntityTracker.h>
#include <Tests/TestMultiplayerComponent.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    class NetworkEntityTests
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            AZ::NameDictionary::Create();

            m_mockComponentApplicationRequests = AZStd::make_unique<NiceMock<MockComponentApplicationRequests>>();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(m_mockComponentApplicationRequests.get());

            ON_CALL(*m_mockComponentApplicationRequests, AddEntity(_)).WillByDefault(Invoke(this, &NetworkEntityTests::AddEntity));
            ON_CALL(*m_mockComponentApplicationRequests, FindEntity(_)).WillByDefault(Invoke(this, &NetworkEntityTests::FindEntity));

            // register components involved in testing
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();

            m_transformDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformDescriptor->Reflect(m_serializeContext.get());

            m_netBindDescriptor.reset(NetBindComponent::CreateDescriptor());
            m_netBindDescriptor->Reflect(m_serializeContext.get());
            m_netBindDescriptor->Reflect(m_behaviorContext.get());

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

            m_visisbilitySystem = AZStd::make_unique<AzFramework::EntityVisibilityBoundsUnionSystem>();
            m_visisbilitySystem->Connect();
            m_networkEntityManager = AZStd::make_unique<NetworkEntityManager>();

            m_mockTime = AZStd::make_unique<AZ::NiceTimeSystemMock>();

            m_eventScheduler = AZStd::make_unique<AZ::EventSchedulerSystemComponent>();

            m_mockNetworkTime = AZStd::make_unique<NiceMock<MockNetworkTime>>();
            AZ::Interface<INetworkTime>::Register(m_mockNetworkTime.get());

            ON_CALL(*m_mockMultiplayer, GetNetworkEntityManager()).WillByDefault(Return(m_networkEntityManager.get()));
            EXPECT_NE(AZ::Interface<IMultiplayer>::Get()->GetNetworkEntityManager(), nullptr);

            const IpAddress address("localhost", 1, ProtocolType::Udp);
            m_mockConnection = AZStd::make_unique<NiceMock<IMultiplayerConnectionMock>>(ConnectionId{ 1 }, address, ConnectionRole::Connector);
            m_mockConnectionListener = AZStd::make_unique<MockConnectionListener>();

            m_entityReplicationManager = AZStd::make_unique<EntityReplicationManager>(*m_mockConnection, *m_mockConnectionListener, EntityReplicationManager::Mode::LocalClientToRemoteServer);

            m_console.reset(aznew AZ::Console());
            AZ::Interface<AZ::IConsole>::Register(m_console.get());
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());

            m_multiplayerComponentRegistry = AZStd::make_unique<MultiplayerComponentRegistry>();
            RegisterMultiplayerComponents();
            MultiplayerTest::RegisterMultiplayerComponents();
        }

        void TearDown() override
        {
            m_multiplayerComponentRegistry.reset();

            AZ::Interface<AZ::IConsole>::Unregister(m_console.get());
            m_console.reset();

            m_entities.clear();

            m_entityReplicationManager.reset();

            m_mockConnection.reset();
            m_mockConnectionListener.reset();

            AZ::Interface<INetworkTime>::Unregister(m_mockNetworkTime.get());
            AZ::Interface<IMultiplayer>::Unregister(m_mockMultiplayer.get());
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(m_mockComponentApplicationRequests.get());

            m_eventScheduler.reset();
            m_mockTime.reset();

            m_networkEntityManager.reset();
            m_mockMultiplayer.reset();
            m_visisbilitySystem->Disconnect();
            m_visisbilitySystem.reset();

            m_testInputDriverComponentDescriptor.reset();
            m_testMultiplayerComponentDescriptor.reset();
            m_transformDescriptor.reset();
            m_netTransformDescriptor.reset();
            m_hierarchyRootDescriptor.reset();
            m_hierarchyChildDescriptor.reset();
            m_netBindDescriptor.reset();
            m_behaviorContext.reset();
            m_serializeContext.reset();
            m_mockComponentApplicationRequests.reset();

            AZ::NameDictionary::Destroy();
        }

        AZStd::unique_ptr<AZ::IConsole> m_console;

        AZStd::unique_ptr<NiceMock<MockComponentApplicationRequests>> m_mockComponentApplicationRequests;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netBindDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_hierarchyRootDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_hierarchyChildDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netTransformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_testMultiplayerComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_testInputDriverComponentDescriptor;

        AZStd::unique_ptr<NiceMock<MockMultiplayer>> m_mockMultiplayer;
        AZStd::unique_ptr<NetworkEntityManager> m_networkEntityManager;
        AZStd::unique_ptr<AZ::EventSchedulerSystemComponent> m_eventScheduler;
        AZStd::unique_ptr<AZ::NiceTimeSystemMock> m_mockTime;
        AZStd::unique_ptr<NiceMock<MockNetworkTime>> m_mockNetworkTime;
        AZStd::unique_ptr<AzFramework::EntityVisibilityBoundsUnionSystem> m_visisbilitySystem;
        AZStd::unique_ptr<NiceMock<IMultiplayerConnectionMock>> m_mockConnection;
        AZStd::unique_ptr<MockConnectionListener> m_mockConnectionListener;

        AZStd::unique_ptr<EntityReplicationManager> m_entityReplicationManager;

        AZStd::unique_ptr<MultiplayerComponentRegistry> m_multiplayerComponentRegistry;;

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
    };
}
