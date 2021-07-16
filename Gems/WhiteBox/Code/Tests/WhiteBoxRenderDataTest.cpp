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

    INSTANTIATE_TEST_CASE_P(
        NonDegenerateFaceList, WhiteBoxVertexDataTestFixture, ::testing::Values(NonDegenerateFaceList));

    INSTANTIATE_TEST_CASE_P(DegenerateFaceList, WhiteBoxVertexDataTestFixture, ::testing::Values(DegenerateFaceList));

    INSTANTIATE_TEST_CASE_P(
        DegenerateAndNonDegenerateFaceList, WhiteBoxVertexDataTestFixture,
        ::testing::Values(DegenerateAndNonDegenerateFaceList));
} // namespace UnitTest
