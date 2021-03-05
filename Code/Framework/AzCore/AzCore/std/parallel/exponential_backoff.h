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
