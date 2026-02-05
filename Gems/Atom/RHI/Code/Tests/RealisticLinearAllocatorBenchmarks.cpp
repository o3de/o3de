/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <Atom/RHI/LinearAllocator.h>
#include <AzCore/Memory/AllocatorInstance.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <benchmark/benchmark.h>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace RealisticBenchmark
{
    //
    // Realistic LinearAllocator Benchmarks
    //
    // These benchmarks simulate actual Atom renderer workloads based on analysis of:
    // - DeviceDrawPacketBuilder.cpp - DrawPacket construction
    // - StagingMemoryAllocator.cpp - GPU upload buffers
    // - Device.cpp - Frame lifecycle with GarbageCollect
    //

    // Simulated structures matching real Atom renderer sizes
    struct SimulatedDrawPacket
    {
        uint64_t header[8];  // 64 bytes - matches DeviceDrawPacket header
    };

    struct SimulatedDrawItem
    {
        uint64_t data[6];  // 48 bytes - matches DeviceDrawItem
    };

    // Realistic allocation size distributions
    static constexpr size_t DrawPacketAlignment = 8;
    static constexpr size_t StagingBufferAlignment = 256;  // GPU alignment requirement

    // Staging buffer sizes based on real usage patterns
    static constexpr size_t StagingSizes[] = {
        256,      // Small constant buffers
        1024,     // Medium constant buffers
        4096,     // Vertex/index data chunks
        16384,    // Larger mesh data
        65536,    // Texture mip levels
        262144,   // Large texture uploads
    };
    static constexpr size_t NumStagingSizes = sizeof(StagingSizes) / sizeof(StagingSizes[0]);

    // Default arena capacity for realistic frame budgets
    static constexpr size_t DefaultArenaCapacity = 64 * 1024 * 1024;  // 64 MB

    //
    // LinearAllocator Fixture - Arena-style allocation
    //
    class RealisticLinearAllocatorFixture : public ::benchmark::Fixture
    {
    public:
        void SetUp([[maybe_unused]] const ::benchmark::State& state) override
        {
            // Allocate backing memory from SystemAllocator
            m_backingMemory = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().allocate(
                DefaultArenaCapacity, StagingBufferAlignment);

            AZ::RHI::LinearAllocator::Descriptor desc;
            desc.m_capacityInBytes = DefaultArenaCapacity;
            desc.m_addressBase = AZ::RHI::VirtualAddress::CreateFromPointer(m_backingMemory);
            desc.m_garbageCollectLatency = 0;
            m_allocator.Init(desc);
        }

        void TearDown([[maybe_unused]] const ::benchmark::State& state) override
        {
            m_allocator.Shutdown();
            AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(
                m_backingMemory, DefaultArenaCapacity, StagingBufferAlignment);
            m_backingMemory = nullptr;
        }

    protected:
        void* Allocate(size_t size, size_t alignment)
        {
            AZ::RHI::VirtualAddress addr = m_allocator.Allocate(size, alignment);
            return addr.IsNull() ? nullptr : reinterpret_cast<void*>(addr.m_ptr);
        }

        void GarbageCollectForce()
        {
            m_allocator.GarbageCollectForce();
        }

        AZ::RHI::LinearAllocator m_allocator;
        void* m_backingMemory = nullptr;
    };

    //
    // SystemAllocator Fixture - Tracks allocations for bulk free
    //
    class RealisticSystemAllocatorFixture : public ::benchmark::Fixture
    {
    public:
        void SetUp([[maybe_unused]] const ::benchmark::State& state) override
        {
            m_allocations.reserve(100000);
        }

        void TearDown([[maybe_unused]] const ::benchmark::State& state) override
        {
            GarbageCollectForce();
        }

    protected:
        void* Allocate(size_t size, size_t alignment)
        {
            void* ptr = AZ::AllocatorInstance<AZ::SystemAllocator>::Get().allocate(size, alignment);
            if (ptr)
            {
                m_allocations.push_back({ptr, size, alignment});
            }
            return ptr;
        }

        void GarbageCollectForce()
        {
            for (const auto& alloc : m_allocations)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(
                    alloc.ptr, alloc.size, alloc.alignment);
            }
            m_allocations.clear();
        }

    private:
        struct Allocation
        {
            void* ptr;
            size_t size;
            size_t alignment;
        };
        std::vector<Allocation> m_allocations;
    };

    //
    // malloc/free Fixture - Standard C allocation with bulk free
    //
    class RealisticMallocFixture : public ::benchmark::Fixture
    {
    public:
        void SetUp([[maybe_unused]] const ::benchmark::State& state) override
        {
            m_allocations.reserve(100000);
        }

        void TearDown([[maybe_unused]] const ::benchmark::State& state) override
        {
            GarbageCollectForce();
        }

    protected:
        void* Allocate(size_t size, size_t alignment)
        {
            void* ptr = nullptr;
#if defined(_WIN32)
            ptr = _aligned_malloc(size, alignment);
#else
            // aligned_alloc requires size to be multiple of alignment
            size_t alignedSize = (size + alignment - 1) & ~(alignment - 1);
            ptr = aligned_alloc(alignment, alignedSize);
#endif
            if (ptr)
            {
                m_allocations.push_back(ptr);
            }
            return ptr;
        }

        void GarbageCollectForce()
        {
            for (void* ptr : m_allocations)
            {
#if defined(_WIN32)
                _aligned_free(ptr);
#else
                free(ptr);
#endif
            }
            m_allocations.clear();
        }

    private:
        std::vector<void*> m_allocations;
    };

    //
    // Benchmark 1: DrawPacket Simulation
    //
    // Simulates DeviceDrawPacketBuilder::End() allocation pattern.
    // Each DrawPacket is a contiguous allocation containing:
    // - Packet header (64 bytes)
    // - DrawItem array (48 bytes each)
    // - Sort keys array (8 bytes each)
    // - Filter masks array (4 bytes each)
    // - SRG pointer arrays (8 bytes each, typically 2 SRGs per item)
    //

    BENCHMARK_DEFINE_F(RealisticLinearAllocatorFixture, BM_DrawPacketSimulation)(benchmark::State& state)
    {
        const size_t packetsPerFrame = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            size_t totalBytes = 0;

            for (size_t i = 0; i < packetsPerFrame; ++i)
            {
                // Vary draw items per packet (1-8, typical batch sizes)
                size_t drawItemCount = 1 + (i % 8);

                // Calculate total size matching DeviceDrawPacketBuilder::End()
                size_t packetSize = sizeof(SimulatedDrawPacket);                    // 64 bytes
                size_t drawItemsSize = drawItemCount * sizeof(SimulatedDrawItem);   // 48 * N
                size_t sortKeysSize = drawItemCount * sizeof(uint64_t);             // 8 * N
                size_t filterMaskSize = drawItemCount * sizeof(uint32_t);           // 4 * N
                size_t srgPointersSize = drawItemCount * 2 * sizeof(void*);         // 16 * N (2 SRGs typical)

                size_t totalSize = packetSize + drawItemsSize + sortKeysSize +
                                   filterMaskSize + srgPointersSize;

                void* memory = Allocate(totalSize, DrawPacketAlignment);
                if (memory)
                {
                    // Touch memory to simulate initialization (realistic cache behavior)
                    memset(memory, 0, totalSize);
                    totalBytes += totalSize;
                }
                benchmark::DoNotOptimize(memory);
            }

            GarbageCollectForce();

            state.counters["BytesPerFrame"] = benchmark::Counter(
                static_cast<double>(totalBytes), benchmark::Counter::kDefaults);
        }

        state.SetItemsProcessed(state.iterations() * packetsPerFrame);
    }

    BENCHMARK_DEFINE_F(RealisticSystemAllocatorFixture, BM_DrawPacketSimulation)(benchmark::State& state)
    {
        const size_t packetsPerFrame = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            size_t totalBytes = 0;

            for (size_t i = 0; i < packetsPerFrame; ++i)
            {
                size_t drawItemCount = 1 + (i % 8);

                size_t packetSize = sizeof(SimulatedDrawPacket);
                size_t drawItemsSize = drawItemCount * sizeof(SimulatedDrawItem);
                size_t sortKeysSize = drawItemCount * sizeof(uint64_t);
                size_t filterMaskSize = drawItemCount * sizeof(uint32_t);
                size_t srgPointersSize = drawItemCount * 2 * sizeof(void*);

                size_t totalSize = packetSize + drawItemsSize + sortKeysSize +
                                   filterMaskSize + srgPointersSize;

                void* memory = Allocate(totalSize, DrawPacketAlignment);
                if (memory)
                {
                    memset(memory, 0, totalSize);
                    totalBytes += totalSize;
                }
                benchmark::DoNotOptimize(memory);
            }

            GarbageCollectForce();

            state.counters["BytesPerFrame"] = benchmark::Counter(
                static_cast<double>(totalBytes), benchmark::Counter::kDefaults);
        }

        state.SetItemsProcessed(state.iterations() * packetsPerFrame);
    }

    BENCHMARK_DEFINE_F(RealisticMallocFixture, BM_DrawPacketSimulation)(benchmark::State& state)
    {
        const size_t packetsPerFrame = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            size_t totalBytes = 0;

            for (size_t i = 0; i < packetsPerFrame; ++i)
            {
                size_t drawItemCount = 1 + (i % 8);

                size_t packetSize = sizeof(SimulatedDrawPacket);
                size_t drawItemsSize = drawItemCount * sizeof(SimulatedDrawItem);
                size_t sortKeysSize = drawItemCount * sizeof(uint64_t);
                size_t filterMaskSize = drawItemCount * sizeof(uint32_t);
                size_t srgPointersSize = drawItemCount * 2 * sizeof(void*);

                size_t totalSize = packetSize + drawItemsSize + sortKeysSize +
                                   filterMaskSize + srgPointersSize;

                void* memory = Allocate(totalSize, DrawPacketAlignment);
                if (memory)
                {
                    memset(memory, 0, totalSize);
                    totalBytes += totalSize;
                }
                benchmark::DoNotOptimize(memory);
            }

            GarbageCollectForce();

            state.counters["BytesPerFrame"] = benchmark::Counter(
                static_cast<double>(totalBytes), benchmark::Counter::kDefaults);
        }

        state.SetItemsProcessed(state.iterations() * packetsPerFrame);
    }

    // Register DrawPacket benchmarks with realistic packet counts
    BENCHMARK_REGISTER_F(RealisticLinearAllocatorFixture, BM_DrawPacketSimulation)
        ->Arg(100)->Arg(500)->Arg(1000)->Arg(2000)->Arg(5000)
        ->Unit(benchmark::kMicrosecond);

    BENCHMARK_REGISTER_F(RealisticSystemAllocatorFixture, BM_DrawPacketSimulation)
        ->Arg(100)->Arg(500)->Arg(1000)->Arg(2000)->Arg(5000)
        ->Unit(benchmark::kMicrosecond);

    BENCHMARK_REGISTER_F(RealisticMallocFixture, BM_DrawPacketSimulation)
        ->Arg(100)->Arg(500)->Arg(1000)->Arg(2000)->Arg(5000)
        ->Unit(benchmark::kMicrosecond);

    //
    // Benchmark 2: Staging Buffer Simulation
    //
    // Simulates StagingMemoryAllocator patterns for GPU upload buffers.
    // Uses GPU-aligned allocations (256 bytes) with realistic size distribution.
    //

    BENCHMARK_DEFINE_F(RealisticLinearAllocatorFixture, BM_StagingBufferSimulation)(benchmark::State& state)
    {
        const size_t allocationsPerFrame = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            size_t totalBytes = 0;

            for (size_t i = 0; i < allocationsPerFrame; ++i)
            {
                size_t size = StagingSizes[i % NumStagingSizes];
                void* memory = Allocate(size, StagingBufferAlignment);

                if (memory)
                {
                    // Simulate filling buffer with upload data
                    memset(memory, 0xAB, size);
                    totalBytes += size;
                }
                benchmark::DoNotOptimize(memory);
            }

            GarbageCollectForce();

            state.counters["BytesPerFrame"] = benchmark::Counter(
                static_cast<double>(totalBytes), benchmark::Counter::kDefaults);
        }

        state.SetItemsProcessed(state.iterations() * allocationsPerFrame);
        state.SetBytesProcessed(state.iterations() * allocationsPerFrame *
            (StagingSizes[0] + StagingSizes[1] + StagingSizes[2] +
             StagingSizes[3] + StagingSizes[4] + StagingSizes[5]) / NumStagingSizes);
    }

    BENCHMARK_DEFINE_F(RealisticSystemAllocatorFixture, BM_StagingBufferSimulation)(benchmark::State& state)
    {
        const size_t allocationsPerFrame = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            size_t totalBytes = 0;

            for (size_t i = 0; i < allocationsPerFrame; ++i)
            {
                size_t size = StagingSizes[i % NumStagingSizes];
                void* memory = Allocate(size, StagingBufferAlignment);

                if (memory)
                {
                    memset(memory, 0xAB, size);
                    totalBytes += size;
                }
                benchmark::DoNotOptimize(memory);
            }

            GarbageCollectForce();

            state.counters["BytesPerFrame"] = benchmark::Counter(
                static_cast<double>(totalBytes), benchmark::Counter::kDefaults);
        }

        state.SetItemsProcessed(state.iterations() * allocationsPerFrame);
    }

    BENCHMARK_DEFINE_F(RealisticMallocFixture, BM_StagingBufferSimulation)(benchmark::State& state)
    {
        const size_t allocationsPerFrame = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            size_t totalBytes = 0;

            for (size_t i = 0; i < allocationsPerFrame; ++i)
            {
                size_t size = StagingSizes[i % NumStagingSizes];
                void* memory = Allocate(size, StagingBufferAlignment);

                if (memory)
                {
                    memset(memory, 0xAB, size);
                    totalBytes += size;
                }
                benchmark::DoNotOptimize(memory);
            }

            GarbageCollectForce();

            state.counters["BytesPerFrame"] = benchmark::Counter(
                static_cast<double>(totalBytes), benchmark::Counter::kDefaults);
        }

        state.SetItemsProcessed(state.iterations() * allocationsPerFrame);
    }

    // Register Staging Buffer benchmarks
    BENCHMARK_REGISTER_F(RealisticLinearAllocatorFixture, BM_StagingBufferSimulation)
        ->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)
        ->Unit(benchmark::kMicrosecond);

    BENCHMARK_REGISTER_F(RealisticSystemAllocatorFixture, BM_StagingBufferSimulation)
        ->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)
        ->Unit(benchmark::kMicrosecond);

    BENCHMARK_REGISTER_F(RealisticMallocFixture, BM_StagingBufferSimulation)
        ->Arg(50)->Arg(100)->Arg(200)->Arg(500)->Arg(1000)
        ->Unit(benchmark::kMicrosecond);

    //
    // Benchmark 3: Mixed Frame Workload
    //
    // Combines DrawPacket + Staging + Miscellaneous allocations to simulate
    // a complete rendering frame. Scale factor controls scene complexity.
    //

    BENCHMARK_DEFINE_F(RealisticLinearAllocatorFixture, BM_MixedFrameWorkload)(benchmark::State& state)
    {
        const size_t sceneComplexity = static_cast<size_t>(state.range(0));

        const size_t drawPackets = 100 * sceneComplexity;
        const size_t stagingBuffers = 20 * sceneComplexity;
        const size_t miscAllocations = 50 * sceneComplexity;

        for (auto _ : state)
        {
            size_t totalAllocations = 0;

            // Phase 1: DrawPacket construction (render graph compilation)
            for (size_t i = 0; i < drawPackets; ++i)
            {
                size_t size = 64 + (i % 8) * 56;  // 64-504 bytes
                void* p = Allocate(size, DrawPacketAlignment);
                if (p)
                {
                    memset(p, 0, size);
                    ++totalAllocations;
                }
                benchmark::DoNotOptimize(p);
            }

            // Phase 2: Staging buffer allocation (resource uploads)
            for (size_t i = 0; i < stagingBuffers; ++i)
            {
                size_t size = 256 << (i % 6);  // 256 bytes to 8KB
                void* p = Allocate(size, StagingBufferAlignment);
                if (p)
                {
                    memset(p, 0xCD, size);
                    ++totalAllocations;
                }
                benchmark::DoNotOptimize(p);
            }

            // Phase 3: Miscellaneous frame allocations (transient data)
            for (size_t i = 0; i < miscAllocations; ++i)
            {
                size_t size = 32 + (i % 32) * 16;  // 32-528 bytes
                void* p = Allocate(size, 8);
                if (p)
                {
                    ++totalAllocations;
                }
                benchmark::DoNotOptimize(p);
            }

            GarbageCollectForce();

            state.counters["Allocations"] = benchmark::Counter(
                static_cast<double>(totalAllocations), benchmark::Counter::kDefaults);
        }

        size_t expectedAllocations = drawPackets + stagingBuffers + miscAllocations;
        state.SetItemsProcessed(state.iterations() * expectedAllocations);
    }

    BENCHMARK_DEFINE_F(RealisticSystemAllocatorFixture, BM_MixedFrameWorkload)(benchmark::State& state)
    {
        const size_t sceneComplexity = static_cast<size_t>(state.range(0));

        const size_t drawPackets = 100 * sceneComplexity;
        const size_t stagingBuffers = 20 * sceneComplexity;
        const size_t miscAllocations = 50 * sceneComplexity;

        for (auto _ : state)
        {
            size_t totalAllocations = 0;

            for (size_t i = 0; i < drawPackets; ++i)
            {
                size_t size = 64 + (i % 8) * 56;
                void* p = Allocate(size, DrawPacketAlignment);
                if (p)
                {
                    memset(p, 0, size);
                    ++totalAllocations;
                }
                benchmark::DoNotOptimize(p);
            }

            for (size_t i = 0; i < stagingBuffers; ++i)
            {
                size_t size = 256 << (i % 6);
                void* p = Allocate(size, StagingBufferAlignment);
                if (p)
                {
                    memset(p, 0xCD, size);
                    ++totalAllocations;
                }
                benchmark::DoNotOptimize(p);
            }

            for (size_t i = 0; i < miscAllocations; ++i)
            {
                size_t size = 32 + (i % 32) * 16;
                void* p = Allocate(size, 8);
                if (p)
                {
                    ++totalAllocations;
                }
                benchmark::DoNotOptimize(p);
            }

            GarbageCollectForce();

            state.counters["Allocations"] = benchmark::Counter(
                static_cast<double>(totalAllocations), benchmark::Counter::kDefaults);
        }

        size_t expectedAllocations = drawPackets + stagingBuffers + miscAllocations;
        state.SetItemsProcessed(state.iterations() * expectedAllocations);
    }

    BENCHMARK_DEFINE_F(RealisticMallocFixture, BM_MixedFrameWorkload)(benchmark::State& state)
    {
        const size_t sceneComplexity = static_cast<size_t>(state.range(0));

        const size_t drawPackets = 100 * sceneComplexity;
        const size_t stagingBuffers = 20 * sceneComplexity;
        const size_t miscAllocations = 50 * sceneComplexity;

        for (auto _ : state)
        {
            size_t totalAllocations = 0;

            for (size_t i = 0; i < drawPackets; ++i)
            {
                size_t size = 64 + (i % 8) * 56;
                void* p = Allocate(size, DrawPacketAlignment);
                if (p)
                {
                    memset(p, 0, size);
                    ++totalAllocations;
                }
                benchmark::DoNotOptimize(p);
            }

            for (size_t i = 0; i < stagingBuffers; ++i)
            {
                size_t size = 256 << (i % 6);
                void* p = Allocate(size, StagingBufferAlignment);
                if (p)
                {
                    memset(p, 0xCD, size);
                    ++totalAllocations;
                }
                benchmark::DoNotOptimize(p);
            }

            for (size_t i = 0; i < miscAllocations; ++i)
            {
                size_t size = 32 + (i % 32) * 16;
                void* p = Allocate(size, 8);
                if (p)
                {
                    ++totalAllocations;
                }
                benchmark::DoNotOptimize(p);
            }

            GarbageCollectForce();

            state.counters["Allocations"] = benchmark::Counter(
                static_cast<double>(totalAllocations), benchmark::Counter::kDefaults);
        }

        size_t expectedAllocations = drawPackets + stagingBuffers + miscAllocations;
        state.SetItemsProcessed(state.iterations() * expectedAllocations);
    }

    // Register Mixed Frame benchmarks (1 = simple scene, 10 = complex scene)
    BENCHMARK_REGISTER_F(RealisticLinearAllocatorFixture, BM_MixedFrameWorkload)
        ->DenseRange(1, 10)
        ->Unit(benchmark::kMicrosecond);

    BENCHMARK_REGISTER_F(RealisticSystemAllocatorFixture, BM_MixedFrameWorkload)
        ->DenseRange(1, 10)
        ->Unit(benchmark::kMicrosecond);

    BENCHMARK_REGISTER_F(RealisticMallocFixture, BM_MixedFrameWorkload)
        ->DenseRange(1, 10)
        ->Unit(benchmark::kMicrosecond);

    //
    // Benchmark 4: Sustained Multi-Frame Load
    //
    // Tests allocator behavior over multiple frames to capture cache effects,
    // fragmentation, and sustained throughput (simulates 1 second at 60 FPS).
    //

    BENCHMARK_DEFINE_F(RealisticLinearAllocatorFixture, BM_SustainedFrameLoad)(benchmark::State& state)
    {
        const size_t framesPerIteration = 60;  // 1 second at 60 FPS
        const size_t allocationsPerFrame = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            for (size_t frame = 0; frame < framesPerIteration; ++frame)
            {
                // Vary allocation pattern slightly per frame (simulates different draw counts)
                for (size_t i = 0; i < allocationsPerFrame; ++i)
                {
                    size_t size = 64 + ((frame + i) % 64) * 32;  // 64-2080 bytes
                    void* p = Allocate(size, DrawPacketAlignment);
                    benchmark::DoNotOptimize(p);
                }

                GarbageCollectForce();
            }
        }

        state.SetItemsProcessed(state.iterations() * framesPerIteration * allocationsPerFrame);
    }

    BENCHMARK_DEFINE_F(RealisticSystemAllocatorFixture, BM_SustainedFrameLoad)(benchmark::State& state)
    {
        const size_t framesPerIteration = 60;
        const size_t allocationsPerFrame = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            for (size_t frame = 0; frame < framesPerIteration; ++frame)
            {
                for (size_t i = 0; i < allocationsPerFrame; ++i)
                {
                    size_t size = 64 + ((frame + i) % 64) * 32;
                    void* p = Allocate(size, DrawPacketAlignment);
                    benchmark::DoNotOptimize(p);
                }

                GarbageCollectForce();
            }
        }

        state.SetItemsProcessed(state.iterations() * framesPerIteration * allocationsPerFrame);
    }

    BENCHMARK_DEFINE_F(RealisticMallocFixture, BM_SustainedFrameLoad)(benchmark::State& state)
    {
        const size_t framesPerIteration = 60;
        const size_t allocationsPerFrame = static_cast<size_t>(state.range(0));

        for (auto _ : state)
        {
            for (size_t frame = 0; frame < framesPerIteration; ++frame)
            {
                for (size_t i = 0; i < allocationsPerFrame; ++i)
                {
                    size_t size = 64 + ((frame + i) % 64) * 32;
                    void* p = Allocate(size, DrawPacketAlignment);
                    benchmark::DoNotOptimize(p);
                }

                GarbageCollectForce();
            }
        }

        state.SetItemsProcessed(state.iterations() * framesPerIteration * allocationsPerFrame);
    }

    // Register Sustained Frame benchmarks
    BENCHMARK_REGISTER_F(RealisticLinearAllocatorFixture, BM_SustainedFrameLoad)
        ->Arg(100)->Arg(500)->Arg(1000)->Arg(2000)
        ->Unit(benchmark::kMillisecond);

    BENCHMARK_REGISTER_F(RealisticSystemAllocatorFixture, BM_SustainedFrameLoad)
        ->Arg(100)->Arg(500)->Arg(1000)->Arg(2000)
        ->Unit(benchmark::kMillisecond);

    BENCHMARK_REGISTER_F(RealisticMallocFixture, BM_SustainedFrameLoad)
        ->Arg(100)->Arg(500)->Arg(1000)->Arg(2000)
        ->Unit(benchmark::kMillisecond);

    //
    // Benchmark 5: DrawPacket with Memory Access Pattern
    //
    // Simulates not just allocation but also the memory access pattern that
    // follows: sequential writes during construction, then sequential reads
    // during rendering. This tests cache locality benefits of contiguous allocation.
    //

    BENCHMARK_DEFINE_F(RealisticLinearAllocatorFixture, BM_DrawPacketWithAccess)(benchmark::State& state)
    {
        const size_t packetsPerFrame = static_cast<size_t>(state.range(0));
        std::vector<void*> packets;
        std::vector<size_t> sizes;
        packets.reserve(packetsPerFrame);
        sizes.reserve(packetsPerFrame);

        for (auto _ : state)
        {
            packets.clear();
            sizes.clear();

            // Phase 1: Allocate and write (construction)
            for (size_t i = 0; i < packetsPerFrame; ++i)
            {
                size_t drawItemCount = 1 + (i % 8);
                size_t totalSize = sizeof(SimulatedDrawPacket) +
                                   drawItemCount * sizeof(SimulatedDrawItem) +
                                   drawItemCount * sizeof(uint64_t) +
                                   drawItemCount * sizeof(uint32_t) +
                                   drawItemCount * 2 * sizeof(void*);

                void* memory = Allocate(totalSize, DrawPacketAlignment);
                if (memory)
                {
                    // Write phase: initialize packet data
                    memset(memory, static_cast<int>(i & 0xFF), totalSize);
                    packets.push_back(memory);
                    sizes.push_back(totalSize);
                }
            }

            // Phase 2: Read (simulates rendering traversal)
            uint64_t checksum = 0;
            for (size_t i = 0; i < packets.size(); ++i)
            {
                const uint8_t* data = static_cast<const uint8_t*>(packets[i]);
                size_t size = sizes[i];

                // Read every cache line (64 bytes) to simulate access pattern
                for (size_t offset = 0; offset < size; offset += 64)
                {
                    checksum += data[offset];
                }
            }
            benchmark::DoNotOptimize(checksum);

            GarbageCollectForce();
        }

        state.SetItemsProcessed(state.iterations() * packetsPerFrame);
    }

    BENCHMARK_DEFINE_F(RealisticSystemAllocatorFixture, BM_DrawPacketWithAccess)(benchmark::State& state)
    {
        const size_t packetsPerFrame = static_cast<size_t>(state.range(0));
        std::vector<void*> packets;
        std::vector<size_t> sizes;
        packets.reserve(packetsPerFrame);
        sizes.reserve(packetsPerFrame);

        for (auto _ : state)
        {
            packets.clear();
            sizes.clear();

            for (size_t i = 0; i < packetsPerFrame; ++i)
            {
                size_t drawItemCount = 1 + (i % 8);
                size_t totalSize = sizeof(SimulatedDrawPacket) +
                                   drawItemCount * sizeof(SimulatedDrawItem) +
                                   drawItemCount * sizeof(uint64_t) +
                                   drawItemCount * sizeof(uint32_t) +
                                   drawItemCount * 2 * sizeof(void*);

                void* memory = Allocate(totalSize, DrawPacketAlignment);
                if (memory)
                {
                    memset(memory, static_cast<int>(i & 0xFF), totalSize);
                    packets.push_back(memory);
                    sizes.push_back(totalSize);
                }
            }

            uint64_t checksum = 0;
            for (size_t i = 0; i < packets.size(); ++i)
            {
                const uint8_t* data = static_cast<const uint8_t*>(packets[i]);
                size_t size = sizes[i];

                for (size_t offset = 0; offset < size; offset += 64)
                {
                    checksum += data[offset];
                }
            }
            benchmark::DoNotOptimize(checksum);

            GarbageCollectForce();
        }

        state.SetItemsProcessed(state.iterations() * packetsPerFrame);
    }

    BENCHMARK_DEFINE_F(RealisticMallocFixture, BM_DrawPacketWithAccess)(benchmark::State& state)
    {
        const size_t packetsPerFrame = static_cast<size_t>(state.range(0));
        std::vector<void*> packets;
        std::vector<size_t> sizes;
        packets.reserve(packetsPerFrame);
        sizes.reserve(packetsPerFrame);

        for (auto _ : state)
        {
            packets.clear();
            sizes.clear();

            for (size_t i = 0; i < packetsPerFrame; ++i)
            {
                size_t drawItemCount = 1 + (i % 8);
                size_t totalSize = sizeof(SimulatedDrawPacket) +
                                   drawItemCount * sizeof(SimulatedDrawItem) +
                                   drawItemCount * sizeof(uint64_t) +
                                   drawItemCount * sizeof(uint32_t) +
                                   drawItemCount * 2 * sizeof(void*);

                void* memory = Allocate(totalSize, DrawPacketAlignment);
                if (memory)
                {
                    memset(memory, static_cast<int>(i & 0xFF), totalSize);
                    packets.push_back(memory);
                    sizes.push_back(totalSize);
                }
            }

            uint64_t checksum = 0;
            for (size_t i = 0; i < packets.size(); ++i)
            {
                const uint8_t* data = static_cast<const uint8_t*>(packets[i]);
                size_t size = sizes[i];

                for (size_t offset = 0; offset < size; offset += 64)
                {
                    checksum += data[offset];
                }
            }
            benchmark::DoNotOptimize(checksum);

            GarbageCollectForce();
        }

        state.SetItemsProcessed(state.iterations() * packetsPerFrame);
    }

    // Register DrawPacket with Access benchmarks
    BENCHMARK_REGISTER_F(RealisticLinearAllocatorFixture, BM_DrawPacketWithAccess)
        ->Arg(100)->Arg(500)->Arg(1000)->Arg(2000)->Arg(5000)
        ->Unit(benchmark::kMicrosecond);

    BENCHMARK_REGISTER_F(RealisticSystemAllocatorFixture, BM_DrawPacketWithAccess)
        ->Arg(100)->Arg(500)->Arg(1000)->Arg(2000)->Arg(5000)
        ->Unit(benchmark::kMicrosecond);

    BENCHMARK_REGISTER_F(RealisticMallocFixture, BM_DrawPacketWithAccess)
        ->Arg(100)->Arg(500)->Arg(1000)->Arg(2000)->Arg(5000)
        ->Unit(benchmark::kMicrosecond);

}  // namespace RealisticBenchmark

#endif  // HAVE_BENCHMARK
