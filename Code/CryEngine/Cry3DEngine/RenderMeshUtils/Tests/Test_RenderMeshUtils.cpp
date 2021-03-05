/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "Cry3DEngine_precompiled.h"
#include <AzTest/AzTest.h>

#include "RenderMeshUtils.h"

namespace TestRenderMeshUtils
{
    void TestBarycentricCoordinates(Vec3 t0, Vec3 t1, Vec3 t2, const std::vector<Vec3>& hitPoints, const std::vector<Vec3>& expectedBarycentricCoordinates, bool expectedIsHit)
    {
        assert(hitPoints.size() == expectedBarycentricCoordinates.size());

        float u = 0.f;
        float v = 0.f;
        float w = 0.f;
        for (uint i = 0; i < hitPoints.size(); ++i)
        {
            EXPECT_TRUE(GetBarycentricCoordinates(t0, t1, t2, hitPoints[i], u, v, w) == expectedIsHit);

            EXPECT_FLOAT_EQ(expectedBarycentricCoordinates[i].x, u);
            EXPECT_FLOAT_EQ(expectedBarycentricCoordinates[i].y, v);
            EXPECT_FLOAT_EQ(expectedBarycentricCoordinates[i].z, w);
        }
    }

    GTEST_TEST(GetBarycentricCoordinatesTest, Call_CoordinatesOnVertexOfUnitTriangle_ReturnsTrue)
    {
        // Test triangle
        Vec3 t0(1.f, 0.f, 0.f);
        Vec3 t1(0.f, 1.f, 0.f);
        Vec3 t2(0.f, 0.f, 1.f);

        // Test data
        std::vector<Vec3> vertexHitPoints = {
            { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }
        };
        std::vector<Vec3> vertexExpectedBarycentricCoordinates = {
            { 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }
        };

        TestBarycentricCoordinates(t0, t1, t2, vertexHitPoints, vertexExpectedBarycentricCoordinates, true);
    }

    GTEST_TEST(GetBarycentricCoordinatesTest, Call_CoordinatesOnEdgeOfUnitTriangle_ReturnsTrue)
    {
        // Test triangle
        Vec3 t0(1.f, 0.f, 0.f);
        Vec3 t1(0.f, 1.f, 0.f);
        Vec3 t2(0.f, 0.f, 1.f);

        // Test data
        std::vector<Vec3> edgeHitPoints = {
            { 0.5f, 0.5f, 0.0f }, { 0.5f, 0.0f, 0.5f }, { 0.0f, 0.5f, 0.5f }
        };
        std::vector<Vec3> edgeExpectedBarycentricCoordinates = {
            { 0.5f, 0.5f, 0.0f }, { 0.5f, 0.0f, 0.5f }, { 0.0f, 0.5f, 0.5f }
        };

        TestBarycentricCoordinates(t0, t1, t2, edgeHitPoints, edgeExpectedBarycentricCoordinates, true);
    }

    GTEST_TEST(GetBarycentricCoordinatesTest, Call_CoordinatesOnCenterOfUnitTriangle_ReturnsTrue)
    {
        // Test triangle
        Vec3 t0(1.f, 0.f, 0.f);
        Vec3 t1(0.f, 1.f, 0.f);
        Vec3 t2(0.f, 0.f, 1.f);

        // Test data
        std::vector<Vec3> centerHitPoints = {
            { 0.5f, 0.5f, 0.5f }, { 1.f, 1.f, 1.f }, { -1.f, -1.f, -1.f }
        };
        std::vector<Vec3> centerExpectedBarycentricCoordinates = {
            { 1.f / 3.f, 1.f / 3.f, 1.f / 3.f }, { 1.f / 3.f, 1.f / 3.f, 1.f / 3.f }, { 1.f / 3.f, 1.f / 3.f, 1.f / 3.f }
        };

        TestBarycentricCoordinates(t0, t1, t2, centerHitPoints, centerExpectedBarycentricCoordinates, true);
    }

    GTEST_TEST(GetBarycentricCoordinatesTest, Call_CoordinatesOffCenterOfUnitTriangle_ReturnsTrue)
    {
        // Test triangle
        Vec3 t0(1.f, 0.f, 0.f);
        Vec3 t1(0.f, 1.f, 0.f);
        Vec3 t2(0.f, 0.f, 1.f);

        // Test data
        std::vector<Vec3> offCenterHitPoints = {
            { 1.f, 1.f, .75f }, { -1.f, -1.f, -.75f }
        };
        std::vector<Vec3> offCenterExpectedBarycentricCoordinates = {
            { 5.f / 12.f, 5.f / 12.f, 1.f / 6.f }, { 0.25f, 0.25f, 0.5f }
        };

        TestBarycentricCoordinates(t0, t1, t2, offCenterHitPoints, offCenterExpectedBarycentricCoordinates, true);
    }

    GTEST_TEST(GetBarycentricCoordinatesTest, Call_CoordinatesOutsideOfUnitTriangle_ReturnsFalse)
    {
        // Test triangle
        Vec3 t0(1.f, 0.f, 0.f);
        Vec3 t1(0.f, 1.f, 0.f);
        Vec3 t2(0.f, 0.f, 1.f);

        // Test data
        std::vector<Vec3> nonHitPoints = {
            { 1.f, 1.f, 0.f }, { 1.f, 0.f, 1.f }, { 0.f, 1.f, 1.f }
        };
        std::vector<Vec3> nonHitExpectedBarycentricCoordinates = {
            { 2.f / 3.f, 2.f / 3.f, -1.f / 3.f }, { 2.f / 3.f, -1.f / 3.f, 2.f / 3.f }, { -1.f / 3.f, 2.f / 3.f, 2.f / 3.f }
        };

        TestBarycentricCoordinates(t0, t1, t2, nonHitPoints, nonHitExpectedBarycentricCoordinates, false);
    }
}
