/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <unistd.h>
#include <sched.h>

#include <AzCore/std/tuple.h>

namespace AZStd
{
    namespace Internal
    {
        /**
         * Create and run thread
         */
        pthread_t create_thread(const thread_desc* desc, thread_info* ti);
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
        m_thread = Internal::create_thread(&desc, ti);
    }

    inline bool thread::joinable() const
    {
        if (pthread_equal(m_thread, native_thread_invalid_id))
        {
            return false;
        }
        return !pthread_equal(m_thread, this_thread::get_id().m_id);
    }

    inline thread::id thread::get_id() const
    {
        return thread::id(m_thread);
    }

    thread::native_handle_type
    inline thread::native_handle()
    {
        return m_thread;
    }
    //////////////////////////////////////////////////////////////////////////

    namespace this_thread
    {
        AZ_FORCE_INLINE thread::id get_id()
        {
            return thread::id(pthread_self());
        }

        AZ_FORCE_INLINE void yield()
        {
            sched_yield();
        }

        AZ_FORCE_INLINE void pause(int numLoops)
        {
            for (int i = 0; i < numLoops; ++i)
            {
                sched_yield();
            }
        }

        template <class Rep, class Period>
        AZ_FORCE_INLINE void sleep_for(const chrono::duration<Rep, Period>& rel_time)
        {
            chrono::microseconds toSleep = rel_time;
            //Note: nanosleep is reccomended over usleep
            usleep(static_cast<useconds_t>(toSleep.count()));
        }
    }
}
