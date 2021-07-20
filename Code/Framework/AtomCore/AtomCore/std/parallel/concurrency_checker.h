/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
    //! Simple class for verifying that no concurrent access is occurring.
    //! This is *not* a synchronization primitive, and is intended simply for checking that no concurrency issues exist.
    //! It will be compiled out in release builds.
    //! Use concurrency_checker like a mutex (i.e. call soft_lock() and soft_unlock() around all instances of your data access).
    //! Use soft_lock_shared and soft_unlock_shared around places where multiple threads are allowed to have read access
    //! at the same time as long as nothing else already has a soft lock
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
            AZ_Assert(count == 1 && m_sharedConcurrencyCounter == 0, "Concurrency check failed. Multiple threads are trying to access data at the same time, or there is a lock/unlock mismatch.");
#endif
        }

        AZ_FORCE_INLINE void soft_unlock()
        {
#ifdef AZ_CONCURRENCY_CHECKER_ENABLED
            uint32_t count = --m_concurrencyCounter;
            AZ_Assert(count == 0, "Concurrency check failed. If the assert in soft_lock() has not triggered already, then most likely there is a lock/unlock mismatch.");
#endif
        }

        AZ_FORCE_INLINE void soft_lock_shared()
        {
#ifdef AZ_CONCURRENCY_CHECKER_ENABLED
            AZ_Assert(m_concurrencyCounter == 0, "Concurrency check failed. A soft_lock_shared was attempted when there was already a soft_lock.");
            ++m_sharedConcurrencyCounter;
#endif
        }

        AZ_FORCE_INLINE void soft_unlock_shared()
        {
#ifdef AZ_CONCURRENCY_CHECKER_ENABLED
            AZ_Assert(m_sharedConcurrencyCounter != 0, "Concurrency check failed. There is a shared_lock/shared_unlock mismatch.");
            --m_sharedConcurrencyCounter;
#endif
        }


    private:
#ifdef AZ_CONCURRENCY_CHECKER_ENABLED
        AZStd::atomic_uint32_t m_concurrencyCounter = 0;
        AZStd::atomic_uint32_t m_sharedConcurrencyCounter = 0;
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
