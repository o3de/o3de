/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <SkinnedMesh/SkinnedMeshDispatchItem.h>

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/UnitTest.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Render;

    TEST(SkinnedMeshDispatchItemTest, CalculateSkinnedMeshTotalThreadsPerDimension_TotalThreadsLessThanPerDimensionMax_AllThreadsInXDimension)
    {
        uint32_t maxThreadsPerDimension = static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
        uint32_t xThreads = 0;
        uint32_t yThreads = 0;

        // Test minimum threads for one dimension
        uint32_t vertexCount = 1;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount);
        EXPECT_EQ(yThreads, 1);

        // Test maximum threads for one dimension
        vertexCount = maxThreadsPerDimension;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount);
        EXPECT_EQ(yThreads, 1);
    }

    TEST(SkinnedMeshDispatchItemTest, CalculateSkinnedMeshTotalThreadsPerDimension_TotalThreadsEvenlyDivisibleByYThreads_XYProductEqualsTotalVertexCount)
    {
        uint32_t maxThreadsPerDimension = static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
        uint32_t xThreads = 0;
        uint32_t yThreads = 0;

        // Test for one vertex more than the max that can fit in the x dimension
        uint32_t vertexCount = maxThreadsPerDimension + 1;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 2);
        EXPECT_EQ(yThreads, 2);
        EXPECT_EQ(xThreads * yThreads, vertexCount);

        // Test for two vertices less than the max that can fit with two y threads
        vertexCount = maxThreadsPerDimension * 2 - 2;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 2);
        EXPECT_EQ(yThreads, 2);
        EXPECT_EQ(xThreads * yThreads, vertexCount);

        // Test for the max number of vertices that can fit with two y threads
        vertexCount = maxThreadsPerDimension * 2;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 2);
        EXPECT_EQ(yThreads, 2);
        EXPECT_EQ(xThreads * yThreads, vertexCount);

        // Test for three vertices more than the max that can fit with two y threads
        vertexCount = maxThreadsPerDimension * 2 + 3;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 3);
        EXPECT_EQ(yThreads, 3);
        EXPECT_EQ(xThreads * yThreads, vertexCount);

        // Test for three vertices less than the max that can fit with two y threads
        vertexCount = maxThreadsPerDimension * 3 - 3;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 3);
        EXPECT_EQ(yThreads, 3);
        EXPECT_EQ(xThreads * yThreads, vertexCount);

        // Test for one fewer dimension than the max
        vertexCount = maxThreadsPerDimension * (maxThreadsPerDimension - 1);
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, maxThreadsPerDimension);
        EXPECT_EQ(yThreads, maxThreadsPerDimension - 1);
        EXPECT_EQ(static_cast<uint32_t>(xThreads) * static_cast<uint32_t>(yThreads), vertexCount);

        // Test for maximum supported vertex count in each dimension
        vertexCount = maxThreadsPerDimension * maxThreadsPerDimension;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, maxThreadsPerDimension);
        EXPECT_EQ(yThreads, maxThreadsPerDimension);
        EXPECT_EQ(static_cast<uint32_t>(xThreads) * static_cast<uint32_t>(yThreads), vertexCount);
    }


    TEST(SkinnedMeshDispatchItemTest, CalculateSkinnedMeshTotalThreadsPerDimension_TotalThreadsNotEvenlyDivisibleByYThreads_ExtraXThreadAndTotalThreadsExceedsVertexCount)
    {
        uint32_t maxThreadsPerDimension = static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
        uint32_t xThreads = 0;
        uint32_t yThreads = 0;

        // Test for two vertices more than the max that can fit in the x dimension
        uint32_t vertexCount = maxThreadsPerDimension + 2;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 2 + 1);
        EXPECT_EQ(yThreads, 2);
        EXPECT_EQ(xThreads * yThreads, vertexCount + 1);

        // Test for one vertex less than the max that can fit with two y threads
        vertexCount = maxThreadsPerDimension * 2 - 1;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 2 + 1);
        EXPECT_EQ(yThreads, 2);
        EXPECT_EQ(xThreads * yThreads, vertexCount + 1);

        // Test for one vertex more than the max that can fit with two y threads
        vertexCount = maxThreadsPerDimension * 2 + 1;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 3 + 1);
        EXPECT_EQ(yThreads, 3);
        EXPECT_EQ(xThreads * yThreads, vertexCount + 2);

        // Test for two vertices more than the max that can fit with two y threads
        vertexCount = maxThreadsPerDimension * 2 + 2;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 3 + 1);
        EXPECT_EQ(yThreads, 3);
        EXPECT_EQ(xThreads * yThreads, vertexCount + 1);

        // Test for two vertices less than the max that can fit with three y threads
        vertexCount = maxThreadsPerDimension * 3 - 2;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 3 + 1);
        EXPECT_EQ(yThreads, 3);
        EXPECT_EQ(xThreads * yThreads, vertexCount + 2);

        // Test for one vertex less than the max that can fit with three y threads
        vertexCount = maxThreadsPerDimension * 3 - 1;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, vertexCount / 3 + 1);
        EXPECT_EQ(yThreads, 3);
        EXPECT_EQ(xThreads * yThreads, vertexCount + 1);

        // Test for the fewest number of vertices that would still max out each dimension
        vertexCount = maxThreadsPerDimension * (maxThreadsPerDimension - 1) + 1;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, maxThreadsPerDimension);
        EXPECT_EQ(yThreads, maxThreadsPerDimension);
        EXPECT_EQ(static_cast<uint32_t>(xThreads) * static_cast<uint32_t>(yThreads), vertexCount + (maxThreadsPerDimension - 1));

        // Test for one vertex less than the maximum supported vertex count
        vertexCount = maxThreadsPerDimension * maxThreadsPerDimension - 1;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        EXPECT_EQ(xThreads, maxThreadsPerDimension);
        EXPECT_EQ(yThreads, maxThreadsPerDimension);
        EXPECT_EQ(static_cast<uint32_t>(xThreads) * static_cast<uint32_t>(yThreads), vertexCount + 1);
    }

    TEST(SkinnedMeshDispatchItemTest, CalculateSkinnedMeshTotalThreadsPerDimension_VertexCountExceedsMaxSupported_Error)
    {
        uint32_t maxThreadsPerDimension = static_cast<uint32_t>(std::numeric_limits<uint16_t>::max());
        uint32_t xThreads = 0;
        uint32_t yThreads = 0;

        // Test beyond maximum supported vertex count
        uint32_t vertexCount = static_cast<uint32_t>(std::numeric_limits<uint32_t>::max());
        AZ_TEST_START_ASSERTTEST;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(xThreads, maxThreadsPerDimension);
        EXPECT_EQ(yThreads, maxThreadsPerDimension);
        EXPECT_NE(static_cast<uint32_t>(xThreads) * static_cast<uint32_t>(yThreads), vertexCount);
    }    

    TEST(SkinnedMeshDispatchItemTest, CalculateSkinnedMeshTotalThreadsPerDimension_VertexCountIsZero_Error)
    {
        uint32_t xThreads = 0;
        uint32_t yThreads = 0;

        // Test zero vertex count
        uint32_t vertexCount = 0;
        AZ_TEST_START_ASSERTTEST;
        CalculateSkinnedMeshTotalThreadsPerDimension(vertexCount, xThreads, yThreads);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(xThreads, 0);
        EXPECT_EQ(yThreads, 0);
    }
} // namespace UnitTest
