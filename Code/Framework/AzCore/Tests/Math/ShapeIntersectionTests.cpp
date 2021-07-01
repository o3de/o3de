/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Frustum.h>
#include <AzCore/Math/Sphere.h>
#include <AzCore/Math/ShapeIntersection.h>

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

        AZ::Vector3 point(0.f, 0.f, 0.f);
        AZ::Vector3 point1(10.f, 10.f, 10.f);

        {
            AZ::Vector3 intersectionPoint;
            EXPECT_TRUE(AZ::ShapeIntersection::IntersectThreePlanes(near_value, left, bottom, intersectionPoint));
            EXPECT_TRUE(intersectionPoint.IsClose(AZ::Vector3(-5.f, -5.f, -5.f)));
            EXPECT_FALSE(AZ::ShapeIntersection::IntersectThreePlanes(near_value, far_value, bottom, intersectionPoint));
        }

        {
            AZ::Vector3 intersectionPoint;
            EXPECT_TRUE(AZ::ShapeIntersection::IntersectThreePlanes(near_value, left, bottom, intersectionPoint));
            EXPECT_TRUE(intersectionPoint.IsClose(AZ::Vector3(-5.f, -5.f, -5.f)));
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

            EXPECT_TRUE(AZ::ShapeIntersection::Classify(near_value, s) ==  AZ::IntersectResult::Interior);
            EXPECT_TRUE(AZ::ShapeIntersection::Classify(near_value, s1) == AZ::IntersectResult::Overlaps);
            EXPECT_TRUE(AZ::ShapeIntersection::Classify(near_value, s2) == AZ::IntersectResult::Exterior);

            EXPECT_TRUE(AZ::ShapeIntersection::Classify(frustum, s) ==  AZ::IntersectResult::Interior);
            EXPECT_TRUE(AZ::ShapeIntersection::Classify(frustum, s1) == AZ::IntersectResult::Overlaps);
            EXPECT_TRUE(AZ::ShapeIntersection::Classify(frustum, s2) == AZ::IntersectResult::Exterior);
        }

        AZ::Vector3 axisX = AZ::Vector3::CreateAxisX();
        AZ::Vector3 axisY = AZ::Vector3::CreateAxisY();
        AZ::Vector3 axisZ = AZ::Vector3::CreateAxisZ();
        {
            AZ::Obb obb = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
                AZ::Vector3(0.0f, -3.9f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne());
            AZ::Obb obb1 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
                AZ::Vector3(0.0f, -5.1f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne());
            AZ::Obb obb2 = AZ::Obb::CreateFromPositionRotationAndHalfLengths(
                AZ::Vector3(0.0f, -6.1f, 0.0f), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateOne());
            EXPECT_TRUE(AZ::ShapeIntersection::Classify(near_value, obb) ==  AZ::IntersectResult::Interior);
            EXPECT_TRUE(AZ::ShapeIntersection::Classify(near_value, obb1) == AZ::IntersectResult::Overlaps);
            EXPECT_TRUE(AZ::ShapeIntersection::Classify(near_value, obb2) == AZ::IntersectResult::Exterior);
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
}
