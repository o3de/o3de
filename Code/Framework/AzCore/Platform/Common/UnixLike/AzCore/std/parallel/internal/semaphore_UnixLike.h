/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "time_UnixLike.h"
#include <AzCore/std/chrono/chrono.h>

/**
* This file is to be included from the semaphore.h only. It should NOT be included by the user.
*/


namespace AZStd
{
    inline semaphore::semaphore(unsigned int initialCount, unsigned int maximumCount)
    {
        int result = sem_init(&m_semaphore, 0, initialCount);
        (void)result;
        AZ_Assert(result == 0, "sem_init error %s\n", strerror(errno));

        result = sem_init(&m_maxCountSemaphore, 0, maximumCount);
        AZ_Assert(result == 0, "sem_init error for max count semaphore %s\n", strerror(errno));
    }

    inline semaphore::semaphore(const char* name, unsigned int initialCount, unsigned int maximumCount)
    {
        (void)name; // name is used only for debug, if we pass it to the semaphore it will become named semaphore
        // we can share the semaphore and use the name, but the intention of the name is for debugging
        // for named shared semaphores use the OS directly as this is a slot operation
        int result = sem_init(&m_semaphore, 0, initialCount);
        (void)result;
        AZ_Assert(result == 0, "sem_init error %s\n", strerror(errno));

        result = sem_init(&m_maxCountSemaphore, 0, maximumCount);
        AZ_Assert(result == 0, "sem_init error for max count semaphore %s\n", strerror(errno));
    }

    inline semaphore::~semaphore()
    {
        int result = sem_destroy(&m_semaphore);
        (void)result;
        AZ_Assert(result == 0, "sem_destroy error %s\n", strerror(errno));

        result = sem_destroy(&m_maxCountSemaphore);
        AZ_Assert(result == 0, "sem_destroy error for max count semaphore:%s\n", strerror(errno));
    }

    inline void semaphore::acquire()
    {
        int result = sem_post(&m_maxCountSemaphore);
        AZ_Assert(result == 0, "post error for max count semaphore:%s\n", strerror(errno));

        result = sem_wait(&m_semaphore);
        (void)result;
        // note that we could return 0, or we could return EINTR, and its okay to be 'interrupted'
        // by a system call.  Therefore, the only real error we care about is EINVAL.
        AZ_Assert(result != EINVAL && result != EINTR, "sem_wait error %s\n", strerror(errno));
    }

    template <class Rep, class Period>
    AZ_FORCE_INLINE bool semaphore::try_acquire_for(const chrono::duration<Rep, Period>& rel_time)
    {
        timespec ts = Internal::CurrentTimeAndOffset(rel_time);
        int result = 0;
        while ((result = sem_timedwait(&m_semaphore, &ts)) == -1 && errno == EINTR)
        {
            continue; /* Restart if interrupted by handler */
        }

        // as above, EINVAL is the only "unexpected" code
        AZ_Assert(result != EINVAL && result != EINTR, "sem_timedwait error %s\n", strerror(errno));
        return result == 0;
    }

    template <class Clock, class Duration>
    AZ_FORCE_INLINE bool semaphore::try_acquire_until(const chrono::time_point<Clock, Duration>& abs_time)
    {
        auto timeNow = chrono::steady_clock::now();
        if (timeNow >= abs_time)
        {
            // we already timed out.
            return false;
        }
        auto timeDiff = abs_time - timeNow;
        return try_acquire_for(timeDiff);
    }

    AZ_FORCE_INLINE void semaphore::release(unsigned int releaseCount)
    {
        int result = sem_wait(&m_maxCountSemaphore);
        AZ_Assert(result == 0, "sem_wait error for max count semaphore: %s\n", strerror(errno));

        while (releaseCount)
        {
            result = sem_post(&m_semaphore);
            AZ_Assert(result == 0, "sem_post error: %s\n", strerror(errno));

            if (result != 0)
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
