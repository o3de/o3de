/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
