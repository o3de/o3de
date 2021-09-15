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
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>
#include <NetworkEntity/EntityReplication/EntityReplicator.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    /*
     * Parent -> Child
     */
    class ServerSimpleHierarchyTests : public HierarchyTests
    {
    public:
        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_rootEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(1), "root");
            m_childEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(2), "child");

            m_rootEntityInfo = AZStd::make_unique<EntityInfo>(*m_rootEntity.get(), NetEntityId{ 1 }, EntityInfo::Role::Root);
            m_childEntityInfo = AZStd::make_unique<EntityInfo>(*m_childEntity.get(), NetEntityId{ 2 }, EntityInfo::Role::Child);

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
            SetupEntity(root.m_entity, root.m_netId, NetEntityRole::Authority);

            PopulateHierarchicalEntity(child);
            SetupEntity(child.m_entity, child.m_netId, NetEntityRole::Authority);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childHandle(&child.m_entity, m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, childHandle);
            child.m_replicator->Initialize(childHandle);

            // Create an entity replicator for the root entity
            const NetworkEntityHandle rootHandle(&root.m_entity, m_networkEntityTracker.get());
            root.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, rootHandle);
            root.m_replicator->Initialize(rootHandle);

            root.m_entity.Activate();
            child.m_entity.Activate();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity;
        AZStd::unique_ptr<AZ::Entity> m_childEntity;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo;
    };

    TEST_F(ServerSimpleHierarchyTests, Server_Sets_Appropriate_Network_Fields_For_Clients)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            NetEntityId{ 1 }
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Root_Is_Top_Level_Root)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            false
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Child_Has_Root_Set)
    {
        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            NetEntityId{ 1 }
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Child_Has_Root_Cleared_On_Detach)
    {
        // now detach the child
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Root_Has_Child_Reference)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Root_Has_Child_References_Removed_On_Detach)
    {
        // now detach the child
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            1
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Root_Deactivates_Child_Has_No_References_To_Root)
    {
        StopEntity(*m_rootEntity);
        m_rootEntity->Deactivate();
        m_rootEntity.reset();

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Child_Deactivates_Root_Has_No_References_To_Child)
    {
        StopEntity(*m_childEntity);
        m_childEntity->Deactivate();
        m_childEntity.reset();

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            1
        );
    }

    /*
     * Parent -> Child -> ChildOfChild
     */
    class ServerDeepHierarchyTests : public HierarchyTests
    {
    public:
        const NetEntityId RootNetEntityId = NetEntityId{ 1 };
        const NetEntityId ChildNetEntityId = NetEntityId{ 2 };
        const NetEntityId ChildOfChildNetEntityId = NetEntityId{ 3 };

        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_rootEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(1), "root");
            m_childEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(2), "child");
            m_childOfChildEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(3), "child of child");

            m_rootEntityInfo = AZStd::make_unique<EntityInfo>(*m_rootEntity.get(), RootNetEntityId, EntityInfo::Role::Root);
            m_childEntityInfo = AZStd::make_unique<EntityInfo>(*m_childEntity.get(), ChildNetEntityId, EntityInfo::Role::Child);
            m_childOfChildEntityInfo = AZStd::make_unique<EntityInfo>(*m_childOfChildEntity.get(), ChildOfChildNetEntityId, EntityInfo::Role::Child);

            CreateDeepHierarchy(*m_rootEntityInfo, *m_childEntityInfo, *m_childOfChildEntityInfo);

            m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());
            m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChildEntityInfo.reset();
            m_childEntityInfo.reset();
            m_rootEntityInfo.reset();

            StopAndDeleteEntity(m_childOfChildEntity);
            StopAndDeleteEntity(m_childEntity);
            StopAndDeleteEntity(m_rootEntity);

            HierarchyTests::TearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity;
        AZStd::unique_ptr<AZ::Entity> m_childEntity;
        AZStd::unique_ptr<AZ::Entity> m_childOfChildEntity;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childOfChildEntityInfo;
    };

    TEST_F(ServerDeepHierarchyTests, Root_Is_Top_Level_Root)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            false
        );
    }

    TEST_F(ServerDeepHierarchyTests, Root_Has_Child_References)
    {
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

    TEST_F(ServerDeepHierarchyTests, Root_Has_Child_Of_Child_Reference_Removed_On_Detach)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    TEST_F(ServerDeepHierarchyTests, Root_Has_All_References_Removed_On_Detach_Of_Mid_Child)
    {
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            1
        );
    }

    TEST_F(ServerDeepHierarchyTests, Root_Has_All_References_If_Mid_Child_Added_With_Child)
    {
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerDeepHierarchyTests, Root_Has_All_References_If_Child_Of_Child_Added)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerDeepHierarchyTests, Child_Of_Child_Points_To_Root_After_Attach)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());

        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
    }

    TEST_F(ServerDeepHierarchyTests, All_New_Children_Point_To_Root_If_Mid_Child_Added_With_Child)
    {
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
    }

    TEST_F(ServerDeepHierarchyTests, Children_Clear_Reference_To_Root_After_Mid_Child_Detached)
    {
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
    }

    TEST_F(ServerDeepHierarchyTests, Child_Of_Child_Clears_Reference_To_Root_After_Detached)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
    }

    TEST_F(ServerDeepHierarchyTests, Root_Deactivates_Children_Have_No_References_To_Root)
    {
        StopAndDeleteEntity(m_rootEntity);

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );

        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
    }

    TEST_F(ServerDeepHierarchyTests, Child_Of_Child_Deactivates_Root_Removes_References_To_It)
    {
        StopAndDeleteEntity(m_childOfChildEntity);

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    TEST_F(ServerDeepHierarchyTests, Testing_Limiting_Hierarchy_Maximum_Size)
    {
        uint32_t currentMaxLimit = 0;
        m_console->GetCvarValue<uint32_t>("bg_hierarchyEntityMaxLimit", currentMaxLimit);
        m_console->PerformCommand("bg_hierarchyEntityMaxLimit 2");

        // remake the hierarchy
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );

        m_console->PerformCommand((AZStd::string("bg_hierarchyEntityMaxLimit ") + AZStd::to_string(currentMaxLimit)).c_str());
        m_console->GetCvarValue<uint32_t>("bg_hierarchyEntityMaxLimit", currentMaxLimit);
    }

    /*
     * Parent -> Child  -> Child Of Child
     *        -> Child2 -> Child Of Child2
     *                  -> Child2 Of Child2
     */
    class ServerBranchedHierarchyTests : public HierarchyTests
    {
    public:
        static const NetEntityId RootNetEntityId = NetEntityId{ 1 };
        static const NetEntityId ChildNetEntityId = NetEntityId{ 2 };
        static const NetEntityId ChildOfChildNetEntityId = NetEntityId{ 3 };
        static const NetEntityId Child2NetEntityId = NetEntityId{ 4 };
        static const NetEntityId ChildOfChild2NetEntityId = NetEntityId{ 5 };
        static const NetEntityId Child2OfChild2NetEntityId = NetEntityId{ 6 };

        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_rootEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(1), "root");
            m_childEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(2), "child");
            m_childOfChildEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(3), "child of child");
            m_child2Entity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(4), "child2");
            m_childOfChild2Entity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(5), "child of child2");
            m_child2OfChild2Entity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(6), "child2 of child2");

            m_rootEntityInfo = AZStd::make_unique<EntityInfo>(*m_rootEntity.get(), RootNetEntityId, EntityInfo::Role::Root);
            m_childEntityInfo = AZStd::make_unique<EntityInfo>(*m_childEntity.get(), ChildNetEntityId, EntityInfo::Role::Child);
            m_childOfChildEntityInfo = AZStd::make_unique<EntityInfo>(*m_childOfChildEntity.get(), ChildOfChildNetEntityId, EntityInfo::Role::Child);
            m_child2EntityInfo = AZStd::make_unique<EntityInfo>(*m_child2Entity.get(), Child2NetEntityId, EntityInfo::Role::Child);
            m_childOfChild2EntityInfo = AZStd::make_unique<EntityInfo>(*m_childOfChild2Entity.get(), ChildOfChild2NetEntityId, EntityInfo::Role::Child);
            m_child2OfChild2EntityInfo = AZStd::make_unique<EntityInfo>(*m_child2OfChild2Entity.get(), Child2OfChild2NetEntityId, EntityInfo::Role::Child);

            CreateBranchedHierarchy(*m_rootEntityInfo, *m_childEntityInfo, *m_childOfChildEntityInfo,
                *m_child2EntityInfo, *m_childOfChild2EntityInfo, *m_child2OfChild2EntityInfo);

            m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());
            m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_child2OfChild2EntityInfo.reset();
            m_childOfChild2EntityInfo.reset();
            m_child2EntityInfo.reset();
            m_childOfChildEntityInfo.reset();
            m_childEntityInfo.reset();
            m_rootEntityInfo.reset();

            StopAndDeleteEntity(m_child2OfChild2Entity);
            StopAndDeleteEntity(m_childOfChild2Entity);
            StopAndDeleteEntity(m_child2Entity);
            StopAndDeleteEntity(m_childOfChildEntity);
            StopAndDeleteEntity(m_childEntity);
            StopAndDeleteEntity(m_rootEntity);

            HierarchyTests::TearDown();
        }


        void CreateBranchedHierarchy(EntityInfo& root, EntityInfo& child, EntityInfo& childOfChild,
            EntityInfo& child2, EntityInfo& childOfChild2, EntityInfo& child2OfChild2)
        {
            PopulateHierarchicalEntity(root);
            PopulateHierarchicalEntity(child);
            PopulateHierarchicalEntity(childOfChild);
            PopulateHierarchicalEntity(child2);
            PopulateHierarchicalEntity(childOfChild2);
            PopulateHierarchicalEntity(child2OfChild2);

            SetupEntity(root.m_entity, root.m_netId, NetEntityRole::Authority);
            SetupEntity(child.m_entity, child.m_netId, NetEntityRole::Authority);
            SetupEntity(childOfChild.m_entity, childOfChild.m_netId, NetEntityRole::Authority);
            SetupEntity(child2.m_entity, child2.m_netId, NetEntityRole::Authority);
            SetupEntity(childOfChild2.m_entity, childOfChild2.m_netId, NetEntityRole::Authority);
            SetupEntity(child2OfChild2.m_entity, child2OfChild2.m_netId, NetEntityRole::Authority);

            // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
            SetParentIdOnNetworkTransform(child.m_entity, root.m_netId);
            SetParentIdOnNetworkTransform(childOfChild.m_entity, child.m_netId);
            SetParentIdOnNetworkTransform(child2.m_entity, root.m_netId);
            SetParentIdOnNetworkTransform(childOfChild2.m_entity, child2.m_netId);
            SetParentIdOnNetworkTransform(child2OfChild2.m_entity, child2.m_netId);

            // Create entity replicators
            const NetworkEntityHandle childOfChild2Handle(&childOfChild2.m_entity, m_networkEntityTracker.get());
            childOfChild.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, childOfChild2Handle);
            childOfChild.m_replicator->Initialize(childOfChild2Handle);

            const NetworkEntityHandle child2OfChild2Handle(&child2OfChild2.m_entity, m_networkEntityTracker.get());
            childOfChild.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, child2OfChild2Handle);
            childOfChild.m_replicator->Initialize(child2OfChild2Handle);

            const NetworkEntityHandle child2Handle(&child2.m_entity, m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, child2Handle);
            child.m_replicator->Initialize(child2Handle);

            const NetworkEntityHandle childOfChildHandle(&childOfChild.m_entity, m_networkEntityTracker.get());
            childOfChild.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, childOfChildHandle);
            childOfChild.m_replicator->Initialize(childOfChildHandle);

            const NetworkEntityHandle childHandle(&child.m_entity, m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, childHandle);
            child.m_replicator->Initialize(childHandle);

            const NetworkEntityHandle rootHandle(&root.m_entity, m_networkEntityTracker.get());
            root.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, rootHandle);
            root.m_replicator->Initialize(rootHandle);

            root.m_entity.Activate();
            child.m_entity.Activate();
            childOfChild.m_entity.Activate();
            child2.m_entity.Activate();
            childOfChild2.m_entity.Activate();
            child2OfChild2.m_entity.Activate();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity;
        AZStd::unique_ptr<AZ::Entity> m_childEntity;
        AZStd::unique_ptr<AZ::Entity> m_childOfChildEntity;
        AZStd::unique_ptr<AZ::Entity> m_child2Entity;
        AZStd::unique_ptr<AZ::Entity> m_childOfChild2Entity;
        AZStd::unique_ptr<AZ::Entity> m_child2OfChild2Entity;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childOfChildEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_child2EntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childOfChild2EntityInfo;
        AZStd::unique_ptr<EntityInfo> m_child2OfChild2EntityInfo;
    };

    TEST_F(ServerBranchedHierarchyTests, Sanity_Check)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );

        if (m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 6)
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
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[3],
                m_child2Entity.get()
            );
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[4],
                m_child2OfChild2Entity.get()
            );
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[5],
                m_childOfChild2Entity.get()
            );
        }
    }

    TEST_F(ServerBranchedHierarchyTests, Detach_Child_While_Child2_Remains_Attached)
    {
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            4
        );

        if (m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 4)
        {
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[0],
                m_rootEntity.get()
            );
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[1],
                m_child2Entity.get()
            );
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[2],
                m_child2OfChild2Entity.get()
            );
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[3],
                m_childOfChild2Entity.get()
            );
        }

        EXPECT_EQ(
            m_child2Entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            m_rootEntity.get()
        );
        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            nullptr
        );
        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            nullptr
        );
    }

    TEST_F(ServerBranchedHierarchyTests, Detach_Child_Then_Attach_To_Child2)
    {
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child2Entity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    /*
     * Sets up 2 deep hierarchies.
     */
    class ServerHierarchyOfHierarchyTests : public ServerDeepHierarchyTests
    {
    public:
        static const NetEntityId Root2NetEntityId = NetEntityId{ 4 };
        static const NetEntityId Child2NetEntityId = NetEntityId{ 5 };
        static const NetEntityId ChildOfChild2NetEntityId = NetEntityId{ 6 };

        void SetUp() override
        {
            ServerDeepHierarchyTests::SetUp();

            m_rootEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(4), "root 2");
            m_childEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(5), "child 2");
            m_childOfChildEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(6), "child of child 2");

            m_rootEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_rootEntity2.get(), Root2NetEntityId, EntityInfo::Role::Root);
            m_childEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_childEntity2.get(), Child2NetEntityId, EntityInfo::Role::Child);
            m_childOfChildEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_childOfChildEntity2.get(), ChildOfChild2NetEntityId, EntityInfo::Role::Child);

            CreateDeepHierarchy(*m_rootEntityInfo2, *m_childEntityInfo2, *m_childOfChildEntityInfo2);

            m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity2->GetId());
            m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity2->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChildEntityInfo2.reset();
            m_childEntityInfo2.reset();
            m_rootEntityInfo2.reset();

            StopAndDeleteEntity(m_childOfChildEntity2);
            StopAndDeleteEntity(m_childEntity2);
            StopAndDeleteEntity(m_rootEntity2);

            ServerDeepHierarchyTests::TearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity2;
        AZStd::unique_ptr<AZ::Entity> m_childEntity2;
        AZStd::unique_ptr<AZ::Entity> m_childOfChildEntity2;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo2;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo2;
        AZStd::unique_ptr<EntityInfo> m_childOfChildEntityInfo2;
    };

    TEST_F(ServerHierarchyOfHierarchyTests, Hierarchies_Are_Not_Related)
    {
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

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );

        if (m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 3)
        {
            EXPECT_EQ(
                m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[0],
                m_rootEntity2.get()
            );
            EXPECT_EQ(
                m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[1],
                m_childEntity2.get()
            );
            EXPECT_EQ(
                m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[2],
                m_childOfChildEntity2.get()
            );
        }
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Is_Not_Top_Level_Root)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            false
        );
        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            true
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_References_All_When_Another_Hierarchy_Attached_At_Root)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_References_All_When_Another_Hierarchy_Attached_At_Child)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_References_All_When_Another_Hierarchy_Attached_At_Child_Of_Child)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_References_Top_Root_When_Another_Hierarchy_Attached_At_Root)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            true
        );
        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_References_Top_Root_When_Another_Hierarchy_Attached_At_Child)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            true
        );
        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_References_Top_Root_When_Another_Hierarchy_Attached_At_Child_Of_Child)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            true
        );
        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Doesnt_Keep_Child_References)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            0
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Detachment_From_Top_Root)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());
        // detach
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
        if (m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 3)
        {
            EXPECT_EQ(
                m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[0],
                m_rootEntity2.get()
            );
            EXPECT_EQ(
                m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[1],
                m_childEntity2.get()
            );
            EXPECT_EQ(
                m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[2],
                m_childOfChildEntity2.get()
            );
        }
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Detachment_From_Child_Of_Child)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Top_Root_Deactivates)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());

        StopAndDeleteEntity(m_rootEntity);

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Child_Of_Top_Root_Deactivates)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());

        StopAndDeleteEntity(m_childEntity);

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Child_Of_Child_Deactivates)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        StopAndDeleteEntity(m_childOfChildEntity);

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Stress_Test_Inner_Root_Has_Child_References_After_Detachment_From_Child_Of_Child)
    {
        for (int i = 0; i < 100; ++i)
        {
            m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
            // detach
            m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        }

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Detachment_Of_Child_Of_Child_In_Inner_Hierarchy)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            5
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Attachment_Of_Child_Of_Child_In_Inner_Hierarchy)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // re-connect
        m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity2->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Child_Of_Child_Changed_Hierarchies)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        // connect to a different hierarchy
        m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Detachment_Of_Child_In_Inner_Hierarchy)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            4
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Child_Changed_Hierarchies)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        // connect to a different hierarchy
        m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_No_Child_References_After_All_Children_Moved_To_Another_Hierarchy)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());

        m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        // detach
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            1
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Child_Deactivated_Top_Root_Has_No_Child_Reference_To_It)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());

        StopAndDeleteEntity(m_childEntity2);

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            4
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Testing_Limiting_Hierarchy_Maximum_Size)
    {
        uint32_t currentMaxLimit = 0;
        m_console->GetCvarValue<uint32_t>("bg_hierarchyEntityMaxLimit", currentMaxLimit);
        m_console->PerformCommand("bg_hierarchyEntityMaxLimit 2");

        // remake the top level hierarchy
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );

        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );

        m_console->PerformCommand((AZStd::string("bg_hierarchyEntityMaxLimit ") + AZStd::to_string(currentMaxLimit)).c_str());
        m_console->GetCvarValue<uint32_t>("bg_hierarchyEntityMaxLimit", currentMaxLimit);
    }

    /*
     * Parent -> Child -> ChildOfChild (not marked as in a hierarchy)
     */
    class ServerMixedDeepHierarchyTests : public HierarchyTests
    {
    public:
        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_rootEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(1), "root");
            m_childEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(2), "child");
            m_childOfChildEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(3), "child of child");

            m_rootEntityInfo = AZStd::make_unique<EntityInfo>(*m_rootEntity.get(), NetEntityId{ 1 }, EntityInfo::Role::Root);
            m_childEntityInfo = AZStd::make_unique<EntityInfo>(*m_childEntity.get(), NetEntityId{ 2 }, EntityInfo::Role::Child);
            m_childOfChildEntityInfo = AZStd::make_unique<EntityInfo>(*m_childOfChildEntity.get(), NetEntityId{ 3 }, EntityInfo::Role::None);

            CreateDeepHierarchy(*m_rootEntityInfo, *m_childEntityInfo, *m_childOfChildEntityInfo);

            m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());
            m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChildEntityInfo.reset();
            m_childEntityInfo.reset();
            m_rootEntityInfo.reset();

            StopAndDeleteEntity(m_childOfChildEntity);
            StopAndDeleteEntity(m_childEntity);
            StopAndDeleteEntity(m_rootEntity);

            HierarchyTests::TearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity;
        AZStd::unique_ptr<AZ::Entity> m_childEntity;
        AZStd::unique_ptr<AZ::Entity> m_childOfChildEntity;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childOfChildEntityInfo;
    };

    TEST_F(ServerMixedDeepHierarchyTests, Top_Root_Ignores_Non_Hierarchical_Entities)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    TEST_F(ServerMixedDeepHierarchyTests, Detaching_Non_Hierarchical_Entity_Has_No_Effect_On_Top_Root)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    TEST_F(ServerMixedDeepHierarchyTests, Attaching_Non_Hierarchical_Entity_Has_No_Effect_On_Top_Root)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    /*
     * 1st hierarchy: Parent -> Child -> ChildOfChild (not marked as in a hierarchy)
     * 2nd hierarchy: Parent2 -> Child2 (not marked as in a hierarchy) -> ChildOfChild2
     */
    class ServerMixedHierarchyOfHierarchyTests : public ServerMixedDeepHierarchyTests
    {
    public:
        void SetUp() override
        {
            ServerMixedDeepHierarchyTests::SetUp();

            m_rootEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(4), "root 2");
            m_childEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(5), "child 2");
            m_childOfChildEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(6), "child of child 2");

            m_rootEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_rootEntity2.get(), NetEntityId{ 4 }, EntityInfo::Role::Root);
            m_childEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_childEntity2.get(), NetEntityId{ 5 }, EntityInfo::Role::None);
            m_childOfChildEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_childOfChildEntity2.get(), NetEntityId{ 6 }, EntityInfo::Role::Child);

            CreateDeepHierarchy(*m_rootEntityInfo2, *m_childEntityInfo2, *m_childOfChildEntityInfo2);

            m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity2->GetId());
            m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity2->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChildEntityInfo2.reset();
            m_childEntityInfo2.reset();
            m_rootEntityInfo2.reset();


            StopAndDeleteEntity(m_childOfChildEntity2);
            StopAndDeleteEntity(m_childEntity2);
            StopAndDeleteEntity(m_rootEntity2);

            ServerMixedDeepHierarchyTests::TearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity2;
        AZStd::unique_ptr<AZ::Entity> m_childEntity2;
        AZStd::unique_ptr<AZ::Entity> m_childOfChildEntity2;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo2;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo2;
        AZStd::unique_ptr<EntityInfo> m_childOfChildEntityInfo2;
    };

    TEST_F(ServerMixedHierarchyOfHierarchyTests, Sanity_Check_Ingore_Children_Without_Hierarchy_Components)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            1
        );
    }

    TEST_F(ServerMixedHierarchyOfHierarchyTests, Adding_Mixed_Hierarchy_Ingores_Children_Without_Hierarchy_Components)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerMixedHierarchyOfHierarchyTests, Attaching_Hierarchy_To_Non_Hierarchical_Entity_Does_Not_Merge_Hierarchies)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            false
        );
    }
}
