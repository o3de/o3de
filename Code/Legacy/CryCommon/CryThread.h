/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Public include file for the multi-threading API.


#pragma once


// Include basic multithread primitives.
#include "MultiThread.h"
#include "BitFiddling.h"
#include <AzCore/std/string/string.h>
//////////////////////////////////////////////////////////////////////////
// Lock types:
//
// CRYLOCK_FAST
//   A fast potentially (non-recursive) mutex.
// CRYLOCK_RECURSIVE
//   A recursive mutex.
//////////////////////////////////////////////////////////////////////////
enum CryLockType
{
    CRYLOCK_FAST = 1,
    CRYLOCK_RECURSIVE = 2,
};

#define CRYLOCK_HAVE_FASTLOCK 1

/////////////////////////////////////////////////////////////////////////////
//
// Primitive locks and conditions.
//
// Primitive locks are represented by instance of class CryLockT<Type>
//
//
template<CryLockType Type>
class CryLockT
{
    /* Unsupported lock type. */
};

//////////////////////////////////////////////////////////////////////////
// Typedefs.
//////////////////////////////////////////////////////////////////////////
typedef CryLockT<CRYLOCK_RECURSIVE>   CryCriticalSection;
typedef CryLockT<CRYLOCK_FAST>        CryCriticalSectionNonRecursive;
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//
// CryAutoCriticalSection implements a helper class to automatically
// lock critical section in constructor and release on destructor.
//
//////////////////////////////////////////////////////////////////////////
template<class LockClass>
class CryAutoLock
{
private:
    LockClass* m_pLock;

    CryAutoLock();
    CryAutoLock(const CryAutoLock<LockClass>&);
    CryAutoLock<LockClass>& operator = (const CryAutoLock<LockClass>&);

public:
    CryAutoLock(LockClass& Lock)
        : m_pLock(&Lock) { m_pLock->Lock(); }
    CryAutoLock(const LockClass& Lock)
        : m_pLock(const_cast<LockClass*>(&Lock)) { m_pLock->Lock(); }
    ~CryAutoLock() { m_pLock->Unlock(); }
};

//////////////////////////////////////////////////////////////////////////
//
// CryOptionalAutoLock implements a helper class to automatically
// lock critical section (if needed) in constructor and release on destructor.
//
//////////////////////////////////////////////////////////////////////////
template<class LockClass>
class CryOptionalAutoLock
{
private:
    LockClass* m_Lock;
    bool m_bLockAcquired;

    CryOptionalAutoLock();
    CryOptionalAutoLock(const CryOptionalAutoLock<LockClass>&);
    CryOptionalAutoLock<LockClass>& operator = (const CryOptionalAutoLock<LockClass>&);

public:
    CryOptionalAutoLock(LockClass& Lock, bool acquireLock)
        : m_Lock(&Lock)
        , m_bLockAcquired(false)
    {
        if (acquireLock)
        {
            Acquire();
        }
    }
    ~CryOptionalAutoLock()
    {
        Release();
    }
    void Release()
    {
        if (m_bLockAcquired)
        {
            m_Lock->Unlock();
            m_bLockAcquired = false;
        }
    }
    void Acquire()
    {
        if (!m_bLockAcquired)
        {
            m_Lock->Lock();
            m_bLockAcquired = true;
        }
    }
};

//////////////////////////////////////////////////////////////////////////
//
// CryAutoSet implements a helper class to automatically
// set and reset value in constructor and release on destructor.
//
//////////////////////////////////////////////////////////////////////////
template<class ValueClass>
class CryAutoSet
{
private:
    ValueClass* m_pValue;

    CryAutoSet();
    CryAutoSet(const CryAutoSet<ValueClass>&);
    CryAutoSet<ValueClass>& operator = (const CryAutoSet<ValueClass>&);

public:
    CryAutoSet(ValueClass& value)
        : m_pValue(&value) { *m_pValue = (ValueClass)1; }
    ~CryAutoSet() { *m_pValue = (ValueClass)0; }
};

//////////////////////////////////////////////////////////////////////////
//
// Auto critical section is the most commonly used type of auto lock.
//
//////////////////////////////////////////////////////////////////////////
typedef CryAutoLock<CryCriticalSection> CryAutoCriticalSection;

