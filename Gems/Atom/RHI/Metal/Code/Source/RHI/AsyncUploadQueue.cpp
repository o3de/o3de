/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <Atom/RHI/DeviceBufferPool.h>
#include <Atom/RHI/CommandQueue.h>
#include <AzCore/Component/TickBus.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <RHI/Fence.h>

namespace AZ
{
    namespace Metal
    {
        AsyncUploadQueue::Descriptor::Descriptor(size_t stagingSizeInBytes)
        {
            m_stagingSizeInBytes = stagingSizeInBytes;
        }

        void AsyncUploadQueue::Init(Device& device, const Descriptor& descriptor)
        {
            Base::Init(device);
            id<MTLDevice> hwDevice = device.GetMtlDevice();
            
            m_copyQueue = &device.GetCommandQueueContext().GetCommandQueue(RHI::HardwareQueueClass::Copy);
            
            m_uploadFence.Init(&device, RHI::FenceState::Signaled);
            m_commandBuffer.Init(m_copyQueue->GetPlatformQueue());
            
            NSUInteger bufferOptions = MTLResourceCPUCacheModeWriteCombined //< optimize for cpu write once
                                     | CovertStorageMode(GetCPUGPUMemoryMode()) //< Managed for mac and shared for ios
                                     | MTLResourceHazardTrackingModeUntracked //< The upload queue already has to track this so tell the driver not to duplicate the work
                                     ;
             
           for (size_t i = 0; i < descriptor.m_frameCount; ++i)
           {
                m_framePackets.emplace_back();

                FramePacket& framePacket = m_framePackets.back();
                framePacket.m_fence.Init(&device, RHI::FenceState::Signaled);

                framePacket.m_stagingResource = [hwDevice newBufferWithLength:descriptor.m_stagingSizeInBytes options:bufferOptions];

                framePacket.m_stagingResourceData = static_cast<uint8_t*>(framePacket.m_stagingResource.contents);
            }
            
            m_asyncWaitQueue.Init();
        }

        void AsyncUploadQueue::Shutdown()
        {
            m_copyQueue->Shutdown();

            for (size_t i = 0; i < m_descriptor.m_frameCount; ++i)
            {
                m_framePackets[i].m_fence.Shutdown();
            }
            m_framePackets.clear();
            m_uploadFence.Shutdown();
            m_asyncWaitQueue.ShutDown();
            m_callbackList.clear();
            Base::Shutdown();
        }

