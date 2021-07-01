/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
