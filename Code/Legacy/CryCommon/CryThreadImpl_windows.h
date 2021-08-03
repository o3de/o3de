/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <process.h>
#include <AzCore/std/parallel/semaphore.h> // for CreateSemaphore
struct SThreadNameDesc
{
    DWORD dwType;
    LPCSTR szName;
    DWORD dwThreadID;
    DWORD dwFlags;
};

THREADLOCAL CrySimpleThreadSelf* CrySimpleThreadSelf::m_Self = NULL;

//////////////////////////////////////////////////////////////////////////
CryEvent::CryEvent()
{
    m_handle = (void*)CreateEvent(NULL, FALSE, FALSE, NULL);
}

//////////////////////////////////////////////////////////////////////////
CryEvent::~CryEvent()
{
    CloseHandle(m_handle);
}

//////////////////////////////////////////////////////////////////////////
void CryEvent::Reset()
{
    ResetEvent(m_handle);
}

//////////////////////////////////////////////////////////////////////////
void CryEvent::Set()
{
    SetEvent(m_handle);
}

//////////////////////////////////////////////////////////////////////////
void CryEvent::Wait() const
{
    WaitForSingleObject(m_handle, INFINITE);
}

//////////////////////////////////////////////////////////////////////////
bool CryEvent::Wait(const uint32 timeoutMillis) const
{
    if (WaitForSingleObject(m_handle, timeoutMillis) == WAIT_TIMEOUT)
    {
        return false;
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
// CryLock_WinMutex
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CryLock_WinMutex::CryLock_WinMutex()
    : m_hdl(CreateMutex(NULL, FALSE, NULL)) {}
CryLock_WinMutex::~CryLock_WinMutex()
{
    CloseHandle(m_hdl);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_WinMutex::Lock()
{
    WaitForSingleObject(m_hdl, INFINITE);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_WinMutex::Unlock()
{
    ReleaseMutex(m_hdl);
}

//////////////////////////////////////////////////////////////////////////
bool CryLock_WinMutex::TryLock()
{
    return WaitForSingleObject(m_hdl, 0) != WAIT_TIMEOUT;
}

//////////////////////////////////////////////////////////////////////////
// CryLock_CritSection
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CryLock_CritSection::CryLock_CritSection()
{
    InitializeCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
CryLock_CritSection::~CryLock_CritSection()
{
    DeleteCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_CritSection::Lock()
{
    EnterCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
void CryLock_CritSection::Unlock()
{
    LeaveCriticalSection((CRITICAL_SECTION*)&m_cs);
}

//////////////////////////////////////////////////////////////////////////
bool CryLock_CritSection::TryLock()
{
    return TryEnterCriticalSection((CRITICAL_SECTION*)&m_cs) != FALSE;
}

//////////////////////////////////////////////////////////////////////////
// most of this is taken from http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
//////////////////////////////////////////////////////////////////////////
CryConditionVariable::CryConditionVariable()
{
    m_waitersCount = 0;
    m_wasBroadcast = 0;
    m_sema = CreateSemaphore(NULL, 0, 0x7fffffff, NULL);
    InitializeCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
    m_waitersDone = CreateEvent(NULL, FALSE, FALSE, NULL);
}

//////////////////////////////////////////////////////////////////////////
CryConditionVariable::~CryConditionVariable()
{
    CloseHandle(m_sema);
    DeleteCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
    CloseHandle(m_waitersDone);
}

//////////////////////////////////////////////////////////////////////////
void CryConditionVariable::Wait(LockType& lock)
{
    EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
    m_waitersCount++;
    LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);

    SignalObjectAndWait(lock._get_win32_handle(), m_sema, INFINITE, FALSE);

    EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
    m_waitersCount--;
    bool lastWaiter = m_wasBroadcast && m_waitersCount == 0;
    LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);

    if (lastWaiter)
    {
        SignalObjectAndWait(m_waitersDone, lock._get_win32_handle(), INFINITE, FALSE);
    }
    else
    {
        WaitForSingleObject(lock._get_win32_handle(), INFINITE);
    }
}

//////////////////////////////////////////////////////////////////////////
bool CryConditionVariable::TimedWait(LockType& lock, uint32 millis)
{
    EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
    m_waitersCount++;
    LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);

    bool ok = true;
    if (WAIT_TIMEOUT == SignalObjectAndWait(lock._get_win32_handle(), m_sema, millis, FALSE))
    {
        ok = false;
    }

    EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
    m_waitersCount--;
    bool lastWaiter = m_wasBroadcast && m_waitersCount == 0;
    LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);

    if (lastWaiter)
    {
        SignalObjectAndWait(m_waitersDone, lock._get_win32_handle(), INFINITE, FALSE);
    }
    else
    {
        WaitForSingleObject(lock._get_win32_handle(), INFINITE);
    }

    return ok;
}

//////////////////////////////////////////////////////////////////////////
void CryConditionVariable::NotifySingle()
{
    EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
    bool haveWaiters = m_waitersCount > 0;
    LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
    if (haveWaiters)
    {
        ReleaseSemaphore(m_sema, 1, 0);
    }
}

//////////////////////////////////////////////////////////////////////////
void CryConditionVariable::Notify()
{
    EnterCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
    bool haveWaiters = false;
    if (m_waitersCount > 0)
    {
        m_wasBroadcast = 1;
        haveWaiters = true;
    }
    if (haveWaiters)
    {
        ReleaseSemaphore(m_sema, m_waitersCount, 0);
        LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
        WaitForSingleObject(m_waitersDone, INFINITE);
        m_wasBroadcast = 0;
    }
    else
    {
        LeaveCriticalSection((CRITICAL_SECTION*)&m_waitersCountLock);
    }
}

//////////////////////////////////////////////////////////////////////////
CrySemaphore::CrySemaphore(int nMaximumCount, int nInitialCount)
{
    m_Semaphore = (void*)CreateSemaphore(NULL, nInitialCount, nMaximumCount, NULL);
}

//////////////////////////////////////////////////////////////////////////
CrySemaphore::~CrySemaphore()
{
    CloseHandle((HANDLE)m_Semaphore);
}

//////////////////////////////////////////////////////////////////////////
void CrySemaphore::Acquire()
{
    WaitForSingleObject((HANDLE)m_Semaphore, INFINITE);
}

//////////////////////////////////////////////////////////////////////////
void CrySemaphore::Release()
{
    ReleaseSemaphore((HANDLE)m_Semaphore, 1, NULL);
}

//////////////////////////////////////////////////////////////////////////
CryFastSemaphore::CryFastSemaphore(int nMaximumCount, int nInitialCount)
    : m_Semaphore(nMaximumCount)
    , m_nCounter(nInitialCount)
{
}

//////////////////////////////////////////////////////////////////////////
CryFastSemaphore::~CryFastSemaphore()
{
}

//////////////////////////////////////////////////////////////////////////
void CryFastSemaphore::Acquire()
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
void CryFastSemaphore::Release()
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

//////////////////////////////////////////////////////////////////////////
CryRWLock::CryRWLock()
{
    STATIC_ASSERT(sizeof(m_Lock) == sizeof(PSRWLOCK), "RWLock-pointer has invalid size");
    InitializeSRWLock(reinterpret_cast<PSRWLOCK>(&m_Lock));
}

//////////////////////////////////////////////////////////////////////////
CryRWLock::~CryRWLock()
{
}

//////////////////////////////////////////////////////////////////////////
void CryRWLock::RLock()
{
    AcquireSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_Lock));
}

//////////////////////////////////////////////////////////////////////////
#if defined(_CRYTHREAD_WANT_TRY_RWLOCK)
bool CryRWLock::TryRLock()
{
    return TryAcquireSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_Lock)) != 0;
}
#endif

