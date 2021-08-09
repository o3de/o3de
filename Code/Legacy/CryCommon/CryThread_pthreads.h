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

#if !defined(RegisterThreadName)
    #define RegisterThreadName(id, name)
    #define UnRegisterThreadName(id)
#endif

#if defined(APPLE) || defined(ANDROID)
// PTHREAD_MUTEX_FAST_NP is only defined by Pthreads-w32, thus not on MAC
    #define PTHREAD_MUTEX_FAST_NP PTHREAD_MUTEX_NORMAL
#endif

// Define LARGE_THREAD_STACK to use larger than normal per-thread stack
#if defined(_DEBUG) && (defined(MAC) || defined(LINUX) || defined(AZ_PLATFORM_IOS))
#define LARGE_THREAD_STACK
#endif

#if defined(LINUX)
#undef RegisterThreadName
ILINE void RegisterThreadName(pthread_t id, const char* name)
{
    if ((!name) || (!id))
    {
        return;
    }
    int ret;
    // pthread names on linux are limited to 16 char
    if (strlen(name) >= 16)
    {
        char thread_name[16];
        memcpy(thread_name, name, 15);
        thread_name[15] = 0;
        ret = pthread_setname_np(id, thread_name);
    }
    else
    {
        ret = pthread_setname_np(id, name);
    }
    if (ret != 0)
    {
        CryLog("Failed to set thread name for %" PRI_THREADID ", name: %s", id, name);
    }
}
#endif

#if !defined _CRYTHREAD_HAVE_LOCK
template<class LockClass>
class _PthreadCond;
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
    friend class _PthreadCond<LockClass>;

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
#if !defined(LINUX) && !defined(APPLE)
#define CRYTHREAD_PTHREADS_H_TRAIT_SET_THREAD_NAME 1
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

template<class LockClass>
class _PthreadCond
{
    pthread_cond_t m_Cond;

public:
    _PthreadCond() { pthread_cond_init(&m_Cond, NULL); }
    ~_PthreadCond() { pthread_cond_destroy(&m_Cond); }
    void Notify() { pthread_cond_broadcast(&m_Cond); }
    void NotifySingle() { pthread_cond_signal(&m_Cond); }
    void Wait(LockClass& Lock) { pthread_cond_wait(&m_Cond, &Lock.m_Lock); }
    bool TimedWait(LockClass& Lock, uint32 milliseconds)
    {
        struct timeval now;
        struct timespec timeout;
        int err;

        gettimeofday(&now, NULL);
        while (true)
        {
            timeout.tv_sec = now.tv_sec + milliseconds / 1000;
            uint64 nsec = (uint64)now.tv_usec * 1000 + (uint64)milliseconds * 1000000;
            if (nsec >= 1000000000)
            {
                timeout.tv_sec += (long)(nsec / 1000000000);
                nsec %= 1000000000;
            }
            timeout.tv_nsec = (long)nsec;
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYTHREAD_PTHREADS_H_SECTION_PTHREADCOND
    #include AZ_RESTRICTED_FILE(CryThread_pthreads_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
            #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            err = pthread_cond_timedwait(&m_Cond, &Lock.m_Lock, &timeout);
            if (err == EINTR)
            {
                // Interrupted by a signal.
                continue;
            }
            else if (err == ETIMEDOUT)
            {
                return false;
            }
#endif
            else
            {
                assert(err == 0);
            }
            break;
        }
        return true;
    }

    // Get the POSIX pthread_cont_t.
    // Warning:
    // This method will not be available in the Win32 port of CryThread.
    pthread_cond_t& Get_pthread_cond_t() { return m_Cond; }
};

#if AZ_LEGACY_CRYCOMMON_TRAIT_USE_PTHREADS
template <class LockClass>
class CryConditionVariableT
    : public _PthreadCond<LockClass>
{
};

#if defined CRYLOCK_HAVE_FASTTLOCK
template<>
class CryConditionVariableT< CryLockT<CRYLOCK_FAST> >
    : public _PthreadCond< CryLockT<CRYLOCK_FAST> >
{
    typedef CryLockT<CRYLOCK_FAST> LockClass;
    CryConditionVariableT(const CryConditionVariableT<LockClass>&);
    CryConditionVariableT<LockClass>& operator = (const CryConditionVariableT<LockClass>&);

public:
    CryConditionVariableT() { }
};
#endif // CRYLOCK_HAVE_FASTLOCK

