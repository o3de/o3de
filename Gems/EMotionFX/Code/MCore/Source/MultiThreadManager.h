/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include "StandardHeaders.h"

// some c++11 concurrency features
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/condition_variable.h>


namespace MCore
{

    class MCORE_API Mutex
    {
        friend class ConditionVariable;
    public:
        MCORE_INLINE Mutex()                {}
        MCORE_INLINE ~Mutex()               {}

        MCORE_INLINE void Lock()            { m_mutex.lock(); }
        MCORE_INLINE void Unlock()          { m_mutex.unlock(); }
        MCORE_INLINE bool TryLock()         { return m_mutex.try_lock(); }

    private:
        AZStd::mutex  m_mutex;
    };


    class MCORE_API MutexRecursive
    {
    public:
        MCORE_INLINE MutexRecursive()       {}
        MCORE_INLINE ~MutexRecursive()      {}

        MCORE_INLINE void Lock()            { m_mutex.lock(); }
        MCORE_INLINE void Unlock()          { m_mutex.unlock(); }
        MCORE_INLINE bool TryLock()         { return m_mutex.try_lock(); }

    private:
        AZStd::recursive_mutex    m_mutex;
    };


    class MCORE_API ConditionVariable
    {
    public:
        MCORE_INLINE ConditionVariable()    {}
        MCORE_INLINE ~ConditionVariable()   {}

        MCORE_INLINE void Wait(Mutex& mtx, const AZStd::function<bool()>& predicate)              { AZStd::unique_lock<AZStd::mutex> lock(mtx.m_mutex); m_variable.wait(lock, predicate); }
        MCORE_INLINE void WaitWithTimeout(Mutex& mtx, uint32 microseconds, const AZStd::function<bool()>& predicate)  { AZStd::unique_lock<AZStd::mutex> lock(mtx.m_mutex); m_variable.wait_for(lock, AZStd::chrono::microseconds(microseconds), predicate); }
        MCORE_INLINE void NotifyOne()                                                           { m_variable.notify_one(); }
        MCORE_INLINE void NotifyAll()                                                           { m_variable.notify_all(); }

    private:
        AZStd::condition_variable m_variable;
    };


    class MCORE_API AtomicInt32
    {
    public:
        MCORE_INLINE AtomicInt32()                  { SetValue(0); }
        MCORE_INLINE ~AtomicInt32()                 {}

        MCORE_INLINE void SetValue(int32 value)     { m_atomic.store(value); }
        MCORE_INLINE int32 GetValue() const         { int32 value = m_atomic.load(); return value; }

        MCORE_INLINE int32 Increment()              { return m_atomic++; }
        MCORE_INLINE int32 Decrement()              { return m_atomic--; }

    private:
        AZStd::atomic<int32>  m_atomic;
    };



    class MCORE_API AtomicUInt32
    {
    public:
        MCORE_INLINE AtomicUInt32()                 { SetValue(0); }
        MCORE_INLINE ~AtomicUInt32()                {}

        MCORE_INLINE void SetValue(uint32 value)    { m_atomic.store(value); }
        MCORE_INLINE uint32 GetValue() const        { uint32 value = m_atomic.load(); return value; }

        MCORE_INLINE uint32 Increment()             { return m_atomic++; }
        MCORE_INLINE uint32 Decrement()             { return m_atomic--; }

    private:
        AZStd::atomic<uint32> m_atomic;
    };


    class MCORE_API AtomicSizeT
    {
    public:
        MCORE_INLINE AtomicSizeT()                  { SetValue(0); }

        MCORE_INLINE void SetValue(size_t value)    { m_atomic.store(value); }
        MCORE_INLINE size_t GetValue() const        { size_t value = m_atomic.load(); return value; }

        MCORE_INLINE size_t Increment()             { return m_atomic++; }
        MCORE_INLINE size_t Decrement()             { return m_atomic--; }

    private:
        AZStd::atomic<size_t> m_atomic;
    };


    class MCORE_API Thread
    {
    public:
        Thread() {}
        Thread(const AZStd::function<void()>& threadFunction)         { Init(threadFunction); }
        ~Thread() {}

        void Init(const AZStd::function<void()>& threadFunction)      { m_thread = AZStd::thread(threadFunction); }
        void Join()         { m_thread.join(); }

    private:
        AZStd::thread m_thread;
    };


    class MCORE_API LockGuard
    {
    public:
        MCORE_INLINE LockGuard(Mutex& mutex)        { m_mutex = &mutex; mutex.Lock(); }
        MCORE_INLINE ~LockGuard()                   { m_mutex->Unlock(); }

    private:
        Mutex*  m_mutex;
    };


    class MCORE_API LockGuardRecursive
    {
    public:
        MCORE_INLINE LockGuardRecursive(MutexRecursive& mutex)  { m_mutex = &mutex; mutex.Lock(); }
        MCORE_INLINE ~LockGuardRecursive()                      { m_mutex->Unlock(); }

    private:
        MutexRecursive* m_mutex;
    };


    class MCORE_API ConditionEvent
    {
    public:
        ConditionEvent()            { m_conditionValue = false; }
        ~ConditionEvent()           { }

        void Reset()                { m_conditionValue = false; }
        void Wait()
        {
            m_cv.Wait(m_mutex, [this] { return m_conditionValue; });
        }
        void WaitWithTimeout(uint32 microseconds)
        {
            m_cv.WaitWithTimeout(m_mutex, microseconds, [this] { return m_conditionValue; });
        }
        void NotifyAll() { { LockGuard lockMutex(m_mutex); m_conditionValue = true; } m_cv.NotifyAll(); }
        void NotifyOne() { { LockGuard lockMutex(m_mutex); m_conditionValue = true; } m_cv.NotifyOne(); }

    private:
        Mutex               m_mutex;
        ConditionVariable   m_cv;
        bool                m_conditionValue;
    };

}   // namespace MCore
