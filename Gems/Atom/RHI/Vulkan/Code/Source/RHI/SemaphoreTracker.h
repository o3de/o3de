/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/condition_variable.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    namespace Vulkan
    {
        // These classes are used for synchronizing recording commands on the queue when timeline semaphores are used.
        // The signal operation of a timeline semaphore can be submitted before the respective wait operation is submitted.
        // Timeline semaphore however cannot be used to synchronize vkQueuePresentKHR in the swapchain.
        // For this we need a binary semaphore, for which all dependent signal operations must have been submitted:
        // https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkQueuePresentKHR.html#VUID-vkQueuePresentKHR-pWaitSemaphores-03268
        // These classes are used to track how many semaphores are in the framegraph and how many have already been signalled
        // We create one tracker for each swapchain in the framegraph, which is used to wait on all dependent semaphores to be signalled

        // Semaphore tracker for a single swapchain
        // Instances of this class should always be created through a SemaphoreTrackerCollection
        class SemaphoreTracker
        {
        public:
            SemaphoreTracker(int initialNumberOfSemaphores);
            void AddSemaphores(int numSemaphores);
            void SignalSemaphores(int numSemaphores);
            void WaitForSignalAllSemaphores();

        private:
            int m_semaphoreCounter = 0;
            int m_signalledSemaphoreCounter = 0;
            AZStd::mutex m_lock;
            AZStd::condition_variable m_waitCondition;
        };

        class SemaphoreTrackerHandle;
        // Semaphore tracker collection for all swapchains in the framgraph
        // There is one SemaphoreTracker per swapchain
        class SemaphoreTrackerCollection : public AZStd::intrusive_base
        {
            friend class SemaphoreTrackerHandle;

        public:
            void AddSemaphores(int numSemaphores);

            AZStd::shared_ptr<SemaphoreTrackerHandle> CreateHandle();
            const AZStd::shared_ptr<SemaphoreTracker>& GetCurrentTracker();

        private:
            void SignalSemaphores(int firstSemaphoreToTrigger, int numSemphores);

            AZStd::vector<AZStd::shared_ptr<SemaphoreTracker>> m_trackers;
            int m_semaphoreCount = 0;
        };

        // Handle which can signal all SemaphoreTrackers, for all swapchains a scope is used for
        // The SignalSemaphore function must be called for all semaphore signal operations submitted to a command queue,
        // and all semaphore signal operations triggered from the CPU
        class SemaphoreTrackerHandle
        {
        public:
            SemaphoreTrackerHandle(AZStd::intrusive_ptr<SemaphoreTrackerCollection> tracker, int firstSemaphoreToTrigger);
            void SignalSemaphores(int numSemaphores);

        private:
            int m_firstSemaphoreToTrigger = 0;
            AZStd::intrusive_ptr<SemaphoreTrackerCollection> m_tracker;
        };

    } // namespace Vulkan
} // namespace AZ
