/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/parallel/allocator_concurrent_static.h>


namespace UnitTest
{
    using namespace AZStd;
    using namespace UnitTestInternal;

    static constexpr size_t s_allocatorCapacity = 1024;
    static constexpr size_t s_numberThreads = 4;

    template <typename AllocatorType>
    class ConcurrentAllocatorTestFixture
        : public LeakDetectionFixture
    {
    protected:
        using this_type = ConcurrentAllocatorTestFixture<AllocatorType>;
        using allocator_type = AllocatorType;
    };

    struct NodeType
    {
        int m_number;
    };

    using AllocatorTypes = ::testing::Types<
        AZStd::static_pool_concurrent_allocator<NodeType, s_allocatorCapacity>
    >;
    TYPED_TEST_CASE(ConcurrentAllocatorTestFixture, AllocatorTypes);

    TYPED_TEST(ConcurrentAllocatorTestFixture, Name)
    {
        const char name[] = "My test allocator";
        typename TestFixture::allocator_type myalloc(name);
        EXPECT_EQ(0, strcmp(myalloc.get_name(), name));
        {
            const char newName[] = "My new test allocator";
            myalloc.set_name(newName);
            EXPECT_EQ(0, strcmp(myalloc.get_name(), newName));
            EXPECT_EQ(sizeof(typename TestFixture::allocator_type::value_type) * s_allocatorCapacity, myalloc.max_size());
        }
    }

    TYPED_TEST(ConcurrentAllocatorTestFixture, AllocateDeallocate)
    {
        typename TestFixture::allocator_type myalloc;

        EXPECT_EQ(0, myalloc.get_allocated_size());
        typename TestFixture::allocator_type::pointer data = myalloc.allocate();
        EXPECT_NE(nullptr, data);
        EXPECT_EQ(sizeof(typename TestFixture::allocator_type::value_type), myalloc.get_allocated_size());
        EXPECT_EQ(sizeof(typename TestFixture::allocator_type::value_type) * (s_allocatorCapacity - 1), myalloc.max_size() - myalloc.get_allocated_size());
        myalloc.deallocate(data);
        EXPECT_EQ(0, myalloc.get_allocated_size());
        EXPECT_EQ(sizeof(typename TestFixture::allocator_type::value_type) * s_allocatorCapacity, myalloc.max_size());
    }

    TYPED_TEST(ConcurrentAllocatorTestFixture, MultipleAllocateDeallocate)
    {
        typename TestFixture::allocator_type myalloc;

        // Allocate N (6) and free half (evens)
        constexpr size_t dataSize = 6; // keep this number even
        typename TestFixture::allocator_type::pointer data[dataSize];
        AZStd::set<typename TestFixture::allocator_type::pointer> dataSet; // to test for uniqueness
        for (size_t i = 0; i < dataSize; ++i)
        {
            data[i] = myalloc.allocate();
            EXPECT_NE(nullptr, data[i]);
            dataSet.insert(data[i]);
        }
        EXPECT_EQ(dataSize, dataSet.size());
        dataSet.clear();
        EXPECT_EQ(sizeof(typename TestFixture::allocator_type::value_type) * dataSize, myalloc.get_allocated_size());
        EXPECT_EQ((s_allocatorCapacity - dataSize) * sizeof(typename TestFixture::allocator_type::value_type), myalloc.max_size() - myalloc.get_allocated_size());
        for (size_t i = 0; i < dataSize; i += 2)
        {
            myalloc.deallocate(data[i]);
        }
        EXPECT_EQ(sizeof(typename TestFixture::allocator_type::value_type) * (dataSize / 2), myalloc.get_allocated_size());
        EXPECT_EQ((s_allocatorCapacity - dataSize / 2) * sizeof(typename TestFixture::allocator_type::value_type), myalloc.max_size() - myalloc.get_allocated_size());
        for (size_t i = 1; i < dataSize; i += 2)
        {
            myalloc.deallocate(data[i]);
        }
        EXPECT_EQ(0, myalloc.get_allocated_size());
        EXPECT_EQ(s_allocatorCapacity * sizeof(typename TestFixture::allocator_type::value_type), myalloc.max_size());
    }

    TYPED_TEST(ConcurrentAllocatorTestFixture, ConcurrentAllocateoDeallocate)
    {
        typename TestFixture::allocator_type myalloc;

        AZStd::atomic<int> failures{ 0 };
        AZStd::array<AZStd::thread, s_numberThreads> threads;
        for (size_t i = 0; i < s_numberThreads; ++i)
        {
            threads[i] = AZStd::thread([&myalloc, &failures]
            {
                // We have 4 threads, each thread can allocate at most s_allocatorCapacity/s_numberThreads values.
                // The amount of iterations do not affect since each thread will free all the values before the next
                // iteration
                constexpr size_t numIterations = 100;
                constexpr size_t numValues = s_allocatorCapacity / s_numberThreads;
                AZStd::array<typename TestFixture::allocator_type::pointer, numValues> allocations;
                for (int iter = 0; iter < numIterations; ++iter)
                {
                    // allocate
                    for (int i = 0; i < numValues; ++i)
                    {
                        allocations[i] = myalloc.allocate();
                        if (!allocations[i])
                        {
                            ++failures;
                        }
                    }
                    // deallocate
                    for (int i = 0; i < numValues; ++i)
                    {
                        myalloc.deallocate(allocations[i]);
                        allocations[i] = nullptr;
                    }
                }
            });
        }
        for (size_t i = 0; i < s_numberThreads; ++i)
        {
            threads[i].join();
        }
        EXPECT_EQ(0, failures);
        EXPECT_EQ(0, myalloc.get_allocated_size());
    }

    using StaticPoolConcurrentAllocatorTestFixture = LeakDetectionFixture;

    TEST(StaticPoolConcurrentAllocatorTestFixture, Aligment)
    {
        // static pool allocator
        // Generally we can't use more then 16 byte alignment on the stack.
        // Some platforms might fail. Which is ok, higher alignment should be handled by US. Or not on the stack.
        const int dataAlignment = 16;

        typedef aligned_storage<sizeof(int), dataAlignment>::type aligned_int_type;
        typedef AZStd::static_pool_concurrent_allocator<aligned_int_type, s_allocatorCapacity> aligned_int_node_pool_type;
        aligned_int_node_pool_type myaligned_pool;
        aligned_int_type* aligned_data = reinterpret_cast<aligned_int_type*>(myaligned_pool.allocate(sizeof(aligned_int_type), dataAlignment));

        EXPECT_NE(nullptr, aligned_data);
        EXPECT_EQ(0, ((AZStd::size_t)aligned_data & (dataAlignment - 1)));
        EXPECT_EQ((s_allocatorCapacity - 1) * sizeof(aligned_int_type), myaligned_pool.max_size() - myaligned_pool.get_allocated_size());
        EXPECT_EQ(sizeof(aligned_int_type), myaligned_pool.get_allocated_size());

        myaligned_pool.deallocate(aligned_data, sizeof(aligned_int_type), dataAlignment); // Make sure we free what we have allocated.
    }
}
