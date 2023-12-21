/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <MCore/Source/OBB.h>
#include <AzCore/Math/Transform.h>
#include <Tests/SystemComponentFixture.h>

namespace EMotionFX
{
    using OBBFixture = SystemComponentFixture;

    TEST_F(OBBFixture, InitFromPoints)
    {
        const float angle = 15.0f;
        const AZ::Transform transform = AZ::Transform::CreateRotationZ(AZ::DegToRad(angle));

        // generate a vector of points at the vertices of a rotated cuboid
        const float x[2] = { 2.4f, 3.7f }; // half-extent = 0.65
        const float y[2] = { -0.3f, 1.8f }; // half-extent = 1.05
        const float z[2] = { 0.2f, 0.6f }; // half-extent = 0.2

        const int numPoints = 8;
        AZ::Vector3 points[8];
        for (int i = 0; i < 8; i++)
        {
            const int xi = i / 4;
            const int yi = (i / 2) % 2;
            const int zi = i % 2;
            const AZ::Vector3 point(x[xi], y[yi], z[zi]);
            points[i] = transform.TransformPoint(point);
        }

        // fit an OBB to the points
        MCore::OBB obb;
        obb.InitFromPoints(points, numPoints);

        // the dimensions of the OBB should match those of the cuboid
        EXPECT_TRUE(AZ::IsClose(obb.GetExtents().GetMaxElement(), 1.05f, 0.01f));
        EXPECT_TRUE(AZ::IsClose(obb.GetExtents().GetMinElement(), 0.2f, 0.01f));
    }

    TEST_F(OBBFixture, Contains)
    {
        const AZ::Vector3 center(0.3f, 0.4f, 0.5f);
        const AZ::Vector3 extents(0.2f, 0.1f, 0.15f);
        const AZ::Transform transform = AZ::Transform::CreateFromQuaternion(AZ::Quaternion(0.22f, 0.70f, 0.50f, 0.46f));
        MCore::OBB obb(center, extents, transform);

        EXPECT_TRUE(obb.Contains(center));

        // test pairs of points close together on either side of boundaries
        EXPECT_TRUE(obb.Contains(AZ::Vector3(0.24f, 0.6f, 0.46f)));
        EXPECT_FALSE(obb.Contains(AZ::Vector3(0.23f, 0.6f, 0.46f)));
        EXPECT_TRUE(obb.Contains(AZ::Vector3(0.4f, 0.35f, 0.43f)));
        EXPECT_FALSE(obb.Contains(AZ::Vector3(0.4f, 0.34f, 0.43f)));
        EXPECT_TRUE(obb.Contains(AZ::Vector3(0.15f, 0.35f, 0.4f)));
        EXPECT_FALSE(obb.Contains(AZ::Vector3(0.15f, 0.35f, 0.5f)));
    }

    TEST_F(OBBFixture, CheckIfIsInside)
    {
        const AZ::Vector3 center1(0.2f, 0.4f, -0.1f);
        const AZ::Vector3 extents1(0.1f, 0.3f, 0.2f);
        const AZ::Transform transform1 = AZ::Transform::CreateFromQuaternion(AZ::Quaternion(0.72f, 0.48f, 0.24f, 0.44f));
        MCore::OBB obb1(center1, extents1, transform1);

        const AZ::Vector3 center2(0.25f, 0.35f, -0.15f);
        const AZ::Vector3 extents2(0.04f, 0.05f, 0.06f);
        const AZ::Transform transform2 = AZ::Transform::CreateFromQuaternion(AZ::Quaternion(0.58f, 0.46f, 0.26f, 0.62f));
        MCore::OBB obb2(center2, extents2, transform2);

        // the second obb should be inside the first obb
        EXPECT_TRUE(obb2.CheckIfIsInside(obb1));
        EXPECT_FALSE(obb1.CheckIfIsInside(obb2));

        // moving the first obb a little bit should mean the second obb is no longer inside it
        obb1.SetCenter(AZ::Vector3(0.2f, 0.45f, -0.1f));
        EXPECT_FALSE(obb2.CheckIfIsInside(obb1));
        obb1.SetCenter(AZ::Vector3(0.15f, 0.4f, -0.1f));
        EXPECT_FALSE(obb2.CheckIfIsInside(obb1));
        obb1.SetCenter(AZ::Vector3(0.2f, 0.4f, 0.0f));
        EXPECT_FALSE(obb2.CheckIfIsInside(obb1));

        // shrinking the second obb slightly should make it fit inside the first again
        obb2.SetExtents(AZ::Vector3(0.03f, 0.04f, 0.05f));
        EXPECT_TRUE(obb2.CheckIfIsInside(obb1));
    }
} // namespace EMotionFX
