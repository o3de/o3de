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

#pragma once

#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/tuple.h>

namespace AZStd
{
    /**
     * scoped_lock is a mutex wrapper that provides RAII style mechanism for
     * owning multiple mutexes in a scoped block
     */
    template<class... Mutexes>
    class scoped_lock;

    template<>
    class scoped_lock<>
    {
    public:
        scoped_lock() = default;
        explicit scoped_lock(adopt_lock_t)
        {
        }
        ~scoped_lock() = default;

    private:
        scoped_lock(scoped_lock const&) = delete;
        scoped_lock& operator=(scoped_lock const&) = delete;
    };

    template<class Mutex>
    class scoped_lock<Mutex>
    {
    public:
        using mutex_type = Mutex;
        explicit scoped_lock(Mutex& mutex)
            : m_mutex(mutex)
        {
            m_mutex.lock();
        }
        scoped_lock(adopt_lock_t, Mutex& mutex)
            : m_mutex(mutex)
        {
        }
        ~scoped_lock()
        {
            m_mutex.unlock();
        }

    private:
        scoped_lock(scoped_lock const&) = delete;
        scoped_lock& operator=(scoped_lock const&) = delete;
        Mutex& m_mutex;
    };

    template<class... Mutexes>
    class scoped_lock
    {
    public:
        static_assert(sizeof...(Mutexes) >= 2, "variadic scoped_lock specialization requires at least 2 mutexes");

        explicit scoped_lock(Mutexes&... mutexes)
            : m_mutexes(mutexes...)
        {
            AZStd::apply(AZStd::lock<Mutexes...>, m_mutexes);
        }
        scoped_lock(adopt_lock_t, Mutexes&... mutexes)
            : m_mutexes(mutexes...)
        {
        }
        ~scoped_lock()
        {
            AZStd::apply(AZStd::unlock<Mutexes...>, m_mutexes);
        }

    private:
        scoped_lock(scoped_lock const&) = delete;
        scoped_lock& operator=(scoped_lock const&) = delete;
        AZStd::tuple<Mutexes&...> m_mutexes;
    };
}
