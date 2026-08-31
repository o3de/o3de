/*
* Modifications Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <SharedBuffer.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace AZ::Meshlets;

    // ---------------------------------------------------------------------
    // SharedBuffer::ComputeAlignment
    // ---------------------------------------------------------------------

    class SharedBufferAlignmentTests : public ::testing::Test
    {
    protected:
        static SrgBufferDescriptor DescriptorWithElementSize(uint32_t elementSize)
        {
            SrgBufferDescriptor d;
            d.m_elementSize = elementSize;
            d.m_elementCount = 1;
            return d;
        }
    };

    TEST_F(SharedBufferAlignmentTests, NoDescriptors_ReturnsMinAllowedAlignment)
    {
        AZStd::vector<SrgBufferDescriptor> empty;
        EXPECT_EQ(16u, SharedBuffer::ComputeAlignment(empty, /*minAllowedAlignment=*/16));
    }

    TEST_F(SharedBufferAlignmentTests, AllElementSizesDivideMinAlignment_ReturnsMinAlignment)
    {
        // 4 and 8 both divide 16, so the LCM is still 16. Historical bug: older code
        // added an extra (16 - 0) padding step here, doubling to 32.
        AZStd::vector<SrgBufferDescriptor> descs = {
            DescriptorWithElementSize(4),
            DescriptorWithElementSize(8),
        };
        EXPECT_EQ(16u, SharedBuffer::ComputeAlignment(descs, /*minAllowedAlignment=*/16));
    }

    TEST_F(SharedBufferAlignmentTests, ElementSizeLargerThanMinAlignment_TakesLcm)
    {
        // lcm(16, 12) = 48. 48 is already a multiple of 16, so no extra padding.
        AZStd::vector<SrgBufferDescriptor> descs = {
            DescriptorWithElementSize(12),
        };
        EXPECT_EQ(48u, SharedBuffer::ComputeAlignment(descs, /*minAllowedAlignment=*/16));
    }

    TEST_F(SharedBufferAlignmentTests, OddElementSize_ResultIsAlwaysMultipleOfMinAlignment)
    {
        // lcm(16, 7) = 112. Multiple of 16.
        AZStd::vector<SrgBufferDescriptor> descs = {
            DescriptorWithElementSize(7),
        };
        const uint32_t result = SharedBuffer::ComputeAlignment(descs, /*minAllowedAlignment=*/16);
        EXPECT_EQ(0u, result % 16u);
        EXPECT_EQ(112u, result);
    }

    TEST_F(SharedBufferAlignmentTests, MixedSizes_CombinesViaLcmAcrossAllDescriptors)
    {
        // lcm(16, 12, 20) = 240. Verify it's both a multiple of every element size
        // and a multiple of MinAllowedAlignment.
        AZStd::vector<SrgBufferDescriptor> descs = {
            DescriptorWithElementSize(12),
            DescriptorWithElementSize(20),
        };
        const uint32_t result = SharedBuffer::ComputeAlignment(descs, /*minAllowedAlignment=*/16);
        EXPECT_EQ(0u, result % 12u);
        EXPECT_EQ(0u, result % 20u);
        EXPECT_EQ(0u, result % 16u);
    }

    AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
}
