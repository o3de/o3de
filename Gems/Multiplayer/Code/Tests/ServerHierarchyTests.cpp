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
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>

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

            m_root = AZStd::make_unique<EntityInfo>(1, "root", NetEntityId{ 1 }, EntityInfo::Role::Root);
            m_child = AZStd::make_unique<EntityInfo>(2, "child", NetEntityId{ 2 }, EntityInfo::Role::Child);

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
            SetupEntity(root.m_entity, root.m_netId, NetEntityRole::Authority);

            PopulateHierarchicalEntity(child);
            SetupEntity(child.m_entity, child.m_netId, NetEntityRole::Authority);

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
        }

        AZStd::unique_ptr<EntityInfo> m_root;
        AZStd::unique_ptr<EntityInfo> m_child;
    };

    TEST_F(ServerSimpleHierarchyTests, Server_Sets_Appropriate_Network_Fields_For_Clients)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            NetEntityId{ 1 }
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Root_Is_Top_Level_Root)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            false
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Child_Has_Root_Set)
    {
        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            NetEntityId{ 1 }
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Child_Has_Root_Cleared_On_Detach)
    {
        // now detach the child
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Root_Has_Child_Reference)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Root_Has_Child_References_Removed_On_Detach)
    {
        // now detach the child
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            1
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Root_Deactivates_Child_Has_No_References_To_Root)
    {
        StopEntity(m_root->m_entity);
        m_root->m_entity->Deactivate();
        m_root->m_entity.reset();

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Child_Deactivates_Root_Has_No_References_To_Child)
    {
        m_child.reset();

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            1
        );
    }

    TEST_F(ServerSimpleHierarchyTests, Root_Deactivates_IsHierarchyEnabled_Is_False)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchyEnabled(),
            true
        );

        StopEntity(m_root->m_entity);
        m_root->m_entity->Deactivate();

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchyEnabled(),
            false
        );

        m_root->m_entity.reset();
    }

    TEST_F(ServerSimpleHierarchyTests, Child_Deactivates_IsHierarchyEnabled_Is_False)
    {
        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->IsHierarchyEnabled(),
            true
        );

        StopEntity(m_child->m_entity);
        m_child->m_entity->Deactivate();

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->IsHierarchyEnabled(),
            false
        );

        m_child->m_entity.reset();
    }

    TEST_F(ServerSimpleHierarchyTests, ChildPointsToRootAfterReattachment)
    {
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );

        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            m_root->m_entity->FindComponent<NetBindComponent>()->GetNetEntityId()
        );
    }

    TEST_F(ServerSimpleHierarchyTests, ChildHasOwningConnectionIdOfParent)
    {
        // disconnect and assign new connection ids
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_root->m_entity->FindComponent<NetBindComponent>()->SetOwningConnectionId(ConnectionId{ 1 });
        m_child->m_entity->FindComponent<NetBindComponent>()->SetOwningConnectionId(ConnectionId{ 2 });

        const ConnectionId previousConnectionId = m_child->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId();

        // re-attach, child's owning connection id should then be root's connection id
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId(),
            m_root->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId()
        );

        // detach, the child should roll back to his previous owning connection id
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId(),
            previousConnectionId
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

            m_root = AZStd::make_unique<EntityInfo>((1), "root", RootNetEntityId, EntityInfo::Role::Root);
            m_child = AZStd::make_unique<EntityInfo>((2), "child", ChildNetEntityId, EntityInfo::Role::Child);
            m_childOfChild = AZStd::make_unique<EntityInfo>((3), "child of child", ChildOfChildNetEntityId, EntityInfo::Role::Child);

            CreateDeepHierarchy(*m_root, *m_child, *m_childOfChild);

            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());
            m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child->m_entity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChild.reset();
            m_child.reset();
            m_root.reset();

            HierarchyTests::TearDown();
        }

        AZStd::unique_ptr<EntityInfo> m_root;
        AZStd::unique_ptr<EntityInfo> m_child;
        AZStd::unique_ptr<EntityInfo> m_childOfChild;
    };

    TEST_F(ServerDeepHierarchyTests, Root_Is_Top_Level_Root)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            false
        );
    }

    TEST_F(ServerDeepHierarchyTests, Root_Has_Child_References)
    {
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

    TEST_F(ServerDeepHierarchyTests, Root_Has_Child_Of_Child_Reference_Removed_On_Detach)
    {
        m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    TEST_F(ServerDeepHierarchyTests, Root_Has_All_References_Removed_On_Detach_Of_Mid_Child)
    {
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            1
        );
    }

    TEST_F(ServerDeepHierarchyTests, Root_Has_All_References_If_Mid_Child_Added_With_Child)
    {
        m_root->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_root->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerDeepHierarchyTests, Root_Has_All_References_If_Child_Of_Child_Added)
    {
        m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerDeepHierarchyTests, Child_Of_Child_Points_To_Root_After_Attach)
    {
        m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_childOfChild->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
    }

    TEST_F(ServerDeepHierarchyTests, All_New_Children_Point_To_Root_If_Mid_Child_Added_With_Child)
    {
        m_root->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_root->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
        EXPECT_EQ(
            m_childOfChild->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
    }

    TEST_F(ServerDeepHierarchyTests, Children_Clear_Reference_To_Root_After_Mid_Child_Detached)
    {
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
        EXPECT_EQ(
            m_childOfChild->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
    }

    TEST_F(ServerDeepHierarchyTests, Child_Of_Child_Clears_Reference_To_Root_After_Detached)
    {
        m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_childOfChild->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
    }

    TEST_F(ServerDeepHierarchyTests, Root_Deactivates_Children_Have_No_References_To_Root)
    {
        m_root.reset();

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );

        EXPECT_EQ(
            m_childOfChild->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            InvalidNetEntityId
        );
    }

    TEST_F(ServerDeepHierarchyTests, Child_Of_Child_Deactivates_Root_Removes_References_To_It)
    {
        m_childOfChild.reset();

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    TEST_F(ServerDeepHierarchyTests, Testing_Limiting_Hierarchy_Maximum_Size)
    {
        uint32_t currentMaxLimit = 0;
        m_console->GetCvarValue<uint32_t>("bg_hierarchyEntityMaxLimit", currentMaxLimit);
        m_console->PerformCommand("bg_hierarchyEntityMaxLimit 2");

        // remake the hierarchy
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );

        m_console->PerformCommand((AZStd::string("bg_hierarchyEntityMaxLimit ") + AZStd::to_string(currentMaxLimit)).c_str());
        m_console->GetCvarValue<uint32_t>("bg_hierarchyEntityMaxLimit", currentMaxLimit);
    }

    TEST_F(ServerDeepHierarchyTests, ReattachMiddleChildRebuildInvokedTwice)
    {
        MockNetworkHierarchyCallbackHandler mock;
        EXPECT_CALL(mock, OnNetworkHierarchyUpdated(m_root->m_entity->GetId())).Times(2);

        m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->BindNetworkHierarchyChangedEventHandler(mock.m_changedHandler);

        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());
    }

    /*
     * Parent -> Child  -> Child Of Child
     *        -> Child2 -> Child Of Child2
     *                  -> Child2 Of Child2
     */
    class ServerBranchedHierarchyTests : public HierarchyTests
    {
    public:
        const NetEntityId RootNetEntityId = NetEntityId{ 1 };
        const NetEntityId ChildNetEntityId = NetEntityId{ 2 };
        const NetEntityId ChildOfChildNetEntityId = NetEntityId{ 3 };
        const NetEntityId Child2NetEntityId = NetEntityId{ 4 };
        const NetEntityId ChildOfChild2NetEntityId = NetEntityId{ 5 };
        const NetEntityId Child2OfChild2NetEntityId = NetEntityId{ 6 };

        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_root = AZStd::make_unique<EntityInfo>((1), "root", RootNetEntityId, EntityInfo::Role::Root);
            m_child = AZStd::make_unique<EntityInfo>((2), "child", ChildNetEntityId, EntityInfo::Role::Child);
            m_childOfChild = AZStd::make_unique<EntityInfo>((3), "child of child", ChildOfChildNetEntityId, EntityInfo::Role::Child);
            m_child2 = AZStd::make_unique<EntityInfo>((4), "child2", Child2NetEntityId, EntityInfo::Role::Child);
            m_childOfChild2 = AZStd::make_unique<EntityInfo>((5), "child of child2", ChildOfChild2NetEntityId, EntityInfo::Role::Child);
            m_child2OfChild2 = AZStd::make_unique<EntityInfo>((6), "child2 of child2", Child2OfChild2NetEntityId, EntityInfo::Role::Child);

            CreateBranchedHierarchy(*m_root, *m_child, *m_childOfChild,
                *m_child2, *m_childOfChild2, *m_child2OfChild2);

            m_child2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());
            m_childOfChild2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child2->m_entity->GetId());
            m_child2OfChild2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child2->m_entity->GetId());
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());
            m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child->m_entity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_child2OfChild2.reset();
            m_childOfChild2.reset();
            m_child2.reset();
            m_childOfChild.reset();
            m_child.reset();
            m_root.reset();

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

            // Create entity replicators
            const NetworkEntityHandle childOfChild2Handle(childOfChild2.m_entity.get(), m_networkEntityTracker.get());
            childOfChild.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, childOfChild2Handle);
            childOfChild.m_replicator->Initialize(childOfChild2Handle);

            const NetworkEntityHandle child2OfChild2Handle(child2OfChild2.m_entity.get(), m_networkEntityTracker.get());
            childOfChild.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, child2OfChild2Handle);
            childOfChild.m_replicator->Initialize(child2OfChild2Handle);

            const NetworkEntityHandle child2Handle(child2.m_entity.get(), m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, child2Handle);
            child.m_replicator->Initialize(child2Handle);

            const NetworkEntityHandle childOfChildHandle(childOfChild.m_entity.get(), m_networkEntityTracker.get());
            childOfChild.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, childOfChildHandle);
            childOfChild.m_replicator->Initialize(childOfChildHandle);

            const NetworkEntityHandle childHandle(child.m_entity.get(), m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, childHandle);
            child.m_replicator->Initialize(childHandle);

            const NetworkEntityHandle rootHandle(root.m_entity.get(), m_networkEntityTracker.get());
            root.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, rootHandle);
            root.m_replicator->Initialize(rootHandle);

            root.m_entity->Activate();
            child.m_entity->Activate();
            childOfChild.m_entity->Activate();
            child2.m_entity->Activate();
            childOfChild2.m_entity->Activate();
            child2OfChild2.m_entity->Activate();
        }

        AZStd::unique_ptr<EntityInfo> m_root;
        AZStd::unique_ptr<EntityInfo> m_child;
        AZStd::unique_ptr<EntityInfo> m_childOfChild;
        AZStd::unique_ptr<EntityInfo> m_child2;
        AZStd::unique_ptr<EntityInfo> m_childOfChild2;
        AZStd::unique_ptr<EntityInfo> m_child2OfChild2;
    };

    TEST_F(ServerBranchedHierarchyTests, Sanity_Check)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );

        if (m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 6)
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
                m_child2->m_entity.get()
            );
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[3],
                m_childOfChild->m_entity.get()
            );
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[4],
                m_child2OfChild2->m_entity.get()
            );
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[5],
                m_childOfChild2->m_entity.get()
            );
        }
    }

    TEST_F(ServerBranchedHierarchyTests, Detach_Child_While_Child2_Remains_Attached)
    {
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            4
        );

        if (m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 4)
        {
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[0],
                m_root->m_entity.get()
            );
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[1],
                m_child2->m_entity.get()
            );
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[2],
                m_child2OfChild2->m_entity.get()
            );
            EXPECT_EQ(
                m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[3],
                m_childOfChild2->m_entity.get()
            );
        }

        EXPECT_EQ(
            m_child2->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            m_root->m_entity.get()
        );
        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            nullptr
        );
        EXPECT_EQ(
            m_childOfChild->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchicalRoot(),
            nullptr
        );
    }

    TEST_F(ServerBranchedHierarchyTests, Detach_Child_Then_Attach_To_Child2)
    {
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child2->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    /*
     * Sets up 2 deep hierarchies.
     */
    class ServerHierarchyOfHierarchyTests : public ServerDeepHierarchyTests
    {
    public:
        const NetEntityId Root2NetEntityId = NetEntityId{ 4 };
        const NetEntityId Child2NetEntityId = NetEntityId{ 5 };
        const NetEntityId ChildOfChild2NetEntityId = NetEntityId{ 6 };

        void SetUp() override
        {
            ServerDeepHierarchyTests::SetUp();

            m_root2 = AZStd::make_unique<EntityInfo>((4), "root 2", Root2NetEntityId, EntityInfo::Role::Root);
            m_child2 = AZStd::make_unique<EntityInfo>((5), "child 2", Child2NetEntityId, EntityInfo::Role::Child);
            m_childOfChild2 = AZStd::make_unique<EntityInfo>((6), "child of child 2", ChildOfChild2NetEntityId, EntityInfo::Role::Child);

            CreateDeepHierarchy(*m_root2, *m_child2, *m_childOfChild2);

            m_child2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root2->m_entity->GetId());
            m_childOfChild2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child2->m_entity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChild2.reset();
            m_child2.reset();
            m_root2.reset();

            ServerDeepHierarchyTests::TearDown();
        }

        AZStd::unique_ptr<EntityInfo> m_root2;
        AZStd::unique_ptr<EntityInfo> m_child2;
        AZStd::unique_ptr<EntityInfo> m_childOfChild2;
    };

    TEST_F(ServerHierarchyOfHierarchyTests, Hierarchies_Are_Not_Related)
    {
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

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );

        if (m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 3)
        {
            EXPECT_EQ(
                m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[0],
                m_root2->m_entity.get()
            );
            EXPECT_EQ(
                m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[1],
                m_child2->m_entity.get()
            );
            EXPECT_EQ(
                m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[2],
                m_childOfChild2->m_entity.get()
            );
        }
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Is_Not_Top_Level_Root)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            false
        );
        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            true
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_References_All_When_Another_Hierarchy_Attached_At_Root)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_References_All_When_Another_Hierarchy_Attached_At_Child)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_References_All_When_Another_Hierarchy_Attached_At_Child_Of_Child)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_References_Top_Root_When_Another_Hierarchy_Attached_At_Root)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            true
        );
        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_References_Top_Root_When_Another_Hierarchy_Attached_At_Child)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            true
        );
        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_References_Top_Root_When_Another_Hierarchy_Attached_At_Child_Of_Child)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            true
        );
        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            RootNetEntityId
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Doesnt_Keep_Child_References)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            0
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Detachment_From_Top_Root)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());
        // detach
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
        if (m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size() == 3)
        {
            EXPECT_EQ(
                m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[0],
                m_root2->m_entity.get()
            );
            EXPECT_EQ(
                m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[1],
                m_child2->m_entity.get()
            );
            EXPECT_EQ(
                m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities()[2],
                m_childOfChild2->m_entity.get()
            );
        }
    }

    TEST_F(ServerHierarchyOfHierarchyTests, InnerChildrenPointToInnerRootAfterDetachmentFromTopRoot)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());
        // detach
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_child2->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            m_root2->m_entity->FindComponent<NetBindComponent>()->GetNetEntityId()
        );
        EXPECT_EQ(
            m_childOfChild2->m_entity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            m_root2->m_entity->FindComponent<NetBindComponent>()->GetNetEntityId()
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Detachment_From_Child_Of_Child)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        // detach
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Top_Root_Deactivates)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        m_root.reset();

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Child_Of_Top_Root_Deactivates)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        m_child.reset();

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Child_Of_Child_Deactivates)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        m_childOfChild.reset();

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Stress_Test_Inner_Root_Has_Child_References_After_Detachment_From_Child_Of_Child)
    {
        for (int i = 0; i < 100; ++i)
        {
            m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
            // detach
            m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        }

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Detachment_Of_Child_Of_Child_In_Inner_Hierarchy)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        // detach
        m_childOfChild2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            5
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Attachment_Of_Child_Of_Child_In_Inner_Hierarchy)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        // detach
        m_childOfChild2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // re-connect
        m_childOfChild2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child2->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Child_Of_Child_Changed_Hierarchies)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        // detach
        m_childOfChild2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        // connect to a different hierarchy
        m_childOfChild2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Detachment_Of_Child_In_Inner_Hierarchy)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        // detach
        m_child2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            4
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Child_Changed_Hierarchies)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        // detach
        m_child2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        // connect to a different hierarchy
        m_child2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            6
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Has_No_Child_References_After_All_Children_Moved_To_Another_Hierarchy)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        m_child2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        // detach
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            1
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Inner_Root_Child_Deactivated_Top_Root_Has_No_Child_Reference_To_It)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        m_child2.reset();

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            4
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, Testing_Limiting_Hierarchy_Maximum_Size)
    {
        uint32_t currentMaxLimit = 0;
        m_console->GetCvarValue<uint32_t>("bg_hierarchyEntityMaxLimit", currentMaxLimit);
        m_console->PerformCommand("bg_hierarchyEntityMaxLimit 2");

        // remake the top level hierarchy
        m_root->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_root->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );

        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );

        m_console->PerformCommand((AZStd::string("bg_hierarchyEntityMaxLimit ") + AZStd::to_string(currentMaxLimit)).c_str());
        m_console->GetCvarValue<uint32_t>("bg_hierarchyEntityMaxLimit", currentMaxLimit);
    }

    TEST_F(ServerHierarchyOfHierarchyTests, InnerRootAndItsChildrenHaveOwningConnectionIdOfTopRoot)
    {
        // Assign new connection ids.
        m_root->m_entity->FindComponent<NetBindComponent>()->SetOwningConnectionId(ConnectionId{ 1 });
        m_root2->m_entity->FindComponent<NetBindComponent>()->SetOwningConnectionId(ConnectionId{ 2 });

        // Attach then inner hierarchy's owning connection id should then be top root's connection id.
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId(),
            m_root->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId()
        );

        EXPECT_EQ(
            m_child2->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId(),
            m_root->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId()
        );

        EXPECT_EQ(
            m_childOfChild2->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId(),
            m_root->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId()
        );
    }

    TEST_F(ServerHierarchyOfHierarchyTests, InnerRootAndItsChildrenHaveTheirOriginalOwningConnectionIdAfterDetachingFromTopRoot)
    {
        // Assign new connection ids.
        m_root->m_entity->FindComponent<NetBindComponent>()->SetOwningConnectionId(ConnectionId{ 1 });
        m_root2->m_entity->FindComponent<NetBindComponent>()->SetOwningConnectionId(ConnectionId{ 2 });

        // Attach then inner hierarchy's owning connection id should then be top root's connection id.
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        // detach, inner hierarchy should roll back to his previous owning connection id
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId(),
            ConnectionId{ 2 }
        );

        EXPECT_EQ(
            m_child2->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId(),
            m_root2->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId()
        );

        EXPECT_EQ(
            m_childOfChild2->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId(),
            m_root2->m_entity->FindComponent<NetBindComponent>()->GetOwningConnectionId()
        );
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

            m_root = AZStd::make_unique<EntityInfo>((1), "root", NetEntityId{ 1 }, EntityInfo::Role::Root);
            m_child = AZStd::make_unique<EntityInfo>((2), "child", NetEntityId{ 2 }, EntityInfo::Role::Child);
            m_childOfChild = AZStd::make_unique<EntityInfo>((3), "child of child", NetEntityId{ 3 }, EntityInfo::Role::None);

            CreateDeepHierarchy(*m_root, *m_child, *m_childOfChild);

            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());
            m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child->m_entity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChild.reset();
            m_child.reset();
            m_root.reset();

            HierarchyTests::TearDown();
        }

        AZStd::unique_ptr<EntityInfo> m_root;
        AZStd::unique_ptr<EntityInfo> m_child;
        AZStd::unique_ptr<EntityInfo> m_childOfChild;
    };

    TEST_F(ServerMixedDeepHierarchyTests, Top_Root_Ignores_Non_Hierarchical_Entities)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    TEST_F(ServerMixedDeepHierarchyTests, Detaching_Non_Hierarchical_Entity_Has_No_Effect_On_Top_Root)
    {
        m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
    }

    TEST_F(ServerMixedDeepHierarchyTests, Attaching_Non_Hierarchical_Entity_Has_No_Effect_On_Top_Root)
    {
        m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_childOfChild->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
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

            m_root2 = AZStd::make_unique<EntityInfo>((4), "root 2", NetEntityId{ 4 }, EntityInfo::Role::Root);
            m_child2 = AZStd::make_unique<EntityInfo>((5), "child 2", NetEntityId{ 5 }, EntityInfo::Role::None);
            m_childOfChild2 = AZStd::make_unique<EntityInfo>((6), "child of child 2", NetEntityId{ 6 }, EntityInfo::Role::Child);

            CreateDeepHierarchy(*m_root2, *m_child2, *m_childOfChild2);

            m_child2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root2->m_entity->GetId());
            m_childOfChild2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child2->m_entity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChild2.reset();
            m_child2.reset();
            m_root2.reset();

            ServerMixedDeepHierarchyTests::TearDown();
        }

        AZStd::unique_ptr<EntityInfo> m_root2;
        AZStd::unique_ptr<EntityInfo> m_child2;
        AZStd::unique_ptr<EntityInfo> m_childOfChild2;
    };

    TEST_F(ServerMixedHierarchyOfHierarchyTests, Sanity_Check_Ingore_Children_Without_Hierarchy_Components)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            2
        );
        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            1
        );
    }

    TEST_F(ServerMixedHierarchyOfHierarchyTests, Adding_Mixed_Hierarchy_Ingores_Children_Without_Hierarchy_Components)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());

        EXPECT_EQ(
            m_root->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerMixedHierarchyOfHierarchyTests, Attaching_Hierarchy_To_Non_Hierarchical_Entity_Does_Not_Merge_Hierarchies)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->IsHierarchicalChild(),
            false
        );
    }

    /*
     * Sets up a hierarchy with 3 roots, 2 of them being inner roots.
     */
    class ServerHierarchyWithThreeRoots : public ServerHierarchyOfHierarchyTests
    {
    public:
        const NetEntityId Root3NetEntityId = NetEntityId{ 7 };
        const NetEntityId Child3NetEntityId = NetEntityId{ 8 };
        const NetEntityId ChildOfChild3NetEntityId = NetEntityId{ 9 };

        void SetUp() override
        {
            ServerHierarchyOfHierarchyTests::SetUp();

            m_root3 = AZStd::make_unique<EntityInfo>((7), "root 3", Root3NetEntityId, EntityInfo::Role::Root);
            m_child3 = AZStd::make_unique<EntityInfo>((8), "child 3", Child3NetEntityId, EntityInfo::Role::Child);
            m_childOfChild3 = AZStd::make_unique<EntityInfo>((9), "child of child 3", ChildOfChild3NetEntityId, EntityInfo::Role::Child);

            CreateDeepHierarchy(*m_root3, *m_child3, *m_childOfChild3);

            m_child3->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root3->m_entity->GetId());
            m_childOfChild3->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_child3->m_entity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChild3.reset();
            m_child3.reset();
            m_root3.reset();

            ServerHierarchyOfHierarchyTests::TearDown();
        }

        AZStd::unique_ptr<EntityInfo> m_root3;
        AZStd::unique_ptr<EntityInfo> m_child3;
        AZStd::unique_ptr<EntityInfo> m_childOfChild3;
    };

    TEST_F(ServerHierarchyWithThreeRoots, Top_Root_Active_Then_Inner_Roots_Have_No_Child_References)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        m_root3->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            0
        );
        EXPECT_EQ(
            m_root3->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            0
        );
    }

    TEST_F(ServerHierarchyWithThreeRoots, Top_Root_Deactivates_Inner_Roots_Have_Child_References)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        m_root3->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        m_root.reset();

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
        EXPECT_EQ(
            m_root3->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyWithThreeRoots, Child_Of_Top_Root_Deactivates_Inner_Roots_Have_Child_References)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        m_root3->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        m_child.reset();

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
        EXPECT_EQ(
            m_root3->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyWithThreeRoots, Child_Of_Child_Of_Top_Root_Deactivates_Inner_Roots_Have_Child_References)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        m_root3->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        m_childOfChild.reset();

        EXPECT_EQ(
            m_root2->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
        EXPECT_EQ(
            m_root3->m_entity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchicalEntities().size(),
            3
        );
    }

    TEST_F(ServerHierarchyWithThreeRoots, InnerRootLeftTopRootThenLastChildGetsJoinedEventOnce)
    {
        m_root2->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());
        m_root3->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChild->m_entity->GetId());

        MockNetworkHierarchyCallbackHandler mock;
        EXPECT_CALL(mock, OnNetworkHierarchyUpdated(m_root3->m_entity->GetId()));

        m_childOfChild3->m_entity->FindComponent<NetworkHierarchyChildComponent>()->BindNetworkHierarchyChangedEventHandler(mock.m_changedHandler);

        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
    }
}
