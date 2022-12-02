/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/parallel/scoped_lock.h>
#include <AzCore/std/parallel/shared_mutex.h>

#include "UserTypes.h"

namespace UnitTest
{
    // Fixture for AZStd::scoped_lock unit tests
    class ScopedLockTest
        : public LeakDetectionFixture
    {
    };

    struct ScopedTestMutex
    {
        ScopedTestMutex() = default;
        void lock()
        {
            m_locked = true;
        }

        void unlock()
        {
            m_locked = false;
        }

        bool try_lock()
        {
            if (m_locked)
            {
                return false;
            }
            m_locked = true;
            return m_locked;
        }

        bool m_locked{};
    };

    struct AtomicScopedTestMutex
    {
        AtomicScopedTestMutex() = default;
        void lock()
        {
            AZStd::exponential_backoff waitForCounter;
            constexpr bool lockedCounterValue{ true };
            while (true)
            {
                bool unlockedCounterValue{ false };
                if (m_locked.compare_exchange_weak(unlockedCounterValue, lockedCounterValue, AZStd::memory_order_release))
                {
                    break;
                }
                waitForCounter.wait();
            }
        }

        void unlock()
        {
            m_locked.store(false, AZStd::memory_order_release);
        }

        bool try_lock()
        {
            bool unlockedCounterValue{ false };
            constexpr bool lockedCounterValue{ true };
            return m_locked.compare_exchange_strong(unlockedCounterValue, lockedCounterValue, AZStd::memory_order_release);;
        }

        AZStd::atomic<bool> m_locked{};
    };

    TEST_F(ScopedLockTest, scoped_lock_NoArgument_Construct_ConstructorSucceeds)
    {
        // Validates empty mutex scoped_lock is constructible and destructible

        AZStd::scoped_lock<> emptyLock;
        (void)emptyLock;
    }

    TEST_F(ScopedLockTest, scoped_lock_OneArgument_Construct_ConstructorSucceeds)
    {
        ScopedTestMutex testMutex1;
        {
            AZStd::scoped_lock<decltype(testMutex1)> oneArgLock(testMutex1);
            EXPECT_TRUE(testMutex1.m_locked);
        }
        EXPECT_FALSE(testMutex1.m_locked);
    }

    TEST_F(ScopedLockTest, scoped_lock_TwoArgument_Construct_ConstructorSucceeds)
    {
        ScopedTestMutex testMutex1;
        ScopedTestMutex testMutex2;
        {
            AZStd::scoped_lock<decltype(testMutex1), decltype(testMutex2)> twoArgLock(testMutex1, testMutex2);
            EXPECT_TRUE(testMutex1.m_locked);
            EXPECT_TRUE(testMutex2.m_locked);
        }
        EXPECT_FALSE(testMutex1.m_locked);
        EXPECT_FALSE(testMutex2.m_locked);
    }

    TEST_F(ScopedLockTest, scoped_lock_VariadicArgument_Construct_ConstructorSucceeds)
    {
        ScopedTestMutex testMutex1;
        ScopedTestMutex testMutex2;
        ScopedTestMutex testMutex3;
        {
            AZStd::scoped_lock<decltype(testMutex1), decltype(testMutex2), decltype(testMutex3)> threeArgLock(testMutex1, testMutex2, testMutex3);
            EXPECT_TRUE(testMutex1.m_locked);
            EXPECT_TRUE(testMutex2.m_locked);
            EXPECT_TRUE(testMutex3.m_locked);
        }
        EXPECT_FALSE(testMutex1.m_locked);
        EXPECT_FALSE(testMutex2.m_locked);
        EXPECT_FALSE(testMutex3.m_locked);
    }

    inline namespace ScopedLockInternal
    {
        struct NotAType {};
        template<typename T, typename = void>
        struct has_mutex_type
        {
            static constexpr bool value{ false };
        };
        template<typename T>
        struct has_mutex_type<T, AZStd::void_t<typename T::mutex_type>>
        {
            static constexpr bool value{ true };
        };

        template<typename T>
        constexpr bool has_mutex_type_v = has_mutex_type<T>::value;
    }

