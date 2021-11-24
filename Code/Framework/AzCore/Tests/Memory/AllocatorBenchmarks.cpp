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

    static AZ::Debug::DrillerManager* s_drillerManager = nullptr;

    template<typename TAllocator>
    class TestAllocator : public TAllocator
    {
    public:
        TestAllocator()
            : TAllocator()
        {
        }

        static void SetUp()
        {
            AZ::AllocatorInstance<TAllocator>::Create();

            s_drillerManager = AZ::Debug::DrillerManager::Create();
            s_drillerManager->Register(aznew AZ::Debug::MemoryDriller);
        }

        static void TearDown()
        {
            AZ::Debug::DrillerManager::Destroy(s_drillerManager);
            s_drillerManager = nullptr;

            AZ::AllocatorInstance<TAllocator>::Destroy();
        }

        typename TAllocator::pointer_type Allocate(
            typename TAllocator::size_type byteSize,
            typename TAllocator::size_type alignment,
            int = 0,
            const char* = nullptr,
            const char* = nullptr,
            int = 0,
            unsigned int = 0) override
        {
            return AZ::AllocatorInstance<TAllocator>::Get().Allocate(byteSize, alignment);
        }

        void DeAllocate(
            typename TAllocator::pointer_type ptr,
            typename TAllocator::size_type byteSize = 0,
            typename TAllocator::size_type = 0) override
        {
            AZ::AllocatorInstance<TAllocator>::Get().DeAllocate(ptr, byteSize);
        }

        typename TAllocator::pointer_type ReAllocate(
            typename TAllocator::pointer_type ptr,
            typename TAllocator::size_type newSize,
            typename TAllocator::size_type newAlignment) override
        {
            return AZ::AllocatorInstance<TAllocator>::Get().ReAllocate(ptr, newSize, newAlignment);
        }

        typename TAllocator::size_type Resize(typename TAllocator::pointer_type ptr, typename TAllocator::size_type newSize) override
        {
            return AZ::AllocatorInstance<TAllocator>::Get().Resize(ptr, newSize);
        }

        void GarbageCollect() override
        {
            AZ::AllocatorInstance<TAllocator>::Get().GarbageCollect();
        }

        typename TAllocator::size_type NumAllocatedBytes() const override
        {
            return AZ::AllocatorInstance<TAllocator>::Get().NumAllocatedBytes() +
                AZ::AllocatorInstance<TAllocator>::Get().GetUnAllocatedMemory();
        }
    };
}

namespace AZ
{
    AZ_TYPE_INFO_TEMPLATE(Benchmark::TestAllocator, "{ACE2D6E5-4EB8-4DD2-AE95-6BDFD0476801}", AZ_TYPE_INFO_CLASS);
}

namespace Benchmark
{
    class TestRawMallocAllocator {};

    template <>
    class TestAllocator<TestRawMallocAllocator>
        : public TestRawMallocAllocator
    {
    public:
        struct Descriptor {};

        TestAllocator()
        {}

        static void SetUp()
        {
            s_numAllocatedBytes = 0;
        }

        static void TearDown()
        {}

        void* Allocate(
            size_t byteSize,
            size_t alignment,
            int = 0,
            const char* = nullptr,
            const char* = nullptr,
            int = 0,
            unsigned int = 0)
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

        static void GarbageCollect()
        {}

        static size_t NumAllocatedBytes()
        {
            return s_numAllocatedBytes;
        }

    private:
        static size_t s_numAllocatedBytes;
    };

    size_t TestAllocator<TestRawMallocAllocator>::s_numAllocatedBytes = 0;

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

