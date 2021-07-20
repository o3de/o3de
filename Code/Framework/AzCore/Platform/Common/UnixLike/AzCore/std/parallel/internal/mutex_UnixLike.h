/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
    //////////////////////////////////////////////////////////////////////////
    // mutex
    inline mutex::mutex()
    {
        pthread_mutex_init(&m_mutex, NULL);
    }
    inline mutex::mutex(const char* name)
    {
        (void)name;
        pthread_mutex_init(&m_mutex, NULL);
    }

    inline mutex::~mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    AZ_FORCE_INLINE void
    mutex::lock()
    {
        pthread_mutex_lock(&m_mutex);
    }
    AZ_FORCE_INLINE bool
    mutex::try_lock()
    {
        return pthread_mutex_trylock(&m_mutex) == 0;
    }
    AZ_FORCE_INLINE void
    mutex::unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }
    AZ_FORCE_INLINE mutex::native_handle_type
    mutex::native_handle()
    {
        return &m_mutex;
    }
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // recursive_mutex
    inline recursive_mutex::recursive_mutex()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    inline recursive_mutex::recursive_mutex(const char* name)
    {
        (void)name;
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&m_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    inline recursive_mutex::~recursive_mutex()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    AZ_FORCE_INLINE void
    recursive_mutex::lock()
    {
        pthread_mutex_lock(&m_mutex);
    }
    AZ_FORCE_INLINE bool
    recursive_mutex::try_lock()
    {
        return pthread_mutex_trylock(&m_mutex) == 0;
    }
    AZ_FORCE_INLINE void
    recursive_mutex::unlock()
    {
        pthread_mutex_unlock(&m_mutex);
    }
    AZ_FORCE_INLINE recursive_mutex::native_handle_type
    recursive_mutex::native_handle()
    {
        return &m_mutex;
    }
    //////////////////////////////////////////////////////////////////////////
}