        uint64_t AsyncUploadQueue::QueueUpload(const RHI::DeviceBufferStreamRequest& uploadRequest)
        {
            Buffer& destBuffer = static_cast<Buffer&>(*uploadRequest.m_buffer);
            const MemoryView& destMemoryView = destBuffer.GetMemoryView();
            MTLStorageMode mtlStorageMode = destBuffer.GetMemoryView().GetStorageMode();
            RHI::DeviceBufferPool& bufferPool = static_cast<RHI::DeviceBufferPool&>(*destBuffer.GetPool());
            
            // No need to use staging buffers since it's host memory.
            // We just map, copy and then unmap.
            if(mtlStorageMode == MTLStorageModeShared || mtlStorageMode == GetCPUGPUMemoryMode())
            {
                RHI::DeviceBufferMapRequest mapRequest;
                mapRequest.m_buffer = uploadRequest.m_buffer;
                mapRequest.m_byteCount = uploadRequest.m_byteCount;
                mapRequest.m_byteOffset = uploadRequest.m_byteOffset;
                RHI::DeviceBufferMapResponse mapResponse;
                bufferPool.MapBuffer(mapRequest, mapResponse);
                ::memcpy(mapResponse.m_data, uploadRequest.m_sourceData, uploadRequest.m_byteCount);
                bufferPool.UnmapBuffer(*uploadRequest.m_buffer);
                if (uploadRequest.m_fenceToSignal)
                {
                    uploadRequest.m_fenceToSignal->SignalOnCpu();
                }
                return m_uploadFence.GetPendingValue();
            }
            
            Fence* fenceToSignal = nullptr;
            size_t byteCount = uploadRequest.m_byteCount;
            size_t byteOffset = destMemoryView.GetOffset() + uploadRequest.m_byteOffset;            
            uint64_t queueValue = m_uploadFence.Increment();
            
            const uint8_t* sourceData = reinterpret_cast<const uint8_t*>(uploadRequest.m_sourceData);

            if (uploadRequest.m_fenceToSignal)
            {
                Fence& fence = static_cast<FenceImpl*>(uploadRequest.m_fenceToSignal)->Get();
                fenceToSignal = &fence;
            }

            m_copyQueue->QueueCommand([=](void* queue)
            {
                AZ_PROFILE_SCOPE(RHI, "Upload Buffer");
                size_t pendingByteOffset = 0;
                size_t pendingByteCount = byteCount;
                CommandQueue* commandQueue = static_cast<CommandQueue*>(queue);
                
                while (pendingByteCount > 0)
                {
                    AZ_PROFILE_SCOPE(RHI, "Upload Buffer Chunk");

                    FramePacket* framePacket = BeginFramePacket(commandQueue);

                    const size_t bytesToCopy = AZStd::min(pendingByteCount, m_descriptor.m_stagingSizeInBytes);

                    {
                        AZ_PROFILE_SCOPE(RHI, "Copy CPU buffer");
                        memcpy(framePacket->m_stagingResourceData, sourceData + pendingByteOffset, bytesToCopy);
                        Platform::PublishBufferCpuChangeOnGpu(framePacket->m_stagingResource, 0, bytesToCopy);
                    }

                    id<MTLBlitCommandEncoder> blitEncoder = [framePacket->m_mtlCommandBuffer blitCommandEncoder];
                    [blitEncoder copyFromBuffer: framePacket->m_stagingResource
                                   sourceOffset: 0
                                       toBuffer: destMemoryView.GetGpuAddress<id<MTLBuffer>>()
                              destinationOffset: byteOffset + pendingByteOffset
                                           size: bytesToCopy];
                    [blitEncoder endEncoding];
                    blitEncoder = nil;

                    pendingByteOffset += bytesToCopy;
                    pendingByteCount -= bytesToCopy;
                    
                    if (!pendingByteCount) // do signals on the last frame packet
                    {
                        if (fenceToSignal)
                        {
                            fenceToSignal->SignalFromGpu(framePacket->m_mtlCommandBuffer);
                        }
                        
                        m_uploadFence.SignalFromGpu(framePacket->m_mtlCommandBuffer, queueValue);
                    }

                    EndFramePacket(commandQueue);
                }
            });

            return queueValue;
        }
                
