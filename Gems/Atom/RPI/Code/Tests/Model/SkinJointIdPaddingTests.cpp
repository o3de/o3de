/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Atom/RPI.Reflect/Model/SkinJointIdPadding.h>

namespace UnitTest
{
    TEST(SkinJointIdPaddingTest, CalculateJointIdPaddingCount_Is16ByteAligned)
    {
        for (uint32_t jointIdCount = 1; jointIdCount <= 32; ++jointIdCount)
        {
            uint32_t byteSize = (AZ::RPI::CalculateJointIdPaddingCount(jointIdCount) + jointIdCount) * sizeof(uint16_t);
            // Validate that the size in bytes of a given number of joints plus their padding will always be 16-byte aligned
            EXPECT_EQ(byteSize % 16, 0);
        }
    }
} // namespace UnitTest
