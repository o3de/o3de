/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/base.h>
#include <AzCore/PlatformDef.h>
#include <AzCore/std/parallel/config_Platform.h>

/**
 * \page Parallel Parallel Processing
 *
 * Parallel processing is supported as much as possible on
 * each platform. It has lot's of limitations, please refer to each primitive
 * for more information. We are conforming with \ref C++14 but of course have
 * have limited support.
 *                                    
 * \li \ref AZStd::mutex (a.k.a critical_section)                   
 * \li \ref AZStd::recursive_mutex (a.k.a critical_section)         
 * \li \ref AZStd::semaphore                                        
 * \li \ref AZStd::thread                                           
 * \li \ref AZStd::condition_variable                               
 * \li \ref AZStd::lock
 * \li \ref ParallelContainers
 *
 */


/**
 * Native synchronization types.
 */
namespace AZStd
{
    /**
     * Thread id used only as internal type, use thread::id.
     * \note we can't use class id forward in the thread class like the standard implements
     * because some compilers fail to compile it. This this works fine and it's 100% complaint.
     */
    struct thread_id
    {
        thread_id()
            : m_id(native_thread_invalid_id) {}
        thread_id(native_thread_id_type threadId)
            : m_id(threadId) {}
        native_thread_id_type m_id;
    };

    inline bool operator==(AZStd::thread_id x, AZStd::thread_id y)     { return x.m_id == y.m_id; }
    inline bool operator!=(AZStd::thread_id x, AZStd::thread_id y)     { return x.m_id != y.m_id; }
    inline bool operator<(AZStd::thread_id  x, AZStd::thread_id y)     { return x.m_id < y.m_id; }
    inline bool operator<=(AZStd::thread_id x, AZStd::thread_id y)     { return x.m_id <= y.m_id; }
    inline bool operator>(AZStd::thread_id  x, AZStd::thread_id y)     { return x.m_id > y.m_id; }
    inline bool operator>=(AZStd::thread_id x, AZStd::thread_id y)     { return x.m_id >= y.m_id; }
}