#define AUTO_LOCK_T(Type, lock) PREFAST_SUPPRESS_WARNING(6246); CryAutoLock<Type> __AutoLock(lock)
#define AUTO_LOCK(lock)             AUTO_LOCK_T(CryCriticalSection, lock)
#define AUTO_LOCK_CS(csLock) CryAutoCriticalSection __AL__##csLock(csLock)

/////////////////////////////////////////////////////////////////////////////
//
// Threads.

// Base class for runnable objects.
//
// A runnable is an object with a Run() and a Cancel() method.  The Run()
// method should perform the runnable's job.  The Cancel() method may be
// called by another thread requesting early termination of the Run() method.
// The runnable may ignore the Cancel() call, the default implementation of
// Cancel() does nothing.
class CryRunnable
{
public:
    virtual ~CryRunnable() { }
    virtual void Run() = 0;
    virtual void Cancel() { }
};

// Class holding information about a thread.
//
// A reference to the thread information can be obtained by calling GetInfo()
// on the CrySimpleThread (or derived class) instance.
//
// NOTE:
// If the code is compiled with NO_THREADINFO defined, then the GetInfo()
// method will return a reference to a static dummy instance of this
// structure.  It is currently undecided if NO_THREADINFO will be defined for
// release builds!

struct CryThreadInfo
{
    // The symbolic name of the thread.
    //
    // You may set this name directly or through the SetName() method of
    // CrySimpleThread (or derived class).
    AZStd::string m_Name;


    // A thread identification number.
    // The number is unique but architecture specific.  Do not assume anything
    // about that number except for being unique.
    //
    // This field is filled when the thread is started (i.e. before the Run()
    // method or thread routine is called).  It is advised that you do not
    // change this number manually.
    uint32 m_ID;
};

// Simple thread class.
//
// CrySimpleThread is a simple wrapper around a system thread providing
// nothing but system-level functionality of a thread.  There are two typical
// ways to use a simple thread:
//
// 1. Derive from the CrySimpleThread class and provide an implementation of
//    the Run() (and optionally Cancel()) methods.
// 2. Specify a runnable object when the thread is started.  The default
//    runnable type is CryRunnable.
//
// The Runnable class specfied as the template argument must provide Run()
// and Cancel() methods compatible with the following signatures:
//
//   void Runnable::Run();
//   void Runnable::Cancel();
//
// If the Runnable does not support cancellation, then the Cancel() method
// should do nothing.
//
// The same instance of CrySimpleThread may be used for multiple thread
// executions /in sequence/, i.e. it is valid to re-start the thread by
// calling Start() after the thread has been joined by calling WaitForThread().
template<class Runnable = CryRunnable>
class CrySimpleThread;

// Standard thread class.
//
// The class provides a lock (mutex) and an associated condition variable.  If
// you don't need the lock, then you should used CrySimpleThread instead of
// CryThread.
template<class Runnable = CryRunnable>
class CryThread;

///////////////////////////////////////////////////////////////////////////////
// Include architecture specific code.
#if AZ_LEGACY_CRYCOMMON_TRAIT_USE_PTHREADS
#include <CryThread_pthreads.h>
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(WIN32) || defined(WIN64)
#include <CryThread_windows.h>
#define AZ_RESTRICTED_SECTION_IMPLEMENTED
#elif defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(CryThread_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
// Put other platform specific includes here!
#include <CryThread_dummy.h>
#endif

#if !defined _CRYTHREAD_CONDLOCK_GLITCH
typedef CryLockT<CRYLOCK_RECURSIVE> CryMutex;
#endif // !_CRYTHREAD_CONDLOCK_GLITCH

// The the architecture specific code does not define a class CryRWLock, then
// a default implementation is provided here.
#if !defined _CRYTHREAD_HAVE_RWLOCK && !defined _CRYTHREAD_CONDLOCK_GLITCH
class CryRWLock
{
    CryCriticalSection m_lockExclusiveAccess;
    CryCriticalSection m_lockSharedAccessComplete;
    CryConditionVariable m_condSharedAccessComplete;

    int m_nSharedAccessCount;
    int m_nCompletedSharedAccessCount;
    bool m_bExclusiveAccess;

    CryRWLock(const CryRWLock&);
    CryRWLock& operator= (const CryRWLock&);

    void AdjustSharedAccessCount()
    {
        m_nSharedAccessCount -= m_nCompletedSharedAccessCount;
        m_nCompletedSharedAccessCount = 0;
    }

public:
    CryRWLock()
        : m_nSharedAccessCount(0)
        , m_nCompletedSharedAccessCount(0)
        , m_bExclusiveAccess(false)
    { }