//////////////////////////////////////////////////////////////////////////
void CryRWLock::RUnlock()
{
    ReleaseSRWLockShared(reinterpret_cast<PSRWLOCK>(&m_Lock));
}

//////////////////////////////////////////////////////////////////////////
void CryRWLock::WLock()
{
    AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_Lock));
}

//////////////////////////////////////////////////////////////////////////
#if defined(_CRYTHREAD_WANT_TRY_RWLOCK)
bool CryRWLock::TryWLock()
{
    return TryAcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_Lock)) != 0;
}
#endif

//////////////////////////////////////////////////////////////////////////
void CryRWLock::WUnlock()
{
    ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&m_Lock));
}

//////////////////////////////////////////////////////////////////////////
void CryRWLock::Lock()
{
    WLock();
}

//////////////////////////////////////////////////////////////////////////
#if defined(_CRYTHREAD_WANT_TRY_RWLOCK)
bool CryRWLock::TryLock()
{
    return TryWLock();
}
#endif

//////////////////////////////////////////////////////////////////////////
void CryRWLock::Unlock()
{
    WUnlock();
}

//////////////////////////////////////////////////////////////////////////
CrySimpleThreadSelf::CrySimpleThreadSelf()
    : m_thread(NULL)
    , m_threadId(0)
{
}

