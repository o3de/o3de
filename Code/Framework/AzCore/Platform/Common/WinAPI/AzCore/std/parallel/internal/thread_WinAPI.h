/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_THREAD_WINDOWS_H
#define AZSTD_THREAD_WINDOWS_H

#ifndef YieldProcessor
#    define YieldProcessor _mm_pause
#endif

extern "C"
{
    // forward declare to avoid windows.h
    AZ_DLL_IMPORT unsigned long __stdcall GetCurrentThreadId(void);
}

#include <AzCore/std/tuple.h>

namespace AZStd
{
    namespace Internal
    {
        /**
         * Create and run thread
         */
        HANDLE create_thread(const thread_desc* desc, thread_info* ti, unsigned int* id);
    }

    //////////////////////////////////////////////////////////////////////////
    // thread
    template<class F, class... Args, typename>
    thread::thread(F&& f, Args&&... args)
        : thread(thread_desc{}, AZStd::forward<F>(f), AZStd::forward<Args>(args)...)
    {}

    template<class F, class... Args>
    thread::thread(const thread_desc& desc, F&& f, Args&&... args)
    {
        auto threadfunc = [fn = AZStd::forward<F>(f), argsTuple = AZStd::make_tuple(AZStd::forward<Args>(args)...)]() mutable -> void
        {
            AZStd::apply(AZStd::move(fn), AZStd::move(argsTuple));
        };
        Internal::thread_info* ti = Internal::create_thread_info(AZStd::move(threadfunc));
        m_thread.m_handle = Internal::create_thread(&desc, ti, &m_thread.m_id);
    }

    inline bool thread::joinable() const
    {
        if (m_thread.m_id == native_thread_invalid_id)
        {
            return false;
        }
        return m_thread.m_id != this_thread::get_id().m_id;
    }
    inline thread::id thread::get_id() const
    {
        return id(m_thread.m_id);
    }
    thread::native_handle_type
    inline thread::native_handle()
    {
        return m_thread.m_handle;
    }
    //////////////////////////////////////////////////////////////////////////

    namespace this_thread
    {
        AZ_FORCE_INLINE thread::id get_id()
        {
            DWORD threadId = GetCurrentThreadId();
            return thread::id(threadId);
        }

        AZ_FORCE_INLINE void yield()
        {
            ::Sleep(0);
        }

        AZ_FORCE_INLINE void pause(int numLoops)
        {
            for (int i = 0; i < numLoops; ++i)
            {
                YieldProcessor(); //pause instruction on x86, allows hyperthread to run
            }
        }

        template <class Rep, class Period>
        AZ_FORCE_INLINE void sleep_for(const chrono::duration<Rep, Period>& rel_time)
        {
            chrono::milliseconds toSleep = AZStd::chrono::duration_cast<chrono::milliseconds>(rel_time);
            ::Sleep((DWORD)toSleep.count());
        }
    }
}

#endif // AZSTD_THREAD_WINDOWS_H
#pragma once
