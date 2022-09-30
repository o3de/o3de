/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

/**
* This file is to be included from the semaphore.h only. It should NOT be included by the user.
*/

namespace AZStd
{
    inline semaphore::semaphore(unsigned int initialCount, unsigned int maximumCount)
    {
        m_semaphore = CreateSemaphoreW(NULL, initialCount, maximumCount, 0);
        AZ_Assert(m_semaphore != NULL, "CreateSemaphore error: %d\n", GetLastError());
    }

    inline semaphore::semaphore(const char* name, unsigned int initialCount, unsigned int maximumCount)
    {
        (void)name; // name is used only for debug, if we pass it to the semaphore it will become named semaphore
        m_semaphore = CreateSemaphoreW(NULL, initialCount, maximumCount, 0);
        AZ_Assert(m_semaphore != NULL, "CreateSemaphore error: %d\n", GetLastError());
    }

    inline semaphore::~semaphore()
    {
        BOOL ret = CloseHandle(m_semaphore);
        (void)ret;
        AZ_Assert(ret, "CloseHandle error: %d\n", GetLastError());
    }

    inline void semaphore::acquire()
    {
        WaitForSingleObject(m_semaphore, AZ_INFINITE);
    }

    template <class Rep, class Period>
    AZ_FORCE_INLINE bool semaphore::try_acquire_for(const chrono::duration<Rep, Period>& rel_time)
    {
        chrono::milliseconds durationMilliseconds = AZStd::chrono::duration_cast<chrono::milliseconds>(rel_time);
        DWORD millisWinAPI = static_cast<DWORD>(durationMilliseconds.count());
        DWORD resultCode = WaitForSingleObject(m_semaphore, millisWinAPI);
        return (resultCode == AZ_WAIT_OBJECT_0);
    }

    template <class Clock, class Duration>
    AZ_FORCE_INLINE bool semaphore::try_acquire_until(const chrono::time_point<Clock, Duration>& abs_time)
    {
        auto timeNow = AZStd::chrono::steady_clock::now();
        if (timeNow < abs_time)
        {
            chrono::milliseconds timeToTry = AZStd::chrono::duration_cast<AZStd::chrono::milliseconds>(abs_time - timeNow);
            return try_acquire_for(timeToTry);
        }
        return false;
    }

    inline void semaphore::release(unsigned int releaseCount)
    {
        ReleaseSemaphore(m_semaphore, releaseCount, NULL);
    }

    inline semaphore::native_handle_type semaphore::native_handle()
    {
        return m_semaphore;
    }
}
