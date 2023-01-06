/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "UserTypes.h"

#include <AzCore/std/allocator_ref.h>
#include <AzCore/std/allocator_static.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/containers/deque.h>
#include <AzCore/std/containers/queue.h>
#include <AzCore/std/containers/ring_buffer.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/containers/stack.h>
#include <AzCore/std/ranges/transform_view.h>

#define AZ_TEST_VALIDATE_EMPTY_DEQUE(_Deque) \
    AZ_TEST_ASSERT(_Deque.validate());       \
    AZ_TEST_ASSERT(_Deque.size() == 0);      \
    AZ_TEST_ASSERT(_Deque.empty());          \
    AZ_TEST_ASSERT(_Deque.begin() == _Deque.end());

#define AZ_TEST_VALIDATE_DEQUE(_Deque, _NumElements)                       \
    AZ_TEST_ASSERT(_Deque.validate());                                     \
    AZ_TEST_ASSERT(_Deque.size() == _NumElements);                         \
    AZ_TEST_ASSERT((_NumElements > 0) ? !_Deque.empty() : _Deque.empty()); \
    AZ_TEST_ASSERT((_NumElements > 0) ? _Deque.begin() != _Deque.end() : _Deque.begin() == _Deque.end());

namespace UnitTest
{
    class Containers
        : public LeakDetectionFixture
    {
    };
    
    /**
     * Deque container test.
     */
    TEST_F(Containers, Deque)
    {
        // DequeContainerTest-Begin

        using int_deque_type = AZStd::deque<int>;

        int_deque_type int_deque;
        AZ_TEST_VALIDATE_EMPTY_DEQUE(int_deque);

        int_deque_type int_deque1(10);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 10);

        int_deque_type int_deque2(6, 101);
        AZ_TEST_VALIDATE_DEQUE(int_deque2, 6);
        AZ_TEST_ASSERT(int_deque2.front() == 101);
        AZ_TEST_ASSERT(int_deque2.back() == 101);

        int_deque_type int_deque3(int_deque1);
        AZ_TEST_VALIDATE_DEQUE(int_deque3, int_deque1.size());
        AZ_TEST_ASSERT(int_deque3 == int_deque1);
        AZ_TEST_ASSERT(int_deque3 != int_deque2);

        int_deque_type int_deque4(int_deque2.begin(), int_deque2.end());
        AZ_TEST_VALIDATE_DEQUE(int_deque4, int_deque2.size());
        AZ_TEST_ASSERT(int_deque4 == int_deque2);
        AZ_TEST_ASSERT(int_deque4 != int_deque3);

