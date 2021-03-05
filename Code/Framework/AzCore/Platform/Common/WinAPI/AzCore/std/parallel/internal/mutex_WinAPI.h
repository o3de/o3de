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
#pragma once

/**
 * This file is to be included from the mutex.h only. It should NOT be included by the user.
 */

namespace AZStd
{
#define AZ_STD_MUTEX_CAST(m) reinterpret_cast<LPCRITICAL_SECTION>(&m)
    //////////////////////////////////////////////////////////////////////////
    // mutex
    inline mutex::mutex()
    {
        InitializeCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }
    inline mutex::mutex(const char* name)
    {
        (void)name;
        InitializeCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }

    inline mutex::~mutex()
    {
        DeleteCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }

    AZ_FORCE_INLINE void
    mutex::lock()
    {
        EnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }
    AZ_FORCE_INLINE bool
    mutex::try_lock()
    {
        return TryEnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex)) != 0;
    }
    AZ_FORCE_INLINE void
    mutex::unlock()
    {
        LeaveCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }
    AZ_FORCE_INLINE mutex::native_handle_type
    mutex::native_handle()
    {
        return AZ_STD_MUTEX_CAST(m_mutex);
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // recursive_mutex
    inline recursive_mutex::recursive_mutex()
    {
        InitializeCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }

    inline recursive_mutex::recursive_mutex(const char* name)
    {
        (void)name;
        InitializeCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }

    inline recursive_mutex::~recursive_mutex()
    {
        DeleteCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }

    AZ_FORCE_INLINE void
    recursive_mutex::lock()
    {
        EnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }
    AZ_FORCE_INLINE bool
    recursive_mutex::try_lock()
    {
        return TryEnterCriticalSection(AZ_STD_MUTEX_CAST(m_mutex)) != 0;
    }
    AZ_FORCE_INLINE void
    recursive_mutex::unlock()
    {
        LeaveCriticalSection(AZ_STD_MUTEX_CAST(m_mutex));
    }
    AZ_FORCE_INLINE recursive_mutex::native_handle_type
    recursive_mutex::native_handle()
    {
        return AZ_STD_MUTEX_CAST(m_mutex);
    }
    //////////////////////////////////////////////////////////////////////////
#undef AZ_STD_MUTEX_CAST
}

