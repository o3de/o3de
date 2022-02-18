/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/PlatformIncl.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Memory/BestFitExternalMapAllocator.h>
#include <AzCore/Memory/HeapSchema.h>
#include <AzCore/Memory/HphaSchema.h>
#include <AzCore/Memory/MallocSchema.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/PoolSchema.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/Utils/Utils.h>

#include <benchmark/benchmark.h>

namespace Benchmark
{
    namespace Platform
    {
        size_t GetProcessMemoryUsageBytes();
        size_t GetMemorySize(void* memory);
    }

    /// <summary>
    /// Test allocator wrapper that redirects the calls to the passed TAllocator by using AZ::AllocatorInstance.
    /// It also creates/destroys the TAllocator type (to reflect what happens at runtime)
    /// </summary>
    /// <typeparam name="TAllocator">Allocator type to wrap</typeparam>
    template<typename TAllocator>
    class TestAllocatorWrapper
    {
    public:
        static void SetUp()
        {
            AZ::AllocatorInstance<TAllocator>::Create();
        }

        static void TearDown()
        {
            AZ::AllocatorInstance<TAllocator>::Destroy();
        }

        static void* Allocate(size_t byteSize, size_t alignment)
        {
            return AZ::AllocatorInstance<TAllocator>::Get().Allocate(byteSize, alignment);
        }

        static void DeAllocate(void* ptr, size_t byteSize = 0)
        {
            AZ::AllocatorInstance<TAllocator>::Get().DeAllocate(ptr, byteSize);
        }

        static void* ReAllocate(void* ptr, size_t newSize, size_t newAlignment)
        {
            return AZ::AllocatorInstance<TAllocator>::Get().ReAllocate(ptr, newSize, newAlignment);
        }

        static size_t Resize(void* ptr, size_t newSize)
        {
            return AZ::AllocatorInstance<TAllocator>::Get().Resize(ptr, newSize);
        }

        static void GarbageCollect()
        {
            AZ::AllocatorInstance<TAllocator>::Get().GarbageCollect();
        }

        static size_t NumAllocatedBytes()
        {
            return AZ::AllocatorInstance<TAllocator>::Get().NumAllocatedBytes() +
                AZ::AllocatorInstance<TAllocator>::Get().GetUnAllocatedMemory();
        }

        static size_t GetSize(void* ptr)
        {
            return AZ::AllocatorInstance<TAllocator>::Get().AllocationSize(ptr);
        }
    };

    /// <summary>
    /// Basic allocator used as a baseline. This allocator is the most basic allocation possible with the OS (AZ_OS_MALLOC).
    /// MallocSchema cannot be used here because it has extra logic that we don't want to use as a baseline.
    /// </summary>
    class RawMallocAllocator {};

    template<>
    class TestAllocatorWrapper<RawMallocAllocator>
    {
    public:
        TestAllocatorWrapper()
        {
            s_numAllocatedBytes = 0;
        }

        static void SetUp()
        {
            s_numAllocatedBytes = 0;
        }

        static void TearDown()
        {
        }

        static void* Allocate(size_t byteSize, size_t)
        {
            s_numAllocatedBytes += byteSize;
            // Don't pass an alignment since we wont be able to get the memory size without also passing the alignment
            return AZ_OS_MALLOC(byteSize, 1);
        }

        static void DeAllocate(void* ptr, size_t = 0)
        {
            s_numAllocatedBytes -= Platform::GetMemorySize(ptr);
            AZ_OS_FREE(ptr);
        }

        static void* ReAllocate(void* ptr, size_t newSize, size_t)
        {
            s_numAllocatedBytes -= Platform::GetMemorySize(ptr);
            AZ_OS_FREE(ptr);

            s_numAllocatedBytes += newSize;
            return AZ_OS_MALLOC(newSize, 1);
        }

        static size_t Resize(void* ptr, size_t newSize)
        {
            AZ_UNUSED(ptr);
            AZ_UNUSED(newSize);

            return 0;
        }

        static void GarbageCollect() {}

        static size_t NumAllocatedBytes()
        {
            return s_numAllocatedBytes;
        }

        static size_t GetSize(void* ptr)
        {
            return Platform::GetMemorySize(ptr);
        }

    private:
         inline static size_t s_numAllocatedBytes = 0;
    };

