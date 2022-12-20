/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/containers/lock_free_queue.h>
#include <AzCore/std/parallel/containers/lock_free_stamped_queue.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>


namespace UnitTest
{
    using namespace AZStd;
    using namespace UnitTestInternal;

    class LockFreeQueue
        : public LeakDetectionFixture
    {
    protected:
#ifdef _DEBUG
        static const int NUM_ITERATIONS = 5000;
#else
        static const int NUM_ITERATIONS = 100000;
#endif
    public:
        template <class Q>
        void Push(Q* queue)
        {
            for (int i = 0; i < NUM_ITERATIONS; ++i)
            {
                queue->push(i);
            }
        }

        template <class Q>
        void Pop(Q* queue)
        {
            int expected = 0;
            while (expected < NUM_ITERATIONS)
            {
                typename Q::value_type value = NUM_ITERATIONS;
                if (queue->pop(&value))
                {
                    if (value == expected)
                    {
                        ++m_counter;
                    }
                    ++expected;
                }
            }
        }

        atomic<int> m_counter;
    };

    TEST_F(LockFreeQueue, LockFreeQueue)
    {
        lock_free_queue<int, MyLockFreeAllocator> queue;
        int result;
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        queue.push(30);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 30);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        {
            m_counter = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeQueue::Push<decltype(queue)>, this, &queue));
            AZStd::thread thread1(AZStd::bind(&LockFreeQueue::Pop<decltype(queue)>, this, &queue));
            thread0.join();
            thread1.join();
            AZ_TEST_ASSERT(m_counter == NUM_ITERATIONS);
            AZ_TEST_ASSERT(queue.empty());
        }
    }


    struct SharedInt {
        SharedInt() : m_ptr(nullptr) {}
        SharedInt(int i) : m_ptr(new int(i)) {}
        bool operator==(const SharedInt& other) 
        {
            if (!m_ptr || !other.m_ptr) 
            { 
                return false; 
            }
            return *m_ptr == *other.m_ptr;
        }
    private:
        AZStd::shared_ptr<int> m_ptr;
    };

    TEST_F(LockFreeQueue, LockFreeQueueNonTrivialDestructor)
    {
        lock_free_queue<SharedInt, MyLockFreeAllocator> queue;
        SharedInt result;
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        queue.push(30);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 30);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        {
            m_counter = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeQueue::Push<decltype(queue)>, this, &queue));
            AZStd::thread thread1(AZStd::bind(&LockFreeQueue::Pop<decltype(queue)>, this, &queue));
            thread0.join();
            thread1.join();
            AZ_TEST_ASSERT(m_counter == NUM_ITERATIONS);
            AZ_TEST_ASSERT(queue.empty());
        }
    }

    TEST_F(LockFreeQueue, LockFreeStampedQueue)
    {
        lock_free_stamped_queue<int, MyLockFreeAllocator> queue;

        int result;
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        queue.push(30);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 30);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        {
            m_counter = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeQueue::Push<decltype(queue)>, this, &queue));
            AZStd::thread thread1(AZStd::bind(&LockFreeQueue::Pop<decltype(queue)>, this, &queue));
            thread0.join();
            thread1.join();
            AZ_TEST_ASSERT(m_counter == NUM_ITERATIONS);
            AZ_TEST_ASSERT(queue.empty());
        }
    }

    TEST_F(LockFreeQueue, LockFreeStampedQueueNonTrivialDestructor)
    {
        lock_free_stamped_queue<SharedInt, MyLockFreeAllocator> queue;

        SharedInt result;
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        queue.push(20);
        queue.push(30);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(!queue.empty());
        AZ_TEST_ASSERT(queue.pop(&result));
        AZ_TEST_ASSERT(result == 30);
        AZ_TEST_ASSERT(queue.empty());
        AZ_TEST_ASSERT(!queue.pop(&result));

        {
            m_counter = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeQueue::Push<decltype(queue)>, this, &queue));
            AZStd::thread thread1(AZStd::bind(&LockFreeQueue::Pop<decltype(queue)>, this, &queue));
            thread0.join();
            thread1.join();

            AZ_TEST_ASSERT(m_counter == NUM_ITERATIONS);
            AZ_TEST_ASSERT(queue.empty());
        }
    }
}
