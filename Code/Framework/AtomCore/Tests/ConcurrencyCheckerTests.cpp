/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AtomCore/std/parallel/concurrency_checker.h>

#include <AzCore/UnitTest/TestTypes.h>

using namespace AZStd;

namespace UnitTest
{
    class ConcurrencyCheckerTestFixture
        : public AllocatorsTestFixture
    {

        void SetUp() override
        {
            AllocatorsFixture::SetUp();
        }
    };

    TEST_F(AllocatorsTestFixture, SoftLock_NoContention_NoAsserts)
    {
        concurrency_checker concurrencyChecker;
        concurrencyChecker.soft_lock();
        concurrencyChecker.soft_unlock();
        concurrencyChecker.soft_lock();
        concurrencyChecker.soft_unlock();
    }

    TEST_F(AllocatorsTestFixture, SoftLock_AlreadyLocked_Assert)
    {
        concurrency_checker concurrencyChecker;
        concurrencyChecker.soft_lock();
        AZ_TEST_START_TRACE_SUPPRESSION;
        concurrencyChecker.soft_lock();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(AllocatorsTestFixture, SoftUnlock_NotAlreadyLocked_Assert)
    {
        concurrency_checker concurrencyChecker;
        concurrencyChecker.soft_lock();
        concurrencyChecker.soft_unlock();
        AZ_TEST_START_TRACE_SUPPRESSION;
        concurrencyChecker.soft_unlock();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(AllocatorsTestFixture, SoftLockShared_NoContention_NoAsserts)
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

    TEST_F(AllocatorsTestFixture, SoftLockShared_SharedLockAfterSoftLock_Assert)
    {
        concurrency_checker concurrencyChecker;

        concurrencyChecker.soft_lock();
        AZ_TEST_START_TRACE_SUPPRESSION;
        concurrencyChecker.soft_lock_shared();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }

    TEST_F(AllocatorsTestFixture, SoftUnlockShared_NotAlreadyLocked_Assert)
    {
        concurrency_checker concurrencyChecker;
        concurrencyChecker.soft_lock_shared();
        concurrencyChecker.soft_unlock_shared();
        AZ_TEST_START_TRACE_SUPPRESSION;
        concurrencyChecker.soft_unlock_shared();
        AZ_TEST_STOP_TRACE_SUPPRESSION(1);
    }
}
