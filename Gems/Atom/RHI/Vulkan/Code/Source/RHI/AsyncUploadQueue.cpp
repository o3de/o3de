/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <Atom/RHI/BufferPool.h>
#include <Atom/RHI/StreamingImagePool.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/vector.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Buffer.h>
#include <RHI/BufferPool.h>
#include <RHI/CommandList.h>
#include <RHI/CommandQueue.h>
#include <RHI/CommandQueueContext.h>
#include <RHI/Conversion.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>
#include <RHI/Image.h>
#include <RHI/MemoryView.h>
#include <RHI/Queue.h>

namespace AZ
{
    namespace Vulkan
    {
        AsyncUploadQueue::Descriptor::Descriptor(size_t stagingSizeInBytes)
        {
            m_stagingSizeInBytes = stagingSizeInBytes;
        }

        RHI::ResultCode AsyncUploadQueue::Init(const Descriptor& descriptor)
        {
            m_descriptor = descriptor;
            Device& device = *descriptor.m_device;
            Base::Init(device);
            RHI::ResultCode result = RHI::ResultCode::Success;

            m_queue = &device.GetCommandQueueContext().GetCommandQueue(RHI::HardwareQueueClass::Copy);

            result = BuilidFramePackets();
            RETURN_RESULT_IF_UNSUCCESSFUL(result);

            m_asyncWaitQueue.Init();

            return result;
        }

        void AsyncUploadQueue::Shutdown()
        {
            m_asyncWaitQueue.ShutDown();
            m_callbackList.clear();
        }

