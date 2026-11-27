/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "time_Apple.h"
#include <AzCore/std/algorithm.h>
#include <mach/kern_return.h>
#include <unistd.h>
#include <AzCore/std/chrono/chrono.h>

/**
 * This file is to be included from the semaphore.h only. It should NOT be included by the user.
 */

namespace AZStd
{
    inline semaphore::semaphore(unsigned int initialCount, unsigned int maximumCount)
    {
        // Clamp the semaphores initial value to prevent issues with the semaphore.
        int initSemCount = static_cast<int>(AZStd::min(initialCount, static_cast<unsigned int>(semaphore::MAXIMUM_COUNT)));
        [[maybe_unused]] int result = semaphore_create(mach_task_self(), &m_semaphore, SYNC_POLICY_FIFO, initSemCount);
        AZ_Assert(result == 0, "semaphore_create error %s\n", strerror(errno));

        int maxSemCount = static_cast<int>(AZStd::min(maximumCount, static_cast<unsigned int>(semaphore::MAXIMUM_COUNT)));
        int availSemCount = maxSemCount - AZStd::min(initSemCount, maxSemCount);
        result = semaphore_create(mach_task_self(), &m_maxCountSemaphore, SYNC_POLICY_FIFO, availSemCount);
        AZ_Assert(result == 0, "semaphore_create error for max count semaphore %s\n", strerror(errno));
    }

    inline semaphore::semaphore(const char* name, unsigned int initialCount, unsigned int maximumCount)
    {
        (void) name; // name is used only for debug, if we pass it to the semaphore it will become named semaphore
        // Clamp the semaphores initial value to prevent issues with the semaphore.
        int initSemCount = static_cast<int>(AZStd::min(initialCount, static_cast<unsigned int>(semaphore::MAXIMUM_COUNT)));
        [[maybe_unused]] int result = semaphore_create(mach_task_self(), &m_semaphore, SYNC_POLICY_FIFO, initSemCount);
        AZ_Assert(result == 0, "semaphore_create error %s\n", strerror(errno));

        int maxSemCount = static_cast<int>(AZStd::min(maximumCount, static_cast<unsigned int>(semaphore::MAXIMUM_COUNT)));
        int availSemCount = maxSemCount - AZStd::min(initSemCount, maxSemCount);
        result = semaphore_create(mach_task_self(), &m_maxCountSemaphore, SYNC_POLICY_FIFO, availSemCount);
        AZ_Assert(result == 0, "semaphore_create error for max count semaphore %s\n", strerror(errno));
    }

    inline semaphore::~semaphore()
    {
        int result = semaphore_destroy(mach_task_self(), m_semaphore);
        (void) result;
        AZ_Assert(result == 0, "semaphore_destroy error %s\n", strerror(errno));

        result = semaphore_destroy(mach_task_self(), m_maxCountSemaphore);
        AZ_Assert(result == 0, "semaphore_destroy error for max count semaphore:%s\n", strerror(errno));
    }

    inline void semaphore::acquire()
    {
        int result = 0;
        while ((result = semaphore_wait(m_semaphore)) == KERN_ABORTED)
        {
            continue;
        }
        (void)result;
        AZ_Assert(result == KERN_SUCCESS, "semaphore_wait error %d\n", result);

        result = semaphore_signal(m_maxCountSemaphore);
        AZ_Assert(result == KERN_SUCCESS, "semaphore_signal error for max count semaphore: %d\n", result);
    }

    template <class Rep, class Period>
    AZ_FORCE_INLINE bool semaphore::try_acquire_for(const chrono::duration<Rep, Period>& rel_time)
    {
        mach_timespec_t mts;
        // note that in the mach kernel (Mac), semaphore_timedwait is used instead of the posix
        // sem_timedwait.  The major difference is that semaphore_timedwait expects a delta time
        // wheras sem_timedwait expects an absolute time.
        mts.tv_sec = static_cast<unsigned int>(chrono::duration_cast<chrono::seconds>(rel_time).count());
        chrono::duration<Rep, Period> remainder = rel_time - chrono::seconds(mts.tv_sec);
        mts.tv_nsec = static_cast<clock_res_t>(chrono::duration_cast<chrono::nanoseconds>(remainder).count());

        int result = 0;
        while ((result = semaphore_timedwait(m_semaphore, mts)) == KERN_ABORTED)
        {
            continue; /* Restart if interrupted by handler */
        }

        // semaphore_timedwait never sets errno, but returns KERN_OPERATION_TIMED_OUT
        // if "Some thread-oriented operation (semaphore_wait) timed out"
        // http://opensource.apple.com//source/xnu/xnu-2422.1.72/osfmk/kern/sync_sema.c
        // http://opensource.apple.com/source/xnu/xnu-2422.1.72/osfmk/mach/kern_return.h
        AZ_Assert(result == KERN_SUCCESS || result == KERN_OPERATION_TIMED_OUT, "semaphore_timedwait error %d\n", result);
        if (result == KERN_SUCCESS)
        {
            int sigResult = semaphore_signal(m_maxCountSemaphore);
            (void)sigResult;
            AZ_Assert(sigResult == KERN_SUCCESS, "semaphore_signal error for max count semaphore: %d\n", sigResult);
            return true;
        }
        return false;
    }

    template <class Clock, class Duration>
    AZ_FORCE_INLINE bool semaphore::try_acquire_until(const chrono::time_point<Clock, Duration>& abs_time)
    {
        auto nowTime = chrono::steady_clock::now();
        if (nowTime >= abs_time)
        {
            return false; // we have already timed out.
        }
        auto deltaTime = abs_time - nowTime;
        return try_acquire_for(deltaTime);
    }

    inline void semaphore::release(unsigned int releaseCount)
    {
        int result = 0;
        while (releaseCount)
        {
            while ((result = semaphore_wait(m_maxCountSemaphore)) == KERN_ABORTED)
            {
                continue;
            }
            AZ_Assert(result == KERN_SUCCESS, "semaphore_wait error for max count semaphore: %d\n", result);
            if (result != KERN_SUCCESS)
            {
                break;
            }

            result = semaphore_signal(m_semaphore);
            AZ_Assert(result == KERN_SUCCESS, "semaphore_signal error: %d\n", result);

            if (result != KERN_SUCCESS)
            {
                break; // exit on error
            }

            --releaseCount;
        }
    }

    inline semaphore::native_handle_type semaphore::native_handle()
    {
        return &m_semaphore;
    }
}
