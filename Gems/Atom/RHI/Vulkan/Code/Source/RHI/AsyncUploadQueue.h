/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceFence.h>
#include <Atom/RHI/DeviceStreamingImagePool.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/containers/unordered_map.h>
#include <Atom/RHI/AsyncWorkQueue.h>
#include <RHI/CommandQueue.h>
#include <RHI/Device.h>
#include <RHI/Queue.h>
#include <RHI/Semaphore.h>

namespace AZ
{
    namespace Vulkan
    {
        class Buffer;
        class CommandList;
        class Queue;
        class Fence;
        struct QueueId;

        /**
         * This class implements a dedicate upload queue for uploading data to device resources in its own thread.
         */
        class AsyncUploadQueue final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AZ_CLASS_ALLOCATOR(AsyncUploadQueue, AZ::SystemAllocator);

            AZ_DISABLE_COPY_MOVE(AsyncUploadQueue);

            struct Descriptor
            {
                Device* m_device = nullptr;
                size_t m_stagingSizeInBytes = RHI::DefaultValues::Memory::AsyncQueueStagingBufferSizeInBytes;
                uint32_t m_frameCount = RHI::Limits::Device::FrameCountMax;

                Descriptor() = default;
                Descriptor(size_t stagingSizeInBytes);
            };

            AsyncUploadQueue() = default;
            ~AsyncUploadQueue() = default;

            RHI::ResultCode Init(const Descriptor& descriptor);
            void Shutdown();

            RHI::AsyncWorkHandle QueueUpload(const RHI::DeviceBufferStreamRequest& request);
            RHI::AsyncWorkHandle QueueUpload(const RHI::DeviceStreamingImageExpandRequest& request, uint32_t residentMip);

            void WaitForUpload(const RHI::AsyncWorkHandle& workHandle);

            // queue sparse bindings
            void QueueBindSparse(const VkBindSparseInfo& bindSparseInfo);

        private:
            RHI::Ptr<CommandQueue> m_queue;
            RHI::Ptr<CommandList> m_commandList;

            struct FramePacket
            {
                RHI::Ptr<Buffer> m_stagingBuffer;
                RHI::Ptr<Fence> m_fence;

                uint32_t m_dataOffset = 0;
            };

            RHI::ResultCode BuildFramePackets();

            FramePacket* BeginFramePacket(Queue* queue);
            void EndFramePacket(Queue* queue, Semaphore* semaphoreToSignal = nullptr);


            void EmmitPrologueMemoryBarrier(const Buffer& buffer, size_t offset, size_t size);
            void EmmitPrologueMemoryBarrier(const RHI::DeviceStreamingImageExpandRequest& request, uint32_t residentMip);

            void EmmitEpilogueMemoryBarrier(
                CommandList& commandList,
                const Buffer& buffer,
                size_t offset,
                size_t size);

            void EmmitEpilogueMemoryBarrier(
                CommandList& commandList,
                const RHI::DeviceStreamingImageExpandRequest& request,
                uint32_t residentMip);

            // Handles the end of the upload. This includes emitting the epilogue barriers and doing any
            // necessary cross queue synchronization and ownership transfer (if needed).
            template<typename ...Args>
            void ProcessEndOfUpload(
                Queue* queue,
                VkPipelineStageFlags waitStage,
                const AZStd::vector<Fence*> fencesToSignal,
                Args&& ...args);
            
            RHI::AsyncWorkHandle CreateAsyncWork(RHI::Ptr<Fence> fence, RHI::DeviceFence::SignalCallback callback = nullptr);
            void ProcessCallback(const RHI::AsyncWorkHandle& handle);

            Descriptor m_descriptor;
            AZStd::vector<FramePacket> m_framePackets;
            uint32_t m_frameIndex = 0;
            bool m_recordingFrame = false;
            // Async queue used for waiting for an upload event to complete.
            RHI::AsyncWorkQueue m_asyncWaitQueue;

            AZStd::mutex m_callbackListMutex;
            AZStd::unordered_map<RHI::AsyncWorkHandle, RHI::CompleteCallback> m_callbackList;
        };

        template<typename ...Args>
        void AsyncUploadQueue::ProcessEndOfUpload(
            Queue* queue,
            [[maybe_unused]] VkPipelineStageFlags waitStage,
            const AZStd::vector<Fence*> fencesToSignal,
            Args&& ...args)
        {
            BeginFramePacket(queue);

            EmmitEpilogueMemoryBarrier(
                *m_commandList,
                AZStd::forward<Args>(args)...);

            EndFramePacket(queue);
            AZStd::for_each(fencesToSignal.begin(), fencesToSignal.end(), [&](Fence* fence)
            {
                queue->GetDescriptor().m_commandQueue->Signal(*fence);
            });
        }
    }
}
