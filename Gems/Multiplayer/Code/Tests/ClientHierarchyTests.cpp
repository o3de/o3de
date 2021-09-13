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
#include <NetworkEntity/EntityReplication/EntityReplicator.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    /*
     * Test NetBindComponent activation. This must work before more complicated tests.
     */
    TEST_F(HierarchyTests, On_Client_NetBindComponent_Activate)
    {
        AZ::Entity entity;
        entity.CreateComponent<NetBindComponent>();
        SetupEntity(entity, NetEntityId{ 1 }, NetEntityRole::Client);
        entity.Activate();

        StopEntity(entity);

        entity.Deactivate();
    }

    /*
     * Hierarchy test - a child entity on a client delaying activation until its hierarchical parent has been activated
     */
    TEST_F(HierarchyTests, On_Client_EntityReplicator_DontActivate_BeforeParent)
    {
        // Create a child entity that will be tested for activation inside a hierarchy
        AZ::Entity childEntity;
        CreateEntityWithChildHierarchy(childEntity);
        SetupEntity(childEntity, NetEntityId{ 2 }, NetEntityRole::Client);
        // child entity is not activated on purpose here, we are about to test conditional activation check

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(childEntity, NetEntityId{ 1 });
        SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyChildComponent>(childEntity, NetEntityId{ 1 });

        // Create an entity replicator for the child entity
        const NetworkEntityHandle childHandle(&childEntity, m_networkEntityTracker.get());
        EntityReplicator entityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
        entityReplicator.Initialize(childHandle);

        // Entity replicator should not be ready to activate the entity because its parent does not exist
        EXPECT_EQ(entityReplicator.IsReadyToActivate(), false);
    }

    TEST_F(HierarchyTests, On_Client_EntityReplicator_DontActivate_Inner_Root_Before_Top_Root)
    {
        // Create a child entity that will be tested for activation inside a hierarchy
        AZ::Entity innerRootEntity;
        CreateEntityWithRootHierarchy(innerRootEntity);
        SetupEntity(innerRootEntity, NetEntityId{ 2 }, NetEntityRole::Client);
        // child entity is not activated on purpose here, we are about to test conditional activation check

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(innerRootEntity, NetEntityId{ 1 });
        SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyRootComponent>(innerRootEntity, NetEntityId{ 1 });

        // Create an entity replicator for the child entity
        const NetworkEntityHandle innerRootHandle(&innerRootEntity, m_networkEntityTracker.get());
        EntityReplicator entityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, innerRootHandle);
        entityReplicator.Initialize(innerRootHandle);

        // Entity replicator should not be ready to activate the entity because its parent does not exist
        EXPECT_EQ(entityReplicator.IsReadyToActivate(), false);
    }

    TEST_F(HierarchyTests, On_Client_Not_In_Hierarchy_EntityReplicator_Ignores_Parent)
    {
        // Create a child entity that will be tested for activation inside a hierarchy
        AZ::Entity childEntity;
        CreateEntityWithChildHierarchy(childEntity);
        SetupEntity(childEntity, NetEntityId{ 2 }, NetEntityRole::Client);
        // child entity is not activated on purpose here, we are about to test conditional activation check

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(childEntity, NetEntityId{ 1 });
        SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyChildComponent>(childEntity, InvalidNetEntityId);

        // Create an entity replicator for the child entity
        const NetworkEntityHandle childHandle(&childEntity, m_networkEntityTracker.get());
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
        AZ::Entity childEntity;
        CreateEntityWithChildHierarchy(childEntity);
        SetupEntity(childEntity, NetEntityId{ 2 }, NetEntityRole::Client);

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(childEntity, NetEntityId{ 1 });
        SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyChildComponent>(childEntity, NetEntityId{ 1 });

        // Create an entity replicator for the child entity
        const NetworkEntityHandle childHandle(&childEntity, m_networkEntityTracker.get());
        EntityReplicator childEntityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
        childEntityReplicator.Initialize(childHandle);

        // Now let's create a parent entity and activate it
        AZ::Entity parentEntity;
        CreateEntityWithRootHierarchy(parentEntity);
        SetupEntity(parentEntity, NetEntityId{ 1 }, NetEntityRole::Client);

        // Create an entity replicator for the parent entity
        const NetworkEntityHandle parentHandle(&parentEntity, m_networkEntityTracker.get());
        ON_CALL(*m_mockNetworkEntityManager, GetEntity(_)).WillByDefault(Return(parentHandle));
        EntityReplicator parentEntityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, parentHandle);
        parentEntityReplicator.Initialize(parentHandle);

        parentEntity.Activate();

        // The child should be ready to be activated
        EXPECT_EQ(childEntityReplicator.IsReadyToActivate(), true);

        StopEntity(parentEntity);

        parentEntity.Deactivate();
    }

    /*
     * Parent -> Child
     */
    class ClientSimpleHierarchyTests : public HierarchyTests
    {
    public:
        static const NetEntityId RootNetEntityId = NetEntityId{ 1 };
        static const NetEntityId ChildNetEntityId = NetEntityId{ 2 };

        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_rootEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(1), "root");
            m_childEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(2), "child");

            m_rootEntityInfo = AZStd::make_unique<EntityInfo>(*m_rootEntity.get(), RootNetEntityId, EntityInfo::Role::Root);
            m_childEntityInfo = AZStd::make_unique<EntityInfo>(*m_childEntity.get(), ChildNetEntityId, EntityInfo::Role::Child);

            CreateSimpleHierarchy(*m_rootEntityInfo, *m_childEntityInfo);

            m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());
            // now the two entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childEntityInfo.reset();
            m_rootEntityInfo.reset();

            StopAndDeleteEntity(m_childEntity);
            StopAndDeleteEntity(m_rootEntity);

            HierarchyTests::TearDown();
        }

        void CreateSimpleHierarchy(EntityInfo& root, EntityInfo& child)
        {
            PopulateHierarchicalEntity(root);
            SetupEntity(root.m_entity, root.m_netId, NetEntityRole::Client);

            PopulateHierarchicalEntity(child);
            SetupEntity(child.m_entity, child.m_netId, NetEntityRole::Client);

            // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
            SetParentIdOnNetworkTransform(child.m_entity, root.m_netId);
            SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyChildComponent>(child.m_entity, root.m_netId);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childHandle(&child.m_entity, m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
            child.m_replicator->Initialize(childHandle);

            // Create an entity replicator for the root entity
            const NetworkEntityHandle rootHandle(&root.m_entity, m_networkEntityTracker.get());
            root.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, rootHandle);
            root.m_replicator->Initialize(rootHandle);

            root.m_entity.Activate();
            child.m_entity.Activate();
        }

        void SetHierarchyRootFieldOnNetworkHierarchyChildOnClient(const AZ::Entity& entity, NetEntityId value)
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
            inSerializer.Serialize(reinterpret_cast<uint32_t&>(value),
                "hierarchyRoot", /* Derived from NetworkHierarchyChildComponent.AutoComponent.xml */
                AZStd::numeric_limits<uint32_t>::min(), AZStd::numeric_limits<uint32_t>::max());

            NetworkOutputSerializer outSerializer(buffer.begin(), bufferSize);

            ReplicationRecord notifyRecord = currentRecord;

            entity.FindComponent<NetworkHierarchyChildComponent>()->SerializeStateDeltaMessage(currentRecord, outSerializer);

            entity.FindComponent<NetworkHierarchyChildComponent>()->NotifyStateDeltaChanges(notifyRecord);
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity;
        AZStd::unique_ptr<AZ::Entity> m_childEntity;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo;
    };

    TEST_F(ClientSimpleHierarchyTests, Client_Activates_Hierarchy_From_Network_Fields)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            m_rootEntity.get()
        );

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
        if (m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 2)
        {
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[0],
                m_rootEntity.get()
            );
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[1],
                m_childEntity.get()
            );
        }
    }

    TEST_F(ClientSimpleHierarchyTests, Client_Detaches_Child_When_Server_Detaches)
    {
        // simulate server detaching child entity
        SetParentIdOnNetworkTransform(*m_childEntity, InvalidNetEntityId);
        SetHierarchyRootFieldOnNetworkHierarchyChildOnClient(*m_childEntity, InvalidNetEntityId);

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            nullptr
        );
    }

    /*
     * Parent -> Child -> ChildOfChild
     */
    class ClientDeepHierarchyTests : public ClientSimpleHierarchyTests
    {
    public:
        static const NetEntityId ChildOfChildNetEntityId = NetEntityId{ 3 };

        void SetUp() override
        {
            ClientSimpleHierarchyTests::SetUp();

            m_childOfChildEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(3), "child of child");
            m_childOfChildEntityInfo = AZStd::make_unique<EntityInfo>(*m_childOfChildEntity.get(), ChildOfChildNetEntityId, EntityInfo::Role::Child);

            CreateDeepHierarchyOnClient(*m_childOfChildEntityInfo);

            m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());
        }

        void TearDown() override
        {
            m_childOfChildEntityInfo.reset();
            StopAndDeleteEntity(m_childOfChildEntity);

            ClientSimpleHierarchyTests::TearDown();
        }

        void CreateDeepHierarchyOnClient(EntityInfo& childOfChild)
        {
            PopulateHierarchicalEntity(childOfChild);
            SetupEntity(childOfChild.m_entity, childOfChild.m_netId, NetEntityRole::Client);

            // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
            SetParentIdOnNetworkTransform(childOfChild.m_entity, m_childEntityInfo->m_netId);
            SetHierarchyRootFieldOnNetworkHierarchyChild<NetworkHierarchyChildComponent>(childOfChild.m_entity, m_rootEntityInfo->m_netId);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childOfChildHandle(&childOfChild.m_entity, m_networkEntityTracker.get());
            childOfChild.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childOfChildHandle);
            childOfChild.m_replicator->Initialize(childOfChildHandle);

            childOfChild.m_entity.Activate();
        }

        AZStd::unique_ptr<AZ::Entity> m_childOfChildEntity;
        AZStd::unique_ptr<EntityInfo> m_childOfChildEntityInfo;
    };

    TEST_F(ClientDeepHierarchyTests, Client_Activates_Hierarchy_From_Network_Fields)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            m_rootEntity.get()
        );

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
        if (m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 3)
        {
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[0],
                m_rootEntity.get()
            );
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[1],
                m_childEntity.get()
            );
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[2],
                m_childOfChildEntity.get()
            );
        }
    }
}
