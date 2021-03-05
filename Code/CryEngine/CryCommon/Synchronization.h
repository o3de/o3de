/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

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
