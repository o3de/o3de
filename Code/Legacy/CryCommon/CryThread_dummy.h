/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_CRYTHREAD_DUMMY_H
#define CRYINCLUDE_CRYCOMMON_CRYTHREAD_DUMMY_H
#pragma once

#include <AzCore/base.h>

//////////////////////////////////////////////////////////////////////////
CryEvent::CryEvent() {}
CryEvent::~CryEvent() {}
void CryEvent::Reset() {}
void CryEvent::Set() {}
void CryEvent::Wait() const {}
bool CryEvent::Wait(const uint32 timeoutMillis) const {}
typedef CryEvent CryEventTimed;

//////////////////////////////////////////////////////////////////////////
class _DummyLock
{
public:
    _DummyLock();

    void Lock();
    bool TryLock();
    void Unlock();

#if defined(AZ_DEBUG_BUILD)
    bool IsLocked();
#endif
};

template<>
class CryLock<CRYLOCK_FAST>
    : public _DummyLock
{
    CryLock(const CryLock<CRYLOCK_FAST>&);
    void operator = (const CryLock<CRYLOCK_FAST>&);

public:
    CryLock();
};

template<>
class CryLock<CRYLOCK_RECURSIVE>
    : public _DummyLock
{
    CryLock(const CryLock<CRYLOCK_RECURSIVE>&);
    void operator = (const CryLock<CRYLOCK_RECURSIVE>&);

public:
    CryLock();
};

template<>
class CryCondLock<CRYLOCK_FAST>
    : public CryLock<CRYLOCK_FAST>
{
};

template<>
class CryCondLock<CRYLOCK_RECURSIVE>
    : public CryLock<CRYLOCK_FAST>
{
};

template<>
class CryCond< CryLock<CRYLOCK_FAST> >
{
    typedef CryLock<CRYLOCK_FAST> LockT;
    CryCond(const CryCond<LockT>&);
    void operator = (const CryCond<LockT>&);

public:
    CryCond();

    void Notify();
    void NotifySingle();
    void Wait(LockT&);
    bool TimedWait(LockT &, uint32);
};

template<>
class CryCond< CryLock<CRYLOCK_RECURSIVE> >
{
    typedef CryLock<CRYLOCK_RECURSIVE> LockT;
    CryCond(const CryCond<LockT>&);
    void operator = (const CryCond<LockT>&);

public:
    CryCond();

    void Notify();
    void NotifySingle();
    void Wait(LockT&);
    bool TimedWait(LockT &, uint32);
};

class _DummyRWLock
{
public:
    _DummyRWLock() { }

    void RLock();
    bool TryRLock();
    void WLock();
    bool TryWLock();
    void Lock() { WLock(); }
    bool TryLock() { return TryWLock(); }
    void Unlock();
};

template<class Runnable>
class CrySimpleThread
    : public CryRunnable
{
public:
    typedef void (* ThreadFunction)(void*);

    CrySimpleThread();
    virtual ~CrySimpleThread();
#if !defined(NO_THREADINFO)
    CryThreadInfo& GetInfo();
#endif
    const char* GetName();
    void SetName(const char*);

    virtual void Run();
    virtual void Cancel();
    virtual void Start(Runnable&, unsigned = 0, const char* = NULL);
    virtual void Start(unsigned = 0, const char* = NULL);
    void StartFunction(ThreadFunction, void* = NULL, unsigned = 0);

    void Exit();
    void Join();
    unsigned SetCpuMask(unsigned);
    unsigned GetCpuMask();

    void Stop();
    bool IsStarted() const;
    bool IsRunning() const;
};

#endif // CRYINCLUDE_CRYCOMMON_CRYTHREAD_DUMMY_H

