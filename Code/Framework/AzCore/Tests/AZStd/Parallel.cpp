/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UserTypes.h"

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/combinable.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/std/parallel/spin_mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/parallel/threadbus.h>

#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/delegate/delegate.h>
#include <AzCore/std/chrono/chrono.h>

#include <AzCore/Memory/SystemAllocator.h>

using namespace AZStd;
using namespace AZStd::placeholders;
using namespace UnitTestInternal;

namespace UnitTest
{
    /**
     * Synchronization primitives test.
     */
    TEST(Parallel, Mutex)
    {
        mutex m;
        m.lock();
        m.unlock();
    }

    TEST(Parallel, RecursiveMutex)
    {

        recursive_mutex m1;
        m1.lock();
        AZ_TEST_ASSERT(m1.try_lock());  // we should be able to lock it from the same thread again...
        m1.unlock();
        m1.unlock();
        {
            mutex m2;
            lock_guard<mutex> l(m2);
        }
    }

    TEST(Parallel, Semaphore_Sanity)
    {
        semaphore sema;
        sema.release(1);
        sema.acquire();
    }

    // MARGIN_OF_ERROR_MS: margin of error for waits.
    // This is necessary because timers are not exact.
    // Also, the failure conditions we are looking for are massive failures, for example, asking it to wait
    // 100ms and having it not wait at all!
    // Note that on most platforms, a margin of 2ms and wait time of 20ms was adequate, but
    // there are some platforms that have poor timer resolution.   So we'll greatly increase the margin.
    constexpr AZStd::chrono::milliseconds MARGIN_OF_ERROR_MS(20);

    // This is how long we wait when asked to wait a FULL duration.  This number should be as small as possible
    // for test efficiency while still being significant compared to the margin above.
    constexpr AZStd::chrono::milliseconds WAIT_TIME_MS(60); 

    TEST(Parallel, Semaphore_TryAcquireFor_WaitsMinimumTime)
    {
        // try_acquire_for according to standard is a minimum amount of time.
        // that it should wait for.
        semaphore sema;
        auto minDuration = AZStd::chrono::milliseconds(WAIT_TIME_MS);
        auto minDurationWithMarginForError = minDuration - AZStd::chrono::milliseconds(MARGIN_OF_ERROR_MS);

        auto startTime = AZStd::chrono::system_clock::now();
        
        EXPECT_FALSE(sema.try_acquire_for(minDuration));
        
        auto actualDuration = AZStd::chrono::system_clock::now() - startTime;
        EXPECT_GE(actualDuration, minDurationWithMarginForError);
    }

    TEST(Parallel, Semaphore_TryAcquireUntil_ActuallyWaits)
    {
        // try_acquire_until should not wake up until the time specified
        semaphore sema;
        auto minDuration = AZStd::chrono::milliseconds(WAIT_TIME_MS);
        auto minDurationWithMarginForError = minDuration - AZStd::chrono::milliseconds(MARGIN_OF_ERROR_MS);
        auto startTime = AZStd::chrono::system_clock::now();
        auto absTime = startTime + minDuration;
        
        EXPECT_FALSE(sema.try_acquire_until(absTime));
        
        auto duration = AZStd::chrono::system_clock::now() - startTime;
        EXPECT_GE(duration, minDurationWithMarginForError);
    }

    TEST(Parallel, Semaphore_TryAcquireFor_Signalled_DoesNotWait)
    {
        semaphore sema;

        // this duration should not matter since it should not wait at all so we don't need an error margin.
        auto minDuration = AZStd::chrono::milliseconds(WAIT_TIME_MS);
        auto startTime = AZStd::chrono::system_clock::now();
        sema.release();
        EXPECT_TRUE(sema.try_acquire_for(minDuration));
        
        auto durationSpent = AZStd::chrono::system_clock::now() - startTime;
        EXPECT_LT(durationSpent, minDuration);
    }

    TEST(Parallel, Semaphore_TryAcquireUntil_Signalled_DoesNotWait)
    {
        semaphore sema;
        // we should not wait all at here, since we start with it already signalled.
        auto minDuration = AZStd::chrono::milliseconds(WAIT_TIME_MS);
        auto startTime = AZStd::chrono::system_clock::now();
        auto absTime = startTime + minDuration;
        sema.release();
        EXPECT_TRUE(sema.try_acquire_until(absTime));
        
        auto duration = AZStd::chrono::system_clock::now() - startTime;
        EXPECT_LT(duration, minDuration);
    }

    TEST(Parallel, BinarySemaphore)
    {
        binary_semaphore event;
        event.release();
        event.acquire();
    }

    TEST(Parallel, SpinMutex)
    {

        spin_mutex sm;
        sm.lock();
        sm.unlock();
    }

    /**
     * Thread test
     */
    class Parallel_Thread
        : public AllocatorsFixture
    {
        int m_data;
        int m_dataMax;

        static const int m_threadStackSize = 32 * 1024;
        thread_desc      m_desc[3];
        int m_numThreadDesc = 0;
    public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
        }

        void TearDown() override
        {
            AllocatorsFixture::TearDown();
        }

        void increment_data()
        {
            while (m_data < m_dataMax)
            {
                m_data++;
            }
        }

        void sleep_thread(chrono::milliseconds time)
        {
            this_thread::sleep_for(time);
        }

        void do_nothing()
        {}

        void test_thread_id_for_default_constructed_thread_is_default_constructed_id()
        {
            AZStd::thread t;
            AZ_TEST_ASSERT(t.get_id() == AZStd::thread::id());
        }

        void test_thread_id_for_running_thread_is_not_default_constructed_id()
        {
            const thread_desc desc = m_numThreadDesc ? m_desc[0] : thread_desc{};
            AZStd::thread t(desc, AZStd::bind(&Parallel_Thread::do_nothing, this));
            AZ_TEST_ASSERT(t.get_id() != AZStd::thread::id());
            t.join();
        }

        void test_different_threads_have_different_ids()
        {
            const thread_desc desc1 = m_numThreadDesc ? m_desc[0] : thread_desc{};
            const thread_desc desc2 = m_numThreadDesc ? m_desc[1] : thread_desc{};
            AZStd::thread t(desc1, AZStd::bind(&Parallel_Thread::do_nothing, this));
            AZStd::thread t2(desc2, AZStd::bind(&Parallel_Thread::do_nothing, this));
            AZ_TEST_ASSERT(t.get_id() != t2.get_id());
            t.join();
            t2.join();
        }

