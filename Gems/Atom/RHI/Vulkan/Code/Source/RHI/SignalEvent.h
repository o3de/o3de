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

#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    namespace Vulkan
    {
        class SignalEvent
        {
        public:
            SignalEvent() = default;
            ~SignalEvent() = default;

            void Signal();
            void SetValue(bool ready);
            void Wait() const;

        private:
            // Condition variable to handle the signal of the event
            mutable AZStd::condition_variable m_eventSignal;
            mutable AZStd::mutex m_eventMutex;
            bool m_ready = false;
        };
    }
}