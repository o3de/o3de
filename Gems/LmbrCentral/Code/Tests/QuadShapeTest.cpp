/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <LmbrCentral/Shape/QuadShapeComponentBus.h>
#include <Shape/QuadShapeComponent.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzFramework/UnitTest/TestDebugDisplayRequests.h>

namespace
{
    const uint32_t QuadCount = 5;

    // Various transforms for quads
    const AZStd::array<AZ::Transform, QuadCount> QuadTransforms =
    {
        AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3( 1.0f,   2.0f,  3.0f), AZ::Transform::Axis::ZPositive),
        AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(-5.0f,   3.0f, -2.0f), AZ::Transform::Axis::ZPositive),
        AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3( 2.0f, -10.0f,  5.0f), AZ::Transform::Axis::ZPositive),
        AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(-5.0f,  -2.0f, -1.0f), AZ::Transform::Axis::ZPositive),
        AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(-1.0f,  -7.0f,  2.0f), AZ::Transform::Axis::ZPositive),
    };

    // Various width/height for quads
    const AZStd::array<LmbrCentral::QuadShapeConfig, QuadCount> QuadDims =
    {
        LmbrCentral::QuadShapeConfig(0.5f, 1.0f),
        LmbrCentral::QuadShapeConfig(2.0f, 4.0f),
        LmbrCentral::QuadShapeConfig(3.0f, 3.0f),
        LmbrCentral::QuadShapeConfig(4.0f, 2.0f),
        LmbrCentral::QuadShapeConfig(1.0f, 0.5f),
    };

    const uint32_t RayCountQuad = 5;

    // Various normalized offset directions from center of quad along quad's surface.
    const AZStd::array<AZ::Vector3, RayCountQuad> OffsetsFromCenterQuad =
    {
        AZ::Vector3( 0.18f, -0.50f, 0.0f).GetNormalized(),
        AZ::Vector3(-0.08f,  0.59f, 0.0f).GetNormalized(),
        AZ::Vector3( 0.92f,  0.94f, 0.0f).GetNormalized(),
        AZ::Vector3(-0.10f, -0.99f, 0.0f).GetNormalized(),
        AZ::Vector3(-0.44f,  0.48f, 0.0f).GetNormalized(),
    };

    // Various directions away from a point on the quad's surface
    const AZStd::array<AZ::Vector3, RayCountQuad> OffsetsFromSurfaceQuad =
    {
        AZ::Vector3( 0.69f,  0.38f,  0.09f).GetNormalized(),
        AZ::Vector3(-0.98f, -0.68f, -0.28f).GetNormalized(),
        AZ::Vector3(-0.45f,  0.31f, -0.05f).GetNormalized(),
        AZ::Vector3( 0.51f, -0.75f,  0.73f).GetNormalized(),
        AZ::Vector3(-0.99f,  0.56f,  0.41f).GetNormalized(),
    };

    // Various distance away from the surface for the rays
    const AZStd::array<float, RayCountQuad> RayDistancesQuad =
    {
        0.5f, 1.0f, 2.0f, 4.0f, 8.0f
    };
}

