/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "WhiteBoxTestFixtures.h"

#include <AzCore/UnitTest/TestTypes.h>
#include <AzTest/AzTest.h>
#include <vector>
#include <limits>

namespace UnitTest
{
    using namespace WhiteBox;

    FaceTestData NonDegenerateFaceList = {
        {// Tri 0: non-degenerate
         AZ::Vector3{0.0f, 1.0f, 0.0f}, AZ::Vector3{1.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f},

         // Tri 1: non-degenerate
         AZ::Vector3{1.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 1.0f, 0.0f},

         // Tri 2: non-degenerate
         AZ::Vector3{0.0f, 0.0f, 1.0f}, AZ::Vector3{0.0f, 1.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f},

         // Tri 3: non-degenerate
         AZ::Vector3{0.0f, 1.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 1.0f}},
        0};

    FaceTestData DegenerateFaceList = {
        {// Tri 0: degenerate
         AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{1.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f},

         // Tri 1: degenerate
         AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f},

         // Tri 2: degenerate
         AZ::Vector3{0.0f, 0.0f, 1.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f},

         // Tri 3: degenerate
         AZ::Vector3{0.0f, 0.0f, 1.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 1.0f}},
        4};

    FaceTestData DegenerateAndNonDegenerateFaceList = {
        {// Tri 0: degenerate
         AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{1.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f},

         // Tri 1: non-degenerate
         AZ::Vector3{0.0f, 1.0f, 0.0f}, AZ::Vector3{1.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f},

         // Tri 2: degenerate
         AZ::Vector3{0.0f, 0.0f, 1.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f},

         // Tri 3: non-degenerate
         AZ::Vector3{1.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 1.0f, 0.0f},

         // Tri 4: non-degenerate
         AZ::Vector3{0.0f, 0.0f, 1.0f}, AZ::Vector3{0.0f, 1.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f},

         // Tri 5: non-degenerate
         AZ::Vector3{0.0f, 1.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 1.0f},

         // Tri 6: degenerate
         AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f}, AZ::Vector3{0.0f, 0.0f, 0.0f}},
        3};

    TEST_F(WhiteBoxTestFixture, RenderCullingKeepsSmallValidFacesAtDifferentScales)
    {
        for (const float scale : {1e-6f, 1e-4f, 0.01f, 1.0f, 10000.0f})
        {
            WhiteBoxFace valid{};
            valid.m_v1.m_position = AZ::Vector3::CreateZero();
            valid.m_v2.m_position = AZ::Vector3(0, scale, 0);
            valid.m_v3.m_position = AZ::Vector3(0, 0, scale);
            valid.m_normal = AZ::Vector3::CreateAxisX();
            valid.m_v1.m_uv = AZ::Vector2(0, 0);
            valid.m_v2.m_uv = AZ::Vector2(1, 0);
            valid.m_v3.m_uv = AZ::Vector2(0, 1);
            valid.m_paintColor = 0xFF00FF00u;
            auto collapsed = valid;
            collapsed.m_v3.m_position = collapsed.m_v2.m_position;
            auto collinear = valid;
            collinear.m_v3.m_position = AZ::Vector3(0, 2.0f * scale, 0);
            const auto result = BuildCulledWhiteBoxFaces({collapsed, valid, collinear});
            ASSERT_EQ(result.size(), 1);
            EXPECT_TRUE(result.front().m_v3.m_position.IsClose(valid.m_v3.m_position));
            EXPECT_EQ(result.front().m_paintColor, valid.m_paintColor);
        }
    }

    TEST_F(WhiteBoxTestFixture, RenderCullingRejectsNonFiniteTriangles)
    {
        WhiteBoxFace face{};
        face.m_v1.m_position = AZ::Vector3::CreateZero();
        face.m_v2.m_position = AZ::Vector3::CreateAxisX();
        for (const float invalid : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
        {
            face.m_v3.m_position = AZ::Vector3(0, invalid, 0);
            EXPECT_TRUE(BuildCulledWhiteBoxFaces({face}).empty());
        }
    }

    TEST_P(WhiteBoxVertexDataTestFixture, BuildCulledTriangleList)
    {
        // given the raw positional data
        const FaceTestData& faceData = GetParam();

        // expect the vertex data to be composed of only triangle primitives
        // (we cannot proceed any further with the test if this isn't the case)
        ASSERT_EQ(faceData.m_positions.size() % 3, 0);

        // given an input list of valid and/or degenerate triangles
        const WhiteBoxFaces inVertexData = ConstructFaceData(faceData);

        // build the output list of visible triangles from the input list
        const WhiteBoxFaces outVertexData = BuildCulledWhiteBoxFaces(inVertexData);

        const size_t numInTriangles = inVertexData.size();
        const size_t numOutTriangles = outVertexData.size();

        // expect the number of triangles in the input list to always be at least as
        // as the the number of culled triangles in the output list
        EXPECT_GE(numInTriangles, numOutTriangles);

        // expect the number of triangles culled from the input list to equal that
        // of the expected number of triangles to be culled
        EXPECT_EQ(numOutTriangles, numInTriangles - faceData.m_numCulledFaces);
    }

    INSTANTIATE_TEST_SUITE_P(
        NonDegenerateFaceList, WhiteBoxVertexDataTestFixture, ::testing::Values(NonDegenerateFaceList));

    INSTANTIATE_TEST_SUITE_P(DegenerateFaceList, WhiteBoxVertexDataTestFixture, ::testing::Values(DegenerateFaceList));

    INSTANTIATE_TEST_SUITE_P(
        DegenerateAndNonDegenerateFaceList, WhiteBoxVertexDataTestFixture,
        ::testing::Values(DegenerateAndNonDegenerateFaceList));
} // namespace UnitTest
