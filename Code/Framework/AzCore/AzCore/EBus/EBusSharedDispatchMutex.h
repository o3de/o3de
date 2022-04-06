/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/shared_mutex.h>

namespace AZ
{
    /**
     * %EBusSharedDispatchMutex is a custom mutex type that can be used with an EBus to allow for parallel dispatch calls, but still
     * prevents connects / disconnects during a dispatch.
     *
     * Features:
     *   - Event dispatches can execute in parallel when called on separate threads
     *   - Bus connects / disconnects will only execute when no event dispatches are executing
     *   - Event dispatches can call other event dispatches on the same bus recursively
     *
     * Limitations:
     *   - If the bus contains custom connect / disconnect logic, it must not call any event dispatches on the same bus.
     *   - Bus connects / disconnects cannot happen within event dispatches on the same bus.
     *
     * Usage:
     *   Add code like the following to the definition of a specific EBus:
     *      using MutexType = AZ::EBusSharedDispatchMutex;
     *
     *      template <typename MutexType, bool IsLocklessDispatch>
     *      using DispatchLockGuard = AZ::EBusSharedDispatchMutexDispatchLockGuard<AZ::EBus<MyBus>>;
     *
     *      template<typename MutexType>
     *      using ConnectLockGuard = AZ::EBusSharedDispatchMutexConnectLockGuard<AZ::EBus<MyBus>>;
     *
     *      template<typename MutexType>
     *      using CallstackTrackerLockGuard = AZ::EBusSharedDispatchMutexCallstackLockGuard<AZ::EBus<MyBus>>;
     */

    // Simple custom mutex class that contains a shared_mutex for use with connects / disconnects / event dispatches
    // and a separate mutex for callstack tracking thread protection.
    class EBusSharedDispatchMutex
    {
    public:
        EBusSharedDispatchMutex() = default;
        ~EBusSharedDispatchMutex() = default;

        void CallstackMutexLock()
        {
            m_callstackMutex.lock();
        }

        void CallstackMutexUnlock()
        {
            m_callstackMutex.unlock();
        }

        void EventMutexLockExclusive()
        {
            m_eventMutex.lock();
        }

        void EventMutexUnlockExclusive()
        {
            m_eventMutex.unlock();
        }

        void EventMutexLockShared()
        {
            m_eventMutex.lock_shared();
        }

        void EventMutexUnlockShared()
        {
            m_eventMutex.unlock_shared();
        }

    private:
        AZStd::shared_mutex m_eventMutex;
        AZStd::mutex m_callstackMutex;

        // This custom mutex type should only be used with the lock guards below since it needs additional context to know which
        // mutex to lock and what type of lock to request. If you get a compile error due to these methods being private, the EBus
        // declaration is likely missing one or more of the lock guards below.
        void lock(){}
        void unlock(){}
    };

    // Custom lock guard to handle Connection lock management.
    // This will lock/unlock the event mutex with an exclusive lock. It will assert and disallow
    // exclusive locks if currently inside of a shared lock.
    template<class EBusType>
    class EBusSharedDispatchMutexConnectLockGuard
    {
    public:
        AZ_FORCE_INLINE EBusSharedDispatchMutexConnectLockGuard(EBusSharedDispatchMutex& mutex, AZStd::adopt_lock_t)
            : m_mutex(mutex)
        {
        }

        AZ_FORCE_INLINE explicit EBusSharedDispatchMutexConnectLockGuard(EBusSharedDispatchMutex& mutex)
            : m_mutex(mutex)
        {
            AZ_Assert(!EBusType::IsInDispatchThisThread(), "Can't connect/disconnect while inside an event dispatch.");
            m_mutex.EventMutexLockExclusive();
        }
        AZ_FORCE_INLINE ~EBusSharedDispatchMutexConnectLockGuard()
        {
            m_mutex.EventMutexUnlockExclusive();
        }

    private:
        AZ_FORCE_INLINE EBusSharedDispatchMutexConnectLockGuard(EBusSharedDispatchMutexConnectLockGuard const&) = delete;
        AZ_FORCE_INLINE EBusSharedDispatchMutexConnectLockGuard& operator=(EBusSharedDispatchMutexConnectLockGuard const&) = delete;
        EBusSharedDispatchMutex& m_mutex;
    };

    // Custom lock guard to handle Dispatch lock management.
    // This will lock/unlock the event mutex with a shared lock. It also allows for recursive shared locks by only holding the
    // shared lock at the top level of the recursion.
    template<class EBusType>
    class EBusSharedDispatchMutexDispatchLockGuard
    {
    public:
        AZ_FORCE_INLINE EBusSharedDispatchMutexDispatchLockGuard(EBusSharedDispatchMutex& mutex, AZStd::adopt_lock_t)
            : m_mutex(mutex)
        {
        }

        AZ_FORCE_INLINE explicit EBusSharedDispatchMutexDispatchLockGuard(EBusSharedDispatchMutex& mutex)
            : m_mutex(mutex)
        {
            if (!EBusType::IsInDispatchThisThread())
            {
                m_ownSharedLockOnThread = true;
                m_mutex.EventMutexLockShared();
            }
        }
        AZ_FORCE_INLINE ~EBusSharedDispatchMutexDispatchLockGuard()
        {
            if (m_ownSharedLockOnThread)
            {
                m_mutex.EventMutexUnlockShared();
            }
        }

    private:
        AZ_FORCE_INLINE EBusSharedDispatchMutexDispatchLockGuard(EBusSharedDispatchMutexDispatchLockGuard const&) = delete;
        AZ_FORCE_INLINE EBusSharedDispatchMutexDispatchLockGuard& operator=(EBusSharedDispatchMutexDispatchLockGuard const&) = delete;
        EBusSharedDispatchMutex& m_mutex;
        bool m_ownSharedLockOnThread = false;
    };

    // Custom lock guard to handle callstack tracking lock management.
    // This uses a separate always-exclusive lock for the callstack tracking, regardless of whether or not we're in a shared lock
    // for event dispatches.
    template<class EBusType>
    class EBusSharedDispatchMutexCallstackLockGuard
    {
    public:
        AZ_FORCE_INLINE EBusSharedDispatchMutexCallstackLockGuard(EBusSharedDispatchMutex& mutex, AZStd::adopt_lock_t)
            : m_mutex(mutex)
        {
        }

        AZ_FORCE_INLINE explicit EBusSharedDispatchMutexCallstackLockGuard(EBusSharedDispatchMutex& mutex)
            : m_mutex(mutex)
        {
            m_mutex.CallstackMutexLock();
        }
        AZ_FORCE_INLINE ~EBusSharedDispatchMutexCallstackLockGuard()
        {
            m_mutex.CallstackMutexUnlock();
        }

    private:
        AZ_FORCE_INLINE EBusSharedDispatchMutexCallstackLockGuard(EBusSharedDispatchMutexCallstackLockGuard const&) = delete;
        AZ_FORCE_INLINE EBusSharedDispatchMutexCallstackLockGuard& operator=(EBusSharedDispatchMutexCallstackLockGuard const&) = delete;
        EBusSharedDispatchMutex& m_mutex;
        bool m_ownSharedLockOnThread = false;
    };

} // namespace AZ

