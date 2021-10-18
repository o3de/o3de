/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/BestFitExternalMapAllocator.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/UnitTest/TestTypes.h>


namespace UnitTest
{
    // Dummy test class
    class TestClass
    {
    public:
        AZ_CLASS_ALLOCATOR(TestClass, AZ::SystemAllocator, 0);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing the AllocatorsTestFixture base class. Testing that detects leaks
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class AllocatorsTestFixtureLeakDetectionTest
        : public AllocatorsTestFixture
        , public UnitTest::TraceBusRedirector
    {
    public:
        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }
        void TearDown() override
        {
            AllocatorsTestFixture::TearDown();

            EXPECT_EQ(m_leakExpected, m_leakDetected);
            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

            if (m_leakExpected)
            {
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            }
        }

        void SetLeakExpected() { AZ_TEST_START_TRACE_SUPPRESSION; m_leakExpected = true; }

    private:
        bool OnPreError(const char* window, const char* file, int line, const char* func, const char* message) override
        {
            AZ_UNUSED(file);
            AZ_UNUSED(line);
            AZ_UNUSED(func);

            if (AZStd::string_view(window) == "Memory"
                && AZStd::string_view(message) == "We still have 1 allocations on record! They must be freed prior to destroy!")
            {
                // Leak detected, flag it so we validate on tear down that this happened. We also will 
                // mark this test since it will assert
                m_leakDetected = true;
                return true;
            }
            return false;
        }

        bool OnPrintf(const char* window, const char* message) override
        {
            AZ_UNUSED(window);
            AZ_UNUSED(message);
            // Do not print the error message twice. The error message will already be printed by the TraceBusRedirector
            // in UnitTest.h. Here we override it to prevent it from printing twice.
            return true;
        }

        bool m_leakDetected = false;
        bool m_leakExpected = false;
    };

#if AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    TEST_F(AllocatorsTestFixtureLeakDetectionTest, DISABLED_Leak)
#else
    TEST_F(AllocatorsTestFixtureLeakDetectionTest, Leak)
#endif // AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    {
        SetLeakExpected();

        TestClass* leakyObject = aznew TestClass();
        AZ_UNUSED(leakyObject);
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
    class LeakDetection_TestAllocator
        : public AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>
    {
    public:
        AZ_TYPE_INFO(LeakDetection_TestAllocator, "{186B6E32-344D-4322-820A-4C3E4F30650B}");

        using Base = AZ::SimpleSchemaAllocator<AZ::ChildAllocatorSchema<AZ::SystemAllocator>>;
        using Descriptor = Base::Descriptor;

        LeakDetection_TestAllocator()
            : LeakDetection_TestAllocator(TYPEINFO_Name(), "LeakDetection_TestAllocator")
        {
        }

        LeakDetection_TestAllocator(const char* name, const char* desc)
            : Base(name, desc)
        {
            Create();
        }

        ~LeakDetection_TestAllocator() override = default;
    };

