/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