    // Some allocator are not fully declared, those we simply setup from the schema
    class MallocSchemaAllocator : public AZ::SimpleSchemaAllocator<AZ::MallocSchema>
    {
    public:
        AZ_TYPE_INFO(MallocSchemaAllocator, "{3E68224F-E676-402C-8276-CE4B49C05E89}");

        MallocSchemaAllocator()
            : AZ::SimpleSchemaAllocator<AZ::MallocSchema>("MallocSchemaAllocator", "")
        {}
    };

    // We use both this HphaSchemaAllocator and the SystemAllocator configured with Hpha because the SystemAllocator
    // has extra things
    class HphaSchemaAllocator : public AZ::SimpleSchemaAllocator<AZ::HphaSchema>
    {
    public:
        AZ_TYPE_INFO(HphaSchemaAllocator, "{6563AB4B-A68E-4499-8C98-D61D640D1F7F}");

        HphaSchemaAllocator()
            : AZ::SimpleSchemaAllocator<AZ::HphaSchema>("TestHphaSchemaAllocator", "")
        {}
    };

    // For the SystemAllocator we inherit so we have a different stack. The SystemAllocator is used globally so we dont want
    // to get that data affecting the benchmark
    class TestSystemAllocator : public AZ::SystemAllocator
    {
    public:
        AZ_TYPE_INFO(TestSystemAllocator, "{360D4DAA-D65D-4D5C-A6FA-1A4C5261C35C}");

        TestSystemAllocator()
            : AZ::SystemAllocator()
        {
        }
    };

    // Allocated bytes reported by the allocator
    static const char* s_counterAllocatorMemory = "Allocator_Memory";

    // Allocated bytes as counted by the benchmark
    static const char* s_counterBenchmarkMemory = "Benchmark_Memory";

    enum AllocationSize
    {
        SMALL,
        BIG,
        MIXED,
        COUNT
    };

    static const size_t s_kiloByte = 1024;
    static const size_t s_megaByte = s_kiloByte * s_kiloByte;
    using AllocationSizeArray = AZStd::array<size_t, 10>;
    static const AZStd::array<AllocationSizeArray, COUNT> s_allocationSizes = {
            /* SMALL */ AllocationSizeArray{ 2, 16, 20, 59, 100, 128, 160, 250, 300, 512 },
            /* BIG */ AllocationSizeArray{ 513, s_kiloByte, 2 * s_kiloByte, 4 * s_kiloByte, 10 * s_kiloByte, 64 * s_kiloByte, 128 * s_kiloByte, 200 * s_kiloByte, s_megaByte, 2 * s_megaByte },
            /* MIXED */ AllocationSizeArray{ 2, s_kiloByte, 59, 4 * s_kiloByte, 128, 200 * s_kiloByte, 250, s_megaByte, 512, 2 * s_megaByte }
        };

    template <typename TAllocator>
    class AllocatorBenchmarkFixture
        : public ::benchmark::Fixture
    {
    protected:
        using TestAllocatorType = TestAllocatorWrapper<TAllocator>;

        virtual void internalSetUp(const ::benchmark::State& state)
        {
            if (state.thread_index == 0) // Only setup in the first thread
            {
                TestAllocatorType::SetUp();

                m_allocations.resize(state.threads);
                for (auto& perThreadAllocations : m_allocations)
                {
                    perThreadAllocations.resize(state.range(0), nullptr);
                }
            }
        }

        virtual void internalTearDown(const ::benchmark::State& state)
        {
            if (state.thread_index == 0) // Only setup in the first thread
            {
                m_allocations.clear();
                m_allocations.shrink_to_fit();

                TestAllocatorType::TearDown();
            }
        }

        AZStd::vector<void*>& GetPerThreadAllocations(size_t threadIndex)
        {
            return m_allocations[threadIndex];
        }

    public:
        void SetUp(const ::benchmark::State& state) override
        {
            internalSetUp(state);
        }
        void SetUp(::benchmark::State& state) override
        {
            internalSetUp(state);
        }

        void TearDown(const ::benchmark::State& state) override
        {
            internalTearDown(state);
        }
        void TearDown(::benchmark::State& state) override
        {
            internalTearDown(state);
        }

    private:
        AZStd::vector<AZStd::vector<void*>> m_allocations;
    };

