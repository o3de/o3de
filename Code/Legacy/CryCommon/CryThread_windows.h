/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <process.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define CRYTHREAD_WINDOWS_H_SECTION_1 1
#define CRYTHREAD_WINDOWS_H_SECTION_2 2
#endif

//////////////////////////////////////////////////////////////////////////
// CryEvent represent a synchronization event
//////////////////////////////////////////////////////////////////////////
class CryEvent
{
public:
    CryEvent();
    ~CryEvent();

    // Reset the event to the unsignalled state.
    void Reset();
    // Set the event to the signalled state.
    void Set();
    // Access a HANDLE to wait on.
    void* GetHandle() const { return m_handle; };
    // Wait indefinitely for the object to become signalled.
    void Wait() const;
    // Wait, with a time limit, for the object to become signalled.
    bool Wait(const uint32 timeoutMillis) const;

private:
    CryEvent(const CryEvent&);
    CryEvent& operator = (const CryEvent&);

private:
    void* m_handle;
};

typedef CryEvent CryEventTimed;

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// from winnt.h
struct CRY_CRITICAL_SECTION
{
    void* DebugInfo;
    long LockCount;
    long RecursionCount;
    threadID OwningThread;
    void* LockSemaphore;
    unsigned long* SpinCount;        // force size on 64-bit systems when packed
};

//////////////////////////////////////////////////////////////////////////

// kernel mutex - don't use... use CryMutex instead
class CryLock_WinMutex
{
public:
    CryLock_WinMutex();
    ~CryLock_WinMutex();

    void Lock();
    void Unlock();
    bool TryLock();

    void* _get_win32_handle() { return m_hdl; }

private:
    CryLock_WinMutex(const CryLock_WinMutex&);
    CryLock_WinMutex& operator = (const CryLock_WinMutex&);

private:
    void* m_hdl;
};

// critical section... don't use... use CryCriticalSection instead
class CryLock_CritSection
{
public:
    CryLock_CritSection();
    ~CryLock_CritSection();

    void Lock();
    void Unlock();
    bool TryLock();

    bool IsLocked()
    {
        return m_cs.RecursionCount > 0 && m_cs.OwningThread == CryGetCurrentThreadId();
    }

private:
    CryLock_CritSection(const CryLock_CritSection&);
    CryLock_CritSection& operator = (const CryLock_CritSection&);

private:
    CRY_CRITICAL_SECTION m_cs;
};

template <>
class CryLockT<CRYLOCK_RECURSIVE>
    : public CryLock_CritSection
{
};
template <>
class CryLockT<CRYLOCK_FAST>
    : public CryLock_CritSection
{
};
class CryMutex
    : public CryLock_WinMutex
{
};
#define _CRYTHREAD_CONDLOCK_GLITCH 1

//////////////////////////////////////////////////////////////////////////
class CryConditionVariable
{
public:
    typedef CryMutex LockType;

    CryConditionVariable();
    ~CryConditionVariable();
    void Wait(LockType& lock);
    bool TimedWait(LockType& lock, uint32 millis);
    void NotifySingle();
    void Notify();

private:
    CryConditionVariable(const CryConditionVariable&);
    CryConditionVariable& operator = (const CryConditionVariable&);

private:
    int m_waitersCount;
    CRY_CRITICAL_SECTION m_waitersCountLock;
    void* m_sema;
    void* m_waitersDone;
    size_t m_wasBroadcast;
};

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
    void* m_Semaphore;
};

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
#if !defined(_CRYTHREAD_HAVE_RWLOCK)
class CryRWLock
{
    void* /*SRWLOCK*/ m_Lock;

    CryRWLock(const CryRWLock&);
    CryRWLock& operator= (const CryRWLock&);

public:
    CryRWLock();
    ~CryRWLock();

    void RLock();
    void RUnlock();

    void WLock();
    void WUnlock();

    void Lock();
    void Unlock();

#if defined(_CRYTHREAD_WANT_TRY_RWLOCK)
    // Enabling TryXXX requires Windows 7 or newer
    bool TryRLock();
    bool TryWLock();
    bool TryLock();
#endif
};

// Indicate that this implementation header provides an implementation for
// CryRWLock.
#define _CRYTHREAD_HAVE_RWLOCK 1
#endif

//////////////////////////////////////////////////////////////////////////
class CrySimpleThreadSelf
{
public:
    CrySimpleThreadSelf();
    void WaitForThread();
    virtual ~CrySimpleThreadSelf();
protected:
    void StartThread(unsigned (__stdcall * func)(void*), void* argList);
    static THREADLOCAL CrySimpleThreadSelf* m_Self;
private:
    CrySimpleThreadSelf(const CrySimpleThreadSelf&);
    CrySimpleThreadSelf& operator = (const CrySimpleThreadSelf&);
protected:
    void* m_thread;
    uint32 m_threadId;
};

