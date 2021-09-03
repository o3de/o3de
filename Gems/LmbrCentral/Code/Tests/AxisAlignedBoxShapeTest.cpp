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

    bool RandomPointsAreInAxisAlignedBox(const AZ::Entity& entity, const AZ::RandomDistributionType distributionType)
    {
        const size_t testPoints = 10000;

        AZ::Vector3 testPoint;
        bool testPointInVolume = false;

        // test a bunch of random points generated with a random distribution type
        // they should all end up in the volume
        for (size_t i = 0; i < testPoints; ++i)
        {
            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                testPoint, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GenerateRandomPointInside, distributionType);

            LmbrCentral::ShapeComponentRequestsBus::EventResult(
                testPointInVolume, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, testPoint);

            if (!testPointInVolume)
            {
                return false;
            }
        }

        return true;
    }

    TEST_F(AxisAlignedBoxShapeTest, NormalDistributionRandomPointsAreInBox)
    {
        // don't rotate transform so that this is an AABB
        const AZ::Transform transform = AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 5.0f, 5.0f));

        AZ::Entity entity;
        CreateDefaultAxisAlignedBox(transform, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInAxisAlignedBox(entity, AZ::RandomDistributionType::Normal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(AxisAlignedBoxShapeTest, UniformRealDistributionRandomPointsAreInBox)
    {
        // don't rotate transform so that this is an AABB
        const AZ::Transform transform = AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 5.0f, 5.0f));

        AZ::Entity entity;
        CreateDefaultAxisAlignedBox(transform, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInAxisAlignedBox(entity, AZ::RandomDistributionType::UniformReal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(AxisAlignedBoxShapeTest, UniformRealDistributionRandomPointsAreInBoxWithNonUniformScale)
    {
        AZ::Entity entity;
        const AZ::Transform transform = AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, 6.0f, -3.0f));
        const AZ::Vector3 dimensions(2.4f, 1.2f, 0.6f);
        const AZ::Vector3 nonUniformScale(0.2f, 0.3f, 0.1f);
        CreateAxisAlignedBoxWithNonUniformScale(transform, nonUniformScale, dimensions, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInAxisAlignedBox(entity, AZ::RandomDistributionType::UniformReal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(AxisAlignedBoxShapeTest, EntityTransformCorrect)
    {
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.0f)) * AZ::Transform::CreateRotationZ(AZ::Constants::QuarterPi),
            AZ::Vector3(1.0f), entity);

        AZ::Transform transform;
        AZ::TransformBus::EventResult(transform, entity.GetId(), &AZ::TransformBus::Events::GetWorldTM);

        EXPECT_EQ(transform, AZ::Transform::CreateRotationZ(AZ::Constants::QuarterPi));
    }

    TEST_F(AxisAlignedBoxShapeTest, GetRayIntersectBoxSuccess1)
    {
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 5.0f)) * AZ::Transform::CreateRotationZ(AZ::Constants::QuarterPi),
            AZ::Vector3(1.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(0.0f, 5.0f, 5.0f),
            AZ::Vector3(0.0f, -1.0f, 0.0f), distance);

        // 5.0 - 0.5 ~= 4.5 (box not rotated even though entity is created with 45 degree rotation)
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.5f, 1e-2f);
    }

    TEST_F(AxisAlignedBoxShapeTest, GetRayIntersectBoxSuccess2)
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

        // 10.0 - 1.0 ~= 9 (box not rotated even though entity is created with rotation in X and Z)
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 9.00f, 1e-2f);
    }

   TEST_F(AxisAlignedBoxShapeTest, GetRayIntersectBoxSuccess3)
    {
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), AZ::Vector3(100.0f, 100.0f, 0.0f)),
            AZ::Vector3(5.0f, 5.0f, 5.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(100.0f, 100.0f, -100.0f),
            AZ::Vector3(0.0f, 0.0f, 1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 97.5f, 1e-2f);
    }

       // transformed scaled
    TEST_F(AxisAlignedBoxShapeTest, GetRayIntersectBoxSuccess4)
    {
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi), AZ::Vector3(0.0f, 0.0f, 5.0f)) *
                AZ::Transform::CreateUniformScale(3.0f),
            AZ::Vector3(2.0f, 4.0f, 1.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3(1.0f, -10.0f, 4.0f),
            AZ::Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.0f, 1e-2f);
    }

    TEST_F(AxisAlignedBoxShapeTest, GetRayIntersectBoxFailure)
    {
        AZ::Entity entity;
        CreateAxisAlignedBox(
            AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, -10.0f, 0.0f)),
            AZ::Vector3(2.0f, 6.0f, 4.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay, AZ::Vector3::CreateZero(),
            AZ::Vector3(1.0f, 0.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }
} // namespace UnitTest
