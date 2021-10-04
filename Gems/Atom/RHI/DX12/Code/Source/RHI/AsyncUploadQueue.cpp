/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>
#include <Atom/RHI.Reflect/PlatformLimitsDescriptor.h>
#include <RHI/AsyncUploadQueue.h>
#include <RHI/Buffer.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/Conversions.h>
#include <Atom/RHI/StreamingImagePool.h>
#include <Atom/RHI/BufferPool.h>
#include <AzCore/Component/TickBus.h>

namespace AZ
{
    namespace DX12
    {
        AsyncUploadQueue::Descriptor::Descriptor(size_t stagingSizeInBytes)
        {
            m_stagingSizeInBytes = stagingSizeInBytes;
        }

        void AsyncUploadQueue::Init(RHI::Device& deviceBase, const Descriptor& descriptor)
        {
            Base::Init(deviceBase);
            auto& device = static_cast<Device&>(deviceBase);
            ID3D12DeviceX* dx12Device = device.GetDevice();

            m_copyQueue = CommandQueue::Create();
            
            // Make a secondary Copy queue, the primary queue is owned by the CommandQueueContext
            CommandQueueDescriptor commandQueueDesc;
            commandQueueDesc.m_hardwareQueueClass = RHI::HardwareQueueClass::Copy;
            commandQueueDesc.m_hardwareQueueSubclass = HardwareQueueSubclass::Secondary;
            m_copyQueue->Init(device, commandQueueDesc);
            m_uploadFence.Init(dx12Device, RHI::FenceState::Signaled);

            for (size_t i = 0; i < descriptor.m_frameCount; ++i)
            {
                m_framePackets.emplace_back();

                FramePacket& framePacket = m_framePackets.back();
                framePacket.m_fence.Init(dx12Device, RHI::FenceState::Signaled);

                CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_UPLOAD);
                CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(descriptor.m_stagingSizeInBytes);

                Microsoft::WRL::ComPtr<ID3D12Resource> stagingResource;
                AssertSuccess(dx12Device->CreateCommittedResource(
                    &heapProperties,
                    D3D12_HEAP_FLAG_NONE,
                    &bufferDesc,
                    D3D12_RESOURCE_STATE_GENERIC_READ,
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(stagingResource.GetAddressOf())));

                framePacket.m_stagingResource = stagingResource.Get();
                CD3DX12_RANGE readRange(0, 0);
                framePacket.m_stagingResource->Map(0, &readRange, reinterpret_cast<void**>(&framePacket.m_stagingResourceData));

                
                Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
                AssertSuccess(dx12Device->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_COPY,
                    IID_GRAPHICS_PPV_ARGS(commandAllocator.GetAddressOf())));
                framePacket.m_commandAllocator = commandAllocator.Get();

                Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
                AssertSuccess(dx12Device->CreateCommandList(
                    0,
                    D3D12_COMMAND_LIST_TYPE_COPY,
                    framePacket.m_commandAllocator.get(),
                    nullptr,
                    IID_GRAPHICS_PPV_ARGS(commandList.GetAddressOf())));