        void test_thread_ids_have_a_total_order()
        {
            const thread_desc desc1 = m_numThreadDesc ? m_desc[0] : thread_desc{};
            const thread_desc desc2 = m_numThreadDesc ? m_desc[1] : thread_desc{};
            const thread_desc desc3 = m_numThreadDesc ? m_desc[2] : thread_desc{};

            AZStd::thread t(desc1, AZStd::bind(&Parallel_Thread::do_nothing, this));
            AZStd::thread t2(desc2, AZStd::bind(&Parallel_Thread::do_nothing, this));
            AZStd::thread t3(desc3, AZStd::bind(&Parallel_Thread::do_nothing, this));
            AZ_TEST_ASSERT(t.get_id() != t2.get_id());
            AZ_TEST_ASSERT(t.get_id() != t3.get_id());
            AZ_TEST_ASSERT(t2.get_id() != t3.get_id());

            AZ_TEST_ASSERT((t.get_id() < t2.get_id()) != (t2.get_id() < t.get_id()));
            AZ_TEST_ASSERT((t.get_id() < t3.get_id()) != (t3.get_id() < t.get_id()));
            AZ_TEST_ASSERT((t2.get_id() < t3.get_id()) != (t3.get_id() < t2.get_id()));

            AZ_TEST_ASSERT((t.get_id() > t2.get_id()) != (t2.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t.get_id() > t3.get_id()) != (t3.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t2.get_id() > t3.get_id()) != (t3.get_id() > t2.get_id()));

            AZ_TEST_ASSERT((t.get_id() < t2.get_id()) == (t2.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t2.get_id() < t.get_id()) == (t.get_id() > t2.get_id()));
            AZ_TEST_ASSERT((t.get_id() < t3.get_id()) == (t3.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t3.get_id() < t.get_id()) == (t.get_id() > t3.get_id()));
            AZ_TEST_ASSERT((t2.get_id() < t3.get_id()) == (t3.get_id() > t2.get_id()));
            AZ_TEST_ASSERT((t3.get_id() < t2.get_id()) == (t2.get_id() > t3.get_id()));

            AZ_TEST_ASSERT((t.get_id() < t2.get_id()) == (t2.get_id() >= t.get_id()));
            AZ_TEST_ASSERT((t2.get_id() < t.get_id()) == (t.get_id() >= t2.get_id()));
            AZ_TEST_ASSERT((t.get_id() < t3.get_id()) == (t3.get_id() >= t.get_id()));
            AZ_TEST_ASSERT((t3.get_id() < t.get_id()) == (t.get_id() >= t3.get_id()));
            AZ_TEST_ASSERT((t2.get_id() < t3.get_id()) == (t3.get_id() >= t2.get_id()));
            AZ_TEST_ASSERT((t3.get_id() < t2.get_id()) == (t2.get_id() >= t3.get_id()));

            AZ_TEST_ASSERT((t.get_id() <= t2.get_id()) == (t2.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t2.get_id() <= t.get_id()) == (t.get_id() > t2.get_id()));
            AZ_TEST_ASSERT((t.get_id() <= t3.get_id()) == (t3.get_id() > t.get_id()));
            AZ_TEST_ASSERT((t3.get_id() <= t.get_id()) == (t.get_id() > t3.get_id()));
            AZ_TEST_ASSERT((t2.get_id() <= t3.get_id()) == (t3.get_id() > t2.get_id()));
            AZ_TEST_ASSERT((t3.get_id() <= t2.get_id()) == (t2.get_id() > t3.get_id()));

            if ((t.get_id() < t2.get_id()) && (t2.get_id() < t3.get_id()))
            {
                AZ_TEST_ASSERT(t.get_id() < t3.get_id());
            }
            else if ((t.get_id() < t3.get_id()) && (t3.get_id() < t2.get_id()))
            {
                AZ_TEST_ASSERT(t.get_id() < t2.get_id());
            }
            else if ((t2.get_id() < t3.get_id()) && (t3.get_id() < t.get_id()))
            {
                AZ_TEST_ASSERT(t2.get_id() < t.get_id());
            }
            else if ((t2.get_id() < t.get_id()) && (t.get_id() < t3.get_id()))
            {
                AZ_TEST_ASSERT(t2.get_id() < t3.get_id());
            }
            else if ((t3.get_id() < t.get_id()) && (t.get_id() < t2.get_id()))
            {
                AZ_TEST_ASSERT(t3.get_id() < t2.get_id());
            }
            else if ((t3.get_id() < t2.get_id()) && (t2.get_id() < t.get_id()))
            {
                AZ_TEST_ASSERT(t3.get_id() < t.get_id());
            }
            else
            {
                AZ_TEST_ASSERT(false);
            }

            AZStd::thread::id default_id;

            AZ_TEST_ASSERT(default_id < t.get_id());
            AZ_TEST_ASSERT(default_id < t2.get_id());
            AZ_TEST_ASSERT(default_id < t3.get_id());

            AZ_TEST_ASSERT(default_id <= t.get_id());
            AZ_TEST_ASSERT(default_id <= t2.get_id());
            AZ_TEST_ASSERT(default_id <= t3.get_id());

            AZ_TEST_ASSERT(!(default_id > t.get_id()));
            AZ_TEST_ASSERT(!(default_id > t2.get_id()));
            AZ_TEST_ASSERT(!(default_id > t3.get_id()));

            AZ_TEST_ASSERT(!(default_id >= t.get_id()));
            AZ_TEST_ASSERT(!(default_id >= t2.get_id()));
            AZ_TEST_ASSERT(!(default_id >= t3.get_id()));

            t.join();
            t2.join();
            t3.join();
        }

        void get_thread_id(AZStd::thread::id* id)
        {
            *id = this_thread::get_id();
        }

        void test_thread_id_of_running_thread_returned_by_this_thread_get_id()
        {
            const thread_desc desc1 = m_numThreadDesc ? m_desc[0] : thread_desc{};

            AZStd::thread::id id;
            AZStd::thread t(desc1, AZStd::bind(&Parallel_Thread::get_thread_id, this, &id));
            AZStd::thread::id t_id = t.get_id();
            t.join();
            AZ_TEST_ASSERT(id == t_id);
        }


        class MfTest
        {
        public:
            mutable unsigned int m_hash;

            MfTest()
                : m_hash(0) {}

            int f0() { f1(17); return 0; }
            int g0() const { g1(17); return 0; }

            int f1(int a1) { m_hash = (m_hash * 17041 + a1) % 32768; return 0; }
            int g1(int a1) const { m_hash = (m_hash * 17041 + a1 * 2) % 32768; return 0; }

            int f2(int a1, int a2) { f1(a1); f1(a2); return 0; }
            int g2(int a1, int a2) const { g1(a1); g1(a2); return 0; }

            int f3(int a1, int a2, int a3) { f2(a1, a2); f1(a3); return 0; }
            int g3(int a1, int a2, int a3) const { g2(a1, a2); g1(a3); return 0; }

            int f4(int a1, int a2, int a3, int a4) { f3(a1, a2, a3); f1(a4); return 0; }
            int g4(int a1, int a2, int a3, int a4) const { g3(a1, a2, a3); g1(a4); return 0; }

            int f5(int a1, int a2, int a3, int a4, int a5) { f4(a1, a2, a3, a4); f1(a5); return 0; }
            int g5(int a1, int a2, int a3, int a4, int a5) const { g4(a1, a2, a3, a4); g1(a5); return 0; }

            int f6(int a1, int a2, int a3, int a4, int a5, int a6) { f5(a1, a2, a3, a4, a5); f1(a6); return 0; }
            int g6(int a1, int a2, int a3, int a4, int a5, int a6) const { g5(a1, a2, a3, a4, a5); g1(a6); return 0; }