        RHI::AsyncWorkHandle AsyncUploadQueue::QueueUpload(const RHI::BufferStreamRequest& request)
        {
            auto& device = static_cast<Device&>(GetDevice());

            const uint8_t* sourceData = reinterpret_cast<const uint8_t*>(request.m_sourceData);
            const size_t byteCount = request.m_byteCount;
            auto* buffer = static_cast<Buffer*>(request.m_buffer);
            RHI::BufferPool* bufferPool = static_cast<RHI::BufferPool*>(buffer->GetPool());

            if (byteCount == 0)
            {
                AZ_Assert(false, "Trying to upload 0 bytes to buffer");
                return RHI::AsyncWorkHandle::Null;
            }

            if (bufferPool->GetDescriptor().m_heapMemoryLevel == RHI::HeapMemoryLevel::Host)
            {
                // No need to use staging buffers since it's host memory.
                // We just map, copy and then unmap.
                RHI::BufferMapRequest mapRequest;
                mapRequest.m_buffer = request.m_buffer;
                mapRequest.m_byteCount = request.m_byteCount;
                mapRequest.m_byteOffset = request.m_byteOffset;
                RHI::BufferMapResponse mapResponse;
                bufferPool->MapBuffer(mapRequest, mapResponse);
                ::memcpy(mapResponse.m_data, request.m_sourceData, request.m_byteCount);
                bufferPool->UnmapBuffer(*request.m_buffer);
                if (request.m_fenceToSignal)
                {
                    request.m_fenceToSignal->SignalOnCpu();
                }
                return RHI::AsyncWorkHandle::Null;
            }

            RHI::Ptr<Fence> uploadFence = Fence::Create();
            uploadFence->Init(device, RHI::FenceState::Reset);
            CommandQueue::Command command = [=, &device](void* queue)
            {
                AZ_PROFILE_SCOPE(RHI, "Upload Buffer");
                size_t pendingByteOffset = 0;
                size_t pendingByteCount = byteCount;
                FramePacket* framePacket = nullptr;
                Queue* vulkanQueue = static_cast<Queue*>(queue);

                // Insert a barrier to wait for anybody using this buffer range.
                // We need to create a command list to insert the barrier.
                BeginFramePacket(vulkanQueue);
                EmmitPrologueMemoryBarrier(*buffer, 0, pendingByteCount);
                EndFramePacket(vulkanQueue);

                while (pendingByteCount > 0)
                {
                    AZ_PROFILE_SCOPE(RHI, "Upload Buffer Chunk");

                    framePacket = BeginFramePacket(vulkanQueue);
                    const size_t bytesToCopy = AZStd::min(pendingByteCount, m_descriptor.m_stagingSizeInBytes);
                    EmmitPrologueMemoryBarrier(*buffer, pendingByteOffset, bytesToCopy);

                    void* mapped = framePacket->m_stagingBuffer->GetBufferMemoryView()->Map(RHI::HostMemoryAccess::Write);
                    memcpy(mapped, sourceData + pendingByteOffset, bytesToCopy);
                    framePacket->m_stagingBuffer->GetBufferMemoryView()->Unmap(RHI::HostMemoryAccess::Write);

                    RHI::CopyBufferDescriptor copyDescriptor;
                    copyDescriptor.m_sourceBuffer = framePacket->m_stagingBuffer.get();
                    copyDescriptor.m_sourceOffset = 0;
                    copyDescriptor.m_destinationBuffer = buffer;
                    copyDescriptor.m_destinationOffset = static_cast<uint32_t>(pendingByteOffset);
                    copyDescriptor.m_size = static_cast<uint32_t>(bytesToCopy);

                    m_commandList->Submit(RHI::CopyItem(copyDescriptor));

                    pendingByteOffset += bytesToCopy;
                    pendingByteCount -= bytesToCopy;

                    EndFramePacket(vulkanQueue);
                }

                AZStd::vector<Fence*> fencesToSignal;
                if (request.m_fenceToSignal)
                {
                    fencesToSignal.push_back(static_cast<Fence*>(request.m_fenceToSignal));
                }
                fencesToSignal.push_back(uploadFence.get());

                VkPipelineStageFlags waitStage = GetResourcePipelineStateFlags(buffer->GetDescriptor().m_bindFlags) & device.GetSupportedPipelineStageFlags();
                ProcessEndOfUpload(
                    vulkanQueue,
                    waitStage,
                    fencesToSignal,
                    *buffer,
                    0,
                    byteCount);
            };

            m_queue->QueueCommand(AZStd::move(command));

            buffer->SetOwnerQueue(m_queue->GetId());
            auto waitEvent = [buffer]()
            {
                buffer->SetUploadHandle(RHI::AsyncWorkHandle::Null);
            };

            // Add the wait event so we can wait for upload if necessary.
            RHI::AsyncWorkHandle handle = CreateAsyncWork(uploadFence, waitEvent);
            buffer->SetUploadHandle(handle);
            m_asyncWaitQueue.UnlockAsyncWorkQueue();
            return handle;
        }

