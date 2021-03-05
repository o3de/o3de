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
