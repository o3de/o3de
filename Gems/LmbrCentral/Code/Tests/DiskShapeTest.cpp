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
#include <LmbrCentral/Shape/DiskShapeComponentBus.h>
#include <Shape/DiskShapeComponent.h>

namespace
{
    const uint32_t DiskCount = 5;

    // Various transforms for disks
    const AZStd::array<AZ::Transform, DiskCount> DiskTransforms =
    {
        AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(1.0, 2.0, 3.0), AZ::Transform::Axis::ZPositive),
        AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(-5.0, 3.0, -2.0), AZ::Transform::Axis::ZPositive),
        AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(2.0, -10.0, 5.0), AZ::Transform::Axis::ZPositive),
        AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(-5.0, -2.0, -1.0), AZ::Transform::Axis::ZPositive),
        AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(-1.0, -7.0, 2.0), AZ::Transform::Axis::ZPositive),
    };

    // Various radii for disks
    const AZStd::array<float, DiskCount> DiskRadii =
    {
        0.5f, 1.0f, 2.0f, 4.0f, 8.0f,
    };

    const uint32_t RayCountDisk = 5;

    // Various normalized offset directions from center of disk along disk's surface.
    const AZStd::array<AZ::Vector3, RayCountDisk> OffsetsFromCenterDisk =
    {
        AZ::Vector3(0.18f, -0.50f, 0.0f).GetNormalized(),
        AZ::Vector3(-0.08f,  0.59f, 0.0f).GetNormalized(),
        AZ::Vector3(0.92f,  0.94f, 0.0f).GetNormalized(),
        AZ::Vector3(-0.10f, -0.99f, 0.0f).GetNormalized(),
        AZ::Vector3(-0.44f,  0.48f, 0.0f).GetNormalized(),
    };

    // Various directions away from a point on the disk's surface
    const AZStd::array<AZ::Vector3, RayCountDisk> OffsetsFromSurfaceDisk =
    {
        AZ::Vector3(0.69f,  0.38f,  0.09f).GetNormalized(),
        AZ::Vector3(-0.98f, -0.68f, -0.28f).GetNormalized(),
        AZ::Vector3(-0.45f,  0.31f, -0.05f).GetNormalized(),
        AZ::Vector3(0.51f, -0.75f,  0.73f).GetNormalized(),
        AZ::Vector3(-0.99f,  0.56f,  0.41f).GetNormalized(),
    };

    // Various distance away from the surface for the rays
    const AZStd::array<float, RayCountDisk> RayDistancesDisk =
    {
        0.5f, 1.0f, 2.0f, 4.0f, 8.0f
    };
}

namespace UnitTest
{
    class DiskShapeTest
        : public AllocatorsFixture
    {
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformShapeComponentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_diskShapeComponentDescriptor;

    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_transformShapeComponentDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformShapeComponentDescriptor->Reflect(&(*m_serializeContext));
            m_diskShapeComponentDescriptor.reset(LmbrCentral::DiskShapeComponent::CreateDescriptor());
            m_diskShapeComponentDescriptor->Reflect(&(*m_serializeContext));
        }

