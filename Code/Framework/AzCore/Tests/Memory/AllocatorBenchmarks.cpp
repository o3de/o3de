/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if defined(HAVE_BENCHMARK)

#include <AzCore/PlatformIncl.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Driller/Driller.h>
#include <AzCore/Memory/MemoryDriller.h>
#include <AzCore/Memory/BestFitExternalMapSchema.h>
#include <AzCore/Memory/HeapSchema.h>
#include <AzCore/Memory/HphaSchema.h>
#include <AzCore/Memory/MallocSchema.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/PoolSchema.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/vector.h>

#include <benchmark/benchmark.h>

namespace Benchmark
{
    namespace Platform
    {
        size_t GetProcessMemoryUsageBytes();
        size_t GetMemorySize(void* memory);
    }

    //static AZ::Debug::DrillerManager* s_drillerManager = nullptr;

    /// <summary>
    /// Test allocator wrapper that redirects the calls to the passed TAllocator by using AZ::AllocatorInstance.
    /// It also creates/destroys the TAllocator type and connects the driller (to reflect what happens at runtime)
    /// </summary>
    /// <typeparam name="TAllocator">Allocator type to wrap</typeparam>
    template<typename TAllocator>
    class TestAllocatorWrapper
    {
    public:
        static void SetUp()
        {
            AZ::AllocatorInstance<TAllocator>::Create();

            /*s_drillerManager = AZ::Debug::DrillerManager::Create();
            s_drillerManager->Register(aznew AZ::Debug::MemoryDriller);*/
        }

        static void TearDown()
        {
            /*AZ::Debug::DrillerManager::Destroy(s_drillerManager);
            s_drillerManager = nullptr;*/

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
    class TestRawMallocAllocator {};

    template<>
    class TestAllocatorWrapper<TestRawMallocAllocator>
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

        // IAllocatorAllocate
        static void* Allocate(size_t byteSize, size_t alignment)
        {
            s_numAllocatedBytes += byteSize;
            if (alignment)
            {
                return AZ_OS_MALLOC(byteSize, alignment);
            }
            else
            {
                return AZ_OS_MALLOC(byteSize, 1);
            }
        }

        static void DeAllocate(void* ptr, size_t = 0)
        {
            s_numAllocatedBytes -= Platform::GetMemorySize(ptr);
            AZ_OS_FREE(ptr);
        }

        static void* ReAllocate(void* ptr, size_t newSize, size_t newAlignment)
        {
            s_numAllocatedBytes -= Platform::GetMemorySize(ptr);
            AZ_OS_FREE(ptr);

            s_numAllocatedBytes += newSize;
            if (newAlignment)
            {
                return AZ_OS_MALLOC(newSize, newAlignment);
            }
            else
            {
                return AZ_OS_MALLOC(newSize, 1);
            }
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
        static size_t s_numAllocatedBytes;
    };

    size_t TestAllocatorWrapper<TestRawMallocAllocator>::s_numAllocatedBytes = 0;

    // Here we require to implement this to be able to configure a name for the allocator, otherswise the AllocatorManager crashes when trying to configure the overrides
    class TestMallocSchemaAllocator : public AZ::SimpleSchemaAllocator<AZ::MallocSchema>
    {
    public:
        AZ_TYPE_INFO(TestMallocSchemaAllocator, "{3E68224F-E676-402C-8276-CE4B49C05E89}");

        TestMallocSchemaAllocator()
            : AZ::SimpleSchemaAllocator<AZ::MallocSchema>("TestMallocSchemaAllocator", "")
        {}
    };

    class TestHeapSchemaAllocator : public AZ::SimpleSchemaAllocator<AZ::HeapSchema>
    {
    public:
        AZ_TYPE_INFO(TestHeapSchemaAllocator, "{456E6C30-AA84-488F-BE47-5C1E6AF636B7}");

        TestHeapSchemaAllocator()
            : AZ::SimpleSchemaAllocator<AZ::HeapSchema>("TestHeapSchemaAllocator", "")
        {}
    };

    class TestHphaSchemaAllocator : public AZ::SimpleSchemaAllocator<AZ::HphaSchema>
    {
    public:
        AZ_TYPE_INFO(TestHphaSchemaAllocator, "{6563AB4B-A68E-4499-8C98-D61D640D1F7F}");

        TestHphaSchemaAllocator()
            : AZ::SimpleSchemaAllocator<AZ::HphaSchema>("TestHphaSchemaAllocator", "")
        {}
    };

    class TestSystemAllocator : public AZ::SystemAllocator
    {
    public:
        AZ_TYPE_INFO(TestSystemAllocator, "{360D4DAA-D65D-4D5C-A6FA-1A4C5261C35C}");
    };

    // Allocated bytes reported by the allocator
    static const char* s_counterAllocatorMemory = "Allocator_Memory";

    // Allocated bytes reported by the process
    static const char* s_counterProcessMemory = "Process_Memory";

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
            for (auto _ : state)
            {
                state.PauseTiming();
                const size_t processMemoryBaseline = Platform::GetProcessMemoryUsageBytes();

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
                state.counters[s_counterProcessMemory] = benchmark::Counter(static_cast<double>(Platform::GetProcessMemoryUsageBytes() - processMemoryBaseline), benchmark::Counter::kDefaults);
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
            for (auto _ : state)
            {
                state.PauseTiming();
                AZStd::vector<void*>& perThreadAllocations = base::GetPerThreadAllocations(state.thread_index);
                const size_t processMemoryBaseline = Platform::GetProcessMemoryUsageBytes();

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
                state.counters[s_counterProcessMemory] = benchmark::Counter(static_cast<double>(Platform::GetProcessMemoryUsageBytes() - processMemoryBaseline), benchmark::Counter::kDefaults);
                state.counters[s_counterBenchmarkMemory] = benchmark::Counter(static_cast<double>(totalAllocationSize), benchmark::Counter::kDefaults);

                state.SetItemsProcessed(numberOfAllocations);

                TestAllocatorType::GarbageCollect();
            }
        }
    };
      