        TestSystemAllocator()
            : AZ::SystemAllocator()
        {}
    };
}

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Benchmark::TestAllocator<Benchmark::TestRawMallocAllocator>,    "{1065B446-4873-4B3E-9CB1-069E148D4DF6}");
    AZ_TYPE_INFO_SPECIALIZE(Benchmark::TestAllocator<Benchmark::TestMallocSchemaAllocator>, "{92CFDF86-02EE-4247-9809-884EE9F7BA18}");
    AZ_TYPE_INFO_SPECIALIZE(Benchmark::TestAllocator<Benchmark::TestHeapSchemaAllocator>,   "{67DA01DF-9232-493A-B11C-6952FEDEB2A9}");
    AZ_TYPE_INFO_SPECIALIZE(Benchmark::TestAllocator<Benchmark::TestHphaSchemaAllocator>,   "{47384CB4-6729-43A9-B0CE-402E3A7AEFB2}");
    AZ_TYPE_INFO_SPECIALIZE(Benchmark::TestAllocator<Benchmark::TestSystemAllocator>,       "{096423BC-DC36-48D2-8E89-8F1D600F488A}");
}

namespace Benchmark
{
    // Allocated bytes reported by the allocator / actually requested bytes
    static const char* s_counterAllocatorMemoryRatio = "Allocator_MemoryRatio";

    // Allocated bytes reported by the process / actually requested bytes
    static const char* s_counterProcessMemoryRatio = "Process_MemoryRatio";

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
        using TestAllocatorType = TestAllocator<TAllocator>;

        virtual void internalSetUp(const ::benchmark::State&)
        {
            TestAllocatorType::SetUp();
        }

        virtual void internalTearDown(const ::benchmark::State&)
        {
            TestAllocatorType::TearDown();
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
    };

    template <typename TAllocator, AllocationSize TAllocationSize>
    class AllocationBenchmarkFixture
        : public AllocatorBenchmarkFixture<TAllocator>
    {
        using TestAllocatorType = AllocatorBenchmarkFixture<TAllocator>::TestAllocatorType;

        void internalSetUp(const ::benchmark::State& state) override
        {
            AllocatorBenchmarkFixture<TAllocator>::SetUp(state);

            m_allocations.resize(state.range_x(), nullptr);
        }

        void internalTearDown(const ::benchmark::State& state) override
        {
            m_allocations.clear();
            m_allocations.shrink_to_fit();

            AllocatorBenchmarkFixture<TAllocator>::TearDown(state);
        }

    public:
        void Benchmark(benchmark::State& state)
        {
            TestAllocatorType allocatorType;

            for (auto _ : state)
            {
                const size_t processMemoryBaseline = Platform::GetProcessMemoryUsageBytes();

                const size_t numberOfAllocations = m_allocations.size();
                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    state.PauseTiming();
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    
                    state.ResumeTiming();
                    m_allocations[allocationIndex] = allocatorType.Allocate(allocationSize, 0);
                }

                state.PauseTiming();
                state.counters[s_counterAllocatorMemoryRatio] =
                    benchmark::Counter(static_cast<double>(allocatorType.NumAllocatedBytes()), benchmark::Counter::kDefaults);
                state.counters[s_counterProcessMemoryRatio] = benchmark::Counter(
                    static_cast<double>(Platform::GetProcessMemoryUsageBytes() - processMemoryBaseline),
                    benchmark::Counter::kDefaults);

                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    allocatorType.DeAllocate(m_allocations[allocationIndex], allocationSize);
                    m_allocations[allocationIndex] = nullptr;
                }
                allocatorType.GarbageCollect();

                state.SetItemsProcessed(numberOfAllocations);
                state.ResumeTiming();
            }
        }

    private:
        AZStd::vector<void*> m_allocations;
    };

    template <typename TAllocator, AllocationSize TAllocationSize>
    class DeAllocationBenchmarkFixture
        : public AllocatorBenchmarkFixture<TAllocator>
    {
        using TestAllocatorType = AllocatorBenchmarkFixture<TAllocator>::TestAllocatorType;

        void internalSetUp(const ::benchmark::State& state) override
        {
            AllocatorBenchmarkFixture<TAllocator>::SetUp(state);

            m_allocations.resize(state.range_x(), nullptr);
        }

        void internalTearDown(const ::benchmark::State& state) override
        {
            m_allocations.clear();
            m_allocations.shrink_to_fit();

            AllocatorBenchmarkFixture<TAllocator>::TearDown(state);
        }
    public:
        void Benchmark(benchmark::State& state)
        {
            TestAllocatorType allocatorType;

            for (auto _ : state)
            {
                state.PauseTiming();
                const size_t processMemoryBaseline = Platform::GetProcessMemoryUsageBytes();

                const size_t numberOfAllocations = m_allocations.size();
                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    m_allocations[allocationIndex] = allocatorType.Allocate(allocationSize, 0);
                }

                for (size_t allocationIndex = 0; allocationIndex < numberOfAllocations; ++allocationIndex)
                {
                    const AllocationSizeArray& allocationArray = s_allocationSizes[TAllocationSize];
                    const size_t allocationSize = allocationArray[allocationIndex % allocationArray.size()];
                    state.ResumeTiming();
                    allocatorType.DeAllocate(m_allocations[allocationIndex], allocationSize);
                    state.PauseTiming();
                    m_allocations[allocationIndex] = nullptr;
                }

                state.counters[s_counterAllocatorMemoryRatio] =
                    benchmark::Counter(static_cast<double>(allocatorType.NumAllocatedBytes()), benchmark::Counter::kDefaults);
                state.counters[s_counterProcessMemoryRatio] = benchmark::Counter(
                    static_cast<double>(Platform::GetProcessMemoryUsageBytes() - processMemoryBaseline),
                    benchmark::Counter::kDefaults);

                state.SetItemsProcessed(numberOfAllocations);

                allocatorType.GarbageCollect();

                state.ResumeTiming();
            }
        }

    private:
        AZStd::vector<void*> m_allocations;
    };

    static void RunRanges(benchmark::internal::Benchmark* b)
    {
        for (int i = 0; i < 6; ++i)
        {
            b->Arg((1 << i) * 1000);
        }
    }

