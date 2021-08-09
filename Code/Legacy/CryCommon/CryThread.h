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
