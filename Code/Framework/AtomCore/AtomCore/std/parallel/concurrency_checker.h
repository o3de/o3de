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

#include <AzCore/std/parallel/atomic.h>
#include <AzCore/Debug/Trace.h>

#if defined(AZ_PROFILE_BUILD) || defined(AZ_DEBUG_BUILD)
#define AZ_CONCURRENCY_CHECKER_ENABLED
#endif

namespace AZStd
{
    //! Simple class for verifying that no concurrent access is occuring.
    //! This is *not* a synchronization primitive, and is intended simply for checking that no concurrency issues exist.
    //! It will be compiled out in release builds.
    //! Use concurrency_checker like a mutex (i.e. call soft_lock() and soft_unlock() around all instances of your data access).
    //! It will assert if there are multiple threads accessing the locked code/data at the same time.
    //! Expected use case is for defensive programming: when you do not expect any concurrent access within a system,
    //! but want to verify that it stays that way in the future, without incurring the overhead of a mutex.
    class concurrency_checker
    {
    public:
        AZ_FORCE_INLINE void soft_lock()
        {
#ifdef AZ_CONCURRENCY_CHECKER_ENABLED
            uint32_t count = ++m_concurrencyCounter;
            AZ_Assert(count == 1, "Concurrency check failed. Multiple threads are trying to access data at the same time, or there is a lock/unlock mismatch.");
#endif
        }

        AZ_FORCE_INLINE void soft_unlock()
        {
#ifdef AZ_CONCURRENCY_CHECKER_ENABLED
            uint32_t count = --m_concurrencyCounter;
            AZ_Assert(count == 0, "Concurrency check failed. If the assert in soft_lock() has not triggered already, then most likely there is a lock/unlock mismatch.");
#endif
        }

    private:
#ifdef AZ_CONCURRENCY_CHECKER_ENABLED
        AZStd::atomic_uint32_t m_concurrencyCounter = 0;
#endif
    };

    //! Simple scope wrapper for concurrency check (so you don't have to manually call soft_lock() and soft_unlock())
    class concurrency_check_scope
    {
    public:
        AZ_FORCE_INLINE explicit concurrency_check_scope(concurrency_checker& checker)
            : m_checker(checker)
        {
            m_checker.soft_lock();
        }

        AZ_FORCE_INLINE ~concurrency_check_scope()
        {
            m_checker.soft_unlock();
        }

    private:
        AZ_FORCE_INLINE concurrency_check_scope() = delete;
        AZ_FORCE_INLINE concurrency_check_scope(concurrency_check_scope const &) = delete;

        concurrency_checker& m_checker;
    };
} //namespace AZStd
