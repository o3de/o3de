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
#include <AzCore/Memory/HphaAllocator.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>
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

    class RawMallocAllocator
        : public AZStd::stateless_allocator
    {
    public:
        void GarbageCollect() {}
        size_t NumAllocatedBytes() { return 0; }
    };
    // We use both this HphaSchemaAllocator and the SystemAllocator configured with Hpha because the SystemAllocator
    // has extra things
    class HphaSchemaAllocator : public AZ::SimpleSchemaAllocator<AZ::HphaSchema>
    {
    public:
        AZ_TYPE_INFO(HphaSchemaAllocator, "{6563AB4B-A68E-4499-8C98-D61D640D1F7F}");
    };

    // For the SystemAllocator we inherit so we have a different stack. The SystemAllocator is used globally so we dont want
    // to get that data affecting the benchmark
    class TestSystemAllocator : public AZ::SystemAllocator
    {
    public:
        AZ_RTTI(TestSystemAllocator, "{360D4DAA-D65D-4D5C-A6FA-1A4C5261C35C}", AZ::SystemAllocator);

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
        using TestAllocatorType = TAllocator;

        virtual void internalSetUp(const ::benchmark::State& state)
        {
            m_allocator = AZStd::make_unique<TestAllocatorType>();
            if (state.thread_index() == 0)
            {
                m_allocations.resize(state.threads());
                for (auto& perThreadAllocations : m_allocations)
                {
                    perThreadAllocations.resize(state.range(0), nullptr);
                }
            }
        }

        virtual void internalTearDown(const ::benchmark::State& state)
        {
            if (state.thread_index() == 0) // Only setup in the first thread
            {
                m_allocations.clear();
                m_allocations.shrink_to_fit();
            }
            m_allocator = nullptr;
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

        const TestAllocatorType& GetAllocator() const { return *m_allocator; }
        TestAllocatorType& GetAllocator() { return *m_allocator; }

    private:
        AZStd::unique_ptr<TestAllocatorType> m_allocator;
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

                AZStd::vector<void*>& perThreadAllocations = base::GetPerThreadAllocations(state.thread_index());
                const size_t numberOfAllocations = perThreadAllocations.size();
                size_t totalAllocationSize = 0;
                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    totalAllocationSize += allocationSize;

                    state.ResumeTiming();
                    perThreadAllocations[allocationIndex] = this->GetAllocator().allocate(allocationSize, 0);
                    state.PauseTiming();
                }

                state.counters[s_counterAllocatorMemory] = benchmark::Counter(static_cast<double>(this->GetAllocator().NumAllocatedBytes()), benchmark::Counter::kDefaults);
                state.counters[s_counterBenchmarkMemory] = benchmark::Counter(static_cast<double>(totalAllocationSize), benchmark::Counter::kDefaults);

                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    this->GetAllocator().deallocate(perThreadAllocations[allocationIndex], allocationSize);
                    perThreadAllocations[allocationIndex] = nullptr;
                }
                this->GetAllocator().GarbageCollect();

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
                AZStd::vector<void*>& perThreadAllocations = base::GetPerThreadAllocations(state.thread_index());

                const size_t numberOfAllocations = perThreadAllocations.size();
                size_t totalAllocationSize = 0;
                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    totalAllocationSize += allocationSize;
                    perThreadAllocations[allocationIndex] = this->GetAllocator().allocate(allocationSize, 0);
                }

                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    state.ResumeTiming();
                    this->GetAllocator().deallocate(perThreadAllocations[allocationIndex], allocationSize);
                    state.PauseTiming();
                    perThreadAllocations[allocationIndex] = nullptr;
                }

                state.counters[s_counterAllocatorMemory] = benchmark::Counter(static_cast<double>(this->GetAllocator().NumAllocatedBytes()), benchmark::Counter::kDefaults);
                state.counters[s_counterBenchmarkMemory] = benchmark::Counter(static_cast<double>(totalAllocationSize), benchmark::Counter::kDefaults);

                state.SetItemsProcessed(numberOfAllocations);

                this->GetAllocator().GarbageCollect();

                state.ResumeTiming();
            }
        }
    };

    template<typename TAllocator>
    class RecordedAllocationBenchmarkFixture : public ::benchmark::Fixture
    {
        using TestAllocatorType = TAllocator;

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

        void InternalSetUp([[maybe_unused]] const ::benchmark::State& state)
        {
            m_allocator = AZStd::make_unique<TestAllocatorType>();
        }

        void InternalTearDown([[maybe_unused]] const ::benchmark::State& state)
        {
            m_allocator = nullptr;
        }
    public:
        void SetUp(const ::benchmark::State& state) override
        {
            InternalSetUp(state);
        }
        void SetUp(::benchmark::State& state) override
        {
            InternalSetUp(state);
        }

        void TearDown(const ::benchmark::State& state) override
        {
            InternalTearDown(state);
        }
        void TearDown(::benchmark::State& state) override
        {
            InternalTearDown(state);
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
                                    void* ptr = this->GetAllocator().allocate(operation.m_size, operation.m_alignment);
                                    state.PauseTiming();
                                    totalAllocationSize += operation.m_size;
                                    it.first->second = ptr;
                                }
                                else
                                {
                                    // Doing a resize, dont account for this memory change, this operation is rare and we dont have
                                    // the size of the previous allocation
                                    state.ResumeTiming();
                                    this->GetAllocator().reallocate(it.first->second, operation.m_size, operation.m_alignment);
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
                                        this->GetAllocator().deallocate(
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
                                    this->GetAllocator().deallocate(nullptr, /*operation.m_size*/ 0);
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
                        this->GetAllocator().deallocate(pointerMapping.second);
                        state.PauseTiming();
                    }
                    itemsProcessed += pointerRemapping.size();
                    pointerRemapping.clear();
                }

                state.counters[s_counterAllocatorMemory] = benchmark::Counter(static_cast<double>(this->GetAllocator().NumAllocatedBytes()), benchmark::Counter::kDefaults);
                state.counters[s_counterBenchmarkMemory] = benchmark::Counter(static_cast<double>(totalAllocationSize), benchmark::Counter::kDefaults);

                state.SetItemsProcessed(itemsProcessed);

                this->GetAllocator().GarbageCollect();

                state.ResumeTiming();
            }
        }
    private:
        AZStd::unique_ptr<TestAllocatorType> m_allocator;
        TestAllocatorType& GetAllocator() { return *m_allocator; }
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
    BM_REGISTER_ALLOCATOR(HphaSchemaAllocator, HphaSchemaAllocator);
    BM_REGISTER_ALLOCATOR(SystemAllocator, TestSystemAllocator);

    //BM_REGISTER_SCHEMA(PoolSchema); // Requires special alignment requests while allocating
    // BM_REGISTER_ALLOCATOR(OSAllocator, OSAllocator); // Requires special treatment to initialize since it will be already initialized, maybe creating a different instance?

#undef BM_REGISTER_ALLOCATOR
#undef BM_REGISTER_SIZE_FIXTURES
#undef BM_REGISTER_TEMPLATE

} // Benchmark

#endif // HAVE_BENCHMARK
