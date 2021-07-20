/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Random.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <Shape/BoxShapeComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>

namespace UnitTest
{
    class BoxShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_boxShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_boxShapeDebugDisplayComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_nonUniformScaleComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_boxShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::BoxShapeComponent::CreateDescriptor());
            m_boxShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_boxShapeDebugDisplayComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(
                LmbrCentral::BoxShapeDebugDisplayComponent::CreateDescriptor());
            m_boxShapeDebugDisplayComponentDescriptor->Reflect(&(*m_serializeContext));
            m_nonUniformScaleComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(
                AzFramework::NonUniformScaleComponent::CreateDescriptor());
            m_nonUniformScaleComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_boxShapeComponentDescriptor.reset();
            m_boxShapeDebugDisplayComponentDescriptor.reset();
            m_nonUniformScaleComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateBox(const AZ::Transform& transform, const AZ::Vector3& dimensions, AZ::Entity& entity)
    {
        entity.CreateComponent<LmbrCentral::BoxShapeComponent>();
        entity.CreateComponent<LmbrCentral::BoxShapeDebugDisplayComponent>();
        entity.CreateComponent<AzFramework::TransformComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::BoxShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::BoxShapeComponentRequestsBus::Events::SetBoxDimensions, dimensions);
    }

    void CreateBoxWithNonUniformScale(const AZ::Transform& transform, const AZ::Vector3& nonUniformScale,
        const AZ::Vector3& dimensions, AZ::Entity& entity)
    {
        entity.CreateComponent<LmbrCentral::BoxShapeComponent>();
        entity.CreateComponent<LmbrCentral::BoxShapeDebugDisplayComponent>();
        entity.CreateComponent<AzFramework::TransformComponent>();
        entity.CreateComponent<AzFramework::NonUniformScaleComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::BoxShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::BoxShapeComponentRequestsBus::Events::SetBoxDimensions, dimensions);
        AZ::NonUniformScaleRequestBus::Event(entity.GetId(), &AZ::NonUniformScaleRequests::SetScale, nonUniformScale);
    }

    void CreateDefaultBox(const AZ::Transform& transform, AZ::Entity& entity)
    {
        CreateBox(transform, AZ::Vector3(10.0f, 10.0f, 10.0f), entity);
    }

    bool RandomPointsAreInBox(const AZ::Entity& entity, const AZ::RandomDistributionType distributionType)
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

    TEST_F(BoxShapeTest, NormalDistributionRandomPointsAreInAABB)
    {
        // don't rotate transform so that this is an AABB
        const AZ::Transform transform = AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 5.0f, 5.0f));

        AZ::Entity entity;
        CreateDefaultBox(transform, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInBox(entity, AZ::RandomDistributionType::Normal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(BoxShapeTest, UniformRealDistributionRandomPointsAreInAABB)
    {
        // don't rotate transform so that this is an AABB
        const AZ::Transform transform = AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 5.0f, 5.0f));

        AZ::Entity entity;
        CreateDefaultBox(transform, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInBox(entity, AZ::RandomDistributionType::UniformReal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(BoxShapeTest, NormalDistributionRandomPointsAreInOBB)
    {
        // rotate to end up with an OBB
        const AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationY(AZ::Constants::QuarterPi), AZ::Vector3(5.0f, 5.0f, 5.0f));

        AZ::Entity entity;
        CreateDefaultBox(transform, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInBox(entity, AZ::RandomDistributionType::Normal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(BoxShapeTest, UniformRealDistributionRandomPointsAreInOBB)
    {
        // rotate to end up with an OBB
        const AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationY(AZ::Constants::QuarterPi), AZ::Vector3(5.0f, 5.0f, 5.0f));

        AZ::Entity entity;
        CreateDefaultBox(transform, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInBox(entity, AZ::RandomDistributionType::UniformReal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(BoxShapeTest, UniformRealDistributionRandomPointsAreInAABBWithNonUniformScale)
    {
        AZ::Entity entity;
        const AZ::Transform transform = AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, 6.0f, -3.0f));
        const AZ::Vector3 dimensions(2.4f, 1.2f, 0.6f);
        const AZ::Vector3 nonUniformScale(0.2f, 0.3f, 0.1f);
        CreateBoxWithNonUniformScale(transform, nonUniformScale, dimensions, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInBox(entity, AZ::RandomDistributionType::UniformReal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(BoxShapeTest, UniformRealDistributionRandomPointsAreInOBBWithNonUniformScale)
    {
        AZ::Entity entity;
        const AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.48f, 0.60f, 0.0f, 0.64f), AZ::Vector3(2.0f, 6.0f, -3.0f));
        const AZ::Vector3 dimensions(1.5f, 2.2f, 1.6f);
        const AZ::Vector3 nonUniformScale(0.4f, 0.1f, 0.3f);
        CreateBoxWithNonUniformScale(transform, nonUniformScale, dimensions, entity);

        const bool allRandomPointsInVolume = RandomPointsAreInBox(entity, AZ::RandomDistributionType::UniformReal);
        EXPECT_TRUE(allRandomPointsInVolume);
    }

    TEST_F(BoxShapeTest, GetRayIntersectBoxSuccess1)
    {
        AZ::Entity entity;
        CreateBox(
            AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 5.0f)) *
            AZ::Transform::CreateRotationZ(AZ::Constants::QuarterPi),
            AZ::Vector3(1.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(0.0f, 5.0f, 5.0f), AZ::Vector3(0.0f, -1.0f, 0.0f), distance);

        // 5.0 - 0.707 ~= 4.29 (box rotated by 45 degrees)
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.29f, 1e-2f);
    }

    TEST_F(BoxShapeTest, GetRayIntersectBoxSuccess2)
    {
        AZ::Entity entity;
        CreateBox(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi) *
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), AZ::Constants::QuarterPi), AZ::Vector3(-10.0f, -10.0f, -10.0f)),
            AZ::Vector3(4.0f, 4.0f, 2.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(-10.0f, -10.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);
        
        // 0.70710678 * 4 = 2.8284271
        // 10.0 - 2.8284271 ~= 7.17157287
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 7.17f, 1e-2f);
    }

    TEST_F(BoxShapeTest, GetRayIntersectBoxSuccess3)
    {
        AZ::Entity entity;
        CreateBox(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateIdentity(), AZ::Vector3(100.0f, 100.0f, 0.0f)),
            AZ::Vector3(5.0f, 5.0f, 5.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(100.0f, 100.0f, -100.0f), AZ::Vector3(0.0f, 0.0f, 1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 97.5f, 1e-2f);
    }

    // transformed scaled
    TEST_F(BoxShapeTest, GetRayIntersectBoxSuccess4)
    {
        AZ::Entity entity;
        CreateBox(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi),
                AZ::Vector3(0.0f, 0.0f, 5.0f)) *
            AZ::Transform::CreateUniformScale(3.0f),
            AZ::Vector3(2.0f, 4.0f, 1.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(1.0f, -10.0f, 4.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.0f, 1e-2f);
    }

    TEST_F(BoxShapeTest, GetRayIntersectBoxFailure)
    {
        AZ::Entity entity;
        CreateBox(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, -10.0f, 0.0f)),
            AZ::Vector3(2.0f, 6.0f, 4.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3::CreateZero(), AZ::Vector3(1.0f, 0.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(BoxShapeTest, GetRayIntersectBoxUnrotatedNonUniformScale)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, -5.0f, 3.0f));
        transform.MultiplyByUniformScale(0.5f);
        const AZ::Vector3 dimensions(2.2f, 1.8f, 0.4f);
        const AZ::Vector3 nonUniformScale(0.2f, 2.6f, 1.2f);
        CreateBoxWithNonUniformScale(transform, dimensions, nonUniformScale, entity);

        // should just miss the box
        bool rayHit = false;
        float distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(1.8f, -6.2f, 3.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), distance);
        EXPECT_FALSE(rayHit);

        // should just hit the box
        rayHit = false;
        distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(1.8f, -6.1f, 3.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), distance);
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 0.09f, 1e-3f);

        // should just miss the box
        rayHit = false;
        distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(2.2f, -6.2f, 3.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), distance);
        EXPECT_FALSE(rayHit);

        // should just hit the box
        rayHit = false;
        distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(2.1f, -6.2f, 3.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), distance);
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 0.03f, 1e-3f);
    }

    TEST_F(BoxShapeTest, GetRayIntersectBoxRotatedNonUniformScale)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.50f, 0.10f, 0.02f, 0.86f), AZ::Vector3(4.0f, 1.0f, -2.0f));
        transform.MultiplyByUniformScale(1.5f);
        const AZ::Vector3 dimensions(1.2f, 0.7f, 2.1f);
        const AZ::Vector3 nonUniformScale(0.8f, 0.6f, 0.7f);
        CreateBoxWithNonUniformScale(transform, dimensions, nonUniformScale, entity);

        // should just miss the box
        bool rayHit = false;
        float distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(5.0f, 0.6f, -1.5f), AZ::Vector3(-0.1f, 0.1f, -0.02f).GetNormalized(), distance);
        EXPECT_FALSE(rayHit);

        // should just hit the box
        rayHit = false;
        distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(4.9f, 0.6f, -1.5f), AZ::Vector3(-0.1f, 0.1f, -0.02f).GetNormalized(), distance);
        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 0.0553f, 1e-3f);
    }

    TEST_F(BoxShapeTest, GetAabbIdentityTransform)
    {
        // not rotated - AABB input
        AZ::Entity entity;
        CreateBox(AZ::Transform::CreateIdentity(), AZ::Vector3(1.5f, 3.5f, 5.5f), entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-0.75f, -1.75f, -2.75f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(0.75f, 1.75f, 2.75f)));
    }

    TEST_F(BoxShapeTest, GetAabbRotatedAndTranslated)
    {
        // rotated - OBB input
        AZ::Entity entity;
        CreateDefaultBox(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateRotationY(AZ::Constants::QuarterPi),
                AZ::Vector3(5.0f, 5.0f, 5.0f)), entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-2.07106f, 0.0f, -2.07106f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(12.07106f, 10.0f, 12.07106f)));
    }

    TEST_F(BoxShapeTest, GetAabbRotated)
    {
        // rotated - OBB input
        AZ::Entity entity;
        CreateBox(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi) *
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi), AZ::Vector3(0.0f, 0.0f, 0.0f)),
            AZ::Vector3(2.0f, 5.0f, 1.0f), entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-1.06066f, -2.517766f, -2.517766f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(1.06066f, 2.517766f, 2.517766f)));
    }

    TEST_F(BoxShapeTest, GetAabbTranslated)
    {
        // not rotated - AABB input
        AZ::Entity entity;
        CreateBox(AZ::Transform::CreateTranslation(
            AZ::Vector3(100.0f, 70.0f, 30.0f)), AZ::Vector3(1.8f, 3.5f, 5.2f), entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(99.1f, 68.25f, 27.4f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(100.9f, 71.75f, 32.6f)));
    }

    TEST_F(BoxShapeTest, GetAabbRotatedAndUniformScaled)
    {
        AZ::Entity entity;
        CreateBox(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi),
                AZ::Vector3::CreateZero()) *
            AZ::Transform::CreateUniformScale(3.0f),
            AZ::Vector3(2.0f, 4.0f, 1.0f), entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-3.1819f, -6.0f, -3.1819f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(3.1819f, 6.0f, 3.1819f)));
    }

    TEST_F(BoxShapeTest, GetAabbRotatedAndNonUniformScaled)
    {
        AZ::Entity entity;
        const AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.08f, 0.44f, 0.16f, 0.88f), AZ::Vector3(1.0f, 2.0f, 3.0f));
        const AZ::Vector3 nonUniformScale(0.5f, 1.2f, 2.0f);
        const AZ::Vector3 boxDimensions(2.4f, 2.0f, 4.8f);
        CreateBoxWithNonUniformScale(transform, nonUniformScale, boxDimensions, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-3.4304f, 0.6656f, -0.6672f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(5.4304f, 3.3344f, 6.6672f)));
    }

    TEST_F(BoxShapeTest, GetTransformAndLocalBounds1)
    {
        // not rotated - AABB input
        AZ::Entity entity;
        CreateBox(AZ::Transform::CreateIdentity(), AZ::Vector3(1.5f, 3.5f, 5.5f), entity);

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_THAT(transformOut, IsClose(AZ::Transform::CreateIdentity()));
        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-0.75f, -1.75f, -2.75f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(0.75f, 1.75f, 2.75f)));
    }

    TEST_F(BoxShapeTest, GetTransformAndLocalBounds2)
    {
        // not rotated - AABB input
        AZ::Entity entity;
        AZ::Transform transformIn = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::QuarterPi) * AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi),
            AZ::Vector3(9.0f, 11.0f, 13.0f));
        transformIn.MultiplyByUniformScale(3.0f);
        CreateBox(transformIn, AZ::Vector3(1.5f, 3.5f, 5.5f), entity);

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_THAT(transformOut, IsClose(transformIn));
        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-0.75f, -1.75f, -2.75f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(0.75f, 1.75f, 2.75f)));
    }

    TEST_F(BoxShapeTest, GetTransformAndLocalBoundsNonUniformScale)
    {
        AZ::Entity entity;
        AZ::Transform transformIn = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.62f, 0.62f, 0.14f, 0.46f), AZ::Vector3(0.8f, -1.2f, 2.7f));
        transformIn.MultiplyByUniformScale(2.0f);
        const AZ::Vector3 nonUniformScale(1.5f, 2.0f, 0.4f);
        const AZ::Vector3 boxDimensions(2.0f, 1.7f, 0.5f);
        CreateBoxWithNonUniformScale(transformIn, nonUniformScale, boxDimensions, entity);

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_THAT(transformOut, IsClose(transformIn));
        // the local bounds should include the effect of non-uniform scale, but not the scale from the transform
        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-1.5f, -1.7f, -0.1f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(1.5f, 1.7f, 0.1f)));
    }

    bool IsPointInside(const AZ::Entity& entity, const AZ::Vector3& point)
    {
        bool inside;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, point);
        return inside;
    }

    // point inside scaled
    TEST_F(BoxShapeTest, IsPointInside1)
    {
        AZ::Entity entity;
        CreateBox(
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisZ(), AZ::Constants::QuarterPi),
                AZ::Vector3(23.0f, 12.0f, 40.0f)) *
            AZ::Transform::CreateUniformScale(3.0f),
            AZ::Vector3(2.0f, 6.0f, 3.5f), entity);

        // test some pairs of nearby points which should be just either side of the surface of the box
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(28.0f, 5.0f, 36.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(29.0f, 5.0f, 36.0f)));
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(24.0, 14.0f, 45.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(24.0, 14.0f, 46.0f)));
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(16.0, 15.0f, 42.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(16.0, 14.0f, 42.0f)));
    }

    // point inside scaled
    TEST_F(BoxShapeTest, IsPointInside2)
    {
        AZ::Entity entity;
        CreateBox(
            AZ::Transform::CreateTranslation(AZ::Vector3(23.0f, 12.0f, 40.0f)) *
            AZ::Transform::CreateRotationX(-AZ::Constants::QuarterPi) *
            AZ::Transform::CreateRotationZ(AZ::Constants::QuarterPi) *
            AZ::Transform::CreateUniformScale(2.0f),
            AZ::Vector3(4.0f, 7.0f, 3.5f), entity);

        // test some pairs of nearby points which should be just either side of the surface of the box
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(16.0f, 16.0f, 40.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(16.0f, 17.0f, 40.0f)));
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(24.0f, 10.0f, 38.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(24.0f, 10.0f, 37.0f)));
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(21.0f, 10.0f, 42.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(20.0f, 10.0f, 42.0f)));
    }

    TEST_F(BoxShapeTest, IsPointInsideNonUniformScale)
    {
        AZ::Entity entity;
        const AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.26f, 0.74f, 0.22f, 0.58f), AZ::Vector3(12.0f, -16.0f, 3.0f));
        const AZ::Vector3 nonUniformScale(0.5f, 2.0f, 3.0f);
        const AZ::Vector3 boxDimensions(4.0f, 3.0f, 7.0f);
        CreateBoxWithNonUniformScale(transform, nonUniformScale, boxDimensions, entity);

        // test some pairs of nearby points which should be just either side of the surface of the box
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(2.0f, -16.0f, 6.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(1.0f, -16.0f, 6.0f)));
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(13.0f, -14.0f, 5.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(13.0f, -13.0f, 5.0f)));
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(9.0f, -18.0f, 3.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(9.0f, -18.0f, 4.0f)));
    }

    // distance scaled
    TEST_F(BoxShapeTest, DistanceFromPoint1)
    {
        AZ::Entity entity;
        CreateBox(
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 37.0f, 32.0f)) *
            AZ::Transform::CreateRotationZ(AZ::Constants::QuarterPi) *
            AZ::Transform::CreateUniformScale(2.0f),
            AZ::Vector3(6.0f, 1.0f, 5.0f), entity);

        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(4.0f, 33.5f, 38.0f));

        EXPECT_NEAR(distance, 1.45f, 1e-2f);
    }

    // distance scaled
    TEST_F(BoxShapeTest, DistanceFromPoint2)
    {
        AZ::Entity entity;
        CreateBox(
            AZ::Transform::CreateTranslation(AZ::Vector3(10.0f, 37.0f, 32.0f)) *
            AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
            AZ::Transform::CreateRotationY(AZ::Constants::HalfPi) *
            AZ::Transform::CreateUniformScale(0.5f),
            AZ::Vector3(24.0f, 4.0f, 20.0f), entity);

        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(10.0f, 37.0f, 48.0f));

        EXPECT_NEAR(distance, 15.0f, 1e-2f);
    }

    TEST_F(BoxShapeTest, DistanceFromPointNonUniformScale)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationY(AZ::DegToRad(30.0f)), AZ::Vector3(3.0f, 4.0f, 5.0f));
        transform.MultiplyByUniformScale(2.0f);
        const AZ::Vector3 dimensions(2.0f, 3.0f, 1.5f);
        const AZ::Vector3 nonUniformScale(1.4f, 2.2f, 0.8f);
        CreateBoxWithNonUniformScale(transform, nonUniformScale, dimensions, entity);

        float distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(7.0f, 11.0f, 5.0f));

        EXPECT_NEAR(distance, 1.1140f, 1e-3f);
    }

    TEST_F(BoxShapeTest, DebugDraw)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.70f, 0.10f, 0.34f, 0.62f), AZ::Vector3(3.0f, -1.0f, 2.0f));
        transform.MultiplyByUniformScale(2.0f);
        const AZ::Vector3 dimensions(1.2f, 0.8f, 1.7f);
        const AZ::Vector3 nonUniformScale(2.4f, 1.3f, 1.8f);
        CreateBoxWithNonUniformScale(transform, nonUniformScale, dimensions, entity);

        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;

        AzFramework::EntityDebugDisplayEventBus::Event(entity.GetId(), &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);

        const AZStd::vector<AZ::Vector3>& points = testDebugDisplayRequests.GetPoints();
        const AZ::Aabb debugDrawAabb = points.size() > 0 ? AZ::Aabb::CreatePoints(points.data(), points.size()) : AZ::Aabb::CreateNull();

        AZ::Aabb shapeAabb = AZ::Aabb::CreateNull();
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            shapeAabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);
        EXPECT_THAT(debugDrawAabb.GetMin(), IsClose(shapeAabb.GetMin()));
        EXPECT_THAT(debugDrawAabb.GetMax(), IsClose(shapeAabb.GetMax()));
    }
}
