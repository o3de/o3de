/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
