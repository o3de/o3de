/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/allocator.h>
#include <AzCore/std/allocator_static.h>
#include <AzCore/std/allocator_ref.h>
#include <AzCore/std/allocator_stack.h>
#include <AzCore/std/allocator_traits.h>

#include <AzCore/Memory/SystemAllocator.h>


using namespace AZStd;
using namespace UnitTestInternal;

namespace UnitTest
{
    /**
     * Test for AZSTD provided default allocators.
     */

    /// Default allocator.
    class AllocatorDefaultTest
        : public AllocatorsTestFixture
    {
    public:
        void run()
        {
            const char name[] = "My test allocator";
            AZStd::allocator myalloc(name);
            AZ_TEST_ASSERT(strcmp(myalloc.get_name(), name) == 0);
            const char newName[] = "My new test allocator";
            myalloc.set_name(newName);
            AZ_TEST_ASSERT(strcmp(myalloc.get_name(), newName) == 0);

            AZStd::allocator::pointer_type data = myalloc.allocate(100, 1);
            AZ_TEST_ASSERT(data != nullptr);

            myalloc.deallocate(data, 100, 1);

            data = myalloc.allocate(50, 128);
            AZ_TEST_ASSERT(data != nullptr);

            myalloc.deallocate(data, 50, 128);

            AZStd::allocator myalloc2;
            AZ_TEST_ASSERT(myalloc == myalloc2); // always true
            AZ_TEST_ASSERT(!(myalloc != myalloc2)); // always false
        }
    };

    TEST_F(AllocatorDefaultTest, Test)
    {
        run();
    }

