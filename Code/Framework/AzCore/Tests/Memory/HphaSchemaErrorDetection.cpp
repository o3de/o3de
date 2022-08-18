/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// This test validates the Debug functionality of the HphaSchema allocator(via its DebugAllocator template parameter)

#include <AzCore/PlatformIncl.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Memory/HphaSchema.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/std/containers/set.h>

namespace UnitTest
{
    // allocator implementation of the HphaSchema with the debugging functionality enabled 
    static constexpr bool HphaDebugAllocator = true;

    class HphaSchemaErrorDetection_TestAllocator
        : public AZ::SimpleSchemaAllocator<AZ::HphaSchemaBase<HphaDebugAllocator>>
    {
    public:
        AZ_TYPE_INFO(HphaSchemaErrorDetection_TestAllocator, "{ACE2D6E5-4EB8-4DD2-AE95-6BDFD0476801}");

        using Base = AZ::SimpleSchemaAllocator<AZ::HphaSchemaBase<HphaDebugAllocator>>;
        using Descriptor = Base::Descriptor;

        HphaSchemaErrorDetection_TestAllocator()
            : Base("HphaSchemaErrorDetection_TestAllocator", "Allocator for Test")
        {}
    };

    // Another allocator to test allocating/deallocating with different allocators
    class AnotherTestAllocator
        : public AZ::SimpleSchemaAllocator<AZ::HphaSchemaBase<HphaDebugAllocator>>
    {
    public:
        AZ_TYPE_INFO(AnotherTestAllocator, "{83038931-010E-407F-8183-2ACBB50706C2}");

        using Base = AZ::SimpleSchemaAllocator<AZ::HphaSchemaBase<HphaDebugAllocator>>;
        using Descriptor = Base::Descriptor;

        AnotherTestAllocator()
            : Base("AnotherTestAllocator", "Another allocator for Test")
        {}
    };

    // Dummy test class with configurable size
    template <size_t Size>
    class TestClass
    {
    public:
        AZ_CLASS_ALLOCATOR(TestClass, HphaSchemaErrorDetection_TestAllocator, 0);

        TestClass()
        {
            // Constructor declared to avoid default constructor which will initialize m_classSize
        }

        unsigned char m_member[Size];
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Testing HphaSchema detection of memory issues:
    // - Detecting double delete
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class HphaSchemaErrorDetectionTest
        : public ::testing::Test
        , public UnitTest::TraceBusRedirector
    {
    protected:
        void SetUp() override 
        {
            AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Create();
            AZ::Debug::TraceMessageBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Destroy();

            EXPECT_EQ(m_nonEmptyBuckets.m_expected, m_nonEmptyBuckets.m_actual);
            EXPECT_EQ(m_doubleDelete.m_expected, m_doubleDelete.m_actual);
            EXPECT_EQ(m_deletePtrNotInBucket.m_expected, m_deletePtrNotInBucket.m_actual);
            EXPECT_EQ(m_overflow.m_expected, m_overflow.m_actual);

            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

            if (m_expectedAsserts > 0)
            {
                AZ_TEST_STOP_TRACE_SUPPRESSION(m_expectedAsserts);
            }
        }

        bool OnPreAssert(const char* file, int line, const char* func, const char* message) override
        {
            AZ_UNUSED(file);
            AZ_UNUSED(line);
            AZ_UNUSED(func);

            const AZStd::string_view messageView(message);
            if (messageView.starts_with("HPHA Assert"))
            {
                // Extract possible reason
                static const AZStd::string_view reasonToken("possible reason: \"");
                const AZStd::string_view::size_type reasonPos = messageView.find(reasonToken);
                if (reasonPos != AZStd::string_view::npos)
                {
                    
                    const AZStd::string_view reason = messageView.substr(reasonPos + reasonToken.size(), messageView.size() - reasonPos - reasonToken.size() - 1);
                    if (reason == "small object leak")
                    {
                        ++m_nonEmptyBuckets.m_actual;
                        return true;
                    }
                    else if (reason == "double delete or pointer not in this allocator")
                    {
                        ++m_doubleDelete.m_actual;
                        return true;
                    }
                    else if (reason == "small object ptr not in a bucket")
                    {
                        ++m_deletePtrNotInBucket.m_actual;
                        return true;
                    }
                    else if (reason == "overflow")
                    {
                        ++m_overflow.m_actual;
                        return true;
                    }
                }
            }
            return false;
        }

        bool OnPrintf(const char* window, const char* message) override
        {
            // We print here messages that are not printed by the UnitTest's TraceBusRedirector
            if (AZStd::string_view(window) == "HPHA")
            {
                ColoredPrintf(COLOR_RED, "[  MEMORY  ] %s", message);
            }
            return true;
        }

        struct TestCounter
        {
            int m_expected = 0;
            int m_actual = 0;
        };
        TestCounter m_nonEmptyBuckets;
        TestCounter m_doubleDelete;
        TestCounter m_deletePtrNotInBucket;
        TestCounter m_overflow;
        int m_expectedAsserts = 0;
    };

    TEST_F(HphaSchemaErrorDetectionTest, NoBucketLeak)
    {
        TestClass<1>* leakyObject = aznew TestClass<1>();
        delete leakyObject;
    }

    TEST_F(HphaSchemaErrorDetectionTest, BucketLeak)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_nonEmptyBuckets.m_expected = 1;
        m_expectedAsserts = 1;

        TestClass<1>* leakyObject = aznew TestClass<1>();
        AZ_UNUSED(leakyObject);
    }

    // Leak on two different buckets
    TEST_F(HphaSchemaErrorDetectionTest, TwoBucketLeak)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_nonEmptyBuckets.m_expected = 2;
        m_expectedAsserts = 2;

