/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

        MCORE_INLINE void Lock()            { mMutex.lock(); }
        MCORE_INLINE void Unlock()          { mMutex.unlock(); }
        MCORE_INLINE bool TryLock()         { return mMutex.try_lock(); }

    private:
        AZStd::mutex  mMutex;
    };


    class MCORE_API MutexRecursive
    {
    public:
        MCORE_INLINE MutexRecursive()       {}
        MCORE_INLINE ~MutexRecursive()      {}

        MCORE_INLINE void Lock()            { mMutex.lock(); }
        MCORE_INLINE void Unlock()          { mMutex.unlock(); }
        MCORE_INLINE bool TryLock()         { return mMutex.try_lock(); }

    private:
        AZStd::recursive_mutex    mMutex;
    };


    class MCORE_API ConditionVariable
    {
    public:
        MCORE_INLINE ConditionVariable()    {}
        MCORE_INLINE ~ConditionVariable()   {}

        MCORE_INLINE void Wait(Mutex& mtx, const AZStd::function<bool()>& predicate)              { AZStd::unique_lock<AZStd::mutex> lock(mtx.mMutex); mVariable.wait(lock, predicate); }
        MCORE_INLINE void WaitWithTimeout(Mutex& mtx, uint32 microseconds, const AZStd::function<bool()>& predicate)  { AZStd::unique_lock<AZStd::mutex> lock(mtx.mMutex); mVariable.wait_for(lock, AZStd::chrono::microseconds(microseconds), predicate); }
        MCORE_INLINE void NotifyOne()                                                           { mVariable.notify_one(); }
        MCORE_INLINE void NotifyAll()                                                           { mVariable.notify_all(); }

    private:
        AZStd::condition_variable mVariable;
    };


    class MCORE_API AtomicInt32
    {
    public:
        MCORE_INLINE AtomicInt32()                  { SetValue(0); }
        MCORE_INLINE ~AtomicInt32()                 {}

        MCORE_INLINE void SetValue(int32 value)     { mAtomic.store(value); }
        MCORE_INLINE int32 GetValue() const         { int32 value = mAtomic.load(); return value; }

        MCORE_INLINE int32 Increment()              { return mAtomic++; }
        MCORE_INLINE int32 Decrement()              { return mAtomic--; }

    private:
        AZStd::atomic<int32>  mAtomic;
    };



    class MCORE_API AtomicUInt32
    {
    public:
        MCORE_INLINE AtomicUInt32()                 { SetValue(0); }
        MCORE_INLINE ~AtomicUInt32()                {}

        MCORE_INLINE void SetValue(uint32 value)    { mAtomic.store(value); }
        MCORE_INLINE uint32 GetValue() const        { uint32 value = mAtomic.load(); return value; }

        MCORE_INLINE uint32 Increment()             { return mAtomic++; }
        MCORE_INLINE uint32 Decrement()             { return mAtomic--; }

    private:
        AZStd::atomic<uint32> mAtomic;
    };


    class MCORE_API Thread
    {
    public:
        Thread() {}
        Thread(const AZStd::function<void()>& threadFunction)         { Init(threadFunction); }
        ~Thread() {}

        void Init(const AZStd::function<void()>& threadFunction)      { mThread = AZStd::thread(threadFunction); }
        void Join()         { mThread.join(); }

    private:
        AZStd::thread mThread;
    };


    class MCORE_API LockGuard
    {
    public:
        MCORE_INLINE LockGuard(Mutex& mutex)        { mMutex = &mutex; mutex.Lock(); }
        MCORE_INLINE ~LockGuard()                   { mMutex->Unlock(); }

    private:
        Mutex*  mMutex;
    };


    class MCORE_API LockGuardRecursive
    {
    public:
        MCORE_INLINE LockGuardRecursive(MutexRecursive& mutex)  { mMutex = &mutex; mutex.Lock(); }
        MCORE_INLINE ~LockGuardRecursive()                      { mMutex->Unlock(); }

    private:
        MutexRecursive* mMutex;
    };


    class MCORE_API ConditionEvent
    {
    public:
        ConditionEvent()            { mConditionValue = false; }
        ~ConditionEvent()           { }

        void Reset()                { mConditionValue = false; }
        void Wait()
        {
            mCV.Wait(mMutex, [this] { return mConditionValue; });
        }
        void WaitWithTimeout(uint32 microseconds)
        {
            mCV.WaitWithTimeout(mMutex, microseconds, [this] { return mConditionValue; });
        }
        void NotifyAll() { { LockGuard lockMutex(mMutex); mConditionValue = true; } mCV.NotifyAll(); }
        void NotifyOne() { { LockGuard lockMutex(mMutex); mConditionValue = true; } mCV.NotifyOne(); }

    private:
        Mutex               mMutex;
        ConditionVariable   mCV;
        bool                mConditionValue;
    };

}   // namespace MCore
