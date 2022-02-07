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
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>
#include <Shape/AxisAlignedBoxShapeComponent.h>

namespace UnitTest
{
    class AxisAlignedBoxShapeTest : public AllocatorsFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_axisAlignedBoxShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_axisAlignedBoxShapeDebugDisplayComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_nonUniformScaleComponentDescriptor;

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
            m_nonUniformScaleComponentDescriptor =
                AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::NonUniformScaleComponent::CreateDescriptor());
            m_nonUniformScaleComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_axisAlignedBoxShapeComponentDescriptor.reset();
            m_axisAlignedBoxShapeDebugDisplayComponentDescriptor.reset();
            m_nonUniformScaleComponentDescriptor.reset();
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

    void CreateAxisAlignedBoxWithNonUniformScale(
        const AZ::Transform& transform, const AZ::Vector3& nonUniformScale, const AZ::Vector3& dimensions, AZ::Entity& entity)
    {
        entity.CreateComponent<LmbrCentral::AxisAlignedBoxShapeComponent>();
        entity.CreateComponent<LmbrCentral::AxisAlignedBoxShapeDebugDisplayComponent>();
        entity.CreateComponent<AzFramework::TransformComponent>();
        entity.CreateComponent<AzFramework::NonUniformScaleComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::BoxShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::BoxShapeComponentRequestsBus::Events::SetBoxDimensions, dimensions);
        AZ::NonUniformScaleRequestBus::Event(entity.GetId(), &AZ::NonUniformScaleRequests::SetScale, nonUniformScale);
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

    TEST_F(AxisAlignedBoxShapeTest, RayIntersectWithBoxRotatedNonUniformScale)
    {
        AZ::Entity entity;
        CreateAxisAlignedBoxWithNonUniformScale(
            AZ::Transform(
                AZ::Vector3(2.0f, -5.0f, 3.0f), AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi),
                0.5f),
            AZ::Vector3(2.2f, 1.8f, 0.4f), AZ::Vector3(0.2f, 2.6f, 1.2f), entity);

        // This test creates a box of dimensions (2.2, 1.8, 0.4) centered on (2.0, -5, 3) and rotated about the Y axis by 45 degrees.
        // The box is tested for axis-alignment by firing various rays and ensuring they either hit or miss the box. Any failure here
        // would show the box has been rotated.

        // Ray should just miss the box
        bool rayHit = false;
        float distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(1.8f, -6.2f, 3.0f),
            AZ::Vector3(1.0f, 0.0f, 0.0f), distance);
        EXPECT_FALSE(rayHit);

        // Ray should just hit the box
        rayHit = false;
        distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(1.8f, -6.1f, 3.0f),
            AZ::Vector3(1.0f, 0.0f, 0.0f), distance);
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 0.09f, 1e-3f);

        // Ray should just miss the box
        rayHit = false;
        distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(2.2f, -6.2f, 3.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f), distance);
        EXPECT_FALSE(rayHit);

        // Ray should just hit the box
        rayHit = false;
        distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(2.1f, -6.2f, 3.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f), distance);
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 0.03f, 1e-3f);
    }
} // namespace UnitTest
