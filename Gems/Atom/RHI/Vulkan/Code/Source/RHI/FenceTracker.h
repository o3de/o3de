/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "SemaphoreTracker.h"
#include <Atom/RHI/FenceTracker.h>

namespace AZ::Vulkan
{
    class FenceTracker : public RHI::FenceTracker
    {
    public:
        FenceTracker(AZStd::shared_ptr<SemaphoreTrackerHandle> semaphoreTracker)
            : m_semaphoreTracker(semaphoreTracker)
        {
        }
        void SignalUserSemaphore() override
        {
            m_semaphoreTracker->SignalSemaphores(1);
        }

        const AZStd::shared_ptr<SemaphoreTrackerHandle>& GetSemaphoreTracker()
        {
            return m_semaphoreTracker;
        }

    private:
        AZStd::shared_ptr<SemaphoreTrackerHandle> m_semaphoreTracker;
    };
} // namespace AZ::Vulkan
