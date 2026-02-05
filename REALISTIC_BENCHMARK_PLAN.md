# Realistic LinearAllocator Benchmark Plan

## Overview

This plan outlines a benchmark that simulates actual Atom renderer workloads to compare LinearAllocator performance against SystemAllocator and malloc/free in realistic scenarios.

## Findings from Codebase Analysis

### How LinearAllocator is Actually Used

1. **DrawPacket Construction** (`Gems/Atom/RHI/Code/Source/RHI/DeviceDrawPacketBuilder.cpp`)
   - Primary use case for LinearAllocator
   - Allocates contiguous memory for:
     - `DeviceDrawPacket` header
     - Array of `DeviceDrawItem` (one per draw call in batch)
     - Array of `DrawItemSortKey`
     - Array of `DrawFilterMask`
     - Array of `const ShaderResourceGroup*` pointers
   - Pattern: Build many packets per frame, bulk-free at frame end

2. **Staging Memory Allocation** (`Gems/Atom/RHI/Code/Source/RHI/StagingMemoryAllocator.cpp`)
   - Uses `MemoryLinearSubAllocator` (wraps LinearAllocator)
   - Allocates GPU upload buffers for textures, vertex data, constants
   - Pattern: Many variable-size allocations, reset per frame

3. **Frame Lifecycle** (`Gems/Atom/RHI/Code/Source/RHI/Device.cpp`)
   - `GarbageCollect()` called in `Device::EndFrameInternal()`
   - All frame allocations bulk-freed simultaneously

### Key Allocation Patterns

| Pattern | Sizes | Count/Frame | Alignment |
|---------|-------|-------------|-----------|
| DrawPacket headers | 64-256 bytes | 100-10,000 | 8 bytes |
| DrawItem arrays | 48 bytes × N | varies | 8 bytes |
| SRG pointer arrays | 8 bytes × N | varies | 8 bytes |
| Staging buffers | 256 bytes - 16 MB | 10-1,000 | 256 bytes |

## Benchmark Design

### Benchmark 1: DrawPacket Simulation

Simulates the DrawPacketBuilder allocation pattern.

```cpp
struct SimulatedDrawPacket
{
    uint64_t header[8];           // 64 bytes - packet metadata
    // Followed by variable-length arrays allocated contiguously
};

struct SimulatedDrawItem
{
    uint64_t data[6];             // 48 bytes - matches DeviceDrawItem
};

void BM_DrawPacketSimulation(benchmark::State& state)
{
    const size_t packetsPerFrame = state.range(0);
    const size_t avgDrawItemsPerPacket = 4;  // Typical batch size

    for (auto _ : state)
    {
        // Simulate one frame of DrawPacket construction
        for (size_t i = 0; i < packetsPerFrame; ++i)
        {
            size_t drawItemCount = 1 + (i % 8);  // 1-8 items per packet

            // Calculate total size (mimics DeviceDrawPacketBuilder::End())
            size_t packetSize = sizeof(SimulatedDrawPacket);
            size_t drawItemsSize = drawItemCount * sizeof(SimulatedDrawItem);
            size_t sortKeysSize = drawItemCount * sizeof(uint64_t);
            size_t filterMaskSize = drawItemCount * sizeof(uint32_t);
            size_t srgPointersSize = drawItemCount * 2 * sizeof(void*);  // 2 SRGs typical

            size_t totalSize = packetSize + drawItemsSize + sortKeysSize +
                              filterMaskSize + srgPointersSize;

            void* memory = allocator.Allocate(totalSize, 8);

            // Simulate initialization (touch memory to be realistic)
            memset(memory, 0, totalSize);
            benchmark::DoNotOptimize(memory);
        }

        // Frame end - bulk free
        allocator.GarbageCollectForce();  // or free all for malloc
    }

    state.SetItemsProcessed(state.iterations() * packetsPerFrame);
}

// Test with realistic packet counts
BENCHMARK(BM_DrawPacketSimulation)->Arg(100)->Arg(500)->Arg(1000)->Arg(5000);
```

### Benchmark 2: Staging Buffer Simulation

Simulates GPU upload buffer allocation patterns.

