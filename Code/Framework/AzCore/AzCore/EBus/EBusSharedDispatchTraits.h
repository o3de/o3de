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
     * EBusSharedDispatchTraits is a custom mutex type and lock guards that can be used with an EBus to allow for
     * parallel dispatch calls, but still prevents connects / disconnects during a dispatch.
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
     *   To use the traits, inherit from EBusSharedDispatchTraits<BusType>:
     *      class MyBus : public AZ::EBusSharedDispatchTraits<MyBus>
     * 
     *   Alternatively, you can directly define the specific traits via the following:
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
        EBusSharedDispatchMutexConnectLockGuard(EBusSharedDispatchMutex& mutex, AZStd::adopt_lock_t)
            : m_mutex(mutex)
        {
        }

        explicit EBusSharedDispatchMutexConnectLockGuard(EBusSharedDispatchMutex& mutex)
            : m_mutex(mutex)
        {
            AZ_Assert(!EBusType::IsInDispatchThisThread(), "Can't connect/disconnect while inside an event dispatch.");
            m_mutex.EventMutexLockExclusive();
        }

        ~EBusSharedDispatchMutexConnectLockGuard()
        {
            m_mutex.EventMutexUnlockExclusive();
        }

    private:
        EBusSharedDispatchMutexConnectLockGuard(EBusSharedDispatchMutexConnectLockGuard const&) = delete;
        EBusSharedDispatchMutexConnectLockGuard& operator=(EBusSharedDispatchMutexConnectLockGuard const&) = delete;
        EBusSharedDispatchMutex& m_mutex;
    };

    // Custom lock guard to handle Dispatch lock management.
    // This will lock/unlock the event mutex with a shared lock. It also allows for recursive shared locks by only holding the
    // shared lock at the top level of the recursion.
    // Here's how this works:
    //  - Each thread that has an EBus call ends up creating a lock guard.
    //  - The lock guard checks (via EBusType::IsInDispatchThisThread) whether or not it is the first EBus call to occur on this thread. 
    //  - If it is, it sets the boolean and share locks the shared_mutex.
    //  - If not, it doesn't, and accepts that something higher up the callstack has a share lock on the shared_mutex.
    //  - If a recursive call to the EBus occurs on the thread, it does _not_ grab the share lock.
    // This is required because shared_mutex by itself doesn't support recursion. Attempting to call lock_shared() twice in the same
    // thread will result in a deadlock.
    template<class EBusType>
    class EBusSharedDispatchMutexDispatchLockGuard
    {
    public:
        EBusSharedDispatchMutexDispatchLockGuard(EBusSharedDispatchMutex& mutex, AZStd::adopt_lock_t)
            : m_mutex(mutex)
        {
        }

        explicit EBusSharedDispatchMutexDispatchLockGuard(EBusSharedDispatchMutex& mutex)
            : m_mutex(mutex)
        {
            if (!EBusType::IsInDispatchThisThread())
            {
                m_ownSharedLockOnThread = true;
                m_mutex.EventMutexLockShared();
            }
        }

        ~EBusSharedDispatchMutexDispatchLockGuard()
        {
            if (m_ownSharedLockOnThread)
            {
                m_mutex.EventMutexUnlockShared();
            }
        }

    private:
        EBusSharedDispatchMutexDispatchLockGuard(EBusSharedDispatchMutexDispatchLockGuard const&) = delete;
        EBusSharedDispatchMutexDispatchLockGuard& operator=(EBusSharedDispatchMutexDispatchLockGuard const&) = delete;
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
        EBusSharedDispatchMutexCallstackLockGuard(EBusSharedDispatchMutex& mutex, AZStd::adopt_lock_t)
            : m_mutex(mutex)
        {
        }

        explicit EBusSharedDispatchMutexCallstackLockGuard(EBusSharedDispatchMutex& mutex)
            : m_mutex(mutex)
        {
            m_mutex.CallstackMutexLock();
        }

        ~EBusSharedDispatchMutexCallstackLockGuard()
        {
            m_mutex.CallstackMutexUnlock();
        }

    private:
        EBusSharedDispatchMutexCallstackLockGuard(EBusSharedDispatchMutexCallstackLockGuard const&) = delete;
        EBusSharedDispatchMutexCallstackLockGuard& operator=(EBusSharedDispatchMutexCallstackLockGuard const&) = delete;
        EBusSharedDispatchMutex& m_mutex;
    };

    // The EBusTraits that can be inherited from to automatically set up the MutexType and LockGuards.
    // To inherit, use "class MyBus : public AZ::EBusSharedDispatchTraits<MyBus>"
    template<class BusType>
    struct EBusSharedDispatchTraits : EBusTraits
    {
        using MutexType = AZ::EBusSharedDispatchMutex;

        template<typename MutexType, bool IsLocklessDispatch>
        using DispatchLockGuard = AZ::EBusSharedDispatchMutexDispatchLockGuard<AZ::EBus<BusType>>;

        template<typename MutexType>
        using ConnectLockGuard = AZ::EBusSharedDispatchMutexConnectLockGuard<AZ::EBus<BusType>>;

        template<typename MutexType>
        using CallstackTrackerLockGuard = AZ::EBusSharedDispatchMutexCallstackLockGuard<AZ::EBus<BusType>>;
    };

} // namespace AZ

