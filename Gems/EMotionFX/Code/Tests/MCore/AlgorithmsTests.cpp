/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <MCore/Source/Algorithms.h>
#include <Tests/SystemComponentFixture.h>

namespace EMotionFX
{
    using AlgorithmsTestsFixture = SystemComponentFixture;

    // Test if the smooth data remain unchanged when giving a motion data that contains the same data point.
    TEST_F(AlgorithmsTestsFixture, MovingAverageSmoothVec3Basic)
    {
        const size_t tests_size = 100;
        AZStd::vector<AZ::Vector3> motionData;
        motionData.reserve(tests_size);
        for (size_t i = 0; i < tests_size; ++i)
        {
            motionData.emplace_back(AZ::Vector3(1.0f, 2.0f, 3.0f));
        }

        // Try it with all the possible sample frame number.
        for (size_t i = 1; i < 10; ++i)
        {
            AZStd::vector<AZ::Vector3> smoothedData = motionData;
            MCore::MovingAverageSmooth(smoothedData, i);
            EXPECT_EQ(smoothedData, motionData);
        }
    }

    // Test on the algorithms with motion data that has a particular pattern.
    TEST_F(AlgorithmsTestsFixture, MovingAverageSmoothVec3)
    {
        const size_t tests_size = 100;
        AZStd::vector<AZ::Vector3> motionData;
        motionData.reserve(tests_size);
        for (size_t i = 0; i < tests_size; ++i)
        {
            float num = aznumeric_caster(i);
            motionData.emplace_back(AZ::Vector3(num, 2.0f * num, 3.0f * num));
        }

        // Try it with all the possible sample frame number.
        for (size_t i = 1; i < 10; ++i)
        {
            AZStd::vector<AZ::Vector3> smoothedData = motionData;
            MCore::MovingAverageSmooth(smoothedData, i);
            EXPECT_EQ(smoothedData, motionData);
        }
    }
} // namespace MCore
