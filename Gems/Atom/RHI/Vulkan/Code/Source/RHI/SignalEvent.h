/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/bitset.h>
#include <AzCore/std/parallel/conditional_variable.h>
#include <AzCore/std/parallel/mutex.h>

namespace AZ
{
    namespace Vulkan
    {
        class SignalEvent
        {
        public:
            static constexpr const int MaxSignalEvents = 64;
            using BitSet = AZStd::bitset<MaxSignalEvents>;
            SignalEvent() = default;
            ~SignalEvent() = default;

            void Signal(int bit);
            void Wait(BitSet dependentBits) const;

        private:
            // Condition variable to handle the signal of the event
            mutable AZStd::condition_variable m_eventSignal;
            mutable AZStd::mutex m_eventMutex;
            BitSet m_readyBits = 0;
        };
    }
}