        // [GFX TODO][ATOM-4205] Stage/Upload 3D streaming images more efficiently.
        RHI::AsyncWorkHandle AsyncUploadQueue::QueueUpload(const RHI::StreamingImageExpandRequest& request, uint32_t residentMip)
        {
            auto* image = static_cast<Image*>(request.m_image);
            auto& device = static_cast<Device&>(GetDevice());

            const uint16_t startMip = static_cast<uint16_t>(residentMip - 1);
            const uint16_t endMip = static_cast<uint16_t>(residentMip - request.m_mipSlices.size());

            RHI::Ptr<Fence> uploadFence = Fence::Create();
            uploadFence->Init(device, RHI::FenceState::Reset);

            CommandQueue::Command command = [=, &device](void* queue)
            {
                AZ_PROFILE_SCOPE(RHI, "Upload Image");

                Queue* vulkanQueue = static_cast<Queue*>(queue);
                FramePacket* framePacket = BeginFramePacket(vulkanQueue);

                // Set pipeline barriers before copy.
                EmmitPrologueMemoryBarrier(request, residentMip);

                const static uint32_t bufferOffsetAlign = 4; // refer VkBufferImageCopy in the spec.

                // Variables for split subresource slice. 
                // If a subresource slice pitch is large than one staging size, we may split the slice by rows.
                // And using the CopyTextureRegion to only copy a section of the subresource 
                bool needSplitSlice = false;

                for (uint16_t curMip = endMip; curMip <= startMip; ++curMip)
                {
                    const size_t sliceIndex = curMip - endMip;
                    const RHI::ImageSubresourceLayout& subresourceLayout = request.m_mipSlices[sliceIndex].m_subresourceLayout;
                    uint32_t arraySlice = 0;
                    const uint32_t subresourceSlicePitch = subresourceLayout.m_bytesPerImage;

                    // Staging sizes
                    const uint32_t stagingRowPitch = RHI::AlignUp(subresourceLayout.m_bytesPerRow, bufferOffsetAlign);
                    const uint32_t stagingSlicePitch = subresourceLayout.m_rowCount * stagingRowPitch;
                    const uint32_t rowsPerSplit = static_cast<uint32_t>(m_descriptor.m_stagingSizeInBytes) / stagingRowPitch;
                    const uint32_t compressedTexelBlockSizeHeight = subresourceLayout.m_blockElementHeight;

                    // ImageHeight must be bigger than or equal to the Image's row count. Images with a RowCount that is less than the ImageHeight indicates a block compression.
                    // Images with a RowCount which is higher than the ImageHeight indicates a planar image, which is not supported for streaming images.
                    AZ_Error("StreamingImage", subresourceLayout.m_size.m_height >= subresourceLayout.m_rowCount, "AsyncUploadQueue::QueueUpload expects ImageHeight '%d' to be bigger than or equal to the image's RowCount '%d'.", subresourceLayout.m_size.m_height, subresourceLayout.m_rowCount);

                    // The final staging size for each CopyTextureRegion command
                    uint32_t stagingSize = stagingSlicePitch;

                    // Prepare for splitting this subresource if needed.
                    if (stagingSlicePitch > m_descriptor.m_stagingSizeInBytes)
                    {
                        // Calculate minimum size of one row of this subresource.
                        if (stagingRowPitch > m_descriptor.m_stagingSizeInBytes)
                        {
                            AZ_Warning("Vulkan", false, "AsyncUploadQueue staging buffer (%dK) is not big enough"
                                "for the size of one row of image's sub-resource (%dK). Please increase staging buffer size.",
                                m_descriptor.m_stagingSizeInBytes / 1024.0f, stagingRowPitch / 1024.f);
                            continue;
                        }

                        needSplitSlice = true;
                        stagingSize = rowsPerSplit * stagingRowPitch;
                        AZ_Assert(stagingSize <= m_descriptor.m_stagingSizeInBytes, "Final staging size can't be larger than staging buffer size.");
                    }

                    if (!needSplitSlice)
                    {
                        // Try to use one frame packet for all subresources if possible.
                        for (const RHI::StreamingImageSubresourceData& subresourceData : request.m_mipSlices[sliceIndex].m_subresources)
                        {
                            for (uint32_t depth = 0; depth < subresourceLayout.m_size.m_depth; depth++)
                            {
                                const uint8_t* subresourceDataStart = reinterpret_cast<const uint8_t*>(subresourceData.m_data) + (depth * subresourceSlicePitch);

                                // If the current framePacket is not big enough, switch to next one.
                                if (stagingSize > m_descriptor.m_stagingSizeInBytes - framePacket->m_dataOffset)
                                {
                                    EndFramePacket(vulkanQueue);
                                    framePacket = BeginFramePacket(vulkanQueue);
                                }

                                // Copy subresource data to staging memory.
                                {
                                    AZ_PROFILE_SCOPE(RHI, "Copy CPU image");
                                    uint8_t* stagingDataStart = reinterpret_cast<uint8_t*>(framePacket->m_stagingBuffer->GetBufferMemoryView()->Map(RHI::HostMemoryAccess::Write)) + framePacket->m_dataOffset;
                                    for (uint32_t row = 0; row < subresourceLayout.m_rowCount; ++row)
                                    {
                                        memcpy(stagingDataStart + row * stagingRowPitch, subresourceDataStart + row * subresourceLayout.m_bytesPerRow, subresourceLayout.m_bytesPerRow);
                                    }

                                    framePacket->m_stagingBuffer->GetBufferMemoryView()->Unmap(RHI::HostMemoryAccess::Write);
                                }

                                // Add copy command to copy image subresource from staging memory to image GPU resource.
                                RHI::CopyBufferToImageDescriptor copyDescriptor;
                                copyDescriptor.m_sourceBuffer = framePacket->m_stagingBuffer.get();
                                copyDescriptor.m_sourceOffset = framePacket->m_dataOffset;
                                copyDescriptor.m_sourceBytesPerRow = stagingRowPitch;
                                copyDescriptor.m_sourceBytesPerImage = stagingSlicePitch;
                                copyDescriptor.m_sourceSize = subresourceLayout.m_size;
                                copyDescriptor.m_sourceSize.m_depth = 1;
                                copyDescriptor.m_destinationImage = image;
                                copyDescriptor.m_destinationSubresource.m_mipSlice = curMip;
                                copyDescriptor.m_destinationSubresource.m_arraySlice = static_cast<uint16_t>(arraySlice);
                                copyDescriptor.m_destinationOrigin.m_left = 0;
                                copyDescriptor.m_destinationOrigin.m_top = 0;
                                copyDescriptor.m_destinationOrigin.m_front = depth;

                                m_commandList->Submit(RHI::CopyItem(copyDescriptor));

                                framePacket->m_dataOffset += stagingSlicePitch;
                            }
                            // Next slice in this array.
                            ++arraySlice;
                        }
                    }
                    else
                    {
                        // Each subresource need to be split.
                        for (const RHI::StreamingImageSubresourceData& subresourceData : request.m_mipSlices[sliceIndex].m_subresources)
                        {
                            for (uint32_t depth = 0u; depth < subresourceLayout.m_size.m_depth; depth++)
                            {
                                const uint8_t* subresourceDataStart = reinterpret_cast<const uint8_t*>(subresourceData.m_data) + (depth * subresourceSlicePitch);

                                // The copy destination is same for each subresource.
                                RHI::CopyBufferToImageDescriptor copyDescriptor;
                                copyDescriptor.m_sourceBuffer = framePacket->m_stagingBuffer.get();
                                copyDescriptor.m_sourceOffset = framePacket->m_dataOffset;
                                copyDescriptor.m_sourceBytesPerRow = stagingRowPitch;
                                copyDescriptor.m_sourceBytesPerImage = stagingSlicePitch;
                                copyDescriptor.m_sourceSize = subresourceLayout.m_size;
                                copyDescriptor.m_sourceSize.m_depth = 1;
                                copyDescriptor.m_destinationImage = image;
                                copyDescriptor.m_destinationSubresource.m_mipSlice = curMip;
                                copyDescriptor.m_destinationSubresource.m_arraySlice = static_cast<uint16_t>(arraySlice);
                                copyDescriptor.m_destinationOrigin.m_left = 0;
                                copyDescriptor.m_destinationOrigin.m_top = 0;
                                copyDescriptor.m_destinationOrigin.m_front = depth;

                                uint32_t startRow = 0;
                                uint32_t destHeight = 0;
                                while (startRow < subresourceLayout.m_rowCount)
                                {
                                    if (stagingSize > m_descriptor.m_stagingSizeInBytes - framePacket->m_dataOffset)
                                    {
                                        EndFramePacket(vulkanQueue);
                                        framePacket = BeginFramePacket(vulkanQueue);
                                        copyDescriptor.m_sourceBuffer = framePacket->m_stagingBuffer.get();
                                    }

                                    const uint32_t endRow = AZStd::min(startRow + rowsPerSplit, subresourceLayout.m_rowCount);

                                    // Calculate the blocksize for BC formatted images; the copy command works in texels.
                                    uint32_t heightToCopy = (endRow - startRow) * compressedTexelBlockSizeHeight;

                                    // Copy subresource data to staging memory.
                                    {
                                        AZ_PROFILE_SCOPE(RHI, "Copy CPU image");

                                        uint8_t* stagingDataStart = reinterpret_cast<uint8_t*>(framePacket->m_stagingBuffer->GetBufferMemoryView()->Map(RHI::HostMemoryAccess::Write));
                                        stagingDataStart += framePacket->m_dataOffset;
                                        for (uint32_t row = startRow; row < endRow; ++row)
                                        {
                                            memcpy(stagingDataStart + (row - startRow) * stagingRowPitch, subresourceDataStart + row * subresourceLayout.m_bytesPerRow, subresourceLayout.m_bytesPerRow);
                                        }
                                        framePacket->m_stagingBuffer->GetBufferMemoryView()->Unmap(RHI::HostMemoryAccess::Write);
                                    }

                                    //Clamp heightToCopy to match subresourceLayout.m_size.m_height as it is possible to go over
                                    //if subresourceLayout.m_size.m_height is not perfectly divisible by compressedTexelBlockSizeHeight
                                    if(destHeight+heightToCopy > subresourceLayout.m_size.m_height)
                                    {
                                        uint32_t HeightDiff = (destHeight + heightToCopy) - subresourceLayout.m_size.m_height;
                                        heightToCopy -= HeightDiff;
                                    }
                                    
                                    // Add copy command to copy image subresource from staging memory to image GPU resource.
                                    copyDescriptor.m_destinationOrigin.m_top = destHeight;
                                    copyDescriptor.m_sourceSize.m_height = heightToCopy;
                                    copyDescriptor.m_sourceOffset = framePacket->m_dataOffset;

                                    m_commandList->Submit(RHI::CopyItem(copyDescriptor));

                                    framePacket->m_dataOffset += stagingSize;
                                    startRow = endRow;
                                    destHeight += heightToCopy;
                                }
                            }
                            ++arraySlice;
                        }
                    }
                }

                // Set pipeline barriers after the copy.
                VkPipelineStageFlags waitStage = GetResourcePipelineStateFlags(image->GetDescriptor().m_bindFlags) & device.GetSupportedPipelineStageFlags();
                EndFramePacket(vulkanQueue);
                ProcessEndOfUpload(
                    vulkanQueue,
                    waitStage,
                    AZStd::vector<Fence*>{uploadFence.get()},
                    request,
                    residentMip);
            };

            RHI::ImageSubresourceRange range;
            range.m_mipSliceMin = startMip;
            range.m_mipSliceMax = endMip;
            range.m_arraySliceMin = 0;
            range.m_arraySliceMax = aznumeric_caster(image->GetDescriptor().m_arraySize - 1);

            image->SetOwnerQueue(m_queue->GetId(), &range);
            image->SetLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &range);
            m_queue->QueueCommand(AZStd::move(command));

