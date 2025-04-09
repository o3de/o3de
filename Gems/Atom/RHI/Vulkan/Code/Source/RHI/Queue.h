/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <RHI/CommandList.h>
#include <RHI/Semaphore.h>

namespace AZ
{
    namespace Vulkan
    {
        class CommandQueue;
        class Device;
        class Semaphore;
        class SwapChain;
        class Fence;

        struct QueueId
        {
            uint32_t m_familyIndex = 0;
            uint32_t m_queueIndex = 0;

            bool operator==(const QueueId& other) const { return ::memcmp(this, &other, sizeof(other)) == 0; }
            bool operator!=(const QueueId& other) const { return !(*this == other); }
        };

        class Queue final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;
            friend class CommandQueue;

        public:
            AZ_CLASS_ALLOCATOR(Queue, AZ::SystemAllocator);
            AZ_RTTI(Queue, "C3420514-4BB2-4416-A6A1-FEFFF041BCB4", Base);

            struct Descriptor
            {
                uint32_t m_familyIndex = 0;
                uint32_t m_queueIndex = 0;
                CommandQueue* m_commandQueue = nullptr;
            };

            RHI::ResultCode Init(RHI::Device& deviceBase, const Descriptor& descriptor);
            ~Queue() = default;

            /// Get Vulkan's native VkQueue.
            VkQueue GetNativeQueue() const;

            /// Submit commandBuffers to this queue.
            RHI::ResultCode SubmitCommandBuffers(
                const AZStd::vector<RHI::Ptr<CommandList>>& commandBuffers,
                const AZStd::vector<Semaphore::WaitSemaphore>& waitSemaphoresInfo,
                const AZStd::vector<RHI::Ptr<Semaphore>>& semaphoresToSignal,
                const AZStd::vector<RHI::Ptr<AZ::Vulkan::Fence>>& fencesToWaitFor,
                AZ::Vulkan::Fence* fenceToSignal);

            /// Waits (blocks) until all fences are signaled in the fence-list.
            void WaitForIdle();

            const Descriptor& GetDescriptor() const;
            QueueId GetId() const;

            void BeginDebugLabel(const char* label, const AZ::Color color = Debug::DefaultLabelColor);
            void EndDebugLabel();

        private:
            Queue() = default;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            Descriptor m_descriptor;
            VkQueue m_nativeQueue = VK_NULL_HANDLE;
        };
    }
}