    template <typename TAllocator, AllocationSize TAllocationSize>
    class AllocationBenchmarkFixture
        : public AllocatorBenchmarkFixture<TAllocator>
    {
        using base = AllocatorBenchmarkFixture<TAllocator>;
        using TestAllocatorType = typename base::TestAllocatorType;

    public:
        void Benchmark(benchmark::State& state)
        {
            for ([[maybe_unused]] auto _ : state)
            {
                state.PauseTiming();

                AZStd::vector<void*>& perThreadAllocations = base::GetPerThreadAllocations(state.thread_index);
                const size_t numberOfAllocations = perThreadAllocations.size();
                size_t totalAllocationSize = 0;
                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    totalAllocationSize += allocationSize;

                    state.ResumeTiming();
                    perThreadAllocations[allocationIndex] = TestAllocatorType::Allocate(allocationSize, 0);
                    state.PauseTiming();
                }

                state.counters[s_counterAllocatorMemory] = benchmark::Counter(static_cast<double>(TestAllocatorType::NumAllocatedBytes()), benchmark::Counter::kDefaults);
                state.counters[s_counterBenchmarkMemory] = benchmark::Counter(static_cast<double>(totalAllocationSize), benchmark::Counter::kDefaults);

                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    TestAllocatorType::DeAllocate(perThreadAllocations[allocationIndex], allocationSize);
                    perThreadAllocations[allocationIndex] = nullptr;
                }
                TestAllocatorType::GarbageCollect();

                state.SetItemsProcessed(numberOfAllocations);

                state.ResumeTiming();
            }
        }
    };

    template <typename TAllocator, AllocationSize TAllocationSize>
    class DeAllocationBenchmarkFixture
        : public AllocatorBenchmarkFixture<TAllocator>
    {
        using base = AllocatorBenchmarkFixture<TAllocator>;
        using TestAllocatorType = typename base::TestAllocatorType;

    public:
        void Benchmark(benchmark::State& state)
        {
            for ([[maybe_unused]] auto _ : state)
            {
                state.PauseTiming();
                AZStd::vector<void*>& perThreadAllocations = base::GetPerThreadAllocations(state.thread_index);

                const size_t numberOfAllocations = perThreadAllocations.size();
                size_t totalAllocationSize = 0;
                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    totalAllocationSize += allocationSize;
                    perThreadAllocations[allocationIndex] = TestAllocatorType::Allocate(allocationSize, 0);
                }

                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    state.ResumeTiming();
                    TestAllocatorType::DeAllocate(perThreadAllocations[allocationIndex], allocationSize);
                    state.PauseTiming();
                    perThreadAllocations[allocationIndex] = nullptr;
                }

                state.counters[s_counterAllocatorMemory] = benchmark::Counter(static_cast<double>(TestAllocatorType::NumAllocatedBytes()), benchmark::Counter::kDefaults);
                state.counters[s_counterBenchmarkMemory] = benchmark::Counter(static_cast<double>(totalAllocationSize), benchmark::Counter::kDefaults);

                state.SetItemsProcessed(numberOfAllocations);

                TestAllocatorType::GarbageCollect();

                state.ResumeTiming();
            }
        }
    };

    template<typename TAllocator>
    class RecordedAllocationBenchmarkFixture : public ::benchmark::Fixture
    {
        using TestAllocatorType = TestAllocatorWrapper<TAllocator>;

        virtual void internalSetUp()
        {
            TestAllocatorType::SetUp();
        }

        void internalTearDown()
        {
            TestAllocatorType::TearDown();
        }

        #pragma pack(push, 1)
        struct alignas(1) AllocatorOperation
        {
            enum OperationType : size_t
            {
                ALLOCATE,
                DEALLOCATE
            };
            OperationType m_type : 1;
            size_t m_size : 28; // Can represent up to 256Mb requests
            size_t m_alignment : 7; // Can represent up to 128 alignment
            size_t m_recordId : 28; // Can represent up to 256M simultaneous requests, we reuse ids
        };
        #pragma pack(pop)
        static_assert(sizeof(AllocatorOperation) == 8);

    public:
        void SetUp(const ::benchmark::State&) override
        {
            internalSetUp();
        }
        void SetUp(::benchmark::State&) override
        {
            internalSetUp();
        }

        void TearDown(const ::benchmark::State&) override
        {
            internalTearDown();
        }
        void TearDown(::benchmark::State&) override
        {
            internalTearDown();
        }

        void Benchmark(benchmark::State& state)
        {
            for ([[maybe_unused]] auto _ : state)
            {
                state.PauseTiming();

                AZStd::unordered_map<size_t, void*> pointerRemapping;
                constexpr size_t allocationOperationCount = 5 * 1024;
                AZStd::array<AllocatorOperation, allocationOperationCount> m_operations = {};
                [[maybe_unused]] const size_t operationSize = sizeof(AllocatorOperation);

                size_t totalAllocationSize = 0;
                size_t itemsProcessed = 0;

                for (size_t i = 0; i < 100; ++i) // play the recording multiple times to get a good stable sample, this way we can keep a smaller recording
                {
                    AZ::IO::SystemFile file;
                    AZ::IO::FixedMaxPathString filePath = AZ::Utils::GetExecutableDirectory();
                    filePath += "/Tests/AzCore/Memory/AllocatorBenchmarkRecordings.bin";
                    if (!file.Open(filePath.c_str(), AZ::IO::SystemFile::OpenMode::SF_OPEN_READ_ONLY))
                    {
                        return;
                    }
                    size_t elementsRead =
                        file.Read(sizeof(AllocatorOperation) * allocationOperationCount, &m_operations) / sizeof(AllocatorOperation);
                    itemsProcessed += elementsRead;

                    while (elementsRead > 0)
                    {
                        for (size_t operationIndex = 0; operationIndex < elementsRead; ++operationIndex)
                        {
                            const AllocatorOperation& operation = m_operations[operationIndex];
                            if (operation.m_type == AllocatorOperation::ALLOCATE)
                            {
                                const auto it = pointerRemapping.emplace(operation.m_recordId, nullptr);
                                if (it.second) // otherwise already allocated
                                {
                                    state.ResumeTiming();
                                    void* ptr = TestAllocatorType::Allocate(operation.m_size, operation.m_alignment);
                                    state.PauseTiming();
                                    totalAllocationSize += operation.m_size;
                                    it.first->second = ptr;
                                }
                                else
                                {
                                    // Doing a resize, dont account for this memory change, this operation is rare and we dont have
                                    // the size of the previous allocation
                                    state.ResumeTiming();
                                    TestAllocatorType::Resize(it.first->second, operation.m_size);
                                    state.PauseTiming();
                                }
                            }
                            else // AllocatorOperation::DEALLOCATE:
                            {
                                if (operation.m_recordId)
                                {
                                    const auto ptrIt = pointerRemapping.find(operation.m_recordId);
                                    if (ptrIt != pointerRemapping.end())
                                    {
                                        totalAllocationSize -= operation.m_size;
                                        state.ResumeTiming();
                                        TestAllocatorType::DeAllocate(
                                            ptrIt->second,
                                            /*operation.m_size*/ 0); // size is not correct after a resize, a 0 size deals with it
                                        state.PauseTiming();
                                        pointerRemapping.erase(ptrIt);
                                    }
                                }
                                else // deallocate(nullptr) are recorded
                                {
                                    // Just to account of the call of deallocate(nullptr);
                                    state.ResumeTiming();
                                    TestAllocatorType::DeAllocate(nullptr, /*operation.m_size*/ 0);
                                    state.PauseTiming();
                                }
                            }
                        }

                        elementsRead =
                            file.Read(sizeof(AllocatorOperation) * allocationOperationCount, &m_operations) / sizeof(AllocatorOperation);
                        itemsProcessed += elementsRead;
                    }
                    file.Close();

                    // Deallocate the remainder (since we stopped the recording middle-game)(there are leaks as well)
                    for (const auto& pointerMapping : pointerRemapping)
                    {
                        state.ResumeTiming();
                        TestAllocatorType::DeAllocate(pointerMapping.second);
                        state.PauseTiming();
                    }
                    itemsProcessed += pointerRemapping.size();
                    pointerRemapping.clear();
                }

                state.counters[s_counterAllocatorMemory] = benchmark::Counter(static_cast<double>(TestAllocatorType::NumAllocatedBytes()), benchmark::Counter::kDefaults);
                state.counters[s_counterBenchmarkMemory] = benchmark::Counter(static_cast<double>(totalAllocationSize), benchmark::Counter::kDefaults);

                state.SetItemsProcessed(itemsProcessed);

                TestAllocatorType::GarbageCollect();

                state.ResumeTiming();
            }
        }
    };

    // For non-threaded ranges, run 100, 400, 1600 amounts
    static void RunRanges(benchmark::internal::Benchmark* b)
    {
        for (int i = 0; i < 6; i += 2)
        {
            b->Arg((1 << i) * 100);
        }
    }
    static void RecordedRunRanges(benchmark::internal::Benchmark* b)
    {
        b->Iterations(1);
    }

    // For threaded ranges, run just 200, multi-threaded will already multiply by thread
    static void ThreadedRunRanges(benchmark::internal::Benchmark* b)
    {
        b->Arg(100);
    }

    // Test under and over-subscription of threads vs the amount of CPUs available
    static const unsigned int MaxThreadRange = 2 * AZStd::thread::hardware_concurrency();