        // This one will force the map to grow, which is a different code path.
        int_deque1.insert(int_deque1.end(), 10, 99);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 20);
        AZ_TEST_ASSERT(int_deque1.back() == 99);

        int_deque3 = int_deque2;
        AZ_TEST_VALIDATE_DEQUE(int_deque3, int_deque2.size());
        AZ_TEST_ASSERT(int_deque3 == int_deque2);

        int_deque1.resize(30, 199);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 30);
        AZ_TEST_ASSERT(int_deque1.back() == 199);

        int_deque1.resize(40, 299);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 40);

        AZ_TEST_ASSERT(int_deque1.at(29) == 199);
        AZ_TEST_ASSERT(int_deque1.at(30) == 299);

        for (int_deque_type::size_type i = 0; i < int_deque1.size(); ++i)
        {
            AZ_TEST_ASSERT(int_deque1.at(i) == int_deque1[i]);
        }

        AZ_TEST_ASSERT(int_deque2.front() == 101);
        AZ_TEST_ASSERT(int_deque2.back() == 101);

        int_deque2.push_front(11);
        AZ_TEST_VALIDATE_DEQUE(int_deque2, 7);
        AZ_TEST_ASSERT(int_deque2.front() == 11);
        int_deque2.push_back(21);
        AZ_TEST_VALIDATE_DEQUE(int_deque2, 8);
        AZ_TEST_ASSERT(int_deque2.back() == 21);

        int_deque2.pop_front();
        AZ_TEST_VALIDATE_DEQUE(int_deque2, 7);
        AZ_TEST_ASSERT(int_deque2.front() == 101);
        int_deque2.pop_back();
        AZ_TEST_VALIDATE_DEQUE(int_deque2, 6);
        AZ_TEST_ASSERT(int_deque2.back() == 101);

        int_deque1.assign(5, 333);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 5);

        AZStd::array<int, 7> elements = {
            {1, 2, 3, 4, 5, 6, 7}
        };
        int_deque1.assign(elements.begin(), elements.end());
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 7);

        int_deque1.insert(int_deque1.begin(), 101);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 8);
        AZ_TEST_ASSERT(int_deque1.front() == 101);

        int_deque1.insert(int_deque1.end(), 201);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 9);
        AZ_TEST_ASSERT(int_deque1.back() == 201);

        int_deque1.insert(next(int_deque1.begin(), 3), 301);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 10);
        AZ_TEST_ASSERT(int_deque1[3] == 301);

        int_deque1.insert(int_deque1.begin(), 2, 401);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 12);
        AZ_TEST_ASSERT(int_deque1.front() == 401);

        int_deque1.insert(int_deque1.end(), 3, 501);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 15);
        AZ_TEST_ASSERT(int_deque1.back() == 501);

        int_deque1.insert(next(int_deque1.begin(), 3), 5, 601);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 20);
        AZ_TEST_ASSERT(int_deque1[3] == 601);

        int_deque1.insert(int_deque1.begin(), elements.begin(), AZStd::next(elements.begin()));
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 21);
        AZ_TEST_ASSERT(int_deque1.front() == 1);

        int_deque1.insert(int_deque1.end(), AZStd::prev(elements.end()), elements.end());
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 22);
        AZ_TEST_ASSERT(int_deque1.back() == 7);

        int_deque1.insert(next(int_deque1.begin(), 3), elements.begin(), elements.end());
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 29);
        AZ_TEST_ASSERT(int_deque1[3] == 1);

        int_deque1.insert(int_deque1.begin(), { 42 });
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 30);
        AZ_TEST_ASSERT(int_deque1.front() == 42);

        int_deque1.insert(int_deque1.begin(), { 1, 1, 2, 3, 5, 8, 13 });
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 37);
        AZ_TEST_ASSERT(int_deque1.front() == 1);
        AZ_TEST_ASSERT(int_deque1[3] == 3);

        AZ_TEST_VALIDATE_DEQUE(int_deque1, 37);
        int_deque1.erase(int_deque1.begin(), int_deque1.begin() + 8);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 29);

        int_deque1.erase(int_deque1.begin());
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 28);
        AZ_TEST_ASSERT(int_deque1.front() == 401);

        int_deque1.erase(prev(int_deque1.end()));
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 27);
        AZ_TEST_ASSERT(int_deque1.back() == 501);

        int_deque1.erase(next(int_deque1.begin()), int_deque1.end());
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 1);
        AZ_TEST_ASSERT(int_deque1.front() == 401);

        int_deque1.swap(int_deque2);
        AZ_TEST_VALIDATE_DEQUE(int_deque1, 6);
        AZ_TEST_VALIDATE_DEQUE(int_deque2, 1);
        AZ_TEST_ASSERT(int_deque1.front() == 101);
        AZ_TEST_ASSERT(int_deque1.back() == 101);
        AZ_TEST_ASSERT(int_deque2.front() == 401);

        for (int_deque_type::iterator it = int_deque2.begin(); it != int_deque2.end(); ++it)
        {
            AZ_TEST_ASSERT(*it == 401);
        }

        for (int_deque_type::reverse_iterator rit = int_deque2.rbegin(); rit != int_deque2.rend(); ++rit)
        {
            AZ_TEST_ASSERT(*rit == 401);
        }

        // extensions
        int_deque2.emplace_back();
        AZ_TEST_VALIDATE_DEQUE(int_deque2, 2);
        AZ_TEST_ASSERT(int_deque2.front() == 401);

        int_deque2.emplace_front();
        AZ_TEST_VALIDATE_DEQUE(int_deque2, 3);
        AZ_TEST_ASSERT(int_deque2[1] == 401);

        // alignment
        AZStd::deque<UnitTestInternal::MyClass> aligned_deque(5, 99);
        for (AZStd::size_t i = 0; i < aligned_deque.size(); ++i)
        {
            AZ_TEST_ASSERT(((AZStd::size_t)&aligned_deque[i] & (AZStd::alignment_of<UnitTestInternal::MyClass>::value - 1)) == 0);
        }

        // different allocators
        using static_buffer_16KB_type = AZStd::static_buffer_allocator<16 * 1024, 1>;
        static_buffer_16KB_type myMemoryManager1;
        static_buffer_16KB_type myMemoryManager2;
        using static_allocator_ref_type = AZStd::allocator_ref<static_buffer_16KB_type>;
        static_allocator_ref_type allocator1(myMemoryManager1);
        static_allocator_ref_type allocator2(myMemoryManager2);

        using int_deque_myalloc_type = AZStd::deque<int, static_allocator_ref_type>;
        int_deque_myalloc_type int_deque10(100, 13, allocator1); /// Allocate 100 elements using memory manager 1
        AZ_TEST_VALIDATE_DEQUE(int_deque10, 100);
        AZ_TEST_ASSERT(myMemoryManager1.get_allocated_size() >= 100 * sizeof(int));

        // leak_and_reset
        int_deque10.leak_and_reset();  /// leave the allocated memory and reset the vector.
        AZ_TEST_VALIDATE_EMPTY_DEQUE(int_deque10);
        AZ_TEST_ASSERT(myMemoryManager1.get_allocated_size() >= 100 * sizeof(int));
        myMemoryManager1.reset(); /// discard the memory

        // allocate again from myMemoryManager1
        int_deque10.resize(100, 15);

        const size_t allocator1AllocatedSize = myMemoryManager1.get_allocated_size();
        int_deque10.set_allocator(allocator2);
        AZ_TEST_VALIDATE_DEQUE(int_deque10, 100);
        // now we move the allocated size from manager1 to manager2
        EXPECT_LE(myMemoryManager1.get_allocated_size(), allocator1AllocatedSize);
        EXPECT_GE(myMemoryManager2.get_allocated_size(), 100 * sizeof(int));

        myMemoryManager1.reset(); // flush manager 1 again (int_vector10 is stored in manager 2)

        // swap with different allocators
        int_deque_myalloc_type int_deque11(50, 25, allocator1); // create copy in manager1
        AZ_TEST_VALIDATE_DEQUE(int_deque11, 50);

        int_deque11.swap(int_deque10); // swap the vectors content (since the allocators are different)
        AZ_TEST_VALIDATE_DEQUE(int_deque10, 50);
        AZ_TEST_VALIDATE_DEQUE(int_deque11, 100);
        AZ_TEST_ASSERT(int_deque11.front() == 15);
        AZ_TEST_ASSERT(int_deque10.front() == 25);

        //////////////////////////////////////////////////////////////////////////////////////////
        // Test asserts (which don't cause throw exceptions)
        //AZ_TEST_START_TRACE_SUPPRESSION;
        //int_deque10.resize(1000000);  // too many elements, 1 assert on too many, 1 assert on allocator returning NULL
        //AZ_TEST_STOP_TRACE_SUPPRESSION(2);