    void RLock()
    {
        m_lockExclusiveAccess.Lock();
        if (++m_nSharedAccessCount == INT_MAX)
        {
            m_lockSharedAccessComplete.Lock();
            AdjustSharedAccessCount();
            m_lockSharedAccessComplete.Unlock();
        }
        m_lockExclusiveAccess.Unlock();
    }

    bool TryRLock()
    {
        if (!m_lockExclusiveAccess.TryLock())
        {
            return false;
        }
        if (++m_nSharedAccessCount == INT_MAX)
        {
            m_lockSharedAccessComplete.Lock();
            AdjustSharedAccessCount();
            m_lockSharedAccessComplete.Unlock();
        }
        m_lockExclusiveAccess.Unlock();
        return true;
    }

    void RUnlock()
    {
        Unlock();
    }

    void WLock()
    {
        m_lockExclusiveAccess.Lock();
        m_lockSharedAccessComplete.Lock();
        assert(!m_bExclusiveAccess);
        AdjustSharedAccessCount();
        if (m_nSharedAccessCount > 0)
        {
            m_nCompletedSharedAccessCount -= m_nSharedAccessCount;
            do
            {
                m_condSharedAccessComplete.Wait(m_lockSharedAccessComplete);
            }
            while (m_nCompletedSharedAccessCount < 0);
            m_nSharedAccessCount = 0;
        }
        m_bExclusiveAccess = true;
    }

    bool TryWLock()
    {
        if (!m_lockExclusiveAccess.TryLock())
        {
            return false;
        }
        if (!m_lockSharedAccessComplete.TryLock())
        {
            m_lockExclusiveAccess.Unlock();
            return false;
        }
        assert(!m_bExclusiveAccess);
        AdjustSharedAccessCount();
        if (m_nSharedAccessCount > 0)
        {
            m_lockSharedAccessComplete.Unlock();
            m_lockExclusiveAccess.Unlock();
            return false;
        }
        else
        {
            m_bExclusiveAccess = true;
        }
        return true;
    }

    void WUnlock()
    {
        Unlock();
    }

    void Unlock()
    {
        if (!m_bExclusiveAccess)
        {
            m_lockSharedAccessComplete.Lock();
            if (++m_nCompletedSharedAccessCount == 0)
            {
                m_condSharedAccessComplete.NotifySingle();
            }
            m_lockSharedAccessComplete.Unlock();
        }
        else
        {
            m_bExclusiveAccess = false;
            m_lockSharedAccessComplete.Unlock();
            m_lockExclusiveAccess.Unlock();
        }
    }
};
#endif // !defined _CRYTHREAD_HAVE_RWLOCK

// Thread class.
//
// CryThread is an extension of CrySimpleThread providing a lock (mutex) and a
// condition variable per instance.
template<class Runnable>
class CryThread
    : public CrySimpleThread<Runnable>
{
    CryMutex m_Lock;
    CryConditionVariable m_Cond;

    CryThread(const CryThread<Runnable>&);
    void operator = (const CryThread<Runnable>&);

public:
    CryThread() { }
    void Lock() { m_Lock.Lock(); }
    bool TryLock() { return m_Lock.TryLock(); }
    void Unlock() { m_Lock.Unlock(); }
    void Wait() { m_Cond.Wait(m_Lock); }
    // Timed wait on the associated condition.
    //
    // The 'milliseconds' parameter specifies the relative timeout in
    // milliseconds.  The method returns true if a notification was received and
    // false if the specified timeout expired without receiving a notification.
    //
    // UNIX note: the method will _not_ return if the calling thread receives a
    // signal.  Instead the call is re-started with the _original_ timeout
    // value.  This misfeature may be fixed in the future.
    bool TimedWait(uint32 milliseconds)
    {
        return m_Cond.TimedWait(m_Lock, milliseconds);
    }
    void Notify() { m_Cond.Notify(); }
    void NotifySingle() { m_Cond.NotifySingle(); }
    CryMutex& GetLock() { return m_Lock; }
};

