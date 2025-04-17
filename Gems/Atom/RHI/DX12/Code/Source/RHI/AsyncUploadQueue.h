/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/CommandQueue.h>

#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/DeviceStreamingImagePool.h>
#include <AzCore/std/containers/span.h>

namespace AZ
{
    namespace DX12
    {
        class Device;

        
        //! This class implements a dedicate upload queue for uploading data to device resources in its own thread.
        //! It's using idea of ring buffer for staging memory.
        //! It supports both image and buffer data uploading.
        class AsyncUploadQueue  final
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

            void Init(RHI::Device& device, const Descriptor& descriptor);
            void Shutdown();

            // Queue copy commands to upload buffer resource
            // @return queue id which can be use to check whether upload finished or wait for upload finish
            uint64_t QueueUpload(const RHI::DeviceBufferStreamRequest& request);

            // Queue copy commands to upload image subresources.
            // @param residentMip is the resident mip level the expand request starts from. 
            // @return queue id which can be use to check whether upload finished or wait for upload finish
            uint64_t QueueUpload(const RHI::DeviceStreamingImageExpandRequest& request, uint32_t residentMip);
            
            // Queue tile mapping to map tiles from allocate heap for reserved resource. This is usually required before upload data to 
            // reserved resource in this copy queue
            void QueueTileMapping(const CommandList::TileMapRequest& request);

            // Queue a wait command
            void QueueWaitFence(const Fence& fence, uint64_t fenceValue);

            bool IsUploadFinished(uint64_t fenceValue);
            void WaitForUpload(uint64_t fenceValue);

        private:
            // Process all pending callbacks which have same or smaller fenceValue
            void ProcessCallbacks(uint64_t fenceValue);

            RHI::Ptr<CommandQueue> m_copyQueue;

            struct FramePacket
            {
                RHI::Ptr<ID3D12Resource> m_stagingResource;
                RHI::Ptr<ID3D12CommandAllocator> m_commandAllocator;
                RHI::Ptr<ID3D12GraphicsCommandList> m_commandList;

                Fence m_fence;

                // Using persistent mapping for the staging resource so the Map function only need to be called called once.
                // (Advanced Usage Mode in ID3D12Resource::Map api document)
                uint8_t* m_stagingResourceData = nullptr; 
                uint32_t m_dataOffset = 0;
            };
            
            // Begin the frame packet which m_frameIndex point to and get ready to start recording copy command by using this frame packet 
            FramePacket* BeginFramePacket();
            void EndFramePacket(ID3D12CommandQueue* commandQueue);
            bool m_recordingFrame = false;

            AZStd::vector<FramePacket> m_framePackets; 
            size_t m_frameIndex = 0;

            FenceEvent m_fenceEvent{ "Wait for Frame" };
            Descriptor m_descriptor;

            // Fence for external upload request
            Fence m_uploadFence;
            FenceEvent m_uploadFenceEvent{ "Wait For Upload" };

            // pending upload callbacks and their corresponding fence values
            AZStd::queue<AZStd::pair<AZStd::function<void()>, uint64_t>> m_callbacks;
            AZStd::mutex m_callbackMutex;
        };
    }
}
