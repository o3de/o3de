/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/parallel/containers/lock_free_stack.h>
#include <AzCore/std/parallel/containers/lock_free_stamped_stack.h>
#include <AzCore/std/parallel/containers/lock_free_intrusive_stack.h>
#include <AzCore/std/parallel/containers/lock_free_intrusive_stamped_stack.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/functional.h>

namespace UnitTest
{
    using namespace AZStd;
    using namespace UnitTestInternal;

    class LockFreeStack
        : public LeakDetectionFixture
    {
    public:
        
#ifdef _DEBUG
        static const int NUM_ITERATIONS = 5000;
#else
        static const int NUM_ITERATIONS = 50000;
#endif
        template <class S>
        void Push(S* stack)
        {
            for (int i = 1; i <= NUM_ITERATIONS; ++i)
            {
                stack->push(i);
                m_total.fetch_add(i);
            }
        }

        template <class S>
        void Pop(S* stack)
        {
            int numPopped = 0;
            while (numPopped != NUM_ITERATIONS)
            {
                int value = 0;
                if (stack->pop(&value))
                {
                    m_total.fetch_sub(value);
                    ++numPopped;
                }
                else
                {
                    // for some schedulers we need the yield otherwise we will deadlock
                    AZStd::this_thread::yield();
                }
            }
        }

        atomic<int> m_total;
    };

    TEST_F(LockFreeStack, LockFreeStack)
    {
        lock_free_stack<int, MyLockFreeAllocator> stack;

        int result = 0;
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop(&result));