//////////////////////////////////////////////////////////////////////////
//
// Sync primitive for multiple reads and exclusive locking change access
//
//  Desc:
//      Useful in case if you have rarely modified object that needs
//      to be read quite often from different threads but still
//      need to be exclusively modified sometimes
//  Debug functionality:
//      Can be used for debug-only lock calls, which verify that no
//      simultaneous access is attempted.
//      Use the bDebug argument of LockRead or LockModify,
//      or use the DEBUG_READLOCK or DEBUG_MODIFYLOCK macros.
//      There is no overhead in release builds, if you use the macros,
//      and the lock definition is inside #ifdef _DEBUG.
//////////////////////////////////////////////////////////////////////////

class CryReadModifyLock
{
public:
    CryReadModifyLock()
        : m_modifyCount(0)
        , m_readCount(0)
    {
        SetDebugLocked(false);
    }

    bool LockRead(bool bTry = false, cstr strDebug = 0, bool bDebug = false) const
    {
        if (!WriteLock(bTry, bDebug, strDebug))         // wait until write unlocked
        {
            return false;
        }
        CryInterlockedIncrement(&m_readCount);          // increment read counter
        m_writeLock.Unlock();
        return true;
    }
    void UnlockRead() const
    {
        SetDebugLocked(false);
        const int counter = CryInterlockedDecrement(&m_readCount);       // release read
        assert(counter >= 0);
        if (m_writeLock.TryLock())
        {
            m_writeLock.Unlock();
        }
        else
        if (counter == 0 && m_modifyCount)
        {
            m_ReadReleased.Set();                                           // signal the final read released
        }
    }
    bool LockModify(bool bTry = false, cstr strDebug = 0, bool bDebug = false) const
    {
        if (!WriteLock(bTry, bDebug, strDebug))
        {
            return false;
        }
        CryInterlockedIncrement(&m_modifyCount);        // increment write counter (counter is for nested cases)
        while (m_readCount)
        {
            m_ReadReleased.Wait();                                      // wait for all threads finish read operation
        }
        return true;
    }
    void UnlockModify() const
    {
        SetDebugLocked(false);
#if !defined(NDEBUG)
        int counter =
#endif
            CryInterlockedDecrement(&m_modifyCount);      // decrement write counter
        assert(counter >= 0);
        m_writeLock.Unlock();                                               // release exclusive lock
    }

protected:
    mutable volatile int          m_readCount;
    mutable volatile int          m_modifyCount;
    mutable CryEvent              m_ReadReleased;
    mutable CryCriticalSection    m_writeLock;
    mutable bool                                    m_debugLocked;
    mutable const char*                     m_debugLockStr;

    void SetDebugLocked([[maybe_unused]] bool b, [[maybe_unused]] const char* str = 0) const
    {
#ifdef _DEBUG
        m_debugLocked = b;
        m_debugLockStr = str;
#endif
    }

    bool WriteLock(bool bTry, [[maybe_unused]] bool bDebug, [[maybe_unused]] const char* strDebug) const
    {
        if (!m_writeLock.TryLock())
        {
#ifdef _DEBUG
            assert(!m_debugLocked);
            assert(!bDebug);
#endif
            if (bTry)
            {
                return false;
            }
            m_writeLock.Lock();
        }
#ifdef _DEBUG
        if (!m_readCount && !m_modifyCount)                 // not yet locked
        {
            SetDebugLocked(bDebug, strDebug);
        }
#endif
        return true;
    }
};

// Auto-locking classes.
template<class T, bool bDEBUG = false>
class AutoLockRead
{
protected:
    const T& m_lock;
public:
    AutoLockRead(const T& lock, cstr strDebug = 0)
        : m_lock(lock) { m_lock.LockRead(bDEBUG, strDebug, bDEBUG); }
    ~AutoLockRead()
    { m_lock.UnlockRead(); }
};

template<class T, bool bDEBUG = false>
class AutoLockModify
{
protected:
    const T& m_lock;
public:
    AutoLockModify(const T& lock, cstr strDebug = 0)
        : m_lock(lock) { m_lock.LockModify(bDEBUG, strDebug, bDEBUG); }
    ~AutoLockModify()
    { m_lock.UnlockModify(); }
};

#define AUTO_READLOCK(p) PREFAST_SUPPRESS_WARNING(6246) AutoLockRead<CryReadModifyLock> AZ_JOIN(__readlock, __LINE__)(p, __FUNC__)
#define AUTO_READLOCK_PROT(p) PREFAST_SUPPRESS_WARNING(6246) AutoLockRead<CryReadModifyLock> AZ_JOIN(__readlock_prot, __LINE__)(p, __FUNC__)
#define AUTO_MODIFYLOCK(p) PREFAST_SUPPRESS_WARNING(6246) AutoLockModify<CryReadModifyLock> AZ_JOIN(__modifylock, __LINE__)(p, __FUNC__)

