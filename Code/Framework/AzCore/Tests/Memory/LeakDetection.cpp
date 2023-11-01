/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/IAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/ChildAllocatorSchema.h>


namespace UnitTest
{
    static void CheckAllocatorsForLeaks(bool leakExpected)
    {
        auto& allocatorManager = AZ::AllocatorManager::Instance();
        const int numAllocators = allocatorManager.GetNumAllocators();
        bool anyAllocatorHasAllocations = false;
        allocatorManager.GarbageCollect();
        for (int i = numAllocators - 1; i >= 0; --i)
        {
            auto* allocator = allocatorManager.GetAllocator(i);
            const AZ::Debug::AllocationRecords* records = allocator->GetRecords();
            if (records)
            {
                const bool hasAllocations = records->RequestedBytes() != 0;
                anyAllocatorHasAllocations = anyAllocatorHasAllocations || hasAllocations;
            }
        }
        EXPECT_EQ(leakExpected, anyAllocatorHasAllocations);
        if (leakExpected != anyAllocatorHasAllocations)
        {
            for (int i = numAllocators - 1; i >= 0; --i)
            {
                auto* allocator = allocatorManager.GetAllocator(i);
                AZ::Debug::AllocationRecords* records = allocator->GetRecords();
                if (records)
                {
                    records->EnumerateAllocations(AZ::Debug::PrintAllocationsCB{ true, true });
                }
            }
        }
    }

    // Dummy test class
    class TestClass
    {
    public:
        AZ_CLASS_ALLOCATOR(TestClass, AZ::SystemAllocator);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing the LeakDetectionFixture base class. Testing that detects leaks
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class AllocatorsTestFixtureLeakDetectionTest
        : public LeakDetectionFixture
    {
    public:
        void TearDown() override
        {
            UnitTest::CheckAllocatorsForLeaks(m_leakExpected);

            if (m_leakyObject)
            {
                delete m_leakyObject;
                m_leakyObject = nullptr;
                AZ::AllocatorManager::Instance().GarbageCollect();
            }

            LeakDetectionFixture::TearDown();
        }

        void SetLeakExpected() { AZ_TEST_START_TRACE_SUPPRESSION; m_leakExpected = true; }

    protected:
        TestClass* m_leakyObject{};
    private:
        bool m_leakExpected = false;
    };

#if AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    TEST_F(AllocatorsTestFixtureLeakDetectionTest, DISABLED_Leak)
#else
    TEST_F(AllocatorsTestFixtureLeakDetectionTest, Leak)
#endif // AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    {
        SetLeakExpected();

        m_leakyObject = aznew TestClass();
    }

    TEST_F(AllocatorsTestFixtureLeakDetectionTest, NoLeak)
    {
        TestClass* leakyObject = aznew TestClass();
        delete leakyObject;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing Allocator leaks
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Create a dummy allocator so unit tests can leak it
    AZ_CHILD_ALLOCATOR_WITH_NAME(LeakDetection_TestAllocator, "LeakDetection_TestAllocator", "{186B6E32-344D-4322-820A-4C3E4F30650B}", AZ::SystemAllocator);

    // Dummy test class
    class TestClassLeakDetection_TestAllocator
    {
    public:
        AZ_CLASS_ALLOCATOR(TestClass, LeakDetection_TestAllocator);
    };


    class AllocatorsTestFixtureLeakDetectionDeathTest_SKIPCODECOVERAGE
        : public ::testing::Test
    {
    public:
        void TestAllocatorLeak()
        {
            AZ::AllocatorManager::Instance().EnterProfilingMode();
            AZ::AllocatorManager::Instance().SetDefaultProfilingState(true);
            AZ::AllocatorManager::Instance().SetTrackingMode(AZ::Debug::AllocationRecords::Mode::RECORD_FULL);
            AZ::AllocatorManager::Instance().SetDefaultTrackingMode(AZ::Debug::AllocationRecords::Mode::RECORD_FULL);
            [[maybe_unused]] TestClassLeakDetection_TestAllocator* object = new TestClassLeakDetection_TestAllocator();

            // In regular unit test operation, the environment will be teardown at the end and thats where the validation will happen. Here, we need
            // to do a teardown before the test ends so gtest detects the death before it starts to teardown.
            // We suppress the traces so they dont produce more abort calls that would cause the debugger to break (i.e. to stop at a breakpoint). Since
            // this is part of a death test, the trace suppression wont leak because death tests are executed in their own process space.
            AZ_TEST_START_TRACE_SUPPRESSION;
            TraceBusHook* traceBusHook = static_cast<TraceBusHook*>(AZ::Test::sTestEnvironment);
            traceBusHook->TeardownEnvironment();
        }
    };

#if GTEST_HAS_DEATH_TEST
#if AZ_TRAIT_DISABLE_FAILED_DEATH_TESTS
    TEST_F(AllocatorsTestFixtureLeakDetectionDeathTest_SKIPCODECOVERAGE, DISABLED_AllocatorLeak)
#else
    TEST_F(AllocatorsTestFixtureLeakDetectionDeathTest_SKIPCODECOVERAGE, AllocatorLeak)
#endif
    {
        // testing that the TraceBusHook will fail on cause the test to die
        EXPECT_DEATH(TestAllocatorLeak(), "");
    }
#endif // GTEST_HAS_DEATH_TEST

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing LeakDetectionFixture. Testing that detects leaks
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class AllocatorSetupLeakDetectionTest
        : public LeakDetectionFixture
    {
    public:
        ~AllocatorSetupLeakDetectionTest() override
        {
            UnitTest::CheckAllocatorsForLeaks(m_leakExpected);

            if (m_leakyObject)
            {
                delete m_leakyObject;
                m_leakyObject = nullptr;
                AZ::AllocatorManager::Instance().GarbageCollect();
            }
        }

        void SetLeakExpected() { m_leakExpected = true; }

    protected:
        TestClass* m_leakyObject = nullptr;
    private:
        bool m_leakExpected = false;
    };

#if AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    TEST_F(AllocatorSetupLeakDetectionTest, DISABLED_Leak)
#else
    TEST_F(AllocatorSetupLeakDetectionTest, Leak)
#endif // AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    {
        SetLeakExpected();

        m_leakyObject = aznew TestClass();
    }