        void TearDown() override
        {
            m_transformShapeComponentDescriptor.reset();
            m_diskShapeComponentDescriptor.reset();
            m_serializeContext.reset();
            AllocatorsFixture::TearDown();
        }

    };

    void CreateDisk(const AZ::Transform& transform, const float radius, AZ::Entity& entity)
    {
        entity.CreateComponent<AzFramework::TransformComponent>();
        entity.CreateComponent<LmbrCentral::DiskShapeComponent>();

        entity.Init();
        entity.Activate();

        AZ::TransformBus::Event(entity.GetId(), &AZ::TransformBus::Events::SetWorldTM, transform);
        LmbrCentral::DiskShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::DiskShapeComponentRequests::SetRadius, radius);
    }

    void CreateUnitDisk(const AZ::Vector3& position, AZ::Entity& entity)
    {
        CreateDisk(AZ::Transform::CreateTranslation(position), 0.5f, entity);
    }

    void CreateUnitDiskAtOrigin(AZ::Entity& entity)
    {
        CreateUnitDisk(AZ::Vector3::CreateZero(), entity);
    }

    void CheckDistance(const AZ::Entity& entity, const AZ::Transform& transform, const AZ::Vector3& point, float expectedDistance, float epsilon = 0.001f)
    {
        // Check distance between disk and point at center of disk.
        float distance = -1.0f;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, transform.TransformPoint(point));
        EXPECT_NEAR(distance, expectedDistance, epsilon);
    }

    // Tests

    TEST_F(DiskShapeTest, SetRadiusIsPropagatedToGetConfiguration)
    {
        AZ::Entity entity;
        CreateUnitDiskAtOrigin(entity);

        const float newRadius = 123.456f;
        LmbrCentral::DiskShapeComponentRequestBus::Event(entity.GetId(), &LmbrCentral::DiskShapeComponentRequests::SetRadius, newRadius);

        LmbrCentral::DiskShapeConfig config(-1.0f);
        LmbrCentral::DiskShapeComponentRequestBus::EventResult(config, entity.GetId(), &LmbrCentral::DiskShapeComponentRequestBus::Events::GetDiskConfiguration);

        EXPECT_FLOAT_EQ(newRadius, config.m_radius);
    }

    TEST_F(DiskShapeTest, IsPointInsideDisk)
    {
        AZ::Entity entity;
        const AZ::Vector3 center(1.0f, 2.0f, 3.0f);
        const AZ::Vector3 origin = AZ::Vector3::CreateZero();
        CreateUnitDisk(center, entity);

        bool isInside = true; // Initialize to opposite of what's expected to ensure the bus call runs.

        // Check point outside of disk
        LmbrCentral::ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, origin);
        EXPECT_FALSE(isInside);

        // Check point at center of disk (should also return false since a disk is 2D has no inside.
        isInside = true;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(isInside, entity.GetId(), &LmbrCentral::ShapeComponentRequestsBus::Events::IsPointInside, center);
        EXPECT_FALSE(isInside);
    }
    
    TEST_F(DiskShapeTest, GetRayIntersectDiskSuccess)
    {
        // Check simple case - a disk with normal facing down the Z axis intesecting with a ray going down the Z axis

        AZ::Entity entity;
        CreateUnitDisk(AZ::Vector3(0.0f, 0.0f, 5.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(0.0f, 0.0f, 10.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_TRUE(rayHit);
        EXPECT_NEAR(distance, 5.0f, 1e-4f);

        // More complicated cases - construct rays that should intersect by starting from hit points already on the disks and working backwards

        // Create disk entities with test data in test class.
        AZ::Entity diskEntities[DiskCount];
        for (uint32_t i = 0; i < DiskCount; ++i)
        {
            CreateDisk(DiskTransforms[i], DiskRadii[i], diskEntities[i]);
        }

        // Offsets from center scaled down from the disk edge so that all the rays should hit
        const AZStd::array<float, RayCountDisk> offsetFromCenterScale =
        {
            0.8f,
            0.2f,
            0.5f,
            0.9f,
            0.1f,
        };

        // Construct rays and test against the different disks
        for (uint32_t diskIndex = 0; diskIndex < DiskCount; ++diskIndex)
        {
            for (uint32_t rayIndex = 0; rayIndex < RayCountDisk; ++rayIndex)
            {
                AZ::Vector3 scaledOffsetFromCenter = OffsetsFromCenterDisk[rayIndex] * DiskRadii[diskIndex] * offsetFromCenterScale[rayIndex];
                AZ::Vector3 positionOnDiskSurface = DiskTransforms[diskIndex].TransformPoint(scaledOffsetFromCenter);
                AZ::Vector3 rayOrigin = positionOnDiskSurface + OffsetsFromSurfaceDisk[rayIndex] * RayDistancesDisk[rayIndex];

                bool rayHit2 = false;
                float distance2;
                LmbrCentral::ShapeComponentRequestsBus::EventResult(
                    rayHit2, diskEntities[diskIndex].GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
                    rayOrigin, -OffsetsFromSurfaceDisk[rayIndex], distance2);

                EXPECT_TRUE(rayHit2);
                EXPECT_NEAR(distance2, RayDistancesDisk[rayIndex], 1e-4f);
            }
        }

    }

    TEST_F(DiskShapeTest, GetRayIntersectDiskFail)
    {
        // Check simple case - a disk with normal facing down the Z axis intesecting with a ray going down the Z axis, but the ray is offset enough to miss.

        AZ::Entity entity;
        CreateUnitDisk(AZ::Vector3(0.0f, 0.0f, 5.0f), entity);

        bool rayHit = false;
        float distance;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            rayHit, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
            AZ::Vector3(0.0f, 2.0f, 10.0f), AZ::Vector3(0.0f, 0.0f, -1.0f), distance);

        EXPECT_FALSE(rayHit);

        // More complicated cases - construct rays that should not intersect by starting from points on the disk plane but outside the disk, and working backwards

        // Create disk entities with test data in test class.
        AZStd::array<AZ::Entity, DiskCount> diskEntities;
        for (uint32_t i = 0; i < DiskCount; ++i)
        {
            CreateDisk(DiskTransforms[i], DiskRadii[i], diskEntities[i]);
        }

        // Offsets from center scaled up from the disk edge so that all the rays should miss
        const AZStd::array<float, RayCountDisk> offsetFromCenterScale =
        {
            1.8f,
            1.2f,
            1.5f,
            1.9f,
            1.1f,
        };

        // Construct rays and test against the different disks
        for (uint32_t diskIndex = 0; diskIndex < DiskCount; ++diskIndex)
        {
            for (uint32_t rayIndex = 0; rayIndex < RayCountDisk; ++rayIndex)
            {
                AZ::Vector3 scaledOffsetFromCenter = OffsetsFromCenterDisk[rayIndex] * DiskRadii[diskIndex] * offsetFromCenterScale[rayIndex];
                AZ::Vector3 positionOnDiskSurface = DiskTransforms[diskIndex].TransformPoint(scaledOffsetFromCenter);
                AZ::Vector3 rayOrigin = positionOnDiskSurface + OffsetsFromSurfaceDisk[rayIndex] * RayDistancesDisk[rayIndex];

                bool rayHit2 = false;
                float distance2;
                LmbrCentral::ShapeComponentRequestsBus::EventResult(
                    rayHit2, diskEntities[diskIndex].GetId(), &LmbrCentral::ShapeComponentRequests::IntersectRay,
                    rayOrigin, -OffsetsFromSurfaceDisk[rayIndex], distance2);

                EXPECT_FALSE(rayHit2);
            }
        }

    }

    TEST_F(DiskShapeTest, GetAabbNotTransformed)
    {
        AZ::Entity entity;
        CreateDisk(AZ::Transform::CreateTranslation(AZ::Vector3::CreateZero()), 2.0f, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(-2.0f, -2.0f, 0.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(2.0f, 2.0f, 0.0f)));
    }

    TEST_F(DiskShapeTest, GetAabbTranslated)
    {
        AZ::Entity entity;
        CreateDisk(AZ::Transform::CreateTranslation(AZ::Vector3(2.0f, 3.0f, 4.0f)), 2.0f, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(0.0f, 1.0f, 4.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(4.0f, 5.0f, 4.0f)));
    }

    TEST_F(DiskShapeTest, GetAabbTranslatedScaled)
    {
        AZ::Entity entity;
        CreateDisk(
            AZ::Transform::CreateTranslation(AZ::Vector3(100.0f, 200.0f, 300.0f)) *
            AZ::Transform::CreateUniformScale(2.5f),
            0.5f, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        EXPECT_TRUE(aabb.GetMin().IsClose(AZ::Vector3(98.75f, 198.75f, 300.0f)));
        EXPECT_TRUE(aabb.GetMax().IsClose(AZ::Vector3(101.25f, 201.25f, 300.0f)));
    }

    TEST_F(DiskShapeTest, GetAabbRotated)
    {
        const float radius = 0.5f;
        AZ::Entity entity;
        AZ::Transform transform = AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(1.0, 2.0, 3.0), AZ::Transform::Axis::ZPositive);
        CreateDisk(transform, radius, entity);

        AZ::Aabb aabb;
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            aabb, entity.GetId(), &LmbrCentral::ShapeComponentRequests::GetEncompassingAabb);

        // Test against an AZ::Aabb made by sampling many points along the edge.
        AZ::Aabb encompasingAabb = AZ::Aabb::CreateNull();
        const uint32_t numSamples = 1000;
        for (uint32_t i = 0; i < numSamples; ++i)
        {
            const float angle = (aznumeric_cast<float>(i) / aznumeric_cast<float>(numSamples)) * AZ::Constants::TwoPi;
            AZ::Vector3 offsetFromCenter = AZ::Vector3(cos(angle), sin(angle), 0.0f) * radius;
            offsetFromCenter = transform.TransformPoint(offsetFromCenter);
            encompasingAabb.AddPoint(offsetFromCenter);
        }

        EXPECT_TRUE(aabb.GetMin().IsClose(encompasingAabb.GetMin()));
        EXPECT_TRUE(aabb.GetMax().IsClose(encompasingAabb.GetMax()));
    }

    TEST_F(DiskShapeTest, IsPointInsideAlwaysFail)
    {
        // Shapes implement the concept of inside strictly, where a point on the surface is not counted
        // as being inside. Therefore a 2D shape like disk has no inside and should always return false.

        AZ::Entity entity;
        bool inside;
        CreateUnitDiskAtOrigin(entity);

        // Check a point at the center of the disk
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3::CreateZero());
        EXPECT_FALSE(inside);

        // Check a point clearly outside the disk
        LmbrCentral::ShapeComponentRequestsBus::EventResult(
            inside, entity.GetId(), &LmbrCentral::ShapeComponentRequests::IsPointInside, AZ::Vector3(100.f, 10.0f, 10.0f));
        EXPECT_FALSE(inside);
    }

    TEST_F(DiskShapeTest, DistanceFromPoint)
    {
        float distance = 0.0f;
        const float epsilon = 0.001f;

        const uint32_t radiiCount = 2;
        const uint32_t transformCount = 3;
        const float radii[radiiCount] = { 0.5f, 2.0f };
        AZ::Transform transforms[transformCount] =
        {
            AZ::Transform::CreateIdentity(),
            AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(1.0, 2.0, 3.0), AZ::Transform::Axis::ZPositive),
            AZ::Transform::CreateLookAt(AZ::Vector3::CreateZero(), AZ::Vector3(-3.0, -2.0, -1.0), AZ::Transform::Axis::ZPositive),
        };

        for (uint32_t radiusIndex = 0; radiusIndex < radiiCount; ++radiusIndex)
        {
            const float radius = radii[radiusIndex];

            for (uint32_t transformIndex = 0; transformIndex < transformCount; ++transformIndex)
            {
                const AZ::Transform& transform = transforms[transformIndex];

                AZ::Entity entity;
                CreateDisk(transform, radius, entity);

                // Check distance between disk and point at center of disk.
                CheckDistance(entity, transform, AZ::Vector3(0.0f, 0.0f, 0.0f), 0.0f);

                // Check distance between disk and point on edge of disk.
                CheckDistance(entity, transform, AZ::Vector3(0.0f, radius, 0.0f), 0.0f);

                // Check distance between disk and point 1 unit directly in front of it.
                CheckDistance(entity, transform, AZ::Vector3(0.0f, 0.0f, 1.0f), 1.0f);

                // Check distance between disk and point 1 unit directly to the side of the edge
                CheckDistance(entity, transform, AZ::Vector3(0.0f, radius + 1.0f, 0.0f), 1.0f);

                // Check distance between disk and a point 1 up and 1 to the side of it
                CheckDistance(entity, transform, AZ::Vector3(0.0f, radius + 1.0f, 1.0f), sqrt(2.0f));

                // Check distance between disk and a point 1 up and 3 to the side of it
                CheckDistance(entity, transform, AZ::Vector3(0.0f, radius + 3.0f, 1.0f), sqrt(10.0f));

                // Check distance between disk and a point 1 up and 1 to the side of it in x and y
                float a = radius + 1.0f;
                float b = radius + 1.0f;
                float distAlongPlaneFromEdge = sqrt(a * a + b * b) - radius;
                float diagonalDist = sqrt(distAlongPlaneFromEdge * distAlongPlaneFromEdge + 1.0f);
                CheckDistance(entity, transform, AZ::Vector3(radius + 1.0f, radius + 1.0f, 1.0f), diagonalDist);
                distance = -1.0f;
                LmbrCentral::ShapeComponentRequestsBus::EventResult(
                    distance, entity.GetId(), &LmbrCentral::ShapeComponentRequests::DistanceFromPoint, transform.TransformPoint(AZ::Vector3(radius + 1.0f, radius + 1.0f, 1.0f)));
                EXPECT_NEAR(distance, diagonalDist, epsilon);
            }
        }
    }
} // namespace UnitTest