        TestClass<1>* leakyObject1 = aznew TestClass<1>();
        AZ_UNUSED(leakyObject1);

        TestClass<128>* leakyObject128 = aznew TestClass<128>();
        AZ_UNUSED(leakyObject128);
    }

    TEST_F(HphaSchemaErrorDetectionTest, DoubleDelete)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_doubleDelete.m_expected = 1;
        m_nonEmptyBuckets.m_expected = 1; // double deletes produce memory tracking to fail since the counters change
        m_expectedAsserts = 3;

        TestClass<1>* leakyObject = aznew TestClass<1>();
        delete leakyObject;
        delete leakyObject;
    }

AZ_PUSH_DISABLE_WARNING(4700, "-Wuninitialized")
    TEST_F(HphaSchemaErrorDetectionTest, InvalidDelete)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_deletePtrNotInBucket.m_expected = 1;
        m_doubleDelete.m_expected = 1; // invalid deletes will also produce this
        m_expectedAsserts = 3;

        AZ::AllocatorInstance<AnotherTestAllocator>::Create();

        // We force a different allocator so we dont break memory in a bad way, we just want to test that it detects a pointer
        // that is not in that allocator
        TestClass<1>* leakyObject = (TestClass<1>*)AZ::AllocatorInstance<AnotherTestAllocator>::Get().Allocate(sizeof(TestClass<1>), AZStd::alignment_of<TestClass<1>>::value);

        delete leakyObject; // this will free with HphaSchemaErrorDetection_TestAllocator (not AnotherTestAllocator)

        AZ::AllocatorInstance<AnotherTestAllocator>::Destroy();
    }
AZ_POP_DISABLE_WARNING

    TEST_F(HphaSchemaErrorDetectionTest, ValidDelete)
    {
        TestClass<1>* leakyObject = (TestClass<1>*)AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().Allocate(sizeof(TestClass<1>), AZStd::alignment_of<TestClass<1>>::value);

        leakyObject->~TestClass<1>();
        AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().DeAllocate(leakyObject, sizeof(TestClass<1>), AZStd::alignment_of<TestClass<1>>::value);
    }

    // Tests that uninitialized memory has the QNAN pattern
    TEST_F(HphaSchemaErrorDetectionTest, MemoryUnitializedPattern)
    {
        TestClass<16>* someObject = aznew TestClass<16>();
        
        for (size_t i = 0; i < 16; i += 4)
        {
            EXPECT_EQ(0xFF, someObject->m_member[i + 0]);
            EXPECT_EQ(0xC0, someObject->m_member[i + 1]);
            EXPECT_EQ(0xC0, someObject->m_member[i + 2]);
            EXPECT_EQ(0xFF, someObject->m_member[i + 3]);
        }

        delete someObject;
    }
    
    TEST_F(HphaSchemaErrorDetectionTest, MemoryAfterDeletePattern)
    {
        TestClass<32>* someObject = aznew TestClass<32>();

        memset(someObject->m_member, 0, 32);

        delete someObject;

        // The first bytes are reused for an intrusive list node that keeps track of the free entries in the bucket
        for (size_t i = AZ::HphaSchemaBase<HphaDebugAllocator>::GetFreeLinkSize(); i < 32; i += 4)
        {
            EXPECT_EQ(0xFF, someObject->m_member[i + 0]);
            EXPECT_EQ(0xC0, someObject->m_member[i + 1]);
            EXPECT_EQ(0xC0, someObject->m_member[i + 2]);
            EXPECT_EQ(0xFF, someObject->m_member[i + 3]);
        }
    }

    TEST_F(HphaSchemaErrorDetectionTest, OverflowGuard)
    {
        // the overflow guard is generated out of rand, so we set a fixed seed before doing the allocation
        // to get a deterministic guard
        srand(0);
        const unsigned char expectedInitialGuard = static_cast<unsigned char>(rand());

        srand(0);
        TestClass<16>* someObject = aznew TestClass<16>();

        unsigned char* pointerToEnd = &someObject->m_member[16];
        for (size_t i = 0; i < AZ::HphaSchemaBase<HphaDebugAllocator>::GetMemoryGuardSize(); ++i)
        {
            EXPECT_EQ(expectedInitialGuard + i, pointerToEnd[i]);
        }

        // Now produce an overflow, any value different than the initial guard will do
        pointerToEnd[0] = ~expectedInitialGuard;

        // delete the object, we should get the overflow detected
        AZ_TEST_START_TRACE_SUPPRESSION;
        m_overflow.m_expected = 1;
        m_expectedAsserts = 1;
        delete someObject;
    }

    TEST_F(HphaSchemaErrorDetectionTest, Resize)
    {
        // Make an allocation
        TestClass<1>* someObject = (TestClass<1>*)AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().Allocate(sizeof(TestClass<1>), AZStd::alignment_of<TestClass<1>>::value);

        // Make a reallocation, this usually is don internally in a container. Allocators do not support new[]
        const size_t newSize = AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().Resize(someObject, 2 * sizeof(TestClass<1>));
        EXPECT_EQ(8, newSize); // it will extend to 8

        AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().DeAllocate(someObject, newSize);
    }

    TEST_F(HphaSchemaErrorDetectionTest, Reallocation)
    {
        // Make an allocation
        TestClass<1>* someObject = (TestClass<1>*)AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().Allocate(sizeof(TestClass<1>), AZStd::alignment_of<TestClass<1>>::value);

        // Make a reallocation, this usually is don internally in a container. Allocators do not support new[]
        someObject = (TestClass<1>*)AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().ReAllocate(someObject, 2 * sizeof(TestClass<1>), AZStd::alignment_of<TestClass<1>>::value);

        AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().DeAllocate(someObject, 2 * sizeof(TestClass<1>));
    }
}