//////////////////////////////////////////////////////////////////////////
void CrySimpleThreadSelf::WaitForThread()
{
    assert(m_thread);
    PREFAST_ASSUME(m_thread);
    if (GetCurrentThreadId() != m_threadId)
    {
        WaitForSingleObject((HANDLE)m_thread, INFINITE);
    }
}

CrySimpleThreadSelf::~CrySimpleThreadSelf()
{
    if (m_thread)
    {
        CloseHandle(m_thread);
    }
}

void CrySimpleThreadSelf::StartThread(unsigned (__stdcall * func)(void*), void* argList)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(CryThreadImpl_windows_h)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
    m_thread = (void*)_beginthreadex(NULL, 0, func, argList, CREATE_SUSPENDED, &m_threadId);
#endif
    assert(m_thread);
    PREFAST_ASSUME(m_thread);
    ResumeThread((HANDLE)m_thread);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void CryInterlockedPushEntrySList(SLockFreeSingleLinkedListHeader& list,  SLockFreeSingleLinkedListEntry& element)
{
    STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLIST_HEADER), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
    STATIC_CHECK(sizeof(SLockFreeSingleLinkedListEntry) >= sizeof(SLIST_ENTRY), CRY_INTERLOCKED_SLIST_ENTRY_HAS_WRONG_SIZE);

    assert(IsAligned(&list, MEMORY_ALLOCATION_ALIGNMENT) && "LockFree SingleLink List Header has wrong Alignment");
    assert(IsAligned(&element, MEMORY_ALLOCATION_ALIGNMENT) && "LockFree SingleLink List Entry has wrong Alignment");
    InterlockedPushEntrySList(alias_cast<PSLIST_HEADER>(&list), alias_cast<PSLIST_ENTRY>(&element));
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedPopEntrySList(SLockFreeSingleLinkedListHeader& list)
{
    STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLIST_HEADER), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);

    assert(IsAligned(&list, MEMORY_ALLOCATION_ALIGNMENT) && "LockFree SingleLink List Header has wrong Alignment");
    return reinterpret_cast<void*>(InterlockedPopEntrySList(alias_cast<PSLIST_HEADER>(&list)));
}

//////////////////////////////////////////////////////////////////////////
void CryInitializeSListHead(SLockFreeSingleLinkedListHeader& list)
{
    assert(IsAligned(&list, MEMORY_ALLOCATION_ALIGNMENT) && "LockFree SingleLink List Header has wrong Alignment");

    STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLIST_HEADER), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
    InitializeSListHead(alias_cast<PSLIST_HEADER>(&list));
}

//////////////////////////////////////////////////////////////////////////
void* CryInterlockedFlushSList(SLockFreeSingleLinkedListHeader& list)
{
    assert(IsAligned(&list, MEMORY_ALLOCATION_ALIGNMENT) && "LockFree SingleLink List Header has wrong Alignment");

    STATIC_CHECK(sizeof(SLockFreeSingleLinkedListHeader) == sizeof(SLIST_HEADER), CRY_INTERLOCKED_SLIST_HEADER_HAS_WRONG_SIZE);
    return InterlockedFlushSList(alias_cast<PSLIST_HEADER>(&list));
}

///////////////////////////////////////////////////////////////////////////////
// base class for lock less Producer/Consumer queue, due platforms specific they
// are implemeted in CryThead_platform.h
namespace CryMT {
    namespace detail {
        ///////////////////////////////////////////////////////////////////////////////
        void SingleProducerSingleConsumerQueueBase::Push(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, uint32 nBufferSize, void* arrBuffer, uint32 nObjectSize)
        {
            // spin if queue is full
            int iter = 0;
            while (rProducerIndex - rComsumerIndex == nBufferSize)
            {
                CryLowLatencySleep(iter++ > 10 ? 1 : 0);
            }

            MemoryBarrier();
            char* pBuffer = alias_cast<char*>(arrBuffer);
            uint32 nIndex = rProducerIndex % nBufferSize;

            memcpy(pBuffer + (nIndex * nObjectSize), pObj, nObjectSize);
            MemoryBarrier();
            rProducerIndex += 1;
            MemoryBarrier();
        }