        // [GFX TODO][ATOM-4205] Stage/Upload 3D streaming images more efficiently.
        RHI::AsyncWorkHandle AsyncUploadQueue::QueueUpload(const RHI::DeviceStreamingImageExpandRequest& request, uint32_t residentMip)
        {
            auto& device = static_cast<Device&>(GetDevice());
            id<MTLDevice> mtlDevice = device.GetMtlDevice();
            auto* image = static_cast<Image*>(request.m_image.get());
            const uint16_t startMip = residentMip - 1;
            const uint16_t endMip = static_cast<uint16_t>(residentMip - request.m_mipSlices.size());

            uint64_t queueValue = m_uploadFence.Increment();
            
            CommandQueue::Command command = [=](void* queue)
            {
                CommandQueue* commandQueue = static_cast<CommandQueue*>(queue);
                FramePacket* framePacket = BeginFramePacket(commandQueue);
                
                //[GFX TODO][ATOM-5605] - Cache alignments for all formats at Init
                const static uint32_t bufferOffsetAlign = static_cast<uint32_t>([mtlDevice minimumTextureBufferAlignmentForPixelFormat: ConvertPixelFormat(image->GetDescriptor().m_format)]);

                // Variables for split subresource slice.
                // If a subresource slice pitch is large than one staging size, we may split the slice by rows.
                // And using the CopyTextureRegion to only copy a section of the subresource
                bool needSplitSlice = false;

                for (uint16_t curMip = endMip; curMip <= startMip; ++curMip)
                {
                    const size_t sliceIndex = curMip - endMip;
                    const RHI::DeviceImageSubresourceLayout& subresourceLayout = request.m_mipSlices[sliceIndex].m_subresourceLayout;
                    uint32_t arraySlice = 0;
                    const uint32_t subresourceSlicePitch = subresourceLayout.m_bytesPerImage;

                    // Staging sizes
                    const uint32_t stagingRowPitch = RHI::AlignUp(subresourceLayout.m_bytesPerRow, bufferOffsetAlign);
                    const uint32_t stagingSlicePitch = RHI::AlignUp(subresourceLayout.m_rowCount * stagingRowPitch, bufferOffsetAlign);
                    const uint32_t rowsPerSplit = static_cast<uint32_t>(m_descriptor.m_stagingSizeInBytes) / stagingRowPitch;
                    const uint32_t compressedTexelBlockSizeHeight = subresourceLayout.m_blockElementHeight;
                    
                    // ImageHeight must be bigger than or equal to the Image's row count. Images with a RowCount that is less than the ImageHeight indicates a block compression.
                    // Images with a RowCount which is higher than the ImageHeight indicates a planar image, which is not supported for streaming images.
                    if (subresourceLayout.m_size.m_height < subresourceLayout.m_rowCount)
                    {
                        AZ_Error("Metal", false, "AsyncUploadQueue::QueueUpload expects ImageHeight '%d' to be bigger than or equal to the image's RowCount '%d'.", subresourceLayout.m_size.m_height, subresourceLayout.m_rowCount);
                    }

                    // The final staging size for each CopyTextureRegion command
                    uint32_t stagingSize = stagingSlicePitch;

                    // Prepare for splitting this subresource if needed.
                    if (stagingSlicePitch > m_descriptor.m_stagingSizeInBytes)
                    {
                        // Calculate minimum size of one row of this subresource.
                        if (stagingRowPitch > m_descriptor.m_stagingSizeInBytes)
                        {
                            AZ_Warning("Metal", false, "AsyncUploadQueue staging buffer (%dK) is not big enough"
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
                                const uint8_t* subresourceDataStart = reinterpret_cast<const uint8_t*>(subresourceData.m_data) + depth * subresourceSlicePitch;

                                // If the current framePacket is not big enough, switch to next one.
                                if (stagingSize > m_descriptor.m_stagingSizeInBytes - framePacket->m_dataOffset)
                                {
                                    EndFramePacket(commandQueue);
                                    framePacket = BeginFramePacket(commandQueue);
                                }

                                // Copy subresource data to staging memory.
                                uint8_t* stagingDataStart = framePacket->m_stagingResourceData + framePacket->m_dataOffset;
                                for (uint32_t row = 0; row < subresourceLayout.m_rowCount; ++row)
                                {
                                    memcpy(stagingDataStart + row * stagingRowPitch, subresourceDataStart + row * subresourceLayout.m_bytesPerRow, subresourceLayout.m_bytesPerRow);
                                }

                                const uint32_t bytesCopied = subresourceLayout.m_rowCount * stagingRowPitch;
                                Platform::PublishBufferCpuChangeOnGpu(framePacket->m_stagingResource, framePacket->m_dataOffset, bytesCopied);

                                RHI::Size sourceSize = subresourceLayout.m_size;
                                sourceSize.m_depth = 1;
                                CopyBufferToImage(framePacket, image, stagingRowPitch, stagingSlicePitch,
                                    curMip, arraySlice, sourceSize, RHI::Origin(0, 0, depth));

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
                                const uint8_t* subresourceDataStart = reinterpret_cast<const uint8_t*>(subresourceData.m_data) + depth * subresourceSlicePitch;

                                uint32_t startRow = 0;
                                uint32_t destHeight = 0;
                                while (startRow < subresourceLayout.m_rowCount)
                                {
                                    if (stagingSize > m_descriptor.m_stagingSizeInBytes - framePacket->m_dataOffset)
                                    {
                                        EndFramePacket(commandQueue);
                                        framePacket = BeginFramePacket(commandQueue);
                                    }

                                    const uint32_t endRow = AZStd::min(startRow + rowsPerSplit, subresourceLayout.m_rowCount);

                                    // Calculate the blocksize for BC formatted images; the copy command works in texels.
                                    uint32_t heightToCopy = (endRow - startRow) * compressedTexelBlockSizeHeight;

                                    // Copy subresource data to staging memory.
                                    uint8_t* stagingDataStart = framePacket->m_stagingResourceData + framePacket->m_dataOffset;
                                    for (uint32_t row = startRow; row < endRow; ++row)
                                    {
                                        memcpy(stagingDataStart + (row - startRow) * stagingRowPitch, subresourceDataStart + row * subresourceLayout.m_bytesPerRow, subresourceLayout.m_bytesPerRow);
                                    }

                                    const uint32_t bytesCopied = (endRow - startRow) * stagingRowPitch;
                                    Platform::PublishBufferCpuChangeOnGpu(framePacket->m_stagingResource, framePacket->m_dataOffset, bytesCopied);

                                    //Clamp heightToCopy to match subresourceLayout.m_size.m_height as it is possible to go over
                                    //if subresourceLayout.m_size.m_height is not perfectly divisible by compressedTexelBlockSizeHeight
                                    if(destHeight+heightToCopy > subresourceLayout.m_size.m_height)
                                    {
                                        uint32_t HeightDiff = (destHeight + heightToCopy) - subresourceLayout.m_size.m_height;
                                        heightToCopy -= HeightDiff;
                                    }
                                                                                         
                                    const RHI::Size sourceSize = RHI::Size(subresourceLayout.m_size.m_width, heightToCopy, 1);
                                    const RHI::Origin sourceOrigin = RHI::Origin(0, destHeight, depth);
                                    CopyBufferToImage(framePacket, image, stagingRowPitch, bytesCopied,
                                        curMip, arraySlice, sourceSize, sourceOrigin);

                                    framePacket->m_dataOffset += stagingSize;
                                    startRow = endRow;
                                    destHeight += heightToCopy;
                                }
                            }
                            ++arraySlice;
                        }
                    }
                }

                m_uploadFence.SignalFromGpu(framePacket->m_mtlCommandBuffer, queueValue);
                EndFramePacket(commandQueue);
            };

            m_copyQueue->QueueCommand(AZStd::move(command));
                    
            if (request.m_waitForUpload)
            {
                // No need to add wait event.
                m_uploadFence.WaitOnCpu();
                if (request.m_completeCallback)
                {
                    request.m_completeCallback();
                }
            }
            else
            {
                RHI::AsyncWorkHandle uploadHandle;
                if (request.m_completeCallback)
                {
                    auto waitEvent = [this, request, image]()
                    {
                        RHI::AsyncWorkHandle uploadHandle = image->GetUploadHandle();                        
                        // Add the callback so it can be processed from the main thread.
                        {
                            AZStd::unique_lock<AZStd::mutex> lock(m_callbackListMutex);
                            m_callbackList.insert(AZStd::make_pair(uploadHandle, [request, image]() { image->SetUploadHandle(RHI::AsyncWorkHandle::Null); request.m_completeCallback(); }));
                        }
                        // We could just add a lambda that calls the request.m_completeCallback() but that could create a crash
                        // if the image is destroyed before the callback is triggered from the TickBus. Because of this we save the callbacks in this
                        // class and when an image is destroyed, we just execute any pending callback for that image.
                        AZ::TickBus::QueueFunction([this, uploadHandle]() { ProcessCallback(uploadHandle); });
                        
                    };
                    uploadHandle = CreateAsyncWork(m_uploadFence, waitEvent);
                    
                }
                else
                {
                    uploadHandle = CreateAsyncWork(m_uploadFence, nullptr);
                }
                image->SetUploadHandle(uploadHandle);
                m_asyncWaitQueue.UnlockAsyncWorkQueue();
                return uploadHandle;
            }

            return RHI::AsyncWorkHandle::Null;
        }

