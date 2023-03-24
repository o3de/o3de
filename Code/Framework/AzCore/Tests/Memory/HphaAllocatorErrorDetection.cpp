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
#include <AzCore/Memory/HphaAllocator.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Debug/StackTracer.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Random.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/std/containers/set.h>

namespace AZ
{
    // Specialize TypeInfo for HphaSchemaBase only for the UnitTest
    AZ_TYPE_INFO_TEMPLATE(AZ::HphaSchemaBase, "{FC350AF7-31D9-4B70-ABF5-CC99787BE830}", AZ_TYPE_INFO_AUTO);
}

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
    };

    // Another allocator to test allocating/deallocating with different allocators
    class AnotherTestAllocator
        : public AZ::SimpleSchemaAllocator<AZ::HphaSchemaBase<HphaDebugAllocator>>
    {
    public:
        AZ_TYPE_INFO(AnotherTestAllocator, "{83038931-010E-407F-8183-2ACBB50706C2}");

        using Base = AZ::SimpleSchemaAllocator<AZ::HphaSchemaBase<HphaDebugAllocator>>;
    };
}

namespace AZ
{
    template<>
    class AllocatorInstance<UnitTest::HphaSchemaErrorDetection_TestAllocator>
        : public UnitTest::HphaSchemaErrorDetection_TestAllocator
    {
        using Allocator = UnitTest::HphaSchemaErrorDetection_TestAllocator;
    public:
        AllocatorInstance()
        {
            Create();
            s_allocator = this;
        }

        static Allocator& Get()
        {
            return *s_allocator;
        }

    private:
        inline static Allocator* s_allocator{};

    };

    template<>
    class AllocatorInstance<UnitTest::AnotherTestAllocator>
        : public UnitTest::AnotherTestAllocator
    {
        using Allocator = UnitTest::AnotherTestAllocator;
    public:
        AllocatorInstance()
        {
            Create();
            s_allocator = this;
        }

        static Allocator& Get()
        {
            return *s_allocator;
        }

    private:
        inline static Allocator* s_allocator{};

    };
} // namespace AZ

namespace UnitTest
{

    // Dummy test class with configurable size
    template <size_t Size>
    class TestClass
    {
    public:
        AZ_CLASS_ALLOCATOR(TestClass, HphaSchemaErrorDetection_TestAllocator);

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
            m_allocator = new AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>;
            AZ::Debug::TraceMessageBus::Handler::BusConnect();

            using testing::_;
            EXPECT_CALL(*this, OnPrintf(_, _))
                .WillRepeatedly(testing::Return(false));
            EXPECT_CALL(*this, OnPreAssert(_, _, _, _))
                .WillRepeatedly(testing::Return(false));
        }

        void TearDown() override
        {
            delete m_allocator;
            m_allocator = nullptr;

            AZ::Debug::TraceMessageBus::Handler::BusDisconnect();

            if (m_expectedAsserts > 0)
            {
                AZ_TEST_STOP_TRACE_SUPPRESSION(m_expectedAsserts);
            }
        }

        MOCK_METHOD4(OnPreAssert, bool(const char* file, int line, const char* func, const char* message));
        MOCK_METHOD2(OnPrintf, bool(const char* window, const char* message));

        void ExpectNonEmptyBuckets(int count)
        {
            using testing::_;
            EXPECT_CALL(*this, OnPreAssert(_, _, _, testing::MatchesRegex(R"(HPHA Assert.*possible reason: "small object leak".*)")))
                .Times(count)
                .WillRepeatedly(testing::Return(true));
        }

        void ExpectDoubleDelete(int count)
        {
            using testing::_;
            EXPECT_CALL(*this, OnPreAssert(_, _, _, testing::MatchesRegex(R"(.*possible reason: "double delete or pointer not in this allocator".*)")))
                .Times(count)
                .WillRepeatedly(testing::Return(true));
        }