    TEST_F(AllocatorSetupLeakDetectionTest, NoLeak)
    {
        TestClass* leakyObject = aznew TestClass();
        delete leakyObject;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing Allocators, testing that the different allocator types detect leaks
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename AllocatorType>
    class AllocatorTypeLeakDetectionTest
        : public ::testing::Test
    {
    public:
        void SetUp() override
        {
            AZ::AllocatorManager::Instance().EnterProfilingMode();
            AZ::AllocatorManager::Instance().SetDefaultProfilingState(true);
            AZ::AllocatorManager::Instance().SetDefaultTrackingMode(AZ::Debug::AllocationRecords::Mode::RECORD_FULL);
            AZ::AllocatorManager::Instance().SetTrackingMode(AZ::Debug::AllocationRecords::Mode::RECORD_FULL);
        }

        void TearDown() override
        {
            UnitTest::CheckAllocatorsForLeaks(m_leakExpected);

            if (m_leakyObject)
            {
                delete m_leakyObject;
                m_leakyObject = nullptr;
                AZ::AllocatorManager::Instance().GarbageCollect();
            }

            AZ::AllocatorManager::Instance().SetTrackingMode(AZ::Debug::AllocationRecords::Mode::RECORD_NO_RECORDS);
            AZ::AllocatorManager::Instance().SetDefaultTrackingMode(AZ::Debug::AllocationRecords::Mode::RECORD_NO_RECORDS);
            AZ::AllocatorManager::Instance().SetDefaultProfilingState(false);
            AZ::AllocatorManager::Instance().ExitProfilingMode();
        }

        void SetLeakExpected() { m_leakExpected = true; }

        // Dummy test class that uses AllocatorType
        class ThisAllocatorTestClass
        {
        public:
            AZ_CLASS_ALLOCATOR(ThisAllocatorTestClass, AllocatorType);
        };

    protected:
        ThisAllocatorTestClass* m_leakyObject = nullptr;
    private:
        bool m_leakExpected = false;
    };

    using AllocatorTypes = ::testing::Types<
        AZ::SystemAllocator,
        AZ::PoolAllocator,
        AZ::ThreadPoolAllocator
    >;
    TYPED_TEST_CASE(AllocatorTypeLeakDetectionTest, AllocatorTypes);

#if AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    TYPED_TEST(AllocatorTypeLeakDetectionTest, DISABLED_Leak)
#else
    TYPED_TEST(AllocatorTypeLeakDetectionTest, Leak)
#endif // AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    {
        TestFixture::SetLeakExpected();

        this->m_leakyObject = aznew typename TestFixture::ThisAllocatorTestClass();
    }

    TYPED_TEST(AllocatorTypeLeakDetectionTest, NoLeak)
    {
        auto* leakyObject = aznew typename TestFixture::ThisAllocatorTestClass();
        delete leakyObject;
    }
}