        AsyncUploadQueue::FramePacket* AsyncUploadQueue::BeginFramePacket(CommandQueue* commandQueue)
        {
            AZ_Assert(!m_recordingFrame, "The previous frame packet isn't ended");

            AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: Wait copy frame");
            FramePacket& framePacket = m_framePackets[m_frameIndex];
            framePacket.m_fence.WaitOnCpu(); // ensure any previous uploads using this frame have completed
                        
            framePacket.m_fence.Increment();
            framePacket.m_dataOffset = 0;
            
            framePacket.m_mtlCommandBuffer = commandQueue->GetCommandBuffer().AcquireMTLCommandBuffer();
            
            m_recordingFrame = true;

            return &framePacket;
        }

        void AsyncUploadQueue::EndFramePacket(CommandQueue* commandQueue)
        {
            //Autoreleasepool is to ensure that the driver is not leaking memory related to the command buffer and encoder
            @autoreleasepool
            {
                AZ_Assert(m_recordingFrame, "The frame packet wasn't started. You need to call StartFramePacket first.");

                AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: Execute command");
                FramePacket& framePacket = m_framePackets[m_frameIndex];
                framePacket.m_fence.SignalFromGpu(framePacket.m_mtlCommandBuffer); // signal fence when this upload haas completed

                m_frameIndex = (m_frameIndex + 1) % m_descriptor.m_frameCount;
                commandQueue->GetCommandBuffer().CommitMetalCommandBuffer();
                framePacket.m_mtlCommandBuffer = nil;
                m_recordingFrame = false;
            }
        }