namespace UnitTest
{
    class QuadShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_quadShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_quadShapeDebugDisplayComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_nonUniformScaleComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_transformShapeComponentDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_quadShapeComponentDescriptor.reset(LmbrCentral::QuadShapeComponent::CreateDescriptor());
            m_quadShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_quadShapeDebugDisplayComponentDescriptor.reset(LmbrCentral::QuadShapeDebugDisplayComponent::CreateDescriptor());
            m_quadShapeDebugDisplayComponentDescriptor->Reflect(&(*m_serializeContext));
            m_nonUniformScaleComponentDescriptor.reset(AzFramework::NonUniformScaleComponent::CreateDescriptor());
            m_nonUniformScaleComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformShapeComponentDescriptor.reset();
            m_quadShapeComponentDescriptor.reset();
            m_quadShapeDebugDisplayComponentDescriptor.reset();
            m_nonUniformScaleComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }
    };

    void CreateQuad(const AZ::Transform& transform, float width, float height, AZ::Entity& entity)
    {
        entity.CreateComponent<AzFramework::TransformComponent>();
        entity.CreateComponent<LmbrCentral::QuadShapeComponent>();
        entity.CreateComponent<LmbrCentral::QuadShapeDebugDisplayComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::QuadShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::QuadShapeComponentRequests::SetQuadWidth, width);
        LmbrCentral::QuadShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::QuadShapeComponentRequests::SetQuadHeight, height);
    }

    void CreateUnitQuad(const AZ::Vector3& position, AZ::Entity& entity)
    {
        CreateQuad(AZ::Transform::CreateTranslation(position), 0.5f, 0.5f, entity);
    }

    void CreateUnitQuadAtOrigin(AZ::Entity& entity)
    {
        CreateUnitQuad(AZ::Vector3::CreateZero(), entity);
    }

    void CreateQuadWithNonUniformScale(const AZ::Transform& transform, const AZ::Vector3& nonUniformScale,
        float width, float height, AZ::Entity& entity)
    {
        entity.CreateComponent<AzFramework::TransformComponent>();
        entity.CreateComponent<LmbrCentral::QuadShapeComponent>();
        entity.CreateComponent<LmbrCentral::QuadShapeDebugDisplayComponent>();
        entity.CreateComponent<AzFramework::NonUniformScaleComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::QuadShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::QuadShapeComponentRequests::SetQuadWidth, width);
        LmbrCentral::QuadShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::QuadShapeComponentRequests::SetQuadHeight, height);
        AZ::NonUniformScaleRequestBus::Event(entity.GetId(), &AZ::NonUniformScaleRequests::SetScale, nonUniformScale);
    }

    void CheckQuadDistance(const AZ::Entity& entity, const AZ::Transform& transform, const AZ::Vector3& point,
        float expectedDistance, float epsilon = 0.01f)
    {
        // Check distance between quad and point at center of quad.
        float distance = -1.0f;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, transform.TransformPoint(point));
        EXPECT_NEAR(distance, expectedDistance, epsilon);
    }

    // Tests

    TEST_F(QuadShapeTest, SetWidthHeightIsPropagatedToGetConfiguration)
    {
        AZ::Entity entity;
        CreateUnitQuadAtOrigin(entity);

        const float newWidth = 123.456f;
        const float newHeight = 654.321f;
        LmbrCentral::QuadShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::QuadShapeComponentRequests::SetQuadWidth, newWidth);
        LmbrCentral::QuadShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::QuadShapeComponentRequests::SetQuadHeight, newHeight);

        LmbrCentral::QuadShapeConfig config{ -1.0f, -1.0f };
        LmbrCentral::QuadShapeComponentRequestBus::EventResult(config, entity.GetId(),
            &LmbrCentral::QuadShapeComponentRequestBus::Events::GetQuadConfiguration);

        EXPECT_FLOAT_EQ(newWidth, config.m_width);
        EXPECT_FLOAT_EQ(newHeight, config.m_height);
    }

    TEST_F(QuadShapeTest, GetTransformAndLocalBoundsWithNonUniformScale)
    {
        AZ::Entity entity;
        AZ::Transform transformIn = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.46f, 0.34f, 0.02f, 0.82f), AZ::Vector3(1.7f, -0.4f, 2.3f));
        transformIn.MultiplyByUniformScale(2.2f);
        const AZ::Vector3 nonUniformScale(0.8f, 0.6f, 1.3f);
        const float width = 0.7f;
        const float height = 1.3f;
        CreateQuadWithNonUniformScale(transformIn, nonUniformScale, width, height, entity);

        AZ::Transform transformOut;
        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::Event(entity.GetId(),
            &LmbrCentral::ShapeComponentRequests::GetTransformAndLocalBounds, transformOut, aabb);

        EXPECT_THAT(transformOut, IsClose(transformIn));
        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(-0.28f, -0.39f, 0.0f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(0.28f, 0.39f, 0.0f)));
    }

    TEST_F(QuadShapeTest, IsPointInsideQuad)
    {
        AZ::Entity entity;
        const AZ::Vector3 center(1.0f, 2.0f, 3.0f);
        const AZ::Vector3 origin = AZ::Vector3::CreateZero();
        CreateUnitQuad(center, entity);

        bool isInside = true; // Initialize to opposite of what's expected to ensure the bus call runs.

        // Check point outside of quad
        LmbrCentral::ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(),
            &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, origin);
        EXPECT_FALSE(isInside);

        // Check point at center of quad (should also return false since a quad is 2D has no inside.
        isInside = true;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(),
            &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, center);
        EXPECT_FALSE(isInside);
    }

    TEST_F(QuadShapeTest, GetRayIntersectQuadSuccess)
    {
        // Check simple case - a quad with normal facing down the Z axis intesecting with a ray going down the Z axis

        AZ::Entity entity;
        CreateUnitQuad(AZ::Vector3(0.0f, 0.0f, 5.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(0.0f, 0.0f, 10.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 5.0f, 1e-4f);

        // More complicated cases - construct rays that should intersect by starting from hit points already on the quads and working backwards

        // Create quad entities with test data in test class.
        AZ::Entity quadEntities[QuadCount];
        for (uint32_t i = 0; i < QuadCount; ++i)
        {
            CreateQuad(QuadTransforms[i], QuadDims[i].m_width, QuadDims[i].m_height, quadEntities[i]);
        }

        // Construct rays and test against the different quads
        for (uint32_t quadIndex = 0; quadIndex < QuadCount; ++quadIndex)
        {
            for (uint32_t rayIndex = 0; rayIndex < RayCountQuad; ++rayIndex)
            {
                // OffsetsFromCenterQuad are all less than 1, so scale by the dimensions of the quad.
                AZ::Vector3 scaledWidthHeight = AZ::Vector3(QuadDims[quadIndex].m_width, QuadDims[quadIndex].m_height, 0.0f);
                // Scale the offset and multiply by 0.5 because distance from center is half the width/height
                AZ::Vector3 scaledOffsetFromCenter = OffsetsFromCenterQuad[rayIndex] * scaledWidthHeight * 0.5f;
                AZ::Vector3 positionOnQuadSurface = QuadTransforms[quadIndex].TransformPoint(scaledOffsetFromCenter);
                AZ::Vector3 rayOrigin = positionOnQuadSurface + OffsetsFromSurfaceQuad[rayIndex] * RayDistancesQuad[rayIndex];

                bool rayHit2 = false;
                float distance2;
                LmbrCentral::ShapeComponentRequestsBus::EventResult(
                    rayHit2, quadEntities[quadIndex].GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
                    rayOrigin, -OffsetsFromSurfaceQuad[rayIndex], distance2);

                EXPECT_TRUE(rayHit2);
                EXPECT_NEAR(distance2, RayDistancesQuad[rayIndex], 1e-4f);
            }
        }

    }

    TEST_F(QuadShapeTest, GetRayIntersectQuadFail)
    {
        // Check simple case - a quad with normal facing down the Z axis intesecting with a ray going down the Z axis, but the ray is offset enough to miss.

        AZ::Entity entity;
        CreateUnitQuad(AZ::Vector3(0.0f, 0.0f, 5.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(0.0f, 2.0f, 10.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_FALSE(rayHit);

        // More complicated cases - construct rays that should not intersect by starting from points on the quad plane but outside the quad, and working backwards

        // Create quad entities with test data in test class.
        AZStd::array<AZ::Entity, QuadCount> quadEntities;
        for (uint32_t i = 0; i < QuadCount; ++i)
        {
            CreateQuad(QuadTransforms[i], QuadDims[i].m_width, QuadDims[i].m_height, quadEntities[i]);
        }

        // Construct rays and test against the different quads
        for (uint32_t quadIndex = 0; quadIndex < QuadCount; ++quadIndex)
        {
            for (uint32_t rayIndex = 0; rayIndex < RayCountQuad; ++rayIndex)
            {
                // OffsetsFromCenterQuad are all less than 1, so scale by the dimensions of the quad.
                AZ::Vector3 scaledWidthHeight = AZ::Vector3(QuadDims[quadIndex].m_width, QuadDims[quadIndex].m_height, 0.0f);
                // Scale the offset and add 1.0 to OffsetsFromCenterQuad to ensure the point is outside the quad.
                AZ::Vector3 scaledOffsetFromCenter = (AZ::Vector3::CreateOne() + OffsetsFromCenterQuad[rayIndex]) * scaledWidthHeight;
                AZ::Vector3 positionOnQuadSurface = QuadTransforms[quadIndex].TransformPoint(scaledOffsetFromCenter);
                AZ::Vector3 rayOrigin = positionOnQuadSurface + OffsetsFromSurfaceQuad[rayIndex] * RayDistancesQuad[rayIndex];

                bool rayHit2 = false;
                float distance2;
                LmbrCentral::ShapeComponentRequestsBus::EventResult(
                    rayHit2, quadEntities[quadIndex].GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
                    rayOrigin, -OffsetsFromSurfaceQuad[rayIndex], distance2);

                EXPECT_FALSE(rayHit2);
            }
        }
    }

    TEST_F(QuadShapeTest, GetRayIntersectQuadNonUniformScaled)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.64f, 0.16f, 0.68f, 0.32f), AZ::Vector3(0.4f, -2.3f, -0.9f));
        transform.MultiplyByUniformScale(1.3f);
        const AZ::Vector3 nonUniformScale(0.7f, 0.5f, 1.3f);
        const float width = 0.9f;
        const float height = 1.3f;
        CreateQuadWithNonUniformScale(transform, nonUniformScale, width, height, entity);

        // a ray which should hit the quad very close to the edge
        AZ::Vector3 rayOrigin(0.2f, -2.3f, -0.6f);
        AZ::Vector3 rayDirection = AZ::Vector3(1.2f, -0.4f, 2.6f).GetNormalized();
        bool rayHit = false;
        float distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            rayOrigin, rayDirection, distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 0.2847f, 1e-3f);

        // move the origin of the ray very slightly so that the ray now just misses the quad
        rayOrigin -= AZ::Vector3(0.1f, 0.0f, 0.0f);
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            rayOrigin, rayDirection, distance);
        EXPECT_FALSE(rayHit);
    }

    TEST_F(QuadShapeTest, GetAabbNotTransformed)
    {
        AZ::Entity entity;
        CreateQuad(AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), 2.0f, 4.0f, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-1.0f, -2.0f, 0.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(1.0f, 2.0f, 0.0f)));
    }

    TEST_F(QuadShapeTest, GetAabbTranslated)
    {
        AZ::Entity entity;
        CreateQuad(AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, 3.0f, 4.0f)), 2.0f, 4.0f, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(1.0f, 1.0f, 4.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(3.0f, 5.0f, 4.0f)));
    }

    TEST_F(QuadShapeTest, GetAabbTranslatedScaled)
    {
        AZ::Entity entity;
        CreateQuad(
            AZ::Transform::CreateTranslation(AZ::Vector3(100.0f, 200.0f, 300.0f)) *
            AZ::Transform::CreateUniformScale(2.5f),
            1.0f, 2.0f, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3( 98.75f, 197.50f, 300.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(101.25f, 202.50f, 300.0f)));
    }

    TEST_F(QuadShapeTest, GetAabbRotated)
    {
        const LmbrCentral::QuadShapeConfig quadShape { 2.0f /*width*/, 3.0f /*height*/};

        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(1.0f, 2.0f, 3.0f),
            AZ::Transform::Axis::ZPositive);
        CreateQuad(transform, quadShape.m_width, quadShape.m_height, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        // Test against an Aabb made by sampling points at corners.
        AZ::Aabb encompasingAabb = AZ::Aabb::CreateNull();
        AZStd::array<AZ::Vector3, 4> corners = quadShape.GetCorners();
        for (uint32_t i = 0; i < corners.size(); ++i)
        {
            encompasingAabb.AddPoint(transform.TransformPoint(corners[i]));
        }

        EXPECT_TRUE(aabb.GetMin().IsClose(encompasingAabb.GetMin()));
        EXPECT_TRUE(aabb.GetMax().IsClose(encompasingAabb.GetMax()));
    }

    TEST_F(QuadShapeTest, GetAabbRotatedTranslatedAndNonUniformScaled)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.44f, 0.24f, 0.48f, 0.72f), AZ::Vector3(3.4f, 1.2f, -2.8f));
        transform.MultiplyByUniformScale(1.5f);
        const AZ::Vector3 nonUniformScale(1.2f, 1.1f, 0.8f);
        const float width = 1.2f;
        const float height = 1.7f;
        CreateQuadWithNonUniformScale(transform, nonUniformScale, width, height, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_THAT(aabb.GetMin(), IsClose(AZ::Vector3(2.2689f, 0.0122f, -4.0947f)));
        EXPECT_THAT(aabb.GetMax(), IsClose(AZ::Vector3(4.5311f, 2.3878f, -1.5053f)));
    }

    TEST_F(QuadShapeTest, IsPointInsideAlwaysFail)
    {
        // Shapes implement the concept of inside strictly, where a point on the surface is not counted
        // as being inside. Therefore a 2D shape like quad has no inside and should always return false.

        AZ::Entity entity;
        bool inside;
        CreateUnitQuadAtOrigin(entity);

        // Check a point at the center of the quad
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3::CreateZero());
        EXPECT_FALSE(inside);

        // Check a point clearly outside the quad
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(100.f, 10.0f, 10.0f));
        EXPECT_FALSE(inside);
    }

    TEST_F(QuadShapeTest, DistanceFromPoint)
    {
        const uint32_t dimCount = 2;
        const uint32_t transformCount = 3;
        const AZ::Vector2 dims[dimCount] = { AZ::Vector2 { 0.5f, 2.0f }, AZ::Vector2 { 1.5f, 0.25f } };
        AZ::Transform transforms[transformCount] =
        {
            AZ::Transform::CreateIdentity(),
            AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3( 1.0f,  2.0f,  3.0f), AZ::Transform::Axis::ZPositive),
            AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(-3.0f, -2.0f, -1.0f), AZ::Transform::Axis::ZPositive),
        };

        for (uint32_t dimIndex = 0; dimIndex < dimCount; ++dimIndex)
        {
            const AZ::Vector2 dim = dims[dimIndex];

            for (uint32_t transformIndex = 0; transformIndex < transformCount; ++transformIndex)
            {
                const AZ::Transform& transform = transforms[transformIndex];

                AZ::Entity entity;
                CreateQuad(transform, dim.GetX(), dim.GetY(), entity);

                AZ::Vector2 offset = dim * 0.5f;

                // Check distance between quad and point at center of quad.
                CheckQuadDistance(entity, transform, AZ::Vector3(0.0f, 0.0f, 0.0f), 0.0f);

                // Check distance between quad and points on edge of quad.
                CheckQuadDistance(entity, transform, AZ::Vector3(offset.GetX(), 0.0f, 0.0f), 0.0f);
                CheckQuadDistance(entity, transform, AZ::Vector3(0.0f, -offset.GetY(), 0.0f), 0.0f);
                CheckQuadDistance(entity, transform, AZ::Vector3(-offset.GetX(), offset.GetY(), 0.0f), 0.0f);

                // Check distance between quad and point 1 unit directly in front of it.
                CheckQuadDistance(entity, transform, AZ::Vector3(0.0f, 0.0f, 1.0f), 1.0f);

                // Check distance between quad and point 1 unit directly to the side of the edge
                CheckQuadDistance(entity, transform, AZ::Vector3(0.0f, offset.GetY() + 1.0f, 0.0f), 1.0f);
                CheckQuadDistance(entity, transform, AZ::Vector3(offset.GetX() + 1.0f, 0.0f, 0.0f), 1.0f);
                CheckQuadDistance(entity, transform, AZ::Vector3(offset.GetX() + 1.0f, offset.GetY() + 1.0f, 0.0f), sqrtf(2.0f)); // offset 1 in both x and y from corner = sqrt(1*1 + 1*1)

                // Check distance between quad and a point 1 up and 1 to the sides and corner of it
                CheckQuadDistance(entity, transform, AZ::Vector3(0.0f, offset.GetY() + 1.0f, 1.0f), sqrtf(2.0f));
                CheckQuadDistance(entity, transform, AZ::Vector3(offset.GetX() + 1.0f, 0.0f, 1.0f), sqrtf(2.0f));
                CheckQuadDistance(entity, transform, AZ::Vector3(offset.GetX() + 1.0f, offset.GetY() + 1.0f, 1.0f), sqrtf(3.0f)); // sqrt(1*1 + 1*1 + 1*1)

                // Check distance between quad and a point 1 up and 3 to the side of it
                CheckQuadDistance(entity, transform, AZ::Vector3(0.0f, offset.GetY() + 3.0f, 1.0f), sqrtf(10.0f)); // sqrt(3*3 + 1*1)
                CheckQuadDistance(entity, transform, AZ::Vector3(offset.GetX() + 3.0f, 0.0f, 1.0f), sqrtf(10.0f));
                CheckQuadDistance(entity, transform, AZ::Vector3(offset.GetX() + 3.0f, offset.GetY() + 3.0f, 1.0f), sqrtf(19.0f)); // sqrt(3*3 + 3*3 + 1*1)
            }
        }
    }

    TEST_F(QuadShapeTest, DistanceFromPointNonUniformScaled)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.24f, 0.72f, 0.44f, 0.48f), AZ::Vector3(2.7f, 2.3f, -1.8f));
        transform.MultiplyByUniformScale(1.2f);
        const AZ::Vector3 nonUniformScale(0.4f, 2.2f, 1.3f);
        const float width = 1.6f;
        const float height = 0.7f;
        CreateQuadWithNonUniformScale(transform, nonUniformScale, width, height, entity);

        // a point closest to the interior of the quad
        float distance = AZ::Constants::FloatMax;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(3.1f, 2.3f, -2.6f));
        EXPECT_NEAR(distance, 0.4826f, 1e-3f);

        // a point closest to an edge of the quad
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(2.8f, 1.8f, -1.3f));
        EXPECT_NEAR(distance, 0.3389f, 1e-3f);

        // a point closest to a corner of the quad
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, AZ::Vector3(3.0f, 2.3f, -3.3f));
        EXPECT_NEAR(distance, 0.6696f, 1e-3f);
    }

    TEST_F(QuadShapeTest, DebugDraw)
    {
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion(0.70f, 0.10f, 0.34f, 0.62f), AZ::Vector3(3.0f, -1.0f, 2.0f));
        transform.MultiplyByUniformScale(2.0f);
        const AZ::Vector3 nonUniformScale(2.4f, 1.3f, 1.8f);
        const float width = 0.8f;
        const float height = 1.4f;
        CreateQuadWithNonUniformScale(transform, nonUniformScale, width, height, entity);

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
} // namespace UnitTest