        void ExpectDeletePointerNotInBucket(int count)
        {
            using testing::_;
            EXPECT_CALL(*this, OnPreAssert(_, _, _, testing::MatchesRegex(R"(.*possible reason: "small object ptr not in a bucket".*)")))
                .Times(count)
                .WillRepeatedly(testing::Return(true));
        }

        void ExpectOverflow(int count)
        {
            using testing::_;
            EXPECT_CALL(*this, OnPreAssert(_, _, _, testing::MatchesRegex(R"(.*possible reason: "overflow".*)")))
                .Times(count)
                .WillRepeatedly(testing::Return(true));
        }

        int m_expectedAsserts = 0;
        AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>* m_allocator;
    };

    TEST_F(HphaSchemaErrorDetectionTest, NoBucketLeak)
    {
        TestClass<1>* leakyObject = aznew TestClass<1>();
        delete leakyObject;
    }

    TEST_F(HphaSchemaErrorDetectionTest, BucketLeak)
    {
        AZ_TEST_START_TRACE_SUPPRESSION;
        ExpectNonEmptyBuckets(1);
        m_expectedAsserts = 1;

        TestClass<1>* leakyObject = aznew TestClass<1>();
        AZ_UNUSED(leakyObject);
    }

    // Leak on two different buckets
    TEST_F(HphaSchemaErrorDetectionTest, TwoBucketLeak)
    {
        using testing::_;

        AZ_TEST_START_TRACE_SUPPRESSION;
        ExpectNonEmptyBuckets(2);
        m_expectedAsserts = 2;

        TestClass<1>* leakyObject1 = aznew TestClass<1>();
        AZ_UNUSED(leakyObject1);

        TestClass<128>* leakyObject128 = aznew TestClass<128>();
        AZ_UNUSED(leakyObject128);
    }

    TEST_F(HphaSchemaErrorDetectionTest, DoubleDelete)
    {
        using testing::_;

        AZ_TEST_START_TRACE_SUPPRESSION;
        ExpectDoubleDelete(1);

        // double deletes produce memory tracking to fail since the counters change
        ExpectNonEmptyBuckets(1);

        m_expectedAsserts = 3;

        TestClass<1>* leakyObject = aznew TestClass<1>();
        delete leakyObject;
        delete leakyObject;
    }

AZ_PUSH_DISABLE_WARNING(4700, "-Wuninitialized")
    TEST_F(HphaSchemaErrorDetectionTest, InvalidDelete)
    {
        using testing::_;

        AZ_TEST_START_TRACE_SUPPRESSION;
        ExpectDeletePointerNotInBucket(1);

        // invalid deletes will also produce this
        ExpectDoubleDelete(1);

        m_expectedAsserts = 3;

        AZ::AllocatorInstance<AnotherTestAllocator> allocator;

        // We force a different allocator so we dont break memory in a bad way, we just want to test that it detects a pointer
        // that is not in that allocator
        TestClass<1>* leakyObject = (TestClass<1>*)AZ::AllocatorInstance<AnotherTestAllocator>::Get().Allocate(sizeof(TestClass<1>), AZStd::alignment_of<TestClass<1>>::value);

        delete leakyObject; // this will free with HphaSchemaErrorDetection_TestAllocator (not AnotherTestAllocator)
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
        ExpectOverflow(1);
        m_expectedAsserts = 1;
        delete someObject;
    }

    TEST_F(HphaSchemaErrorDetectionTest, Reallocate)
    {
        // Make an allocation
        TestClass<1>* someObject = (TestClass<1>*)AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().Allocate(sizeof(TestClass<1>), AZStd::alignment_of<TestClass<1>>::value);

        // Make a reallocation, this usually is don internally in a container. Allocators do not support new[]
        const auto newPointer = AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().reallocate(someObject, 2 * sizeof(TestClass<1>));
        EXPECT_EQ(AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().get_allocated_size(newPointer), 8); // it will extend to 8

        AZ::AllocatorInstance<HphaSchemaErrorDetection_TestAllocator>::Get().DeAllocate(newPointer, 8);
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
