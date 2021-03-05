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

#include "CrySimpleMutex.hpp"

#include <Core/StdTypes.hpp>
#include <Core/Error.hpp>
#include <Core/STLHelper.hpp>


CCrySimpleMutex::CCrySimpleMutex()
{
#if defined(AZ_PLATFORM_WINDOWS)
    InitializeCriticalSectionAndSpinCount(&cs, 10000);
#else
    pthread_mutex_init(&m_Mutex, nullptr);
#endif
}

CCrySimpleMutex::~CCrySimpleMutex()
{
#if defined(AZ_PLATFORM_WINDOWS)
    DeleteCriticalSection(&cs);
#else
    pthread_mutex_destroy(&m_Mutex);
#endif
}


void CCrySimpleMutex::Lock()
{
#if defined(AZ_PLATFORM_WINDOWS)
    EnterCriticalSection(&cs);
#else
    pthread_mutex_lock(&m_Mutex);
#endif
}

void CCrySimpleMutex::Unlock()
{
#if defined(AZ_PLATFORM_WINDOWS)
    LeaveCriticalSection(&cs);
#else
    pthread_mutex_unlock(&m_Mutex);
#endif
}