        stack.push(20);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop(&result));

        stack.push(20);
        stack.push(30);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop(&result));
        AZ_TEST_ASSERT(result == 30);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop(&result));

        {
            m_total = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeStack::Push<decltype(stack)>, this, &stack));
            AZStd::thread thread1(AZStd::bind(&LockFreeStack::Pop<decltype(stack)>, this, &stack));
            AZStd::thread thread2(AZStd::bind(&LockFreeStack::Push<decltype(stack)>, this, &stack));
            AZStd::thread thread3(AZStd::bind(&LockFreeStack::Pop<decltype(stack)>, this, &stack));
            thread0.join();
            thread1.join();
            thread2.join();
            thread3.join();
            AZ_TEST_ASSERT(m_total == 0);
            AZ_TEST_ASSERT(stack.empty());
        }
    }

    TEST_F(LockFreeStack, LockFreeStampedStack)
    {
        lock_free_stamped_stack<int, MyLockFreeAllocator> stack;

        int result = 0;
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop(&result));

        stack.push(20);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop(&result));

        stack.push(20);
        stack.push(30);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop(&result));
        AZ_TEST_ASSERT(result == 30);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop(&result));
        AZ_TEST_ASSERT(result == 20);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop(&result));

        {
            m_total = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeStack::Push<decltype(stack)>, this, &stack));
            AZStd::thread thread1(AZStd::bind(&LockFreeStack::Pop<decltype(stack)>, this, &stack));
            AZStd::thread thread2(AZStd::bind(&LockFreeStack::Push<decltype(stack)>, this, &stack));
            AZStd::thread thread3(AZStd::bind(&LockFreeStack::Pop<decltype(stack)>, this, &stack));
            thread0.join();
            thread1.join();
            thread2.join();
            thread3.join();
            AZ_TEST_ASSERT(m_total == 0);
            AZ_TEST_ASSERT(stack.empty());
        }
    }

    struct MyStackItem
        : public lock_free_intrusive_stack_node<MyStackItem>
    {
    public:
        MyStackItem() {}
        MyStackItem(int data)
            : m_data(data) {}

        lock_free_intrusive_stack_node<MyStackItem> m_listHook;     // Public member hook.

        int m_data; // This is left public only for testing purpose.
    };

    
    class LockFreeIntrusiveStack
        : public LeakDetectionFixture
    {
    public:
#ifdef _DEBUG
        static const int NUM_ITERATIONS = 5000;
#else
        static const int NUM_ITERATIONS = 50000;
#endif
        template <class S>
        void Push(S* stack, vector<MyStackItem>* items)
        {
            for (int i = 0; i < NUM_ITERATIONS; ++i)
            {
                stack->push((*items)[i]);
                m_total.fetch_add((*items)[i].m_data);
            }
        }

        template <class S>
        void Pop(S* stack)
        {
            int numPopped = 0;
            while (numPopped != NUM_ITERATIONS)
            {
                MyStackItem* item = stack->pop();
                if (item)
                {
                    m_total.fetch_sub(item->m_data);
                    ++numPopped;
                }
            }
        }
        atomic<int> m_total;
    };

    typedef lock_free_intrusive_stack<MyStackItem,
        lock_free_intrusive_stack_base_hook<MyStackItem> > MyIntrusiveStackBase;
    typedef lock_free_intrusive_stack<MyStackItem,
        lock_free_intrusive_stack_member_hook<MyStackItem, & MyStackItem::m_listHook> > MyIntrusiveStackMember;
    typedef lock_free_intrusive_stamped_stack<MyStackItem,
        lock_free_intrusive_stack_base_hook<MyStackItem> > MyIntrusiveStampedStackBase;
    typedef lock_free_intrusive_stamped_stack<MyStackItem,
        lock_free_intrusive_stack_member_hook<MyStackItem, & MyStackItem::m_listHook> > MyIntrusiveStampedStackMember;

    TEST_F(LockFreeIntrusiveStack, MyIntrusiveStackBase)
    {
        MyIntrusiveStackBase stack;

        MyStackItem item1(100);
        MyStackItem item2(200);

        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        stack.push(item1);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item1);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        stack.push(item1);
        stack.push(item2);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item2);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item1);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        vector<MyStackItem> items;
        items.reserve(NUM_ITERATIONS);
        for (int i = 1; i <= NUM_ITERATIONS; ++i)
        {
            items.push_back(MyStackItem(i));
        }

        {
            m_total = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeIntrusiveStack::Push<decltype(stack)>, this, &stack, &items));
            AZStd::thread thread1(AZStd::bind(&LockFreeIntrusiveStack::Pop<decltype(stack)>, this, &stack));
            thread0.join();
            thread1.join();
            AZ_TEST_ASSERT(m_total == 0);
            AZ_TEST_ASSERT(stack.empty());
        }
    }
    

    TEST_F(LockFreeIntrusiveStack, MyIntrusiveStackMember)
    {
        MyIntrusiveStackMember stack;

        MyStackItem item1(100);
        MyStackItem item2(200);

        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        stack.push(item1);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item1);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        stack.push(item1);
        stack.push(item2);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item2);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item1);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        vector<MyStackItem> items;
        items.reserve(NUM_ITERATIONS);
        for (int i = 1; i <= NUM_ITERATIONS; ++i)
        {
            items.push_back(MyStackItem(i));
        }

        {
            m_total = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeIntrusiveStack::Push<decltype(stack)>, this, &stack, &items));
            AZStd::thread thread1(AZStd::bind(&LockFreeIntrusiveStack::Pop<decltype(stack)>, this, &stack));
            thread0.join();
            thread1.join();
            AZ_TEST_ASSERT(m_total == 0);
            AZ_TEST_ASSERT(stack.empty());
        }
    }

    TEST_F(LockFreeIntrusiveStack, MyIntrusiveStampedStackBase)
    {
        MyIntrusiveStampedStackBase stack;

        MyStackItem item1(100);
        MyStackItem item2(200);

        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        stack.push(item1);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item1);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        stack.push(item1);
        stack.push(item2);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item2);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item1);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        vector<MyStackItem> items;
        items.reserve(NUM_ITERATIONS);
        for (int i = 1; i <= NUM_ITERATIONS; ++i)
        {
            items.push_back(MyStackItem(i));
        }

        {
            m_total = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeIntrusiveStack::Push<decltype(stack)>, this, &stack, &items));
            AZStd::thread thread1(AZStd::bind(&LockFreeIntrusiveStack::Pop<decltype(stack)>, this, &stack));
            thread0.join();
            thread1.join();
            AZ_TEST_ASSERT(m_total == 0);
            AZ_TEST_ASSERT(stack.empty());
        }
    }

    TEST_F(LockFreeIntrusiveStack, MyIntrusiveStampedStackMember)
    {
        MyIntrusiveStampedStackMember stack;

        MyStackItem item1(100);
        MyStackItem item2(200);

        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        stack.push(item1);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item1);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        stack.push(item1);
        stack.push(item2);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item2);
        AZ_TEST_ASSERT(!stack.empty());
        AZ_TEST_ASSERT(stack.pop() == &item1);
        AZ_TEST_ASSERT(stack.empty());
        AZ_TEST_ASSERT(!stack.pop());

        vector<MyStackItem> items;
        items.reserve(NUM_ITERATIONS);
        for (int i = 1; i <= NUM_ITERATIONS; ++i)
        {
            items.push_back(MyStackItem(i));
        }

        {
            m_total = 0;
            AZStd::thread thread0(AZStd::bind(&LockFreeIntrusiveStack::Push<decltype(stack)>, this, &stack, &items));
            AZStd::thread thread1(AZStd::bind(&LockFreeIntrusiveStack::Pop<decltype(stack)>, this, &stack));
            thread0.join();
            thread1.join();
            AZ_TEST_ASSERT(m_total == 0);
            AZ_TEST_ASSERT(stack.empty());
        }
    }
}

