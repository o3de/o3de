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

#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <mach/mach.h>

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
    using native_semaphore_data_type = semaphore_t;
    using native_semaphore_handle_type = semaphore_t*;

    // Thread
    using native_thread_id_type = pthread_t;
    static constexpr pthread_t native_thread_invalid_id = 0;
    using native_thread_data_type = pthread_t;
    using native_thread_handle_type = pthread_t;
}
