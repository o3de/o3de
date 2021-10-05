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
#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    /*
     * (Networked) Parent -> (Networked) Child
     */
    class ServerNetTransformTests : public HierarchyTests
    {
    public:
        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_root = AZStd::make_unique<EntityInfo>(1, "root", NetEntityId{ 1 }, EntityInfo::Role::Root);
            m_child = AZStd::make_unique<EntityInfo>(2, "child", NetEntityId{ 2 }, EntityInfo::Role::Child);

            CreateNetworkParentChild(*m_root, *m_child);

            AZ::Transform rootTransform = AZ::Transform::CreateIdentity();
            rootTransform.SetTranslation(AZ::Vector3::CreateOne());
            m_root->m_entity->FindComponent<AzFramework::TransformComponent>()->SetWorldTM(rootTransform);
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetWorldTM(rootTransform);

            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_root->m_entity->GetId());
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetLocalTM(AZ::Transform::CreateIdentity());

            AZ::EntityBus::Broadcast(&AZ::EntityBus::Events::OnEntityActivated, m_root->m_entity->GetId());

            MultiplayerTick();
        }

        void TearDown() override
        {
            m_child.reset();
            m_root.reset();

            HierarchyTests::TearDown();
        }

        void PopulateNetworkEntity(const EntityInfo& entityInfo)
        {
            entityInfo.m_entity->CreateComponent<AzFramework::TransformComponent>();
            entityInfo.m_entity->CreateComponent<NetBindComponent>();
            entityInfo.m_entity->CreateComponent<NetworkTransformComponent>();
        }

        void CreateNetworkParentChild(EntityInfo& root, EntityInfo& child)
        {
            PopulateNetworkEntity(root);
            SetupEntity(root.m_entity, root.m_netId, NetEntityRole::Authority);

            PopulateNetworkEntity(child);
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

        void MultiplayerTick()
        {
            m_root->m_entity->FindComponent<NetBindComponent>()->NotifyPreRender(0.1f);
            m_child->m_entity->FindComponent<NetBindComponent>()->NotifyPreRender(0.1f);
        }
    };

    TEST_F(ServerNetTransformTests, SanityCheck)
    {
        EXPECT_EQ(
            m_root->m_entity->FindComponent<AzFramework::TransformComponent>()->GetWorldTM().GetTranslation(),
            AZ::Vector3::CreateOne()
        );

        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetWorldTM().GetTranslation(),
            AZ::Vector3::CreateOne()
        );
        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetLocalTM().GetTranslation(),
            AZ::Vector3::CreateZero()
        );
    }

    TEST_F(ServerNetTransformTests, NetTransformSavesLocalTransformWhenParentSet)
    {
        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkTransformComponent>()->GetTranslation(),
            AZ::Vector3::CreateZero()
        );
    }

    TEST_F(ServerNetTransformTests, NetTransformSavesWorldTransformWhenParentIsNotSet)
    {
        m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        MultiplayerTick();

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkTransformComponent>()->GetTranslation(),
            AZ::Vector3::CreateOne() // back at the parent translation
        );
    }

    TEST_F(ServerNetTransformTests, ParentMovesChildNetTransformDoesntChange)
    {
        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkTransformComponent>()->GetTranslation(),
            AZ::Vector3::CreateZero()
        );

        // move the parent
        AZ::Transform rootTransform = AZ::Transform::CreateIdentity();
        rootTransform.SetTranslation(AZ::Vector3::CreateOne() * 10.f);
        m_root->m_entity->FindComponent<AzFramework::TransformComponent>()->SetWorldTM(rootTransform);

        MultiplayerTick();

        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetWorldTM().GetTranslation(),
            AZ::Vector3::CreateOne() * 10.f
        );
        // child local tm doesn't change
        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetLocalTM().GetTranslation(),
            AZ::Vector3::CreateZero()
        );

        EXPECT_EQ(
            m_child->m_entity->FindComponent<NetworkTransformComponent>()->GetTranslation(),
            AZ::Vector3::CreateZero()
        );
    }

    /*
     * (Networked) Parent -> (Networked) Child
     */
    class ClientNetTransformTests : public HierarchyTests
    {
    public:
        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_root = AZStd::make_unique<EntityInfo>(1, "root", NetEntityId{ 1 }, EntityInfo::Role::Root);
            m_child = AZStd::make_unique<EntityInfo>(2, "child", NetEntityId{ 2 }, EntityInfo::Role::Child);

            CreateNetworkParentChild(*m_root, *m_child);
        }

        void TearDown() override
        {
            m_child.reset();
            m_root.reset();

            HierarchyTests::TearDown();
        }

        void PopulateNetworkEntity(const EntityInfo& entityInfo)
        {
            entityInfo.m_entity->CreateComponent<AzFramework::TransformComponent>();
            entityInfo.m_entity->CreateComponent<NetBindComponent>();
            entityInfo.m_entity->CreateComponent<NetworkTransformComponent>();
        }

        void CreateNetworkParentChild(EntityInfo& root, EntityInfo& child)
        {
            PopulateNetworkEntity(root);
            SetupEntity(root.m_entity, root.m_netId, NetEntityRole::Client);

            PopulateNetworkEntity(child);
            SetupEntity(child.m_entity, child.m_netId, NetEntityRole::Client);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childHandle(child.m_entity.get(), m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
            child.m_replicator->Initialize(childHandle);

            // Create an entity replicator for the root entity
            const NetworkEntityHandle rootHandle(root.m_entity.get(), m_networkEntityTracker.get());
            root.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, rootHandle);
            root.m_replicator->Initialize(rootHandle);
        }

        AZStd::unique_ptr<EntityInfo> m_root;
        AZStd::unique_ptr<EntityInfo> m_child;

        void MultiplayerTick()
        {
            m_root->m_entity->FindComponent<NetBindComponent>()->NotifyPreRender(0.1f);
            m_child->m_entity->FindComponent<NetBindComponent>()->NotifyPreRender(0.1f);
        }
    };

    TEST_F(ClientNetTransformTests, ClientSetsLocalTmWhenParentIsSet)
    {
        m_root->m_entity->Activate();
        m_child->m_entity->Activate();

        SetTranslationOnNetworkTransform(m_root->m_entity, AZ::Vector3::CreateOne());

        SetParentIdOnNetworkTransform(m_child->m_entity, NetEntityId{ 1 });
        SetTranslationOnNetworkTransform(m_child->m_entity, AZ::Vector3::CreateZero());

        AZ::EntityBus::Broadcast(&AZ::EntityBus::Events::OnEntityActivated, m_root->m_entity->GetId());

        MultiplayerTick();

        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetWorldTM().GetTranslation(),
            AZ::Vector3::CreateOne()
        );
        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetLocalTM().GetTranslation(),
            AZ::Vector3::CreateZero()
        );
    }

    TEST_F(ClientNetTransformTests, ClientSetsWorldTmWhenParentIsNotSet)
    {
        m_root->m_entity->Activate();
        m_child->m_entity->Activate();

        SetTranslationOnNetworkTransform(m_root->m_entity, AZ::Vector3::CreateOne());
        SetTranslationOnNetworkTransform(m_child->m_entity, AZ::Vector3::CreateZero());

        AZ::EntityBus::Broadcast(&AZ::EntityBus::Events::OnEntityActivated, m_root->m_entity->GetId());

        MultiplayerTick();

        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetWorldTM().GetTranslation(),
            AZ::Vector3::CreateZero()
        );
        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetLocalTM().GetTranslation(),
            AZ::Vector3::CreateZero()
        );
    }

    TEST_F(ClientNetTransformTests, ChildFollowsWhenParentMovesOnServer)
    {
        m_root->m_entity->Activate();
        m_child->m_entity->Activate();

        SetTranslationOnNetworkTransform(m_root->m_entity, AZ::Vector3::CreateOne());

        SetParentIdOnNetworkTransform(m_child->m_entity, NetEntityId{ 1 });
        SetTranslationOnNetworkTransform(m_child->m_entity, AZ::Vector3::CreateZero());

        AZ::EntityBus::Broadcast(&AZ::EntityBus::Events::OnEntityActivated, m_root->m_entity->GetId());

        MultiplayerTick();

        // now parent moves
        SetTranslationOnNetworkTransform(m_root->m_entity, AZ::Vector3::CreateOne() * 2.f);
        MultiplayerTick();

        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetWorldTM().GetTranslation(),
            AZ::Vector3::CreateOne() * 2.f
        );
        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetLocalTM().GetTranslation(),
            AZ::Vector3::CreateZero()
        );
    }

    TEST_F(ClientNetTransformTests, ChildAttachesToParentIfParentIdIsSetBeforeActivation)
    {
        m_root->m_entity->Activate();

        SetTranslationOnNetworkTransform(m_root->m_entity, AZ::Vector3::CreateOne());

        SetParentIdOnNetworkTransform(m_child->m_entity, NetEntityId{ 1 });
        SetTranslationOnNetworkTransform(m_child->m_entity, AZ::Vector3::CreateZero());

        m_child->m_entity->Activate();

        AZ::EntityBus::Broadcast(&AZ::EntityBus::Events::OnEntityActivated, m_root->m_entity->GetId());

        MultiplayerTick();

        EXPECT_EQ(
            m_child->m_entity->FindComponent<AzFramework::TransformComponent>()->GetParentId(),
            AZ::EntityId(1)
        );
    }
}