            int f7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) { f6(a1, a2, a3, a4, a5, a6); f1(a7); return 0; }
            int g7(int a1, int a2, int a3, int a4, int a5, int a6, int a7) const { g6(a1, a2, a3, a4, a5, a6); g1(a7); return 0; }

            int f8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) { f7(a1, a2, a3, a4, a5, a6, a7); f1(a8); return 0; }
            int g8(int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8) const { g7(a1, a2, a3, a4, a5, a6, a7); g1(a8); return 0; }
        };

        void do_nothing_id(AZStd::thread::id* my_id)
        {
            *my_id = this_thread::get_id();
        }

        void test_move_on_construction()
        {
            const thread_desc desc1 = m_numThreadDesc ? m_desc[0] : thread_desc{};
            AZStd::thread::id the_id;
            AZStd::thread x;
            x = AZStd::thread(desc1, AZStd::bind(&Parallel_Thread::do_nothing_id, this, &the_id));
            AZStd::thread::id x_id = x.get_id();
            x.join();
            AZ_TEST_ASSERT(the_id == x_id);
        }

        AZStd::thread make_thread(AZStd::thread::id* the_id)
        {
            const thread_desc desc1 = m_numThreadDesc ? m_desc[0] : thread_desc{};
            return AZStd::thread(desc1, AZStd::bind(&Parallel_Thread::do_nothing_id, this, the_id));
        }

        void test_move_from_function_return()
        {
            AZStd::thread::id the_id;
            AZStd::thread x;
            x = make_thread(&the_id);
            AZStd::thread::id x_id = x.get_id();
            x.join();
            AZ_TEST_ASSERT(the_id == x_id);
        }

        /*void test_move_from_function_return_lvalue()
        {
            thread::id the_id;
            thread x=make_thread_return_lvalue(&the_id);
            thread::id x_id=x.get_id();
            x.join();
            AZ_TEST_ASSERT(the_id==x_id);
        }

        void test_move_assign()
        {
            thread::id the_id;
            thread x(do_nothing_id,&the_id);
            thread y;
            y=AZStd::move(x);
            thread::id y_id=y.get_id();
            y.join();
            AZ_TEST_ASSERT(the_id==y_id);
        }*/

        void simple_thread()
        {
            m_data = 999;
        }

        void comparison_thread(AZStd::thread::id parent)
        {
            AZStd::thread::id const my_id = this_thread::get_id();

            AZ_TEST_ASSERT(my_id != parent);
            AZStd::thread::id const my_id2 = this_thread::get_id();
            AZ_TEST_ASSERT(my_id == my_id2);

            AZStd::thread::id const no_thread_id = AZStd::thread::id();
            AZ_TEST_ASSERT(my_id != no_thread_id);
        }

        void do_test_creation()
        {
            const thread_desc desc1 = m_numThreadDesc ? m_desc[0] : thread_desc{};
            m_data = 0;
            AZStd::thread t(desc1, AZStd::bind(&Parallel_Thread::simple_thread, this));
            t.join();
            AZ_TEST_ASSERT(m_data == 999);
        }

        void test_creation()
        {
            //timed_test(&do_test_creation, 1);
            do_test_creation();
        }

        void do_test_id_comparison()
        {
            const thread_desc desc1 = m_numThreadDesc ? m_desc[0] : thread_desc{};
            AZStd::thread::id self = this_thread::get_id();
            AZStd::thread thrd(desc1, AZStd::bind(&Parallel_Thread::comparison_thread, this, self));
            thrd.join();
        }

        void test_id_comparison()
        {
            //timed_test(&do_test_id_comparison, 1);
            do_test_id_comparison();
        }

        struct non_copyable_functor
        {
            unsigned value;

            non_copyable_functor()
                : value(0)
            {}

            void operator()()
            {
                value = 999;
            }
        private:
            non_copyable_functor(const non_copyable_functor&);
            non_copyable_functor& operator=(const non_copyable_functor&);
        };

        void do_test_creation_through_reference_wrapper()
        {
            const thread_desc desc1 = m_numThreadDesc ? m_desc[0] : thread_desc{};
            non_copyable_functor f;

            AZStd::thread thrd(desc1, AZStd::ref(f));
            thrd.join();
            AZ_TEST_ASSERT(f.value == 999);
        }

        void test_creation_through_reference_wrapper()
        {
            do_test_creation_through_reference_wrapper();
        }

        void test_swap()
        {
            const thread_desc desc1 = m_numThreadDesc ? m_desc[0] : thread_desc{};
            const thread_desc desc2 = m_numThreadDesc ? m_desc[1] : thread_desc{};
            AZStd::thread t(desc1, AZStd::bind(&Parallel_Thread::simple_thread, this));
            AZStd::thread t2(desc2, AZStd::bind(&Parallel_Thread::simple_thread, this));
            AZStd::thread::id id1 = t.get_id();
            AZStd::thread::id id2 = t2.get_id();

            t.swap(t2);
            AZ_TEST_ASSERT(t.get_id() == id2);
            AZ_TEST_ASSERT(t2.get_id() == id1);

            swap(t, t2);
            AZ_TEST_ASSERT(t.get_id() == id1);
            AZ_TEST_ASSERT(t2.get_id() == id2);

            t.detach();
            t2.detach();
        }

        void run()
        {
            const thread_desc desc1 = m_numThreadDesc ? m_desc[0] : thread_desc{};

            // We need to have at least one processor
            AZ_TEST_ASSERT(AZStd::thread::hardware_concurrency() >= 1);

            // Create thread to increment data till we need to
            m_data = 0;
            m_dataMax = 10;
            AZStd::thread tr(desc1, AZStd::bind(&Parallel_Thread::increment_data, this));
            tr.join();
            AZ_TEST_ASSERT(m_data == m_dataMax);

            m_data = 0;
            AZStd::thread trDel(desc1, make_delegate(this, &Parallel_Thread::increment_data));
            trDel.join();
            AZ_TEST_ASSERT(m_data == m_dataMax);

            chrono::system_clock::time_point startTime = chrono::system_clock::now();
            {
                AZStd::thread tr1(desc1, AZStd::bind(&Parallel_Thread::sleep_thread, this, chrono::milliseconds(100)));
                tr1.join();
            }
            auto sleepTime = chrono::system_clock::now() - startTime;
            //printf("\nSleeptime: %d Ms\n",(unsigned int)  ());
            // On Windows we use Sleep. Sleep is dependent on MM timers.
            // 99000 can be used only if we support 1 ms resolution timeGetDevCaps() and we set it timeBeginPeriod(1) timeEndPeriod(1)
            // We will need to drag mmsystem.h and we don't really need to test the OS jus our math.
            AZ_TEST_ASSERT(sleepTime.count() >= /*99000*/ 50000);

            //////////////////////////////////////////////////////////////////////////
            test_creation();
            test_id_comparison();
            test_creation_through_reference_wrapper();
            test_swap();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Thread id
            test_thread_id_for_default_constructed_thread_is_default_constructed_id();
            test_thread_id_for_running_thread_is_not_default_constructed_id();
            test_different_threads_have_different_ids();
            test_thread_ids_have_a_total_order();
            test_thread_id_of_running_thread_returned_by_this_thread_get_id();
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Member function tests
            // 0
            {
                MfTest x;
                AZStd::function<void ()> func = AZStd::bind(&MfTest::f0, &x);
                AZStd::thread(desc1, func).join();
                func = AZStd::bind(&MfTest::f0, AZStd::ref(x));
                AZStd::thread(desc1, func).join();
                func = AZStd::bind(&MfTest::g0, &x);
                AZStd::thread(desc1, func).join();
                func = AZStd::bind(&MfTest::g0, x);
                AZStd::thread(desc1, func).join();
                func = AZStd::bind(&MfTest::g0, AZStd::ref(x));
                AZStd::thread(desc1, func).join();

                //// 1
                //thread( AZStd::bind(desc1, &MfTest::f1, &x, 1)).join();
                //thread( AZStd::bind(desc1, &MfTest::f1, AZStd::ref(x), 1)).join();
                //thread( AZStd::bind(desc1, &MfTest::g1, &x, 1)).join();
                //thread( AZStd::bind(desc1, &MfTest::g1, x, 1)).join();
                //thread( AZStd::bind(desc1, &MfTest::g1, AZStd::ref(x), 1)).join();

                //// 2
                //thread( AZStd::bind(desc1, &MfTest::f2, &x, 1, 2)).join();
                //thread( AZStd::bind(desc1, &MfTest::f2, AZStd::ref(x), 1, 2)).join();
                //thread( AZStd::bind(desc1, &MfTest::g2, &x, 1, 2)).join();
                //thread( AZStd::bind(desc1, &MfTest::g2, x, 1, 2)).join();
                //thread( AZStd::bind(desc1, &MfTest::g2, AZStd::ref(x), 1, 2)).join();

                //// 3
                //thread( AZStd::bind(desc1, &MfTest::f3, &x, 1, 2, 3)).join();
                //thread( AZStd::bind(desc1, &MfTest::f3, AZStd::ref(x), 1, 2, 3)).join();
                //thread( AZStd::bind(desc1, &MfTest::g3, &x, 1, 2, 3)).join();
                //thread( AZStd::bind(desc1, &MfTest::g3, x, 1, 2, 3)).join();
                //thread( AZStd::bind(desc1, &MfTest::g3, AZStd::ref(x), 1, 2, 3)).join();

                //// 4
                //thread( AZStd::bind(desc1, &MfTest::f4, &x, 1, 2, 3, 4)).join();
                //thread( AZStd::bind(desc1, &MfTest::f4, AZStd::ref(x), 1, 2, 3, 4)).join();
                //thread( AZStd::bind(desc1, &MfTest::g4, &x, 1, 2, 3, 4)).join();
                //thread( AZStd::bind(desc1, &MfTest::g4, x, 1, 2, 3, 4)).join();
                //thread( AZStd::bind(desc1, &MfTest::g4, AZStd::ref(x), 1, 2, 3, 4)).join();

                //// 5
                //thread( AZStd::bind(desc1, &MfTest::f5, &x, 1, 2, 3, 4, 5)).join();
                //thread( AZStd::bind(desc1, &MfTest::f5, AZStd::ref(x), 1, 2, 3, 4, 5)).join();
                //thread( AZStd::bind(desc1, &MfTest::g5, &x, 1, 2, 3, 4, 5)).join();
                //thread( AZStd::bind(desc1, &MfTest::g5, x, 1, 2, 3, 4, 5)).join();
                //thread( AZStd::bind(desc1, &MfTest::g5, AZStd::ref(x), 1, 2, 3, 4, 5)).join();

                //// 6
                //thread( AZStd::bind(desc1, &MfTest::f6, &x, 1, 2, 3, 4, 5, 6)).join();
                //thread( AZStd::bind(desc1, &MfTest::f6, AZStd::ref(x), 1, 2, 3, 4, 5, 6)).join();
                //thread( AZStd::bind(desc1, &MfTest::g6, &x, 1, 2, 3, 4, 5, 6)).join();
                //thread( AZStd::bind(desc1, &MfTest::g6, x, 1, 2, 3, 4, 5, 6)).join();
                //thread( AZStd::bind(desc1, &MfTest::g6, AZStd::ref(x), 1, 2, 3, 4, 5, 6)).join();

                //// 7
                //thread( AZStd::bind(desc1, &MfTest::f7, &x, 1, 2, 3, 4, 5, 6, 7)).join();
                //thread( AZStd::bind(desc1, &MfTest::f7, AZStd::ref(x), 1, 2, 3, 4, 5, 6, 7)).join();
                //thread( AZStd::bind(desc1, &MfTest::g7, &x, 1, 2, 3, 4, 5, 6, 7)).join();
                //thread( AZStd::bind(desc1, &MfTest::g7, x, 1, 2, 3, 4, 5, 6, 7)).join();
                //thread( AZStd::bind(desc1, &MfTest::g7, AZStd::ref(x), 1, 2, 3, 4, 5, 6, 7)).join();

                //// 8
                //thread( AZStd::bind(desc1, &MfTest::f8, &x, 1, 2, 3, 4, 5, 6, 7, 8)).join();
                //thread( AZStd::bind(desc1, &MfTest::f8, AZStd::ref(x), 1, 2, 3, 4, 5, 6, 7, 8)).join();
                //thread( AZStd::bind(desc1, &MfTest::g8, &x, 1, 2, 3, 4, 5, 6, 7, 8)).join();
                //thread( AZStd::bind(desc1, &MfTest::g8, x, 1, 2, 3, 4, 5, 6, 7, 8)).join();
                //thread( AZStd::bind(desc1, &MfTest::g8, AZStd::ref(x), 1, 2, 3, 4, 5, 6, 7, 8)).join();

                AZ_TEST_ASSERT(x.m_hash == 1366);
            }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // Move
            test_move_on_construction();

            test_move_from_function_return();
            //////////////////////////////////////////////////////////////////////////
        }
    };

