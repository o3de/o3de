/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

/**
 * This file is to be included from the mutex.h only. It should NOT be included by the user.
 */

namespace AZStd
{
#define AZ_COND_VAR_CAST(m) reinterpret_cast<PCONDITION_VARIABLE>(&m)
#define AZ_STD_MUTEX_CAST(m) reinterpret_cast<LPCRITICAL_SECTION>(&m)


    //////////////////////////////////////////////////////////////////////////
    // Condition variable
    inline condition_variable::condition_variable()
    {
        InitializeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable::condition_variable(const char* name)
    {
        (void)name;
        InitializeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable::~condition_variable()
    {
    }

    inline void condition_variable::notify_one()
    {
        WakeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }
    inline void condition_variable::notify_all()
    {
        WakeAllConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }
    inline void condition_variable::wait(unique_lock<mutex>& lock)
    {
        SleepConditionVariableCS(AZ_COND_VAR_CAST(m_cond_var), lock.mutex()->native_handle(), AZ_INFINITE);
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
        chrono::system_clock::time_point now = chrono::system_clock::now();
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
        return pred();
    }

    template <class Rep, class Period>
    AZ_FORCE_INLINE cv_status condition_variable::wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time)
    {
        chrono::milliseconds toWait = rel_time;
        // We need to make sure we use CriticalSection based mutex.
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleepconditionvariablecs
        // indicates that this returns nonzero on success, zero on failure
        // we need to return cv_status::timeout IFF a timeout occurred and no_timeout in all other situations
        // including error.
        int ret = SleepConditionVariableCS(AZ_COND_VAR_CAST(m_cond_var), lock.mutex()->native_handle(), static_cast<DWORD>(toWait.count()));
        if (ret == 0)
        {
            int lastError = GetLastError();
            // detect a sitaution where the mutex or the cond var are invalid, or the duration
            AZ_Assert(lastError == AZ_ERROR_TIMEOUT, "Error from SleepConditionVariableCS: 0x%08x\n", lastError);
            // asserts are continuable so we still check.
            if (lastError == AZ_ERROR_TIMEOUT)
            {
                return cv_status::timeout;
            }
        } 
        return cv_status::no_timeout;
    }
    template <class Rep, class Period, class Predicate>
    AZ_FORCE_INLINE bool condition_variable::wait_for(unique_lock<mutex>& lock, const chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {
        while (!pred())
        {
            if (wait_for(lock, rel_time) == cv_status::timeout)
            {
                return pred();
            }
        }
        return pred();
    }
    condition_variable::native_handle_type
    inline condition_variable::native_handle()
    {
        return AZ_COND_VAR_CAST(m_cond_var);
    }

    //////////////////////////////////////////////////////////////////////////
    // Condition variable any
    inline condition_variable_any::condition_variable_any()
    {
        InitializeCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        InitializeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable_any::condition_variable_any(const char* name)
    {
        InitializeCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        (void)name;
        InitializeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
    }

    inline condition_variable_any::~condition_variable_any()
    {
        DeleteCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }

    inline void condition_variable_any::notify_one()
    {
        EnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        WakeConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
        LeaveCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }
    inline void condition_variable_any::notify_all()
    {
        EnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        WakeAllConditionVariable(AZ_COND_VAR_CAST(m_cond_var));
        LeaveCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }

    template<class Lock>
    AZ_FORCE_INLINE void condition_variable_any::wait(Lock& lock)
    {
        EnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        lock.unlock();
        // We need to make sure we use CriticalSection based mutex.
        SleepConditionVariableCS(AZ_COND_VAR_CAST(m_cond_var), AZ_STD_MUTEX_CAST(m_mutex), AZ_INFINITE);
        LeaveCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
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
        auto now = chrono::system_clock::now();
        if (now < abs_time)
        {
            return wait_for(lock, abs_time - now);
        }

        return cv_status::no_timeout;
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
        chrono::milliseconds toWait = rel_time;
        EnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        lock.unlock();

        // We need to make sure we use CriticalSection based mutex.
        cv_status returnCode = cv_status::no_timeout;
        int ret = SleepConditionVariableCS(AZ_COND_VAR_CAST(m_cond_var), AZ_STD_MUTEX_CAST(m_mutex), toWait.count());
        // according to MSDN docs, ret is 0 on FAILURE, and nonzero otherwise.
        if (ret == 0)
        {
            int lastError = GetLastError();
            // if we hit this assert, its likely a programmer error
            // and the values passed into SleepConditionVariableCS are invalid.
            AZ_Assert(lastError == AZ_ERROR_TIMEOUT, "Error from SleepConditionVariableCS: 0x%08x\n", lastError );

            if (lastError == AZ_ERROR_TIMEOUT)
            {
                returnCode = cv_status::timeout;
            }
        }
        LeaveCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
        lock.lock();
        return returnCode;
    }
    template <class Lock, class Rep, class Period, class Predicate>
    AZ_FORCE_INLINE bool condition_variable_any::wait_for(Lock& lock, const chrono::duration<Rep, Period>& rel_time, Predicate pred)
    {
        while (!pred())
        {
            if (!wait_for(lock, rel_time))
            {
                return pred();
            }
        }
        return pred();
    }
    condition_variable_any::native_handle_type
    inline condition_variable_any::native_handle()
    {
        return AZ_COND_VAR_CAST(m_cond_var);
    }
    //////////////////////////////////////////////////////////////////////////
#undef AZ_COND_VAR_CAST //
#undef AZ_STD_MUTEX_CAST
}
