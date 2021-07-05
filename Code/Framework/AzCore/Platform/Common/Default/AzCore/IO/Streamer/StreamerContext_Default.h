/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/condition_variable.h>

namespace AZ::Platform
{
    class StreamerContextThreadSync
    {
    public:
        void Suspend();
        void Resume();

    private:
        AZStd::mutex m_threadSleepLock;
        AZStd::atomic<bool> m_threadWakeUpQueued{ false }; //!< Whether or not there's a wake up call queued.
        AZStd::condition_variable m_threadSleepCondition;
    };

} // namespace AZ::Platform
