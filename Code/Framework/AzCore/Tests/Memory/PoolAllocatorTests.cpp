/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/containers/vector.h>

constexpr size_t s_testPoolPageSize = 1024;
constexpr size_t s_testPoolMinAllocationSize = 64;
constexpr size_t s_testPoolMaxAllocationSize = 256;

// Define a custom pool allocator
class TestPoolAllocator final : public AZ::Internal::PoolAllocatorHelper<AZ::PoolSchema>
{
public:
    AZ_CLASS_ALLOCATOR(TestPoolAllocator, AZ::SystemAllocator, 0)
    AZ_TYPE_INFO(TestPoolAllocator, "{5AB88680-581F-4F0B-B67F-BAEAA78D3122}")

    TestPoolAllocator()
        // Invoke the base constructor explicitely to use the override that takes custom page, min, and max allocation sizes
        : AZ::Internal::PoolAllocatorHelper<AZ::PoolSchema>(s_testPoolPageSize, s_testPoolMinAllocationSize, s_testPoolMaxAllocationSize)
    {
    }
};

using AllocationSizeArray = AZStd::fixed_vector<size_t, 10>;
static const AllocationSizeArray s_smallAllocationSizes = {s_testPoolMinAllocationSize, s_testPoolMinAllocationSize + 1 };
static const AllocationSizeArray s_bigAllocationSizes = { s_testPoolMaxAllocationSize, s_testPoolMaxAllocationSize - 1 };
static const AllocationSizeArray s_mixedAllocationSizes = { 64, 65, 127, 128, 129, 255, 256 };
namespace UnitTest
{
    class PoolAllocatorTestParameters
    {
    public:
        PoolAllocatorTestParameters(const AllocationSizeArray& allocationSizes,
                                 size_t numberOfAllocationsPerSize)
            : m_allocationSizes(allocationSizes)
            , m_numberOfAllocationsPerSize(numberOfAllocationsPerSize)
        {}

        const AllocationSizeArray& m_allocationSizes;
        const size_t m_numberOfAllocationsPerSize;
    };

    class PoolAllocatorTestFixture
        : public LeakDetectionFixture
        , public ::testing::WithParamInterface<PoolAllocatorTestParameters>
    {
    };

    class PoolAllocatorDeathTestFixture : public PoolAllocatorTestFixture
        {};

    void DoTestAllocations(const PoolAllocatorTestParameters& testParameters)
    {
        AZStd::vector<void*, AZ::AZStdAlloc<AZ::OSAllocator>> allocations;
        const size_t totalNumberOfAllocations = testParameters.m_allocationSizes.size() * testParameters.m_numberOfAllocationsPerSize;
        for (size_t i = 0; i < totalNumberOfAllocations; ++i)
        {
            const size_t allocationIndex = allocations.size();
            const size_t allocationSize = testParameters.m_allocationSizes[allocationIndex % testParameters.m_allocationSizes.size()];
            void* allocation = AZ::AllocatorInstance<TestPoolAllocator>::Get().Allocate(allocationSize, s_testPoolMinAllocationSize);
            EXPECT_NE(nullptr, allocation);
            EXPECT_LE(allocationSize, AZ::AllocatorInstance<TestPoolAllocator>::Get().AllocationSize(allocation));
            allocations.emplace_back(allocation);
        }

        const size_t numberOfAllocations = allocations.size();
        for (size_t i = 0; i < numberOfAllocations; ++i)
        {
            AZ::AllocatorInstance<TestPoolAllocator>::Get().DeAllocate(
                allocations[i], testParameters.m_allocationSizes[i % testParameters.m_allocationSizes.size()]);
        }

    }

    TEST_P(PoolAllocatorTestFixture, Allocate)
    {
        const PoolAllocatorTestParameters& testParameters = GetParam();
        DoTestAllocations(testParameters);
    }

    static const AZStd::array<PoolAllocatorTestParameters, 2> s_smallInstancesParameters = {
        PoolAllocatorTestParameters(s_smallAllocationSizes, 2), PoolAllocatorTestParameters(s_smallAllocationSizes, 100)
    };
    INSTANTIATE_TEST_CASE_P(SmallAllocation, PoolAllocatorTestFixture,
        ::testing::ValuesIn(s_smallInstancesParameters));

    static const AZStd::array<PoolAllocatorTestParameters, 2> s_bigInstancesParameters = {
        PoolAllocatorTestParameters(s_bigAllocationSizes, 2), PoolAllocatorTestParameters(s_bigAllocationSizes, 100)
    };
    INSTANTIATE_TEST_CASE_P(BigAllocation, PoolAllocatorTestFixture,
        ::testing::ValuesIn(s_bigInstancesParameters));

    static const AZStd::array<PoolAllocatorTestParameters, 2> s_mixedInstancesParameters = {
        PoolAllocatorTestParameters(s_mixedAllocationSizes, 2), PoolAllocatorTestParameters(s_mixedAllocationSizes, 100)
    };
    INSTANTIATE_TEST_CASE_P(MixedAllocation, PoolAllocatorTestFixture,
        ::testing::ValuesIn(s_mixedInstancesParameters));

    TEST_F(PoolAllocatorDeathTestFixture, PoolAllocator_AllocateLessThanMinAllocationSize_FailsToAllocate)
    {
        // Validate that the minimum allocation size of a custom PoolAllocator is respected
        AZ::IAllocator::pointer result =
            AZ::AllocatorInstance<TestPoolAllocator>::Get().Allocate(s_testPoolMinAllocationSize - 1, 1);
        EXPECT_EQ(AZ::AllocatorInstance<TestPoolAllocator>::Get().NumAllocatedBytes(), s_testPoolMinAllocationSize);

        // DeAllocate so the test doesn't leak
        AZ::AllocatorInstance<TestPoolAllocator>::Get().DeAllocate(result);
    }

    TEST_F(PoolAllocatorDeathTestFixture, PoolAllocator_AllocateMoreThanMaxAllocationSize_FailsToAllocate)
    {
        // Validate that the maximum allocation size of a custom PoolAllocator is respected
        AZ_TEST_START_ASSERTTEST;
        AZ::IAllocator::pointer result =
            AZ::AllocatorInstance<TestPoolAllocator>::Get().Allocate(s_testPoolMaxAllocationSize + 1, 1);
        AZ_TEST_STOP_ASSERTTEST(1);
        EXPECT_EQ(result, nullptr);
    }
}