```cpp
void BM_StagingBufferSimulation(benchmark::State& state)
{
    const size_t allocationsPerFrame = state.range(0);

    // Realistic staging buffer size distribution
    const size_t sizes[] = {
        256,      // Small constant buffers
        1024,     // Medium constant buffers
        4096,     // Vertex/index data chunks
        16384,    // Larger mesh data
        65536,    // Texture mip levels
        262144,   // Large texture uploads
    };

    for (auto _ : state)
    {
        size_t totalBytes = 0;

        for (size_t i = 0; i < allocationsPerFrame; ++i)
        {
            size_t size = sizes[i % 6];
            void* memory = allocator.Allocate(size, 256);  // GPU alignment

            // Simulate filling buffer with upload data
            memset(memory, 0xAB, size);
            benchmark::DoNotOptimize(memory);

            totalBytes += size;
        }

        allocator.GarbageCollectForce();
        state.SetBytesProcessed(state.iterations() * totalBytes);
    }

    state.SetItemsProcessed(state.iterations() * allocationsPerFrame);
}

BENCHMARK(BM_StagingBufferSimulation)->Arg(50)->Arg(200)->Arg(500)->Arg(1000);
```

### Benchmark 3: Mixed Frame Workload

Combines both patterns to simulate a complete frame.

```cpp
void BM_MixedFrameWorkload(benchmark::State& state)
{
    const size_t sceneComplexity = state.range(0);  // 1 = simple, 10 = complex

    const size_t drawPackets = 100 * sceneComplexity;
    const size_t stagingBuffers = 20 * sceneComplexity;
    const size_t miscAllocations = 50 * sceneComplexity;  // Various frame data

    for (auto _ : state)
    {
        // Phase 1: Draw packet construction (during render graph compilation)
        for (size_t i = 0; i < drawPackets; ++i)
        {
            size_t size = 64 + (i % 8) * 56;  // 64-504 bytes
            void* p = allocator.Allocate(size, 8);
            memset(p, 0, size);
        }

        // Phase 2: Staging buffer allocation (during resource uploads)
        for (size_t i = 0; i < stagingBuffers; ++i)
        {
            size_t size = 256 << (i % 6);  // 256 bytes to 8KB
            void* p = allocator.Allocate(size, 256);
            memset(p, 0xCD, size);
        }

        // Phase 3: Miscellaneous frame allocations
        for (size_t i = 0; i < miscAllocations; ++i)
        {
            size_t size = 32 + (i % 32) * 16;  // 32-528 bytes
            void* p = allocator.Allocate(size, 8);
            benchmark::DoNotOptimize(p);
        }

        // Frame end
        allocator.GarbageCollectForce();
    }

    size_t totalAllocations = drawPackets + stagingBuffers + miscAllocations;
    state.SetItemsProcessed(state.iterations() * totalAllocations);
}

BENCHMARK(BM_MixedFrameWorkload)->DenseRange(1, 10);
```

### Benchmark 4: Multi-Frame Sustained Load

Tests allocator behavior over multiple frames (cache effects, fragmentation).

```cpp
void BM_SustainedFrameLoad(benchmark::State& state)
{
    const size_t framesPerIteration = 60;  // One second at 60 FPS
    const size_t allocationsPerFrame = 1000;

    for (auto _ : state)
    {
        for (size_t frame = 0; frame < framesPerIteration; ++frame)
        {
            // Vary allocation pattern slightly per frame
            for (size_t i = 0; i < allocationsPerFrame; ++i)
            {
                size_t size = 64 + ((frame + i) % 64) * 32;
                void* p = allocator.Allocate(size, 16);
                benchmark::DoNotOptimize(p);
            }

            allocator.GarbageCollectForce();
        }
    }

    state.SetItemsProcessed(state.iterations() * framesPerIteration * allocationsPerFrame);
}
```

## Implementation Fixtures

### LinearAllocator Fixture

```cpp
class RealisticLinearAllocatorFixture : public benchmark::Fixture
{
public:
    void SetUp(benchmark::State& state) override
    {
        AZ::RHI::LinearAllocator::Descriptor desc;
        desc.m_capacityInBytes = 64 * 1024 * 1024;  // 64 MB - realistic frame budget
        desc.m_addressBase = AZ::RHI::VirtualAddress::CreateFromPointer(
            AZ::AllocatorInstance<AZ::SystemAllocator>::Get().allocate(
                desc.m_capacityInBytes, 256));
        m_allocator.Init(desc);
    }

    void TearDown(benchmark::State& state) override
    {
        auto base = m_allocator.GetDescriptor().m_addressBase;
        m_allocator.Shutdown();
        AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(
            base.m_ptr, 64 * 1024 * 1024, 256);
    }

protected:
    AZ::RHI::LinearAllocator m_allocator;
};
```

