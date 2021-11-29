/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Memory/HphaSchema.h>
#include <AzCore/std/containers/vector.h>

#if defined(HAVE_BENCHMARK)
#include <benchmark/benchmark.h>
#endif // HAVE_BENCHMARK

class HphaSchema_TestAllocator
    : public AZ::SimpleSchemaAllocator<AZ::HphaSchema>
{
public:
    AZ_TYPE_INFO(HphaSchema_TestAllocator, "{ACE2D6E5-4EB8-4DD2-AE95-6BDFD0476801}");

    using Base = AZ::SimpleSchemaAllocator<AZ::HphaSchema>;
    using Descriptor = Base::Descriptor;

    HphaSchema_TestAllocator()
        : Base("HphaSchema_TestAllocator", "Allocator for Test")
    {}
};

static const size_t s_kiloByte = 1024;
static const size_t s_megaByte = s_kiloByte * s_kiloByte;
using AllocationSizeArray = AZStd::array<size_t, 10>;
static const AllocationSizeArray s_smallAllocationSizes = { 2, 16, 20, 59, 100, 128, 160, 250, 300, 512 };
static const AllocationSizeArray s_bigAllocationSizes = { 513, s_kiloByte, 2 * s_kiloByte, 4 * s_kiloByte, 10 * s_kiloByte, 64 * s_kiloByte, 128 * s_kiloByte, 200 * s_kiloByte, s_megaByte, 2 * s_megaByte };
static const AllocationSizeArray s_mixedAllocationSizes = { 2, s_kiloByte, 59, 4 * s_kiloByte, 128, 200 * s_kiloByte, 250, s_megaByte, 512, 2 * s_megaByte };

namespace UnitTest
{
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
        : public AllocatorsTestFixture
        , public ::testing::WithParamInterface<HphaSchemaTestParameters>
    {
    public:
        void SetUp() override
        {
            AZ::AllocatorInstance<HphaSchema_TestAllocator>::Create();
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<HphaSchema_TestAllocator>::Destroy();
        }
    };

    TEST_P(HphaSchemaTestFixture, Allocate)
    {
        AZStd::vector<void*, AZ::AZStdAlloc<AZ::OSAllocator>> allocations;
        const HphaSchemaTestParameters& testParameters = GetParam();
        const size_t totalNumberOfAllocations = testParameters.m_allocationSizes.size() * testParameters.m_numberOfAllocationsPerSize;
        for (size_t i = 0; i < totalNumberOfAllocations; ++i)
        {
            const size_t allocationIndex = allocations.size();
            const size_t allocationSize = testParameters.m_allocationSizes[allocationIndex % testParameters.m_allocationSizes.size()];
            void* allocation = AZ::AllocatorInstance<HphaSchema_TestAllocator>::Get().Allocate(allocationSize, 0);
            EXPECT_NE(nullptr, allocation);
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


#if defined(HAVE_BENCHMARK)
namespace Benchmark
{
    class HphaSchemaBenchmarkFixture 
        : public ::benchmark::Fixture
    {
        void internalSetUp()
        {
            AZ::AllocatorInstance<HphaSchema_TestAllocator>::Create();
        }

        void internalTearDown()
        {
            AZ::AllocatorInstance<HphaSchema_TestAllocator>::Destroy();
        }

    public:
        void SetUp(const benchmark::State&) override
        {
            internalSetUp();
        }
        void SetUp(benchmark::State&) override
        {
            internalSetUp();
        }
        void TearDown(const benchmark::State&) override
        {
            internalTearDown();
        }
        void TearDown(benchmark::State&) override
        {
            internalTearDown();
        }

        static void BM_Allocations(benchmark::State& state, const AllocationSizeArray& allocationArray)
        {
            AZStd::vector<void*> allocations;
            while (state.KeepRunning())
            {
                state.PauseTiming();
                const size_t allocationIndex = allocations.size();
                const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];

                state.ResumeTiming();
                void* allocation = AZ::AllocatorInstance<HphaSchema_TestAllocator>::Get().Allocate(allocationSize, 0);

                state.PauseTiming();
                allocations.emplace_back(allocation);
           
                state.ResumeTiming();
            }

            const size_t numberOfAllocations = allocations.size();
            state.SetItemsProcessed(numberOfAllocations);

            for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
            {
                AZ::AllocatorInstance<HphaSchema_TestAllocator>::Get().DeAllocate(allocations[allocationIndex], allocationArray[allocationIndex % allocationArray.size()]);
            }
            AZ::AllocatorInstance<HphaSchema_TestAllocator>::Get().GarbageCollect();
        }
    };

    // Small allocations, these are allocations that are going to end up in buckets in the HphaSchema
    BENCHMARK_F(HphaSchemaBenchmarkFixture, SmallAllocations)(benchmark::State& state)
    {
        BM_Allocations(state, s_smallAllocationSizes);
    }

    BENCHMARK_F(HphaSchemaBenchmarkFixture, BigAllocations)(benchmark::State& state)
    {
        BM_Allocations(state, s_bigAllocationSizes);
    }

    BENCHMARK_F(HphaSchemaBenchmarkFixture, MixedAllocations)(benchmark::State& state)
    {
        BM_Allocations(state, s_mixedAllocationSizes);
    }


} // Benchmark
#endif // HAVE_BENCHMARK
