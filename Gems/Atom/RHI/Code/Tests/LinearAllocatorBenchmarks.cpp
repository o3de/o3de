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
#include <AzCore/Memory/AllocatorInstance.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

#include <benchmark/benchmark.h>
#include <cstdlib>
#include <vector>

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
    // Comparison: SystemAllocator (O3DE's main heap allocator)
    //
    // This compares the arena approach against a full-featured allocator
    // using the same "allocate many, free all" pattern
    //

    class SystemAllocatorFixture : public ::benchmark::Fixture
    {
    public:
        void SetUp(const ::benchmark::State& state) override
        {
            m_numAllocations = static_cast<size_t>(state.range(0));
            m_pointers.reserve(m_numAllocations);
        }

        void TearDown([[maybe_unused]] const ::benchmark::State& state) override
        {
            // Ensure all allocations are freed
            for (void* ptr : m_pointers)
            {
                if (ptr)
                {
                    AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(ptr, 0, 0);
                }
            }
            m_pointers.clear();
        }

    protected:
        size_t m_numAllocations = 0;
        AZStd::vector<void*> m_pointers;
    };

    // Benchmark: SystemAllocator - allocate many small objects, then free all
    BENCHMARK_DEFINE_F(SystemAllocatorFixture, BM_AllocateThenFreeAll_Small)(benchmark::State& state)
    {
        const size_t allocationSize = SmallAllocationSize;
        const size_t numAllocations = m_numAllocations;
        auto& allocator = AZ::AllocatorInstance<AZ::SystemAllocator>::Get();

        for (auto _ : state)
        {
            m_pointers.clear();

            // Allocate phase
            for (size_t i = 0; i < numAllocations; ++i)
            {
                void* ptr = allocator.allocate(allocationSize, DefaultAlignment);
                m_pointers.push_back(ptr);
                benchmark::DoNotOptimize(ptr);
            }

            // Free all phase
            for (void* ptr : m_pointers)
            {
                allocator.deallocate(ptr, allocationSize, DefaultAlignment);
            }
            m_pointers.clear();
        }

        state.SetItemsProcessed(state.iterations() * numAllocations);
        state.SetBytesProcessed(state.iterations() * numAllocations * allocationSize);
    }

    // Benchmark: SystemAllocator - allocate many medium objects, then free all
    BENCHMARK_DEFINE_F(SystemAllocatorFixture, BM_AllocateThenFreeAll_Medium)(benchmark::State& state)
    {
        const size_t allocationSize = MediumAllocationSize;
        const size_t numAllocations = m_numAllocations;
        auto& allocator = AZ::AllocatorInstance<AZ::SystemAllocator>::Get();

        for (auto _ : state)
        {
            m_pointers.clear();

            // Allocate phase
            for (size_t i = 0; i < numAllocations; ++i)
            {
                void* ptr = allocator.allocate(allocationSize, DefaultAlignment);
                m_pointers.push_back(ptr);
                benchmark::DoNotOptimize(ptr);
            }

            // Free all phase
            for (void* ptr : m_pointers)
            {
                allocator.deallocate(ptr, allocationSize, DefaultAlignment);
            }
            m_pointers.clear();
        }

        state.SetItemsProcessed(state.iterations() * numAllocations);
        state.SetBytesProcessed(state.iterations() * numAllocations * allocationSize);
    }

    // Benchmark: SystemAllocator - mixed sizes, then free all
    BENCHMARK_DEFINE_F(SystemAllocatorFixture, BM_AllocateThenFreeAll_Mixed)(benchmark::State& state)
    {
        const size_t numAllocations = m_numAllocations;
        auto& allocator = AZ::AllocatorInstance<AZ::SystemAllocator>::Get();

        // Pre-generate allocation sizes
        AZ::SimpleLcgRandom random(42);
        AZStd::vector<size_t> sizes;
        sizes.reserve(numAllocations);
        for (size_t i = 0; i < numAllocations; ++i)
        {
            size_t sizeClass = random.GetRandom() % 3;
            switch (sizeClass)
            {
            case 0: sizes.push_back(SmallAllocationSize); break;
            case 1: sizes.push_back(MediumAllocationSize); break;
            case 2: sizes.push_back(LargeAllocationSize); break;
            }
        }

        for (auto _ : state)
        {
            m_pointers.clear();
            size_t totalBytes = 0;

            // Allocate phase
            for (size_t i = 0; i < numAllocations; ++i)
            {
                void* ptr = allocator.allocate(sizes[i], DefaultAlignment);
                m_pointers.push_back(ptr);
                totalBytes += sizes[i];
                benchmark::DoNotOptimize(ptr);
            }

            // Free all phase
            for (size_t i = 0; i < numAllocations; ++i)
            {
                allocator.deallocate(m_pointers[i], sizes[i], DefaultAlignment);
            }
            m_pointers.clear();

            state.counters[s_counterTotalAllocated] = benchmark::Counter(
                static_cast<double>(totalBytes), benchmark::Counter::kDefaults);
        }

        state.SetItemsProcessed(state.iterations() * numAllocations);
    }

    //
    // Comparison: Raw malloc/free (standard C allocator)
    //
    // Baseline comparison against the system's default allocator
    //

    class MallocFixture : public ::benchmark::Fixture
    {
    public:
        void SetUp(const ::benchmark::State& state) override
        {
            m_numAllocations = static_cast<size_t>(state.range(0));
            m_pointers.reserve(m_numAllocations);
        }

        void TearDown([[maybe_unused]] const ::benchmark::State& state) override
        {
            for (void* ptr : m_pointers)
            {
                if (ptr)
                {
                    free(ptr);
                }
            }
            m_pointers.clear();
        }

    protected:
        size_t m_numAllocations = 0;
        std::vector<void*> m_pointers;  // Use std::vector to avoid AZ allocator
    };

    // Benchmark: malloc - allocate many small objects, then free all
    BENCHMARK_DEFINE_F(MallocFixture, BM_AllocateThenFreeAll_Small)(benchmark::State& state)
    {
        const size_t allocationSize = SmallAllocationSize;
        const size_t numAllocations = m_numAllocations;

        for (auto _ : state)
        {
            m_pointers.clear();

            // Allocate phase
            for (size_t i = 0; i < numAllocations; ++i)
            {
                void* ptr = malloc(allocationSize);
                m_pointers.push_back(ptr);
                benchmark::DoNotOptimize(ptr);
            }

            // Free all phase
            for (void* ptr : m_pointers)
            {
                free(ptr);
            }
            m_pointers.clear();
        }

        state.SetItemsProcessed(state.iterations() * numAllocations);
        state.SetBytesProcessed(state.iterations() * numAllocations * allocationSize);
    }

    // Benchmark: malloc - allocate many medium objects, then free all
    BENCHMARK_DEFINE_F(MallocFixture, BM_AllocateThenFreeAll_Medium)(benchmark::State& state)
    {
        const size_t allocationSize = MediumAllocationSize;
        const size_t numAllocations = m_numAllocations;

        for (auto _ : state)
        {
            m_pointers.clear();

            // Allocate phase
            for (size_t i = 0; i < numAllocations; ++i)
            {
                void* ptr = malloc(allocationSize);
                m_pointers.push_back(ptr);
                benchmark::DoNotOptimize(ptr);
            }

            // Free all phase
            for (void* ptr : m_pointers)
            {
                free(ptr);
            }
            m_pointers.clear();
        }

        state.SetItemsProcessed(state.iterations() * numAllocations);
        state.SetBytesProcessed(state.iterations() * numAllocations * allocationSize);
    }

    // Benchmark: malloc - mixed sizes, then free all
    BENCHMARK_DEFINE_F(MallocFixture, BM_AllocateThenFreeAll_Mixed)(benchmark::State& state)
    {
        const size_t numAllocations = m_numAllocations;

        // Pre-generate allocation sizes (use std random to avoid AZ allocator)
        std::vector<size_t> sizes;
        sizes.reserve(numAllocations);
        unsigned int seed = 42;
        for (size_t i = 0; i < numAllocations; ++i)
        {
            seed = seed * 1103515245 + 12345;  // Simple LCG
            size_t sizeClass = (seed >> 16) % 3;
            switch (sizeClass)
            {
            case 0: sizes.push_back(SmallAllocationSize); break;
            case 1: sizes.push_back(MediumAllocationSize); break;
            case 2: sizes.push_back(LargeAllocationSize); break;
            }
        }

        for (auto _ : state)
        {
            m_pointers.clear();
            size_t totalBytes = 0;

            // Allocate phase
            for (size_t i = 0; i < numAllocations; ++i)
            {
                void* ptr = malloc(sizes[i]);
                m_pointers.push_back(ptr);
                totalBytes += sizes[i];
                benchmark::DoNotOptimize(ptr);
            }

            // Free all phase
            for (void* ptr : m_pointers)
            {
                free(ptr);
            }
            m_pointers.clear();

            state.counters[s_counterTotalAllocated] = benchmark::Counter(
                static_cast<double>(totalBytes), benchmark::Counter::kDefaults);
        }

        state.SetItemsProcessed(state.iterations() * numAllocations);
    }

    //
    // Direct comparison benchmark: Same workload across all allocators
    //
    // This benchmark runs the exact same allocation pattern across:
    // - LinearAllocator (arena)
    // - SystemAllocator (O3DE heap)
    // - malloc/free (system heap)
    //

    // LinearAllocator version of the canonical benchmark
    BENCHMARK_DEFINE_F(LinearAllocatorFixture, BM_CanonicalWorkload)(benchmark::State& state)
    {
        const size_t numAllocations = 1000;
        const size_t sizes[] = { 32, 64, 128, 256, 512, 1024, 2048, 4096 };
        const size_t numSizes = sizeof(sizes) / sizeof(sizes[0]);

        for (auto _ : state)
        {
            for (size_t i = 0; i < numAllocations; ++i)
            {
                size_t size = sizes[i % numSizes];
                AZ::RHI::VirtualAddress addr = m_allocator.Allocate(size, 16);
                benchmark::DoNotOptimize(addr);
            }

            // Reset arena
            m_allocator.GarbageCollectForce();
        }

        state.SetItemsProcessed(state.iterations() * numAllocations);
    }

    // SystemAllocator version of the canonical benchmark
    BENCHMARK_DEFINE_F(SystemAllocatorFixture, BM_CanonicalWorkload)(benchmark::State& state)
    {
        const size_t numAllocations = 1000;
        const size_t sizes[] = { 32, 64, 128, 256, 512, 1024, 2048, 4096 };
        const size_t numSizes = sizeof(sizes) / sizeof(sizes[0]);
        auto& allocator = AZ::AllocatorInstance<AZ::SystemAllocator>::Get();

        for (auto _ : state)
        {
            m_pointers.clear();

            for (size_t i = 0; i < numAllocations; ++i)
            {
                size_t size = sizes[i % numSizes];
                void* ptr = allocator.allocate(size, 16);
                m_pointers.push_back(ptr);
                benchmark::DoNotOptimize(ptr);
            }

            // Free all
            for (size_t i = 0; i < numAllocations; ++i)
            {
                size_t size = sizes[i % numSizes];
                allocator.deallocate(m_pointers[i], size, 16);
            }
            m_pointers.clear();
        }

        state.SetItemsProcessed(state.iterations() * numAllocations);
    }

    // malloc version of the canonical benchmark
    BENCHMARK_DEFINE_F(MallocFixture, BM_CanonicalWorkload)(benchmark::State& state)
    {
        const size_t numAllocations = 1000;
        const size_t sizes[] = { 32, 64, 128, 256, 512, 1024, 2048, 4096 };
        const size_t numSizes = sizeof(sizes) / sizeof(sizes[0]);

        for (auto _ : state)
        {
            m_pointers.clear();

            for (size_t i = 0; i < numAllocations; ++i)
            {
                size_t size = sizes[i % numSizes];
                void* ptr = malloc(size);
                m_pointers.push_back(ptr);
                benchmark::DoNotOptimize(ptr);
            }

            // Free all
            for (void* ptr : m_pointers)
            {
                free(ptr);
            }
            m_pointers.clear();
        }

        state.SetItemsProcessed(state.iterations() * numAllocations);
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

    // SystemAllocator benchmarks (allocate-then-free-all pattern)
    BENCHMARK_REGISTER_F(SystemAllocatorFixture, BM_AllocateThenFreeAll_Small)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1000)             // 1000 allocations
        ->Arg(5000)             // 5000 allocations
        ->Arg(10000);           // 10000 allocations

    BENCHMARK_REGISTER_F(SystemAllocatorFixture, BM_AllocateThenFreeAll_Medium)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1000)
        ->Arg(5000)
        ->Arg(10000);

    BENCHMARK_REGISTER_F(SystemAllocatorFixture, BM_AllocateThenFreeAll_Mixed)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1000)
        ->Arg(5000)
        ->Arg(10000);

    // malloc/free benchmarks (allocate-then-free-all pattern)
    BENCHMARK_REGISTER_F(MallocFixture, BM_AllocateThenFreeAll_Small)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1000)
        ->Arg(5000)
        ->Arg(10000);

    BENCHMARK_REGISTER_F(MallocFixture, BM_AllocateThenFreeAll_Medium)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1000)
        ->Arg(5000)
        ->Arg(10000);

    BENCHMARK_REGISTER_F(MallocFixture, BM_AllocateThenFreeAll_Mixed)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1000)
        ->Arg(5000)
        ->Arg(10000);

    // Canonical workload comparison (same pattern, all allocators)
    BENCHMARK_REGISTER_F(LinearAllocatorFixture, BM_CanonicalWorkload)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(16 * 1024 * 1024);  // 16 MB arena

    BENCHMARK_REGISTER_F(SystemAllocatorFixture, BM_CanonicalWorkload)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1000);              // Number of allocations

    BENCHMARK_REGISTER_F(MallocFixture, BM_CanonicalWorkload)
        ->Unit(benchmark::kMicrosecond)
        ->Arg(1000);              // Number of allocations

} // namespace Benchmark

#endif // HAVE_BENCHMARK