template<>
class CryConditionVariableT< CryLockT<CRYLOCK_RECURSIVE> >
    : public _PthreadCond< CryLockT<CRYLOCK_RECURSIVE> >
{
    typedef CryLockT<CRYLOCK_RECURSIVE> LockClass;
    CryConditionVariableT(const CryConditionVariableT<LockClass>&);
    CryConditionVariableT<LockClass>& operator = (const CryConditionVariableT<LockClass>&);

public:
    CryConditionVariableT() { }
};

#if !defined(_CRYTHREAD_CONDLOCK_GLITCH)
typedef CryConditionVariableT< CryLockT<CRYLOCK_RECURSIVE> > CryConditionVariable;
#else
typedef CryConditionVariableT< CryLockT<CRYLOCK_FAST> > CryConditionVariable;
#endif

#define _CRYTHREAD_HAVE_LOCK 1

#else // LINUX MAC

#if defined CRYLOCK_HAVE_FASTLOCK
template<>
class CryConditionVariable
    : public _PthreadCond< CryLockT<CRYLOCK_FAST> >
{
    typedef CryLockT<CRYLOCK_FAST> LockClass;
    CryConditionVariable(const CryConditionVariable&);
    CryConditionVariable& operator = (const CryConditionVariable&);

public:
    CryConditionVariable() { }
};
#endif // CRYLOCK_HAVE_FASTLOCK

template<>
class CryConditionVariable
    : public _PthreadCond< CryLockT<CRYLOCK_RECURSIVE> >
{
    typedef CryLockT<CRYLOCK_RECURSIVE> LockClass;
    CryConditionVariable(const CryConditionVariable&);
    CryConditionVariable& operator = (const CryConditionVariable&);

public:
    CryConditionVariable() { }
};

#define _CRYTHREAD_HAVE_LOCK 1

#endif // LINUX MAC
#endif // !defined _CRYTHREAD_HAVE_LOCK

//////////////////////////////////////////////////////////////////////////
// Platform independet wrapper for a counting semaphore
class CrySemaphore
{
public:
    CrySemaphore(int nMaximumCount, int nInitialCount = 0);
    ~CrySemaphore();

    void Acquire();
    void Release();

private:
#if defined(APPLE)
    // Apple only supports named semaphores so have to use sem_open/unlink/sem_close instead
    // of sem_open/sem_destroy, passing in this array for the name.
    char m_semaphoreName[L_tmpnam];
#endif
    sem_t* m_Semaphore;
};

//////////////////////////////////////////////////////////////////////////
inline CrySemaphore::CrySemaphore(int nMaximumCount, int nInitialCount)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYTHREAD_PTHREADS_H_SECTION_SEMAPHORE_CONSTRUCT
    #include AZ_RESTRICTED_FILE(CryThread_pthreads_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(APPLE)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wdeprecated-declarations"
    tmpnam(m_semaphoreName);
#   pragma clang diagnostic pop
    m_Semaphore = sem_open(m_semaphoreName, O_CREAT | O_EXCL, 0644, nInitialCount);
#else
    m_Semaphore = new sem_t;
    sem_init(m_Semaphore, 0, nInitialCount);
#endif
}

//////////////////////////////////////////////////////////////////////////
inline CrySemaphore::~CrySemaphore()
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYTHREAD_PTHREADS_H_SECTION_SEMAPHORE_DESTROY
    #include AZ_RESTRICTED_FILE(CryThread_pthreads_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(APPLE)
    sem_close(m_Semaphore);
    sem_unlink(m_semaphoreName);
#else
    sem_destroy(m_Semaphore);
    delete m_Semaphore;
#endif
}

//////////////////////////////////////////////////////////////////////////
inline void CrySemaphore::Acquire()
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYTHREAD_PTHREADS_H_SECTION_SEMAPHORE_ACQUIRE
    #include AZ_RESTRICTED_FILE(CryThread_pthreads_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    while (sem_wait(m_Semaphore) != 0 && errno == EINTR)
    {
        ;
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
inline void CrySemaphore::Release()
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYTHREAD_PTHREADS_H_SECTION_SEMAPHORE_RELEASE
    #include AZ_RESTRICTED_FILE(CryThread_pthreads_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
    #undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    sem_post(m_Semaphore);
#endif
}

//////////////////////////////////////////////////////////////////////////
// Platform independet wrapper for a counting semaphore
// except that this version uses C-A-S only until a blocking call is needed
// -> No kernel call if there are object in the semaphore

class CryFastSemaphore
{
public:
    CryFastSemaphore(int nMaximumCount, int nInitialCount = 0);
    ~CryFastSemaphore();
    void Acquire();
    void Release();

private:
    CrySemaphore m_Semaphore;
    volatile int32 m_nCounter;
};

