/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CommonNetworkEntitySetup.h>
#include <MockInterfaces.h>
#include <TestMultiplayerComponent.h>
#include <Source/NetworkEntity/NetworkEntityManager.h>
#include <Source/NetworkEntity/EntityReplication/PropertyPublisher.h>
#include <Source/EntityDomains/FullOwnershipEntityDomain.h>
#include <Source/EntityDomains/NullEntityDomain.h>
#include <Source/ReplicationWindows/NullReplicationWindow.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Name/Name.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzNetworking/Serialization/StringifySerializer.h>
#include <AzNetworking/UdpTransport/UdpPacketHeader.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>
#include <Multiplayer/NetworkInput/NetworkInput.h>
#include <Multiplayer/NetworkInput/NetworkInputArray.h>
#include <Multiplayer/NetworkInput/NetworkInputHistory.h>
#include <Multiplayer/NetworkInput/NetworkInputMigrationVector.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    class MultiplayerNetworkEntityTests : public NetworkEntityTests
    {
    public:
        void SetUp() override
        {
            NetworkEntityTests::SetUp();

            m_root = AZStd::make_unique<EntityInfo>(1, "root", NetEntityId{ 1 }, EntityInfo::Role::Root);

            PopulateNetworkEntity(*m_root);
            SetupEntity(m_root->m_entity, m_root->m_netId, NetEntityRole::Authority);

            // Create an entity replicator for the root entity
            const NetworkEntityHandle rootHandle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());
            m_root->m_replicator = AZStd::make_unique<EntityReplicator>(
                *m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, rootHandle);
            m_root->m_replicator->Initialize(rootHandle);
            m_root->m_replicator->ActivateNetworkEntity();
        }

        void TearDown() override
        {
            m_root.reset();

            NetworkEntityTests::TearDown();
        }

        void PopulateNetworkEntity(const EntityInfo& entityInfo)
        {
            entityInfo.m_entity->CreateComponent<AzFramework::TransformComponent>();
            entityInfo.m_entity->CreateComponent<NetBindComponent>();
            entityInfo.m_entity->CreateComponent<NetworkTransformComponent>();
        }

        AZStd::unique_ptr<EntityInfo> m_root;
    };

    TEST_F(MultiplayerNetworkEntityTests, ConstNetworkEntityHandleTest)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());

        EXPECT_TRUE(handle.Exists());
        EXPECT_NE(handle.GetEntity(), nullptr);
        EXPECT_NE(handle.GetNetBindComponent(), nullptr);
        EXPECT_NE(handle.GetNetEntityId(), InvalidNetEntityId);

        EXPECT_TRUE(handle == handle.GetEntity());
        EXPECT_TRUE(handle.GetEntity() == handle);
        EXPECT_FALSE(handle != handle.GetEntity());
        EXPECT_FALSE(handle.GetEntity() != handle);
        EXPECT_FALSE(handle == nullptr);
        EXPECT_FALSE(nullptr == handle);
        EXPECT_TRUE(handle != nullptr);
        EXPECT_TRUE(nullptr != handle);

        EXPECT_TRUE(handle == handle);
        EXPECT_FALSE(handle != handle);
        EXPECT_FALSE(handle < handle);

        EXPECT_NE(handle.FindComponent<NetworkTransformComponent>(), nullptr);
        EXPECT_NE(handle.FindComponent(NetworkTransformComponent::RTTI_Type()), nullptr);
        EXPECT_EQ(handle.FindComponent<NetworkHierarchyChildComponent>(), nullptr);
        EXPECT_EQ(handle.FindComponent(NetworkHierarchyChildComponent::RTTI_Type()), nullptr);
        handle.Reset(handle);
        EXPECT_NE(handle, nullptr);
        handle.Reset();
        EXPECT_EQ(handle, nullptr);
        EXPECT_EQ(handle.GetNetBindComponent(), nullptr);
    }

    TEST_F(MultiplayerNetworkEntityTests, NetworkEntityHandleTest)
    {
        NetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());

        EXPECT_NE(handle.FindComponent(NetworkTransformComponent::RTTI_Type()), nullptr);
        EXPECT_NE(handle.FindController(NetworkTransformComponent::RTTI_Type()), nullptr);
    }

    TEST_F(MultiplayerNetworkEntityTests, TestEntityAuthorityTracker)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());
        const HostId localhost = HostId("127.0.0.1", 6777, ProtocolType::Udp);

        m_networkEntityManager->Initialize(localhost, AZStd::make_unique<NullEntityDomain>());
        NetworkEntityAuthorityTracker* netEntityAuthorityTracker = m_networkEntityManager->GetNetworkEntityAuthorityTracker();
        
        netEntityAuthorityTracker->AddEntityAuthorityManager(handle, localhost);
        EXPECT_TRUE(netEntityAuthorityTracker->DoesEntityHaveOwner(handle));
        netEntityAuthorityTracker->RemoveEntityAuthorityManager(handle, localhost);
        // Succeeds on authority role
        EXPECT_TRUE(netEntityAuthorityTracker->DoesEntityHaveOwner(handle));
        netEntityAuthorityTracker->SetTimeoutTimeMs(AZ::TimeMs(33));
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNullDomain)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());
        const HostId localhost = HostId("127.0.0.1", 6777, ProtocolType::Udp);

        m_networkEntityManager->Initialize(localhost, AZStd::make_unique<NullEntityDomain>());
        EXPECT_TRUE(m_networkEntityManager->IsInitialized());
        IEntityDomain* domain = m_networkEntityManager->GetEntityDomain();
        EXPECT_NE(domain, nullptr);
        EXPECT_EQ(domain->GetAabb(), AZ::Aabb::CreateNull());
        EXPECT_FALSE(domain->IsInDomain(handle));
        domain->SetAabb(AZ::Aabb::CreateFromMinMax(AZ::Vector3(1, 1, 1), AZ::Vector3(2, 2, 2)));
        EXPECT_EQ(domain->GetAabb(), AZ::Aabb::CreateNull());
        domain->HandleLossOfAuthoritativeReplicator(handle);
        EXPECT_TRUE(m_networkEntityManager->IsMarkedForRemoval(handle));
        domain->DebugDraw();
    }

    TEST_F(MultiplayerNetworkEntityTests, TestFullOwnershipDomain)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());
        const HostId localhost = HostId("127.0.0.1", 6777, ProtocolType::Udp);

        m_networkEntityManager->Initialize(localhost, AZStd::make_unique<FullOwnershipEntityDomain>());
        EXPECT_TRUE(m_networkEntityManager->IsInitialized());
        IEntityDomain* domain = m_networkEntityManager->GetEntityDomain();
        EXPECT_NE(domain, nullptr);
        EXPECT_EQ(domain->GetAabb(), AZ::Aabb::CreateNull());
        EXPECT_TRUE(domain->IsInDomain(handle));
        domain->SetAabb(AZ::Aabb::CreateFromMinMax(AZ::Vector3(1, 1, 1), AZ::Vector3(2, 2, 2)));
        EXPECT_EQ(domain->GetAabb(), AZ::Aabb::CreateNull());
        AZ_TEST_START_TRACE_SUPPRESSION;
        domain->HandleLossOfAuthoritativeReplicator(handle);
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        domain->DebugDraw();
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetworkEntityTracker)
    {
        const NetworkEntityTracker* constNetEntityTracker = m_networkEntityManager->GetNetworkEntityTracker();

        EXPECT_EQ(constNetEntityTracker->GetNetBindComponent(nullptr), nullptr);
        uint8_t entityCount = 0;
        for (NetworkEntityTracker::const_iterator it = constNetEntityTracker->begin(); it != constNetEntityTracker->end(); ++it)
        {
            ++entityCount;
        }
        EXPECT_EQ(entityCount, constNetEntityTracker->size());

        NetworkEntityTracker* netEntityTracker = m_networkEntityManager->GetNetworkEntityTracker();

        Multiplayer::NetEntityId netId = netEntityTracker->Get(m_root->m_entity->GetId());
        NetworkEntityHandle handle = netEntityTracker->Get(netId);
        EXPECT_NE(handle.GetEntity(), nullptr);
        AZ::EntityId InvalidId = AZ::EntityId();
        EXPECT_EQ(netEntityTracker->Get(InvalidId), InvalidNetEntityId);
        ConstNetworkEntityHandle constHandle = constNetEntityTracker->Get(netId);
        EXPECT_EQ(handle, constHandle);

        EXPECT_TRUE(netEntityTracker->Exists(netId));

        NetworkEntityTracker::iterator it = netEntityTracker->begin();
        AZ::Entity* entity = netEntityTracker->Move(it);
        EXPECT_NE(entity, nullptr);
        netEntityTracker->Add(it->first, entity);
        netEntityTracker->erase(it->first);
        EXPECT_FALSE(netEntityTracker->Exists(netId));
    }

    TEST_F(MultiplayerNetworkEntityTests, TestReplicatorPendingDeletion)
    {
        m_root->m_replicator->SetPendingRemoval(AZ::TimeMs(100));
        EXPECT_TRUE(m_root->m_replicator->IsPendingRemoval());
        m_root->m_replicator->ClearPendingRemoval();
        EXPECT_FALSE(m_root->m_replicator->IsPendingRemoval());
        EXPECT_FALSE(m_root->m_replicator->IsDeletionAcknowledged());
    }

    struct TestRpcStruct : public Multiplayer::IRpcParamStruct
    {
        TestRpcStruct()
        {
        }

        TestRpcStruct(const AZ::Vector3& impulse, const AZ::Vector3& worldPoint)
            : m_impulse(impulse)
            , m_worldPoint(worldPoint)
        {
            ;
        }

        bool Serialize(AzNetworking::ISerializer& serializer) override
        {
            bool ret(true);
            ret &= serializer.Serialize(m_impulse, "impulse");
            ret &= serializer.Serialize(m_worldPoint, "worldPoint");
            return ret;
        };

        AZ::Vector3 m_impulse;
        AZ::Vector3 m_worldPoint;
    };

    TEST_F(MultiplayerNetworkEntityTests, TestNetworkEntityRpcMessage)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());

        NetworkEntityRpcMessage message = NetworkEntityRpcMessage(
            RpcDeliveryType::AuthorityToClient,
            handle.GetNetEntityId(),
            handle.FindComponent<NetworkTransformComponent>()->GetNetComponentId(),
            RpcIndex(0),
            ReliabilityType::Unreliable);

        // Test getters
        TestRpcStruct params;
        EXPECT_FALSE(message.GetRpcParams(params));
        message.SetRpcParams(params);
        EXPECT_EQ(message.GetComponentId(), handle.FindComponent<NetworkTransformComponent>()->GetNetComponentId());
        EXPECT_EQ(message.GetEntityId(), handle.GetNetEntityId());
        EXPECT_EQ(message.GetReliability(), ReliabilityType::Unreliable);
        EXPECT_EQ(message.GetRpcDeliveryType(), RpcDeliveryType::AuthorityToClient);
        EXPECT_EQ(message.GetRpcIndex(), RpcIndex(0));
        EXPECT_TRUE(message.GetRpcParams(params));

        // Test setters
        message.SetReliability(ReliabilityType::Reliable);
        EXPECT_EQ(message.GetReliability(), ReliabilityType::Reliable);
        message.SetRpcDeliveryType(RpcDeliveryType::AuthorityToAutonomous);
        EXPECT_EQ(message.GetRpcDeliveryType(), RpcDeliveryType::AuthorityToAutonomous);

        const NetworkEntityRpcMessage constMessage = NetworkEntityRpcMessage(
            RpcDeliveryType::AuthorityToClient,
            handle.GetNetEntityId(),
            handle.FindComponent<NetworkTransformComponent>()->GetNetComponentId(),
            RpcIndex(1),
            ReliabilityType::Unreliable);

        // Test ctors, assignment and comparisons (const and non const versions)
        NetworkEntityRpcMessage message2(AZStd::move(message));
        EXPECT_EQ(message, message2);
        EXPECT_TRUE(message != constMessage);
        message = constMessage;
        EXPECT_EQ(message, constMessage);
        message = NetworkEntityRpcMessage(constMessage);
        EXPECT_EQ(message, constMessage);

        NetworkEntityRpcMessage message3 = constMessage;
        message3 = message;
        EXPECT_EQ(message, message3);

        const NetworkEntityRpcMessage constMessage2(message2);
        NetworkEntityRpcMessage message4 = constMessage;
        message4 = constMessage2;

        AzNetworking::StringifySerializer serializer;
        EXPECT_TRUE(message.Serialize(serializer));
        EXPECT_EQ(message.GetEstimatedSerializeSize(), 15);

        m_entityReplicationManager->AddDeferredRpcMessage(message);
        m_entityReplicationManager->AddDeferredRpcMessage(message2);
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_entityReplicationManager->SendUpdates();
        AZ_TEST_STOP_TRACE_SUPPRESSION(3);

        m_entityReplicationManager->SetReplicationWindow(AZStd::make_unique<NullReplicationWindow>(m_mockConnection.get()));
        m_entityReplicationManager->AddDeferredRpcMessage(message);
        m_entityReplicationManager->AddDeferredRpcMessage(message2);
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_entityReplicationManager->SendUpdates();
        AZ_TEST_STOP_TRACE_SUPPRESSION(0);
        NetworkEntityRpcVector rpcVector;
        rpcVector.push_back(message);
        m_entityReplicationManager->HandleEntityRpcMessages(m_mockConnection.get(), rpcVector);
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetworkEntityUpdateMessage)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());

        NetworkEntityUpdateMessage message = NetworkEntityUpdateMessage();

        // Test getters
        EXPECT_EQ(message.GetNetworkRole(), NetEntityRole::InvalidRole);
        EXPECT_EQ(message.GetData(), nullptr);
        EXPECT_EQ(message.GetEntityId(), InvalidNetEntityId);
        EXPECT_FALSE(message.GetHasValidPrefabId());
        EXPECT_FALSE(message.GetIsDelete());
        EXPECT_TRUE(message.GetPrefabEntityId().m_prefabName.IsEmpty());
        EXPECT_FALSE(message.GetWasMigrated());

        // Test setters
        PrefabEntityId prefabId;
        prefabId.m_entityOffset = 0;
        prefabId.m_prefabName = AZ::Name("Test");
        message.SetPrefabEntityId(prefabId);
        EXPECT_NE(message.GetPrefabEntityId().m_prefabName.GetCStr(), "");
        message.SetData(message.ModifyData());
        EXPECT_NE(message.GetData(), nullptr);

        AzNetworking::StringifySerializer serializer;
        EXPECT_TRUE(message.Serialize(serializer));
        EXPECT_EQ(message.GetEstimatedSerializeSize(), 17);

        // Test ctors, assignment and comparisons (const and non const versions)
        message = NetworkEntityUpdateMessage(m_root->m_netId, false);
        EXPECT_EQ(m_root->m_netId, message.GetEntityId());
        message = NetworkEntityUpdateMessage(NetEntityRole::Authority, m_root->m_netId);
        EXPECT_EQ(message.GetNetworkRole(), NetEntityRole::Authority);
        message = NetworkEntityUpdateMessage(NetEntityRole::Authority, m_root->m_netId, prefabId);
        AzNetworking::PacketEncodingBuffer buffer;
        EXPECT_NE(message.GetPrefabEntityId().m_prefabName.GetCStr(), "");
        message.SetData(buffer);
        NetworkEntityUpdateMessage message2(AZStd::move(message));
        EXPECT_EQ(message, message2);
        EXPECT_TRUE(m_root->m_replicator->GetPropertyPublisher()->PrepareSerialization());
        const NetworkEntityUpdateMessage constMessage = m_root->m_replicator->GenerateUpdatePacket();
        EXPECT_TRUE(message != constMessage);
        message = constMessage;
        EXPECT_EQ(message, constMessage);
        message = NetworkEntityUpdateMessage(constMessage);
        EXPECT_EQ(message, constMessage);

        NetworkEntityUpdateMessage message3 = constMessage;
        message3 = message;
        EXPECT_EQ(message, message3);

        const NetworkEntityUpdateMessage constMessage2(message2);
        NetworkEntityUpdateMessage message4 = constMessage;
        message4 = constMessage2;

        UdpPacketHeader header(PacketType{ 11111 }, InvalidSequenceId, SequenceId{ 1 }, InvalidSequenceId, 0xF8000FFF, SequenceRolloverCount{ 0 });
        AZ_TEST_START_TRACE_SUPPRESSION;
        EXPECT_FALSE(m_entityReplicationManager->HandleEntityUpdateMessage(m_mockConnection.get(), header, message));
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
        EXPECT_FALSE(m_entityReplicationManager->HandleEntityDeleteMessage(m_root->m_replicator.get(), header, message));
        EXPECT_TRUE(m_entityReplicationManager->HandleEntityUpdateMessage(m_mockConnection.get(), header, constMessage));
    }

} // namespace Multiplayer
