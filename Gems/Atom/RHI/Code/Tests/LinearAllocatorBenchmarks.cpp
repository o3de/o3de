/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <Atom/RHI/LinearAllocator.h>
#include <Atom/RHI/FreeListAllocator.h>
#include <Atom/RHI/PoolAllocator.h>
#include <AzCore/Math/Random.h>
#include <AzCore/std/containers/vector.h>

#include <benchmark/benchmark.h>

namespace Benchmark
{
    // Allocation size categories for benchmarking
    static constexpr size_t SmallAllocationSize = 64;
    static constexpr size_t MediumAllocationSize = 1024;
    static constexpr size_t LargeAllocationSize = 16 * 1024;
    static constexpr size_t DefaultAlignment = 256;

    // Counter names for reporting
    static const char* s_counterTotalAllocated = "TotalAllocatedBytes";
    static const char* s_counterAllocationCount = "AllocationCount";

    //
    // LinearAllocator Benchmarks
    //
    // These benchmarks exercise the arena-style LinearAllocator which:
    // - Allocates by bumping a cursor (O(1) allocation)
    // - Ignores individual DeAllocate calls (no-op)
    // - Reclaims all memory at once via GarbageCollect
    //

    class LinearAllocatorFixture : public ::benchmark::Fixture
    {
    public:
        void SetUp(const ::benchmark::State& state) override
        {
            AZ::RHI::LinearAllocator::Descriptor desc;
            desc.m_capacityInBytes = static_cast<size_t>(state.range(0));
            desc.m_addressBase = AZ::RHI::VirtualAddress::CreateFromOffset(0);
            desc.m_garbageCollectLatency = 0;
            m_allocator.Init(desc);
        }

        void TearDown([[maybe_unused]] const ::benchmark::State& state) override
        {
            m_allocator.Shutdown();
        }

    protected:
        AZ::RHI::LinearAllocator m_allocator;
    };

    // Benchmark: Sequential small allocations until capacity is reached
    // This simulates a frame where many small objects are allocated
    BENCHMARK_DEFINE_F(LinearAllocatorFixture, BM_SequentialSmallAllocations)(benchmark::State& state)
    {
        const size_t allocationSize = SmallAllocationSize;
        const size_t capacity = static_cast<size_t>(state.range(0));
        const size_t maxAllocations = capacity / allocationSize;

        for (auto _ : state)
        {
            size_t allocationCount = 0;

            // Allocate until full
            for (size_t i = 0; i < maxAllocations; ++i)
            {
                AZ::RHI::VirtualAddress addr = m_allocator.Allocate(allocationSize, DefaultAlignment);
                if (addr.IsNull())
                {
                    break;
                }
                ++allocationCount;
                benchmark::DoNotOptimize(addr);
            }

            state.PauseTiming();
            m_allocator.GarbageCollectForce();
            state.ResumeTiming();

            state.counters[s_counterAllocationCount] = benchmark::Counter(
                static_cast<double>(allocationCount), benchmark::Counter::kDefaults);
        }

        state.SetItemsProcessed(state.iterations() * maxAllocations);
        state.SetBytesProcessed(state.iterations() * maxAllocations * allocationSize);
    }

    // Benchmark: Mixed allocation sizes (simulates real-world usage)
    BENCHMARK_DEFINE_F(LinearAllocatorFixture, BM_MixedSizeAllocations)(benchmark::State& state)
    {
        AZ::SimpleLcgRandom random(42); // Fixed seed for reproducibility

        // Pre-generate allocation sizes for consistency
        AZStd::vector<size_t> allocationSizes;
        allocationSizes.reserve(10000);
        for (size_t i = 0; i < 10000; ++i)
        {
            size_t sizeClass = random.GetRandom() % 3;
            switch (sizeClass)
            {
            case 0: allocationSizes.push_back(SmallAllocationSize); break;
            case 1: allocationSizes.push_back(MediumAllocationSize); break;
            case 2: allocationSizes.push_back(LargeAllocationSize); break;
            }
        }

        for (auto _ : state)
        {
            size_t totalAllocated = 0;
            size_t allocationCount = 0;

            for (size_t allocSize : allocationSizes)
            {
                AZ::RHI::VirtualAddress addr = m_allocator.Allocate(allocSize, DefaultAlignment);
                if (addr.IsNull())
                {
                    break;
                }
                totalAllocated += allocSize;
                ++allocationCount;
                benchmark::DoNotOptimize(addr);
            }

            state.PauseTiming();
            state.counters[s_counterTotalAllocated] = benchmark::Counter(
                static_cast<double>(totalAllocated), benchmark::Counter::kDefaults);
            state.counters[s_counterAllocationCount] = benchmark::Counter(
                static_cast<double>(allocationCount), benchmark::Counter::kDefaults);
            m_allocator.GarbageCollectForce();
            state.ResumeTiming();
        }
    }

