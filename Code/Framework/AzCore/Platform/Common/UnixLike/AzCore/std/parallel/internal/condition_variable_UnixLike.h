/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/chrono/chrono.h>

/**
 * This file is to be included from the mutex.h only. It should NOT be included by the user.
 */

namespace AZStd
{
    //////////////////////////////////////////////////////////////////////////
    // Condition variable
    inline condition_variable::condition_variable()
    {
        pthread_cond_init (&m_cond_var, NULL);
    }

    inline condition_variable::condition_variable(const char* name)
    {
        (void)name;
        pthread_cond_init (&m_cond_var, NULL);
    }

    inline condition_variable::~condition_variable()
    {
        pthread_cond_destroy(&m_cond_var);
    }

    inline void condition_variable::notify_one()
    {
        pthread_cond_signal(&m_cond_var);
    }
    inline void condition_variable::notify_all()
    {
        pthread_cond_broadcast(&m_cond_var);
    }
    inline void condition_variable::wait(unique_lock<mutex>& lock)
    {
        pthread_cond_wait(&m_cond_var, lock.mutex()->native_handle());
    }
    template <class Predicate>
    AZ_FORCE_INLINE void condition_variable::wait(unique_lock<mutex>& lock, Predicate pred)
    {
        while (!pred())
        {
            wait(lock);
        }
    }

    template <class Clock, class Duration>
    AZ_FORCE_INLINE cv_status condition_variable::wait_until(unique_lock<mutex>& lock, const chrono::time_point<Clock, Duration>& abs_time)
    {
        const auto now = chrono::steady_clock::now();
        if (now < abs_time)
        {
            return wait_for(lock, abs_time - now);
        }

        return cv_status::timeout;
    }
    template <class Clock, class Duration, class Predicate>
    AZ_FORCE_INLINE bool condition_variable::wait_until(unique_lock<mutex>& lock, const chrono::time_point<Clock, Duration>& abs_time, Predicate pred)
    {
        while (!pred())
        {
            if (wait_until(lock, abs_time) == cv_status::timeout)
            {
                return pred();
            }
        }
        return true;
    }

    // all the other functions eventually call this one,  which invokes pthread.  Pthread wil try its best to wait
    // until the specified time arrives (but it may wake up spuriously if cancelled!).  According to the standard,
    // we return timeout if the reason we woke up  was a time out, and no_timeout in all other situations.
    template <class Rep, class Period>
    AZ_FORCE_INLINE cv_status condition_variable::wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time)
    {
        timespec ts = Internal::CurrentTimeAndOffset(rel_time);

        int ret = pthread_cond_timedwait(&m_cond_var, lock.mutex()->native_handle(), &ts);
        // this assert catches the situation where you've given an invalid value
        // to 'ts', such as nanoseconds > 999999999.
        AZ_Assert(ret != EINVAL && ret != EPERM, "Invalid return from pthread_cond_timedwait - %i errorno: %s\n", ret, strerror(errno));
        // note that the other possible error code is EINTR (interrupted by signal), but in this case, we have to allow it to proceed anyway

        return ret == ETIMEDOUT ? cv_status::timeout : cv_status::no_timeout;
    }
    template <class Rep, class Period, class Predicate>
    AZ_FORCE_INLINE bool condition_variable::wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {
        return wait_until(lock, chrono::steady_clock::now() + rel_time, move(pred));
    }
    condition_variable::native_handle_type
    inline condition_variable::native_handle()
    {
        return &m_cond_var;
    }

    //////////////////////////////////////////////////////////////////////////
    // Condition variable any
    inline condition_variable_any::condition_variable_any()
    {
        pthread_mutex_init(&m_mutex, NULL);
        pthread_cond_init (&m_cond_var, NULL);
    }

    inline condition_variable_any::condition_variable_any(const char* name)
    {
        pthread_mutex_init(&m_mutex, NULL);
        (void)name;
        pthread_cond_init (&m_cond_var, NULL);
    }

    inline condition_variable_any::~condition_variable_any()
    {
        pthread_mutex_destroy(&m_mutex);
        pthread_cond_destroy(&m_cond_var);
    }

    inline void condition_variable_any::notify_one()
    {
        pthread_mutex_lock(&m_mutex);
        pthread_cond_signal(&m_cond_var);
        pthread_mutex_unlock(&m_mutex);
    }
    inline void condition_variable_any::notify_all()
    {
        pthread_mutex_lock(&m_mutex);
        pthread_cond_broadcast(&m_cond_var);
        pthread_mutex_unlock(&m_mutex);
    }

    template<class Lock>
    AZ_FORCE_INLINE void condition_variable_any::wait(Lock& lock)
    {
        pthread_mutex_lock(&m_mutex);
        lock.unlock();
        // We need to make sure we use CriticalSection based mutex.
        pthread_cond_wait(&m_cond_var, &m_mutex);
        pthread_mutex_unlock(&m_mutex);
        lock.lock();
    }
    template <class Lock, class Predicate>
    AZ_FORCE_INLINE void condition_variable_any::wait(Lock& lock, Predicate pred)
    {
        while (!pred())
        {
            wait(lock);
        }
    }
    template <class Lock, class Clock, class Duration>
    AZ_FORCE_INLINE cv_status condition_variable_any::wait_until(Lock& lock, const chrono::time_point<Clock, Duration>& abs_time)
    {
        const auto now = chrono::steady_clock::now();
        if (now < abs_time)
        {
            // note that wait_for will either time out, in which case we should return time out, or it will return
            // without timing out which means either an error occurred, or we got the signal we were waiting for.
            // This means that either way, there is no point in having a while loop here.
            return wait_for(lock, abs_time - now);
        }

        // if we started with the timeout already passed, we are immediately timed out.
        return cv_status::timeout;
    }
    template <class Lock, class Clock, class Duration, class Predicate>
    AZ_FORCE_INLINE bool condition_variable_any::wait_until(Lock& lock, const chrono::time_point<Clock, Duration>& abs_time, Predicate pred)
    {
        while (!pred())
        {
            if (wait_until(lock, abs_time) == cv_status::timeout)
            {
                return pred();
            }
        }
        return true;
    }
    template <class Lock, class Rep, class Period>
    AZ_FORCE_INLINE cv_status condition_variable_any::wait_for(Lock& lock, const chrono::duration<Rep, Period>& rel_time)
    {
        pthread_mutex_lock(&m_mutex);
        lock.unlock();

        // We need to make sure we use CriticalSection based mutex.
        timespec ts = Internal::CurrentTimeAndOffset(rel_time);

        int ret = pthread_cond_timedwait(&m_cond_var, lock.mutex()->native_handle(), &ts);

        pthread_mutex_unlock(&m_mutex);
        lock.lock();

        return ret == ETIMEDOUT ? cv_status::timeout : cv_status::no_timeout;
    }

    template <class Lock, class Rep, class Period, class Predicate>
    AZ_FORCE_INLINE bool condition_variable_any::wait_for(Lock& lock, const chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {
        return wait_until(lock, chrono::steady_clock::now() + rel_time, move(pred));
    }
    condition_variable_any::native_handle_type
    inline condition_variable_any::native_handle()
    {
        return &m_cond_var;
    }
    //////////////////////////////////////////////////////////////////////////
}