            auto waitEvent = [this, request, image, range]()
            {
                RHI::AsyncWorkHandle uploadHandle = image->GetUploadHandle();
                image->SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &range);
                if (request.m_completeCallback)
                {
                    if (request.m_waitForUpload)
                    {
                        request.m_completeCallback();
                    }
                    else
                    {
                        // Add the callback so it can be processed from the main thread.                       
                        {
                            AZStd::unique_lock<AZStd::mutex> lock(m_callbackListMutex);
                            m_callbackList.insert(AZStd::make_pair(uploadHandle, [request, image]() { image->SetUploadHandle(RHI::AsyncWorkHandle::Null); request.m_completeCallback(); }));
                        }
                        // We could just add a lambda that calls the request.m_completeCallback() but that could create a crash
                        // if the image is destroyed before the callback is triggered from the TickBus. Because of this we save the callbacks in this
                        // class and when and image is destroy, we just execute any pending callback for that image.
                        AZ::TickBus::QueueFunction([this, uploadHandle]() { ProcessCallback(uploadHandle); });
                    }
                }
            };

            if (request.m_waitForUpload)
            {
                // No need to add wait event.
                uploadFence->WaitOnCpu();
                waitEvent();
            }
            else
            {
                auto uploadHandle = CreateAsyncWork(uploadFence, waitEvent);
                image->SetUploadHandle(uploadHandle);
                m_asyncWaitQueue.UnlockAsyncWorkQueue();
                return uploadHandle;
            }

