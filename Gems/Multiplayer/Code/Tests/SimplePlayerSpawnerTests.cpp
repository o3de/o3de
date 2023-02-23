/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <CommonBenchmarkSetup.h>
#include <MockInterfaces.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/Components/SimplePlayerSpawnerComponent.h>


namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    class SimplePlayerSpawnerTests
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            m_componentApplicationRequests = AZStd::make_unique<BenchmarkComponentApplicationRequests>();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(m_componentApplicationRequests.get());

            // register components involved in testing
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_transformDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformDescriptor->Reflect(m_serializeContext.get());

            m_simplePlayerSpawnerComponentDescriptor.reset(SimplePlayerSpawnerComponent::CreateDescriptor());
            m_simplePlayerSpawnerComponentDescriptor->Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(m_componentApplicationRequests.get());
            m_componentApplicationRequests.reset();

            m_simplePlayerSpawnerComponentDescriptor.reset();
            m_transformDescriptor.reset();
            m_serializeContext.reset();
        }

        void CreateSimplePlayerSpawner(AZ::Entity& entity, AZStd::vector<AZ::EntityId> spawnPoints)
        {
            auto spawner = entity.CreateComponent<SimplePlayerSpawnerComponent>();
            spawner->GetSpawnPoints() = spawnPoints;
            entity.Init();
            entity.Activate();
        }

        void CreateSpawnPoint(AZ::Entity& spawnPointEntity, const AZ::Vector3& position)
        {
            auto transform = spawnPointEntity.CreateComponent<AzFramework::TransformComponent>();
            transform->SetWorldTM(AZ::Transform::CreateTranslation(position));

            spawnPointEntity.Init();
            spawnPointEntity.Activate();
        }

        AZStd::unique_ptr<BenchmarkComponentApplicationRequests> m_componentApplicationRequests;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_simplePlayerSpawnerComponentDescriptor;
    };

    TEST_F(SimplePlayerSpawnerTests, RoundRobinWrapAround)
    {
        AZ::Entity spawnPoint1(AZ::EntityId(1));
        AZ::Entity spawnPoint2(AZ::EntityId(2));
        AZ::Entity spawnPoint3(AZ::EntityId(3));

        CreateSpawnPoint(spawnPoint1, AZ::Vector3(1.0f, 0, 0));
        CreateSpawnPoint(spawnPoint2, AZ::Vector3(2.0f, 0, 0));
        CreateSpawnPoint(spawnPoint3, AZ::Vector3(3.0f, 0, 0));

        AZ::Entity simplePlayerSpawnerEntity;
        CreateSimplePlayerSpawner(simplePlayerSpawnerEntity, { spawnPoint1.GetId(), spawnPoint2.GetId(), spawnPoint3.GetId() });

        uint32_t numSpawnPoints;
        SimplePlayerSpawnerRequestBus::BroadcastResult(numSpawnPoints, &SimplePlayerSpawnerRequests::GetSpawnPointCount);
        EXPECT_EQ(numSpawnPoints, 3);

        AZ::Transform nextSpawnPoint;
        SimplePlayerSpawnerRequestBus::BroadcastResult(nextSpawnPoint, &SimplePlayerSpawnerRequests::RoundRobinNextSpawnPoint);
        EXPECT_EQ(nextSpawnPoint.GetTranslation().GetX(), 1);

        SimplePlayerSpawnerRequestBus::BroadcastResult(nextSpawnPoint, &SimplePlayerSpawnerRequests::RoundRobinNextSpawnPoint);
        EXPECT_EQ(nextSpawnPoint.GetTranslation().GetX(), 2);

        SimplePlayerSpawnerRequestBus::BroadcastResult(nextSpawnPoint, &SimplePlayerSpawnerRequests::RoundRobinNextSpawnPoint);
        EXPECT_EQ(nextSpawnPoint.GetTranslation().GetX(), 3);

        // Ran through each spawn point; make sure the spawner goes back to spawn point #1
        SimplePlayerSpawnerRequestBus::BroadcastResult(nextSpawnPoint, &SimplePlayerSpawnerRequests::RoundRobinNextSpawnPoint);
        EXPECT_EQ(nextSpawnPoint.GetTranslation().GetX(), 1);

        AZ::Outcome<uint32_t, AZStd::string> spawnPointIndexOutcome;
        SimplePlayerSpawnerRequestBus::BroadcastResult(spawnPointIndexOutcome, &SimplePlayerSpawnerRequests::GetNextSpawnPointIndex);
        EXPECT_TRUE(spawnPointIndexOutcome.IsSuccess());
        EXPECT_EQ(spawnPointIndexOutcome.GetValue(), 1); // Spawn point #2 (aka index 1)
    }

    TEST_F(SimplePlayerSpawnerTests, NoSpawnPoints)
    {
        AZ::Entity simplePlayerSpawnerEntity;
        CreateSimplePlayerSpawner(simplePlayerSpawnerEntity, {});

        uint32_t numSpawnPoints;
        SimplePlayerSpawnerRequestBus::BroadcastResult(numSpawnPoints, &SimplePlayerSpawnerRequests::GetSpawnPointCount);
        EXPECT_EQ(numSpawnPoints, 0);

        // No spawn points. The next spawn position will be the world origin.
        AZ::Transform nextSpawnPoint;
        SimplePlayerSpawnerRequestBus::BroadcastResult(nextSpawnPoint, &SimplePlayerSpawnerRequests::GetNextSpawnPoint);
        EXPECT_EQ(nextSpawnPoint, AZ::Transform::CreateIdentity());

        // No spawn points. Getting the next spawn point index should return a failure outcome.
        AZ::Outcome<uint32_t, AZStd::string> spawnPointIndexOutcome;
        SimplePlayerSpawnerRequestBus::BroadcastResult(spawnPointIndexOutcome, &SimplePlayerSpawnerRequests::GetNextSpawnPointIndex);
        EXPECT_FALSE(spawnPointIndexOutcome.IsSuccess());
    }
} // namespace Multiplayer