    TEST_F(AllocatorDefaultTest, AllocatorTraitsExistForAZStdAllocator)
    {
        using AZStdAllocatorTraits = AZStd::allocator_traits<AZStd::allocator>;
        static_assert(AZStd::is_same<AZStdAllocatorTraits::allocator_type, AZStd::allocator>::value, "Allocator trait allocator_type is not the same as AZStd::allocator");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::value_type, uint8_t>::value, "Allocator trait value_type is not the same as uint8_t");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::pointer, void*>::value, "Allocator trait pointer is not the same as void*");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::const_pointer, const uint8_t*>::value, "Allocator trait const_pointer is not the same as const uint8_t*");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::void_pointer, void*>::value, "Allocator trait void_pointer is not the same as void*");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::const_void_pointer, const void*>::value, "Allocator trait const_void_pointer is not the same as void*");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::difference_type, ptrdiff_t>::value, "Allocator trait difference_type is not the same as ptrdiff_t");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::size_type, size_t>::value, "Allocator trait size_type is not the same as size_t");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::propagate_on_container_copy_assignment, false_type>::value, "Allocator trait propagate_on_container_copy_assignment is not the same as false_type");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::propagate_on_container_move_assignment, false_type>::value, "Allocator trait propagate_on_container_move_assignment is not the same as false_type");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::propagate_on_container_swap, false_type>::value, "Allocator trait propagate_on_container_swap is not the same as false_type");
        static_assert(AZStd::is_same<AZStdAllocatorTraits::is_always_equal, false_type>::value, "Allocator trait is_always_equal is not the same as false_type");
        static_assert(AZStd::is_same<typename AZStdAllocatorTraits::template rebind_alloc<int32_t>, AZStd::allocator>::value, "Rebind alloc for AZStd::allocator should return AZStd::allocator");
        static_assert(AZStd::is_same<typename AZStdAllocatorTraits::template rebind_traits<int32_t>::allocator_type, AZStd::allocator>::value, "Rebind traits allocator_type should still be AZStd::allocator");
    }

    TEST_F(AllocatorDefaultTest, AllocatorTraitsAllocateAndDeallocateSucceeds)
    {
        using AZStdAllocatorTraits = AZStd::allocator_traits<AZStd::allocator>;
        AZStd::allocator testAllocator("trait allocator");
        typename AZStdAllocatorTraits::pointer data = AZStdAllocatorTraits::allocate(testAllocator, 50, 128);
        EXPECT_NE(nullptr, data);
        AZStdAllocatorTraits::deallocate(testAllocator, data, 50, 128);
    }

    TEST_F(AllocatorDefaultTest, AllocatorTraitsConstructAndDestroySucceeds)
    {
        static int32_t constructedCount;
        struct TestAllocated
        {
            TestAllocated(int32_t value)
                : m_value(value)
            {
                ++constructedCount;
            }
            ~TestAllocated()
            {
                --constructedCount;
            }

            int32_t m_value{};
        };

        using AZStdAllocatorTraits = AZStd::allocator_traits<AZStd::allocator>;
        AZStd::allocator testAllocator("trait allocator");
        typename AZStdAllocatorTraits::pointer data = AZStdAllocatorTraits::allocate(testAllocator, sizeof(TestAllocated), alignof(TestAllocated));
        EXPECT_NE(nullptr, data);
        auto testPtr = static_cast<TestAllocated*>(data);
        AZStdAllocatorTraits::construct(testAllocator, testPtr, 42);
        EXPECT_EQ(1, constructedCount);
        EXPECT_EQ(42, testPtr->m_value);
        AZStdAllocatorTraits::destroy(testAllocator, testPtr);
        EXPECT_EQ(0, constructedCount);
        AZStdAllocatorTraits::deallocate(testAllocator, data, sizeof(TestAllocated), alignof(TestAllocated));
    }

    TEST_F(AllocatorDefaultTest, AllocatorTraitsMaxSizeCompilesWithoutErrors)
    {
        struct AllocatorWithGetMaxSize
            : AZStd::allocator
        {
            using AZStd::allocator::allocator;
            size_t get_max_size() { return max_size(); }
        };

        using AZStdAllocatorTraits = AZStd::allocator_traits<AllocatorWithGetMaxSize>;
        AllocatorWithGetMaxSize testAllocator("trait allocator");
        typename AZStdAllocatorTraits::size_type maxSize = AZStdAllocatorTraits::max_size(testAllocator);
        EXPECT_EQ(testAllocator.get_max_size(), maxSize);
    }

    TEST_F(AllocatorDefaultTest, AllocatorTraitsSelectOnContainerCopyConstructionCompilesWithoutErrors)
    {
        using AZStdAllocatorTraits = AZStd::allocator_traits<AZStd::allocator>;
        AZStd::allocator testAllocator("trait allocator");
        AZStd::allocator copiedAllocator = AZStdAllocatorTraits::select_on_container_copy_construction(testAllocator);
        EXPECT_EQ(testAllocator, copiedAllocator);
    }

    /// Static buffer allocator.
    TEST(Allocator, StaticBuffer)
    {
        const int bufferSize = 500;
        typedef static_buffer_allocator<bufferSize, 4> buffer_alloc_type;

        const char name[] = "My test allocator";
        buffer_alloc_type myalloc(name);
        AZ_TEST_ASSERT(strcmp(myalloc.get_name(), name) == 0);
        const char newName[] = "My new test allocator";
        myalloc.set_name(newName);
        AZ_TEST_ASSERT(strcmp(myalloc.get_name(), newName) == 0);

        EXPECT_EQ(bufferSize, myalloc.max_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 0);

        buffer_alloc_type::pointer_type data = myalloc.allocate(100, 1);
        AZ_TEST_ASSERT(data != nullptr);
        EXPECT_EQ(bufferSize - 100, myalloc.max_size() - myalloc.get_allocated_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 100);

        myalloc.deallocate(data, 100, 1); // we can free the last allocation only
        EXPECT_EQ(bufferSize, myalloc.max_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 0);

        data = myalloc.allocate(100, 1);
        myalloc.allocate(3, 1);
        myalloc.deallocate(data); // can't free allocation which is not the last.
        EXPECT_EQ(bufferSize - 103, myalloc.max_size() - myalloc.get_allocated_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 103);

        myalloc.reset();
        EXPECT_EQ(bufferSize, myalloc.max_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 0);

        data = myalloc.allocate(50, 64);
        AZ_TEST_ASSERT(data != nullptr);
        AZ_TEST_ASSERT(((AZStd::size_t)data & 63) == 0);
        EXPECT_LE(myalloc.max_size() - myalloc.get_allocated_size(), bufferSize - 50);
        AZ_TEST_ASSERT(myalloc.get_allocated_size() >= 50);

        buffer_alloc_type myalloc2;
        AZ_TEST_ASSERT(myalloc == myalloc);
        AZ_TEST_ASSERT((myalloc2 != myalloc));
    }

    /// Static pool allocator.
    TEST(Allocator, StaticPool)
    {
        const int numNodes = 100;
        const char name[] = "My test allocator";
        const char newName[] = "My new test allocator";
        typedef static_pool_allocator<int, numNodes> int_node_pool_type;
        int_node_pool_type myalloc(name);
        AZ_TEST_ASSERT(strcmp(myalloc.get_name(), name) == 0);
        myalloc.set_name(newName);
        AZ_TEST_ASSERT(strcmp(myalloc.get_name(), newName) == 0);

        EXPECT_EQ(numNodes * sizeof(int), myalloc.max_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 0);

        int* data = reinterpret_cast<int*>(myalloc.allocate(sizeof(int), 1));
        AZ_TEST_ASSERT(data != nullptr);
        EXPECT_EQ((numNodes - 1) * sizeof(int), myalloc.max_size() - myalloc.get_allocated_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == sizeof(int));

        myalloc.deallocate(data, sizeof(int), 1);
        EXPECT_EQ(numNodes * sizeof(int), myalloc.max_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 0);

        for (int i = 0; i < numNodes; ++i)
        {
            data = reinterpret_cast<int*>(myalloc.allocate(sizeof(int), 1));
            AZ_TEST_ASSERT(data != nullptr);
            EXPECT_EQ((numNodes - (i + 1)) * sizeof(int), myalloc.max_size() - myalloc.get_allocated_size());
            AZ_TEST_ASSERT(myalloc.get_allocated_size() == (i + 1) * sizeof(int));
        }

        myalloc.reset();
        EXPECT_EQ(numNodes * sizeof(int), myalloc.max_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 0);

        AZ_TEST_ASSERT(myalloc == myalloc);

        //////////////////////////////////////////////////////////////////////////
        // static pool allocator
        // Generally we can't use more then 16 byte alignment on the stack.
        // Some platforms might fail. Which is ok, higher alignment should be handled by US. Or not on the stack.
        const int dataAlignment = 16;

        typedef aligned_storage<sizeof(int), dataAlignment>::type aligned_int_type;
        typedef static_pool_allocator<aligned_int_type, numNodes> aligned_int_node_pool_type;
        aligned_int_node_pool_type myaligned_pool;
        aligned_int_type* aligned_data = reinterpret_cast<aligned_int_type*>(myaligned_pool.allocate(sizeof(aligned_int_type), dataAlignment));

        AZ_TEST_ASSERT(aligned_data != nullptr);
        AZ_TEST_ASSERT(((AZStd::size_t)aligned_data & (dataAlignment - 1)) == 0);
        EXPECT_EQ((numNodes - 1) * sizeof(aligned_int_type), myaligned_pool.max_size() - myaligned_pool.get_allocated_size());
        AZ_TEST_ASSERT(myaligned_pool.get_allocated_size() == sizeof(aligned_int_type));

        myaligned_pool.deallocate(aligned_data, sizeof(aligned_int_type), dataAlignment); // Make sure we free what we have allocated.
        //////////////////////////////////////////////////////////////////////////
    }

    /// Reference allocator.
    TEST(Allocator, Reference)
    {
        const int bufferSize = 500;
        typedef static_buffer_allocator<bufferSize, 4> buffer_alloc_type;
        buffer_alloc_type shared_allocator("Shared allocator");

        typedef allocator_ref<buffer_alloc_type> ref_allocator_type;

        const char name1[] = "Ref allocator1";
        ref_allocator_type ref_allocator1(shared_allocator, name1);
        const char name2[] = "Ref allocator2";
        ref_allocator_type ref_allocator2(shared_allocator, name2);

        AZ_TEST_ASSERT(strcmp(ref_allocator1.get_name(), name1) == 0);
        AZ_TEST_ASSERT(strcmp(ref_allocator2.get_name(), name2) == 0);

        const char newName1[] = "Ref new allocator1";
        ref_allocator1.set_name(newName1);
        AZ_TEST_ASSERT(strcmp(ref_allocator1.get_name(), newName1) == 0);
        const char newName2[] = "Ref new allocator2";
        ref_allocator2.set_name(newName2);
        AZ_TEST_ASSERT(strcmp(ref_allocator2.get_name(), newName2) == 0);

        AZ_TEST_ASSERT(ref_allocator2.get_allocator() == ref_allocator1.get_allocator());

        ref_allocator_type::pointer_type data1 = ref_allocator1.allocate(10, 1);
        AZ_TEST_ASSERT(data1 != nullptr);
        EXPECT_EQ(bufferSize - 10, ref_allocator1.max_size() - ref_allocator1.get_allocated_size());
        AZ_TEST_ASSERT(ref_allocator1.get_allocated_size() == 10);
        EXPECT_EQ(bufferSize - 10, shared_allocator.max_size() - shared_allocator.get_allocated_size());
        AZ_TEST_ASSERT(shared_allocator.get_allocated_size() == 10);

        ref_allocator_type::pointer_type data2 = ref_allocator2.allocate(10, 1);
        AZ_TEST_ASSERT(data2 != nullptr);
        EXPECT_LE(ref_allocator2.max_size() - ref_allocator2.get_allocated_size(), bufferSize - 20);
        AZ_TEST_ASSERT(ref_allocator2.get_allocated_size() >= 20);
        EXPECT_LE(shared_allocator.max_size() - shared_allocator.get_allocated_size(), bufferSize - 20);
        AZ_TEST_ASSERT(shared_allocator.get_allocated_size() >= 20);

        shared_allocator.reset();

        data1 = ref_allocator1.allocate(10, 32);
        AZ_TEST_ASSERT(data1 != nullptr);
        EXPECT_LE(ref_allocator1.max_size() - ref_allocator1.get_allocated_size(), bufferSize - 10);
        AZ_TEST_ASSERT(ref_allocator1.get_allocated_size() >= 10);
        EXPECT_LE(shared_allocator.max_size() - shared_allocator.get_allocated_size(), bufferSize - 10);
        AZ_TEST_ASSERT(shared_allocator.get_allocated_size() >= 10);

        data2 = ref_allocator2.allocate(10, 32);
        AZ_TEST_ASSERT(data2 != nullptr);
        EXPECT_LE(ref_allocator1.max_size() - ref_allocator1.get_allocated_size(), bufferSize - 20);
        AZ_TEST_ASSERT(ref_allocator1.get_allocated_size() >= 20);
        EXPECT_LE(shared_allocator.max_size() - shared_allocator.get_allocated_size(), bufferSize - 20);
        AZ_TEST_ASSERT(shared_allocator.get_allocated_size() >= 20);

        AZ_TEST_ASSERT(ref_allocator1 == ref_allocator2);
        AZ_TEST_ASSERT(!(ref_allocator1 != ref_allocator2));
    }

    /// Stack buffer allocator.
    TEST(Allocator, Stack)
    {
        size_t bufferSize = 500;

        const char name[] = "My test allocator";
        stack_allocator myalloc(alloca(bufferSize), bufferSize, name);
        AZ_TEST_ASSERT(strcmp(myalloc.get_name(), name) == 0);
        const char newName[] = "My new test allocator";
        myalloc.set_name(newName);
        AZ_TEST_ASSERT(strcmp(myalloc.get_name(), newName) == 0);

        EXPECT_EQ(bufferSize, myalloc.max_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 0);

        stack_allocator::pointer_type data = myalloc.allocate(100, 1);
        AZ_TEST_ASSERT(data != nullptr);
        EXPECT_EQ(bufferSize - 100, myalloc.max_size() - myalloc.get_allocated_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 100);

        myalloc.deallocate(data, 100, 1); // this allocator doesn't free data
        EXPECT_EQ(bufferSize - 100, myalloc.max_size() - myalloc.get_allocated_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 100);

        myalloc.reset();
        EXPECT_EQ(bufferSize, myalloc.max_size());
        AZ_TEST_ASSERT(myalloc.get_allocated_size() == 0);

        data = myalloc.allocate(50, 64);
        AZ_TEST_ASSERT(data != nullptr);
        AZ_TEST_ASSERT(((AZStd::size_t)data & 63) == 0);
        EXPECT_LE(myalloc.max_size() - myalloc.get_allocated_size(), bufferSize - 50);
        AZ_TEST_ASSERT(myalloc.get_allocated_size() >= 50);

        AZ_STACK_ALLOCATOR(myalloc2, 200); // test the macro declaration

        EXPECT_EQ(200, myalloc2.max_size() );

        AZ_TEST_ASSERT(myalloc == myalloc);
        AZ_TEST_ASSERT((myalloc2 != myalloc));
    }
}