#if defined(_DEBUG)
    #define DEBUG_READLOCK(p) AutoLockRead<CryReadModifyLock> AZ_JOIN(__readlock, __LINE__)(p, __FUNC__)
    #define DEBUG_MODIFYLOCK(p) AutoLockModify<CryReadModifyLock> AZ_JOIN(__modifylock, __LINE__)(p, __FUNC__)
#else
    #define DEBUG_READLOCK(p)
    #define DEBUG_MODIFYLOCK(p)
#endif

// producer consumer queue implementations, but here instead of MultiThread_Container.h
// since they requiere platform specific code, and including windows.h in a very common
// header file leads to all kinds of problems
namespace CryMT
{
    //////////////////////////////////////////////////////////////////////////
    // Producer/Consumer Queue for 1 to 1 thread communication
    // Realized with only volatile variables and memory barriers
    // *warning* this producer/consumer queue is only thread safe in a 1 to 1 situation
    // and doesn't provide any yields or similar to prevent spinning
    //////////////////////////////////////////////////////////////////////////
    template<typename T>
    class SingleProducerSingleConsumerQueue
        : public CryMT::detail::SingleProducerSingleConsumerQueueBase
    {
    public:
        SingleProducerSingleConsumerQueue();
        ~SingleProducerSingleConsumerQueue();

        void Init(size_t nSize);

        void Push(const T& rObj);
        void Pop(T* pResult);
        uint32 Size() { return (m_nProducerIndex - m_nComsumerIndex); }
        uint32 BufferSize() { return m_nBufferSize;  }
        uint32 FreeCount() { return (m_nBufferSize - (m_nProducerIndex - m_nComsumerIndex)); }

    private:
        T* m_arrBuffer;
        uint32 m_nBufferSize;

        volatile uint32 m_nProducerIndex _ALIGN(16);
        volatile uint32 m_nComsumerIndex _ALIGN(16);
    } _ALIGN(128);

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline SingleProducerSingleConsumerQueue<T>::SingleProducerSingleConsumerQueue()
        : m_arrBuffer(NULL)
        , m_nBufferSize(0)
        , m_nProducerIndex(0)
        , m_nComsumerIndex(0)
    {}

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline SingleProducerSingleConsumerQueue<T>::~SingleProducerSingleConsumerQueue()
    {
        CryModuleMemalignFree(m_arrBuffer);
        m_nBufferSize = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline void SingleProducerSingleConsumerQueue<T>::Init(size_t nSize)
    {
        assert(m_arrBuffer == NULL);
        assert(m_nBufferSize == 0);
        assert((nSize & (nSize - 1)) == 0);

        m_arrBuffer = alias_cast<T*>(CryModuleMemalign(nSize * sizeof(T), 16));
        m_nBufferSize = nSize;
    }

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline void SingleProducerSingleConsumerQueue<T>::Push(const T& rObj)
    {
        assert(m_arrBuffer != NULL);
        assert(m_nBufferSize != 0);
        SingleProducerSingleConsumerQueueBase::Push((void*)&rObj, m_nProducerIndex, m_nComsumerIndex, m_nBufferSize, m_arrBuffer, sizeof(T));
    }

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline void SingleProducerSingleConsumerQueue<T>::Pop(T* pResult)
    {
        assert(m_arrBuffer != NULL);
        assert(m_nBufferSize != 0);
        SingleProducerSingleConsumerQueueBase::Pop(pResult, m_nProducerIndex, m_nComsumerIndex, m_nBufferSize, m_arrBuffer, sizeof(T));
    }

    //////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////
    // Producer/Consumer Queue for N to 1 thread communication
    // lockfree implemenation, to copy with multiple producers,
    // a internal producer refcount is managed, the queue is empty
    // as soon as there are no more producers and no new elements
    //////////////////////////////////////////////////////////////////////////
    template<typename T>
    class N_ProducerSingleConsumerQueue
        : public CryMT::detail::N_ProducerSingleConsumerQueueBase
    {
    public:
        N_ProducerSingleConsumerQueue();
        ~N_ProducerSingleConsumerQueue();

        void Init(size_t nSize);

        void Push(const T& rObj);
        bool Pop(T* pResult);

        // needs to be called before using, assumes that there is at least one producer
        // so the first one doesn't need to call AddProducer, but he has to deregister itself
        void SetRunningState();

        // to correctly track when the queue is empty(and no new jobs will be added), refcount the producer
        void AddProducer();
        void RemoveProducer();

        uint32 Size() { return (m_nProducerIndex - m_nComsumerIndex); }
        uint32 BufferSize() { return m_nBufferSize; }
        uint32 FreeCount() { return (m_nBufferSize - (m_nProducerIndex - m_nComsumerIndex)); }

    private:
        T*                              m_arrBuffer;
        volatile uint32*    m_arrStates;
        uint32                      m_nBufferSize;

        volatile uint32 m_nProducerIndex;
        volatile uint32 m_nComsumerIndex;
        volatile uint32 m_nRunning;
        volatile uint32 m_nProducerCount;
    } _ALIGN(128);

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline N_ProducerSingleConsumerQueue<T>::N_ProducerSingleConsumerQueue()
        : m_arrBuffer(NULL)
        , m_arrStates(NULL)
        , m_nBufferSize(0)
        , m_nProducerIndex(0)
        , m_nComsumerIndex(0)
        , m_nRunning(0)
        , m_nProducerCount(0)
    {}

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline N_ProducerSingleConsumerQueue<T>::~N_ProducerSingleConsumerQueue()
    {
        CryModuleMemalignFree(m_arrBuffer);
        CryModuleMemalignFree((void*)m_arrStates);
        m_nBufferSize = 0;
    }

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline void N_ProducerSingleConsumerQueue<T>::Init(size_t nSize)
    {
        assert(m_arrBuffer == NULL);
        assert(m_arrStates == NULL);
        assert(m_nBufferSize == 0);
        assert((nSize & (nSize - 1)) == 0);

        m_arrBuffer = alias_cast<T*>(CryModuleMemalign(nSize * sizeof(T), 16));
        m_arrStates = alias_cast<uint32*>(CryModuleMemalign(nSize * sizeof(uint32), 16));
        memset((void*)m_arrStates, 0, sizeof(uint32) * nSize);
        m_nBufferSize = nSize;
    }

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline void N_ProducerSingleConsumerQueue<T>::SetRunningState()
    {
#if !defined(_RELEASE)
        if (m_nRunning == 1)
        {
            __debugbreak();
        }
#endif
        m_nRunning = 1;
        m_nProducerCount = 1;
    }

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline void N_ProducerSingleConsumerQueue<T>::AddProducer()
    {
        assert(m_arrBuffer != NULL);
        assert(m_arrStates != NULL);
        assert(m_nBufferSize != 0);
#if !defined(_RELEASE)
        if (m_nRunning == 0)
        {
            __debugbreak();
        }
#endif
        CryInterlockedIncrement((volatile int*)&m_nProducerCount);
    }

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline void N_ProducerSingleConsumerQueue<T>::RemoveProducer()
    {
        assert(m_arrBuffer != NULL);
        assert(m_arrStates != NULL);
        assert(m_nBufferSize != 0);
#if !defined(_RELEASE)
        if (m_nRunning == 0)
        {
            __debugbreak();
        }
#endif
        if (CryInterlockedDecrement((volatile int*)&m_nProducerCount) == 0)
        {
            m_nRunning = 0;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline void N_ProducerSingleConsumerQueue<T>::Push(const T& rObj)
    {
        assert(m_arrBuffer != NULL);
        assert(m_arrStates != NULL);
        assert(m_nBufferSize != 0);
        CryMT::detail::N_ProducerSingleConsumerQueueBase::Push((void*)&rObj, m_nProducerIndex, m_nComsumerIndex, m_nRunning, m_arrBuffer, m_nBufferSize, sizeof(T), m_arrStates);
    }

    ///////////////////////////////////////////////////////////////////////////////
    template<typename T>
    inline bool N_ProducerSingleConsumerQueue<T>::Pop(T* pResult)
    {
        assert(m_arrBuffer != NULL);
        assert(m_arrStates != NULL);
        assert(m_nBufferSize != 0);
        return CryMT::detail::N_ProducerSingleConsumerQueueBase::Pop(pResult, m_nProducerIndex, m_nComsumerIndex, m_nRunning, m_arrBuffer, m_nBufferSize, sizeof(T), m_arrStates);
    }
} //namespace CryMT

// Include all multithreading containers.
#include "MultiThread_Containers.h"