    template<typename TAllocator>
    class RecordedAllocationBenchmarkFixture : public AllocatorBenchmarkFixture<TAllocator>
    {
        using base = AllocatorBenchmarkFixture<TAllocator>;
        using TestAllocatorType = typename base::TestAllocatorType;

        struct AllocatorOperation
        {
            enum OperationType : unsigned int
            {
                ALLOCATE,
                DEALLOCATE,
                REALLOCATE,
                RESIZE
            };
            OperationType m_operationType : 2;
            size_t m_size : 46;
            size_t m_alignment : 16;
            void* m_ptr;
            void* m_newptr; // required for resize
        };

    public:
        void Benchmark(benchmark::State& state)
        {
            for (auto _ : state)
            {
                state.PauseTiming();

                AZStd::unordered_map<void*, void*> pointerRemapping;
                AZStd::unordered_map<void*, size_t> allocationSize;
                constexpr size_t allocationOperationCount = 5 * 1024;
                AZStd::array<AllocatorOperation, allocationOperationCount> m_operations = {};

                FILE* file = nullptr;
                fopen_s(&file, "memoryrecordings.bin", "rb");
                if (!file)
                {
                    return;
                }
                size_t elementsRead = fread(&m_operations, sizeof(AllocatorOperation), allocationOperationCount, file);
                size_t totalElementsRead = elementsRead;
                const size_t processMemoryBaseline = Platform::GetProcessMemoryUsageBytes();
                size_t totalAllocationSize = 0;

                while (elementsRead > 0)
                {
                    for (size_t operationIndex = 0; operationIndex < elementsRead; ++operationIndex)
                    {
                        const AllocatorOperation& operation = m_operations[operationIndex];
                        switch (operation.m_operationType)
                        {
                        case AllocatorOperation::ALLOCATE:
                        {
                            if (operation.m_ptr)
                            {
                                const auto it = pointerRemapping.emplace(operation.m_ptr, nullptr);
                                if (it.second) // otherwise already allocated
                                {
                                    state.ResumeTiming();
                                    void* ptr = TestAllocatorType::Allocate(operation.m_size, operation.m_alignment);
                                    state.PauseTiming();
                                    totalAllocationSize += operation.m_size;
                                    it.first->second = ptr;
                                    allocationSize[ptr] = operation.m_size;
                                }
                                else
                                {
                                    //AZ_Warning("RecordedAllocationBenchmarkFixture", false, "Allocation on %p was already made", operation.m_ptr);
                                }
                            }
                            break;
                        }
                        case AllocatorOperation::DEALLOCATE:
                        {
                            if (operation.m_ptr) // some deallocate(nullptr) are recorded
                            {
                                const auto ptrIt = pointerRemapping.find(operation.m_ptr);
                                if (ptrIt != pointerRemapping.end())
                                {
                                    totalAllocationSize -= allocationSize[ptrIt->second];
                                    state.ResumeTiming();
                                    TestAllocatorType::DeAllocate(ptrIt->second, /*operation.m_size*/ 0); // size is not correct after a resize, a 0 size deals with it
                                    state.PauseTiming();
                                    pointerRemapping.erase(ptrIt);
                                }
                            }
                            else
                            {
                                // Just to account of the call of deallocate(nullptr);
                                // totalAllocationSize -= 0; // No real deallocation happened
                                state.ResumeTiming();
                                TestAllocatorType::DeAllocate(operation.m_ptr, /*operation.m_size*/ 0);
                                state.PauseTiming();
                            }
                            break;
                        }
                        case AllocatorOperation::REALLOCATE:
                        {
                            void* ptr = nullptr;
                            if (operation.m_ptr)
                            {
                                AZ_Assert(operation.m_newptr, "Need to consider other cases?");
                                const auto ptrIt = pointerRemapping.find(operation.m_ptr);
                                AZ_Assert(ptrIt != pointerRemapping.end(), "Missing allocation for reallocation"); // In case the recording didnt catch something
                                ptr = ptrIt->second;
                                pointerRemapping.erase(ptrIt);
                            }
                            AZ_Assert(operation.m_newptr != nullptr, "Reallocation failed in the game");
                            const auto it = pointerRemapping.emplace(operation.m_newptr, nullptr);
                            if (it.second)
                            {
                                totalAllocationSize -= allocationSize[ptr];
                                state.ResumeTiming();
                                void* newPtr = TestAllocatorType::ReAllocate(ptr, operation.m_size, operation.m_alignment);
                                state.PauseTiming();
                                totalAllocationSize += operation.m_size;
                                it.first->second = newPtr;
                                allocationSize[newPtr] = operation.m_size;
                            }
                            else
                            {
                                totalAllocationSize -= allocationSize[ptr];
                                state.ResumeTiming();
                                TestAllocatorType::DeAllocate(ptr);
                                state.PauseTiming();
                            }
                            break;
                        }
                        case AllocatorOperation::RESIZE:
                        {
                            const auto ptrIt = pointerRemapping.find(operation.m_ptr);
                            AZ_Assert(ptrIt != pointerRemapping.end(), "Missing allocation for resize"); // In case the recording didnt catch something
                            totalAllocationSize -= allocationSize[ptrIt->second];
                            state.ResumeTiming();
                            TestAllocatorType::Resize(ptrIt->second, operation.m_size);
                            state.PauseTiming();
                            totalAllocationSize += operation.m_size;
                            if (operation.m_size == 0)
                            {
                                pointerRemapping.erase(ptrIt);
                            }
                            break;
                        }
                        }
                    }

                    elementsRead = fread(&m_operations, sizeof(AllocatorOperation), allocationOperationCount, file);
                    totalElementsRead += elementsRead;
                }
                fclose(file);

                state.counters[s_counterAllocatorMemory] = benchmark::Counter(static_cast<double>(TestAllocatorType::NumAllocatedBytes()), benchmark::Counter::kDefaults);
                state.counters[s_counterProcessMemory] = benchmark::Counter(static_cast<double>(Platform::GetProcessMemoryUsageBytes() - processMemoryBaseline), benchmark::Counter::kDefaults);
                state.counters[s_counterBenchmarkMemory] = benchmark::Counter(static_cast<double>(totalAllocationSize), benchmark::Counter::kDefaults);

                state.SetItemsProcessed(totalElementsRead);

                // Deallocate the remainder (since we stopped the recording middle-game)(there are leaks as well)
                for (const auto& pointerMapping : pointerRemapping)
                {
                    TestAllocatorType::DeAllocate(pointerMapping.second);
                }
                pointerRemapping.clear();
                TestAllocatorType::GarbageCollect();
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
        b->Arg(1);
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
    namespace TESTNAME \
    { \
        BM_REGISTER_SIZE_FIXTURES(AllocationBenchmarkFixture, TESTNAME, ALLOCATORTYPE); \
        BM_REGISTER_SIZE_FIXTURES(DeAllocationBenchmarkFixture, TESTNAME, ALLOCATORTYPE); \
        BM_REGISTER_TEMPLATE(RecordedAllocationBenchmarkFixture, TESTNAME, ALLOCATORTYPE)->Apply(RecordedRunRanges); \
    }

    BM_REGISTER_ALLOCATOR(RawMallocAllocator, TestRawMallocAllocator);
    BM_REGISTER_ALLOCATOR(MallocSchemaAllocator, TestMallocSchemaAllocator);
    BM_REGISTER_ALLOCATOR(HphaSchemaAllocator, TestHphaSchemaAllocator);
    BM_REGISTER_ALLOCATOR(SystemAllocator, TestSystemAllocator);

    //BM_REGISTER_ALLOCATOR(HeapSchemaAllocator, TestHeapSchemaAllocator); // Requires to pre-allocate blocks and cannot work as a general-purpose allocator
    //BM_REGISTER_SCHEMA(BestFitExternalMapSchema); // Requires to implement AZ::IAllocatorAllocate
    //BM_REGISTER_SCHEMA(PoolSchema); // Requires special alignment requests while allocating

#undef BM_REGISTER_ALLOCATOR
#undef BM_REGISTER_SIZE_FIXTURES
#undef BM_REGISTER_TEMPLATE

} // Benchmark

#endif // HAVE_BENCHMARK
