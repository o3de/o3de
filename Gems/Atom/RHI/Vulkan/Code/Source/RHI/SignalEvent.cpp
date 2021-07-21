/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "Atom_RHI_Vulkan_precompiled.h"
#include <RHI/SignalEvent.h>

namespace AZ
{
    namespace Vulkan
    {
        void SignalEvent::SetValue(bool ready)
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_eventMutex);
            m_ready = ready;
        }

        void SignalEvent::Signal()
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_eventMutex);
            m_ready = true;
            m_eventSignal.notify_all();
        }

        void SignalEvent::Wait() const
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_eventMutex);
            m_eventSignal.wait(lock, [&]() { return m_ready; });
        }
    }
}
