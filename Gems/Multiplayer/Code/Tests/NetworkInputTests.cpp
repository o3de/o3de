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
#include <Multiplayer/NetworkInput/NetworkInput.h>
#include <Multiplayer/NetworkEntity/EntityReplication/EntityReplicator.h>
#include <Multiplayer/NetworkInput/NetworkInputArray.h>
#include <Multiplayer/NetworkInput/NetworkInputHistory.h>
#include <Multiplayer/NetworkInput/NetworkInputMigrationVector.h>

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    class NetworkInputTests : public HierarchyTests
    {
    public:
        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_root = AZStd::make_unique<EntityInfo>(1, "root", NetEntityId{ 1 }, EntityInfo::Role::Root);

            PopulateNetworkEntity(*m_root);
            SetupEntity(m_root->m_entity, m_root->m_netId, NetEntityRole::Authority);

            // Create an entity replicator for the root entity
            const NetworkEntityHandle rootHandle(m_root->m_entity.get(), m_networkEntityTracker.get());
            m_root->m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Client, rootHandle);
            m_root->m_replicator->Initialize(rootHandle);

            m_root->m_entity->Activate();
        }

        void TearDown() override
        {
            m_root.reset();

            HierarchyTests::TearDown();
        }

        void PopulateNetworkEntity(const EntityInfo& entityInfo)
        {
            entityInfo.m_entity->CreateComponent<AzFramework::TransformComponent>();
            entityInfo.m_entity->CreateComponent<NetBindComponent>();
            entityInfo.m_entity->CreateComponent<NetworkTransformComponent>();
        }

        AZStd::unique_ptr<EntityInfo> m_root;
    };

    constexpr float BLEND_FACTOR_SCALE = 1.1f;
    constexpr uint32_t TIME_SCALE = 10;

    TEST_F(NetworkInputTests, NetworkInputMembers)
    {
        const NetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityTracker.get());
        NetworkInputArray inArray = NetworkInputArray(handle);

        for (uint32_t i = 0; i < NetworkInputArray::MaxElements; ++i)
        {
            inArray[i].SetClientInputId(ClientInputId(i));
            inArray[i].SetHostFrameId(HostFrameId(i));
            inArray[i].SetHostBlendFactor(i * BLEND_FACTOR_SCALE);
            inArray[i].SetHostTimeMs(AZ::TimeMs(i * TIME_SCALE));

            EXPECT_EQ(inArray[i].GetClientInputId(), ClientInputId(i));
            EXPECT_EQ(inArray[i].GetHostFrameId(), HostFrameId(i));
            EXPECT_NEAR(inArray[i].GetHostBlendFactor(), i * BLEND_FACTOR_SCALE, 0.001f);
            EXPECT_EQ(inArray[i].GetHostTimeMs(), AZ::TimeMs(i * TIME_SCALE));
        }

        for (uint32_t i = 0; i < NetworkInputArray::MaxElements; ++i)
        {
            ClientInputId& cid = inArray[i].ModifyClientInputId();
            cid = ClientInputId(i * 2);
            HostFrameId& hid = inArray[i].ModifyHostFrameId();
            hid = HostFrameId(i * 2);
            AZ::TimeMs& time = inArray[i].ModifyHostTimeMs();
            time = AZ::TimeMs(i * 2 * TIME_SCALE);

            EXPECT_EQ(inArray[i].GetClientInputId(), cid);
            EXPECT_EQ(inArray[i].GetHostFrameId(), hid);
            EXPECT_EQ(inArray[i].GetHostTimeMs(), time);
        }
    }

    TEST_F(NetworkInputTests, NetworkInputArraySerialization)
    {
        const NetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityTracker.get());
        NetworkInputArray inArray = NetworkInputArray(handle);

        for (uint32_t i = 0; i < NetworkInputArray::MaxElements; ++i)
        {
            inArray[i].SetClientInputId(ClientInputId(i));
            inArray[i].SetHostFrameId(HostFrameId(i));
            inArray[i].SetHostBlendFactor(i * BLEND_FACTOR_SCALE);
            inArray[i].SetHostTimeMs(AZ::TimeMs(i * TIME_SCALE));
        }

        AZStd::array<uint8_t, 1024> buffer;
        AzNetworking::NetworkInputSerializer inSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        // Always serialize the full first element
        EXPECT_TRUE(inArray.Serialize(inSerializer));

        NetworkInputArray outArray;
        AzNetworking::NetworkOutputSerializer outSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        EXPECT_TRUE(outArray.Serialize(outSerializer));

        for (uint32_t i = 0; i > NetworkInputArray::MaxElements; ++i)
        {
            EXPECT_EQ(inArray[i].GetClientInputId(), outArray[i].GetClientInputId());
            EXPECT_EQ(inArray[i].GetHostFrameId(), outArray[i].GetHostFrameId());
            EXPECT_NEAR(inArray[i].GetHostBlendFactor(), outArray[i].GetHostBlendFactor(),0.001f);
            EXPECT_EQ(inArray[i].GetHostTimeMs(), outArray[i].GetHostTimeMs());
        }
    }

    TEST_F(NetworkInputTests, NetworkInputHistory)
    {
        const NetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityTracker.get());
        NetworkInputArray inArray = NetworkInputArray(handle);
        NetworkInputHistory inHistory = NetworkInputHistory();

        for (uint32_t i = 0; i < NetworkInputArray::MaxElements; ++i)
        {
            inArray[i].SetClientInputId(ClientInputId(i));
            inArray[i].SetHostFrameId(HostFrameId(i));
            inArray[i].SetHostBlendFactor(i * BLEND_FACTOR_SCALE);
            inArray[i].SetHostTimeMs(AZ::TimeMs(i * TIME_SCALE));

            inHistory.PushBack(inArray[i]);
        }

        EXPECT_EQ(inHistory.Size(), NetworkInputArray::MaxElements);

        for (uint32_t i = 0; i < NetworkInputArray::MaxElements; ++i)
        {
            NetworkInput input = inHistory.Front();
            EXPECT_EQ(input.GetClientInputId(), ClientInputId(i));
            EXPECT_EQ(input.GetHostFrameId(), HostFrameId(i));
            EXPECT_NEAR(input.GetHostBlendFactor(), i * BLEND_FACTOR_SCALE, 0.001f);
            EXPECT_EQ(input.GetHostTimeMs(), AZ::TimeMs(i * TIME_SCALE));
            inHistory.PopFront();
        }

        EXPECT_EQ(inHistory.Size(), 0);
    }

    TEST_F(NetworkInputTests, NetworkInputMigrationVector)
    {
        const NetworkEntityHandle handle(m_root->m_entity.get(), m_networkEntityTracker.get());
        NetworkInputArray inArray = NetworkInputArray(handle);
        NetworkInputMigrationVector inVector = NetworkInputMigrationVector();

        for (uint32_t i = 0; i < NetworkInputArray::MaxElements; ++i)
        {
            inArray[i].SetClientInputId(ClientInputId(i));
            inArray[i].SetHostFrameId(HostFrameId(i));
            inArray[i].SetHostBlendFactor(i * BLEND_FACTOR_SCALE);
            inArray[i].SetHostTimeMs(AZ::TimeMs(i * TIME_SCALE));

            inVector.PushBack(inArray[i]);
        }

        EXPECT_EQ(inVector.GetSize(), NetworkInputArray::MaxElements);

        AZStd::array<uint8_t, 1024> buffer;
        AzNetworking::NetworkInputSerializer inSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        // Always serialize the full first element
        EXPECT_TRUE(inVector.Serialize(inSerializer));

        NetworkInputArray outArray;
        AzNetworking::NetworkOutputSerializer outSerializer(buffer.data(), static_cast<uint32_t>(buffer.size()));

        NetworkInputMigrationVector outVector = NetworkInputMigrationVector();
        EXPECT_TRUE(outVector.Serialize(outSerializer));

        for (uint32_t i = 0; i > NetworkInputArray::MaxElements; ++i)
        {
            EXPECT_EQ(inVector[i].GetClientInputId(), outVector[i].GetClientInputId());
            EXPECT_EQ(inVector[i].GetHostFrameId(), outVector[i].GetHostFrameId());
            EXPECT_NEAR(inVector[i].GetHostBlendFactor(), outVector[i].GetHostBlendFactor(),0.001f);
            EXPECT_EQ(inVector[i].GetHostTimeMs(), outVector[i].GetHostTimeMs());
        }
        EXPECT_EQ(inVector.GetSize(), outVector.GetSize());
    }
}
