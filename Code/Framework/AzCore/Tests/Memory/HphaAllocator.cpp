/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/HphaAllocator.h>
#include <AzCore/std/containers/vector.h>

namespace UnitTest
{
    class HphaSchema_TestAllocator : public AZ::SimpleSchemaAllocator<AZ::HphaSchema>
    {
    public:
        AZ_TYPE_INFO(HphaSchema_TestAllocator, "{ACE2D6E5-4EB8-4DD2-AE95-6BDFD0476801}");

        using Base = AZ::SimpleSchemaAllocator<AZ::HphaSchema>;

        HphaSchema_TestAllocator()
        {
            Create();
        }

        ~HphaSchema_TestAllocator() override = default;
    };

    static const size_t s_kiloByte = 1024;
    static const size_t s_megaByte = s_kiloByte * s_kiloByte;
    using AllocationSizeArray = AZStd::array<size_t, 10>;
    static const AllocationSizeArray s_smallAllocationSizes = { 2, 16, 20, 59, 100, 128, 160, 250, 300, 512 };
    static const AllocationSizeArray s_bigAllocationSizes = {
        513,        s_kiloByte,    2 * s_kiloByte, 4 * s_kiloByte, 10 * s_kiloByte, 64 * s_kiloByte, 128 * s_kiloByte, 200 * s_kiloByte,
        s_megaByte, 2 * s_megaByte
    };
    static const AllocationSizeArray s_mixedAllocationSizes = { 2,   s_kiloByte, 59,  4 * s_kiloByte, 128, 200 * s_kiloByte,
                                                                250, s_megaByte, 512, 2 * s_megaByte };

    class HphaSchemaTestParameters
    {
    public:
        HphaSchemaTestParameters(const AllocationSizeArray& allocationSizes,
                                 size_t numberOfAllocationsPerSize)
            : m_allocationSizes(allocationSizes)
            , m_numberOfAllocationsPerSize(numberOfAllocationsPerSize)
        {}

        const AllocationSizeArray& m_allocationSizes;
        const size_t m_numberOfAllocationsPerSize;
    };

    class HphaSchemaTestFixture
        : public LeakDetectionFixture
        , public ::testing::WithParamInterface<HphaSchemaTestParameters>
    {
    };

    TEST_P(HphaSchemaTestFixture, Allocate)
    {
        AZStd::vector<void*, AZ::OSStdAllocator> allocations;
        const HphaSchemaTestParameters& testParameters = GetParam();
        const size_t totalNumberOfAllocations = testParameters.m_allocationSizes.size() * testParameters.m_numberOfAllocationsPerSize;
        for (size_t i = 0; i < totalNumberOfAllocations; ++i)
        {
            const size_t allocationIndex = allocations.size();
            const size_t allocationSize = testParameters.m_allocationSizes[allocationIndex % testParameters.m_allocationSizes.size()];
            void* allocation = AZ::AllocatorInstance<HphaSchema_TestAllocator>::Get().Allocate(allocationSize, 0);
            ASSERT_NE(nullptr, allocation);
            EXPECT_LE(allocationSize, AZ::AllocatorInstance<HphaSchema_TestAllocator>::Get().AllocationSize(allocation));
            allocations.emplace_back(allocation);
        }

        const size_t numberOfAllocations = allocations.size();
        for (size_t i = 0; i < numberOfAllocations; ++i)
        {
            AZ::AllocatorInstance<HphaSchema_TestAllocator>::Get().DeAllocate(allocations[i], testParameters.m_allocationSizes[i % testParameters.m_allocationSizes.size()]);
        }
    }

    static const AZStd::array<HphaSchemaTestParameters, 2> s_smallInstancesParameters = {
         HphaSchemaTestParameters(s_smallAllocationSizes, 2),
         HphaSchemaTestParameters(s_smallAllocationSizes, 100)
    };
    INSTANTIATE_TEST_CASE_P(Small,
        HphaSchemaTestFixture,
        ::testing::ValuesIn(s_smallInstancesParameters));

    static const AZStd::array<HphaSchemaTestParameters, 2> s_bigInstancesParameters = {
         HphaSchemaTestParameters(s_bigAllocationSizes, 2),
         HphaSchemaTestParameters(s_bigAllocationSizes, 100)
    };
    INSTANTIATE_TEST_CASE_P(Big,
        HphaSchemaTestFixture,
        ::testing::ValuesIn(s_bigInstancesParameters));

    static const AZStd::array<HphaSchemaTestParameters, 2> s_mixedInstancesParameters = {
         HphaSchemaTestParameters(s_mixedAllocationSizes, 2),
         HphaSchemaTestParameters(s_mixedAllocationSizes, 100)
    };
    INSTANTIATE_TEST_CASE_P(Mixed,
        HphaSchemaTestFixture,
        ::testing::ValuesIn(s_mixedInstancesParameters));
}
