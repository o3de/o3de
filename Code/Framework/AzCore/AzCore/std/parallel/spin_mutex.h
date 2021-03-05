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
#ifndef AZSTD_PARALLEL_SPIN_MUTEX_H
#define AZSTD_PARALLEL_SPIN_MUTEX_H 1

#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/exponential_backoff.h>

namespace AZStd
{
    /**
     * A simple mutex implemented with a spin lock, please don't use this instead of a regular mutex with the
     * assumption it will be faster. Spin locks can introduce a whole new set of performance issues, this is provided
     * only for use in special cases.
     */
    class spin_mutex
    {
    public:
        spin_mutex(bool isLocked = false)
        {
            m_flag.store(isLocked, memory_order_release);
        }

        void lock()
        {
            bool expected = false;
            if (!m_flag.compare_exchange_weak(expected, true, memory_order_acq_rel, memory_order_acquire))
            {
                AZ_PROFILE_FUNCTION_STALL(AZ::Debug::ProfileCategory::AzCore);

                exponential_backoff backoff;
                for (;; )
                {
                    expected = false;
                    if (m_flag.compare_exchange_weak(expected, true, memory_order_acq_rel, memory_order_acquire))
                    {
                        break;
                    }
                    backoff.wait();
                }
            }
        }

        bool try_lock()
        {
            bool expected = false;
            return m_flag.compare_exchange_weak(expected, true, memory_order_acq_rel, memory_order_acquire);
        }

        void unlock()
        {
            m_flag.store(false, memory_order_release);
        }

    private:
        spin_mutex(const spin_mutex&);
        spin_mutex& operator=(const spin_mutex&);

        atomic<bool> m_flag;
    };
}

#endif
#pragma once
