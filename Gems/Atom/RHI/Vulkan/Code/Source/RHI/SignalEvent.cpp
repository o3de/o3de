/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/SignalEvent.h>

namespace AZ
{
    namespace Vulkan
    {
        void SignalEvent::Signal(int bit)
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_eventMutex);
            if (m_readyBits.test(bit))
            {
                return;
            }
            m_readyBits.set(bit);
            m_eventSignal.notify_all();
        }

        void SignalEvent::Wait(SignalEvent::BitSet dependentBits) const
        {
            if (!dependentBits.any())
            {
                return;
            }
            AZStd::unique_lock<AZStd::mutex> lock(m_eventMutex);
            m_eventSignal.wait(
                lock,
                [&]()
                {
                    return (m_readyBits & dependentBits) == dependentBits;
                });
        }
    }
}