#ifdef AZSTD_HAS_CHECKED_ITERATORS
        int_deque.clear();
        int_deque_type::iterator iter = int_deque.end();
        // We have exeption when we access the map.
        //AZ_TEST_START_TRACE_SUPPRESSION;
        //int b = *iter; // the end if is valid but can not dereferenced
        //int_deque.validate_iterator(iter);
        //(void)b;
        //AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        int_deque.push_back(1);
        AZ_TEST_START_TRACE_SUPPRESSION;
        int_deque.validate_iterator(iter); // The push back should make the end iterator invalid.
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);

        iter = int_deque.begin();
        int_deque.clear();
        AZ_TEST_START_TRACE_SUPPRESSION;
        int_deque.validate_iterator(iter); // The clear should invalidate all iterators
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
#endif

        aligned_deque.emplace_back(10, true, 2.0f);

        // DequeContainerTest-End
    }

    TEST_F(Containers, Deque_DeductionGuide_Compiles)
    {
        constexpr AZStd::string_view testView;
        AZStd::deque testDeque(testView.begin(), testView.end());
        EXPECT_TRUE(testDeque.empty());
    }

    /**
    * Queue container test.
    */
    TEST_F(Containers, Queue)
    {
        // QueueContainerTest-Begin
        using int_queue_type = AZStd::queue<int>;
        int_queue_type int_queue;
        AZ_TEST_ASSERT(int_queue.empty());
        AZ_TEST_ASSERT(int_queue.size() == 0);

        // Queue uses deque as default container, so try to construct to queue from a deque.
        AZStd::deque<int> container(40, 10);
        int_queue_type int_queue2(container);
        AZ_TEST_ASSERT(!int_queue2.empty());
        AZ_TEST_ASSERT(int_queue2.size() == 40);

        int_queue.push(10);
        AZ_TEST_ASSERT(!int_queue.empty());
        AZ_TEST_ASSERT(int_queue.size() == 1);
        AZ_TEST_ASSERT(int_queue.front() == int_queue.back());
        AZ_TEST_ASSERT(int_queue.front() == 10);

        int_queue.pop();
        AZ_TEST_ASSERT(int_queue.empty());
        AZ_TEST_ASSERT(int_queue.size() == 0);

        int_queue2.push(20);
        AZ_TEST_ASSERT(!int_queue2.empty());
        AZ_TEST_ASSERT(int_queue2.size() == 41);
        AZ_TEST_ASSERT(int_queue2.back() == 20);

        int_queue2.pop();
        AZ_TEST_ASSERT(!int_queue2.empty());
        AZ_TEST_ASSERT(int_queue2.size() == 40);
        AZ_TEST_ASSERT(int_queue2.back() == 20);

        int_queue.emplace();
        AZ_TEST_ASSERT(!int_queue.empty());
        AZ_TEST_ASSERT(int_queue.size() == 1);

        // Test Swap
        int_queue.swap(int_queue2);
        AZ_TEST_ASSERT(!int_queue2.empty());
        AZ_TEST_ASSERT(int_queue2.size() == 1);
        AZ_TEST_ASSERT(!int_queue.empty());
        AZ_TEST_ASSERT(int_queue.size() == 40);
        AZ_TEST_ASSERT(int_queue.back() == 20);

        AZStd::queue<UnitTestInternal::MyClass> class_queue;
        class_queue.emplace(3, false, 1.0f);

        // QueueContainerTest-End
    }

    /**
    * Priority queue container test.
    */
    TEST_F(Containers, PriorityQueue)
    {
        // PriorityQueueContainerTest-Begin
        using int_priority_queue_type = AZStd::priority_queue<int>;
        int_priority_queue_type int_queue;
        AZ_TEST_ASSERT(int_queue.empty());
        AZ_TEST_ASSERT(int_queue.size() == 0);

        AZStd::array<int, 10> elements = {
            {10, 2, 6, 3, 5, 8, 7, 9, 1, 4}
        };
        int_priority_queue_type int_queue2(elements.begin(), elements.end());
        AZ_TEST_ASSERT(!int_queue2.empty());
        AZ_TEST_ASSERT(int_queue2.size() == 10);
        int lastValue = 11;
        while (!int_queue2.empty())
        {
            AZ_TEST_ASSERT(int_queue2.top() < lastValue);
            lastValue = int_queue2.top();
            int_queue2.pop();
        }
        AZ_TEST_ASSERT(int_queue2.size() == 0);

        AZStd::priority_queue<int, AZStd::vector<int>, AZStd::greater<int> > int_queue3(elements.begin(), elements.end());
        AZ_TEST_ASSERT(!int_queue3.empty());
        AZ_TEST_ASSERT(int_queue3.size() == 10);
        lastValue = 0;
        while (!int_queue3.empty())
        {
            AZ_TEST_ASSERT(int_queue3.top() > lastValue);
            lastValue = int_queue3.top();
            int_queue3.pop();
        }
        AZ_TEST_ASSERT(int_queue3.size() == 0);

        int_priority_queue_type int_queue4(elements.begin(), elements.end());
        int_queue4.push(100);
        AZ_TEST_ASSERT(!int_queue4.empty());
        AZ_TEST_ASSERT(int_queue4.size() == 11);
        AZ_TEST_ASSERT(int_queue4.top() == 100);
        // PriorityQueueContainerTest-End
    }

    /**
    * Stack container test.
    */
    TEST_F(Containers, Stack)
    {
        // StackContainerTest-Begin
        using int_stack_type = AZStd::stack<int>;
        int_stack_type int_stack;
        AZ_TEST_ASSERT(int_stack.empty());
        AZ_TEST_ASSERT(int_stack.size() == 0);

        AZStd::deque<int> container(40, 10);
        int_stack_type int_stack2(container);
        AZ_TEST_ASSERT(!int_stack2.empty());
        AZ_TEST_ASSERT(int_stack2.size() == 40);

        int_stack.push(20);
        AZ_TEST_ASSERT(!int_stack.empty());
        AZ_TEST_ASSERT(int_stack.size() == 1);
        AZ_TEST_ASSERT(int_stack.top() == 20);

        int_stack.pop();
        AZ_TEST_ASSERT(int_stack.empty());
        AZ_TEST_ASSERT(int_stack.size() == 0);

        int_stack2.push(20);
        AZ_TEST_ASSERT(!int_stack2.empty());
        AZ_TEST_ASSERT(int_stack2.size() == 41);
        AZ_TEST_ASSERT(int_stack2.top() == 20);

        int_stack2.pop();
        AZ_TEST_ASSERT(!int_stack2.empty());
        AZ_TEST_ASSERT(int_stack2.size() == 40);
        AZ_TEST_ASSERT(int_stack2.top() == 10);

        int_stack.emplace();
        AZ_TEST_ASSERT(!int_stack.empty());
        AZ_TEST_ASSERT(int_stack.size() == 1);
        // StackContainerTest-End
    }

    /**
    * Make sure a ring_buffer is empty, and control all functions to return the proper values.
    * Empty ring_buffer as all AZStd containers should not have allocated any memory. Empty and clean containers are not the same.
    */
