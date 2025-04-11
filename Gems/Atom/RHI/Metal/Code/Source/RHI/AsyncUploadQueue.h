/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/AsyncWorkQueue.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceObject.h>
#include <Atom/RHI/DeviceStreamingImagePool.h>
#include <AzCore/std/containers/span.h>
#include <RHI/CommandQueue.h>
#include <RHI/Buffer.h>
#include <RHI/Image.h>
#include <Metal/Metal.h>

namespace AZ
{
    namespace Metal
    {
        class Device;

        //! This class implements a dedicated upload queue for uploading data to device resources in its own thread.
        //! It's using idea of ring buffer for staging memory.
        //! It supports buffer data uploading.
        class AsyncUploadQueue final
            : public RHI::DeviceObject
        {
            using Base = RHI::DeviceObject;

        public:
            AsyncUploadQueue() = default;

            AZ_DISABLE_COPY_MOVE(AsyncUploadQueue);

            struct Descriptor
            {
                size_t m_stagingSizeInBytes = RHI::DefaultValues::Memory::AsyncQueueStagingBufferSizeInBytes;
                size_t m_frameCount = RHI::Limits::Device::FrameCountMax;

                Descriptor() = default;
                Descriptor(size_t stagingSizeInBytes);
            };

            void Init(Device& device, const Descriptor& descriptor);
            void Shutdown();

            //! Queue copy commands to upload buffer resource
            //! @return queue id which can be use to check whether upload finished or wait for upload finish
            uint64_t QueueUpload(const RHI::DeviceBufferStreamRequest& request);

            //! Queue copy commands to upload image subresources.
            //! @param residentMip is the resident mip level the expand request starts from.
            //! @return queue id which can be use to check whether upload finished or wait for upload finish
            RHI::AsyncWorkHandle QueueUpload(const RHI::DeviceStreamingImageExpandRequest& request, uint32_t residentMip);

            bool IsUploadFinished(uint64_t fenceValue);
            void WaitForUpload(const RHI::AsyncWorkHandle& workHandle);

        private:
            struct FramePacket;
            RHI::AsyncWorkHandle CreateAsyncWork(Fence& fence, RHI::DeviceFence::SignalCallback callback = nullptr);
            void ProcessCallback(const RHI::AsyncWorkHandle& handle);
            void CopyBufferToImage(FramePacket* framePacket,
                                   Image* destImage,
                                   uint32_t stagingRowPitch,
                                   uint32_t stagingSlicePitch,
                                   uint16_t mipSlice,
                                   uint16_t arraySlice,
                                   RHI::Size sourceSize,
                                   RHI::Origin sourceOrigin);

            struct FramePacket
            {
                id<MTLCommandBuffer> m_mtlCommandBuffer;
                id<MTLBuffer> m_stagingResource;
                Fence m_fence;

                // Using persistent mapping for the staging resource so the Map function only need to be called called once.
                uint8_t* m_stagingResourceData = nullptr;
                uint32_t m_dataOffset = 0;
            };

            RHI::Ptr<CommandQueue> m_copyQueue;
            // Begin the frame packet which m_frameIndex point to and get ready to start recording copy command by using this frame packet
            FramePacket* BeginFramePacket(CommandQueue* commandQueue);
            void EndFramePacket(CommandQueue* commandQueue);
            bool m_recordingFrame = false;

            AZStd::vector<FramePacket> m_framePackets;
            size_t m_frameIndex = 0;

            Descriptor m_descriptor;

            // Fence for external upload request
            Fence m_uploadFence;

            RHI::Ptr<Device> m_device;

            //Command Buffer associated with the async copy queue
            CommandQueueCommandBuffer m_commandBuffer;

            // Async queue used for waiting for an upload event to complete.
            RHI::AsyncWorkQueue m_asyncWaitQueue;
            AZStd::mutex m_callbackListMutex;
            AZStd::unordered_map<RHI::AsyncWorkHandle, RHI::CompleteCallback> m_callbackList;
        };
    }
}