    TEST_F(ScopedLockTest, scoped_lock_mutex_type_trait_ExistOnlyForOneArgumentTemplate)
    {
        static_assert(!ScopedLockInternal::has_mutex_type_v<AZStd::scoped_lock<>>, "ScopedLock with empty parameter should not have the mutex_type alias");

        {
            using MutexType = ScopedTestMutex;
            using ScopedLockType = AZStd::scoped_lock<MutexType>;
            static_assert(ScopedLockInternal::has_mutex_type_v<ScopedLockType>, "ScopedLock with one parameter type should have a mutex_type alias");
            static_assert(AZStd::is_same<typename ScopedLockType::mutex_type, MutexType>::value, "ScopedLock mutex_type alias should match MutexType alias");
        }
        {
            using MutexType = AZStd::recursive_mutex;
            using ScopedLockType = AZStd::scoped_lock<MutexType>;
            static_assert(ScopedLockInternal::has_mutex_type_v<ScopedLockType>, "ScopedLock with one parameter type should have a mutex_type alias");
            static_assert(AZStd::is_same<typename ScopedLockType::mutex_type, MutexType>::value, "ScopedLock mutex_type alias should match MutexType alias");
        }

        static_assert(!ScopedLockInternal::has_mutex_type_v<AZStd::scoped_lock<ScopedTestMutex, ScopedTestMutex>>, "ScopedLock with two template parameter types should not have a mutex_type alias");
        static_assert(!ScopedLockInternal::has_mutex_type_v<AZStd::scoped_lock<AZStd::recursive_mutex, ScopedTestMutex>>, "ScopedLock with two template parameter types should not have a mutex_type alias");
        static_assert(!ScopedLockInternal::has_mutex_type_v<AZStd::scoped_lock<ScopedTestMutex, AZStd::recursive_mutex, ScopedTestMutex>>, "ScopedLock with two template parameter types should not have a mutex_type alias");
    }

    TEST_F(ScopedLockTest, scoped_lock_NoArgument_adopt_lock_Construct_ConstructorSucceeds)
    {
        // Validates empty mutex scoped_lock is construticble and destructible
        AZStd::scoped_lock<> emptyLock(AZStd::adopt_lock);
    }

    TEST_F(ScopedLockTest, scoped_lock_OneArgument_adopt_lock_Construct_ConstructorSucceeds)
    {
        ScopedTestMutex testMutex1;
        {
            AZStd::scoped_lock<decltype(testMutex1)> oneArgLock(AZStd::adopt_lock, testMutex1);
            EXPECT_FALSE(testMutex1.m_locked);
        }
        testMutex1.lock();
        {
            AZStd::scoped_lock<decltype(testMutex1)> oneArgLock(AZStd::adopt_lock, testMutex1);
            EXPECT_TRUE(testMutex1.m_locked);
        }
        EXPECT_FALSE(testMutex1.m_locked);
    }

    TEST_F(ScopedLockTest, scoped_lock_TwoArgument_adopt_lock_Construct_ConstructorSucceeds)
    {
        ScopedTestMutex testMutex1;
        ScopedTestMutex testMutex2;
        testMutex2.lock();
        {
            AZStd::scoped_lock<decltype(testMutex1), decltype(testMutex2)> twoArgLock(AZStd::adopt_lock, testMutex1, testMutex2);
            EXPECT_FALSE(testMutex1.m_locked);
            EXPECT_TRUE(testMutex2.m_locked);
        }
        EXPECT_FALSE(testMutex1.m_locked);
        EXPECT_FALSE(testMutex2.m_locked);
    }

    TEST_F(ScopedLockTest, scoped_lock_VariadicArgument_adopt_lock_Construct_ConstructorSucceeds)
    {
        ScopedTestMutex testMutex1;
        ScopedTestMutex testMutex2;
        ScopedTestMutex testMutex3;
        testMutex1.lock();
        testMutex3.lock();
        {
            AZStd::scoped_lock<decltype(testMutex1), decltype(testMutex2), decltype(testMutex3)> threeArgLock(AZStd::adopt_lock, testMutex1, testMutex2, testMutex3);
            EXPECT_TRUE(testMutex1.m_locked);
            EXPECT_FALSE(testMutex2.m_locked);
            EXPECT_TRUE(testMutex3.m_locked);
        }
        EXPECT_FALSE(testMutex1.m_locked);
        EXPECT_FALSE(testMutex2.m_locked);
        EXPECT_FALSE(testMutex3.m_locked);
    }

