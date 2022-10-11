/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Math/Random.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Shape/CapsuleShapeComponent.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <ShapeThreadsafeTest.h>

namespace UnitTest
{
    class CapsuleShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_capsuleShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(AzFramework::TransformComponent::CreateDescriptor());
            m_transformComponentDescriptor->Reflect(&(*m_serializeContext));
            m_capsuleShapeComponentDescriptor = AZStd::unique_ptr<AZ::ComponentDescriptor>(LmbrCentral::CapsuleShapeComponent::CreateDescriptor());
            m_capsuleShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformComponentDescriptor.reset();
            m_capsuleShapeComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateCapsule(const AZ::Transform& transform, const float radius, const float height, AZ::Entity& entity)
    {
        entity.CreateComponent<LmbrCentral::CapsuleShapeComponent>();
        entity.CreateComponent<AzFramework::TransformComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);

        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetHeight, height);
        LmbrCentral::CapsuleShapeComponentRequestsBus::Event(
            entity.GetId(), &LmbrCentral::CapsuleShapeComponentRequestsBus::Events::SetRadius, radius);
    }

    TEST_F(CapsuleShapeTest, GetRayIntersectCapsuleSuccess1)
    {
        AZ::Entity entity;
        CreateCapsule(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, 0.0f, 5.0f)),
            0.5f, 5.0f, entity);

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
        CreateCapsule(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi),
            AZ::Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f, 5.0f, entity);

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
        CreateCapsule(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi),
            AZ::Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f, 5.0f, entity);

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
        CreateCapsule(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi),
            AZ::Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f, 0.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(-4.0f, -12.0f, -3.0f)) *
            AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
            AZ::Transform::CreateUniformScale(6.0f),
            0.25f, 1.5f, entity);

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
        CreateCapsule(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, -10.0f, 0.0f)),
            5.0f, 1.0f, entity);

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
        CreateCapsule(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.0f, -10.0f, 0.0f)),
            5.0f, 1.0f, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-5.0f, -15.0f, -5.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(5.0f, -5.0f, 5.0f)));
    }

    TEST_F(CapsuleShapeTest, GetAabb2)
    {
        AZ::Entity entity;
        CreateCapsule(AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisX(), AZ::Constants::HalfPi) *
            AZ::Quaternion::CreateFromAxisAngle(AZ::Vector3::CreateAxisY(), AZ::Constants::QuarterPi), AZ::Vector3(-10.0f, -10.0f, 0.0f)),
            1.0f, 5.0f, entity);

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
        CreateCapsule(AZ::Transform::CreateUniformScale(3.5f), 2.0f, 4.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(5.0f, 20.0f, 0.0f)) *
            AZ::Transform::CreateUniformScale(2.5f), 1.0f, 5.0f, entity);

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
        CreateCapsule(transformIn, 5.0f, 2.0f, entity);

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
        CreateCapsule(transformIn, 5.0f, 2.0f, entity);

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
        CreateCapsule(transformIn, 2.0f, 5.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) *
            AZ::Transform::CreateUniformScale(2.5f),
            0.5f, 2.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) *
            AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
            AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi) *
            AZ::Transform::CreateUniformScale(0.5f),
            0.5f, 2.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) *
            AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
            AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi) *
            AZ::Transform::CreateUniformScale(2.0f),
            0.5f, 4.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3(27.0f, 28.0f, 38.0f)) *
            AZ::Transform::CreateRotationX(AZ::Constants::HalfPi) *
            AZ::Transform::CreateRotationY(AZ::Constants::QuarterPi) *
            AZ::Transform::CreateUniformScale(2.0f),
            0.5f, 4.0f, entity);

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
            AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), ShapeThreadsafeTest::MinDimension,
            ShapeThreadsafeTest::ShapeHeight, entity);

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
}