    // Benchmark: Frame simulation - allocate many, GC once per "frame"
    // This is the primary use case for LinearAllocator in rendering
    BENCHMARK_DEFINE_F(LinearAllocatorFixture, BM_FrameSimulation)(benchmark::State& state)
    {
        const size_t capacity = static_cast<size_t>(state.range(0));
        const size_t allocationsPerFrame = state.range(1);
        const size_t allocationSize = capacity / (allocationsPerFrame * 2); // Use ~50% capacity per frame

        size_t framesProcessed = 0;

        for (auto _ : state)
        {
            // Simulate a frame: many allocations
            for (size_t i = 0; i < allocationsPerFrame; ++i)
            {
                AZ::RHI::VirtualAddress addr = m_allocator.Allocate(allocationSize, DefaultAlignment);
                benchmark::DoNotOptimize(addr);
            }

            // End of frame: garbage collect
            m_allocator.GarbageCollectForce();
            ++framesProcessed;
        }

        state.counters["FramesProcessed"] = benchmark::Counter(
            static_cast<double>(framesProcessed), benchmark::Counter::kDefaults);
        state.SetItemsProcessed(state.iterations() * allocationsPerFrame);
    }

    // Benchmark: Allocation throughput with varying alignment requirements
    BENCHMARK_DEFINE_F(LinearAllocatorFixture, BM_VariableAlignment)(benchmark::State& state)
    {
        const size_t allocationSize = MediumAllocationSize;

        // Test different alignment values
        static const size_t alignments[] = { 1, 4, 16, 64, 256 };
        const size_t numAlignments = sizeof(alignments) / sizeof(alignments[0]);

        for (auto _ : state)
        {
            size_t allocationCount = 0;

            for (size_t i = 0; ; ++i)
            {
                size_t alignment = alignments[i % numAlignments];
                AZ::RHI::VirtualAddress addr = m_allocator.Allocate(allocationSize, alignment);
                if (addr.IsNull())
                {
                    break;
                }
                ++allocationCount;
                benchmark::DoNotOptimize(addr);
            }

            state.PauseTiming();
            m_allocator.GarbageCollectForce();
            state.ResumeTiming();

            state.counters[s_counterAllocationCount] = benchmark::Counter(
                static_cast<double>(allocationCount), benchmark::Counter::kDefaults);
        }
    }

    // Benchmark: Compare DeAllocate overhead (should be negligible for LinearAllocator)
    BENCHMARK_DEFINE_F(LinearAllocatorFixture, BM_DeAllocateNoOp)(benchmark::State& state)
    {
        const size_t allocationSize = SmallAllocationSize;
        const size_t numAllocations = 1000;

        AZStd::vector<AZ::RHI::VirtualAddress> addresses;
        addresses.reserve(numAllocations);

        for (auto _ : state)
        {
            state.PauseTiming();
            addresses.clear();

            // Pre-allocate
            for (size_t i = 0; i < numAllocations; ++i)
            {
                addresses.push_back(m_allocator.Allocate(allocationSize, DefaultAlignment));
            }
            state.ResumeTiming();

            // Time the DeAllocate calls (should be no-ops)
            for (const auto& addr : addresses)
            {
                m_allocator.DeAllocate(addr);
            }

            state.PauseTiming();
            m_allocator.GarbageCollectForce();
            state.ResumeTiming();
        }

        state.SetItemsProcessed(state.iterations() * numAllocations);
    }

    //
    // Comparison: FreeListAllocator vs LinearAllocator
    //
    // This helps quantify the performance advantage of the arena approach
    //

    class FreeListAllocatorFixture : public ::benchmark::Fixture
    {
    public:
        void SetUp(const ::benchmark::State& state) override
        {
            AZ::RHI::FreeListAllocator::Descriptor desc;
            desc.m_capacityInBytes = static_cast<size_t>(state.range(0));
            desc.m_addressBase = AZ::RHI::VirtualAddress::CreateFromOffset(0);
            desc.m_alignmentInBytes = DefaultAlignment;
            desc.m_garbageCollectLatency = 0;
            desc.m_policy = AZ::RHI::FreeListAllocatorPolicy::FirstFit;
            m_allocator.Init(desc);
        }

        void TearDown([[maybe_unused]] const ::benchmark::State& state) override
        {
            m_allocator.Shutdown();
        }

    protected:
        AZ::RHI::FreeListAllocator m_allocator;
    };

