/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzFramework/Components/TransformComponent.h>
#include <LmbrCentral/Shape/SphereShapeComponentBus.h>
#include <Shape/SphereShapeComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <ShapeThreadsafeTest.h>

namespace Constants = AZ::Constants;

namespace UnitTest
{
    class SphereShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_sphereShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_transformShapeComponentDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_sphereShapeComponentDescriptor.reset(LmbrCentral::SphereShapeComponent::CreateDescriptor());
            m_sphereShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformShapeComponentDescriptor.reset();
            m_sphereShapeComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateSphere(const AZ::Transform& transform, const float radius, AZ::Entity& entity)
    {
        entity.CreateComponent<AzFramework::TransformComponent>();
        entity.CreateComponent<LmbrCentral::SphereShapeComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::SphereShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::SphereShapeComponentRequests::SetRadius, radius);
    }

    void CreateUnitSphere(const AZ::Vector3& position, AZ::Entity& entity)
    {
        CreateSphere(AZ::Transform::CreateTranslation(position), 0.5f, entity);
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
        CreateSphere(AZ::Transform::CreateTranslation(AZ::Vector3(-10.0f, -10.0f, -10.0f)), 2.5f, entity);

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
        CreateSphere(AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 0.0f, 0.0f)), 1.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(-8.0f, -15.0f, 5.0f)) *
            AZ::Transform::CreateUniformScale(5.0f),
            0.25f, entity);

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
        CreateSphere(AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)), 2.0f, entity);

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
        CreateSphere(AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), 2.0f, entity);

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
        CreateSphere(AZ::Transform::CreateTranslation(AZ::Vector3(200.0f, 150.0f, 60.0f)), 2.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(100.0f, 200.0f, 300.0f)) *
            AZ::Transform::CreateUniformScale(2.5f),
            0.5f, entity);

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
        CreateSphere(transformIn, 2.0f, entity);

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
        CreateSphere(transformIn, 2.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(-30.0f, -30.0f, 22.0f)) *
            AZ::Transform::CreateUniformScale(2.0f),
            1.2f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(-30.0f, -30.0f, 22.0f)) *
            AZ::Transform::CreateUniformScale(1.5f),
            1.6f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(19.0f, 34.0f, 37.0f)) *
            AZ::Transform::CreateUniformScale(2.0f),
            1.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(19.0f, 34.0f, 37.0f)) *
            AZ::Transform::CreateUniformScale(0.5f),
            1.0f, entity);

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
        CreateSphere(
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), ShapeThreadsafeTest::ShapeHeight / 2.0f, entity);

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
} // namespace UnitTest