            return RHI::AsyncWorkHandle::Null;
        }

        void AsyncUploadQueue::WaitForUpload(const RHI::AsyncWorkHandle& workHandle)
        {
            m_asyncWaitQueue.WaitToFinish(workHandle);
            ProcessCallback(workHandle);
        }

        RHI::ResultCode AsyncUploadQueue::BuilidFramePackets()
        {
            auto& device = static_cast<Device&>(GetDevice());
            m_framePackets.resize(m_descriptor.m_frameCount);
            RHI::ResultCode result = RHI::ResultCode::Success;

            const RHI::BufferDescriptor bufferDescriptor(RHI::BufferBindFlags::CopyRead, m_descriptor.m_stagingSizeInBytes);
            for (FramePacket& framePacket : m_framePackets)
            {
                framePacket.m_stagingBuffer = device.AcquireStagingBuffer(m_descriptor.m_stagingSizeInBytes);
                AZ_Assert(framePacket.m_stagingBuffer, "Failed to acquire staging buffer");
                framePacket.m_fence = Fence::Create();
                result = framePacket.m_fence->Init(device, RHI::FenceState::Signaled);
                RETURN_RESULT_IF_UNSUCCESSFUL(result);
            }

            return result;
        }

        AsyncUploadQueue::FramePacket* AsyncUploadQueue::BeginFramePacket(Queue* queue)
        {
            AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: BeginFramePacket");
            AZ_Assert(!m_recordingFrame, "The previous frame packet isn't ended.");
            auto& device = static_cast<Device&>(GetDevice());

            FramePacket& framePacket = m_framePackets[m_frameIndex];
            framePacket.m_fence->WaitOnCpu();
            framePacket.m_fence->Reset();
            framePacket.m_dataOffset = 0;

            queue->BeginDebugLabel(AZStd::string::format("AsyncUploadQueue Packet %d", static_cast<int>(m_frameIndex)).c_str());
            m_commandList = device.AcquireCommandList(RHI::HardwareQueueClass::Copy);
            m_commandList->BeginCommandBuffer();

            m_recordingFrame = true;

            return &framePacket;
        }

        void AsyncUploadQueue::EndFramePacket(Queue* queue, Semaphore* semaphoreToSignal /*=nullptr*/)
        {
            AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: EndFramePacket");
            AZ_Assert(m_recordingFrame, "The frame packet wasn't started. You need to call StartFramePacket first.");

            m_commandList->EndCommandBuffer();

            FramePacket& framePacket = m_framePackets[m_frameIndex];
            const AZStd::vector<RHI::Ptr<CommandList>> commandBuffers{ m_commandList };
            static const AZStd::vector<Semaphore::WaitSemaphore> semaphoresWaitInfo;
            AZStd::vector<RHI::Ptr<Semaphore>> semaphoresToSignal;
            if (semaphoreToSignal)
            {
                semaphoresToSignal.push_back(semaphoreToSignal);
            }
            queue->SubmitCommandBuffers(commandBuffers, semaphoresWaitInfo, semaphoresToSignal, framePacket.m_fence.get());
            queue->EndDebugLabel();

            m_frameIndex = (m_frameIndex + 1) % m_descriptor.m_frameCount;
            m_recordingFrame = false;
        }

        void AsyncUploadQueue::EmmitPrologueMemoryBarrier(const Buffer& buffer, size_t offset, size_t size)
        {
            const BufferMemoryView* memoryView = buffer.GetBufferMemoryView();

            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = memoryView->GetNativeBuffer();
            barrier.offset = memoryView->GetOffset() + offset;
            barrier.size = size;

            vkCmdPipelineBarrier(m_commandList->GetNativeCommandBuffer(),
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0,
                nullptr,
                1,
                &barrier,
                0,
                nullptr);
        }

        void AsyncUploadQueue::EmmitPrologueMemoryBarrier(const RHI::StreamingImageExpandRequest& request, uint32_t residentMip)
        {
            const auto& image = static_cast<const Image&>(*request.m_image);
            const uint32_t beforeMip = residentMip;
            const uint32_t afterMip = beforeMip - static_cast<uint32_t>(request.m_mipSlices.size());

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image.GetNativeImage();
            barrier.subresourceRange.aspectMask = image.GetImageAspectFlags();
            barrier.subresourceRange.baseMipLevel = afterMip;
            barrier.subresourceRange.levelCount = beforeMip - afterMip;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = image.GetDescriptor().m_arraySize;

            vkCmdPipelineBarrier(m_commandList->GetNativeCommandBuffer(),
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);
        }

        void AsyncUploadQueue::EmmitEpilogueMemoryBarrier(
            [[maybe_unused]] CommandList& commandList,
            const Buffer& buffer,
            size_t offset,
            size_t size)
        {
            const BufferMemoryView* memoryView = buffer.GetBufferMemoryView();

            VkBufferMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex =  VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = memoryView->GetNativeBuffer();
            barrier.offset = memoryView->GetOffset() + offset;
            barrier.size = size;

            vkCmdPipelineBarrier(m_commandList->GetNativeCommandBuffer(),
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0,
                nullptr,
                1,
                &barrier,
                0,
                nullptr);
        }

        void AsyncUploadQueue::EmmitEpilogueMemoryBarrier(
            CommandList& commandList,
            const RHI::StreamingImageExpandRequest& request,
            uint32_t residentMip)
        {
            const auto& image = static_cast<const Image&>(*request.m_image);
            const VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            const uint32_t beforeMip = residentMip;
            const uint32_t afterMip = beforeMip - static_cast<uint32_t>(request.m_mipSlices.size());

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.pNext = nullptr;
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = 0;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = layout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image.GetNativeImage();
            barrier.subresourceRange.aspectMask = image.GetImageAspectFlags();
            barrier.subresourceRange.baseMipLevel = afterMip;
            barrier.subresourceRange.levelCount = beforeMip - afterMip;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = image.GetDescriptor().m_arraySize;

            vkCmdPipelineBarrier(
                commandList.GetNativeCommandBuffer(),
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);
        }

        RHI::AsyncWorkHandle AsyncUploadQueue::CreateAsyncWork(RHI::Ptr<Fence> fence, RHI::Fence::SignalCallback callback /* = nullptr */)
        {
            return m_asyncWaitQueue.CreateAsyncWork([fence, callback]()
            {
                fence->WaitOnCpu();
                if (callback)
                {
                    callback();
                }
            });
        }

        void AsyncUploadQueue::ProcessCallback(const RHI::AsyncWorkHandle& handle)
        {
            AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: ProcessCallback");
            AZStd::unique_lock<AZStd::mutex> lock(m_callbackListMutex);
            auto findIter = m_callbackList.find(handle);
            if (findIter != m_callbackList.end())
            {
                findIter->second();
                m_callbackList.erase(findIter);
            }
        }
    }
}
