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
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Random.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>
#include <Shape/AxisAlignedBoxShapeComponent.h>
#include <ShapeThreadsafeTest.h>

namespace UnitTest
{
    class AxisAlignedBoxShapeTest : public AllocatorsFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_axisAlignedBoxShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_axisAlignedBoxShapeDebugDisplayComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_axisAlignedBoxShapeComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::AxisAlignedBoxShapeComponent::CreateDescriptor());
            m_axisAlignedBoxShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_axisAlignedBoxShapeDebugDisplayComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::AxisAlignedBoxShapeDebugDisplayComponent::CreateDescriptor());
            m_axisAlignedBoxShapeDebugDisplayComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_axisAlignedBoxShapeComponentDescriptor.reset();
            m_axisAlignedBoxShapeDebugDisplayComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateAxisAlignedBox(const AZ::Transform& transform, const AZ::Vector3& dimensions, AZ::Entity& entity)
    {
        entity.CreateComponent<LmbrCentral::AxisAlignedBoxShapeComponent>();
        entity.CreateComponent<LmbrCentral::AxisAlignedBoxShapeDebugDisplayComponent>();
        entity.CreateComponent<AzFramework::TransformComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::BoxShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::BoxShapeComponentRequestsBus::Events::SetBoxDimensions, dimensions);
    }

    void CreateDefaultAxisAlignedBox(const AZ::Transform& transform, AZ::Entity& entity)
    {
        CreateAxisAlignedBox(transform, AZ::Vector3(10.0f, 10.0f, 10.0f), entity);
    }

    TEST_F(AxisAlignedBoxShapeTest, EntityTransformIsCorrect)
    {
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)) * AZ::Transform::CreateRotationZ(AZ::Constants::QuarterPi),
            AZ::Vector3(1.0f), entity);

        AZ::Transform transform;
        AZ::TransformBus::EventResult(transform, entity.GetId(), &AZ::TransformBus::Events::GetWorldTM);

        EXPECT_EQ(transform, AZ::Transform::CreateRotationZ(AZ::Constants::QuarterPi));
    }

    TEST_F(AxisAlignedBoxShapeTest, BoxWithZRotationHasCorrectRayIntersection)
    {
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform::CreateRotationZ(AZ::Constants::QuarterPi),
            AZ::Vector3(1.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(5.0f, 0.0f, 0.0f),
            AZ::Vector3(-1.0f, 0.0f, 0.0f), distance);

        // This test creates a unit box centered on (0, 0, 0) and rotated by 45 degrees. The distance to the box should
        // be 4.5 if it isn't rotated but less if there is any rotation.
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.5f, 1e-2f);
    }

    TEST_F(AxisAlignedBoxShapeTest, BoxWithTranslationAndRotationsHasCorrectRayIntersection)
    {
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi) *
                    AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), AZ::Constants::QuarterPi),
                AZ::Vector3(-10.0f, -10.0f, -10.0f)),
            AZ::Vector3(4.0f, 4.0f, 2.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(-10.0f, -10.0f, 0.0f),
            AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        // This test creates a box of dimensions (4.0, 4.0, 2.0) centered on (-10, -10, 0) and rotated in X and Z. The distance to the box should
        // be 9.0 if it isn't rotated but less if there is any rotation.
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 9.00f, 1e-2f);
    }

   TEST_F(AxisAlignedBoxShapeTest, BoxWithTranslationHasCorrectRayIntersection)
    {
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform::CreateTranslation(AZ::Vector3(100.0f, 100.0f, 0.0f)),
            AZ::Vector3(5.0f, 5.0f, 5.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(100.0f, 100.0f, -100.0f),
            AZ::Vector3(0.0f, 0.0f, 1.0f), distance);

        // This test creates a box of dimensions (5.0, 5.0, 5.0) centered on (100, 100, 0) and not rotated. The distance to the box
        // should be 97.5.
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 97.5f, 1e-2f);
    }

    TEST_F(AxisAlignedBoxShapeTest, BoxWithTranslationRotationAndScaleHasCorrectRayIntersection)
    {
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform(
                AZ::Vector3(0.0f, 0.0f, 5.0f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi), 3.0f),
            AZ::Vector3(2.0f, 4.0f, 1.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(1.0f, -10.0f, 4.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f), distance);

        // This test creates a box of dimensions (2.0, 4.0, 1.0) centered on (0, 0, 5) and rotated about the Y axis by 45 degrees.
        // The distance to the box should be 4.0 if not rotated but scaled and less if it is.
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.0f, 1e-2f);
    }

    TEST_F(AxisAlignedBoxShapeTest, ShapeHasThreadsafeGetSetCalls)
    {
        // Verify that setting values from one thread and querying values from multiple other threads in parallel produces
        // correct, consistent results.

        // Create our axis-aligned box centered at 0 with our height and starting XY dimensions.
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()),
            AZ::Vector3(ShapeThreadsafeTest::MinDimension, ShapeThreadsafeTest::MinDimension, ShapeThreadsafeTest::ShapeHeight), entity);

        // Define the function for setting unimportant dimensions on the shape while queries take place.
        auto setDimensionFn = [](AZ::EntityId shapeEntityId, float minDimension, uint32_t dimensionVariance, float height)
        {
            float x = minDimension + aznumeric_cast<float>(rand() % dimensionVariance);
            float y = minDimension + aznumeric_cast<float>(rand() % dimensionVariance);

            LmbrCentral::BoxShapeComponentRequestsBus::Event(
                shapeEntityId, &LmbrCentral::BoxShapeComponentRequestsBus::Events::SetBoxDimensions, AZ::Vector3(x, y, height));
        };

        // Run the test, which will run multiple queries in parallel with each other and with the dimension-setting function.
        // The number of iterations is arbitrary - it's set high enough to catch most failures, but low enough to keep the test
        // time to a minimum.
        const int numIterations = 30000;
        ShapeThreadsafeTest::TestShapeGetSetCallsAreThreadsafe(entity, numIterations, setDimensionFn);
    }

} // namespace UnitTest
