/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CommonHierarchySetup.h>
#include <MockInterfaces.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Name/Name.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>
#include <Multiplayer/NetworkInput/NetworkInputArray.h>
#include <Source/NetworkEntity/NetworkEntityManager.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    /*
     * Test NetBindComponent activation. This must work before more complicated tests.
     */
    TEST_F(HierarchyTests, On_Client_NetBindComponent_Activate)
    {
        AZStd::unique_ptr<AZ::Entity> entity = AZStd::make_unique<AZ::Entity>();
        entity->CreateComponent<NetBindComponent>();
        SetupEntity(entity, NetEntityId{ 1 }, NetEntityRole::Client);
        entity->Activate();

        StopEntity(entity);

        entity->Deactivate();
    }

    /*
     * Hierarchy test - a child entity on a client delaying activation until its hierarchical parent has been activated
     */
    TEST_F(HierarchyTests, On_Client_EntityReplicator_DontActivate_BeforeParent)
    {
        // Create a child entity that will be tested for activation inside a hierarchy
        AZStd::unique_ptr<AZ::Entity> childEntity = AZStd::make_unique<AZ::Entity>();
        CreateEntityWithChildHierarchy(childEntity);
        SetupEntity(childEntity, NetEntityId{ 2 }, NetEntityRole::Client);
        // child entity is not activated on purpose here, we are about to test conditional activation check

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(childEntity, NetEntityId{ 1 });
        SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyChildComponent>(childEntity, NetEntityId{ 1 });

        // Create an entity replicator for the child entity
        const NetworkEntityHandle childHandle(childEntity.get(), m_networkEntityTracker.get());
        EntityReplicator entityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
        entityReplicator.Initialize(childHandle);

        // Entity replicator should not be ready to activate the entity because its parent does not exist
        EXPECT_EQ(entityReplicator.IsReadyToActivate(), false);
    }

    TEST_F(HierarchyTests, On_Client_EntityReplicator_DontActivate_Inner_Root_Before_Top_Root)
    {
        // Create a child entity that will be tested for activation inside a hierarchy
        AZStd::unique_ptr<AZ::Entity> innerRootEntity = AZStd::make_unique<AZ::Entity>();
        CreateEntityWithRootHierarchy(innerRootEntity);
        SetupEntity(innerRootEntity, NetEntityId{ 2 }, NetEntityRole::Client);
        // child entity is not activated on purpose here, we are about to test conditional activation check

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(innerRootEntity, NetEntityId{ 1 });
        SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyRootComponent>(innerRootEntity, NetEntityId{ 1 });

        // Create an entity replicator for the child entity
        const NetworkEntityHandle innerRootHandle(innerRootEntity.get(), m_networkEntityTracker.get());
        EntityReplicator entityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, innerRootHandle);
        entityReplicator.Initialize(innerRootHandle);

        // Entity replicator should not be ready to activate the entity because its parent does not exist
        EXPECT_EQ(entityReplicator.IsReadyToActivate(), false);
    }

    TEST_F(HierarchyTests, On_Client_Not_In_Hierarchy_EntityReplicator_Ignores_Parent)
    {
        // Create a child entity that will be tested for activation inside a hierarchy
        AZStd::unique_ptr<AZ::Entity> childEntity = AZStd::make_unique<AZ::Entity>();
        CreateEntityWithChildHierarchy(childEntity);
        SetupEntity(childEntity, NetEntityId{ 2 }, NetEntityRole::Client);
        // child entity is not activated on purpose here, we are about to test conditional activation check

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(childEntity, NetEntityId{ 1 });
        SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyChildComponent>(childEntity, InvalidNetEntityId);

        // Create an entity replicator for the child entity
        const NetworkEntityHandle childHandle(childEntity.get(), m_networkEntityTracker.get());
        EntityReplicator entityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
        entityReplicator.Initialize(childHandle);

        // Entity replicator should not be ready to activate the entity because its parent does not exist
        EXPECT_EQ(entityReplicator.IsReadyToActivate(), true);
    }

    /*
     * Hierarchy test - a child entity on a client allowing activation when its hierarchical parent is active
     */
    TEST_F(HierarchyTests, On_Client_EntityReplicator_Activates_AfterParent)
    {
        AZStd::unique_ptr<AZ::Entity> childEntity = AZStd::make_unique<AZ::Entity>();
        CreateEntityWithChildHierarchy(childEntity);
        SetupEntity(childEntity, NetEntityId{ 2 }, NetEntityRole::Client);

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(childEntity, NetEntityId{ 1 });
        SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyChildComponent>(childEntity, NetEntityId{ 1 });

        // Create an entity replicator for the child entity
        const NetworkEntityHandle childHandle(childEntity.get(), m_networkEntityTracker.get());
        EntityReplicator childEntityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
        childEntityReplicator.Initialize(childHandle);

        // Now let's create a parent entity and activate it
        AZStd::unique_ptr<AZ::Entity> parentEntity = AZStd::make_unique<AZ::Entity>();
        CreateEntityWithRootHierarchy(parentEntity);
        SetupEntity(parentEntity, NetEntityId{ 1 }, NetEntityRole::Client);

        // Create an entity replicator for the parent entity
        const NetworkEntityHandle parentHandle(parentEntity.get(), m_networkEntityTracker.get());
        ON_CALL(*m_mockNetworkEntityManager, GetEntity(_)).WillByDefault(Return(parentHandle));
        EntityReplicator parentEntityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, parentHandle);
        parentEntityReplicator.Initialize(parentHandle);

        parentEntity->Activate();

        // The child should be ready to be activated
        EXPECT_EQ(childEntityReplicator.IsReadyToActivate(), true);

        StopEntity(parentEntity);

        parentEntity->Deactivate();
    }

    /*
     * Parent -> Child
     */
    class ClientSimpleHierarchyTests : public HierarchyTests
    {
    public:
        const NetEntityId RootNetEntityId = NetEntityId{ 1 };
        const NetEntityId ChildNetEntityId = NetEntityId{ 2 };

        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_root = AZStd::make_unique<EntityInfo>(1, "root", RootNetEntityId, EntityInfo::Role::Root);
            m_child = AZStd::make_unique<EntityInfo>(2, "child", ChildNetEntityId, EntityInfo::Role::Child);

            CreateSimpleHierarchy(*m_root, *m_child);

            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());
            // now the two entities are under one hierarchy
        }

        void TearDown() override
        {
            m_child.reset();
            m_root.reset();

            HierarchyTests::TearDown();
        }

        void CreateSimpleHierarchy(EntityInfo& root, EntityInfo& child)
        {
            PopulateHierarchicalEntity(root);
            SetupEntity(root.m_entity, root.m_netId, NetEntityRole::Autonomous);

            PopulateHierarchicalEntity(child);
            SetupEntity(child.m_entity, child.m_netId, NetEntityRole::Autonomous);

            // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
            SetParentIdOnNetworkTransform(child.m_entity, root.m_netId);
            SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyChildComponent>(child.m_entity, root.m_netId);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childHandle(child.m_entity.get(), m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
            child.m_replicator->Initialize(childHandle);

            // Create an entity replicator for the root entity
            const NetworkEntityHandle rootHandle(root.m_entity.get(), m_networkEntityTracker.get());
            root.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, rootHandle);
            root.m_replicator->Initialize(rootHandle);

            root.m_entity->Activate();
            child.m_entity->Activate();
        }

        void SetHierarchyRootFieldOnNetworkHierarchyChildOnClient(const AZStd::unique_ptr<AZ::Entity>& entity, NetEntityId value)
        {
            /* Derived from NetworkHierarchyChildComponent.AutoComponent.xml */
            constexpr int totalBits = 1 /*NetworkHierarchyChildComponentInternal::AuthorityToClientDirtyEnum::Count*/;
            constexpr int inHierarchyBit = 0 /*NetworkHierarchyChildComponentInternal::AuthorityToClientDirtyEnum::hierarchyRoot_DirtyFlag*/;

            ReplicationRecord currentRecord(NetEntityRole::Client);
            currentRecord.m_authorityToClient.AddBits(totalBits);
            currentRecord.m_authorityToClient.SetBit(inHierarchyBit, true);

            constexpr uint32_t bufferSize = 100;
            AZStd::array<uint8_t, bufferSize> buffer = {};
            NetworkInputSerializer inSerializer(buffer.begin(), bufferSize);
            ISerializer& serializer = inSerializer;
            serializer.Serialize(value, "hierarchyRoot"); // Derived from NetworkHierarchyChildComponent.AutoComponent.xml

            NetworkOutputSerializer outSerializer(buffer.begin(), bufferSize);

            ReplicationRecord notifyRecord = currentRecord;

            entity->FindComponent<NetworkHierarchyChildComponent>()->SerializeStateDeltaMessage(currentRecord, outSerializer);
            entity->FindComponent<NetworkHierarchyChildComponent>()->NotifyStateDeltaChanges(notifyRecord);
        }

        AZStd::unique_ptr<EntityInfo> m_root;
        AZStd::unique_ptr<EntityInfo> m_child;
    };

    TEST_F(ClientSimpleHierarchyTests, Client_Activates_Hierarchy_From_Network_Fields)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            m_root->m_entity.get()
        );

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
        if (m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 2)
        {
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[0],
                m_root->m_entity.get()
            );
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[1],
                m_child->m_entity.get()
            );
        }
    }

    TEST_F(ClientSimpleHierarchyTests, Client_Detaches_Child_When_Server_Detaches)
    {
        // simulate server detaching child entity
        SetParentIdOnNetworkTransform(m_child->m_entity, InvalidNetEntityId);
        SetHierarchyRootFieldOnNetworkHierarchyChildOnClient(m_child->m_entity, InvalidNetEntityId);

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            nullptr
        );
    }

    TEST_F(ClientSimpleHierarchyTests, Client_Sends_NetworkHierarchy_Updated_Event_On_Child_Detached_On_Server)
    {
        MockNetworkHierarchyCallbackHandler mock;
        EXPECT_CALL(mock, OnNetworkHierarchyUpdated(m_root->m_entity->GetId()));

        m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->BindNetworkHierarchyChangedEventHandler(mock.m_changedHandler);

        // simulate server detaching a child entity
        SetParentIdOnNetworkTransform(m_child->m_entity, InvalidNetEntityId);
        SetHierarchyRootFieldOnNetworkHierarchyChildOnClient(m_child->m_entity, InvalidNetEntityId);
    }

    TEST_F(ClientSimpleHierarchyTests, Client_Sends_NetworkHierarchy_Leave_Event_On_Child_Detached_On_Server)
    {
        MockNetworkHierarchyCallbackHandler mock;
        EXPECT_CALL(mock, OnNetworkHierarchyLeave);

        m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->BindNetworkHierarchyLeaveEventHandler(mock.m_leaveHandler);

        // simulate server detaching a child entity
        SetParentIdOnNetworkTransform(m_child->m_entity, InvalidNetEntityId);
        SetHierarchyRootFieldOnNetworkHierarchyChildOnClient(m_child->m_entity, InvalidNetEntityId);
    }

    TEST_F(ClientSimpleHierarchyTests, ChildHasOwningConnectionIdOfParent)
    {
        // disconnect and assign new connection ids
        SetParentIdOnNetworkTransform(m_child->m_entity, InvalidNetEntityId);
        SetHierarchyRootFieldOnNetworkHierarchyChildOnClient(m_child->m_entity, InvalidNetEntityId);

        m_root->m_entity->FindComponent<NetBindComponent>()->SetOwningConnectionId(ConnectionId{ 1 });
        m_child->m_entity->FindComponent<NetBindComponent>()->SetOwningConnectionId(ConnectionId{ 2 });

        const ConnectionId previousConnectionId = m_child->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId();

        // re-attach, child's owning connection id should then be root's connection id
        SetParentIdOnNetworkTransform(m_child->m_entity, RootNetEntityId);
        SetHierarchyRootFieldOnNetworkHierarchyChildOnClient(m_child->m_entity, RootNetEntityId);

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId(),
            m_root->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId()
        );

        // detach, the child should roll back to his previous owning connection id
        SetParentIdOnNetworkTransform(m_child->m_entity, InvalidNetEntityId);
        SetHierarchyRootFieldOnNetworkHierarchyChildOnClient(m_child->m_entity, InvalidNetEntityId);

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId(),
            previousConnectionId
        );
    }

    /*
     * Parent -> Child -> ChildOfChild
     */
    class ClientDeepHierarchyTests : public ClientSimpleHierarchyTests
    {
    public:
        const NetEntityId ChildOfChildNetEntityId = NetEntityId{ 3 };

        void SetUp() override
        {
            ClientSimpleHierarchyTests::SetUp();

            m_childOfChild = AZStd::make_unique<EntityInfo>((3), "child of child", ChildOfChildNetEntityId, EntityInfo::Role::Child);

            CreateDeepHierarchyOnClient(*m_childOfChild);

            m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child->m_entity->GetId());
        }

        void TearDown() override
        {
            m_childOfChild.reset();

            ClientSimpleHierarchyTests::TearDown();
        }

        void CreateDeepHierarchyOnClient(EntityInfo& childOfChild)
        {
            PopulateHierarchicalEntity(childOfChild);
            SetupEntity(childOfChild.m_entity, childOfChild.m_netId, NetEntityRole::Autonomous);

            // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
            SetParentIdOnNetworkTransform(childOfChild.m_entity, m_childOfChild->m_netId);
            SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyChildComponent>(childOfChild.m_entity, m_root->m_netId);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childOfChildHandle(childOfChild.m_entity.get(), m_networkEntityTracker.get());
            childOfChild.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childOfChildHandle);
            childOfChild.m_replicator->Initialize(childOfChildHandle);

            childOfChild.m_entity->Activate();
        }

        AZStd::unique_ptr<EntityInfo> m_childOfChild;
    };

    TEST_F(ClientDeepHierarchyTests, Client_Activates_Hierarchy_From_Network_Fields)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
        EXPECT_EQ(
            m_childOfChild->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            m_root->m_entity.get()
        );

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
        if (m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 3)
        {
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[0],
                m_root->m_entity.get()
            );
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[1],
                m_child->m_entity.get()
            );
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[2],
                m_childOfChild->m_entity.get()
            );
        }
    }

    TEST_F(ClientDeepHierarchyTests, CreateProcessInputTest)
    {
        using MultiplayerTest::TestMultiplayerComponent;
        using MultiplayerTest::TestMultiplayerComponentController;
        using MultiplayerTest::TestMultiplayerComponentNetworkInput;

        auto* rootNetBind = m_root->m_entity->FindComponent<NetBindComponent>();

        NetworkInputArray inputArray(rootNetBind->GetEntityHandle());
        NetworkInput& input = inputArray[0];

        const float deltaTime = 0.16f;
        rootNetBind->CreateInput(input, deltaTime);

        auto ValidateCreatedInput = [](const NetworkInput& input, const HierarchyTests::EntityInfo& entityInfo)
        {
            // Validate test input for the root entity's TestMultiplayerComponent
            auto* testInput = input.FindComponentInput<TestMultiplayerComponentNetworkInput>();
            EXPECT_NE(testInput, nullptr);

            auto* testMultiplayerComponent = entityInfo.m_entity->FindComponent<TestMultiplayerComponent>();
            EXPECT_NE(testMultiplayerComponent, nullptr);

            EXPECT_EQ(testInput->m_ownerId, testMultiplayerComponent->GetId());
        };

        // Validate root input
        ValidateCreatedInput(input, *m_root);

        // Validate children input
        {
            NetworkHierarchyRootComponentNetworkInput* rootHierarchyInput = input.FindComponentInput<NetworkHierarchyRootComponentNetworkInput>();
            const AZStd::vector<NetworkInputChild>& childInputs = rootHierarchyInput->m_childInputs;
            EXPECT_EQ(childInputs.size(), 2);
            ValidateCreatedInput(childInputs[0].GetNetworkInput(), *m_child);
            ValidateCreatedInput(childInputs[1].GetNetworkInput(), *m_childOfChild);
        }

        // Test ProcessInput
        {
            AZStd::unordered_set<NetEntityId> inputProcessedEntities;
            size_t processInputCallCounter = 0;
            auto processInputCallback = [&inputProcessedEntities, &processInputCallCounter](NetEntityId netEntityId)
            {
                inputProcessedEntities.insert(netEntityId);
                processInputCallCounter++;
            };

            // Set the callbacks for processing input. This allows us to inspect how many times the input was processed
            // and which entity's controller was invoked.
            m_root->m_entity->FindComponent<TestMultiplayerComponent>()->m_processInputCallback = processInputCallback;
            m_child->m_entity->FindComponent<TestMultiplayerComponent>()->m_processInputCallback = processInputCallback;
            m_childOfChild->m_entity->FindComponent<TestMultiplayerComponent>()->m_processInputCallback = processInputCallback;

            rootNetBind->ProcessInput(input, deltaTime);

            EXPECT_EQ(processInputCallCounter, 3);
            EXPECT_EQ(inputProcessedEntities,
                AZStd::unordered_set<NetEntityId>({ m_root->m_netId, m_child->m_netId, m_childOfChild->m_netId }));
        }
    }
}