                framePacket.m_commandList = commandList.Get();
                AssertSuccess(framePacket.m_commandList->Close());
            }
        }

        void AsyncUploadQueue::Shutdown()
        {
            if (m_copyQueue)
            {
                m_copyQueue->Shutdown();
                m_copyQueue = nullptr;
            }

            for (auto& framePacket : m_framePackets)
            {
                framePacket.m_fence.Shutdown();
                framePacket.m_commandList = nullptr;
                framePacket.m_commandAllocator = nullptr;
            }
            m_framePackets.clear();
            m_uploadFence.Shutdown();
            Base::Shutdown();
        }

        uint64_t AsyncUploadQueue::QueueUpload(const RHI::BufferStreamRequest& uploadRequest)
        {
            // Take a reference on the DX12 buffer / fence to make sure that they stay alive
            // for the duration of the upload. This also allows the higher level buffer / fence
            // objects to be independently shutdown without issue.
            //
            // The only requirement is that uploadRequest.m_sourceData remain intact for the duration
            // of the upload operation.

            Buffer& buffer = static_cast<Buffer&>(*uploadRequest.m_buffer);
            RHI::BufferPool& bufferPool = static_cast<RHI::BufferPool&>(*buffer.GetPool());
            if (bufferPool.GetDescriptor().m_heapMemoryLevel == RHI::HeapMemoryLevel::Host)
            {
                // No need to use staging buffers since it's host memory.
                // We just map, copy and then unmap.
                RHI::BufferMapRequest mapRequest;
                mapRequest.m_buffer = uploadRequest.m_buffer;
                mapRequest.m_byteCount = uploadRequest.m_byteCount;
                mapRequest.m_byteOffset = uploadRequest.m_byteOffset;
                RHI::BufferMapResponse mapResponse;
                bufferPool.MapBuffer(mapRequest, mapResponse);
                ::memcpy(mapResponse.m_data, uploadRequest.m_sourceData, uploadRequest.m_byteCount);
                bufferPool.UnmapBuffer(*uploadRequest.m_buffer);
                if (uploadRequest.m_fenceToSignal)
                {
                    uploadRequest.m_fenceToSignal->SignalOnCpu();
                }
                return m_uploadFence.GetPendingValue();
            }

            const MemoryView& memoryView = buffer.GetMemoryView();
            RHI::Ptr<ID3D12Resource> dx12Buffer = memoryView.GetMemory();

            RHI::Ptr<ID3D12Fence> dx12FenceToSignal;
            uint64_t dx12FenceToSignalValue = 0;

            size_t byteCount = uploadRequest.m_byteCount;
            size_t byteOffset = memoryView.GetOffset() + uploadRequest.m_byteOffset;
            const uint8_t* sourceData = reinterpret_cast<const uint8_t*>(uploadRequest.m_sourceData);

            if (uploadRequest.m_fenceToSignal)
            {
                Fence& fence = static_cast<FenceImpl&>(*uploadRequest.m_fenceToSignal).Get();
                dx12FenceToSignal = fence.Get();
                dx12FenceToSignalValue = fence.GetPendingValue();
            }

            uint64_t queueValue = m_uploadFence.Increment();

            m_copyQueue->QueueCommand([=](void* commandQueue)
            {
                AZ_PROFILE_SCOPE(RHI, "Upload Buffer");
                size_t pendingByteOffset = 0;
                size_t pendingByteCount = byteCount;
                ID3D12CommandQueue* dx12CommandQueue = static_cast<ID3D12CommandQueue*>(commandQueue);

                while (pendingByteCount > 0)
                {
                    AZ_PROFILE_SCOPE(RHI, "Upload Buffer Chunk");

                    FramePacket* framePacket = BeginFramePacket();

                    const size_t bytesToCopy = AZStd::min(pendingByteCount, m_descriptor.m_stagingSizeInBytes);

                    {
                        AZ_PROFILE_SCOPE(RHI, "Copy CPU buffer");
                        memcpy(framePacket->m_stagingResourceData, sourceData + pendingByteOffset, bytesToCopy);
                    }

                    framePacket->m_commandList->CopyBufferRegion(
                        dx12Buffer.get(),
                        byteOffset + pendingByteOffset,
                        framePacket->m_stagingResource.get(),
                        0,
                        bytesToCopy);

                    pendingByteOffset += bytesToCopy;
                    pendingByteCount -= bytesToCopy;

                    EndFramePacket(dx12CommandQueue);
                }

                if (dx12FenceToSignal)
                {
                    dx12CommandQueue->Signal(dx12FenceToSignal.get(), dx12FenceToSignalValue);
                }

                dx12CommandQueue->Signal(m_uploadFence.Get(), queueValue);
            });

            return queueValue;
        }

        AsyncUploadQueue::FramePacket* AsyncUploadQueue::BeginFramePacket()
        {
            AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: BeginFramePacket");
            AZ_Assert(!m_recordingFrame, "The previous frame packet isn't ended");

            FramePacket* framePacket = &m_framePackets[m_frameIndex];
            framePacket->m_fence.Wait(m_fenceEvent);
            framePacket->m_fence.Increment();
            framePacket->m_dataOffset = 0;

            AssertSuccess(framePacket->m_commandAllocator->Reset());
            AssertSuccess(framePacket->m_commandList->Reset(framePacket->m_commandAllocator.get(), nullptr));

            m_recordingFrame = true;

            return framePacket;
        }

        void AsyncUploadQueue::EndFramePacket(ID3D12CommandQueue* commandQueue)
        {
            AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: EndFramePacket");
            AZ_Assert(m_recordingFrame, "The frame packet wasn't started. You need to call StartFramePacket first.");

            FramePacket& framePacket = m_framePackets[m_frameIndex];

            AssertSuccess(framePacket.m_commandList->Close());
            ID3D12CommandList* commandLists[] = { framePacket.m_commandList.get() };
            commandQueue->ExecuteCommandLists(1, commandLists);

            commandQueue->Signal(framePacket.m_fence.Get(), framePacket.m_fence.GetPendingValue());

            m_frameIndex = (m_frameIndex + 1) % m_descriptor.m_frameCount;
            m_recordingFrame = false;
        }

        // [GFX TODO][ATOM-4205] Stage/Upload 3D streaming images more efficiently.
        uint64_t AsyncUploadQueue::QueueUpload(const RHI::StreamingImageExpandRequest& request, uint32_t residentMip)
        {
            AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: QueueUpload");

            uint64_t fenceValue = m_uploadFence.Increment();

            Image* image = static_cast<Image*>(request.m_image);
            image->SetUploadFenceValue(fenceValue);

            uint32_t startMip = residentMip - 1;
            uint32_t endMip = residentMip - static_cast<uint32_t>(request.m_mipSlices.size());

            Memory* imageMemory = static_cast<Image&>(*request.m_image).GetMemoryView().GetMemory();

            m_copyQueue->QueueCommand([=](void* commandQueue)
            {
                AZ_PROFILE_SCOPE(RHI, "Upload Image");
                ID3D12CommandQueue* dx12CommandQueue = static_cast<ID3D12CommandQueue*>(commandQueue);
                FramePacket* framePacket = BeginFramePacket();

                uint32_t arraySize = request.m_image->GetDescriptor().m_arraySize;
                uint16_t imageMipLevels = request.m_image->GetDescriptor().m_mipLevels;

                // Variables for split subresource slice. 
                // If a subresource slice pitch is large than one staging size, we may split the slice by rows.
                // And using the CopyTextureRegion to only copy a section of the subresource 
                bool needSplitSlice = false;
                uint32_t rowsPerSplit = 0;

                for (uint32_t curMip = endMip; curMip <= startMip; curMip++)
                {
                    size_t sliceIndex = curMip - endMip;
                    const RHI::ImageSubresourceLayout& subresourceLayout = request.m_mipSlices[sliceIndex].m_subresourceLayout;
                    uint32_t arraySlice = 0;
                    const uint32_t subresourceSlicePitch = subresourceLayout.m_bytesPerImage;

                    // Staging sizes
                    uint32_t stagingRowPitch = RHI::AlignUp(subresourceLayout.m_bytesPerRow, DX12_TEXTURE_DATA_PITCH_ALIGNMENT);
                    uint32_t stagingSlicePitch = RHI::AlignUp(subresourceLayout.m_rowCount*stagingRowPitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
                    const uint32_t compressedTexelBlockSizeHeight = subresourceLayout.m_blockElementHeight;

                    // ImageHeight must be bigger than or equal to the Image's row count. Images with a RowCount that is less than the ImageHeight indicates a block compression.
                    // Images with a RowCount which is higher than the ImageHeight indicates a planar image, which is not supported for streaming images.
                    if (subresourceLayout.m_size.m_height < subresourceLayout.m_rowCount)
                    {
                        AZ_Error("StreamingImage", false, "AsyncUploadQueue::QueueUpload expects ImageHeight '%d' to be bigger than or equal to the image's RowCount '%d'.", subresourceLayout.m_size.m_height, subresourceLayout.m_rowCount);
                        return 0;
                    }

                    // The final staging size for each CopyTextureRegion command
                    uint32_t stagingSize = stagingSlicePitch;

                    // Prepare for spliting this subresource if needed
                    if (stagingSlicePitch > m_descriptor.m_stagingSizeInBytes)
                    {
                        // Calculate minimum size of one row of this subresrouce
                        uint32_t minSize = RHI::AlignUp(stagingRowPitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
                        if (minSize > m_descriptor.m_stagingSizeInBytes)
                        {
                            AZ_Warning("RHI::DX12", false, "AsyncUploadQueue staging buffer (%dK) is not big enough"
                                "for the size of one row of image's sub-resource (%dK). Please increase staging buffer size.",
                                m_descriptor.m_stagingSizeInBytes / 1024.0f, stagingSlicePitch / 1024.f);
                            continue;
                        }

                        needSplitSlice = true;
                        rowsPerSplit = static_cast<uint32_t>(RHI::AlignDown(m_descriptor.m_stagingSizeInBytes, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT) / stagingRowPitch);
                        stagingSize = RHI::AlignUp(rowsPerSplit * stagingRowPitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
                        AZ_Assert(stagingSize <= m_descriptor.m_stagingSizeInBytes, "Final staging size can't be larger than staging buffer size");
                    }

                    if (!needSplitSlice)
                    {
                        // Try to use one frame packet for all sub-resources if it's possible.
                        for (const auto& subresource : request.m_mipSlices[sliceIndex].m_subresources)
                        {
                            for (uint32_t depth = 0; depth < subresourceLayout.m_size.m_depth; depth++)
                            {
                                // If the current framePacket is not big enough, switch to next one.
                                if (stagingSize > m_descriptor.m_stagingSizeInBytes - framePacket->m_dataOffset)
                                {
                                    EndFramePacket(dx12CommandQueue);
                                    framePacket = BeginFramePacket();
                                }

                                // Copy subresource data to staging memory.
                                {
                                    AZ_PROFILE_SCOPE(RHI, "Copy CPU image");

                                    uint8_t* stagingDataStart = framePacket->m_stagingResourceData + framePacket->m_dataOffset;
                                    const uint8_t* subresourceSliceDataStart = static_cast<const uint8_t*>(subresource.m_data) + (depth * subresourceSlicePitch);

                                    for (uint32_t row = 0; row < subresourceLayout.m_rowCount; row++)
                                    {
                                        memcpy(stagingDataStart + row * stagingRowPitch,
                                            subresourceSliceDataStart + row * subresourceLayout.m_bytesPerRow,
                                            subresourceLayout.m_bytesPerRow);
                                    }
                                }

                                // Add copy command to copy image subresource from staging memory to image gpu resource.

                                // Source location
                                D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
                                const DXGI_FORMAT imageFormat = GetBaseFormat(ConvertFormat(request.m_image->GetDescriptor().m_format));
                                footprint.Footprint.Width = subresourceLayout.m_size.m_width;
                                footprint.Footprint.Height = subresourceLayout.m_size.m_height;
                                footprint.Footprint.Depth = 1;
                                footprint.Footprint.Format = imageFormat;
                                footprint.Footprint.RowPitch = stagingRowPitch;
                                footprint.Offset = framePacket->m_dataOffset;
                                CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(framePacket->m_stagingResource.get(), footprint);

                                // Dest location.
                                const uint32_t subresourceIdx = D3D12CalcSubresource(curMip, arraySlice, 0, imageMipLevels, arraySize);
                                CD3DX12_TEXTURE_COPY_LOCATION destLocation(imageMemory, subresourceIdx);

                                framePacket->m_commandList->CopyTextureRegion(
                                    &destLocation,
                                    0, 0, depth,
                                    &sourceLocation,
                                    nullptr);

                                framePacket->m_dataOffset += stagingSlicePitch;
                            }
                            // Next slice in this array.
                            arraySlice++;
                        }
                    }
                    else
                    {
                        // Each subresource need to be split.
                        for (const auto& subresource : request.m_mipSlices[sliceIndex].m_subresources)
                        {
                            // The copy destination is same for each subresource.
                            const uint32_t subresourceIdx = D3D12CalcSubresource(curMip, arraySlice, 0, imageMipLevels, arraySize);
                            CD3DX12_TEXTURE_COPY_LOCATION destLocation(imageMemory, subresourceIdx);

                            for (uint32_t depth = 0; depth < subresourceLayout.m_size.m_depth; depth++)
                            {
                                uint32_t startRow = 0;
                                uint32_t destHeight = 0;
                                while (startRow < subresourceLayout.m_rowCount)
                                {
                                    if (stagingSize > m_descriptor.m_stagingSizeInBytes - framePacket->m_dataOffset)
                                    {
                                        EndFramePacket(dx12CommandQueue);
                                        framePacket = BeginFramePacket();
                                    }

                                    const uint32_t endRow = AZStd::GetMin(startRow + rowsPerSplit, subresourceLayout.m_rowCount);

                                    // Calculate the blocksize for BC formatted images; the copy command works in texels.
                                    uint32_t heightToCopy = (endRow - startRow) * compressedTexelBlockSizeHeight;

                                    // Copy subresource data to staging memory
                                    {
                                        AZ_PROFILE_SCOPE(RHI, "Copy CPU image");
                                        for (uint32_t row = startRow; row < endRow; row++)
                                        {
                                            uint8_t* stagingDataStart = framePacket->m_stagingResourceData + framePacket->m_dataOffset;
                                            const uint8_t* subresourceSliceDataStart = static_cast<const uint8_t*>(subresource.m_data) + (depth * subresourceSlicePitch);

                                            memcpy(stagingDataStart + (row - startRow) * stagingRowPitch,
                                                subresourceSliceDataStart + row * subresourceLayout.m_bytesPerRow,
                                                subresourceLayout.m_bytesPerRow);
                                        }
                                    }

                                    //Clamp heightToCopy to match subresourceLayout.m_size.m_height as it is possible to go over
                                    //if subresourceLayout.m_size.m_height is not perfectly divisible by compressedTexelBlockSizeHeight
                                    if(destHeight+heightToCopy > subresourceLayout.m_size.m_height)
                                    {
                                        uint32_t HeightDiff = (destHeight + heightToCopy) - subresourceLayout.m_size.m_height;
                                        heightToCopy -= HeightDiff;
                                    }
                                    
                                    // Add copy command to copy image subresource from staging memory to image gpu resource

                                    // Source location
                                    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
                                    const DXGI_FORMAT imageFormat = GetBaseFormat(ConvertFormat(request.m_image->GetDescriptor().m_format));
                                    footprint.Footprint.Width = subresourceLayout.m_size.m_width;
                                    footprint.Footprint.Height = heightToCopy;
                                    footprint.Footprint.Depth = 1;
                                    footprint.Footprint.Format = imageFormat;
                                    footprint.Footprint.RowPitch = stagingRowPitch;
                                    footprint.Offset = framePacket->m_dataOffset;
                                    CD3DX12_TEXTURE_COPY_LOCATION sourceLocation(framePacket->m_stagingResource.get(), footprint);

                                    framePacket->m_commandList->CopyTextureRegion(
                                        &destLocation,
                                        0, destHeight, depth,
                                        &sourceLocation,
                                        nullptr);

                                    framePacket->m_dataOffset += stagingSize;
                                    startRow = endRow;
                                    destHeight += heightToCopy;
                                }
                            }
                            // Next slice in this array
                            arraySlice++;

                        }
                    }
                }

                EndFramePacket(dx12CommandQueue);

                dx12CommandQueue->Signal(m_uploadFence.Get(), fenceValue);

                if (request.m_completeCallback && !request.m_waitForUpload)
                {
                    {
                        AZStd::lock_guard<AZStd::mutex> lock(m_callbackMutex);
                        if (m_callbacks.size() > 0)
                        {
                            // The callbacks are added with the increasing order of fenceValue
                            // If this is not true, the ProcessCallbacks function need to updated.
                            AZ_Assert(m_callbacks.back().second < fenceValue, "Callbacks should be added with increasing order of fenceValue");
                        }
                        m_callbacks.push({ request.m_completeCallback, fenceValue });
                    }
                    AZ::SystemTickBus::QueueFunction([this] { ProcessCallbacks(uint64_t(-1)); });
                }

                return 0;
            }); // End m_copyQueue->QueueCommand

            if (request.m_waitForUpload)
            {
                m_uploadFence.Wait(m_uploadFenceEvent, fenceValue);
                if (request.m_completeCallback)
                {
                    request.m_completeCallback();
                }
            }
            return fenceValue;
        }

        bool AsyncUploadQueue::IsUploadFinished(uint64_t fenceValue)
        {
            return m_uploadFence.GetCompletedValue() >= fenceValue;
        }

        void AsyncUploadQueue::WaitForUpload(uint64_t fenceValue)
        {
            AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: WaitForUpload");

            if (!IsUploadFinished(fenceValue))
            {
                AZ_Assert(m_uploadFence.GetPendingValue() >= fenceValue, "Error: Attempting to wait for work that has not been encoded!");
                m_uploadFence.Wait(m_uploadFenceEvent, fenceValue);
            }

            // Process callbacks immediately
            ProcessCallbacks(fenceValue);
        }

        void AsyncUploadQueue::ProcessCallbacks(uint64_t fenceValue)
        {
            AZ_PROFILE_SCOPE(RHI, "AsyncUploadQueue: ProcessCallbacks");
            AZStd::lock_guard<AZStd::mutex> lock(m_callbackMutex);
            while (m_callbacks.size() > 0 && m_callbacks.front().second <= fenceValue)
            {
                AZStd::function<void()> callback = m_callbacks.front().first;
                m_callbacks.pop();
                callback();
            }
        }

        void AsyncUploadQueue::QueueTileMapping(CommandList::TileMapRequest& request)
        {
            m_copyQueue->QueueCommand([=](void* commandQueue)
            {
                AZ_PROFILE_SCOPE(RHI, "QueueTileMapping");

                ID3D12CommandQueue* dx12CommandQueue = static_cast<ID3D12CommandQueue*>(commandQueue);
                const uint32_t tileCount = request.m_sourceRegionSize.NumTiles;

                // DX12 requires that we pass the full array of range counts (even though they are all 1).
                if (m_rangeCounts.size() < tileCount)
                {
                    m_rangeCounts.resize(tileCount, 1);
                }

                // Same with range flags, except that we can skip if they are all 'None'.
                D3D12_TILE_RANGE_FLAGS* rangeFlags = nullptr;
                if (!request.m_destinationHeap)
                {
                    if (m_rangeFlags.size() < tileCount)
                    {
                        m_rangeFlags.resize(tileCount, D3D12_TILE_RANGE_FLAG_NULL);
                    }
                    rangeFlags = m_rangeFlags.data();
                }

                // Maps a single range of source tiles to N disjoint destination tiles on a heap (or null, if no heap is specified).
                dx12CommandQueue->UpdateTileMappings(
                    request.m_sourceMemory,
                    1, &request.m_sourceCoordinate, &request.m_sourceRegionSize,
                    request.m_destinationHeap,
                    tileCount, rangeFlags, request.m_destinationTileMap.data(), m_rangeCounts.data(),
                    D3D12_TILE_MAPPING_FLAG_NONE);
            });
        }
    }
}