    TEST_F(ScopedLockTest, Deadlock_AvoidanceTest_Recursive_And_Shared_Mutex)
    {
        constexpr size_t threadCount = 8;
        enum : size_t { cycleCount = 1000 };
        constexpr uint64_t expectedCounterResult = threadCount * cycleCount;
        AZStd::thread threads[threadCount];

        AZStd::recursive_mutex recursiveMutex1;
        AZStd::shared_mutex sharedMutex1;

        uint64_t testCounter{};
        auto work = [&testCounter, &recursiveMutex1, &sharedMutex1]()
        {
            for (size_t cycleIndex = 0; cycleIndex != cycleCount; ++cycleIndex)
            {
                AZStd::scoped_lock<AZStd::recursive_mutex, AZStd::shared_mutex> recursiveAndSharedMutexLock(recursiveMutex1, sharedMutex1);
                ++testCounter;
            }
        };

        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(work);
        }

        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }

        EXPECT_EQ(expectedCounterResult, testCounter);
    }

    TEST_F(ScopedLockTest, Deadlock_AvoidanceTest_Swap_Lock_Order_For_Half_Threads)
    {
        constexpr size_t threadCount = 8;
        enum : size_t { cycleCount = 1000 };
        constexpr uint64_t expectedCounterResult = threadCount * cycleCount;
        AZStd::thread threads[threadCount];

        AZStd::recursive_mutex recursiveMutex1;
        AZStd::shared_mutex sharedMutex1;

        uint64_t testCounter{};
        auto evenThreadWorkerFunc = [&testCounter, &recursiveMutex1, &sharedMutex1]()
        {
            for (size_t cycleIndex = 0; cycleIndex != cycleCount; ++cycleIndex)
            {
                AZStd::scoped_lock<AZStd::recursive_mutex, AZStd::shared_mutex> recursiveAndSharedMutexLock(recursiveMutex1, sharedMutex1);
                ++testCounter;
            }
        };

        // Just swaps the parameter order so that the shared_mutex comes before the recursive mutex
        auto oddThreadWorkerFunc = [&testCounter, &recursiveMutex1, &sharedMutex1]()
        {
            for (size_t cycleIndex = 0; cycleIndex != cycleCount; ++cycleIndex)
            {
                AZStd::scoped_lock<AZStd::shared_mutex, AZStd::recursive_mutex> recursiveAndSharedMutexLock(sharedMutex1, recursiveMutex1);
                ++testCounter;
            }
        };

        size_t threadIndex = 0;
        for (AZStd::thread& threadRef : threads)
        {
            if (threadIndex % 2 == 0)
            {
                threadRef = AZStd::thread(evenThreadWorkerFunc);
            }
            else
            {
                threadRef = AZStd::thread(oddThreadWorkerFunc);
            }
            ++threadIndex;
        }

        for (AZStd::thread& threadRef : threads)
        {
            threadRef.join();
        }

        EXPECT_EQ(expectedCounterResult, testCounter);
    }

    TEST_F(ScopedLockTest, Deadlock_AvoidanceTest_Atomic_Test_Mutex)
    {
        constexpr size_t threadCount = 8;
        enum : size_t { cycleCount = 1000 };
        constexpr uint64_t expectedCounterResult = threadCount * cycleCount;
        AZStd::thread threads[threadCount];

        AtomicScopedTestMutex testMutex1;
        AtomicScopedTestMutex testMutex2;

        uint64_t testCounter{};
        auto work = [&testCounter, &testMutex1, &testMutex2]()
        {
            for (size_t cycleIndex = 0; cycleIndex != cycleCount; ++cycleIndex)
            {
                AZStd::scoped_lock<AtomicScopedTestMutex, AtomicScopedTestMutex> recursiveAndSharedMutexLock(testMutex1, testMutex2);
                ++testCounter;
            }
        };

        for (AZStd::thread& thread : threads)
        {
            thread = AZStd::thread(work);
        }

        for (AZStd::thread& thread : threads)
        {
            thread.join();
        }

        EXPECT_EQ(expectedCounterResult, testCounter);
    }
}
