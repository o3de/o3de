/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/parallel/lock.h>

#include "UserTypes.h"

namespace UnitTest
{
    // Fixture for non-typed tests
    class LockTest
        : public LeakDetectionFixture
    {
    };

    struct LockableBoolHelper
    {
        LockableBoolHelper() = default;

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
            if (!m_locked)
            {
                m_locked = true;
                return true;
            }

            return false;
        }

        bool m_locked{};
    };


    struct TryLockSetBoolFalse
    {
        TryLockSetBoolFalse() = default;

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
            m_locked = false;
            return m_locked;
        }

        bool m_locked{};
    };

    static uint32_t m_nonAtomicAccumulator1{ 0 };
    static uint32_t m_nonAtomicAccumulator2{ 0 };

    template<typename Lockable1, typename Lockable2>
    static void Thread1Test(Lockable1& lockable1, Lockable2& lockable2)
    {
        LockableBoolHelper lockableBoolHelper;
        AZStd::lock(lockable1, lockable2, lockableBoolHelper);
        m_nonAtomicAccumulator1 += 2;
        m_nonAtomicAccumulator2 += 3;
        EXPECT_TRUE(lockableBoolHelper.m_locked);
        lockable2.unlock();
        lockable1.unlock();
    }

    template<typename Lockable1, typename Lockable2>
    static void Thread2Test(Lockable1& lockable1, Lockable2& lockable2)
    {
        LockableBoolHelper lockableBoolHelper;
        AZStd::lock(lockable1, lockableBoolHelper, lockable2);
        m_nonAtomicAccumulator1 += 5;
        m_nonAtomicAccumulator2 += 1;
        EXPECT_TRUE(lockableBoolHelper.m_locked);
        lockable1.unlock();
        lockable2.unlock();
    }

    TEST_F(LockTest, lock_LockTwoMutexesAtOnce_BothLockedSuccess)
    {
        // lockable bool helper case
        LockableBoolHelper testLockable1;
        LockableBoolHelper testLockable2;
        AZStd::lock(testLockable1, testLockable2);
        EXPECT_TRUE(testLockable1.m_locked);
        EXPECT_TRUE(testLockable2.m_locked);
    }

    TEST_F(LockTest, lock_LockMultipleMutexesAtOnceOnMultipleThreadsInDifferentOrders_LockedWithoutDeadlock)
    {
        // Multi thread case
        constexpr size_t numThreads = 2;
        AZStd::mutex testMutex1;
        AZStd::mutex testMutex2;
        AZStd::thread threads[numThreads];
        threads[0] = AZStd::thread([&mutex1 = testMutex1, &mutex2 = testMutex2]()
        {
            Thread1Test(mutex1, mutex2);
        });
        threads[1] = AZStd::thread([&mutex1 = testMutex1, &mutex2 = testMutex2]()
        {
            Thread2Test(mutex2, mutex1);
        });

        threads[0].join();
        threads[1].join();
    }

    TEST_F(LockTest, try_lock_AttemptLockMultipleAtOnce_AllMutexesAreLocked)
    {
        LockableBoolHelper lockableBool1;
        LockableBoolHelper lockableBool2;

        EXPECT_EQ(-1, AZStd::try_lock(lockableBool1, lockableBool2));

        // LockableBoolHelper locks are still engaged, therefore lockable bool should fail first
        EXPECT_EQ(0, AZStd::try_lock(lockableBool1, lockableBool2));
        lockableBool1.unlock();

        EXPECT_EQ(1, AZStd::try_lock(lockableBool1, lockableBool2));
        lockableBool2.unlock();
    }
    TEST_F(LockTest, try_lock_AttemptLockMultipleAtOnceWith_TryLockSetBoolFalse_Type_ThatAlwaysReturnsFalseForTryLock_TryLockReturnsMutexThatFailedToLock)
    {
        LockableBoolHelper lockableBool1;
        LockableBoolHelper lockableBool2;
        TryLockSetBoolFalse lockableTryLockFalse1;
        EXPECT_EQ(1, AZStd::try_lock(lockableBool2, lockableTryLockFalse1, lockableBool1));
    }
    TEST_F(LockTest, try_lock_AttemptLockMultipleAtOnceWithMutexThatIsLocked_TryLockReturnsFirstMutexThatHasBeenLocked)
    {
        LockableBoolHelper lockableBool1;
        LockableBoolHelper lockableBool2;
        TryLockSetBoolFalse lockableTryLockFalse1;
        EXPECT_EQ(2, AZStd::try_lock(lockableBool1, lockableBool2, lockableTryLockFalse1));
        lockableBool1.lock();
        // Note lockableBool1 has been swapped with the lockableBool2 parameter and locked
        EXPECT_EQ(1, AZStd::try_lock(lockableBool2, lockableBool1, lockableTryLockFalse1));
    }
}
