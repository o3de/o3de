/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomCore/std/parallel/concurrency_checker.h>

#include <AzCore/UnitTest/TestTypes.h>

using namespace AZStd;

namespace UnitTest
{
    TEST_F(LeakDetectionFixture, SoftLock_NoContention_NoAsserts)
    {
        concurrency_checker concurrencyChecker;
        concurrencyChecker.soft_lock();
        concurrencyChecker.soft_unlock();
        concurrencyChecker.soft_lock();
        concurrencyChecker.soft_unlock();
    }

    TEST_F(LeakDetectionFixture, SoftLock_AlreadyLocked_Assert)
    {
        concurrency_checker concurrencyChecker;
        concurrencyChecker.soft_lock();
        AZ_TEST_START_TRACE_SUPPRESSION;
        concurrencyChecker.soft_lock();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(LeakDetectionFixture, SoftUnlock_NotAlreadyLocked_Assert)
    {
        concurrency_checker concurrencyChecker;
        concurrencyChecker.soft_lock();
        concurrencyChecker.soft_unlock();
        AZ_TEST_START_TRACE_SUPPRESSION;
        concurrencyChecker.soft_unlock();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(LeakDetectionFixture, SoftLockShared_NoContention_NoAsserts)
    {
        concurrency_checker concurrencyChecker;
        // Multiple shared locks can be made at once,
        // as long as they are all unlocked before the next soft_lock
        concurrencyChecker.soft_lock_shared();
        concurrencyChecker.soft_lock_shared();
        concurrencyChecker.soft_unlock_shared();
        concurrencyChecker.soft_unlock_shared();

        concurrencyChecker.soft_lock();
        concurrencyChecker.soft_unlock();

        concurrencyChecker.soft_lock_shared();
        concurrencyChecker.soft_lock_shared();
        concurrencyChecker.soft_unlock_shared();
        concurrencyChecker.soft_unlock_shared();

        concurrencyChecker.soft_lock();
        concurrencyChecker.soft_unlock();
    }

    TEST_F(LeakDetectionFixture, SoftLockShared_SharedLockAfterSoftLock_Assert)
    {
        concurrency_checker concurrencyChecker;

        concurrencyChecker.soft_lock();
        AZ_TEST_START_TRACE_SUPPRESSION;
        concurrencyChecker.soft_lock_shared();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(LeakDetectionFixture, SoftUnlockShared_NotAlreadyLocked_Assert)
    {
        concurrency_checker concurrencyChecker;
        concurrencyChecker.soft_lock_shared();
        concurrencyChecker.soft_unlock_shared();
        AZ_TEST_START_TRACE_SUPPRESSION;
        concurrencyChecker.soft_unlock_shared();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }
}
