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

#include "StreamerContext_Default.h"

namespace AZ::Platform
{
    void StreamerContextThreadSync::Suspend()
    {
        auto predicate = [this]() -> bool
        {
            // Don't go to sleep if there's a wake up call queued
            return m_threadWakeUpQueued.load(AZStd::memory_order_acquire);
        };

        AZStd::unique_lock<AZStd::mutex> lock(m_threadSleepLock);
        m_threadSleepCondition.wait(lock, predicate);
        m_threadWakeUpQueued.store(false, AZStd::memory_order_release);
    }

    void StreamerContextThreadSync::Resume()
    {
        AZStd::unique_lock<AZStd::mutex> lock(m_threadSleepLock);
        m_threadWakeUpQueued.store(true, AZStd::memory_order_release);
        m_threadSleepCondition.notify_one();
    }

} // namespace AZ::Platform
