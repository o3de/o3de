/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef AZSTD_PARALLEL_EXPONENTIAL_BACKOFF_H
#define AZSTD_PARALLEL_EXPONENTIAL_BACKOFF_H 1

#include <AzCore/std/parallel/thread.h>

namespace AZStd
{
    class exponential_backoff
    {
    public:
        exponential_backoff()
            : m_count(1) { }

        void wait()
        {
            if (m_count <= MAX_PAUSE_LOOPS)
            {
                AZStd::this_thread::pause(m_count);
                m_count <<= 1;
            }
            else
            {
                AZStd::this_thread::yield();
            }
        }

        void reset()
        {
            m_count = 1;
        }

    private:
        static const int MAX_PAUSE_LOOPS = 32;
        int m_count;
    };
}

#endif
#pragma once
