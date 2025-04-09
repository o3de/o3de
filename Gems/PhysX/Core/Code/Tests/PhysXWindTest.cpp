/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Component/TickBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/UnitTest/Helpers.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/WindBus.h>
#include <AzFramework/Physics/World.h>

#include <AzTest/AzTest.h>

#include <LmbrCentral/Scripting/TagComponentBus.h>

#include <BoxColliderComponent.h>
#include <ForceRegionComponent.h>

#include "PhysXTestFixtures.h"

namespace PhysX
{
    class PhysXWindTest
        : public PhysXDefaultWorldTest
    {
    public:
        AZ::Vector3 GetGlobalWind() const
        {
            return AZ::Interface<Physics::WindRequests>::Get()->GetGlobalWind();
        }

        AZ::Vector3 GetWind(const AZ::Vector3& position) const
        {
            return AZ::Interface<Physics::WindRequests>::Get()->GetWind(position);
        }

        AZ::Vector3 GetWind(const AZ::Aabb& aabb) const
        {
            return AZ::Interface<Physics::WindRequests>::Get()->GetWind(aabb);
        }
    };

    class MockWindNotificationsBusHandler
        : public Physics::WindNotificationsBus::Handler
    {
    public:
        MockWindNotificationsBusHandler()
        {
            Physics::WindNotificationsBus::Handler::BusConnect();
        }

        ~MockWindNotificationsBusHandler()
        {
            Physics::WindNotificationsBus::Handler::BusDisconnect();
        }

        MOCK_METHOD1(OnWindChanged, void(const AZ::Aabb&));
    };

    enum class WindType : bool
    {
        Local,
        Global
    };

    AZStd::unique_ptr<AZ::Entity> AddWindRegion(
        const AZ::Aabb& aabb, const AZ::Vector3& windDirection, float windMagnitude, WindType windType)
    {
        auto forceRegionEntity = AZStd::make_unique<AZ::Entity>("WindForceRegion");

        AZ::TransformConfig transformConfig;
        transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(aabb.GetCenter());
        forceRegionEntity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);

        auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfiguration->m_isTrigger = true;

        auto shapeConfiguration = AZStd::make_shared<BoxColliderComponent::Configuration>();
        shapeConfiguration->m_dimensions = AZ::Vector3(aabb.GetWidth(), aabb.GetHeight(), aabb.GetDepth());

