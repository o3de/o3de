/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/MimallocAllocator.h>
#include <AzCore/std/containers/vector.h>

namespace UnitTest
{
    class MimallocSchema_TestAllocator : public AZ::SimpleSchemaAllocator<AZ::MimallocSchema>
    {
    public:
        AZ_TYPE_INFO(MimallocSchema_TestAllocator, "{ACE2D6E5-4EB8-4DD2-AE95-6BDFD0476801}");

        using Base = AZ::SimpleSchemaAllocator<AZ::MimallocSchema>;

        MimallocSchema_TestAllocator()
        {
            Create();
        }

        ~MimallocSchema_TestAllocator() override = default;
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

    class MimallocSchemaTestParameters
    {
    public:
        MimallocSchemaTestParameters(const AllocationSizeArray& allocationSizes,
                                 size_t numberOfAllocationsPerSize)
            : m_allocationSizes(allocationSizes)
            , m_numberOfAllocationsPerSize(numberOfAllocationsPerSize)
        {}

        const AllocationSizeArray& m_allocationSizes;
        const size_t m_numberOfAllocationsPerSize;
    };

    class MimallocSchemaTestFixture
        : public LeakDetectionFixture
        , public ::testing::WithParamInterface<MimallocSchemaTestParameters>
    {
    };

    TEST_P(MimallocSchemaTestFixture, Allocate)
    {
        AZStd::vector<void*, AZ::OSStdAllocator> allocations;
        const MimallocSchemaTestParameters& testParameters = GetParam();
        const size_t totalNumberOfAllocations = testParameters.m_allocationSizes.size() * testParameters.m_numberOfAllocationsPerSize;
        for (size_t i = 0; i < totalNumberOfAllocations; ++i)
        {
            const size_t allocationIndex = allocations.size();
            const size_t allocationSize = testParameters.m_allocationSizes[allocationIndex % testParameters.m_allocationSizes.size()];
            void* allocation = AZ::AllocatorInstance<MimallocSchema_TestAllocator>::Get().Allocate(allocationSize, 0);
            ASSERT_NE(nullptr, allocation);
            EXPECT_LE(allocationSize, AZ::AllocatorInstance<MimallocSchema_TestAllocator>::Get().AllocationSize(allocation));
            allocations.emplace_back(allocation);
        }

        const size_t numberOfAllocations = allocations.size();
        for (size_t i = 0; i < numberOfAllocations; ++i)
        {
            AZ::AllocatorInstance<MimallocSchema_TestAllocator>::Get().DeAllocate(allocations[i], testParameters.m_allocationSizes[i % testParameters.m_allocationSizes.size()]);
        }
    }

    static const AZStd::array<MimallocSchemaTestParameters, 2> s_smallInstancesParameters = {
         MimallocSchemaTestParameters(s_smallAllocationSizes, 2),
         MimallocSchemaTestParameters(s_smallAllocationSizes, 100)
    };
    INSTANTIATE_TEST_SUITE_P(Small,
        MimallocSchemaTestFixture,
        ::testing::ValuesIn(s_smallInstancesParameters));

    static const AZStd::array<MimallocSchemaTestParameters, 2> s_bigInstancesParameters = {
         MimallocSchemaTestParameters(s_bigAllocationSizes, 2),
         MimallocSchemaTestParameters(s_bigAllocationSizes, 100)
    };
    INSTANTIATE_TEST_SUITE_P(Big,
        MimallocSchemaTestFixture,
        ::testing::ValuesIn(s_bigInstancesParameters));

    static const AZStd::array<MimallocSchemaTestParameters, 2> s_mixedInstancesParameters = {
         MimallocSchemaTestParameters(s_mixedAllocationSizes, 2),
         MimallocSchemaTestParameters(s_mixedAllocationSizes, 100)
    };
    INSTANTIATE_TEST_SUITE_P(Mixed,
        MimallocSchemaTestFixture,
        ::testing::ValuesIn(s_mixedInstancesParameters));
}