#define BM_REGISTER_TEMPLATE(FIXTURE, TESTNAME, ...) \
        BENCHMARK_TEMPLATE_DEFINE_F(FIXTURE, TESTNAME, __VA_ARGS__)(benchmark::State& state) { Benchmark(state); } \
        BENCHMARK_REGISTER_F(FIXTURE, TESTNAME)

    // We test small/big/mixed allocations in single-threaded environments. For multi-threaded environments, we test mixed since
    // the multi threaded fixture will run multiple passes (1, 2, 4, ... until 2*hardware_concurrency)
#define BM_REGISTER_SIZE_FIXTURES(FIXTURE, TESTNAME, ALLOCATORTYPE) \
    BM_REGISTER_TEMPLATE(FIXTURE, TESTNAME##_SMALL, ALLOCATORTYPE, SMALL)->Apply(RunRanges); \
    BM_REGISTER_TEMPLATE(FIXTURE, TESTNAME##_BIG, ALLOCATORTYPE, BIG)->Apply(RunRanges); \
    BM_REGISTER_TEMPLATE(FIXTURE, TESTNAME##_MIXED, ALLOCATORTYPE, MIXED)->Apply(RunRanges); \
    BM_REGISTER_TEMPLATE(FIXTURE, TESTNAME##_MIXED_THREADED, ALLOCATORTYPE, MIXED)->ThreadRange(2, MaxThreadRange)->Apply(ThreadedRunRanges);

#define BM_REGISTER_ALLOCATOR(TESTNAME, ALLOCATORTYPE) \
    namespace BM_##TESTNAME \
    { \
        BM_REGISTER_SIZE_FIXTURES(AllocationBenchmarkFixture, TESTNAME, ALLOCATORTYPE); \
        BM_REGISTER_SIZE_FIXTURES(DeAllocationBenchmarkFixture, TESTNAME, ALLOCATORTYPE); \
        BM_REGISTER_TEMPLATE(RecordedAllocationBenchmarkFixture, TESTNAME, ALLOCATORTYPE)->Apply(RecordedRunRanges); \
    }

    /// Warm up benchmark used to prepare the OS for allocations. Most OS keep allocations for a process somehow
    /// reserved. So the first allocations run always get a bigger impact in a process. This warm up allocator runs
    /// all the benchmarks and is just used for the the next allocators to report more consistent results.
    BM_REGISTER_ALLOCATOR(WarmUpAllocator, RawMallocAllocator);

    BM_REGISTER_ALLOCATOR(RawMallocAllocator, RawMallocAllocator);
    BM_REGISTER_ALLOCATOR(MallocSchemaAllocator, MallocSchemaAllocator);
    BM_REGISTER_ALLOCATOR(HphaSchemaAllocator, HphaSchemaAllocator);
    BM_REGISTER_ALLOCATOR(SystemAllocator, TestSystemAllocator);
    
    //BM_REGISTER_ALLOCATOR(BestFitExternalMapAllocator, BestFitExternalMapAllocator); // Requires to pre-allocate blocks and cannot work as a general-purpose allocator
    //BM_REGISTER_ALLOCATOR(HeapSchemaAllocator, TestHeapSchemaAllocator); // Requires to pre-allocate blocks and cannot work as a general-purpose allocator
    //BM_REGISTER_SCHEMA(PoolSchema); // Requires special alignment requests while allocating
    // BM_REGISTER_ALLOCATOR(OSAllocator, OSAllocator); // Requires special treatment to initialize since it will be already initialized, maybe creating a different instance?

#undef BM_REGISTER_ALLOCATOR
#undef BM_REGISTER_SIZE_FIXTURES
#undef BM_REGISTER_TEMPLATE

} // Benchmark

#endif // HAVE_BENCHMARK