        auto colliderComponent = forceRegionEntity->CreateComponent<BoxColliderComponent>();
        colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfiguration, shapeConfiguration) });

        forceRegionEntity->CreateComponent<ForceRegionComponent>();

        forceRegionEntity->CreateComponent("{0F16A377-EAA0-47D2-8472-9EAAA680B169}"); //TagComponent

        forceRegionEntity->Init();
        forceRegionEntity->Activate();

        PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
            , &PhysX::ForceRegionRequests::AddForceWorldSpace
            , windDirection
            , windMagnitude);

        AZ::Crc32 windTag;

        switch (windType)
        {
        case WindType::Global:
        {
            windTag = AZ::Crc32("global_wind");
            break;
        }
        case WindType::Local:
        {
            windTag = AZ::Crc32("wind");
            break;
        }
        };

        LmbrCentral::TagComponentRequestBus::Event(forceRegionEntity->GetId()
            , &LmbrCentral::TagComponentRequests::AddTag
            , windTag);

        return forceRegionEntity;
    }

    AZStd::unique_ptr<AZ::Entity> AddWindRegion(
        const AZ::Vector3& position, const AZ::Vector3& windDirection, float windMagnitude, WindType windType)
    {
        const AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(position, 0.5f);
        return AddWindRegion(aabb, windDirection, windMagnitude, windType);
    }

    AZ::Vector3 GetWindValue(const AZ::Vector3& windDirection, float windMagnitude)
    {
        if (!windDirection.IsZero())
        {
            return windDirection.GetNormalized() * windMagnitude;
        }

        return AZ::Vector3::CreateZero();
    }

    TEST_F(PhysXWindTest, GlobalWind_SingleForceRegion_WindBusReturnsCorrectValue)
    {
        const AZ::Vector3 position = AZ::Vector3::CreateZero();
        const AZ::Vector3 windDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitude = 1.0f;
        auto forceRegionEntity = AddWindRegion(position, windDirection, windMagnitude, WindType::Global);

        const AZ::Vector3 globalWindValue = GetGlobalWind();
        const AZ::Vector3 expectedWindValue = GetWindValue(windDirection, windMagnitude);

        EXPECT_THAT(globalWindValue, UnitTest::IsClose(expectedWindValue));
    }

    TEST_F(PhysXWindTest, GlobalWind_TwoForceRegions_WindBusReturnsCorrectValue)
    {
        // First entity
        const AZ::Vector3 positionA = AZ::Vector3(100.0f, 0.0f, 0.0f);
        const AZ::Vector3 windDirectionA = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitudeA = 2.0f;
        auto forceRegionEntityA = AddWindRegion(positionA, windDirectionA, windMagnitudeA, WindType::Global);

        // Seconds entity
        const AZ::Vector3 positionB = AZ::Vector3(0.0f, 100.0f, 0.0f);
        const AZ::Vector3 windDirectionB = AZ::Vector3(0.0f, 1.0f, 0.0f);
        const float windMagnitudeB = 3.0f;
        auto forceRegionEntityB = AddWindRegion(positionB, windDirectionB, windMagnitudeB, WindType::Global);

        const AZ::Vector3 globalWindValue = GetGlobalWind();

        // We expect result to be combined value from both entities
        const AZ::Vector3 expectedWindValue =
            GetWindValue(windDirectionA, windMagnitudeA) + GetWindValue(windDirectionB, windMagnitudeB);

        EXPECT_THAT(globalWindValue, UnitTest::IsClose(expectedWindValue));
    }

    TEST_F(PhysXWindTest, GlobalWind_SingleForceRegionDeactivated_WindBusReturnsZero)
    {
        const AZ::Vector3 position = AZ::Vector3::CreateZero();
        const AZ::Vector3 windDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitude = 1.0f;
        auto forceRegionEntity = AddWindRegion(position, windDirection, windMagnitude, WindType::Global);

        // Deactivate entity, it is not expected to contribute to wind values anymore
        forceRegionEntity->Deactivate();

        const AZ::Vector3 globalWindValue = GetGlobalWind();

        EXPECT_TRUE(globalWindValue.IsZero());
    }

    TEST_F(PhysXWindTest, LocalWind_SingleForceRegion_WindBusReturnsCorrectValueAtPosition)
    {
        const AZ::Vector3 position = AZ::Vector3::CreateZero();
        const AZ::Vector3 windDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitude = 3.0f;
        auto forceRegionEntity = AddWindRegion(position, windDirection, windMagnitude, WindType::Local);

        const AZ::Vector3 localWindValue = GetWind(position);
        const AZ::Vector3 expectedWindValue = GetWindValue(windDirection, windMagnitude);

        EXPECT_THAT(localWindValue, UnitTest::IsClose(expectedWindValue));
    }

    TEST_F(PhysXWindTest, LocalWind_SingleForceRegion_WindBusReturnsCorrectValueForPositionInsideWindVolume)
    {
        const AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(
            AZ::Vector3::CreateZero(),
            10.0f
        );
        const AZ::Vector3 windDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitude = 10.0f;
        auto forceRegionEntity = AddWindRegion(aabb, windDirection, windMagnitude, WindType::Local);

        const AZ::Vector3 position = AZ::Vector3(5.0f, 5.0f, 0.0f);
        const AZ::Vector3 localWindValue = GetWind(position);
        const AZ::Vector3 expectedWindValue = GetWindValue(windDirection, windMagnitude);

        EXPECT_THAT(localWindValue, UnitTest::IsClose(expectedWindValue));
    }

    TEST_F(PhysXWindTest, LocalWind_SingleForceRegion_WindBusReturnsZeroForPositionOutsideWindVolume)
    {
        const AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(
            AZ::Vector3::CreateZero(),
            10.0f
        );
        const AZ::Vector3 windDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitude = 10.0f;
        auto forceRegionEntity = AddWindRegion(aabb, windDirection, windMagnitude, WindType::Local);

        // Using position outside of wind entity bounding box
        const AZ::Vector3 position = AZ::Vector3(100.0f, 0.0f, 0.0f);
        const AZ::Vector3 localWindValue = GetWind(position);

        EXPECT_TRUE(localWindValue.IsZero());
    }

    TEST_F(PhysXWindTest, LocalWind_SingleForceRegion_WindBusReturnsCorrectValueForAabbOverlappingWindVolume)
    {
        const AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(
            AZ::Vector3::CreateZero(),
            10.0f
        );
        const AZ::Vector3 windDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitude = 10.0f;
        auto forceRegionEntity = AddWindRegion(aabb, windDirection, windMagnitude, WindType::Local);

        // Using bounding box that overlaps with wind one
        const AZ::Aabb testAabb = AZ::Aabb::CreateCenterRadius(
            AZ::Vector3(1.0f, 2.0f, 3.0f),
            15.0f
        );

        const AZ::Vector3 localWindValue = GetWind(testAabb);
        const AZ::Vector3 expectedWindValue = GetWindValue(windDirection, windMagnitude);

        EXPECT_THAT(localWindValue, UnitTest::IsClose(expectedWindValue));
    }

    TEST_F(PhysXWindTest, LocalWind_SingleForceRegion_WindBusReturnsZeroForAabbNotOverlappingWindVolume)
    {
        const AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(
            AZ::Vector3::CreateZero(),
            10.0f
        );
        const AZ::Vector3 windDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitude = 10.0f;
        auto forceRegionEntity = AddWindRegion(aabb, windDirection, windMagnitude, WindType::Local);

        const AZ::Aabb testAabb = AZ::Aabb::CreateCenterRadius(
            AZ::Vector3(100.0f, 200.0f, 300.0f),
            15.0f
        );

        const AZ::Vector3 localWindValue = GetWind(testAabb);

        EXPECT_TRUE(localWindValue.IsZero());
    }

    TEST_F(PhysXWindTest, LocalWind_TwoForceRegions_WindBusReturnsCombinedValueForPositionInsideBothWindVolumes)
    {
        // First entity
        const AZ::Aabb aabbA = AZ::Aabb::CreateCenterRadius(
            AZ::Vector3::CreateZero(),
            10.0f
        );
        const AZ::Vector3 windDirectionA = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitudeA = 10.0f;
        auto forceRegionEntityA = AddWindRegion(aabbA, windDirectionA, windMagnitudeA, WindType::Local);

        // Second entity
        const AZ::Aabb aabbB = AZ::Aabb::CreateCenterRadius(
            AZ::Vector3(5.0f, 0.0f, 0.0f),
            10.0f
        );
        const AZ::Vector3 windDirectionB = AZ::Vector3(0.0f, 5.0f, 0.0f);
        const float windMagnitudeB = 20.0f;
        auto forceRegionEntityB = AddWindRegion(aabbB, windDirectionB, windMagnitudeB, WindType::Local);

        // Sampling position that is inside bounding boxes of both entities should
        // give back combined wind value
        const AZ::Vector3 position = AZ::Vector3(5.0f, 0.0f, 0.0f);
        const AZ::Vector3 localWindValue = GetWind(position);
        const AZ::Vector3 expectedWindValue =
            GetWindValue(windDirectionA, windMagnitudeA) + GetWindValue(windDirectionB, windMagnitudeB);

        EXPECT_THAT(localWindValue, UnitTest::IsClose(expectedWindValue));
    }

    TEST_F(PhysXWindTest, LocalWind_SingleForceRegionMoved_WindBusReturnsCorrectValuesForOldAndNewPositions)
    {
        const AZ::Vector3 originalPosition = AZ::Vector3::CreateZero();

        const AZ::Aabb aabb = AZ::Aabb::CreateCenterRadius(
            originalPosition,
            10.0f
        );
        const AZ::Vector3 windDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitude = 20.0f;
        auto forceRegionEntity = AddWindRegion(aabb, windDirection, windMagnitude, WindType::Local);

        // Move entity to new position
        const AZ::Vector3 newPosition = AZ::Vector3(500.0f, 100.0f, 0.0f);

        AZ::TransformBus::Event(forceRegionEntity->GetId()
            , &AZ::TransformBus::Events::SetWorldTranslation
            , newPosition
        );

        // Wind value at old position should be zero
        {
            const AZ::Vector3 localWindValue = GetWind(originalPosition);
            EXPECT_TRUE(localWindValue.IsZero());
        }

        // Sampling new position should return original wind value
        {
            const AZ::Vector3 localWindValue = GetWind(newPosition);
            const AZ::Vector3 expectedWindValue = GetWindValue(windDirection, windMagnitude);
            EXPECT_THAT(localWindValue, UnitTest::IsClose(expectedWindValue));
        }
    }

    TEST_F(PhysXWindTest, LocalWind_SingleForceRegionMoved_OnWindChangedNotificationCalledForOldAndNewPosition)
    {
        const AZ::Vector3 originalPosition = AZ::Vector3::CreateZero();

        const AZ::Aabb originalAabb = AZ::Aabb::CreateCenterRadius(
            originalPosition,
            10.0f
        );
        const AZ::Vector3 windDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
        const float windMagnitude = 20.0f;
        auto forceRegionEntity = AddWindRegion(originalAabb, windDirection, windMagnitude, WindType::Local);

        // Tick to flush any existing updates pending in wind system
        AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.01f, AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));

        // Move entity to new position
        const AZ::Vector3 newPosition = AZ::Vector3(500.0f, 100.0f, 0.0f);

        AZ::TransformBus::Event(forceRegionEntity->GetId()
            , &AZ::TransformBus::Events::SetWorldTranslation
            , newPosition
        );

        AZ::Aabb newAabb = originalAabb;
        newAabb.Translate(newPosition - originalPosition);

        // We expect OnWindChanged to be called exactly twice - for old and new bounding boxes
        {
            testing::StrictMock<MockWindNotificationsBusHandler> mockHandler;

            EXPECT_CALL(mockHandler, OnWindChanged(originalAabb)).Times(1);
            EXPECT_CALL(mockHandler, OnWindChanged(newAabb)).Times(1);

            AZ::TickBus::Broadcast(&AZ::TickEvents::OnTick, 0.01f, AZ::ScriptTimePoint(AZStd::chrono::steady_clock::now()));
        }
    }

} // namespace PhysX
