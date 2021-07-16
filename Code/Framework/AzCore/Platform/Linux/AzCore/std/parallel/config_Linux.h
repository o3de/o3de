/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>

namespace AZStd
{
    // Mutex
    using native_mutex_data_type = pthread_mutex_t;
    using native_mutex_handle_type = pthread_mutex_t*;
    using native_recursive_mutex_data_type = pthread_mutex_t;
    using native_recursive_mutex_handle_type = pthread_mutex_t*;

    // Condition variable
    using native_cond_var_data_type = pthread_cond_t;
    using native_cond_var_handle_type = pthread_cond_t*;

    // Semaphore
    using native_semaphore_data_type = sem_t;
    using native_semaphore_handle_type = sem_t*;

    // Thread
    using native_thread_id_type = pthread_t;
    static constexpr pthread_t native_thread_invalid_id = 0;
    using native_thread_data_type = pthread_t;
    using native_thread_handle_type = pthread_t;
}
