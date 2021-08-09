/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/PlatformRestrictedFileDef.h>


#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

#include <ISystem.h>
#include <ILog.h>
#include <errno.h>

// Section dictionary
#if defined(AZ_RESTRICTED_PLATFORM)
#define CRYTHREAD_PTHREADS_H_SECTION_REGISTER_THREAD 1
#define CRYTHREAD_PTHREADS_H_SECTION_TRAITS 2
#define CRYTHREAD_PTHREADS_H_SECTION_PTHREADCOND 3
#define CRYTHREAD_PTHREADS_H_SECTION_SEMAPHORE_CONSTRUCT 4
#define CRYTHREAD_PTHREADS_H_SECTION_SEMAPHORE_DESTROY 5
#define CRYTHREAD_PTHREADS_H_SECTION_SEMAPHORE_ACQUIRE 6
#define CRYTHREAD_PTHREADS_H_SECTION_SEMAPHORE_RELEASE 7
#define CRYTHREAD_PTHREADS_H_SECTION_TRY_RLOCK 8
#define CRYTHREAD_PTHREADS_H_SECTION_TRY_WLOCK 9
#define CRYTHREAD_PTHREADS_H_SECTION_START_RUNNABLE 10
#define CRYTHREAD_PTHREADS_H_SECTION_START_CPUMASK 11
#define CRYTHREAD_PTHREADS_H_SECTION_START_CPUMASK_POSTCREATE 12
#define CRYTHREAD_PTHREADS_H_SECTION_SETCPUMASK 13
#define CRYTHREAD_PTHREADS_H_SECTION_START_RUNNABLE_CPUMASK_POSTCREATE 14
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYTHREAD_PTHREADS_H_SECTION_REGISTER_THREAD
    #include AZ_RESTRICTED_FILE(CryThread_pthreads_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#endif

#if defined(APPLE) || defined(ANDROID)
// PTHREAD_MUTEX_FAST_NP is only defined by Pthreads-w32, thus not on MAC
    #define PTHREAD_MUTEX_FAST_NP PTHREAD_MUTEX_NORMAL
#endif

#if !defined _CRYTHREAD_HAVE_LOCK
template<int PthreadMutexType>
class _PthreadLockBase;

template<int PthreadMutexType>
class _PthreadLockAttr
{
    friend class _PthreadLockBase<PthreadMutexType>;

protected:
    _PthreadLockAttr()
    {
        pthread_mutexattr_init(&m_Attr);
        pthread_mutexattr_settype(&m_Attr, PthreadMutexType);
    }
    ~_PthreadLockAttr()
    {
        pthread_mutexattr_destroy(&m_Attr);
    }
    pthread_mutexattr_t m_Attr;
};

template<int PthreadMutexType>
class _PthreadLockBase
{
protected:
    static pthread_mutexattr_t& GetAttr()
    {
        static _PthreadLockAttr<PthreadMutexType> m_Attr;
        return m_Attr.m_Attr;
    }
};

template<class LockClass, int PthreadMutexType>
class _PthreadLock
    : public _PthreadLockBase<PthreadMutexType>
{
    //#if defined(_DEBUG)
public:
    //#endif
    pthread_mutex_t m_Lock;

public:
    _PthreadLock()
        : LockCount(0)
    {
        pthread_mutex_init(
            &m_Lock,
            &_PthreadLockBase<PthreadMutexType>::GetAttr());
    }
    ~_PthreadLock() { pthread_mutex_destroy(&m_Lock); }

    void Lock() { pthread_mutex_lock(&m_Lock); CryInterlockedIncrement(&LockCount); }

    bool TryLock()
    {
        const int rc = pthread_mutex_trylock(&m_Lock);
        if (0 == rc)
        {
            CryInterlockedIncrement(&LockCount);
            return true;
        }
        return false;
    }

    void Unlock() { CryInterlockedDecrement(&LockCount); pthread_mutex_unlock(&m_Lock); }

    // Get the POSIX pthread_mutex_t.
    // Warning:
    // This method will not be available in the Win32 port of CryThread.
    pthread_mutex_t& Get_pthread_mutex_t() { return m_Lock; }

    bool IsLocked()
    {
#if defined(LINUX) || defined(APPLE)
        // implementation taken from CrysisWars
        return LockCount > 0;
#else
        return true;
#endif
    }

private:
    volatile int LockCount;
};

#if defined CRYLOCK_HAVE_FASTLOCK
    #if defined(_DEBUG) && defined(PTHREAD_MUTEX_ERRORCHECK_NP)
template<>
class CryLockT<CRYLOCK_FAST>
    : public _PthreadLock<CryLockT<CRYLOCK_FAST>, PTHREAD_MUTEX_ERRORCHECK_NP>
    #else
template<>
class CryLockT<CRYLOCK_FAST>
    : public _PthreadLock<CryLockT<CRYLOCK_FAST>, PTHREAD_MUTEX_FAST_NP>
    #endif
{
    CryLockT(const CryLockT<CRYLOCK_FAST>&);
    void operator = (const CryLockT<CRYLOCK_FAST>&);

public:
    CryLockT() { }
};
#endif // CRYLOCK_HAVE_FASTLOCK

template<>
class CryLockT<CRYLOCK_RECURSIVE>
    : public _PthreadLock<CryLockT<CRYLOCK_RECURSIVE>, PTHREAD_MUTEX_RECURSIVE>
{
    CryLockT(const CryLockT<CRYLOCK_RECURSIVE>&);
    void operator = (const CryLockT<CRYLOCK_RECURSIVE>&);

public:
    CryLockT() { }
};

#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYTHREAD_PTHREADS_H_SECTION_TRAITS
    #include AZ_RESTRICTED_FILE(CryThread_pthreads_h)
#else
#if !defined(LINUX) && !defined(APPLE)
#define CRYTHREAD_PTHREADS_H_TRAIT_DEFINE_CRYMUTEX 1
#endif
#endif

#if CRYTHREAD_PTHREADS_H_TRAIT_DEFINE_CRYMUTEX
#if defined CRYLOCK_HAVE_FASTLOCK
class CryMutex
    : public CryLockT<CRYLOCK_FAST>
{
};
#else
class CryMutex
    : public CryLockT<CRYLOCK_RECURSIVE>
{
};
#endif
#endif // CRYTHREAD_PTHREADS_TRAIT_DEFINE_CRYMUTEX

#define _CRYTHREAD_HAVE_LOCK 1

#endif // !defined _CRYTHREAD_HAVE_LOCK

#include "MemoryAccess.h"
