/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Sphere.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AZTestShared/Math/MathTestHelpers.h>

namespace UnitTest
{
    TEST(MATH_ShapeIntersection, Test)
    {
        //Assumes +x runs to the 'right', +y runs 'out' and +z points 'up'

        //A frustum is defined by 6 planes. In this case a box shape.
        AZ::Plane near_value = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 1.f, 0.f), AZ::Vector3(0.f, -5.f, 0.f));
        AZ::Plane far_value = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, -1.f, 0.f), AZ::Vector3(0.f, 5.f, 0.f));
        AZ::Plane left = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(1.f, 0.f, 0.f), AZ::Vector3(-5.f, 0.f, 0.f));
        AZ::Plane right = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(-1.f, 0.f, 0.f), AZ::Vector3(5.f, 0.f, 0.f));
        AZ::Plane top = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, -1.f), AZ::Vector3(0.f, 0.f, 5.f));
        AZ::Plane bottom = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, 1.f), AZ::Vector3(0.f, 0.f, -5.f));
        AZ::Frustum frustum(near_value, far_value, left, right, top, bottom);

        AZ::Plane near1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 1.f, 0.f), AZ::Vector3(0.f, -2.f, 0.f));
        AZ::Plane far1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, -1.f, 0.f), AZ::Vector3(0.f, 2.f, 0.f));
        AZ::Plane left1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(1.f, 0.f, 0.f), AZ::Vector3(-2.f, 0.f, 0.f));
        AZ::Plane right1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(-1.f, 0.f, 0.f), AZ::Vector3(2.f, 0.f, 0.f));
        AZ::Plane top1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, -1.f), AZ::Vector3(0.f, 0.f, 2.f));
        AZ::Plane bottom1 = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.f, 0.f, 1.f), AZ::Vector3(0.f, 0.f, -2.f));
        AZ::Frustum frustum1(near1, far1, left1, right1, top1, bottom1);

        AZ::Sphere unitSphere = AZ::Sphere::CreateUnitSphere();
        AZ::Sphere sphere1(AZ::Vector3(10.f, 10.f, 10.f), 20.f);
        AZ::Sphere sphere2(AZ::Vector3(12.f, 12.f, 12.f), 13.f);

        AZ::Aabb unitBox = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), AZ::Vector3(1.f, 1.f, 1.f));
        AZ::Aabb aabb = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3(10.f, 10.f, 10.f), AZ::Vector3(1.f, 1.f, 1.f));
        AZ::Aabb aabb1 = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3(10.f, 10.f, 10.f), AZ::Vector3(100.f, 100.f, 100.f));
        AZ::Aabb maxSizeAabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-AZ::Constants::FloatMax), AZ::Vector3(AZ::Constants::FloatMax));

        AZ::Vector3 point(0.f, 0.f, 0.f);
        AZ::Vector3 point1(10.f, 10.f, 10.f);

        {
            AZ::Vector3 intersectionPoint;
            EXPECT_TRUE(AZ::ShapeIntersection::IntersectThreePlanes(near_value, left, bottom, intersectionPoint));
            EXPECT_THAT(intersectionPoint, IsClose(AZ::Vector3(-5.f, -5.f, -5.f)));
            EXPECT_FALSE(AZ::ShapeIntersection::IntersectThreePlanes(near_value, far_value, bottom, intersectionPoint));
        }

        {
            AZ::Vector3 intersectionPoint;
            EXPECT_TRUE(AZ::ShapeIntersection::IntersectThreePlanes(near_value, left, bottom, intersectionPoint));
            EXPECT_THAT(intersectionPoint, IsClose(AZ::Vector3(-5.f, -5.f, -5.f)));
            EXPECT_FALSE(AZ::ShapeIntersection::IntersectThreePlanes(near_value, far_value, bottom, intersectionPoint));
        }

        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(aabb, aabb1));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(aabb1, aabb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(unitSphere, unitBox));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(unitSphere, unitBox));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(unitSphere, aabb1));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(unitSphere, frustum));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere1, frustum));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(unitSphere, sphere1));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere1, unitSphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(frustum, unitBox));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(frustum, aabb1));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere1, far_value));

        // Verify that an AABB that covers the max floating point range successfully overlaps with a frustum and doesn't hit any
        // floating-point math overflows.
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(frustum, maxSizeAabb));

        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(frustum, aabb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(unitSphere, aabb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(unitSphere, sphere2));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(unitSphere, near_value));

        EXPECT_FALSE(AZ::ShapeIntersection::Contains(aabb, aabb1));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(aabb1, aabb));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(unitBox, unitSphere));
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(aabb, unitSphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(unitBox, unitSphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(aabb1, unitSphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(frustum, unitSphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(unitSphere, unitSphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(sphere1, aabb));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(frustum, unitBox));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(unitSphere, point));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(sphere1, point1));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(frustum, point));

        EXPECT_FALSE(AZ::ShapeIntersection::Contains(frustum, sphere1));
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(unitSphere, unitBox));
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(frustum, aabb));
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(unitSphere, point1));
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(frustum, point1));

        {
            AZ::Sphere s(AZ::Vector3(0.0f, -4.4f, 0.0f), 0.5f);
            AZ::Sphere s1(AZ::Vector3(0.0f, -5.1f, 0.0f), 0.5f);
            AZ::Sphere s2(AZ::Vector3(0.0f, -5.6f, 0.0f), 0.5f);

            EXPECT_EQ(AZ::ShapeIntersection::Classify(near_value, s), AZ::IntersectResult::Interior);
            EXPECT_EQ(AZ::ShapeIntersection::Classify(near_value, s1), AZ::IntersectResult::Overlaps);
            EXPECT_EQ(AZ::ShapeIntersection::Classify(near_value, s2), AZ::IntersectResult::Exterior);

            EXPECT_EQ(AZ::ShapeIntersection::Classify(frustum, s), AZ::IntersectResult::Interior);
            EXPECT_EQ(AZ::ShapeIntersection::Classify(frustum, s1), AZ::IntersectResult::Overlaps);
            EXPECT_EQ(AZ::ShapeIntersection::Classify(frustum, s2), AZ::IntersectResult::Exterior);
        }

        {
            AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
                AZ::Vector3(0.0f, -3.9f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne());
            AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
                AZ::Vector3(0.0f, -5.1f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne());
            AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
                AZ::Vector3(0.0f, -6.1f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne());
            EXPECT_EQ(AZ::ShapeIntersection::Classify(near_value, obb), AZ::IntersectResult::Interior);
            EXPECT_EQ(AZ::ShapeIntersection::Classify(near_value, obb1), AZ::IntersectResult::Overlaps);
            EXPECT_EQ(AZ::ShapeIntersection::Classify(near_value, obb2), AZ::IntersectResult::Exterior);
        }

        //Test a bunch of different rotations with the OBBs
        AZ::Vector3 rotationAxis = AZ::Vector3(1.0f, 1.0f, 1.0f);
        rotationAxis.Normalize();
        float rotation = 0.0f;
        for(int i = 0; i < 50; ++i, rotation += (2.0f*3.1415/50.0f))
        {
            AZ::Quaternion rot = AZ::Quaternion::CreateFromAxisAngle(rotationAxis, rotation);
            AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
                AZ::Vector3(0.7f, 0.7f, 0.7f), rot, AZ::Vector3::CreateOne());
            AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
                AZ::Vector3(6.8f, 6.8f, 6.8f), rot, AZ::Vector3::CreateOne());

            EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(frustum, obb));
            EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(frustum, obb1));
        }
    }

    TEST(MATH_ShapeIntersection, CapsuleCapsuleCapAJustIntersectsCapB)
    {
        const AZ::Vector3 firstHemisphereCenterA(1.0f, 7.0f, 13.0f);
        const AZ::Vector3 secondHemisphereCenterA(2.0f, 3.0f, 5.0f);
        const float radiusA = 5.51f;
        const AZ::Capsule capsuleA(firstHemisphereCenterA, secondHemisphereCenterA, radiusA);
        const AZ::Vector3 firstHemisphereCenterB(3.0f, -1.0f, -3.0f);
        const AZ::Vector3 secondHemisphereCenterB(4.0f, -5.0f, -11.0f);
        const float radiusB = 3.51f;
        const AZ::Capsule capsuleB(firstHemisphereCenterB, secondHemisphereCenterB, radiusB);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsuleA, capsuleB));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsuleB, capsuleA));
    }

    TEST(MATH_ShapeIntersection, CapsuleCapsuleCapAJustSeparatedFromCapB)
    {
        const AZ::Vector3 firstHemisphereCenterA(1.0f, 7.0f, 13.0f);
        const AZ::Vector3 secondHemisphereCenterA(2.0f, 3.0f, 5.0f);
        const float radiusA = 5.49f;
        const AZ::Capsule capsuleA(firstHemisphereCenterA, secondHemisphereCenterA, radiusA);
        const AZ::Vector3 firstHemisphereCenterB(3.0f, -1.0f, -3.0f);
        const AZ::Vector3 secondHemisphereCenterB(4.0f, -5.0f, -11.0f);
        const float radiusB = 3.49f;
        const AZ::Capsule capsuleB(firstHemisphereCenterB, secondHemisphereCenterB, radiusB);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsuleA, capsuleB));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsuleB, capsuleA));
    }

    TEST(MATH_ShapeIntersection, CapsuleCapsuleCapAJustIntersectsCylinderB)
    {
        const AZ::Vector3 firstHemisphereCenterA(1.0f, 3.0f, -5.0f);
        const AZ::Vector3 secondHemisphereCenterA(4.0f, 7.0f, 7.0f);
        const float radiusA = 1.01f;
        const AZ::Capsule capsuleA(firstHemisphereCenterA, secondHemisphereCenterA, radiusA);
        const AZ::Vector3 firstHemisphereCenterB(0.5f, 6.5f, 1.0f);
        const AZ::Vector3 secondHemisphereCenterB(-3.5f, 9.5f, 1.0f);
        const float radiusB = 1.51f;
        const AZ::Capsule capsuleB(firstHemisphereCenterB, secondHemisphereCenterB, radiusB);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsuleA, capsuleB));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsuleB, capsuleA));
    }

    TEST(MATH_ShapeIntersection, CapsuleCapsuleCapAJustSeparatedFromCylinderB)
    {
        const AZ::Vector3 firstHemisphereCenterA(1.0f, 3.0f, -5.0f);
        const AZ::Vector3 secondHemisphereCenterA(4.0f, 7.0f, 7.0f);
        const float radiusA = 0.99f;
        const AZ::Capsule capsuleA(firstHemisphereCenterA, secondHemisphereCenterA, radiusA);
        const AZ::Vector3 firstHemisphereCenterB(0.5f, 6.5f, 1.0f);
        const AZ::Vector3 secondHemisphereCenterB(-3.5f, 9.5f, 1.0f);
        const float radiusB = 1.49f;
        const AZ::Capsule capsuleB(firstHemisphereCenterB, secondHemisphereCenterB, radiusB);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsuleA, capsuleB));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsuleB, capsuleA));
    }

    TEST(MATH_ShapeIntersection, CapsuleCapsuleCompletelyContainedCapsuleB)
    {
        const AZ::Vector3 firstHemisphereCenter(1.0f, 2.0f, 3.0f);
        const AZ::Vector3 secondHemisphereCenter(5.0f, 7.0f, -4.0f);
        const float radiusA = 2.0f;
        const float radiusB = 1.5f;
        const AZ::Capsule capsuleA(firstHemisphereCenter, secondHemisphereCenter, radiusA);
        const AZ::Capsule capsuleB(firstHemisphereCenter, secondHemisphereCenter, radiusB);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsuleA, capsuleB));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsuleB, capsuleA));
    }

    TEST(MATH_ShapeIntersection, CapsuleCapsuleParallelJustOverlapping)
    {
        const AZ::Vector3 firstHemisphereCenterA(2.0f, -3.0f, -7.0f);
        const AZ::Vector3 secondHemisphereCenterA(11.0f, 17.0f, 5.0f);
        const float radiusA = 3.01f;
        const AZ::Capsule capsuleA(firstHemisphereCenterA, secondHemisphereCenterA, radiusA);
        const AZ::Vector3 firstHemisphereCenterB(-2.0f, -3.0f, -4.0f);
        const AZ::Vector3 secondHemisphereCenterB(7.0f, 17.0f, 8.0f);
        const float radiusB = 2.01f;
        const AZ::Capsule capsuleB(firstHemisphereCenterB, secondHemisphereCenterB, radiusB);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsuleA, capsuleB));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsuleB, capsuleA));
    }

    TEST(MATH_ShapeIntersection, CapsuleCapsuleParallelJustSeparate)
    {
        const AZ::Vector3 firstHemisphereCenterA(2.0f, -3.0f, -7.0f);
        const AZ::Vector3 secondHemisphereCenterA(11.0f, 17.0f, 5.0f);
        const float radiusA = 2.99f;
        const AZ::Capsule capsuleA(firstHemisphereCenterA, secondHemisphereCenterA, radiusA);
        const AZ::Vector3 firstHemisphereCenterB(-2.0f, -3.0f, -4.0f);
        const AZ::Vector3 secondHemisphereCenterB(7.0f, 17.0f, 8.0f);
        const float radiusB = 1.99f;
        const AZ::Capsule capsuleB(firstHemisphereCenterB, secondHemisphereCenterB, radiusB);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsuleA, capsuleB));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsuleB, capsuleA));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCompletelyContainedCapsule)
    {
        const AZ::Vector3 firstHemisphereCenter(2.0f, 3.0f, 4.0f);
        const AZ::Vector3 secondHemisphereCenter(3.0f, 4.0f, 6.0f);
        const float radius = 1.0f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(3.0f, 4.0f, 5.0f);
        const AZ::Quaternion obbRotation(0.64f, -0.08f, -0.56f, 0.52f);
        const AZ::Vector3 obbHalfLengths(4.0f, 4.0f, 5.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCompletelyContainedObb)
    {
        const AZ::Vector3 firstHemisphereCenter(-1.0f, 4.0f, 3.0f);
        const AZ::Vector3 secondHemisphereCenter(5.0f, 0.0f, 7.0f);
        const float radius = 4.0f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(2.0f, 2.0f, 5.0f);
        const AZ::Quaternion obbRotation(0.72f, -0.48f, -0.24f, 0.44f);
        const AZ::Vector3 obbHalfLengths(2.0f, 1.0f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCapJustOverlappingFace)
    {
        const AZ::Vector3 firstHemisphereCenter(0.696f, -2.8432f, 1.4576f);
        const AZ::Vector3 secondHemisphereCenter(-0.84f, -4.072f, 1.096f);
        const float radius = 1.01f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(3.0f, -1.0f, 2.0f);
        const AZ::Quaternion obbRotation(0.48f, -0.60f, 0.0f, 0.64f);
        const AZ::Vector3 obbHalfLengths(2.0f, 1.0f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCapJustSeparateFromFace)
    {
        const AZ::Vector3 firstHemisphereCenter(0.696f, -2.8432f, 1.4576f);
        const AZ::Vector3 secondHemisphereCenter(-0.84f, -4.072f, 1.096f);
        const float radius = 0.99f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(3.0f, -1.0f, 2.0f);
        const AZ::Quaternion obbRotation(0.48f, -0.60f, 0.0f, 0.64f);
        const AZ::Vector3 obbHalfLengths(2.0f, 1.0f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCylinderJustOverlappingFace)
    {
        const AZ::Vector3 firstHemisphereCenter(-1.7872f, 4.4496f, -1.424f);
        const AZ::Vector3 secondHemisphereCenter(1.6368f, 5.2176f, -3.344f);
        const float radius = 2.01f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(1.0f, 1.0f, -2.0f);
        const AZ::Quaternion obbRotation(0.12f, -0.96f, 0.08f, 0.24f);
        const AZ::Vector3 obbHalfLengths(1.5f, 2.0f, 0.5f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCylinderJustSeparateFromFace)
    {
        const AZ::Vector3 firstHemisphereCenter(-1.7872f, 4.4496f, -1.424f);
        const AZ::Vector3 secondHemisphereCenter(1.6368f, 5.2176f, -3.344f);
        const float radius = 1.99f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(1.0f, 1.0f, -2.0f);
        const AZ::Quaternion obbRotation(0.12f, -0.96f, 0.08f, 0.24f);
        const AZ::Vector3 obbHalfLengths(1.5f, 2.0f, 0.5f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCapJustOverlappingEdge)
    {
        const AZ::Vector3 firstHemisphereCenter(-7.64f, -0.534f, -0.088f);
        const AZ::Vector3 secondHemisphereCenter(-5.384f, 0.4796f, -0.4528f);
        const float radius = 1.26f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(-2.0f, 2.0f, -1.0f);
        const AZ::Quaternion obbRotation(0.70f, -0.02f, -0.14f, 0.70f);
        const AZ::Vector3 obbHalfLengths(2.0f, 1.5f, 1.5f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCapJustSeparateFromEdge)
    {
        const AZ::Vector3 firstHemisphereCenter(-7.64f, -0.534f, -0.088f);
        const AZ::Vector3 secondHemisphereCenter(-5.384f, 0.4796f, -0.4528f);
        const float radius = 1.24f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(-2.0f, 2.0f, -1.0f);
        const AZ::Quaternion obbRotation(0.70f, -0.02f, -0.14f, 0.70f);
        const AZ::Vector3 obbHalfLengths(2.0f, 1.5f, 1.5f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCylinderJustOverlappingEdge)
    {
        const AZ::Vector3 firstHemisphereCenter(6.3152f, 5.8864f, -1.088f);
        const AZ::Vector3 secondHemisphereCenter(6.3664f, -3.6752f, -4.016f);
        const float radius = 2.51f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(3.0f, 0.0f, 1.0f);
        const AZ::Quaternion obbRotation(0.16f, 0.92f, -0.16f, 0.32f);
        const AZ::Vector3 obbHalfLengths(1.0f, 1.5f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCylinderJustSeparateFromEdge)
    {
        const AZ::Vector3 firstHemisphereCenter(6.3152f, 5.8864f, -1.088f);
        const AZ::Vector3 secondHemisphereCenter(6.3664f, -3.6752f, -4.016f);
        const float radius = 2.49f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(3.0f, 0.0f, 1.0f);
        const AZ::Quaternion obbRotation(0.16f, 0.92f, -0.16f, 0.32f);
        const AZ::Vector3 obbHalfLengths(1.0f, 1.5f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCapJustOverlappingVertex)
    {
        const AZ::Vector3 firstHemisphereCenter(-8.92f, 8.56f, -2.4f);
        const AZ::Vector3 secondHemisphereCenter(-3.96f, 5.28f, -3.2f);
        const float radius = 3.01f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(1.0f, 2.0f, -4.0f);
        const AZ::Quaternion obbRotation(0.4f, 0.4f, 0.64f, 0.52f);
        const AZ::Vector3 obbHalfLengths(2.0f, 1.0f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCapJustSeparateFromVertex)
    {
        const AZ::Vector3 firstHemisphereCenter(-8.92f, 8.56f, -2.4f);
        const AZ::Vector3 secondHemisphereCenter(-3.96f, 5.28f, -3.2f);
        const float radius = 2.99f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(1.0f, 2.0f, -4.0f);
        const AZ::Quaternion obbRotation(0.4f, 0.4f, 0.64f, 0.52f);
        const AZ::Vector3 obbHalfLengths(2.0f, 1.0f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCylinderJustOverlappingVertex)
    {
        const AZ::Vector3 firstHemisphereCenter(1.5952f, 1.0064f, 9.064f);
        const AZ::Vector3 secondHemisphereCenter(-4.4176f, -6.8432f, 6.568f);
        const float radius = 3.01f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(2.0f, -4.0f, 3.0f);
        const AZ::Quaternion obbRotation(0.56f, -0.08f, -0.20f, 0.80f);
        const AZ::Vector3 obbHalfLengths(2.0f, 2.0f, 1.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleObbCylinderJustSeparateFromVertex)
    {
        const AZ::Vector3 firstHemisphereCenter(1.5952f, 1.0064f, 9.064f);
        const AZ::Vector3 secondHemisphereCenter(-4.4176f, -6.8432f, 6.568f);
        const float radius = 2.99f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, radius);
        const AZ::Vector3 obbPosition(2.0f, -4.0f, 3.0f);
        const AZ::Quaternion obbRotation(0.56f, -0.08f, -0.20f, 0.80f);
        const AZ::Vector3 obbHalfLengths(2.0f, 2.0f, 1.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, obb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb, capsule));
    }

    TEST(MATH_ShapeIntersection, SphereObbCompletelyContainedObb)
    {
        const AZ::Vector3 spherePosition(1.0f, 2.0f, 3.0f);
        const float sphereRadius = 5.0f;
        const AZ::Sphere sphere(spherePosition, sphereRadius);
        const AZ::Vector3 obbPosition(2.0f, 3.0f, 4.0f);
        const AZ::Quaternion obbRotation(0.24f, -0.72f, -0.12f, 0.64f);
        const AZ::Vector3 obbHalfLengths(1.0f, 2.0f, 1.5f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, sphere));
    }

    TEST(MATH_ShapeIntersection, SphereObbCompletelyContainedSphere)
    {
        const AZ::Vector3 sphereCenter(2.0f, 3.0f, 4.0f);
        const float sphereRadius = 2.0f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        const AZ::Vector3 obbPosition(3.0f, 4.0f, 5.0f);
        const AZ::Quaternion obbRotation(0.46f, 0.26f, 0.58f, 0.62f);
        const AZ::Vector3 obbHalfLengths(5.0f, 6.0f, 7.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, sphere));
    }

    TEST(MATH_ShapeIntersection, SphereObbJustIntersectingFace)
    {
        const AZ::Vector3 sphereCenter(-0.4f, -0.24f, 8.32f);
        const float sphereRadius = 2.01f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        const AZ::Vector3 obbPosition(2.0f, -1.0f, 4.0f);
        const AZ::Quaternion obbRotation(0.44f, 0.24f, 0.48f, 0.72f);
        const AZ::Vector3 obbHalfLengths(2.0f, 3.0f, 1.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, sphere));
    }

    TEST(MATH_ShapeIntersection, SphereObbJustSeparateFromFace)
    {
        const AZ::Vector3 sphereCenter(-0.4f, -0.24f, 8.32f);
        const float sphereRadius = 1.99f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        const AZ::Vector3 obbPosition(2.0f, -1.0f, 4.0f);
        const AZ::Quaternion obbRotation(0.44f, 0.24f, 0.48f, 0.72f);
        const AZ::Vector3 obbHalfLengths(2.0f, 3.0f, 1.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(sphere, obb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb, sphere));
    }

    TEST(MATH_ShapeIntersection, SphereObbJustIntersectingEdge)
    {
        const AZ::Vector3 sphereCenter(3.58f, 1.7264f, 1.2048f);
        const float sphereRadius = 5.01f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        const AZ::Vector3 obbPosition(-2.0f, -1.0f, -3.0f);
        const AZ::Quaternion obbRotation(0.16f, 0.20f, 0.40f, 0.88f);
        const AZ::Vector3 obbHalfLengths(1.5f, 1.0f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, sphere));
    }

    TEST(MATH_ShapeIntersection, SphereObbJustSeparateFromEdge)
    {
        const AZ::Vector3 sphereCenter(3.58f, 1.7264f, 1.2048f);
        const float sphereRadius = 4.99f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        const AZ::Vector3 obbPosition(-2.0f, -1.0f, -3.0f);
        const AZ::Quaternion obbRotation(0.16f, 0.20f, 0.40f, 0.88f);
        const AZ::Vector3 obbHalfLengths(1.5f, 1.0f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(sphere, obb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb, sphere));
    }

    TEST(MATH_ShapeIntersection, SphereObbJustIntersectingVertex)
    {
        const AZ::Vector3 sphereCenter(6.84f, 0.12f, -2.8f);
        const float sphereRadius = 3.01f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        const AZ::Vector3 obbPosition(1.0f, -1.0f, -2.0f);
        const AZ::Quaternion obbRotation(-0.22f, -0.26f, 0.38f, 0.86f);
        const AZ::Vector3 obbHalfLengths(2.0f, 1.0f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere, obb));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb, sphere));
    }

    TEST(MATH_ShapeIntersection, SphereObbJustSeparateFromVertex)
    {
        const AZ::Vector3 sphereCenter(6.84f, 0.12f, -2.8f);
        const float sphereRadius = 2.99f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        const AZ::Vector3 obbPosition(1.0f, -1.0f, -2.0f);
        const AZ::Quaternion obbRotation(-0.22f, -0.26f, 0.38f, 0.86f);
        const AZ::Vector3 obbHalfLengths(2.0f, 1.0f, 2.0f);
        const AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obbPosition, obbRotation, obbHalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(sphere, obb));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb, sphere));
    }

    TEST(MATH_ShapeIntersection, CapsuleSphereCompletelyContainedSphere)
    {
        const AZ::Vector3 firstHemisphereCenter(0.0f, 3.0f, 1.0f);
        const AZ::Vector3 secondHemisphereCenter(2.0f, 1.0f, 1.0f);
        const float capsuleRadius = 4.0f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, capsuleRadius);
        const AZ::Vector3 sphereCenter(1.0f, 2.0f, 1.0f);
        const float sphereRadius = 1.5f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, sphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleSphereCompletelyContainedCapsule)
    {
        const AZ::Vector3 firstHemisphereCenter(3.0f, -3.0f, 2.0f);
        const AZ::Vector3 secondHemisphereCenter(5.0f, -1.0f, 4.0f);
        const float capsuleRadius = 2.0f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, capsuleRadius);
        const AZ::Vector3 sphereCenter(4.0f, -2.0f, 3.0f);
        const float sphereRadius = 5.0f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, sphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleSphereJustIntersectingCap)
    {
        const AZ::Vector3 firstHemisphereCenter(-1.0f, 3.0f, -3.0f);
        const AZ::Vector3 secondHemisphereCenter(3.0f, -1.0f, 4.0f);
        const float capsuleRadius = 4.0f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, capsuleRadius);
        const AZ::Vector3 sphereCenter(7.0f, -5.0f, 11.0f);
        const float sphereRadius = 5.01f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, sphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleSphereJustSeparateFromCap)
    {
        const AZ::Vector3 firstHemisphereCenter(-1.0f, 3.0f, -3.0f);
        const AZ::Vector3 secondHemisphereCenter(3.0f, -1.0f, 4.0f);
        const float capsuleRadius = 4.0f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, capsuleRadius);
        const AZ::Vector3 sphereCenter(7.0f, -5.0f, 11.0f);
        const float sphereRadius = 4.99f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, sphere));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(sphere, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleSphereJustIntersectingCylinder)
    {
        const AZ::Vector3 firstHemisphereCenter(2.0f, 5.0f, -3.0f);
        const AZ::Vector3 secondHemisphereCenter(8.0f, -3.0f, -1.0f);
        const float capsuleRadius = 5.0f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, capsuleRadius);
        const AZ::Vector3 sphereCenter(5.0f, 4.0f, 4.0f);
        const float sphereRadius = 2.01f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, sphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere, capsule));
    }

    TEST(MATH_ShapeIntersection, CapsuleSphereJustSeparateFromCylinder)
    {
        const AZ::Vector3 firstHemisphereCenter(2.0f, 5.0f, -3.0f);
        const AZ::Vector3 secondHemisphereCenter(8.0f, -3.0f, -1.0f);
        const float capsuleRadius = 5.0f;
        const AZ::Capsule capsule(firstHemisphereCenter, secondHemisphereCenter, capsuleRadius);
        const AZ::Vector3 sphereCenter(5.0f, 4.0f, 4.0f);
        const float sphereRadius = 1.99f;
        const AZ::Sphere sphere(sphereCenter, sphereRadius);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, sphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(sphere, capsule));
    }

    TEST(MATH_ShapeIntersection, ObbObbCompletelyContained)
    {
        const AZ::Vector3 obb1Position(2.0f, -1.0f, 3.0f);
        const AZ::Quaternion obb1Rotation(0.34f, -0.46f, -0.58f, 0.58f);
        const AZ::Vector3 obb1HalfLengths(4.0f, 5.0f, 6.0f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(1.0f, 0.0f, 3.0f);
        const AZ::Quaternion obb2Rotation(-0.16f, 0.64f, -0.68f, 0.32f);
        const AZ::Vector3 obb2HalfLengths(2.0f, 1.0f, 1.5f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustIntersectingFaceFace)
    {
        const AZ::Vector3 obb1Position(2.0f, 1.0f, -4.0f);
        const AZ::Quaternion obb1Rotation(0.24f, 0.08f, -0.48f, 0.84f);
        const AZ::Vector3 obb1HalfLengths(1.0f, 3.0f, 2.01f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(1.52f, -1.4f, 0.36f);
        const AZ::Quaternion obb2Rotation(0.82f, 0.1f, 0.26f, 0.5f);
        const AZ::Vector3 obb2HalfLengths(2.0f, 3.0f, 1.0f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustSeparateFaceFace)
    {
        const AZ::Vector3 obb1Position(2.0f, 1.0f, -4.0f);
        const AZ::Quaternion obb1Rotation(0.24f, 0.08f, -0.48f, 0.84f);
        const AZ::Vector3 obb1HalfLengths(1.0f, 3.0f, 1.99f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(1.52f, -1.4f, 0.36f);
        const AZ::Quaternion obb2Rotation(0.82f, 0.1f, 0.26f, 0.5f);
        const AZ::Vector3 obb2HalfLengths(2.0f, 3.0f, 1.0f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustIntersectingFaceEdge)
    {
        const AZ::Vector3 obb1Position(-1.0f, 1.0f, 3.0f);
        const AZ::Quaternion obb1Rotation(0.1f, 0.1f, 0.7f, 0.7f);
        const AZ::Vector3 obb1HalfLengths(2.0f, 1.5f, 1.0f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(-4.84f, 1.0f, 4.12f);
        const AZ::Quaternion obb2Rotation(0.14f, 0.02f, 0.98f, 0.14f);
        const AZ::Vector3 obb2HalfLengths(2.41f, 0.7f, 3.0f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustSeparateFaceEdge)
    {
        const AZ::Vector3 obb1Position(-1.0f, 1.0f, 3.0f);
        const AZ::Quaternion obb1Rotation(0.1f, 0.1f, 0.7f, 0.7f);
        const AZ::Vector3 obb1HalfLengths(2.0f, 1.5f, 1.0f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(-4.84f, 1.0f, 4.12f);
        const AZ::Quaternion obb2Rotation(0.14f, 0.02f, 0.98f, 0.14f);
        const AZ::Vector3 obb2HalfLengths(2.39f, 0.7f, 3.0f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustIntersectingFaceVertex)
    {
        const AZ::Vector3 obb1Position(3.0f, -1.0f, -2.0f);
        const AZ::Quaternion obb1Rotation(0.08f, -0.44f, 0.16f, 0.88f);
        const AZ::Vector3 obb1HalfLengths(1.0f, 2.0f, 1.0f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(1.436f, 1.852f, -3.8f);
        const AZ::Quaternion obb2Rotation(0.52f, -0.56f, -0.56f, 0.32f);
        const AZ::Vector3 obb2HalfLengths(0.31f, 1.7f, 1.4f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustSeparateFaceVertex)
    {
        const AZ::Vector3 obb1Position(3.0f, -1.0f, -2.0f);
        const AZ::Quaternion obb1Rotation(0.08f, -0.44f, 0.16f, 0.88f);
        const AZ::Vector3 obb1HalfLengths(1.0f, 2.0f, 1.0f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(1.436f, 1.852f, -3.8f);
        const AZ::Quaternion obb2Rotation(0.52f, -0.56f, -0.56f, 0.32f);
        const AZ::Vector3 obb2HalfLengths(0.29f, 1.7f, 1.4f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustIntersectingEdgeEdge)
    {
        const AZ::Vector3 obb1Position(1.0f, 4.0f, 3.0f);
        const AZ::Quaternion obb1Rotation(0.7f, 0.1f, -0.7f, 0.1f);
        const AZ::Vector3 obb1HalfLengths(2.0f, 1.0f, 1.0f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(-2.072f, 3.104f, 0.6f);
        const AZ::Quaternion obb2Rotation(0.14f, 0.02f, 0.98f, -0.14f);
        const AZ::Vector3 obb2HalfLengths(2.0f, 0.5f, 1.01f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustSeparateEdgeEdge)
    {
        const AZ::Vector3 obb1Position(1.0f, 4.0f, 3.0f);
        const AZ::Quaternion obb1Rotation(0.7f, 0.1f, -0.7f, 0.1f);
        const AZ::Vector3 obb1HalfLengths(2.0f, 1.0f, 1.0f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(-2.072f, 3.104f, 0.6f);
        const AZ::Quaternion obb2Rotation(0.14f, 0.02f, 0.98f, -0.14f);
        const AZ::Vector3 obb2HalfLengths(2.0f, 0.5f, 0.99f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustIntersectingEdgeVertex)
    {
        const AZ::Vector3 obb1Position(4.0f, 2.0f, 5.0f);
        const AZ::Quaternion obb1Rotation(-0.58f, 0.22f, 0.26f, 0.74f);
        const AZ::Vector3 obb1HalfLengths(2.0f, 1.0f, 2.0f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(0.696f, 5.9216f, 5.3968f);
        const AZ::Quaternion obb2Rotation(0.68f, 0.64f, -0.32f, 0.16f);
        const AZ::Vector3 obb2HalfLengths(1.01f, 3.0f, 1.01f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustSeparateEdgeVertex)
    {
        const AZ::Vector3 obb1Position(4.0f, 2.0f, 5.0f);
        const AZ::Quaternion obb1Rotation(-0.58f, 0.22f, 0.26f, 0.74f);
        const AZ::Vector3 obb1HalfLengths(2.0f, 1.0f, 2.0f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(0.696f, 5.9216f, 5.3968f);
        const AZ::Quaternion obb2Rotation(0.68f, 0.64f, -0.32f, 0.16f);
        const AZ::Vector3 obb2HalfLengths(0.99f, 3.0f, 0.99f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustIntersectingVertexVertex)
    {
        const AZ::Vector3 obb1Position(-1.0f, 5.0f, 3.0f);
        const AZ::Quaternion obb1Rotation(0.44f, 0.40f, 0.08f, 0.80f);
        const AZ::Vector3 obb1HalfLengths(1.0f, 1.5f, 3.01f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(3.808f, 0.9208f, 1.4688f);
        const AZ::Quaternion obb2Rotation(0.50f, 0.10f, 0.26f, 0.82f);
        const AZ::Vector3 obb2HalfLengths(1.0f, 2.0f, 2.0f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, ObbObbJustSeparateVertexVertex)
    {
        const AZ::Vector3 obb1Position(-1.0f, 5.0f, 3.0f);
        const AZ::Quaternion obb1Rotation(0.44f, 0.40f, 0.08f, 0.80f);
        const AZ::Vector3 obb1HalfLengths(1.0f, 1.5f, 2.99f);
        const AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb1Position, obb1Rotation, obb1HalfLengths);
        const AZ::Vector3 obb2Position(3.808f, 0.9208f, 1.4688f);
        const AZ::Quaternion obb2Rotation(0.50f, 0.10f, 0.26f, 0.82f);
        const AZ::Vector3 obb2HalfLengths(1.0f, 2.0f, 2.0f);
        const AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(obb2Position, obb2Rotation, obb2HalfLengths);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb1, obb2));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(obb2, obb1));
    }

    TEST(MATH_ShapeIntersection, HemisphereVsSphere)
    {
        const AZ::Sphere unitSphere = AZ::Sphere::CreateUnitSphere();
        const AZ::Sphere sphere1 = AZ::Sphere(AZ::Vector3(2.0f, 2.0f, 2.0f), 2.0f);

        // hemisphere overlaps unit sphere, doesn't touch other sphere
        const AZ::Hemisphere hemisphere0(AZ::Vector3(0.0f, 0.0f, 0.0f), 1.0f, AZ::Vector3(0.0f, 0.0f, 1.0f));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(hemisphere0, unitSphere));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(hemisphere0, sphere1));

        // hemisphere doesn't overlap unit sphere, but overlaps other sphere
        const AZ::Hemisphere hemisphere1(AZ::Vector3(1.0f, 1.0f, 1.0f), 1.0f, AZ::Vector3(0.0f, 0.0f, 1.0f));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(hemisphere1, unitSphere));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(hemisphere1, sphere1));

        // hemisphere faces away from unit sphere but is still within range
        const AZ::Hemisphere hemisphere2(AZ::Vector3(0.0f, 0.0f, -0.5f), 2.0f, AZ::Vector3(0.0f, 0.0f, -1.0f));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(hemisphere2, unitSphere));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(hemisphere2, sphere1));

        // hemisphere faces away from unit sphere and off to the side but is still barely within range
        const AZ::Hemisphere hemisphere3(AZ::Vector3(0.0f, 2.0f, -0.9f), 2.0f, AZ::Vector3(0.0f, 0.0f, -1.0f));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(hemisphere3, unitSphere));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(hemisphere3, sphere1));

        // hemisphere faces away from unit sphere but is out of range.
        const AZ::Hemisphere hemisphere4(AZ::Vector3(0.0f, 0.0f, -1.0f), 2.0f, AZ::Vector3(0.0f, 0.0f, -1.0f));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(hemisphere4, unitSphere));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(hemisphere4, sphere1));

        // hemisphere faces towards unit sphere and is in range
        const AZ::Hemisphere hemisphere5(AZ::Vector3(0.0f, 0.0f, -1.5f), 1.0f, AZ::Vector3(0.0f, 0.0f, 1.0f));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(hemisphere5, unitSphere));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(hemisphere5, sphere1));
    }

    TEST(MATH_ShapeIntersection, HemisphereVsAabb)
    {
        const AZ::Aabb unitAabb = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), AZ::Vector3(1.0f));

        // hemisphere on top of aabb
        const AZ::Hemisphere hemisphere0(AZ::Vector3(0.0f, 0.0f, 0.0f), 1.0f, AZ::Vector3(0.0f, 0.0f, 1.0f));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(hemisphere0, unitAabb));

        // hemisphere just above aabb pointing away
        const AZ::Hemisphere hemisphere1(AZ::Vector3(0.0f, 0.0f, 1.1f), 1.0f, AZ::Vector3(0.0f, 0.0f, 1.0f));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(hemisphere1, unitAabb));

        // hemisphere just above aabb pointing towards
        const AZ::Hemisphere hemisphere2(AZ::Vector3(0.0f, 0.0f, 1.1f), 1.0f, AZ::Vector3(0.0f, 0.0f, -1.0f));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(hemisphere2, unitAabb));

        // hemisphere farther above aabb pointing towards
        const AZ::Hemisphere hemisphere3(AZ::Vector3(0.0f, 0.0f, 2.1f), 1.0f, AZ::Vector3(0.0f, 0.0f, -1.0f));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(hemisphere3, unitAabb));

        // hemisphere just above aabb pointing away, but at an angle so the plane of the hemisphere intersects
        const AZ::Hemisphere hemisphere4(AZ::Vector3(0.0f, 0.0f, 1.1f), 1.0f, AZ::Vector3(0.0f, 1.0f, 1.0f).GetNormalized());
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(hemisphere4, unitAabb));

        // false positive case - hemisphere points away from aabb at an angle where the plane still intersects the aabb, but not near enough
        // to the hemisphere to actually intersect. This typically happens when the hemisphere is much smaller than the aabb.
        const AZ::Hemisphere hemisphere5(AZ::Vector3(0.0f, 0.0f, 1.2f), 0.3f, AZ::Vector3(0.0f, 1.0f, 1.0f).GetNormalized());
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(hemisphere5, unitAabb));

    }

    TEST(MATH_ShapeIntersection, HemisphereContainsAabb)
    {
        const AZ::Aabb unitAabb = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), AZ::Vector3(1.0f));

        // hemisphere intersecting aabb, but not big enough
        const AZ::Hemisphere hemisphere0(AZ::Vector3(0.0f, 0.0f, 0.0f), 2.0f, AZ::Vector3(0.0f, 0.0f, 1.0f));
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(hemisphere0, unitAabb));

        // hemisphere contains aabb
        const AZ::Hemisphere hemisphere1(AZ::Vector3(0.0f, 0.0f, -1.0f), 3.0f, AZ::Vector3(0.0f, 0.0f, 1.0f));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(hemisphere1, unitAabb));

        // aabb contains hemisphere
        const AZ::Hemisphere hemisphere2(AZ::Vector3(0.0f, 0.0f, 0.0f), 0.5f, AZ::Vector3(0.0f, 0.0f, 1.0f));
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(hemisphere2, unitAabb));

        // just one point of aabb is outside
        const AZ::Hemisphere hemisphere3(AZ::Vector3(1.0f, 1.0f, -1.0f), 3.0f, AZ::Vector3(0.0f, 0.0f, 1.0f));
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(hemisphere3, unitAabb));
    }

    TEST(MATH_ShapeIntersection, CapsuleOverlapsAabb)
    {
        const AZ::Aabb unitAabb = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne());
        AZ::Capsule capsule;

        // Clear overlap (both shapes pass through origin)
        capsule = AZ::Capsule(AZ::Vector3(-1.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), 1.0f);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, unitAabb));

        // Clear no overlap
        capsule = AZ::Capsule(AZ::Vector3(-10.0f, 0.0f, 0.0f), AZ::Vector3(-9.0, 0.0f, 0.0f), 1.0f);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, unitAabb));

        // Large capsule vs small aabb tests

        // aabb completely contained in capsule
        capsule = AZ::Capsule(AZ::Vector3(-10.0f, 0.0f, 0.0f), AZ::Vector3(10.0, 0.0f, 0.0f), 5.0f);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, unitAabb));

        // aabb near cap, slightly overlapping
        capsule = AZ::Capsule(AZ::Vector3(-10.0f, 0.0f, 0.0f), AZ::Vector3(-5.9f, 0.0f, 0.0f), 5.0f);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, unitAabb));
        
        // aabb near cap, but not overlapping
        capsule = AZ::Capsule(AZ::Vector3(-10.0f, 0.0f, 0.0f), AZ::Vector3(-6.1f, 0.0f, 0.0f), 5.0f);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, unitAabb));

        // aabb near side, slightly overlapping
        capsule = AZ::Capsule(AZ::Vector3(-10.0f, 5.9f, 0.0f), AZ::Vector3(10.0f, 5.9f, 0.0f), 5.0f);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, unitAabb));

        // aabb near side, not overlapping
        capsule = AZ::Capsule(AZ::Vector3(-10.0f, 6.1f, 0.0f), AZ::Vector3(10.0f, 6.1f, 0.0f), 5.0f);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, unitAabb));

        // aabb diagonal, overlapping
        capsule = AZ::Capsule(AZ::Vector3(-8.0f, -5.0f, 0.0f), AZ::Vector3(5.0f, 8.0f, 0.0f), 0.8f);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, unitAabb));

        // aabb diagonal, not overlapping
        capsule = AZ::Capsule(AZ::Vector3(-8.0f, -5.0f, 0.0f), AZ::Vector3(5.0f, 8.0f, 0.0f), 0.7f);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, unitAabb));
    }

    TEST(MATH_ShapeIntersection, CapsuleOverlapsCapsule)
    {
        AZ::Capsule capsule;
        AZ::Capsule longCapsule = AZ::Capsule(AZ::Vector3(-10.0f, 0.0f, 0.0f), AZ::Vector3(10.0f, 0.0f, 0.0f), 1.0f);
        AZ::Capsule shortCapsule = AZ::Capsule(AZ::Vector3(-0.5f, 0.0f, 0.0f), AZ::Vector3(0.5f, 0.0f, 0.0f), 1.0f);

        // capsule overlaps self
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(longCapsule, longCapsule));

        // narrow perpendicular capsule miss
        capsule = AZ::Capsule(AZ::Vector3(0.0f, -10.0f, 2.0f), AZ::Vector3(0.0f, 10.0f, 2.0f), 0.5f);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, longCapsule));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, shortCapsule));

        // narrow perpendicular capsule hit
        capsule = AZ::Capsule(AZ::Vector3(0.0f, -10.0f, 1.4f), AZ::Vector3(0.0f, 10.0f, 1.4f), 0.5f);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, longCapsule));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, shortCapsule));

        // wide perpendicular capsule miss
        capsule = AZ::Capsule(AZ::Vector3(0.0f, -10.0f, 6.1f), AZ::Vector3(0.0f, 10.0f, 6.1), 5.0f);
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, longCapsule));
        EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule, shortCapsule));

        // wide perpendicular capsule hit
        capsule = AZ::Capsule(AZ::Vector3(0.0f, -10.0f, 5.9f), AZ::Vector3(0.0f, 10.0f, 5.9f), 5.0f);
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, longCapsule));
        EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule, shortCapsule));

        // diagonal capsules
        {
            AZ::Capsule capsule1 = AZ::Capsule(AZ::Vector3(-10.0f, -10.0f, 0.0f), AZ::Vector3(10.0f, 10.0f, 0.0f), 1.0f);
            AZ::Capsule capsule2 = AZ::Capsule(AZ::Vector3(10.0f, -10.0f, 1.0f), AZ::Vector3(-10.0f, 10.0f, 1.0f), 1.0f);
            EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule1, capsule2));

            AZ::Capsule capsule3 = AZ::Capsule(AZ::Vector3(-10.0f, -10.0f, 0.0f), AZ::Vector3(10.0f, 10.0f, 0.0f), 2.0f);
            AZ::Capsule capsule4 = AZ::Capsule(AZ::Vector3(10.0f, -10.0f, 4.1f), AZ::Vector3(-10.0f, 10.0f, 4.1f), 2.0f);
            EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule3, capsule4));
        }

        // parallel capsules, end point intersection
        {
            AZ::Capsule capsule1 = AZ::Capsule(AZ::Vector3(-10.0f, 0.0, 0.0f), AZ::Vector3(-1.0f, 0.0f, 0.0f), 1.1f);
            AZ::Capsule capsule2 = AZ::Capsule(AZ::Vector3(1.0f, 0.0, 0.0f), AZ::Vector3(10.0f, 0.0f, 0.0f), 1.1f);
            EXPECT_TRUE(AZ::ShapeIntersection::Overlaps(capsule1, capsule2));

            AZ::Capsule capsule3 = AZ::Capsule(AZ::Vector3(-10.0f, 0.0, 0.0f), AZ::Vector3(-1.0f, 0.0f, 0.0f), 0.9f);
            AZ::Capsule capsule4 = AZ::Capsule(AZ::Vector3(1.0f, 0.0, 0.0f), AZ::Vector3(10.0f, 0.0f, 0.0f), 0.9f);
            EXPECT_FALSE(AZ::ShapeIntersection::Overlaps(capsule3, capsule4));
        }
    }

    TEST(MATH_ShapeIntersection, CapsuleContainsSphere)
    {
        AZ::Capsule capsule = AZ::Capsule(AZ::Vector3(-10.0f, 0.0f, 0.0f), AZ::Vector3(10.0f, 0.0f, 0.0f), 1.0f);

        AZ::Sphere sphere = AZ::Sphere(AZ::Vector3(0.0f, 0.0f, 0.0f), 1.0f);
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(capsule, sphere));

        sphere = AZ::Sphere(AZ::Vector3(-10.0f, 0.0f, 0.0f), 0.5f);
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(capsule, sphere));

        sphere = AZ::Sphere(AZ::Vector3(10.0f, 0.0f, 0.0f), 1.5f);
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(capsule, sphere));

        sphere = AZ::Sphere(AZ::Vector3(10.0f, 10.0f, 0.0f), 0.5f);
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(capsule, sphere));
    }

    TEST(MATH_ShapeIntersection, CapsuleContainsAabb)
    {
        AZ::Capsule capsule = AZ::Capsule(AZ::Vector3(-1.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), 2.0f);
        AZ::Aabb unitAabb = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne());

        EXPECT_TRUE(AZ::ShapeIntersection::Contains(capsule, unitAabb));

        capsule = AZ::Capsule(AZ::Vector3(-1.0f, 0.0f, 0.0f), AZ::Vector3(1.0f, 0.0f, 0.0f), 1.0f);
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(capsule, unitAabb));

        // long thin aabb inside long capsule
        capsule = AZ::Capsule(AZ::Vector3(-10.0f, 0.0f, 0.0f), AZ::Vector3(10.0f, 0.0f, 0.0f), 2.0f);
        AZ::Aabb longAabb = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), AZ::Vector3(10.0f, 1.0f, 1.0f));
        EXPECT_TRUE(AZ::ShapeIntersection::Contains(capsule, longAabb));

        longAabb = AZ::Aabb::CreateCenterHalfExtents(AZ::Vector3::CreateZero(), AZ::Vector3(11.0f, 1.0f, 10.0f));
        EXPECT_FALSE(AZ::ShapeIntersection::Contains(capsule, longAabb));
    }

} // namespace UnitTest
