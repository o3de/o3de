/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <Shape/SphereShapeComponent.h>
#include <ShapeTestUtils.h>
#include <ShapeThreadsafeTest.h>

namespace Constants = AZ::Constants;

namespace UnitTest
{
    class SphereShapeTest
        : public AllocatorsFixture
        , public ShapeOffsetTestsBase
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_sphereShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_sphereShapeDebugDisplayComponentDescriptor;
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            ShapeOffsetTestsBase::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_transformShapeComponentDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_sphereShapeComponentDescriptor.reset(LmbrCentral::SphereShapeComponent::CreateDescriptor());
            m_sphereShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_sphereShapeDebugDisplayComponentDescriptor.reset(LmbrCentral::SphereShapeDebugDisplayComponent::CreateDescriptor());
            m_sphereShapeDebugDisplayComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformShapeComponentDescriptor.reset();
            m_sphereShapeComponentDescriptor.reset();
            m_sphereShapeDebugDisplayComponentDescriptor.reset();
            m_serializeContext.reset();
            ShapeOffsetTestsBase::TearDown();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateSphere(
        AZ::Entity& entity,
        const AZ::Transform& transform,
        const float radius,
        const AZ::Vector3& translationOffset = AZ::Vector3::CreateZero())
    {
        entity.CreateComponent<AzFramework::TransformComponent>();
        entity.CreateComponent<LmbrCentral::SphereShapeComponent>();
        entity.CreateComponent<LmbrCentral::SphereShapeDebugDisplayComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::SphereShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::SphereShapeComponentRequests::SetRadius, radius);
        LmbrCentral::ShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::SetTranslationOffset, translationOffset);
    }

    void CreateUnitSphere(const AZ::Vector3& position, AZ::Entity& entity)
    {
        CreateSphere(entity, AZ::Transform::CreateTranslation(position), 0.5f);
    }

    void CreateUnitSphereAtOrigin(AZ::Entity& entity)
    {
        CreateUnitSphere(AZ::Vector3::CreateZero(), entity);
    }

    /** 
     * @brief Creates a point in a sphere using spherical coordinates
     * @radius The radial distance from the center of the sphere
     * @verticalAngle The angle around the sphere vertically - think top to bottom
     * @horizontalAngle The angle around the sphere horizontally- think left to right
     * @return A point represeting the coordinates in the sphere
     */
    AZ::Vector3 CreateSpherePoint(float radius, float verticalAngle, float horizontalAngle)
    {
        return AZ::Vector3(
            radius * sinf(verticalAngle) * cosf(horizontalAngle),
            radius * sinf(verticalAngle) * sinf(horizontalAngle),
            radius * cosf(verticalAngle));
    }

    TEST_F(SphereShapeTest, SetRadiusIsPropagatedToGetConfiguration)
    {
        AZ::Entity entity;
        CreateUnitSphereAtOrigin(entity);

        float newRadius = 123.456f;
        LmbrCentral::SphereShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::SphereShapeComponentRequestsBus::Events::SetRadius, newRadius);

        LmbrCentral::SphereShapeConfig config(-1.0f);
        LmbrCentral::SphereShapeComponentRequestsBus::EventResult(config, entity.GetId(), &LmbrCentral::SphereShapeComponentRequestsBus::Events::GetSphereConfiguration);

        EXPECT_FLOAT_EQ(newRadius, config.m_radius);
    }

    TEST_F(SphereShapeTest, GetPointInsideSphere)
    {
        AZ::Entity entity;
        const AZ::Vector3 center(1.0f, 2.0f, 3.0f);
        CreateUnitSphere(center, entity);

        AZ::Vector3 point = center + CreateSpherePoint(0.49f, AZ::Constants::Pi / 4.0f, AZ::Constants::Pi / 4.0f);
        bool isInside = false;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point);

        EXPECT_TRUE(isInside);
    }

    TEST_F(SphereShapeTest, GetPointOutsideSphere)
    {
        AZ::Entity entity;
        const AZ::Vector3 center(1.0f, 2.0f, 3.0f);
        CreateUnitSphere(center, entity);

        AZ::Vector3 point = center + CreateSpherePoint(0.51f, AZ::Constants::Pi / 4.0f, AZ::Constants::Pi / 4.0f);
        bool isInside = true;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            isInside, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, point);

        EXPECT_FALSE(isInside);
    }

    TEST_F(SphereShapeTest, GetRayIntersectSphereSuccess1)
    {
        AZ::Entity entity;
        CreateUnitSphere(AZ::Vector3(0.0f, 0.0f, 5.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(0.0f, 5.0f, 5.0f), AZ::Vector3(0.0f, -1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.5f, 1e-4f);
    }

    TEST_F(SphereShapeTest, GetRayIntersectSphereSuccess2)
    {
        AZ::Entity entity;
        CreateSphere(entity, AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, -10.0f, -10.0f)), 2.5f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(-10.0f, -10.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 7.5f, 1e-4f);
    }

    TEST_F(SphereShapeTest, GetRayIntersectSphereSuccess3)
    {
        AZ::Entity entity;
        CreateSphere(entity, AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 0.0f, 0.0f)), 1.0f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(6.0f, 10.0f, 0.0f), AZ::Vector3(0.0f, -1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 10.0f, 1e-4f);
    }

    // transformed scaled
    TEST_F(SphereShapeTest, GetRayIntersectSphereSuccess4)
    {
        AZ::Entity entity;
        CreateSphere(
            entity, AZ::Transform::CreateTranslation(AZ::Vector3(-8.0f, -15.0f, 5.0f)) * AZ::Transform::CreateUniformScale(5.0f), 0.25f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(-5.0f, -15.0f, 5.0f), AZ::Vector3(-1.0f, 0.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 1.75f, 1e-4f);
    }

    TEST_F(SphereShapeTest, GetRayIntersectSphereFailure)
    {
        AZ::Entity entity;
        CreateSphere(entity, AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)), 2.0f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(3.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    // not transformed
    TEST_F(SphereShapeTest, GetAabb1)
    {
        AZ::Entity entity;
        CreateSphere(entity, AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), 2.0f);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-2.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(2.0f)));
    }

    // transformed
    TEST_F(SphereShapeTest, GetAabb2)
    {
        AZ::Entity entity;
        CreateSphere(entity, AZ::Transform::CreateTranslation(AZ::Vector3(200.0f, 150.0f, 60.0f)), 2.0f);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(198.0f, 148.0f, 58.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(202.0f, 152.0f, 62.0f)));
    }

    // transform scaled
    TEST_F(SphereShapeTest, GetAabb3)
    {
        AZ::Entity entity;
        CreateSphere(
            entity, AZ::Transform::CreateTranslation(AZ::Vector3(100.0f, 200.0f, 300.0f)) * AZ::Transform::CreateUniformScale(2.5f), 0.5f);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(98.75f, 198.75f, 298.75f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(101.25f, 201.25f, 301.25f)));
    }

    TEST_F(SphereShapeTest, GetTransformAndLocalBounds1)
    {
        AZ::Entity entity;
        AZ::Transform transformIn = AZ::Transform::CreateIdentity();
        CreateSphere(entity, transformIn, 2.0f);

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_TRUE(transformOut.IsClose(transformIn));
        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-2.0f, -2.0f, -2.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(2.0f, 2.0f, 2.0f)));
    }

    TEST_F(SphereShapeTest, GetTransformAndLocalBounds2)
    {
        AZ::Entity entity;
        AZ::Transform transformIn = AZ::Transform::CreateTranslation(AZ::Vector3(100.0f, 200.0f, 300.0f)) * AZ::Transform::CreateUniformScale(2.5f);
        CreateSphere(entity, transformIn, 2.0f);

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_TRUE(transformOut.IsClose(transformIn));
        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-2.0f, -2.0f, -2.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(2.0f, 2.0f, 2.0f)));
    }

    // point inside scaled
    TEST_F(SphereShapeTest, IsPointInsideSuccess1)
    {
        AZ::Entity entity;
        CreateSphere(
            entity, AZ::Transform::CreateTranslation(AZ::Vector3(-30.0f, -30.0f, 22.0f)) * AZ::Transform::CreateUniformScale(2.0f), 1.2f);

        bool inside;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(-30.0f, -30.0f, 20.0f));

        EXPECT_TRUE(inside);
    }

    // point inside scaled
    TEST_F(SphereShapeTest, IsPointInsideSuccess2)
    {
        AZ::Entity entity;
        CreateSphere(
            entity, AZ::Transform::CreateTranslation(AZ::Vector3(-30.0f, -30.0f, 22.0f)) * AZ::Transform::CreateUniformScale(1.5f), 1.6f);

        bool inside;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(-31.0f, -32.0f, 21.2f));

        EXPECT_TRUE(inside);
    }

    // distance scaled
    TEST_F(SphereShapeTest, DistanceFromPoint1)
    {
        AZ::Entity entity;
        CreateSphere(
            entity, AZ::Transform::CreateTranslation(AZ::Vector3(19.0f, 34.0f, 37.0f)) * AZ::Transform::CreateUniformScale(2.0f), 1.0f);

        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(13.0f, 34.0f, 37.2f));

        EXPECT_NEAR(distance, 4.0f, 1e-2f);
    }

    // distance scaled
    TEST_F(SphereShapeTest, DistanceFromPoint2)
    {
        AZ::Entity entity;
        CreateSphere(
            entity, AZ::Transform::CreateTranslation(AZ::Vector3(19.0f, 34.0f, 37.0f)) * AZ::Transform::CreateUniformScale(0.5f), 1.0f);

        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(19.0f, 37.0f, 37.2f));

        EXPECT_NEAR(distance, 2.5f, 1e-2f);
    }

    TEST_F(SphereShapeTest, ShapeHasThreadsafeGetSetCalls)
    {
        // Verify that setting values from one thread and querying values from multiple other threads in parallel produces
        // correct, consistent results.

        // Create our sphere centered at 0 with half our height as the radius.
        AZ::Entity entity;
        CreateSphere(entity, AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), ShapeThreadsafeTest::ShapeHeight / 2.0f);

        // Define the function for setting unimportant dimensions on the shape while queries take place.
        auto setDimensionFn = [](AZ::EntityId shapeEntityId, float minDimension, uint32_t dimensionVariance, [[maybe_unused]] float height)
        {
            [[maybe_unused]] float radius = minDimension + aznumeric_cast<float>(rand() % dimensionVariance);
            LmbrCentral::SphereShapeComponentRequestsBus::Event(
                shapeEntityId, &LmbrCentral::SphereShapeComponentRequestsBus::Events::SetRadius, height / 2.0f);
        };

        // Run the test, which will run multiple queries in parallel with each other and with the dimension-setting function.
        // The number of iterations is arbitrary - it's set high enough to catch most failures, but low enough to keep the test
        // time to a minimum.
        const int numIterations = 30000;
        ShapeThreadsafeTest::TestShapeGetSetCallsAreThreadsafe(entity, numIterations, setDimensionFn);
    }

    TEST_F(SphereShapeTest, GetRayIntersectSphereWithTranslationOffsetJustIntersecting)
    {
        AZ::Entity entity;
        CreateSphere(
            entity,
            AZ::Transform(AZ::Vector3(2.0f, 3.0f, 4.0f), AZ::Quaternion(0.12f, 0.24f, 0.08f, 0.96f), 2.0f),
            0.5f,
            AZ::Vector3(3.0f, -6.0f, 3.0f));

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(10.304f, -9.0f, 3.2608f), AZ::Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 0.3344f, 1e-3f);
    }

    TEST_F(SphereShapeTest, GetRayIntersectSphereWithTranslationOffsetJustMissing)
    {
        AZ::Entity entity;
        CreateSphere(
            entity,
            AZ::Transform(AZ::Vector3(2.0f, 3.0f, 4.0f), AZ::Quaternion(0.12f, 0.24f, 0.08f, 0.96f), 2.0f),
            0.5f,
            AZ::Vector3(3.0f, -6.0f, 3.0f));

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(10.254f, -9.0f, 3.2608f), AZ::Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(SphereShapeTest, GetAabbRotatedAndScaledWithTranslationOffset)
    {
        AZ::Entity entity;
        CreateSphere(
            entity,
            AZ::Transform(AZ::Vector3(-5.0f, 6.0f, -2.0f), AZ::Quaternion(0.7f, 0.1f, -0.1f, 0.7f), 0.8f),
            1.5f,
            AZ::Vector3(2.0f, -2.0f, 7.0f));

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-5.112f, -0.8f, -5.184f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(-2.712f, 1.6f, -2.784f)));
    }

    TEST_F(SphereShapeTest, GetTransformAndLocalBoundsWithTranslationOffset)
    {
        AZ::Entity entity;
        AZ::Transform transform(AZ::Vector3(1.0f, 2.0f, 5.0f), AZ::Quaternion(0.58f, 0.22f, 0.26f, 0.74f), 0.5f);
        CreateSphere(
            entity,
            transform,
            2.5f,
            AZ::Vector3(4.0f, -3.0f, 3.0f));

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_THAT(transformOut, IsClose(transform));
        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(1.5f, -5.5f, 0.5f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(6.5f, -0.5f, 5.5f)));
    }

    TEST_F(SphereShapeTest, IsPointInsideWithTranslationOffset)
    {
        AZ::Entity entity;
        CreateSphere(
            entity,
            AZ::Transform(AZ::Vector3(4.0f, 7.0f, 3.0f), AZ::Quaternion(-0.1f, -0.1f, 0.7f, 0.7f), 2.0f),
            1.0f,
            AZ::Vector3(4.0f, -4.0f, 5.0f));

        // test some pairs of nearby points which should be just either side of the surface of the capsule
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(6.9f, 15.0f, 15.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(6.8f, 15.0f, 15.0f)));
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(9.0f, 16.9f, 15.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(9.0f, 17.0f, 15.0f)));
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(9.0f, 15.0f, 16.8f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(9.0f, 15.0f, 16.9f)));
    }

    TEST_F(SphereShapeTest, DistanceFromPointWithTranslationOffset)
    {
        AZ::Entity entity;
        CreateSphere(
            entity,
            AZ::Transform(AZ::Vector3(2.0f, -5.0f, -4.0f), AZ::Quaternion(0.7f, -0.7f, 0.1f, 0.1f), 1.5f),
            3.0f,
            AZ::Vector3(3.0f, 5.0f, 6.0f));

        float distance = AZ::Constants::FloatMax;
        // should be just inside
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(-9.9f, -11.84f, -11.38f));
        EXPECT_NEAR(distance, 0.0f, 1e-3f);

        // should be just outside
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(-10.1f, -11.84f, -11.38f));
        EXPECT_NEAR(distance, 0.1f, 1e-3f);
    }

    TEST_F(SphereShapeTest, DebugDrawWithTranslationOffset)
    {
        AZ::Entity entity;
        CreateSphere(
            entity,
            AZ::Transform(AZ::Vector3(5.0f, 4.0f, 1.0f), AZ::Quaternion(0.62f, 0.62f, 0.14f, 0.46f), 2.5f),
            1.4f,
            AZ::Vector3(2.0f, 6.0f, -7.0f));

        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;

        LmbrCentral::ShapeComponentNotificationsBus::Event(
            entity.GetId(),
            &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
            LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

        AzFramework::EntityDebugDisplayEventBus::Event(entity.GetId(), &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);

        const AZStd::vector<AZ::Vector3>& points = testDebugDisplayRequests.GetPoints();
        const AZ::Aabb debugDrawAabb = points.size() > 0 ? AZ::Aabb::CreatePoints(points.data(), points.size()) : AZ::Aabb::CreateNull();

        // use quite low tolerance because the debug draw mesh is only an approximation to a perfect sphere
        EXPECT_THAT(debugDrawAabb.GetMin(), IsCloseTolerance(AZ::Vector3(-1.0f, 14.8f, 16.1f), 0.1f));
        EXPECT_THAT(debugDrawAabb.GetMax(), IsCloseTolerance(AZ::Vector3(6.0f, 21.8f, 23.1f), 0.1f));
    }
} // namespace UnitTest