    class AllocatorsTestFixtureLeakDetectionDeathTest_SKIPCODECOVERAGE
        : public ::testing::Test
    {
    public:
        void TestAllocatorLeak()
        {
            AZ::AllocatorInstance<LeakDetection_TestAllocator>::Create();

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
    TEST_F(AllocatorsTestFixtureLeakDetectionDeathTest_SKIPCODECOVERAGE, AllocatorLeak)
    {
        // testing that the TraceBusHook will fail on cause the test to die
        EXPECT_DEATH(TestAllocatorLeak(), "");
    }
#endif // GTEST_HAS_DEATH_TEST

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing ScopedAllocatorSetupFixture. Testing that detects leaks
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class AllocatorSetupLeakDetectionTest
        : public ::testing::Test
    {
        // Internal class to manage the bus redirector so we can use construction/destruction order to detect leaks triggered
        // by AllocatorSetup destructor.
        class BusRedirector
            : public UnitTest::TraceBusRedirector
        {
        public:
            BusRedirector()
            {
                AZ::Debug::TraceMessageBus::Handler::BusConnect();
            }
            ~BusRedirector() override
            {
                EXPECT_EQ(m_leakExpected, m_leakDetected);
                AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

                if (m_leakExpected)
                {
                    // The macro AZ_TEST_STOP_TRACE_SUPPRESSION contains a return statement, therefore we cannot use it in a destructor, 
                    // to overcome that, we wrap it in a lambda so we can drop the returned value.
                    auto stopAsserts = [] {
                        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
                    };
                    stopAsserts();
                }
            }

            bool OnPreError(const char* window, const char* file, int line, const char* func, const char* message) override
            {
                AZ_UNUSED(file);
                AZ_UNUSED(line);
                AZ_UNUSED(func);

                if (AZStd::string_view(window) == "Memory"
                    && AZStd::string_view(message) == "We still have 1 allocations on record! They must be freed prior to destroy!")
                {
                    // Leak detected, flag it so we validate on tear down that this happened. We also will 
                    // mark this test since it will assert
                    m_leakDetected = true;
                    return true;
                }
                return false;
            }

            bool OnPrintf(const char* window, const char* message) override
            {
                AZ_UNUSED(window);
                AZ_UNUSED(message);
                // Do not print the error message twice. The error message will already be printed by the TraceBusRedirector
                // in UnitTest.h. Here we override it to prevent it from printing twice.
                return true;
            }

            bool m_leakDetected = false;
            bool m_leakExpected = false;
        };

        // Inheriting to add default implementations for the virtual abstract methods.
        class AllocatorSetup : public ScopedAllocatorSetupFixture
        {
        public:
            void SetUp() override {}
            void TearDown() override {}
            void TestBody() override {}
        };

    public:
        ~AllocatorSetupLeakDetectionTest() override
        {
            EXPECT_EQ(m_busRedirector.m_leakExpected, UnitTest::TestRunner::Instance().m_isAssertTest);
        }

        void SetLeakExpected() { AZ_TEST_START_TRACE_SUPPRESSION; m_busRedirector.m_leakExpected = true; }

    private:
        BusRedirector m_busRedirector;
        // We need the BusRedirector to be destroyed before the AllocatorSetup is, therefore we cannot have this test fixture inheriting 
        // from AllocatorSetup which would be the default implementation. 
        AllocatorSetup m_allocatorSetup;
    };

#if AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    TEST_F(AllocatorSetupLeakDetectionTest, DISABLED_Leak)
#else
    TEST_F(AllocatorSetupLeakDetectionTest, Leak)
#endif // AZ_TRAIT_DISABLE_FAILED_ALLOCATOR_LEAK_DETECTION_TESTS
    {
        SetLeakExpected();

        TestClass* leakyObject = aznew TestClass();
        AZ_UNUSED(leakyObject);
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
        AllocatorTypeLeakDetectionTest()
            : m_busRedirector(m_leakDetected)
        {}

        void SetUp() override
        {
            m_drillerManager = AZ::Debug::DrillerManager::Create();
            m_drillerManager->Register(aznew AZ::Debug::MemoryDriller);
            AZ::AllocatorManager::Instance().SetDefaultTrackingMode(AZ::Debug::AllocationRecords::RECORD_FULL);

            if (azrtti_typeid<AllocatorType>() != azrtti_typeid<AZ::SystemAllocator>()) // simplifies instead of template specialization
            {
                // Other allocators need the SystemAllocator in order to work
                AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            }
            AZ::AllocatorInstance<AllocatorType>::Create();

            m_busRedirector.BusConnect();
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<AllocatorType>::Destroy();
            if (azrtti_typeid<AllocatorType>() != azrtti_typeid<AZ::SystemAllocator>()) // simplifies instead of template specialization
            {
                // Other allocators need the SystemAllocator in order to work
                AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
            }
            AZ::Debug::DrillerManager::Destroy(m_drillerManager);

            m_busRedirector.BusDisconnect();
            EXPECT_EQ(m_leakExpected, m_leakDetected);

            if (m_leakExpected)
            {
                AZ_TEST_STOP_TRACE_SUPPRESSION(1);
            }
        }

        void SetLeakExpected() { AZ_TEST_START_TRACE_SUPPRESSION; m_leakExpected = true; }

        // Dummy test class that uses AllocatorType
        class ThisAllocatorTestClass
        {
        public:
            AZ_CLASS_ALLOCATOR(ThisAllocatorTestClass, AllocatorType, 0);
        };

    private:
        class BusRedirector
            : public UnitTest::TraceBusRedirector
        {
        public:
            BusRedirector(bool& leakDetected)
                : m_leakDetected(leakDetected)
            {}

            bool OnPreError(const char* window, const char* file, int line, const char* func, const char* message) override
            {
                AZ_UNUSED(file);
                AZ_UNUSED(line);
                AZ_UNUSED(func);

                if (AZStd::string_view(window) == "Memory"
                    && AZStd::string_view(message) == "We still have 1 allocations on record! They must be freed prior to destroy!")
                {
                    // Leak detected, flag it so we validate on tear down that this happened. We also will 
                    // mark this test since it will assert
                    m_leakDetected = true;
                    return true;
                }
                return false;
            }

            bool OnPrintf(const char* window, const char* message) override
            {
                AZ_UNUSED(window);
                AZ_UNUSED(message);
                // Do not print the error message twice. The error message will already be printed by the TraceBusRedirector
                // in UnitTest.h. Here we override it to prevent it from printing twice.
                return true;
            }

        private:
            bool& m_leakDetected;
        };

        BusRedirector m_busRedirector;
        AZ::Debug::DrillerManager* m_drillerManager = nullptr;
        bool m_leakDetected = false;
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

        typename TestFixture::ThisAllocatorTestClass* leakyObject = aznew typename TestFixture::ThisAllocatorTestClass();
        AZ_UNUSED(leakyObject);
    }

    TYPED_TEST(AllocatorTypeLeakDetectionTest, NoLeak)
    {
        typename TestFixture::ThisAllocatorTestClass* leakyObject = aznew typename TestFixture::ThisAllocatorTestClass();
        delete leakyObject;
    }
}