### SystemAllocator Fixture (Per-Allocation Tracking)

```cpp
class RealisticSystemAllocatorFixture : public benchmark::Fixture
{
public:
    void SetUp(benchmark::State& state) override
    {
        m_allocations.reserve(100000);  // Pre-allocate tracking
    }

    void TearDown(benchmark::State& state) override
    {
        // Free any remaining allocations
        for (auto& alloc : m_allocations)
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(
                alloc.ptr, alloc.size, alloc.alignment);
        }
        m_allocations.clear();
    }

protected:
    void* Allocate(size_t size, size_t alignment)
    {
        void* ptr = AZ::AllocatorInstance<AZ::SystemAllocator>::Get()
            .allocate(size, alignment);
        m_allocations.push_back({ptr, size, alignment});
        return ptr;
    }

    void GarbageCollectForce()
    {
        for (auto& alloc : m_allocations)
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Get().deallocate(
                alloc.ptr, alloc.size, alloc.alignment);
        }
        m_allocations.clear();
    }

private:
    struct Allocation { void* ptr; size_t size; size_t alignment; };
    AZStd::vector<Allocation> m_allocations;
};
```

### malloc/free Fixture (Per-Allocation Tracking)

```cpp
class RealisticMallocFixture : public benchmark::Fixture
{
public:
    void SetUp(benchmark::State& state) override
    {
        m_allocations.reserve(100000);
    }

    void TearDown(benchmark::State& state) override
    {
        for (void* ptr : m_allocations)
        {
            free(ptr);
        }
        m_allocations.clear();
    }

protected:
    void* Allocate(size_t size, size_t alignment)
    {
        void* ptr;
        if (alignment <= alignof(max_align_t))
        {
            ptr = malloc(size);
        }
        else
        {
            ptr = aligned_alloc(alignment, (size + alignment - 1) & ~(alignment - 1));
        }
        m_allocations.push_back(ptr);
        return ptr;
    }

    void GarbageCollectForce()
    {
        for (void* ptr : m_allocations)
        {
            free(ptr);
        }
        m_allocations.clear();
    }

private:
    std::vector<void*> m_allocations;
};
```

## Expected Results

Based on microbenchmark results and algorithmic analysis:

| Benchmark | LinearAllocator | SystemAllocator | malloc | LinearAllocator Advantage |
|-----------|-----------------|-----------------|--------|---------------------------|
| DrawPacket (1000 packets) | ~5 µs | ~150 µs | ~80 µs | 15-30x |
| Staging Buffers (200 allocs) | ~3 µs | ~100 µs | ~50 µs | 15-35x |
| Mixed Frame (medium) | ~15 µs | ~400 µs | ~200 µs | 15-25x |
| Sustained 60 frames | ~300 µs | ~8 ms | ~4 ms | 15-25x |

## Implementation Steps

1. **Create benchmark file**: `Gems/Atom/RHI/Code/Tests/RealisticLinearAllocatorBenchmarks.cpp`

2. **Add to CMake**: Update `atom_rhi_tests_files.cmake`

3. **Implement fixtures**: All three allocator fixtures with identical interfaces

4. **Implement benchmarks**: All four benchmark scenarios for each fixture

5. **Add memory throughput metrics**: Track bytes/second in addition to items/second

6. **Add latency percentiles**: Use `benchmark::Counter` for p50/p99 latency

7. **Test and validate**: Ensure benchmarks compile and produce meaningful results

8. **Run comparative analysis**: Generate comparison tables and charts

9. **Update ALLOCATION.md**: Add realistic benchmark results to documentation

## File Structure

```
Gems/Atom/RHI/Code/Tests/
├── LinearAllocatorBenchmarks.cpp           # Existing microbenchmarks
└── RealisticLinearAllocatorBenchmarks.cpp  # New realistic workload benchmarks
```

## Success Criteria

1. Benchmarks compile and run without errors
2. Results show consistent ~15-30x advantage for LinearAllocator in frame-scoped patterns
3. Benchmarks exercise realistic memory access patterns (not just allocation calls)
4. Results are reproducible across multiple runs (<5% variance)
5. Documentation updated with realistic benchmark findings