#if AZ_TRAIT_DISABLE_ASSET_JOB_PARALLEL_TESTS
    TEST_F(Parallel_Thread, DISABLED_Test)
#else 
    TEST_F(Parallel_Thread, Test)
#endif // AZ_TRAIT_DISABLE_ASSET_JOB_PARALLEL_TESTS
    {
        run();
    }

    TEST_F(Parallel_Thread, Hashable)
    {
        constexpr size_t ThreadCount = 100;

        // Make sure threadids can be added to a map.
        AZStd::vector<AZStd::thread*> threadVector;
        AZStd::unordered_map<AZStd::thread_id, AZStd::thread*> threadMap;

        // Create a bunch of threads and add them to a map
        for (uint32_t i = 0; i < ThreadCount; ++i)
        {
            AZStd::thread* thread = new AZStd::thread([i]() { return i; });
            threadVector.push_back(thread);
            threadMap[thread->get_id()] = thread;
        }

        // Check and make sure they threads can be found by id and match the ones created.
        for (uint32_t i = 0; i < ThreadCount; ++i)
        {
            AZStd::thread* thread = threadVector.at(i);
            EXPECT_TRUE(threadMap.at(thread->get_id()) == thread);
        }

        // Clean up the threads
        AZStd::for_each(threadVector.begin(), threadVector.end(), 
            [](AZStd::thread* thread)
            {
                thread->join();
                delete thread;
            }
        );
    }

    class Parallel_Combinable
        : public AllocatorsFixture
    {
    public:
        void run()
        {
            //initialize with default value
            {
                combinable<TestStruct> c;
                TestStruct& s = c.local();
                AZ_TEST_ASSERT(s.m_x == 42);
            }

            //detect first initialization
            {
                combinable<int> c;
                bool exists;
                int& v1 = c.local(exists);
                AZ_TEST_ASSERT(!exists);
                v1 = 42;

                int& v2 = c.local(exists);
                AZ_TEST_ASSERT(exists);
                AZ_TEST_ASSERT(v2 == 42);

                int& v3 = c.local();
                AZ_TEST_ASSERT(v3 == 42);
            }

            //custom initializer
            {
                combinable<int> c(&Initializer);
                AZ_TEST_ASSERT(c.local() == 43);
            }

            //clear
            {
                combinable<int> c(&Initializer);
                bool exists;
                int& v1 = c.local(exists);
                AZ_TEST_ASSERT(v1 == 43);
                AZ_TEST_ASSERT(!exists);
                v1 = 44;

                c.clear();
                int& v2 = c.local(exists);
                AZ_TEST_ASSERT(v2 == 43);
                AZ_TEST_ASSERT(!exists);
            }

            //copy constructor and assignment
            {
                combinable<int> c1, c2;
                int& v = c1.local();
                v = 45;

                combinable<int> c3(c1);
                AZ_TEST_ASSERT(c3.local() == 45);

                c2 = c1;
                AZ_TEST_ASSERT(c2.local() == 45);
            }

            //combine
            {
                combinable<int> c(&Initializer);

                //default value when no other values
                AZ_TEST_ASSERT(c.combine(plus<int>()) == 43);

                c.local() = 50;
                AZ_TEST_ASSERT(c.combine(plus<int>()) == 50);
            }

            //combine_each
            {
                combinable<int> c(&Initializer);

                m_numCombinerCalls = 0;
                c.combine_each(bind(&Parallel_Combinable::MyCombiner, this, _1));
                AZ_TEST_ASSERT(m_numCombinerCalls == 0);

                m_numCombinerCalls = 0;
                m_combinerTotal = 0;
                c.local() = 50;
                c.combine_each(bind(&Parallel_Combinable::MyCombiner, this, _1));
                AZ_TEST_ASSERT(m_numCombinerCalls == 1);
                AZ_TEST_ASSERT(m_combinerTotal == 50);
            }

            //multithread test
            {
                AZStd::thread_desc desc;
                desc.m_name = "Test Thread 1";
                AZStd::thread t1(bind(&Parallel_Combinable::MyThreadFunc, this, 0, 10), &desc);
                desc.m_name = "Test Thread 2";
                AZStd::thread t2(bind(&Parallel_Combinable::MyThreadFunc, this, 10, 20), &desc);
                desc.m_name = "Test Thread 3";
                AZStd::thread t3(bind(&Parallel_Combinable::MyThreadFunc, this, 20, 500), &desc);
                desc.m_name = "Test Thread 4";
                AZStd::thread t4(bind(&Parallel_Combinable::MyThreadFunc, this, 500, 510), &desc);
                desc.m_name = "Test Thread 5";
                AZStd::thread t5(bind(&Parallel_Combinable::MyThreadFunc, this, 510, 2001), &desc);

                t1.join();
                t2.join();
                t3.join();
                t4.join();
                t5.join();

                m_numCombinerCalls = 0;
                m_combinerTotal = 0;
                m_threadCombinable.combine_each(bind(&Parallel_Combinable::MyCombiner, this, _1));
                AZ_TEST_ASSERT(m_numCombinerCalls == 5);
                AZ_TEST_ASSERT(m_combinerTotal == 2001000);

                AZ_TEST_ASSERT(m_threadCombinable.combine(plus<int>()) == 2001000);

                m_threadCombinable.clear();
            }
        }

        static int Initializer()
        {
            return 43;
        }

        void MyThreadFunc(int start, int end)
        {
            int& v = m_threadCombinable.local();
            v = 0;
            for (int i = start; i < end; ++i)
            {
                v += i;
            }
        }

        void MyCombiner(int v)
        {
            ++m_numCombinerCalls;
            m_combinerTotal += v;
        }

        int m_numCombinerCalls;
        int m_combinerTotal;

        combinable<int> m_threadCombinable;

        struct TestStruct
        {
            TestStruct()
                : m_x(42) { }
            int m_x;
        };
    };

    TEST_F(Parallel_Combinable, Test)
    {
        run();
    }

    class Parallel_SharedMutex
        : public AllocatorsFixture
    {
    public:
        static const int s_numOfReaders = 4;
        shared_mutex m_access;
        unsigned int m_readSum[s_numOfReaders];
        unsigned int m_currentValue;

        void Reader(int index)
        {
            unsigned int lastCurrentValue = 0;
            while (true)
            {
                {
                    // get shared access
                    shared_lock<shared_mutex> lock(m_access);
                    // now we have shared access
                    if (lastCurrentValue != m_currentValue)
                    {
                        lastCurrentValue = m_currentValue;

                        m_readSum[index] += lastCurrentValue;

                        if (m_currentValue == 100)
                        {
                            break;
                        }
                    }
                }

                this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            }
        }

        void Writer()
        {
            while (m_currentValue < 100)
            {
                {
                    lock_guard<shared_mutex> lock(m_access);
                    // now we have exclusive access
                    
                    // m_currentValue must be checked within the mutex as it is possible that
                    // the other writer thread incremented the m_currentValue to 100 between the check of
                    // the while loop condition and the acquiring of the shared_mutex exclusive lock
                    if (m_currentValue < 100)
                    {
                        unsigned int currentValue = m_currentValue;
                        m_currentValue = currentValue + 1;
                    }
                }

                this_thread::sleep_for(AZStd::chrono::milliseconds(10));
            }
        }

        void run()
        {
            // basic operations
            {
                shared_mutex rwlock;

                // try exclusive lock
                EXPECT_TRUE(rwlock.try_lock());
                rwlock.unlock();

                rwlock.lock(); // get the exclusive lock
                // while exclusive lock is taken nobody else can get a lock
                EXPECT_FALSE(rwlock.try_lock());
                EXPECT_FALSE(rwlock.try_lock_shared());
                rwlock.unlock();

                // try shared lock
                EXPECT_TRUE(rwlock.try_lock_shared());
                rwlock.unlock_shared();

                rwlock.lock_shared(); // get the shared lock
                EXPECT_TRUE(rwlock.try_lock_shared()); // make sure we can have multiple shared locks
                rwlock.unlock_shared();
                rwlock.unlock_shared();
            }

            // spin threads and run test validity of operations
            {
                m_currentValue = 0;
                memset(m_readSum, 0, sizeof(unsigned int) * AZ_ARRAY_SIZE(m_readSum));

                AZStd::thread_desc desc;
                desc.m_name = "Test Reader 1";
                AZStd::thread t1(bind(&Parallel_SharedMutex::Reader, this, 0), &desc);
                desc.m_name = "Test Reader 2";
                AZStd::thread t2(bind(&Parallel_SharedMutex::Reader, this, 1), &desc);
                desc.m_name = "Test Reader 3";
                AZStd::thread t3(bind(&Parallel_SharedMutex::Reader, this, 2), &desc);
                desc.m_name = "Test Reader 4";
                AZStd::thread t4(bind(&Parallel_SharedMutex::Reader, this, 3), &desc);
                desc.m_name = "Test Writer 1";
                AZStd::thread t5(bind(&Parallel_SharedMutex::Writer, this), &desc);
                desc.m_name = "Test Writer 2";
                AZStd::thread t6(bind(&Parallel_SharedMutex::Writer, this), &desc);

                t1.join();
                t2.join();
                t3.join();
                t4.join();
                t5.join();
                t6.join();

                EXPECT_EQ(100, m_currentValue);
                // Check for the range of the sums as we don't guarantee adding all numbers.
                // The minimum value the range of sums for each thread is 100.
                // This occurs in the case where the Reader threads are all starved, while the
                // writer threads increments the m_currentValue to 100.
                // Afterwards the reader threads grabs the shared_mutex and reads the value of 100 from m_currentValue
                // and then finishes the thread execution
                EXPECT_GE(m_readSum[0], 100U);
                EXPECT_LE(m_readSum[0], 5050U);
                EXPECT_GE(m_readSum[1], 100U);
                EXPECT_LE(m_readSum[1], 5050U);
                EXPECT_GE(m_readSum[2], 100U);
                EXPECT_LE(m_readSum[2], 5050U);
                EXPECT_GE(m_readSum[3], 100U);
                EXPECT_LE(m_readSum[3], 5050U);
            }
        }
    };

    TEST_F(Parallel_SharedMutex, Test)
    {
        run();
    }

    class ConditionVariable
        : public AllocatorsFixture
    {};
    
    TEST_F(ConditionVariable, NotifyOneSingleWait)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_int i(0);
        AZStd::atomic_bool done(false);
        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            cv.wait(lock, [&]{ return i == 1; });
            EXPECT_EQ(1, i);
            done = true;
        };

        auto signal = [&]()
        {
            cv.notify_one();
            EXPECT_EQ(0, i);
            EXPECT_FALSE(done);

            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            i = 1;
            while (!done)
            {
                lock.unlock();
                cv.notify_one();
                lock.lock();
            }            
        };

        EXPECT_EQ(0, i);
        EXPECT_FALSE(done);

        AZStd::thread waitThread1(wait);
        AZStd::thread signalThread(signal);
        waitThread1.join();
        signalThread.join();

        EXPECT_EQ(1, i);
        EXPECT_TRUE(done);
    }

    TEST_F(ConditionVariable, NotifyOneMultipleWait)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_int i(0);
        AZStd::atomic_bool done1(false);
        AZStd::atomic_bool done2(false);
        auto wait1 = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            cv.wait(lock, [&] { return i == 1; });
            EXPECT_EQ(1, i);
            done1 = true;
        };

        auto wait2 = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            cv.wait(lock, [&] { return i == 1; });
            EXPECT_EQ(1, i);
            done2 = true;
        };

        auto signal = [&]()
        {
            cv.notify_one();
            EXPECT_EQ(0, i);
            EXPECT_FALSE(done1);
            EXPECT_FALSE(done2);

            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            i = 1;
            while (!(done1 && done2))
            {
                lock.unlock();
                cv.notify_one();
                lock.lock();
            }
        };

        EXPECT_EQ(0, i);
        EXPECT_FALSE(done1);
        EXPECT_FALSE(done2);

        AZStd::thread waitThread1(wait1);
        AZStd::thread waitThread2(wait2);
        AZStd::thread signalThread(signal);
        waitThread1.join();
        waitThread2.join();
        signalThread.join();

        EXPECT_EQ(1, i);
        EXPECT_TRUE(done1);
        EXPECT_TRUE(done2);
    }

    TEST_F(ConditionVariable, NotifyAll)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_int i(0);
        
        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            cv.wait(lock, [&] { return i == 1; });
        };

        auto signal = [&]()
        {
            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));
            {
                AZStd::lock_guard<AZStd::mutex> lock(cv_mutex);
                i = 0;
            }
            cv.notify_all();
            EXPECT_EQ(0, i);

            AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(1));

            {
                AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
                i = 1;
            }
            cv.notify_all();
        };

        EXPECT_EQ(0, i);

        AZStd::thread waitThreads[8];
        for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(waitThreads); ++threadIdx)
        {
            waitThreads[threadIdx] = AZStd::thread(wait);
        }
        AZStd::thread signalThread(signal);
        
        for (auto& thread : waitThreads)
        {
            thread.join();
        }
        signalThread.join();

        EXPECT_EQ(1, i);
    }

    // ensure that WaitUntil actually waits until the time specified instead of returning spuriously and instantly.
    TEST_F(ConditionVariable, Wait_Until_NoPredicate_ActuallyWaits)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic<AZStd::cv_status> status = { AZStd::cv_status::no_timeout };
        AZStd::chrono::system_clock::time_point startTime;
        // note that we capture the start and end time in the thread - this is because threads starting and stopping
        // can have unpredictable scheduling.

        AZStd::chrono::milliseconds timeSpent;

        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            auto waitDuration = AZStd::chrono::milliseconds(WAIT_TIME_MS);
            startTime = AZStd::chrono::system_clock::now();
            auto waitUntilTime = startTime + waitDuration;
            status = cv.wait_until(lock, waitUntilTime);
            timeSpent = AZStd::chrono::system_clock::now() - startTime;
        };
        
        // we aren't going to signal it, and ensure the timeout was reached.
        AZStd::thread waitThread1(wait);

        waitThread1.join();
        // the duration given is a minimum time, for wait_until, so we should have timed out above
        EXPECT_GE(timeSpent, AZStd::chrono::milliseconds(WAIT_TIME_MS - MARGIN_OF_ERROR_MS));
        EXPECT_TRUE(status == AZStd::cv_status::timeout);
    }


    TEST_F(ConditionVariable, Wait_Until_TimeAlreadyPassed_DoesNotWait)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic<AZStd::cv_status> status = { AZStd::cv_status::no_timeout };
        AZStd::chrono::system_clock::time_point startTime;
        AZStd::chrono::milliseconds timeSpent;

        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            auto waitUntilTime = AZStd::chrono::system_clock::now();
            startTime = waitUntilTime;
            status = cv.wait_until(lock, waitUntilTime);
            timeSpent = AZStd::chrono::system_clock::now() - startTime;
        };
        
        AZStd::thread waitThread1(wait);
        waitThread1.join();

        // we should have timed out immediately
        EXPECT_LT(timeSpent, AZStd::chrono::milliseconds(MARGIN_OF_ERROR_MS));
        EXPECT_TRUE(status == AZStd::cv_status::timeout);
    }

    TEST_F(ConditionVariable, Wait_Until_Predicate_TimeAlreadyPassed_DoesNotWait)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_bool status = { true };
        auto pred = [](){ return false; };
        AZStd::chrono::system_clock::time_point startTime;
        AZStd::chrono::milliseconds timeSpent;
        
        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            auto waitUntilTime = AZStd::chrono::system_clock::now();
            startTime = waitUntilTime;
            status = cv.wait_until(lock, waitUntilTime, pred);
            timeSpent = AZStd::chrono::system_clock::now() - startTime;
        };
        
        
        AZStd::thread waitThread1(wait);
        waitThread1.join();
        
        // we should have timed out immediately:
        EXPECT_LT(timeSpent, AZStd::chrono::milliseconds(MARGIN_OF_ERROR_MS));
        EXPECT_FALSE(status); // if the time has passed, the status should be false.
    }

    // ensure that WaitUntil actually waits until the time specified instead of returning spuriously and instantly.
    TEST_F(ConditionVariable, Wait_Until_FalsePredicate_ActuallyWaits)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_bool retVal = { true };
        
        auto pred = []() { return false; }; // should cause it to wait the entire duration

        AZStd::chrono::system_clock::time_point startTime;
        AZStd::chrono::milliseconds timeSpent;

        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            auto waitDuration = AZStd::chrono::milliseconds(WAIT_TIME_MS);
            startTime = AZStd::chrono::system_clock::now();
            auto waitUntilTime =  startTime + waitDuration;
            retVal = cv.wait_until(lock, waitUntilTime, pred);
            timeSpent = AZStd::chrono::system_clock::now() - startTime;
        };
        
        // we aren't going to signal it, and ensure the timeout was reached.
        AZStd::thread waitThread1(wait);
        
        waitThread1.join();

        // the duration given is a minimum time, for wait_until, so we should have timed out above
        EXPECT_GE(timeSpent, AZStd::chrono::milliseconds(WAIT_TIME_MS - MARGIN_OF_ERROR_MS));
        EXPECT_FALSE(retVal); // we didn't wake up
    }

    // ensure that WaitUntil with a predicate returns true when the predicate is true
    TEST_F(ConditionVariable, Wait_Until_TruePredicate_DoesNotWait)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_bool retVal = {true};
        AZStd::chrono::system_clock::time_point startTime;
        AZStd::chrono::milliseconds timeSpent;

        auto pred = []() { return true; }; // should cause it to immediately return
        
        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            auto waitDuration = AZStd::chrono::milliseconds(WAIT_TIME_MS);
            startTime = AZStd::chrono::system_clock::now();
            auto waitUntilTime = startTime + waitDuration;
            
            retVal = cv.wait_until(lock, waitUntilTime, pred);
            timeSpent = AZStd::chrono::system_clock::now() - startTime;
        };
        
        AZStd::thread waitThread1(wait);
        waitThread1.join();
        
        // we should NOT have reached the minimum time or in fact waited at all:
        EXPECT_LE(timeSpent, AZStd::chrono::milliseconds(MARGIN_OF_ERROR_MS));
        EXPECT_TRUE(retVal); // we didn't wake up but still returned true.
    }

    // ensure that WaitFor actually waits for a non zero amount of time and that there are no assertions in it
    // (if there are, the listener will trigger)
    TEST_F(ConditionVariable, Wait_For_ActuallyWaits)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic<AZStd::cv_status> status = { AZStd::cv_status::no_timeout };

        AZStd::chrono::system_clock::time_point startTime;
        AZStd::chrono::milliseconds timeSpent;

        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            auto waitDuration = AZStd::chrono::milliseconds(WAIT_TIME_MS);
            startTime = AZStd::chrono::system_clock::now();
            status = cv.wait_for(lock, waitDuration);
            timeSpent = AZStd::chrono::system_clock::now() - startTime;
        };
        
        // we aren't going to signal it, and ensure the timeout was reached.
        AZStd::thread waitThread1(wait);
        waitThread1.join();
        
        // note that wait_for is allowed to spuriously wake up on some platforms but even when it does, its likely to
        // have taken longer than margin of error to do so.  If the below triggers, its because it wasn't sleeping at
        // all and there is an error in the implementation which is causing it to return without sleeping.
        EXPECT_GE(timeSpent, AZStd::chrono::milliseconds(MARGIN_OF_ERROR_MS));
        EXPECT_TRUE(status == AZStd::cv_status::timeout);
    }

    TEST_F(ConditionVariable, Wait_For_Predicate_ActuallyWaits)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_bool status = {true};
        auto pred = []() { return false; };

        AZStd::chrono::system_clock::time_point startTime;
        AZStd::chrono::milliseconds timeSpent;

        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            startTime = AZStd::chrono::system_clock::now();
            auto waitDuration = AZStd::chrono::milliseconds(WAIT_TIME_MS);
            status = cv.wait_for(lock, waitDuration, pred);
            timeSpent = AZStd::chrono::system_clock::now() - startTime;
        };
        
        // we aren't going to signal it, and ensure the timeout was reached.
        AZStd::thread waitThread1(wait);
        waitThread1.join();

        // wait for with predicate false should always wait the full time.
        EXPECT_GE(timeSpent, AZStd::chrono::milliseconds(WAIT_TIME_MS - MARGIN_OF_ERROR_MS));
        EXPECT_FALSE(status); // we get no signal, we return false.
    }

    TEST_F(ConditionVariable, WaitUntil_Signalled_WakesUp)
    {
        AZStd::condition_variable cv;
        AZStd::mutex cv_mutex;
        AZStd::atomic_int i(0);
        AZStd::atomic_bool done(false);
        AZStd::chrono::system_clock::time_point startTime;
        AZStd::chrono::milliseconds timeSpent;
        constexpr AZStd::chrono::seconds waitTimeCrossThread(10);
        // normally we'd wait for WAIT_TIME_MS, but in this case, a completely different thread is doing the signalling,
        // and it could be very slow to start if the machine is under load.  So instead, we wait for a long time.
        // In normal conditions, the wait will be very short (milliseconds), since we start the other thread that wakes
        // this one up immediately.
        
        auto wait = [&]()
        {
            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            
            auto waitDuration = waitTimeCrossThread;
            startTime = AZStd::chrono::system_clock::now();
            auto waitUntilTime = startTime + waitDuration;
            // we expect the other thread to wake us up before the timeout expires so the following should return true
            EXPECT_TRUE(cv.wait_until(lock, waitUntilTime, [&]{ return i == 1; }));
            timeSpent = AZStd::chrono::system_clock::now() - startTime;
            EXPECT_EQ(1, i);
            done = true;
        };

        auto signal = [&]()
        {
            cv.notify_one();
            EXPECT_EQ(0, i);
            EXPECT_FALSE(done);

            AZStd::unique_lock<AZStd::mutex> lock(cv_mutex);
            i = 1;
            while (!done)
            {
                lock.unlock();
                cv.notify_one();
                lock.lock();
            }
        };

        EXPECT_EQ(0, i);
        EXPECT_FALSE(done);

        AZStd::thread waitThread1(wait);
        AZStd::thread signalThread(signal);
        waitThread1.join();
        signalThread.join();

        // we expect this to resolve before the maximum timeout.
        EXPECT_LT(timeSpent, waitTimeCrossThread);
        EXPECT_EQ(1, i);
        EXPECT_TRUE(done);
    }

    // Fixture for thread-driller-bus related calls
    // exists only to categorize the tests.
    class ThreadEventsBus :
        public AllocatorsFixture
    {
        public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
        }

        void TearDown() override
        {
            AllocatorsFixture::TearDown();
        }
    };

    template <typename T> class ThreadEventCounter :
        public T
    {
        public:
        void Connect() { T::BusConnect(); }
        void Disconnect() { T::BusDisconnect(); }

        virtual ~ThreadEventCounter() {}

        int m_enterCount = 0;
        int m_exitCount = 0;

        protected:


        void OnThreadEnter(const AZStd::thread::id&, const AZStd::thread_desc*) override
        {
            ++m_enterCount;
        }

        void OnThreadExit([[maybe_unused]] const AZStd::thread::id& id) override
        {
            ++m_exitCount;
        }
    };
    
    TEST_F(ThreadEventsBus, Broadcasts_BothBusses)
    {
        ThreadEventCounter<AZStd::ThreadEventBus::Handler> eventBusCounter;
        ThreadEventCounter<AZStd::ThreadDrillerEventBus::Handler> drillerBusCounter;
        auto thread_function = [&]()
        {
               ; // intentionally left blank
        };

        eventBusCounter.Connect();
        drillerBusCounter.Connect();

        AZStd::thread starter =  AZStd::thread(thread_function);
        starter.join();
        EXPECT_EQ(drillerBusCounter.m_enterCount, 1);
        EXPECT_EQ(drillerBusCounter.m_exitCount, 1);
        EXPECT_EQ(eventBusCounter.m_enterCount, 1);
        EXPECT_EQ(eventBusCounter.m_exitCount, 1);
        eventBusCounter.Disconnect();
        drillerBusCounter.Disconnect();
    }

    // this class tests for deadlocks caused by interactions between the thread
    // driller bus and the other driller busses.
    // Client code (ie, not part of the driller system) can connec to the
    // ThreadEventBus and be told when threads are started and stopped
    // However, if they instead listen to the ThreadDrillerEventBus, a deadlock condition
    // could be caused if they lock a mutex that another thread needs in order to proceed.
    // This test makes sure that using the ThreadEventBus (ie, the one meant for client code)
    // instead of the ThreadDrillerEventBus (the one meant only for profilers) does NOT cause
    // a deadlock.

    // We will simulate this series of events by doing the following
    // 1.  Main thread listens on the ThreadEventBus
    // 2.  OnThreadExit will lock a mutex, perform an allocation, unlock a mutex
    // 3.  The thread itself will lock the mutex, perform an allocation, unlock the mutex.
    // As long as there is no cross talk between the client and the driller busses, the
    // above operation should not deadlock.
    // but if there is, then a deadlock can occur where one thread will be unable to perform
    // its allocation because the other is in OnThreadExit()
    // and the other will not be able to perform OnThreadExit() because it cannot lock the mutex.

    class ThreadEventsDeathTest :
        public AllocatorsFixture
    {
        public:
        void SetUp() override
        {
            AllocatorsFixture::SetUp();
        }

        void TearDown() override
        {
            AllocatorsFixture::TearDown();
        }
    };

    class DeadlockCauser : public AZStd::ThreadEventBus::Handler
    {
        public:
        virtual ~DeadlockCauser() { }

        AZStd::recursive_mutex deadlock_mutex;

        void PerformTest()
        {
            BusConnect();

            auto thread_function = [&]()
            {
                // to cause the most amount of flapping threads
                // will yield between each instruction
                AZStd::this_thread::yield();
                AZStd::lock_guard<AZStd::recursive_mutex> locker(this->deadlock_mutex);
                AZStd::this_thread::yield();
                char* mybuffer = (char*)azmalloc(1024 * 1024);
                AZStd::this_thread::yield();
                azfree(mybuffer);
                AZStd::this_thread::yield();
            };

            // IF there's crosstalk between the threads
            // then this deadlocks, instantly, every time, even with just 2 threads.
            // To avoid killing our test CI system, we'll do this in another thread, and kill it
            // if it takes longer than a few seconds.
            AZStd::atomic_bool doneIt = {false};

            auto deadlocker_function = [&]()
            {
                // this test is a 10/10 failure
                // at just 4 retry_count, we quadruple it here to make sure
                for (int retry_count = 0; retry_count < 16; ++retry_count)
                {
                    // cause some contention for the mutex
                    // it takes only 2 threads to do this 10/10 times... quadruple it
                    // to ensure reproduction.
                    AZStd::thread waitThreads[8];
                    for (size_t threadIdx = 0; threadIdx < AZ_ARRAY_SIZE(waitThreads); ++threadIdx)
                    {
                        AZStd::this_thread::yield();
                        waitThreads[threadIdx] = AZStd::thread(thread_function);
                        AZStd::this_thread::yield();
                    }

                    for (auto& thread : waitThreads)
                    {
                        thread.join();
                    }
                }

                doneIt.store(true);
            };

            AZStd::thread deadlocker_thread = AZStd::thread(deadlocker_function);

            chrono::system_clock::time_point startTime = chrono::system_clock::now();
            while (!doneIt.load())
            {
                AZStd::this_thread::yield();
                auto sleepTime = chrono::system_clock::now() - startTime;

                // the test normally succeeds in under a second
                // but machines can be slow, so we'll give it 20x the
                // necessary time
                if (AZStd::chrono::duration_cast<AZStd::chrono::seconds>(sleepTime).count() > 20)
                {
                    // this should not take that long, we have deadlocked.
                    break;
                }
            }

            // if we're deadlocked, doneIt will be false
            // and if that happens, we have to exit() because
            // everyting will deadlock (forever) and this will jeaopardize the CI system.
            EXPECT_TRUE(doneIt.load()) << "A test has deadlocked, aborting module";
            if (!doneIt.load())
            {
                abort();
            }
            BusDisconnect();
            deadlocker_thread.join();
        }

        void OnThreadExit(const AZStd::thread::id&) override
        {
            AZStd::this_thread::yield();
            AZStd::lock_guard<AZStd::recursive_mutex> locker(this->deadlock_mutex);
            AZStd::this_thread::yield();
            char* mybuffer = (char*)azmalloc(1024 * 1024);
            AZStd::this_thread::yield();
            azfree(mybuffer);
            AZStd::this_thread::yield();
        }

        void OnThreadEnter(const AZStd::thread::id& id, const AZStd::thread_desc*) override
        {
            OnThreadExit(id);
        }
    };

#if GTEST_HAS_DEATH_TEST
    TEST_F(ThreadEventsDeathTest, UsingClientBus_AvoidsDeadlock)
    {
        EXPECT_EXIT(
            {
                DeadlockCauser cause;
                cause.PerformTest();
                // you MUST exit for EXPECT_EXIT to function.
                _exit(0); // this will cause spew, but it wont be considered to have failed.
            }
        , ::testing::ExitedWithCode(0),".*");
        
    }
#endif // GTEST_HAS_DEATH_TEST
}
