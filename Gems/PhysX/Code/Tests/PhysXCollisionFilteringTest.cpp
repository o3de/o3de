/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PhysXTestFixtures.h"
#include "PhysXTestUtil.h"
#include "PhysXTestCommon.h"
#include <AzFramework/Physics/CollisionBus.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Components/TransformComponent.h>

#include <BoxColliderComponent.h>
#include <RigidBodyComponent.h>

namespace PhysX
{
    class PhysXCollisionFilteringTest
        : public PhysXDefaultWorldTest
    {
    protected:
        const AZStd::string DefaultLayer = "Default";
        const AZStd::string LayerA = "LayerA";
        const AZStd::string LayerB = "LayerB";
        const AZStd::string GroupA = "GroupA";
        const AZStd::string GroupB = "GroupB";
        const AZStd::string GroupNone = "None";
        const AZStd::string LeftCollider = "LeftCollider";
        const AZStd::string RightCollider = "RightCollider";
        const int FramesToUpdate = 25;

        void SetUp() override 
        {
            PhysXDefaultWorldTest::SetUp();

            // Test layers
            AZStd::vector<AZStd::string> TestCollisionLayers =
            {
                DefaultLayer,
                LayerA,
                LayerB
            };

            // Test groups
            AZStd::vector<AZStd::pair<AZStd::string, AZStd::vector<AZStd::string>>> TestCollisionGroups =
            {
                {GroupA, {DefaultLayer, LayerA}}, // "A" only collides with 'LayerA'
                {GroupB, {DefaultLayer, LayerB}}, // "B" only collides with 'LayerB'
            };

            // Setup test collision layers to use in the tests
            for (int i = 0; i < TestCollisionLayers.size(); ++i)
            {
                Physics::CollisionRequestBus::Broadcast(&Physics::CollisionRequests::SetCollisionLayerName, i, TestCollisionLayers[i]);
            }

            // Setup test collision groups to use in the tests
            for (int i = 0; i < TestCollisionGroups.size(); ++i)
            {
                const auto& groupName = TestCollisionGroups[i].first;
                auto group = CreateGroupFromLayerNames(TestCollisionGroups[i].second);
                Physics::CollisionRequestBus::Broadcast(&Physics::CollisionRequests::CreateCollisionGroup, groupName, group);
            }
        }

        AzPhysics::CollisionGroup CreateGroupFromLayerNames(const AZStd::vector<AZStd::string>& layerNames)
        {
            AzPhysics::CollisionGroup group = AzPhysics::CollisionGroup::None;
            for (const auto& layerName : layerNames)
            {
                AzPhysics::CollisionLayer layer(layerName);
                group.SetLayer(layer, true);
            }
            return group;
        }
    };

