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
        message = NetworkEntityUpdateMessage(NetEntityRole::Authority, m_root->m_netId, true, false);
        EXPECT_EQ(m_root->m_netId, message.GetEntityId());
        EXPECT_EQ(message.GetNetworkRole(), NetEntityRole::Authority);
        message = NetworkEntityUpdateMessage(NetEntityRole::Authority, m_root->m_netId, false, false);
        AzNetworking::PacketEncodingBuffer buffer;
        message.SetPrefabEntityId(prefabId);
        EXPECT_NE(message.GetPrefabEntityId().m_prefabName.GetCStr(), "");
        message.SetData(buffer);
        NetworkEntityUpdateMessage message2(AZStd::move(message));
        EXPECT_EQ(message, message2);
        EXPECT_TRUE(m_root->m_replicator->PrepareToGenerateUpdatePacket());
        const NetworkEntityUpdateMessage constMessage = m_root->m_replicator->GenerateUpdatePacket();
        m_root->m_replicator->RecordSentPacketId(AzNetworking::PacketId{ 1 });
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

    TEST_F(MultiplayerNetworkEntityTests, EntityReplicatorNoDeleteSentIfCreateWasNotSent)
    {
        // Don't send an entity delete message if no create messages have been sent yet either.
        m_root->m_replicator->MarkForRemoval();
        EXPECT_FALSE(m_root->m_replicator->HasChangesToPublish());
    }

    TEST_F(MultiplayerNetworkEntityTests, EntityReplicationManagerNoDeleteHandledIfNoCreateReceived)
    {
        // Don't process an entity delete message if no create message has been received yet.
        // If the message is processed, the entity will get created then immediately destroyed, which is wasted processing.

        // "Send" a creation message.
        EXPECT_TRUE(m_root->m_replicator->HasChangesToPublish());
        EXPECT_TRUE(m_root->m_replicator->PrepareToGenerateUpdatePacket());
        const NetworkEntityUpdateMessage createMessage = m_root->m_replicator->GenerateUpdatePacket();
        m_root->m_replicator->RecordSentPacketId(AzNetworking::PacketId{ 1 });

        // Mark the entity as deleted and "send" a delete message.
        m_root->m_replicator->MarkForRemoval();
        EXPECT_TRUE(m_root->m_replicator->PrepareToGenerateUpdatePacket());
        const NetworkEntityUpdateMessage deleteMessage = m_root->m_replicator->GenerateUpdatePacket();
        m_root->m_replicator->RecordSentPacketId(AzNetworking::PacketId{ 2 });

        // When processing the message, nothing should happen, and the message should be marked as handled because it is dropped.
        // If the message were attempted to be processed, HandleEntityUpdateMessage() would return false from
        // a deserialize failure due to the wrong network role, which would then cause this test to fail.
        UdpPacketHeader header(
            PacketType{ 11111 }, InvalidSequenceId, SequenceId{ 1 }, InvalidSequenceId, 0xF8000FFF, SequenceRolloverCount{ 0 });
        EXPECT_TRUE(m_entityReplicationManager->HandleEntityUpdateMessage(m_mockConnection.get(), header, deleteMessage));
    }

    TEST_F(MultiplayerNetworkEntityTests, EntityReplicatorDeleteMessageIncludesUpdatedProperties)
    {
        // When sending a delete message, the message should also contain any properties that were changed
        // since the previous replication.

        // Always claim that every packet sent was acknowledged.
        ON_CALL(*m_mockConnection, WasPacketAcked).WillByDefault(::testing::Return(true));

        // First "send" a creation message.
        EXPECT_TRUE(m_root->m_replicator->HasChangesToPublish());
        EXPECT_TRUE(m_root->m_replicator->PrepareToGenerateUpdatePacket());
        const NetworkEntityUpdateMessage createMessage = m_root->m_replicator->GenerateUpdatePacket();
        m_root->m_replicator->RecordSentPacketId(AzNetworking::PacketId{ 1 });
        EXPECT_FALSE(createMessage.GetIsDelete());
        

        // It should be seen as sent, so nothing more to publish.
        EXPECT_FALSE(m_root->m_replicator->HasChangesToPublish());

        // Change the translation on the entity and notify that it has been dirtied.
        // Make sure the replicator sees it as a new change.
        AZ::TransformBus::Event(m_root->m_entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, AZ::Vector3(1.0f, 2.0f, 3.0f));
        m_networkEntityManager->NotifyEntitiesDirtied();
        EXPECT_TRUE(m_root->m_replicator->HasChangesToPublish());

        // Next, mark the entity as deleted.
        m_root->m_replicator->MarkForRemoval();

        // The delete should be seen as a change that needs to be sent.
        EXPECT_TRUE(m_root->m_replicator->HasChangesToPublish());

        // Generate the delete packet.
        EXPECT_TRUE(m_root->m_replicator->PrepareToGenerateUpdatePacket());
        const NetworkEntityUpdateMessage deleteMessage = m_root->m_replicator->GenerateUpdatePacket();
        m_root->m_replicator->RecordSentPacketId(AzNetworking::PacketId{ 2 });

        // The message should be a delete message that has a payload larger than 3 bytes.
        // A 3-byte payload is just a header, more than 3 bytes includes property data changes.
        EXPECT_TRUE(deleteMessage.GetIsDelete());
        EXPECT_GT(deleteMessage.GetData()->GetSize(), 3);

        // The delete should now be seen as sent too.
        EXPECT_FALSE(m_root->m_replicator->HasChangesToPublish());
    }

    TEST_F(MultiplayerNetworkEntityTests, EntityReplicatorDeleteMessageResentUntilAcknowledged)
    {
        // When sending a delete message, the message should keep getting resent until it has been acknowledged.

        // Start by mocking that no packets were acknowledged.
        ON_CALL(*m_mockConnection, WasPacketAcked).WillByDefault(::testing::Return(false));

        // First "send" a creation message.
        EXPECT_TRUE(m_root->m_replicator->PrepareToGenerateUpdatePacket());
        const NetworkEntityUpdateMessage createMessage = m_root->m_replicator->GenerateUpdatePacket();
        m_root->m_replicator->RecordSentPacketId(AzNetworking::PacketId{ 1 });

        // Next, mark the entity as deleted.
        m_root->m_replicator->MarkForRemoval();

        // Generate the delete packet.
        EXPECT_TRUE(m_root->m_replicator->PrepareToGenerateUpdatePacket());
        const NetworkEntityUpdateMessage deleteMessage = m_root->m_replicator->GenerateUpdatePacket();
        m_root->m_replicator->RecordSentPacketId(AzNetworking::PacketId{ 2 });
        EXPECT_TRUE(deleteMessage.GetIsDelete());

        // The delete shouldn't be seen as acknowledged yet, so we should still need to publish changes.
        EXPECT_FALSE(m_root->m_replicator->IsDeletionAcknowledged());
        EXPECT_TRUE(m_root->m_replicator->HasChangesToPublish());

        // Generate another delete packet. It should still not be acknowledged, so we should still need to publish changes.
        EXPECT_TRUE(m_root->m_replicator->PrepareToGenerateUpdatePacket());
        const NetworkEntityUpdateMessage deleteMessage2 = m_root->m_replicator->GenerateUpdatePacket();
        m_root->m_replicator->RecordSentPacketId(AzNetworking::PacketId{ 3 });
        EXPECT_TRUE(deleteMessage2.GetIsDelete());
        EXPECT_FALSE(m_root->m_replicator->IsDeletionAcknowledged());
        EXPECT_TRUE(m_root->m_replicator->HasChangesToPublish());

        // Set the messages to acknowledged and make sure there are no longer any changes to publish.
        ON_CALL(*m_mockConnection, WasPacketAcked).WillByDefault(::testing::Return(true));
        EXPECT_TRUE(m_root->m_replicator->IsDeletionAcknowledged());
        EXPECT_FALSE(m_root->m_replicator->HasChangesToPublish());
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetworkEntityManagerRelevancy)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());

        EXPECT_TRUE(m_networkEntityManager->GetAlwaysRelevantToClientsSet().empty());
        EXPECT_TRUE(m_networkEntityManager->GetAlwaysRelevantToServersSet().empty());

        m_networkEntityManager->MarkAlwaysRelevantToClients(handle, true);
        m_networkEntityManager->MarkAlwaysRelevantToServers(handle, true);

        EXPECT_FALSE(m_networkEntityManager->GetAlwaysRelevantToClientsSet().empty());
        EXPECT_FALSE(m_networkEntityManager->GetAlwaysRelevantToServersSet().empty());

        m_networkEntityManager->MarkAlwaysRelevantToClients(handle, false);
        m_networkEntityManager->MarkAlwaysRelevantToServers(handle, false);

        EXPECT_TRUE(m_networkEntityManager->GetAlwaysRelevantToClientsSet().empty());
        EXPECT_TRUE(m_networkEntityManager->GetAlwaysRelevantToServersSet().empty());

        m_networkEntityManager->SetMigrateTimeoutTimeMs(AZ::TimeMs(0));
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetworkEntityManagerHandleExit)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());

        Multiplayer::NetEntityIdSet idSet({ m_root->m_netId });
        m_networkEntityManager->HandleEntitiesExitDomain(idSet);
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetworkEntityManagerForceAssumeAuth)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());

        AZ_TEST_START_TRACE_SUPPRESSION;
        m_networkEntityManager->ForceAssumeAuthority(handle);
        AZ_TEST_STOP_TRACE_SUPPRESSION(2);
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetworkEntityManagerDebugDraw)
    {
        m_networkEntityManager->DebugDraw();
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetBindGetSet)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());

        NetBindComponent* netBindComponent = handle.GetNetBindComponent();
        EXPECT_FALSE(netBindComponent->IsProcessingInput());
        EXPECT_FALSE(netBindComponent->IsReprocessingInput());
        EXPECT_FALSE(netBindComponent->IsNetEntityRoleServer());
        EXPECT_FALSE(netBindComponent->IsNetEntityRoleClient());
        EXPECT_EQ(netBindComponent->GetAllowEntityMigration(), EntityMigration::Enabled);
        netBindComponent->SetAllowEntityMigration(EntityMigration::Disabled);
        EXPECT_EQ(netBindComponent->GetAllowEntityMigration(), EntityMigration::Disabled);
        ConstNetworkEntityHandle handle2 = netBindComponent->GetEntityHandle();
        EXPECT_EQ(handle, handle2);
        EXPECT_TRUE(netBindComponent->GetPredictableRecord().HasChanges());
        netBindComponent->NotifyLocalChanges();
        AZ::Data::AssetId prefabAssetId(AZ::Uuid("Test"), 1);
        EXPECT_NE(prefabAssetId, netBindComponent->GetPrefabAssetId());
        netBindComponent->SetPrefabAssetId(prefabAssetId);
        EXPECT_EQ(prefabAssetId, netBindComponent->GetPrefabAssetId());
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetBindDirty)
    {
        ConstNetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());

        NetBindComponent* netBindComponent = handle.GetNetBindComponent();
        netBindComponent->MarkDirty();
        m_networkEntityManager->NotifyEntitiesDirtied();
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetBindPropertyValidateAuthority)
    {
        AZStd::unique_ptr<EntityInfo> testEntity = AZStd::make_unique<EntityInfo>(3, "root", NetEntityId{ 3 }, EntityInfo::Role::None);
        PopulateNetworkEntity(*testEntity);
        SetupEntity(testEntity->m_entity, testEntity->m_netId, NetEntityRole::Authority);
        testEntity->m_entity->Activate();
        ConstNetworkEntityHandle handle(testEntity->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());
        NetBindComponent* netBindComponent = handle.GetNetBindComponent();

        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Server));
        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous));
        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Client));
        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority));

        const bool predictable(true);
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Server, predictable));
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous, predictable));
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Client, predictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority, predictable));

        const bool notPredictable(false);
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Server, notPredictable));
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous, notPredictable));
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Client, notPredictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority, notPredictable));
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetBindPropertyValidateServer)
    {
        AZStd::unique_ptr<EntityInfo> testEntity = AZStd::make_unique<EntityInfo>(4, "root", NetEntityId{ 4 }, EntityInfo::Role::None);
        PopulateNetworkEntity(*testEntity);
        SetupEntity(testEntity->m_entity, testEntity->m_netId, NetEntityRole::Server);
        testEntity->m_entity->Activate();
        ConstNetworkEntityHandle handle(testEntity->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());
        NetBindComponent* netBindComponent = handle.GetNetBindComponent();

        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Server));
        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous));
        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Client));
        EXPECT_FALSE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority));

        const bool predictable(true);
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Server, predictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous, predictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Client, predictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority, predictable));

        const bool notPredictable(false);
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Server, notPredictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous, notPredictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Client, notPredictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority, notPredictable));
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetBindPropertyValidateAutonomous)
    {
        AZStd::unique_ptr<EntityInfo> testEntity = AZStd::make_unique<EntityInfo>(5, "root", NetEntityId{ 5 }, EntityInfo::Role::None);
        PopulateNetworkEntity(*testEntity);
        SetupEntity(testEntity->m_entity, testEntity->m_netId, NetEntityRole::Autonomous);
        testEntity->m_entity->Activate();
        ConstNetworkEntityHandle handle(testEntity->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());
        NetBindComponent* netBindComponent = handle.GetNetBindComponent();

        EXPECT_FALSE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Server));
        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous));
        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Client));
        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority));

        const bool predictable(true);
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Server, predictable));
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous, predictable));
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Client, predictable));
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority, predictable));

        const bool notPredictable(false);
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Server, notPredictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous, notPredictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Client, notPredictable));
        EXPECT_TRUE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority, notPredictable));
    }

    TEST_F(MultiplayerNetworkEntityTests, TestNetBindPropertyValidateClient)
    {
        AZStd::unique_ptr<EntityInfo> testEntity = AZStd::make_unique<EntityInfo>(6, "root", NetEntityId{ 6 }, EntityInfo::Role::None);
        PopulateNetworkEntity(*testEntity);
        SetupEntity(testEntity->m_entity, testEntity->m_netId, NetEntityRole::Client);
        testEntity->m_entity->Activate();
        ConstNetworkEntityHandle handle(testEntity->m_entity.get(), m_networkEntityManager->GetNetworkEntityTracker());
        NetBindComponent* netBindComponent = handle.GetNetBindComponent();

        EXPECT_FALSE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Server));
        EXPECT_FALSE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous));
        EXPECT_TRUE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Authority, NetEntityRole::Client));
        EXPECT_FALSE(netBindComponent->ValidatePropertyRead("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority));

        const bool predictable(true);
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Server, predictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous, predictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Client, predictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority, predictable));

        const bool notPredictable(false);
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Server, notPredictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Autonomous, notPredictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Authority, NetEntityRole::Client, notPredictable));
        EXPECT_FALSE(netBindComponent->ValidatePropertyWrite("TestProperty", NetEntityRole::Autonomous, NetEntityRole::Authority, notPredictable));
    }
} // namespace Multiplayer