#define AZ_TEST_VALIDATE_EMPTY_RINGBUFFER(_RingBuffer) \
    AZ_TEST_ASSERT(_RingBuffer.validate());            \
    AZ_TEST_ASSERT(_RingBuffer.size() == 0);           \
    AZ_TEST_ASSERT(_RingBuffer.empty());               \
    AZ_TEST_ASSERT(_RingBuffer.capacity() == 0);       \
    AZ_TEST_ASSERT(_RingBuffer.begin() == _RingBuffer.end());

    /**
    * Validate a ring_buffer for certain number of elements.
    */
#define AZ_TEST_VALIDATE_RINGBUFFER(_RingBuffer, _NumElements)                          \
    AZ_TEST_ASSERT(_RingBuffer.validate());                                             \
    AZ_TEST_ASSERT(_RingBuffer.size() == _NumElements);                                 \
    AZ_TEST_ASSERT((_NumElements > 0) ? !_RingBuffer.empty() : _RingBuffer.empty());    \
    AZ_TEST_ASSERT((_NumElements > 0) ? _RingBuffer.capacity() >= _NumElements : true); \
    AZ_TEST_ASSERT((_NumElements > 0) ? _RingBuffer.begin() != _RingBuffer.end() : _RingBuffer.begin() == _RingBuffer.end());

    /**
     * ring_buffer container test.
     */
    TEST_F(Containers, RingBuffer)
    {
        using int_ringbuffer_type = AZStd::ring_buffer<int>;
        using class_ringbuffer_type = AZStd::ring_buffer<UnitTestInternal::MyClass>;

        // Test empty  buffer with intergral type.
        int_ringbuffer_type int_buffer;
        AZ_TEST_VALIDATE_EMPTY_RINGBUFFER(int_buffer);

        // Default vector (non-integral type).
        class_ringbuffer_type myclass_buffer;
        AZ_TEST_VALIDATE_EMPTY_RINGBUFFER(myclass_buffer);

        // Allocate buffer with capacity of 10 elements.
        int_ringbuffer_type int_buffer1(10);
        AZ_TEST_ASSERT(int_buffer1.size() == 0);
        AZ_TEST_ASSERT(int_buffer1.capacity() == 10);
        AZ_TEST_ASSERT(int_buffer1.empty());
        AZ_TEST_ASSERT(int_buffer1.begin() == int_buffer1.end());

        // Allocate buffer with 15 elements init to 13.
        int_ringbuffer_type int_buffer2(15, 13);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer2, 15);
        for (int_ringbuffer_type::iterator iter = int_buffer2.begin(); iter != int_buffer2.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter == 13);
        }

        // Allocate buffer with 15 elements init to 13 and a capacity 31.
        int_ringbuffer_type int_buffer3(31, 15, 13);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer3, 15);
        AZ_TEST_ASSERT(int_buffer3.capacity() == 31);
        for (int_ringbuffer_type::iterator iter = int_buffer3.begin(); iter != int_buffer3.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter == 13);
        }

        // Copy ctor
        int_ringbuffer_type int_buffer4(int_buffer3);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer4, 15);
        AZ_TEST_ASSERT(int_buffer4.capacity() == 31);
        for (int_ringbuffer_type::iterator iter = int_buffer4.begin(); iter != int_buffer4.end(); ++iter)
        {
            AZ_TEST_ASSERT(*iter == 13);
        }

        // Test == and !=
        AZ_TEST_ASSERT(int_buffer4 == int_buffer3);
        AZ_TEST_ASSERT((int_buffer4 != int_buffer3) == false);

        AZStd::array<int, 6> myArr = {
            {0, 1, 2, 3, 4, 5}
        };
        int_ringbuffer_type int_buffer5(myArr.begin(), myArr.end());
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size());
        int i = 0;
        for (int_ringbuffer_type::iterator iter = int_buffer5.begin(); iter != int_buffer5.end(); ++iter, ++i)
        {
            AZ_TEST_ASSERT(*iter == i);
        }

        int_ringbuffer_type int_buffer6(10, myArr.begin(), myArr.end());
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer6, myArr.size());
        AZ_TEST_ASSERT(int_buffer6.capacity() == 10);
        i = 0;
        for (int_ringbuffer_type::iterator iter = int_buffer6.begin(); iter != int_buffer6.end(); ++iter, ++i)
        {
            AZ_TEST_ASSERT(*iter == i);
        }

        // =
        int_buffer1 = int_buffer6;
        AZ_TEST_ASSERT(int_buffer1 == int_buffer6);

        // []
        AZ_TEST_ASSERT(int_buffer5[3] == 3);
        AZ_TEST_ASSERT(int_buffer5[4] == int_buffer5.at(4));

        AZ_TEST_ASSERT(int_buffer5.front() == 0);
        AZ_TEST_ASSERT(int_buffer5.back() == 5);

        // full
        AZ_TEST_ASSERT(int_buffer5.full() == true);
        AZ_TEST_ASSERT(int_buffer6.full() == false);

        // Circular checks
        AZ_TEST_ASSERT(int_buffer5.is_linearized() == true);
        int_ringbuffer_type::array_range arr1 = int_buffer5.array_one();
        int_ringbuffer_type::array_range arr2 = int_buffer5.array_two();
        AZ_TEST_ASSERT(arr1.second == int_buffer5.size());  // we have only 1 linear array
        AZ_TEST_ASSERT(arr2.second == 0);
        AZ_TEST_ASSERT(*arr1.first == 0); // Check that we are pointing to the first elements, which is 0.

        // Overwrite the first 2 elements.
        int_buffer5.push_back(6);
        int_buffer5.push_back(7);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size());
        AZ_TEST_ASSERT(int_buffer5.front() == 2);
        AZ_TEST_ASSERT(int_buffer5.back() == 7);
        arr1 = int_buffer5.array_one();
        arr2 = int_buffer5.array_two();
        AZ_TEST_ASSERT(arr1.second == 4);
        AZ_TEST_ASSERT(*arr1.first == 2);
        AZ_TEST_ASSERT(arr2.second == 2);
        AZ_TEST_ASSERT(*arr2.first == 6);

        // rotate - full buffer
        int_buffer5.rotate(int_buffer5.begin() + 1); // rotate right by 1
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size());
        AZ_TEST_ASSERT(int_buffer5.front() == 3);
        AZ_TEST_ASSERT(int_buffer5.back() == 2);
        arr1 = int_buffer5.array_one();
        arr2 = int_buffer5.array_two();
        AZ_TEST_ASSERT(arr1.second == 3);
        AZ_TEST_ASSERT(*arr1.first == 3);
        AZ_TEST_ASSERT(arr2.second == 3);
        AZ_TEST_ASSERT(*arr2.first == 6);

        // rotate - !full buffer
        int_buffer6.rotate(int_buffer6.begin() + 5);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer6, myArr.size());
        AZ_TEST_ASSERT(int_buffer6.front() == 5);
        AZ_TEST_ASSERT(int_buffer6.back() == 4);
        arr1 = int_buffer6.array_one();
        arr2 = int_buffer6.array_two();
        AZ_TEST_ASSERT(arr1.second == 1);
        AZ_TEST_ASSERT(*arr1.first == 5);
        AZ_TEST_ASSERT(arr2.second == 5);
        AZ_TEST_ASSERT(*arr2.first == 0);

        // linearize
        int_buffer5.linearize();
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size());
        AZ_TEST_ASSERT(int_buffer5.is_linearized());
        AZ_TEST_ASSERT(int_buffer5.front() == 3);
        AZ_TEST_ASSERT(int_buffer5.back() == 2);
        arr1 = int_buffer5.array_one();
        arr2 = int_buffer5.array_two();
        AZ_TEST_ASSERT(arr1.second == 6);
        AZ_TEST_ASSERT(*arr1.first == 3);
        AZ_TEST_ASSERT(arr2.second == 0);

        // resize - grow
        int_buffer5.resize(100, 11);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, 100);
        AZ_TEST_ASSERT(int_buffer5.front() == 3);
        AZ_TEST_ASSERT(int_buffer5.back() == 11);

        // resize - shrink
        int_buffer5.resize(5);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, 5);
        AZ_TEST_ASSERT(int_buffer5.front() == 3);
        AZ_TEST_ASSERT(int_buffer5.back() == 7);

        // swap
        int_buffer5.swap(int_buffer6);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer6, 5);
        AZ_TEST_ASSERT(int_buffer6.front() == 3);
        AZ_TEST_ASSERT(int_buffer6.back() == 7);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size());
        AZ_TEST_ASSERT(int_buffer5.front() == 5);
        AZ_TEST_ASSERT(int_buffer5.back() == 4);

        // push
        int_buffer5.push_back(101);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size() + 1);
        AZ_TEST_ASSERT(int_buffer5.back() == 101);
        int_buffer5.emplace_back();
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size() + 2);

        int_buffer5.emplace_front(201);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size() + 3);
        AZ_TEST_ASSERT(int_buffer5.front() == 201);
        int_buffer5.emplace_front();
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size() + 4);

        // pop
        int_buffer5.pop_front();
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size() + 3);
        AZ_TEST_ASSERT(int_buffer5.front() == 201);
        int_buffer5.pop_back();
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size() + 2);
        AZ_TEST_ASSERT(int_buffer5.back() == 101);

        // insert
        int_buffer5.insert(int_buffer5.begin() + 1, 303);
        AZ_TEST_VALIDATE_RINGBUFFER(int_buffer5, myArr.size() + 3);
        AZ_TEST_ASSERT(int_buffer5[0] == 201);
        AZ_TEST_ASSERT(int_buffer5[1] == 303);

        /*  int_buffer5.insert(int_buffer5.begin(),3,404);
            AZ_TEST_ASSERT(int_buffer5[0]==404);
            AZ_TEST_ASSERT(int_buffer5[3]==201);*/

        // erase
        int_buffer5.erase(int_buffer5.begin() + 1);
        AZ_TEST_ASSERT(int_buffer5[0] == 201);
    }

    TEST_F(Containers, RingBufferReverseIterators)
    {
        using int_ringbuffer_type = AZStd::ring_buffer<int>;
        const int max = 42;
        int_ringbuffer_type rev_buffer(max);
        
        for (int i = 0; i < max; ++i)
        {
            rev_buffer.push_back(i);
        }
        int iteration = 0;
        for (int_ringbuffer_type::const_reverse_iterator rit = rev_buffer.rbegin(); rit != rev_buffer.rend(); ++rit)
        {
            EXPECT_EQ(max - iteration - 1, *rit);
            ++iteration;
        }
    }

    using StackContainerTestFixture = LeakDetectionFixture;

    TEST_F(StackContainerTestFixture, StackEmplaceOperator_SupportsZeroOrMoreArguments)
    {
        using TestPairType = AZStd::pair<int, int>;
        AZStd::stack<TestPairType> testStack;
        testStack.emplace();
        testStack.emplace(1);
        testStack.emplace(2, 3);

        using ContainerType = typename AZStd::stack<TestPairType>::container_type;
        AZStd::stack<TestPairType> expectedStack(ContainerType{ TestPairType{ 0, 0 }, TestPairType{ 1, 0 }, TestPairType{ 2, 3 } });
        EXPECT_EQ(expectedStack, testStack);
    }


    using DequeTestFixture = LeakDetectionFixture;

    TEST_F(DequeTestFixture, RangeConstructors_Succeeds)
    {
        constexpr AZStd::string_view testView = "abc";

        AZStd::deque testDeque(AZStd::from_range, testView);
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));

        testDeque = AZStd::deque(AZStd::from_range, AZStd::vector<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));
        testDeque = AZStd::deque(AZStd::from_range, AZStd::list<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));
        testDeque = AZStd::deque(AZStd::from_range, AZStd::deque<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));
        testDeque = AZStd::deque(AZStd::from_range, AZStd::set<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));
        testDeque = AZStd::deque(AZStd::from_range, AZStd::unordered_set<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));
        testDeque = AZStd::deque(AZStd::from_range, AZStd::fixed_vector<char, 8>{testView.begin(), testView.end()});
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));
        testDeque = AZStd::deque(AZStd::from_range, AZStd::array{ 'a', 'b', 'c' });
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));
        testDeque = AZStd::deque(AZStd::from_range, AZStd::span(testView));
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));

        AZStd::fixed_string<8> testValue(testView);
        testDeque = AZStd::deque(AZStd::from_range, testValue);
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));
        testDeque = AZStd::deque(AZStd::from_range, AZStd::string(testView));
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c'));

        // Test Range views
        testDeque = AZStd::deque(AZStd::from_range, testValue | AZStd::views::transform([](const char elem) -> char { return elem + 1; }));
        EXPECT_THAT(testDeque, ::testing::ElementsAre('b', 'c', 'd'));
    }

    TEST_F(DequeTestFixture, AssignRange_Succeeds)
    {
        constexpr AZStd::string_view testView = "def";
        AZStd::deque testDev{ 'a', 'b', 'c' };
        testDev.assign_range(AZStd::vector<char>{testView.begin(), testView.end()});
        testDev.assign_range(AZStd::vector<char>{testView.begin(), testView.end()});
        EXPECT_THAT(testDev, ::testing::ElementsAre('d', 'e', 'f'));
    }

    TEST_F(DequeTestFixture, InsertRange_Succeeds)
    {
        constexpr AZStd::string_view testView = "abc";
        AZStd::deque testDeque{ 'd', 'e', 'f' };
        testDeque.insert_range(testDeque.begin(), AZStd::vector<char>{testView.begin(), testView.end()});
        testDeque.insert_range(testDeque.end(), testView | AZStd::views::transform([](const char elem) -> char { return elem + 6; }));
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'));
    }

    TEST_F(DequeTestFixture, AppendRange_Succeeds)
    {
        constexpr AZStd::string_view testView = "def";
        AZStd::deque testDeque{ 'a', 'b', 'c' };
        testDeque.append_range(AZStd::vector<char>{testView.begin(), testView.end()});
        testDeque.append_range(testView | AZStd::views::transform([](const char elem) -> char { return elem + 3; }));
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'));
    }

    TEST_F(DequeTestFixture, PrependRange_Succeeds)
    {
        constexpr AZStd::string_view testView = "def";
        AZStd::deque testDeque{ 'g', 'h', 'i' };
        testDeque.prepend_range(AZStd::vector<char>{testView.begin(), testView.end()});
        testDeque.prepend_range(testView | AZStd::views::transform([](const char elem) -> char { return elem + -3; }));
        EXPECT_THAT(testDeque, ::testing::ElementsAre('a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'));
    }
}
