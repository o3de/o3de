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

#ifndef __CRYSIMPLEMUTEX__
#define __CRYSIMPLEMUTEX__

#include <Core/Common.h>

#if !defined(AZ_PLATFORM_WINDOWS)
#include "pthread.h"
#endif

class CCrySimpleMutex
{
#if defined(AZ_PLATFORM_WINDOWS)
    CRITICAL_SECTION cs;
#else
    // Use posix thread support
    pthread_mutex_t m_Mutex;
#endif
public:
    CCrySimpleMutex();
    ~CCrySimpleMutex();

    void    Lock();
    void    Unlock();
};

class CCrySimpleMutexAutoLock
{
    CCrySimpleMutex& m_rMutex;
public:
    CCrySimpleMutexAutoLock(CCrySimpleMutex& rMutex)
        : m_rMutex(rMutex)
    {
        rMutex.Lock();
    }
    ~CCrySimpleMutexAutoLock()
    {
        m_rMutex.Unlock();
    }
};
#endif
