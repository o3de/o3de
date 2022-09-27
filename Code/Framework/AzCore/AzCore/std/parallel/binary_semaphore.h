/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_BINARY_SEMAPHORE_H
#define AZSTD_BINARY_SEMAPHORE_H

#include <AzCore/base.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/Casting/numeric_cast.h>

#if AZ_TRAIT_OS_USE_WINDOWS_SET_EVENT
#   include <AzCore/std/parallel/config.h>
#elif !AZ_TRAIT_SEMAPHORE_HAS_NATIVE_MAX_COUNT
#   include <AzCore/std/parallel/mutex.h>
#   include <AzCore/std/parallel/condition_variable.h>
#else
#   include <AzCore/std/parallel/semaphore.h>
#endif

namespace AZStd
{
    /**
     * Binary semaphore class (aka event). In general is implemented via standard semaphore with max count = 1,
     * but on "some" platforms there are more efficient ways of doing it.
     * \todo Move code to platform files.
     */
    class binary_semaphore
    {
    public:
#if AZ_TRAIT_OS_USE_WINDOWS_SET_EVENT

        typedef HANDLE native_handle_type;

        binary_semaphore(bool initialState = false)
        {
            m_event = CreateEventW(nullptr, false, initialState, nullptr);
            AZ_Assert(m_event != NULL, "CreateEvent error: %d\n", GetLastError());
        }
        binary_semaphore(const char* name, bool initialState = false)
        {
            (void)name; // name is used only for debug, if we pass it to the semaphore it will become named semaphore
            m_event = CreateEventW(nullptr, false, initialState, nullptr);
            AZ_Assert(m_event != NULL, "CreateEvent error: %d\n", GetLastError());
        }

        binary_semaphore(const binary_semaphore&) = delete;
        binary_semaphore& operator=(const binary_semaphore&) = delete;

        ~binary_semaphore()
        {
            BOOL ret = CloseHandle(m_event);
            (void)ret;
            AZ_Assert(ret, "CloseHandle error: %d\n", GetLastError());
        }
        void acquire()  { WaitForSingleObject(m_event, AZ_INFINITE); }

        template <class Rep, class Period>
        bool try_acquire_for(const chrono::duration<Rep, Period>& rel_time)
        {
            auto timeToTry = chrono::duration_cast<chrono::milliseconds>(rel_time);
            return (WaitForSingleObject(m_event, aznumeric_cast<DWORD>(timeToTry.count())) == AZ_WAIT_OBJECT_0);
        }

        template <class Clock, class Duration>
        bool try_acquire_until(const chrono::time_point<Clock, Duration>& abs_time)
        {
            auto timeNow = chrono::steady_clock::now();
            if (timeNow >= abs_time)
            {
                return false; // we timed out already!
            }
            auto deltaTime = abs_time - timeNow;
            auto timeToTry = chrono::duration_cast<chrono::milliseconds>(deltaTime);
            return (WaitForSingleObject(m_event, aznumeric_cast<DWORD>(timeToTry.count())) == AZ_WAIT_OBJECT_0);
        }

        void release()  { SetEvent(m_event); }

        native_handle_type native_handle() { return m_event; }

    private:
        HANDLE m_event;

#elif !AZ_TRAIT_SEMAPHORE_HAS_NATIVE_MAX_COUNT

        typedef condition_variable::native_handle_type native_handle_type;

        binary_semaphore(bool initialState = false)
            : m_isReady(initialState)
        {}
        binary_semaphore(const char* name, bool initialState = false)
            : m_mutex(name)
            , m_condVar(name)
            , m_isReady(initialState)
        {}
        void acquire()
        {
            AZStd::unique_lock<AZStd::mutex> ulock(m_mutex);
            while (!m_isReady)
            {
                m_condVar.wait(ulock);
            }
            m_isReady = false;
        }

        template <class Rep, class Period>
        bool try_acquire_for(const chrono::duration<Rep, Period>& rel_time)
        {
            AZStd::unique_lock<AZStd::mutex> ulock(m_mutex);
            if (!m_isReady)
            {
                auto absTime = chrono::steady_clock::now() + rel_time;

                // Note that the standard specifies that try_acquire_for(.. rel_time) is the MINIMUM time to wait
                // whereas condition_var's wait_for is the maximum time to wait, and may return early.
                // Thus, we call wait_until, instead of wait_for, here.
                m_condVar.wait_until(ulock, absTime);

                if (!m_isReady)
                {
                    return false;
                }
            }
            m_isReady = false;
            return true;
        }

        template <class Clock, class Duration>
        bool try_acquire_until(const chrono::time_point<Clock, Duration>& abs_time)
        {
            AZStd::unique_lock<AZStd::mutex> ulock(m_mutex);
            if (!m_isReady)
            {
                m_condVar.wait_until(ulock, abs_time, [&](){ return m_isReady; });

                if (!m_isReady)
                {
                    return false;
                }
            }
            m_isReady = false;
            return true;
        }

        void release()
        {
            AZStd::lock_guard<AZStd::mutex> ulock(m_mutex);
            m_isReady = true;
            m_condVar.notify_one();
        }

        native_handle_type native_handle()
        {
            return m_condVar.native_handle();
        }

    private:
        // For Linux we use pthread and semaphores don't have max count, we can simulate the semaphore but this will be ineffcient compare to
        // custom solution
        mutex   m_mutex;
        condition_variable m_condVar;
        bool    m_isReady;
#else
        // For platforms with max count semaphore

        typedef native_semaphore_handle_type native_handle_type;

        binary_semaphore(bool initialState = false)
            : m_semaphore(initialState ? 1 : 0, 1) {}
        binary_semaphore(const char* name, bool initialState = false)
            : m_semaphore(name, initialState ? 1 : 0, 1) {}

        void acquire()  { m_semaphore.acquire(); }

        template <class Rep, class Period>
        bool try_acquire_for(const chrono::duration<Rep, Period>& rel_time)
        {
            return m_semaphore.try_acquire_for(rel_time);
        }

        template <class Clock, class Duration>
        bool try_acquire_until(const chrono::time_point<Clock, Duration>& abs_time)
        {
            m_semaphore.try_acquire_until(abs_time);
        }

        void release()  { m_semaphore.release(1); }

        native_handle_type native_handle() { return m_semaphore.native_handle(); }
    private:
        AZStd::semaphore m_semaphore;
#endif //
    };
}

#endif // AZSTD_SEMAPHORE_H
#pragma once