        ///////////////////////////////////////////////////////////////////////////////
        void SingleProducerSingleConsumerQueueBase::Pop(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, uint32 nBufferSize, void* arrBuffer, uint32 nObjectSize)
        {
            MemoryBarrier();
            // busy-loop if queue is empty
            int iter = 0;
            while (rProducerIndex - rComsumerIndex == 0)
            {
                CryLowLatencySleep(iter++ > 10 ? 1 : 0);
            }

            char* pBuffer = alias_cast<char*>(arrBuffer);
            uint32 nIndex = rComsumerIndex % nBufferSize;

            memcpy(pObj, pBuffer + (nIndex * nObjectSize), nObjectSize);
            MemoryBarrier();
            rComsumerIndex += 1;
            MemoryBarrier();
        }

        ///////////////////////////////////////////////////////////////////////////////
        void N_ProducerSingleConsumerQueueBase::Push(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, [[maybe_unused]] volatile uint32& rRunning, void* arrBuffer, uint32 nBufferSize, uint32 nObjectSize, volatile uint32*    arrStates)
        {
            MemoryBarrier();
            uint32 nProducerIndex;
            uint32 nComsumerIndex;

            int iter = 0;
            do
            {
                nProducerIndex = rProducerIndex;
                nComsumerIndex = rComsumerIndex;

                if (nProducerIndex - nComsumerIndex == nBufferSize)
                {
                    CryLowLatencySleep(iter++ > 10 ? 1 : 0);
                    if (iter > 20) // 10 spins + 10 ms wait
                    {
                        uint32 nSizeToAlloc = sizeof(SFallbackList) + nObjectSize - 1;
                        SFallbackList* pFallbackEntry = (SFallbackList*)CryModuleMemalign(nSizeToAlloc, 128);
                        memcpy(pFallbackEntry->object, pObj, nObjectSize);
                        MemoryBarrier();
                        CryInterlockedPushEntrySList(fallbackList,  pFallbackEntry->nextEntry);
                        return;
                    }
                    continue;
                }

                if (CryInterlockedCompareExchange(alias_cast<volatile LONG*>(&rProducerIndex), nProducerIndex + 1, nProducerIndex) == nProducerIndex)
                {
                    break;
                }
            } while (true);

            MemoryBarrier();
            char* pBuffer = alias_cast<char*>(arrBuffer);
            uint32 nIndex = nProducerIndex % nBufferSize;

            memcpy(pBuffer + (nIndex * nObjectSize), pObj, nObjectSize);
            MemoryBarrier();
            arrStates[nIndex] = 1;
            MemoryBarrier();
        }

        ///////////////////////////////////////////////////////////////////////////////
        bool N_ProducerSingleConsumerQueueBase::Pop(void* pObj, volatile uint32& rProducerIndex, volatile uint32& rComsumerIndex, volatile uint32& rRunning, void* arrBuffer, uint32 nBufferSize, uint32 nObjectSize, volatile uint32* arrStates)
        {
            MemoryBarrier();

            // busy-loop if queue is empty
            int iter = 0;
            if (rRunning && rProducerIndex - rComsumerIndex == 0)
            {
                while (rRunning && rProducerIndex - rComsumerIndex == 0)
                {
                    CryLowLatencySleep(iter++ > 10 ? 1 : 0);
                }
            }

            if (rRunning == 0 && rProducerIndex - rComsumerIndex == 0)
            {
                SFallbackList* pFallback = (SFallbackList*)CryInterlockedPopEntrySList(fallbackList);
                IF (pFallback, 0)
                {
                    memcpy(pObj, pFallback->object, nObjectSize);
                    CryModuleMemalignFree(pFallback);
                    return true;
                }
                // if the queue was empty, make sure we really are empty
                return false;
            }

            iter = 0;
            while (arrStates[rComsumerIndex % nBufferSize] == 0)
            {
                CryLowLatencySleep(iter++ > 10 ? 1 : 0);
            }

            char* pBuffer = alias_cast<char*>(arrBuffer);
            uint32 nIndex = rComsumerIndex % nBufferSize;

            memcpy(pObj, pBuffer + (nIndex * nObjectSize), nObjectSize);
            MemoryBarrier();
            arrStates[nIndex] = 0;
            MemoryBarrier();
            rComsumerIndex += 1;
            MemoryBarrier();

            return true;
        }
    } // namespace detail
} // namespace CryMT