    EntityPtr CreateDynamicMultiCollider(const AZStd::string& leftColliderTag, const AZStd::string& rightColliderTag)
    {
        auto entityPtr = AZStd::make_shared<AZ::Entity>("MultiCollider");
        entityPtr->CreateComponent<AzFramework::TransformComponent>();

        auto leftBoxConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>();
        auto leftColliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        leftColliderConfig->m_position = AZ::Vector3(-1.0f, 0.0f, 0.0f);
        leftColliderConfig->m_tag = leftColliderTag;
        
        auto rightBoxConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>();
        auto rightColliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        rightColliderConfig->m_position = AZ::Vector3(1.0f, 0.0f, 0.0f);
        rightColliderConfig->m_tag = rightColliderTag;

        auto leftCollider = entityPtr->CreateComponent<BoxColliderComponent>();
        leftCollider->SetShapeConfigurationList({ AZStd::make_pair(leftColliderConfig, leftBoxConfig) });

        auto rightCollider = entityPtr->CreateComponent<BoxColliderComponent>();
        rightCollider->SetShapeConfigurationList({ AZStd::make_pair(rightColliderConfig, rightBoxConfig) });

        entityPtr->CreateComponent<RigidBodyComponent>();

        entityPtr->Init();
        entityPtr->Activate();
        return AZStd::move(entityPtr);
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetColliderLayerOnStaticObject)
    {
        auto ground = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(10.0f, 10.0f, 0.5f));
        auto fallingBox = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.0f), AZ::Vector3(1.0f, 1.0f, 1.0f));

        // GroupB does not collider with LayerA, no collision expected.
        TestUtils::SetCollisionLayer(ground, LayerA);
        TestUtils::SetCollisionGroup(fallingBox, GroupB);

        CollisionCallbacksListener collisionEvents(ground->GetId());

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetColliderLayerOnDynamicObject)
    {
        auto ground = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(10.0f, 10.0f, 0.5f));
        auto fallingBox = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.0f), AZ::Vector3(1.0f, 1.0f, 1.0f));

        TestUtils::SetCollisionGroup(ground, GroupB);
        TestUtils::SetCollisionLayer(fallingBox, LayerA);

        CollisionCallbacksListener collisionEvents(ground->GetId());

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetCollidesWithGroupOnDynamicObject)
    {
        auto ground = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(10.0f, 10.0f, 0.5f));
        auto fallingBox = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.0f), AZ::Vector3(1.0f, 1.0f, 1.0f));

        TestUtils::SetCollisionGroup(fallingBox, GroupNone);

        CollisionCallbacksListener collisionEvents(ground->GetId());

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetCollidesWithGroupOnStaticObject)
    {
        auto ground = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(10.0f, 10.0f, 0.5f));
        auto fallingBox = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.0f), AZ::Vector3(1.0f, 1.0f, 1.0f));

        TestUtils::SetCollisionGroup(ground, GroupNone);

        CollisionCallbacksListener collisionEvents(ground->GetId());

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetColliderLayerOnFilteredCollider)
    {
        auto leftStatic = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3(-1.0f, 0.0f, -1.5f), AZ::Vector3(1.0f, 1.0f, 1.0f));
        auto rightStatic = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3(1.0f, 0.0f, -1.5f), AZ::Vector3(1.0f, 1.0f, 1.0f));
        auto fallingMultiCollider = CreateDynamicMultiCollider(LeftCollider, RightCollider);

        TestUtils::SetCollisionGroup(leftStatic, GroupA);
        TestUtils::SetCollisionGroup(rightStatic, GroupB);

        TestUtils::SetCollisionLayer(fallingMultiCollider, LayerB, LeftCollider);
        TestUtils::SetCollisionLayer(fallingMultiCollider, LayerA, RightCollider);

        CollisionCallbacksListener collisionEvents(fallingMultiCollider->GetId());

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetCollidesWithGroupOnFilteredCollider)
    {
        auto leftStatic = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3(-1.0f, 0.0f, -1.5f), AZ::Vector3(1.0f, 1.0f, 1.0f));
        auto rightStatic = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3(1.0f, 0.0f, -1.5f), AZ::Vector3(1.0f, 1.0f, 1.0f));
        auto fallingMultiCollider = CreateDynamicMultiCollider(LeftCollider, RightCollider);

        TestUtils::SetCollisionLayer(leftStatic, LayerA);
        TestUtils::SetCollisionLayer(rightStatic, LayerB);

        TestUtils::SetCollisionGroup(fallingMultiCollider, GroupB, LeftCollider);
        TestUtils::SetCollisionGroup(fallingMultiCollider, GroupA, RightCollider);

        CollisionCallbacksListener collisionEvents(fallingMultiCollider->GetId());

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetCollidesWithLayer)
    {
        auto ground = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(1.0f, 1.0f, 1.0f));
        auto fallingBox = TestUtils::CreateBoxEntity(m_testSceneHandle, AZ::Vector3(0.0f, 0.0f, 1.0f), AZ::Vector3(1.0f, 1.0f, 1.0f));

        TestUtils::SetCollisionLayer(fallingBox, LayerA);
        TestUtils::ToggleCollisionLayer(ground, LayerA, false);

        CollisionCallbacksListener collisionEvents(fallingBox->GetId());

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestSetCollidesWithLayerFiltered)
    {
        auto leftStatic = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3(-1.0f, 0.0f, -1.5f), AZ::Vector3(1.0f, 1.0f, 1.0f));
        auto rightStatic = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3(1.0f, 0.0f, -1.5f), AZ::Vector3(1.0f, 1.0f, 1.0f));
        auto fallingMultiCollider = CreateDynamicMultiCollider(LeftCollider, RightCollider);

        TestUtils::SetCollisionLayer(leftStatic, LayerA);
        TestUtils::SetCollisionLayer(rightStatic, LayerB);

        TestUtils::ToggleCollisionLayer(fallingMultiCollider, LayerA, false, LeftCollider);
        TestUtils::ToggleCollisionLayer(fallingMultiCollider, LayerB, false, RightCollider);

        CollisionCallbacksListener collisionEvents(fallingMultiCollider->GetId());

        TestUtils::UpdateScene(m_defaultScene, AzPhysics::SystemConfiguration::DefaultFixedTimestep, FramesToUpdate);

        EXPECT_TRUE(collisionEvents.m_beginCollisions.empty());
    }

    TEST_F(PhysXCollisionFilteringTest, TestGetCollisionLayerName)
    {
        auto staticBody = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(1.0f, 1.0f, 1.0f));

        TestUtils::SetCollisionLayer(staticBody, LayerA);

        AZStd::string collisionLayerName;
        Physics::CollisionFilteringRequestBus::EventResult(collisionLayerName, staticBody->GetId(), &Physics::CollisionFilteringRequests::GetCollisionLayerName);

        EXPECT_EQ(collisionLayerName, LayerA);
    }

    TEST_F(PhysXCollisionFilteringTest, TestGetCollisionGroupName)
    {
        auto staticBody = TestUtils::CreateStaticBoxEntity(m_testSceneHandle, AZ::Vector3::CreateZero(), AZ::Vector3(1.0f, 1.0f, 1.0f));

        TestUtils::SetCollisionGroup(staticBody, GroupA);

        AZStd::string collisionGroupName;
        Physics::CollisionFilteringRequestBus::EventResult(collisionGroupName, staticBody->GetId(), &Physics::CollisionFilteringRequests::GetCollisionGroupName);

        EXPECT_EQ(collisionGroupName, GroupA);
    }
}
