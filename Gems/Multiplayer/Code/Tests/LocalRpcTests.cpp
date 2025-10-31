/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CommonNetworkEntitySetup.h>
#include <MockInterfaces.h>
#include <RpcUnitTesterComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Name/Name.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Source/NetworkEntity/NetworkEntityManager.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    class LocalRpcTests : public NetworkEntityTests
    {
    public:
        void SetUp() override
        {
            NetworkEntityTests::SetUp();

            ON_CALL(*m_mockMultiplayer, GetAgentType()).WillByDefault(Return(MultiplayerAgentType::ClientServer));
            m_rpcTesterDescriptor.reset(MultiplayerTest::RpcUnitTesterComponent::CreateDescriptor());
            m_rpcTesterDescriptor->Reflect(m_serializeContext.get());

            ConfigureEntity(NetEntityRole::Authority);
        }

        void ConfigureEntity(NetEntityRole local)
        {
            m_root = AZStd::make_unique<EntityInfo>(1, "entity", NetEntityId{ 1 }, EntityInfo::Role::Root);

            PopulateNetworkEntity(*m_root);
            SetupEntity(m_root->m_entity, m_root->m_netId, local);
            m_root->m_entity->Activate();

            // For a local client-server entity replicators are NOT created.
        }

        void TearDown() override
        {
            m_root.reset();
            m_rpcTesterDescriptor.reset();

            NetworkEntityTests::TearDown();
        }

        void PopulateNetworkEntity(const EntityInfo& entityInfo)
        {
            entityInfo.m_entity->CreateComponent<AzFramework::TransformComponent>();
            entityInfo.m_entity->CreateComponent<NetBindComponent>();
            entityInfo.m_entity->CreateComponent<NetworkTransformComponent>();
            entityInfo.m_entity->CreateComponent<MultiplayerTest::RpcUnitTesterComponent>();
        }

        AZStd::unique_ptr<EntityInfo> m_root;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_rpcTesterDescriptor;
    };


    TEST_F(LocalRpcTests, LocalRpcServerToAuthority)
    {
        const auto component = m_root->m_entity->FindComponent<MultiplayerTest::RpcUnitTesterComponent>();
        component->RPC_ServerToAuthority();

        m_networkEntityManager->DispatchLocalDeferredRpcMessages();
        EXPECT_EQ(component->GetTestController()->m_serverToAuthorityCalls, 1);
    }

    TEST_F(LocalRpcTests, LocalRpcAuthorityToClient)
    {
        const auto component = m_root->m_entity->FindComponent<MultiplayerTest::RpcUnitTesterComponent>();
        component->GetTestController()->RPC_AuthorityToClient();

        m_networkEntityManager->DispatchLocalDeferredRpcMessages();
        EXPECT_EQ(component->m_authorityToClientCalls, 1);
    }

    TEST_F(LocalRpcTests, LocalRpcAuthorityToAutonomous)
    {
        m_root->m_entity->FindComponent<NetBindComponent>()->EnablePlayerHostAutonomy(true);

        const auto component = m_root->m_entity->FindComponent<MultiplayerTest::RpcUnitTesterComponent>();
        component->GetTestController()->RPC_AuthorityToAutonomous();

        m_networkEntityManager->DispatchLocalDeferredRpcMessages();        

        EXPECT_EQ(component->GetTestController()->m_authorityToAutonomousCalls, 1);
    }

    TEST_F(LocalRpcTests, LocalRpcAuthorityToRemoteAutonomous)
    {
        // Turn off player host autonomy.
        // This simulates a host machine (authority) sending an RPC to a remote autonomous client
        // For example, a local prediction player input correction RPC
        m_root->m_entity->FindComponent<NetBindComponent>()->EnablePlayerHostAutonomy(false);

        const auto component = m_root->m_entity->FindComponent<MultiplayerTest::RpcUnitTesterComponent>();
        component->GetTestController()->RPC_AuthorityToAutonomous();

        m_networkEntityManager->DispatchLocalDeferredRpcMessages();

        EXPECT_EQ(component->GetTestController()->m_authorityToAutonomousCalls, 0);
    }

    TEST_F(LocalRpcTests, LocalRpcAutonomousToAuthority)
    {
        m_root->m_entity->FindComponent<NetBindComponent>()->EnablePlayerHostAutonomy(true);

        const auto component = m_root->m_entity->FindComponent<MultiplayerTest::RpcUnitTesterComponent>();
        component->GetTestController()->RPC_AutonomousToAuthority();
        
        m_networkEntityManager->DispatchLocalDeferredRpcMessages();

        EXPECT_EQ(component->GetTestController()->m_autonomousToAuthorityCalls, 1);
    }
}
