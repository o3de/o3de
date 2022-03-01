/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZCORE_UNITTEST_USERTYPES_H
#define AZCORE_UNITTEST_USERTYPES_H

#include <AzCore/base.h>
#include <AzCore/UnitTest/UnitTest.h>

#include <AzCore/Debug/BudgetTracker.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>

#if defined(HAVE_BENCHMARK)

AZ_PUSH_DISABLE_WARNING(, "-Wdeprecated-declarations", "-Wdeprecated-declarations")
#include <benchmark/benchmark.h>
AZ_POP_DISABLE_WARNING

#endif // HAVE_BENCHMARK

namespace UnitTest
{
    /**
    * Base class to share common allocator code between fixture types.
    */
    class AllocatorsBase
    {
        bool m_ownsAllocator{};
    public:

        virtual ~AllocatorsBase() = default;

        void SetupAllocator(const AZ::SystemAllocator::Descriptor& allocatorDesc = {})
        {
            AZ::AllocatorManager::Instance().EnterProfilingMode();
            AZ::AllocatorManager::Instance().SetDefaultTrackingMode(AZ::Debug::AllocationRecords::RECORD_FULL);

            // Only create the SystemAllocator if it s not ready
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create(allocatorDesc);
                m_ownsAllocator = true;
            }
        }

        void TeardownAllocator()
        {
            // Don't destroy the SystemAllocator if it is not ready aand was not created by
            // the AllocatorsBase
            if (m_ownsAllocator && AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
            m_ownsAllocator = false;

            AZ::AllocatorManager::Instance().SetDefaultTrackingMode(AZ::Debug::AllocationRecords::RECORD_NO_RECORDS);
            AZ::AllocatorManager::Instance().ExitProfilingMode();
        }
    };

    /**
     * RAII wrapper of AllocatorBase.
     * The benefit of using this wrapper instead of AllocatorsTestFixture is that SetUp/TearDown of the allocator is managed
     * on construction/destruction, allowing member variables of derived classes to exist as value (and do heap allocation).
     */
    class ScopedAllocatorFixture : AllocatorsBase
    {
    public:
        ScopedAllocatorFixture()
        {
            SetupAllocator();
        }
        explicit ScopedAllocatorFixture(const AZ::SystemAllocator::Descriptor& allocatorDesc)
        {
            SetupAllocator(allocatorDesc);
        }
        ~ScopedAllocatorFixture() override
        {
            TeardownAllocator();
        }
    };

    // Like ScopedAllocatorFixture, but includes the Test base class
    class ScopedAllocatorSetupFixture
        : public ::testing::Test
        , public ScopedAllocatorFixture
    {
    public:
        ScopedAllocatorSetupFixture() = default;
        explicit ScopedAllocatorSetupFixture(const AZ::SystemAllocator::Descriptor& allocatorDesc) : ScopedAllocatorFixture(allocatorDesc){}
    };

    /**
    * Helper class to handle the boiler plate of setting up a test fixture that uses the system allocators
    * If you wish to do additional setup and tear down be sure to call the base class SetUp first and TearDown
    * last.
    * By default memory tracking is enabled.
    */

    class AllocatorsTestFixture
        : public ::testing::Test
        , public AllocatorsBase
    {
    public:
        //GTest interface
        void SetUp() override
        {
            SetupAllocator();
        }

        void TearDown() override
        {
            TeardownAllocator();
        }
    };

    //Legacy alias to avoid needing to modify tons of files
    //-- Do not use for new tests.
    using AllocatorsFixture = AllocatorsTestFixture;

#if defined(HAVE_BENCHMARK)

    /**
    * Helper class to handle the boiler plate of setting up a benchmark fixture that uses the system allocators
    * If you wish to do additional setup and tear down be sure to call the base class SetUp first and TearDown
    * last.
    * By default memory tracking is enabled.
    */
    class AllocatorsBenchmarkFixture
        : public ::benchmark::Fixture
        , public AllocatorsBase
    {
    public:
        //Benchmark interface
        void SetUp(const ::benchmark::State& st) override
        {
            AZ_UNUSED(st);
            SetupAllocator();
        }
        void SetUp(::benchmark::State& st) override
        {
            AZ_UNUSED(st);
            SetupAllocator();
        }

        void TearDown(const ::benchmark::State& st) override
        {
            AZ_UNUSED(st);
            TeardownAllocator();
        }
        void TearDown(::benchmark::State& st) override
        {
            AZ_UNUSED(st);
            TeardownAllocator();
        }
    };

    /**
    * RAII wrapper around a BenchmarkEnvironmentBase to encapsulate
    * the creation and destuction of the SystemAllocator
    * SetUpBenchmark and TearDownBenchmark can still be used for custom
    * benchmark environment setup
    */
    class ScopedAllocatorBenchmarkEnvironment
        : public AZ::Test::BenchmarkEnvironmentBase
    {
    public:
        ScopedAllocatorBenchmarkEnvironment()
        {
            // Only create the allocator if it doesn't exist
            if (!AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
                m_ownsAllocator = true;
            }
        }
        ~ScopedAllocatorBenchmarkEnvironment() override
        {
            if (m_ownsAllocator)
            {
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
        }
    private:
        bool m_ownsAllocator{};
    };
#endif

    class DLLTestVirtualClass
    {
    public:
        DLLTestVirtualClass()
            : m_data(1) {}
        virtual ~DLLTestVirtualClass() {}

        int m_data;
    };

    template <AZ::u32 size, AZ::u8 instance, size_t alignment = 16>
    struct CreationCounter
    {
        AZ_TYPE_INFO(CreationCounter, "{E9E35486-4366-4066-86E5-1A8CEB44198B}");
        alignas(alignment) int test[size / sizeof(int)];

        static int s_count;
        static int s_copied;
        static int s_moved;
        CreationCounter(int def = 0)
        {
            ++s_count;
            test[0] = def;
        }

        CreationCounter(AZStd::initializer_list<int> il)
        {
            ++s_count;
            if (il.size() > 0)
            {
                test[0] = *il.begin();
            }
        }
        CreationCounter(const CreationCounter& rhs)
            : CreationCounter()
        {
            memcpy(test, rhs.test, AZ_ARRAY_SIZE(test));
            ++s_copied;
        }
        CreationCounter(CreationCounter&& rhs)
            : CreationCounter()
        {
            memmove(test, rhs.test, AZ_ARRAY_SIZE(test));
            ++s_moved;
        }

        ~CreationCounter()
        {
            --s_count;
        }

        const int& val() const { return test[0]; }
        int& val() { return test[0]; }

        static void Reset()
        {
            s_count = 0;
            s_copied = 0;
            s_moved = 0;
        }
    private:
        // The test member variable has configurable alignment and this class has configurable size
        // In some cases (when the size < alignment) padding is required. To avoid the compiler warning
        // us about the padding being added, we added it here for the cases that is required.
        // '% alignment' in the padding is redundant, but it fixes an 'array is too large' clang error.
        static constexpr bool sHasPadding = size < alignment;
        AZStd::enable_if<sHasPadding, char[(alignment - size) % alignment]> mPadding;
    };

    template <AZ::u32 size, AZ::u8 instance, size_t alignment>
    int CreationCounter<size, instance, alignment>::s_count = 0;
    template <AZ::u32 size, AZ::u8 instance, size_t alignment>
    int CreationCounter<size, instance, alignment>::s_copied = 0;
    template <AZ::u32 size, AZ::u8 instance, size_t alignment>
    int CreationCounter<size, instance, alignment>::s_moved = 0;
}

#endif // AZCORE_UNITTEST_USERTYPES_H