//////////////////////////////////////////////////////////////////////////
inline CryFastSemaphore::CryFastSemaphore(int nMaximumCount, int nInitialCount)
    : m_Semaphore(nMaximumCount)
    , m_nCounter(nInitialCount)
{
}

//////////////////////////////////////////////////////////////////////////
inline CryFastSemaphore::~CryFastSemaphore()
{
}

/////////////////////////////////////////////////////////////////////////
inline void CryFastSemaphore::Acquire()
{
    int nCount = ~0;
    do
    {
        nCount = *const_cast<volatile int*>(&m_nCounter);
    } while (CryInterlockedCompareExchange(alias_cast<volatile LONG*>(&m_nCounter), nCount - 1, nCount) != nCount);

    // if the count would have been 0 or below, go to kernel semaphore
    if ((nCount - 1)  < 0)
    {
        m_Semaphore.Acquire();
    }
}

//////////////////////////////////////////////////////////////////////////
inline void CryFastSemaphore::Release()
{
    int nCount = ~0;
    do
    {
        nCount = *const_cast<volatile int*>(&m_nCounter);
    } while (CryInterlockedCompareExchange(alias_cast<volatile LONG*>(&m_nCounter), nCount + 1, nCount) != nCount);

    // wake up kernel semaphore if we have waiter
    if (nCount < 0)
    {
        m_Semaphore.Release();
    }
}

////////////////////////////////////////////////////////////////////////////////
// Provide TLS implementation using pthreads for those platforms without __thread
////////////////////////////////////////////////////////////////////////////////

struct SCryPthreadTLSBase
{
    SCryPthreadTLSBase(void (*pDestructor)(void*))
    {
        pthread_key_create(&m_kKey, pDestructor);
    }

    ~SCryPthreadTLSBase()
    {
        pthread_key_delete(m_kKey);
    }

    void* GetSpecific()
    {
        return pthread_getspecific(m_kKey);
    }

    void SetSpecific(const void* pValue)
    {
        pthread_setspecific(m_kKey, pValue);
    }

    pthread_key_t m_kKey;
};

template <typename T, bool bDirect>
struct SCryPthreadTLSImpl{};

template <typename T>
struct SCryPthreadTLSImpl<T, true>
    : private SCryPthreadTLSBase
{
    SCryPthreadTLSImpl()
        :   SCryPthreadTLSBase(NULL)
    {
    }

    T Get()
    {
        void* pSpecific(GetSpecific());
        return *reinterpret_cast<const T*>(&pSpecific);
    }

    void Set(const T& kValue)
    {
        SetSpecific(*reinterpret_cast<const void* const*>(&kValue));
    }
};

template <typename T>
struct SCryPthreadTLSImpl<T, false>
    : private SCryPthreadTLSBase
{
    SCryPthreadTLSImpl()
        :   SCryPthreadTLSBase(&Destroy)
    {
    }

    T* GetPtr()
    {
        T* pPtr(static_cast<T*>(GetSpecific()));
        if (pPtr == NULL)
        {
            pPtr = new T();
            SetSpecific(pPtr);
        }
        return pPtr;
    }

    static void Destroy(void* pPointer)
    {
        delete static_cast<T*>(pPointer);
    }

    const T& Get()
    {
        return *GetPtr();
    }

    void Set(const T& kValue)
    {
        *GetPtr() = kValue;
    }
};

template <typename T>
struct SCryPthreadTLS
    : SCryPthreadTLSImpl<T, sizeof(T) <= sizeof(void*)>
{
};


//////////////////////////////////////////////////////////////////////////
// CryEvent(Timed) represent a synchronization event
//////////////////////////////////////////////////////////////////////////
class CryEventTimed
{
public:
    ILINE CryEventTimed(){m_flag = false; }
    ILINE ~CryEventTimed(){}

    // Reset the event to the unsignalled state.
    void Reset();
    // Set the event to the signalled state.
    void Set();
    // Access a HANDLE to wait on.
    void* GetHandle() const { return NULL; };
    // Wait indefinitely for the object to become signalled.
    void Wait();
    // Wait, with a time limit, for the object to become signalled.
    bool Wait(const uint32 timeoutMillis);

private:
    // Lock for synchronization of notifications.
    CryCriticalSection m_lockNotify;
#if defined(LINUX) || defined(APPLE)
    CryConditionVariableT< CryLockT<CRYLOCK_RECURSIVE> > m_cond;
#else
    CryConditionVariable m_cond;
#endif
    volatile bool m_flag;
};

typedef CryEventTimed CryEvent;

#include "MemoryAccess.h"