        bool AsyncUploadQueue::IsUploadFinished(uint64_t fenceValue)
        {
            return m_uploadFence.GetCompletedValue() >= fenceValue;
        }

        void AsyncUploadQueue::WaitForUpload(const RHI::AsyncWorkHandle& workHandle)
        {
            m_asyncWaitQueue.WaitToFinish(workHandle);
            ProcessCallback(workHandle);
        }
    
        RHI::AsyncWorkHandle AsyncUploadQueue::CreateAsyncWork(Fence& fence, RHI::DeviceFence::SignalCallback callback )
        {
            return m_asyncWaitQueue.CreateAsyncWork([fence, callback]()
            {
                fence.WaitOnCpu();
                if (callback)
                {
                    callback();
                }
            });
        }
    
        void AsyncUploadQueue::ProcessCallback(const RHI::AsyncWorkHandle& handle)
        {
            AZStd::unique_lock<AZStd::mutex> lock(m_callbackListMutex);
            auto findIter = m_callbackList.find(handle);
            if (findIter != m_callbackList.end())
            {
                findIter->second();
                m_callbackList.erase(findIter);
            }
        }
    
        void AsyncUploadQueue::CopyBufferToImage(FramePacket* framePacket,
                                                 Image* destImage,
                                                 uint32_t stagingRowPitch,
                                                 uint32_t stagingSlicePitch,
                                                 uint16_t mipSlice,
                                                 uint16_t arraySlice,
                                                 RHI::Size sourceSize,
                                                 RHI::Origin sourceOrigin)
        {
            id<MTLBlitCommandEncoder> blitEncoder = [framePacket->m_mtlCommandBuffer blitCommandEncoder];
            MTLOrigin destinationOrigin = MTLOriginMake(sourceOrigin.m_left,sourceOrigin.m_top,sourceOrigin.m_front);
            
            MTLSize mtlSourceSize = MTLSizeMake(sourceSize.m_width,
                                            sourceSize.m_height,
                                            sourceSize.m_depth);
            
            MTLBlitOption mtlBlitOption = GetBlitOption(destImage->GetDescriptor().m_format, RHI::ImageAspect::Color);

            [blitEncoder copyFromBuffer:framePacket->m_stagingResource
                           sourceOffset:framePacket->m_dataOffset
                      sourceBytesPerRow:stagingRowPitch
                    sourceBytesPerImage:stagingSlicePitch
                             sourceSize:mtlSourceSize
                              toTexture:destImage->GetMemoryView().GetGpuAddress<id<MTLTexture>>()
                       destinationSlice:arraySlice
                       destinationLevel:mipSlice
                      destinationOrigin:destinationOrigin
                                options:mtlBlitOption];

            [blitEncoder endEncoding];
            blitEncoder = nil;
        }
    }
}