template<class Runnable>
class CrySimpleThread
    : public CryRunnable
    , public CrySimpleThreadSelf
{
public:
    typedef void (* ThreadFunction)(void*);
    typedef CryRunnable RunnableT;

    void SetName(const char* Name)
    {
        m_name = Name;
    }
    const char* GetName() { return m_name; }

    const volatile bool& GetStartedState() const { return m_bIsStarted; }

private:
    Runnable* m_Runnable;
    struct
    {
        ThreadFunction m_ThreadFunction;
        void* m_ThreadParameter;
    } m_ThreadFunction;
    volatile bool m_bIsStarted;
    volatile bool m_bIsRunning;
    volatile bool m_bCreatedThread;
    string m_name;

protected:
    virtual void Terminate()
    {
        // This method must be empty.
        // Derived classes overriding Terminate() are not required to call this
        // method.
    }

private:
    static unsigned __stdcall RunRunnable(void* thisPtr)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYTHREAD_WINDOWS_H_SECTION_1
    #include AZ_RESTRICTED_FILE(CryThread_windows_h)
#endif
        CrySimpleThread<Runnable>* const self = (CrySimpleThread<Runnable>*)thisPtr;
        self->m_bIsStarted = true;
        self->m_bIsRunning = true;

        self->m_Runnable->Run();
        self->m_bIsRunning = false;
        self->m_bCreatedThread = false;
        self->Terminate();
        return 0;
    }

    static unsigned __stdcall RunThis(void* thisPtr)
    {
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION CRYTHREAD_WINDOWS_H_SECTION_2
    #include AZ_RESTRICTED_FILE(CryThread_windows_h)
#endif
        CrySimpleThread<Runnable>* const self = (CrySimpleThread<Runnable>*)thisPtr;
        self->m_bIsStarted = true;
        self->m_bIsRunning = true;

        self->Run();
        self->m_bIsRunning = false;
        self->m_bCreatedThread = false;
        self->Terminate();
        return 0;
    }

    CrySimpleThread(const CrySimpleThread<Runnable>&);
    void operator = (const CrySimpleThread<Runnable>&);

public:
    CrySimpleThread()
        : m_bIsStarted(false)
        , m_bIsRunning(false)
        , m_bCreatedThread(false)
    {
        m_thread = NULL;
        m_Runnable = NULL;
    }
    void* GetHandle() { return m_thread; }

    virtual ~CrySimpleThread()
    {
        if (IsStarted())
        {
            if (gEnv && gEnv->pLog)
            {
                gEnv->pLog->LogError("Runaway thread %p '%s'", m_thread, m_name.c_str());
            }
        }

        if (m_bCreatedThread)
        {
            Cancel();
            WaitForThread();
        }
    }

    virtual void Run()
    {
        // This Run() implementation supports the void StartFunction() method.
        // However, code using this class (or derived classes) should eventually
        // be refactored to use one of the other Start() methods.  This code will
        // be removed some day and the default implementation of Run() will be
        // empty.
        if (m_ThreadFunction.m_ThreadFunction != NULL)
        {
            m_ThreadFunction.m_ThreadFunction(m_ThreadFunction.m_ThreadParameter);
        }
    }

    // Cancel the running thread.
    //
    // If the thread class is implemented as a derived class of CrySimpleThread,
    // then the derived class should provide an appropriate implementation for
    // this method.  Calling the base class implementation is _not_ required.
    //
    // If the thread was started by specifying a Runnable (template argument),
    // then the Cancel() call is passed on to the specified runnable.
    //
    // If the thread was started using the StartFunction() method, then the
    // caller must find other means to inform the thread about the cancellation
    // request.
    virtual void Cancel()
    {
        if (IsStarted() && m_Runnable != NULL)
        {
            m_Runnable->Cancel();
        }
    }

    virtual void Start(Runnable& runnable, [[maybe_unused]] unsigned cpuMask = 0, const char* = NULL, int32 = 0)
    {
        if (m_bCreatedThread)
        {
            // Don't start thread more than once!
            return;
        }
        m_Runnable = &runnable;
        m_bCreatedThread = true;
        StartThread(RunRunnable, this);
    }

    virtual void Start([[maybe_unused]] unsigned cpuMask = 0, const char* = NULL, int32 = 0, int32 = 0)
    {
        if (m_bCreatedThread)
        {
            // Don't start thread more than once!
            return;
        }
        m_bCreatedThread = true;
        StartThread(RunThis, this);
    }

    void StartFunction(
        ThreadFunction threadFunction,
        void* threadParameter = NULL
        )
    {
        m_ThreadFunction.m_ThreadFunction = threadFunction;
        m_ThreadFunction.m_ThreadParameter = threadParameter;
        Start();
    }

    static CrySimpleThread<Runnable>* Self()
    {
        return reinterpret_cast<CrySimpleThread<Runnable>*>(m_Self);
    }

    void Exit()
    {
        assert(!"implemented");
    }

    void Stop()
    {
        m_bIsStarted = false;
    }

    bool IsStarted() const { return m_bIsStarted; }
    bool IsRunning() const { return m_bIsRunning; }
};

///////////////////////////////////////////////////////////////////////////////
// base class for lock less Producer/Consumer queue, due platforms specific they
// are implemented in CryThead_platform.h
namespace CryMT {
    namespace detail {
        ///////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////
        class SingleProducerSingleConsumerQueueBase
        {
        public:
            SingleProducerSingleConsumerQueueBase()
            {}

            void Push(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, uint32 nBufferSize, void* arrBuffer, uint32 nObjectSize);
            void Pop(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, uint32 nBufferSize, void* arrBuffer, uint32 nObjectSize);
        };


        ///////////////////////////////////////////////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////
        class N_ProducerSingleConsumerQueueBase
        {
        public:
            N_ProducerSingleConsumerQueueBase()
            {
                CryInitializeSListHead(fallbackList);
            }

            void Push(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, volatile uint32& rRunning, void* arrBuffer, uint32 nBufferSize, uint32 nObjectSize, volatile uint32*   arrStates);
            bool Pop(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, volatile uint32& rRunning, void* arrBuffer, uint32 nBufferSize, uint32 nObjectSize, volatile uint32*    arrStates);

        private:
            SLockFreeSingleLinkedListHeader fallbackList;
            struct SFallbackList
            {
                SLockFreeSingleLinkedListEntry nextEntry;
                char alignment_padding[128 - sizeof(SLockFreeSingleLinkedListEntry)];
                char object[1]; // struct will be overallocated with enough memory for the object
            };
        };
    } // namespace detail
} // namespace CryMT
