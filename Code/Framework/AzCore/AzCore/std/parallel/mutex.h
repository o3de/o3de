/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/parallel/config.h>

namespace AZStd
{
    /**
    * Mutex facilitates protection against data races and allows
    * thread-safe synchronization of data between threads.
    * \ref C++0x (30.4.1)
    */
    class mutex
    {
    public:
        typedef native_mutex_handle_type native_handle_type;

        mutex();
        ~mutex();

        void lock();
        bool try_lock();
        void unlock();

        native_handle_type native_handle();

        // Extensions
        mutex(const char* name);
    private:
        mutex(const mutex&) {}
        mutex& operator=(const mutex&) { return *this; }

        native_mutex_data_type  m_mutex;
    };

    /**
    * Recursive mutex provides a recursive mutex with exclusive ownership semantics.
    * \ref C++0x (30.4.1.2)
    */
    class recursive_mutex
    {
    public:
        typedef native_recursive_mutex_handle_type native_handle_type;

        recursive_mutex();
        ~recursive_mutex();

        void lock();
        bool try_lock();
        void unlock();

        native_handle_type native_handle();

        // Extensions
        recursive_mutex(const char* name);
    private:
        recursive_mutex(const recursive_mutex&)     {}
        recursive_mutex& operator=(const recursive_mutex&) { return *this; }

        native_recursive_mutex_data_type    m_mutex;
    };
}

#include <AzCore/std/parallel/internal/mutex_Platform.h>

