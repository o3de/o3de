/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
// Auto critical section is the most commonly used type of auto lock.
//
//////////////////////////////////////////////////////////////////////////
typedef CryAutoLock<CryCriticalSection> CryAutoCriticalSection;

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
#endif

#if !defined _CRYTHREAD_CONDLOCK_GLITCH
typedef CryLockT<CRYLOCK_RECURSIVE> CryMutex;
#endif // !_CRYTHREAD_CONDLOCK_GLITCH

// Include all multithreading containers.
#include "MultiThread_Containers.h"
