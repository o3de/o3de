/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_SYNCHRONIZATION_H
#define CRYINCLUDE_CRYCOMMON_SYNCHRONIZATION_H
#pragma once


//---------------------------------------------------------------------------
// Synchronization policies, for classes (e.g. containers, allocators) that may
// or may not be multithread-safe.
//
// Policies should be used as a template argument to such classes,
// and these class implementations should then utilise the policy, as a base-class or member.
//
//---------------------------------------------------------------------------

#include "MultiThread.h"
#include "CryThread.h"

namespace stl
{
    template<class Sync>
    struct AutoLock
    {
        ILINE AutoLock(Sync& sync)
            : _sync(sync)
        {
            sync.Lock();
        }
        ILINE ~AutoLock()
        {
            _sync.Unlock();
        }

    private:
        Sync& _sync;
    };


    struct PSyncNone
    {
        void Lock() {}
        void Unlock() {}
    };

    struct PSyncMultiThread
    {
        PSyncMultiThread()
            : _Semaphore(0) {}

        void Lock()
        {
            CryWriteLock(&_Semaphore);
        }
        void Unlock()
        {
            CryReleaseWriteLock(&_Semaphore);
        }
        int IsLocked() const volatile
        {
            return _Semaphore;
        }

    private:
        volatile int _Semaphore;
    };

#ifdef _DEBUG

    struct PSyncDebug
        : public PSyncMultiThread
    {
        void Lock()
        {
            assert(!IsLocked());
            PSyncMultiThread::Lock();
        }
    };

#else

    typedef PSyncNone PSyncDebug;

#endif
};

#endif // CRYINCLUDE_CRYCOMMON_SYNCHRONIZATION_H