#define BM_REGISTER_TEMPLATE(FIXTURE, TESTNAME, ...) \
        BENCHMARK_TEMPLATE_DEFINE_F(FIXTURE, TESTNAME, __VA_ARGS__)(benchmark::State& state) { Benchmark(state); } \
        BENCHMARK_REGISTER_F(FIXTURE, TESTNAME)

#define BM_REGISTER_SIZE_FIXTURES(FIXTURE, TESTNAME, ALLOCATORTYPE) \
    BM_REGISTER_TEMPLATE(FIXTURE, TESTNAME##_SMALL, ALLOCATORTYPE, SMALL)->Apply(RunRanges); \
    BM_REGISTER_TEMPLATE(FIXTURE, TESTNAME##_BIG, ALLOCATORTYPE, BIG)->Apply(RunRanges); \
    BM_REGISTER_TEMPLATE(FIXTURE, TESTNAME##_MIXED, ALLOCATORTYPE, MIXED)->Apply(RunRanges);

#define BM_REGISTER_ALLOCATOR(TESTNAME, ALLOCATORTYPE) \
    namespace TESTNAME \
    { \
        BM_REGISTER_SIZE_FIXTURES(AllocationBenchmarkFixture, TESTNAME, ALLOCATORTYPE) \
        BM_REGISTER_SIZE_FIXTURES(DeAllocationBenchmarkFixture, TESTNAME, ALLOCATORTYPE) \
    }

    BM_REGISTER_ALLOCATOR(RawMallocAllocator, TestRawMallocAllocator);
    BM_REGISTER_ALLOCATOR(MallocSchemaAllocator, TestMallocSchemaAllocator);
    BM_REGISTER_ALLOCATOR(HeapSchemaAllocator, TestHeapSchemaAllocator);
    BM_REGISTER_ALLOCATOR(HphaSchemaAllocator, TestHphaSchemaAllocator);
    BM_REGISTER_ALLOCATOR(SystemAllocator, TestSystemAllocator);

    //BM_REGISTER_SCHEMA(BestFitExternalMapSchema); // Requires to implement AZ::IAllocatorAllocate
    //BM_REGISTER_SCHEMA(PoolSchema); // Requires special alignment requests while allocating

#undef BM_REGISTER_ALLOCATOR
#undef BM_REGISTER_SIZE_FIXTURES
#undef BM_REGISTER_TEMPLATE

} // Benchmark

#endif // HAVE_BENCHMARK