    // Benchmark: FreeListAllocator sequential allocations (for comparison)
    BENCHMARK_DEFINE_F(FreeListAllocatorFixture, BM_SequentialSmallAllocations)(benchmark::State& state)
    {
        const size_t allocationSize = SmallAllocationSize;
        const size_t capacity = static_cast<size_t>(state.range(0));
        const size_t maxAllocations = capacity / (allocationSize + DefaultAlignment);

        AZStd::vector<AZ::RHI::VirtualAddress> addresses;
        addresses.reserve(maxAllocations);

        for (auto _ : state)
        {
            addresses.clear();

            // Allocate until full
            for (size_t i = 0; i < maxAllocations; ++i)
            {
                AZ::RHI::VirtualAddress addr = m_allocator.Allocate(allocationSize, DefaultAlignment);
                if (addr.IsNull())
                {
                    break;
                }
                addresses.push_back(addr);
                benchmark::DoNotOptimize(addr);
            }

            state.PauseTiming();
            // Must deallocate individually for FreeListAllocator
            for (const auto& addr : addresses)
            {
                m_allocator.DeAllocate(addr);
            }
            m_allocator.GarbageCollect();
            state.ResumeTiming();

            state.counters[s_counterAllocationCount] = benchmark::Counter(
                static_cast<double>(addresses.size()), benchmark::Counter::kDefaults);
        }

        state.SetItemsProcessed(state.iterations() * maxAllocations);
    }

    // Benchmark: FreeListAllocator with allocation and deallocation interleaved
    BENCHMARK_DEFINE_F(FreeListAllocatorFixture, BM_AllocateAndDeallocate)(benchmark::State& state)
    {
        const size_t allocationSize = SmallAllocationSize;
        const size_t numOperations = 1000;

        AZStd::vector<AZ::RHI::VirtualAddress> addresses;
        addresses.reserve(numOperations);

        AZ::SimpleLcgRandom random(42);

        for (auto _ : state)
        {
            addresses.clear();

            for (size_t i = 0; i < numOperations; ++i)
            {
                // 60% allocate, 40% deallocate (if possible)
                if (addresses.empty() || (random.GetRandom() % 100) < 60)
                {
                    AZ::RHI::VirtualAddress addr = m_allocator.Allocate(allocationSize, DefaultAlignment);
                    if (addr.IsValid())
                    {
                        addresses.push_back(addr);
                    }
                }
                else
                {
                    size_t idx = random.GetRandom() % addresses.size();
                    m_allocator.DeAllocate(addresses[idx]);
                    addresses.erase(addresses.begin() + idx);
                }
            }

            state.PauseTiming();
            for (const auto& addr : addresses)
            {
                m_allocator.DeAllocate(addr);
            }
            m_allocator.GarbageCollect();
            state.ResumeTiming();
        }

        state.SetItemsProcessed(state.iterations() * numOperations);
    }

    //
    // Register benchmarks with various capacity sizes
    //

    // LinearAllocator benchmarks
    BENCHMARK_REGISTER_F(LinearAllocatorFixture, BM_SequentialSmallAllocations)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(64 * 1024)        // 64 KB
        ->Arg(256 * 1024)       // 256 KB
        ->Arg(1024 * 1024)      // 1 MB
        ->Arg(16 * 1024 * 1024); // 16 MB

    BENCHMARK_REGISTER_F(LinearAllocatorFixture, BM_MixedSizeAllocations)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1024 * 1024)      // 1 MB
        ->Arg(16 * 1024 * 1024); // 16 MB

    BENCHMARK_REGISTER_F(LinearAllocatorFixture, BM_FrameSimulation)
        ->Unit(benchmark::kMicrosecond)
        ->Args({1024 * 1024, 100})   // 1 MB, 100 allocations/frame
        ->Args({1024 * 1024, 500})   // 1 MB, 500 allocations/frame
        ->Args({1024 * 1024, 1000})  // 1 MB, 1000 allocations/frame
        ->Args({16 * 1024 * 1024, 1000}); // 16 MB, 1000 allocations/frame

    BENCHMARK_REGISTER_F(LinearAllocatorFixture, BM_VariableAlignment)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1024 * 1024);     // 1 MB

    BENCHMARK_REGISTER_F(LinearAllocatorFixture, BM_DeAllocateNoOp)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1024 * 1024);     // 1 MB

    // FreeListAllocator comparison benchmarks
    BENCHMARK_REGISTER_F(FreeListAllocatorFixture, BM_SequentialSmallAllocations)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(64 * 1024)        // 64 KB
        ->Arg(256 * 1024)       // 256 KB
        ->Arg(1024 * 1024);     // 1 MB

    BENCHMARK_REGISTER_F(FreeListAllocatorFixture, BM_AllocateAndDeallocate)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1024 * 1024);     // 1 MB

} // namespace Benchmark

#endif // HAVE_BENCHMARK
