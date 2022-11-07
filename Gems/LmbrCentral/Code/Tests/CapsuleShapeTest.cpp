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
#include <Geometry/GeometrySystemComponent.h>
#include <Shape/CapsuleShapeComponent.h>
#include <ShapeTestUtils.h>
#include <ShapeThreadsafeTest.h>

namespace UnitTest
{
    class CapsuleShapeTest
        : public AllocatorsFixture
        , public ShapeOffsetTestsBase
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_capsuleShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_capsuleShapeDebugDisplayComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_geometrySystemComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            ShapeOffsetTestsBase::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_capsuleShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::CapsuleShapeComponent::CreateDescriptor());
            m_capsuleShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_capsuleShapeDebugDisplayComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::CapsuleShapeDebugDisplayComponent::CreateDescriptor());
            m_capsuleShapeDebugDisplayComponentDescriptor->Reflect(&(*m_serializeContext));
            m_geometrySystemComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::GeometrySystemComponent::CreateDescriptor());
            m_geometrySystemComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_geometrySystemComponentDescriptor.reset();
            m_capsuleShapeDebugDisplayComponentDescriptor.reset();
            m_capsuleShapeComponentDescriptor.reset();
            m_transformComponentDescriptor.reset();
            m_serializeContext.reset();
            ShapeOffsetTestsBase::TearDown();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateCapsule(
        AZ::Entity& entity,
        const AZ::Transform& transform,
        const float radius,
        const float height,
        const AZ::Vector3& translationOffset = AZ::Vector3::CreateZero())
    {
        entity.CreateComponent<LmbrCentral::CapsuleShapeComponent>();
        entity.CreateComponent<LmbrCentral::CapsuleShapeDebugDisplayComponent>();
        entity.CreateComponent<AzFramework::TransformComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);

        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetHeight, height);
        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetRadius, radius);
        LmbrCentral::ShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::SetTranslationOffset, translationOffset);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess1)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, 0.0f, 5.0f)),
            0.5f,
            5.0f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(0.0f, 5.0f, 5.0f), AZ::Vector3(0.0f, -1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.5f, 1e-4f);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess2)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f,
            5.0f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(-10.0f, -20.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 7.5f, 1e-2f);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess3)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f,
            5.0f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(-10.0f, -10.0f, -10.0f), AZ::Vector3(0.0f, 0.0f, 1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 9.0f, 1e-2f);
    }

    // test sphere case
    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess4)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi), AZ::Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f,
            0.0f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(-10.0f, -10.0f, -10.0f), AZ::Vector3(0.0f, 0.0f, 1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 9.0f, 1e-2f);
    }

    // transformed scaled
    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess5)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateTranslation(AZ::Vector3(-4.0f, -12.0f, -3.0f)) * AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
                AZ::Transform::CreateUniformScale(6.0f),
            0.25f,
            1.5f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(-4.0f, -21.0f, -3.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 4.5f, 1e-2f);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleFailure)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, -10.0f, 0.0f)),
            5.0f,
            1.0f);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(0.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(CapsuleShapeTest, GetAabb1)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, -10.0f, 0.0f)),
            5.0f,
            1.0f);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-5.0f, -15.0f, -5.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(5.0f, -5.0f, 5.0f)));
    }

    TEST_F(CapsuleShapeTest, GetAabb2)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateFromQuaternionAndTranslation(
                AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi) *
                    AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi),
                AZ::Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f,
            5.0f);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-12.0606f, -12.0606f, -1.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(-7.9393f, -7.9393f, 1.0f)));
    }

    // test with scale
    TEST_F(CapsuleShapeTest, GetAabb3)
    {
        AZ::Entity entity;
        CreateCapsule(entity, AZ::Transform::CreateUniformScale(3.5f), 2.0f, 4.0f);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-7.0f, -7.0f, -7.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(7.0f, 7.0f, 7.0f)));
    }

    // test with scale and translation
    TEST_F(CapsuleShapeTest, GetAabb4)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity, AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 20.0f, 0.0f)) * AZ::Transform::CreateUniformScale(2.5f), 1.0f, 5.0f);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(2.5f, 17.5f, -6.25f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(7.5f, 22.5f, 6.25f)));
    }

    TEST_F(CapsuleShapeTest, GetTransformAndLocalBounds1)
    {
        AZ::Entity entity;
        AZ::Transform transformIn = AZ::Transform::CreateIdentity();
        CreateCapsule(entity, transformIn, 5.0f, 2.0f);

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_TRUE(transformOut.IsClose(transformIn));
        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-5.0f, -5.0f, -5.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(5.0f, 5.0f, 5.0f)));
    }

    TEST_F(CapsuleShapeTest, GetTransformAndLocalBounds2)
    {
        AZ::Entity entity;
        AZ::Transform transformIn = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi) *
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi), AZ::Vector3(-10.0f, -10.0f, 0.0f));
        transformIn.MultiplyByUniformScale(3.0f);
        CreateCapsule(entity, transformIn, 5.0f, 2.0f);

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_TRUE(transformOut.IsClose(transformIn));
        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-5.0f, -5.0f, -5.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(5.0f, 5.0f, 5.0f)));
    }

    TEST_F(CapsuleShapeTest, GetTransformAndLocalBounds3)
    {
        AZ::Entity entity;
        AZ::Transform transformIn = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi) *
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi), AZ::Vector3(-10.0f, -10.0f, 0.0f));
        transformIn.MultiplyByUniformScale(3.0f);
        CreateCapsule(entity, transformIn, 2.0f, 5.0f);

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_TRUE(transformOut.IsClose(transformIn));
        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-2.0f, -2.0f, -2.5f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(2.0f, 2.0f, 2.5f)));
    }

    // point inside scaled
    TEST_F(CapsuleShapeTest, IsPointInsideSuccess1)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) * AZ::Transform::CreateUniformScale(2.5f),
            0.5f,
            2.0f);

        bool inside;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(27.0f, 28.5f, 40.0f));

        EXPECT_TRUE(inside);
    }

    // point inside scaled
    TEST_F(CapsuleShapeTest, IsPointInsideSuccess2)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) * AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
                AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi) * AZ::Transform::CreateUniformScale(0.5f),
            0.5f,
            2.0f);

        bool inside;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(27.0f, 28.155f, 37.82f));

        EXPECT_TRUE(inside);
    }

    // distance scaled - along length
    TEST_F(CapsuleShapeTest, DistanceFromPoint1)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) * AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
                AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi) * AZ::Transform::CreateUniformScale(2.0f),
            0.5f,
            4.0f);

        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(27.0f, 28.0f, 41.0f));

        EXPECT_NEAR(distance, 2.0f, 1e-2f);
    }

    // distance scaled - from end
    TEST_F(CapsuleShapeTest, DistanceFromPoint2)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) * AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
                AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi) * AZ::Transform::CreateUniformScale(2.0f),
            0.5f,
            4.0f);

        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(22.757f, 32.243f, 38.0f));

        EXPECT_NEAR(distance, 2.0f, 1e-2f);
    }

    TEST_F(CapsuleShapeTest, ShapeHasThreadsafeGetSetCalls)
    {
        // Verify that setting values from one thread and querying values from multiple other threads in parallel produces
        // correct, consistent results.

        // Create our capsule centered at 0 with our height and a starting radius.
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()),
            ShapeThreadsafeTest::MinDimension,
            ShapeThreadsafeTest::ShapeHeight);

        // Define the function for setting unimportant dimensions on the shape while queries take place.
        auto setDimensionFn =
            [](AZ::EntityId shapeEntityId, float minDimension, uint32_t dimensionVariance, [[maybe_unused]] float height)
            {
                float radius = minDimension + aznumeric_cast<float>(rand() % dimensionVariance);
                LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
                    shapeEntityId, &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetRadius, radius);
            };

        // Run the test, which will run multiple queries in parallel with each other and with the dimension-setting function.
        // The number of iterations is arbitrary - it's set high enough to catch most failures, but low enough to keep the test
        // time to a minimum.
        const int numIterations = 30000;
        ShapeThreadsafeTest::TestShapeGetSetCallsAreThreadsafe(entity, numIterations, setDimensionFn);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleWithTranslationOffsetJustIntersecting)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform(AZ::Vector3(7.0f, 8.0f, 9.0f), AZ::Quaternion(0.46f, 0.22f, 0.70f, 0.50f), 2.0f),
            0.5f,
            2.0f,
            AZ::Vector3(3.0f, 4.0f, 5.0f));

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(13.0224f, 8.2928f, 24.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 0.864f, 1e-3f);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleWithTranslationOffsetJustMissing)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform(AZ::Vector3(7.0f, 8.0f, 9.0f), AZ::Quaternion(0.46f, 0.22f, 0.70f, 0.50f), 2.0f),
            0.5f,
            2.0f,
            AZ::Vector3(3.0f, 4.0f, 5.0f));

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(13.1f, 8.2928f, 24.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_FALSE(rayHit);
    }

    TEST_F(CapsuleShapeTest, GetAabbRotatedAndScaledWithTranslationOffset)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform(AZ::Vector3(4.0f, -6.0f, 3.0f), AZ::Quaternion(0.1f, 0.7f, 0.1f, 0.7f), 2.2f),
            0.8f,
            3.0f,
            AZ::Vector3(7.0f, 4.0f, 2.0f));

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(5.1f, 5.0f, -11.08f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(11.7f, 8.52f, -7.56f)));
    }

    TEST_F(CapsuleShapeTest, GetTransformAndLocalBoundsWithTranslationOffset)
    {
        AZ::Entity entity;
        AZ::Transform transform(AZ::Vector3(-5.0f, -1.0f, 2.0f), AZ::Quaternion(0.46f, 0.26f, 0.58f, 0.62f), 1.7f);
        CreateCapsule(entity, transform, 0.6f, 3.5f, AZ::Vector3(-4.0f, 2.0f, 8.0f));

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_THAT(transformOut, IsClose(transform));
        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-4.6f, 1.4f, 6.25f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(-3.4f, 2.6f, 9.75f)));
    }

    TEST_F(CapsuleShapeTest, IsPointInsideWithTranslationOffset)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform(AZ::Vector3(2.0f, 3.0f, -1.0f), AZ::Quaternion(0.48f, 0.36f, 0.48f, 0.64f), 0.8f),
            0.5f,
            4.0f,
            AZ::Vector3(-2.0f, -3.0f, 7.0f));

        // test some pairs of nearby points which should be just either side of the surface of the capsule
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(5.9f, 0.1f, -2.0f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(5.8f, 0.1f, -2.0f)));
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(8.8f, -0.55f, -1.4f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(8.9f, -0.55f, -1.4f)));
        EXPECT_TRUE(IsPointInside(entity, AZ::Vector3(7.48f, 0.15f, -1.74f)));
        EXPECT_FALSE(IsPointInside(entity, AZ::Vector3(7.49f, 0.15f, -1.74f)));
    }

    TEST_F(CapsuleShapeTest, DistanceFromPointWithTranslationOffset)
    {
        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform(AZ::Vector3(-4.0f, 2.0f, -3.0f), AZ::Quaternion(0.64f, 0.52f, 0.40f, 0.40f), 1.2f),
            1.0f,
            6.0f,
            AZ::Vector3(-1.0f, -1.0f, 5.0f));

        float distance = AZ::Constants::FloatMax;
        // should be inside
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(1.0f, 0.4f, -6.4f));
        EXPECT_NEAR(distance, 0.0f, 1e-3f);

        // should be closest to end cap
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(4.9952f, -0.0064f, -7.944f));
        EXPECT_NEAR(distance, 0.72f, 1e-3f);

        // should be closest to cylindrical section
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(1.1672f, 1.6896f, -6.264f));
        EXPECT_NEAR(distance, 0.1f, 1e-3f);
    }

    TEST_F(CapsuleShapeTest, DebugDrawWithTranslationOffset)
    {
        AZ::Entity systemEntity;
        systemEntity.CreateComponent<LmbrCentral::GeometrySystemComponent>();
        systemEntity.Init();
        systemEntity.Activate();

        AZ::Entity entity;
        CreateCapsule(
            entity,
            AZ::Transform(AZ::Vector3(2.0f, 3.0f, 6.0f), AZ::Quaternion(0.32f, 0.16f, 0.16f, 0.92f), 0.8f),
            2.0f,
            7.0f,
            AZ::Vector3(2.0f, -2.0f, -3.0f));

        UnitTest::TestDebugDisplayRequests testDebugDisplayRequests;

        LmbrCentral::ShapeComponentNotificationsBus::Event(
            entity.GetId(),
            &LmbrCentral::ShapeComponentNotificationsBus::Events::OnShapeChanged,
            LmbrCentral::ShapeComponentNotifications::ShapeChangeReasons::ShapeChanged);

        AzFramework::EntityDebugDisplayEventBus::Event(entity.GetId(), &AzFramework::EntityDebugDisplayEvents::DisplayEntityViewport,
            AzFramework::ViewportInfo{ 0 }, testDebugDisplayRequests);

        const AZStd::vector<AZ::Vector3>& points = testDebugDisplayRequests.GetPoints();
        const AZ::Aabb debugDrawAabb = points.size() > 0 ? AZ::Aabb::CreatePoints(points.data(), points.size()) : AZ::Aabb::CreateNull();

        // use quite low tolerance because the debug draw mesh is only an approximation to a perfect capsule
        EXPECT_THAT(debugDrawAabb.GetMin(), IsCloseTolerance(AZ::Vector3(0.7f, 1.5f, 0.4f), 0.1f));
        EXPECT_THAT(debugDrawAabb.GetMax(), IsCloseTolerance(AZ::Vector3(4.9f, 6.0f, 5.4f), 0.1f));
    }
}
