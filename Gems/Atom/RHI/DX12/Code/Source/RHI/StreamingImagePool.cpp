/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/StreamingImagePool.h>
#include <RHI/Conversions.h>
#include <RHI/CommandList.h>
#include <RHI/Device.h>
#include <RHI/Image.h>
#include <RHI/ResourcePoolResolver.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>

// enable debug output for DX12::StreamingImagePool
#define AZ_RHI_DX12_DEBUG_STREAMINGIMAGEPOOL 0

// enable tiled resource implementation
#define AZ_RHI_DX12_USE_TILED_RESOURCES

namespace AZ
{
    namespace DX12
    {

        // constants for heap page allocation 
        namespace
        {
            const static uint32_t TileSizeInBytes = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            const static uint32_t TileCountPerPage = 256;
        }

        // The StreamingImagePoolResolver adds streaming image transition barriers when scope starts
        // The streaming image transition barriers are added when image was initialized and image mip got expanded or trimmed.
        class StreamingImagePoolResolver
            : public ResourcePoolResolver
        {
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePoolResolver, AZ::SystemAllocator);
            AZ_RTTI(StreamingImagePoolResolver, "{C69BD5E1-15CD-4F60-A899-29E9DEDFA056}", ResourcePoolResolver);

            void QueuePrologueTransitionBarriers(CommandList& commandList) override
            {
                AZStd::shared_lock<AZStd::shared_mutex> lock(m_mutex);
                for (const auto& barrier : m_prologueBarriers)
                {
                    commandList.QueueTransitionBarrier(barrier.second);
                }
            }

            void Deactivate() override
            {
                AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);
                AZStd::for_each(m_prologueBarriers.begin(), m_prologueBarriers.end(), [](auto& barrier)
                {
                    Image* image = barrier.first;
                    AZ_Assert(image->m_pendingResolves, "There's no pending resolves for image %s", image->GetName().GetCStr());
                    image->m_pendingResolves--;
                });
                m_prologueBarriers.clear();
            }

            void AddImageTransitionBarrier(Image& image, uint32_t beforeMip, uint32_t afterMip)
            {
                AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);
                // Expand or Trim
                bool isExpand = beforeMip > afterMip;

                uint32_t lowResMip = isExpand ? beforeMip : afterMip;
                uint32_t highResMip = isExpand ? afterMip : beforeMip;
                
                uint32_t imageMipLevels = image.GetDescriptor().m_mipLevels;
                uint32_t arraySize = image.GetDescriptor().m_arraySize;
                AZStd::vector<uint32_t> subresourceIds;
                for (uint32_t curMip = highResMip; curMip < lowResMip; curMip++)
                {
                    for (uint32_t arrayIndex = 0; arrayIndex < arraySize; arrayIndex++)
                    {
                        uint32_t subresourceId = D3D12CalcSubresource(curMip, arrayIndex, 0, imageMipLevels, arraySize);
                        D3D12_RESOURCE_TRANSITION_BARRIER transition;
                        transition.pResource = image.GetMemoryView().GetMemory();
                        transition.Subresource = subresourceId;
                        // We don't update the "AttachmentState" of the image since Streaming Images are not used as attachments.
                        if (isExpand)
                        {
                            transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
                            transition.StateAfter = image.GetInitialResourceState();
                        }
                        else
                        {
                            transition.StateBefore = image.GetInitialResourceState();
                            transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
                        }
                        m_prologueBarriers.push_back(ImageBarrier{ &image, transition });
                        image.m_pendingResolves++;

#if AZ_RHI_DX12_DEBUG_STREAMINGIMAGEPOOL
                        AZ_TracePrintf("DX12 StreamingImagePool", "Add resource barrier for image [%s] [%d] expand: %d]\n", image.GetName().GetCStr(), subresourceId, isExpand);
#endif
                    }
                }
            }

            void OnResourceShutdown(const RHI::DeviceResource& resource) override
            {
                const Image& image = static_cast<const Image&>(resource);
                if (!image.m_pendingResolves)
                {
                    return;
                }

                auto predicateBarriers = [&image](const ImageBarrier& barrier)
                {
                    return barrier.first == &image;
                };

                AZStd::unique_lock<AZStd::shared_mutex> lock(m_mutex);
                m_prologueBarriers.erase(AZStd::remove_if(m_prologueBarriers.begin(), m_prologueBarriers.end(), predicateBarriers), m_prologueBarriers.end());
            }

        private:
            using ImageBarrier = AZStd::pair<Image*, D3D12_RESOURCE_TRANSITION_BARRIER>;
            AZStd::vector<ImageBarrier> m_prologueBarriers;
            AZStd::shared_mutex m_mutex;
        };

        RHI::Ptr<StreamingImagePool> StreamingImagePool::Create()
        {
            return aznew StreamingImagePool();
        }

        Device& StreamingImagePool::GetDevice() const
        {
            return static_cast<Device&>(Base::GetDevice());
        }

        StreamingImagePoolResolver* StreamingImagePool::GetResolver()
        {
            return static_cast<StreamingImagePoolResolver*>(Base::GetResolver());
        }

        RHI::HeapMemoryUsage& StreamingImagePool::GetDeviceHeapMemoryUsage()
        {
            return m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
        }

        RHI::ResultCode StreamingImagePool::InitInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::StreamingImagePoolDescriptor& descriptor)
        {
            AZ_PROFILE_FUNCTION(RHI);

            Device& device = static_cast<Device&>(deviceBase);

            m_enableTileResource = device.GetFeatures().m_tiledResource;

#ifndef AZ_RHI_DX12_USE_TILED_RESOURCES
            // Disable tile resource for all 
            m_enableTileResource = false;
#endif

            if (m_enableTileResource)
            {
                HeapAllocator::Descriptor heapPageAllocatorDesc;
                heapPageAllocatorDesc.m_device = &device;
                // Heap allocator updates total resident memory
                heapPageAllocatorDesc.m_getHeapMemoryUsageFunction = [this]() { return &GetDeviceHeapMemoryUsage();};
                heapPageAllocatorDesc.m_pageSizeInBytes = TileSizeInBytes * TileCountPerPage; // 16M per page, 256 tiles
                heapPageAllocatorDesc.m_resourceTypeFlags = ResourceTypeFlags::Image; 
                heapPageAllocatorDesc.m_heapMemoryLevel = RHI::HeapMemoryLevel::Device;
                heapPageAllocatorDesc.m_hostMemoryAccess = RHI::HostMemoryAccess::Write;
                heapPageAllocatorDesc.m_collectLatency = 0;
                heapPageAllocatorDesc.m_recycleOnCollect = false;    // Release the heap page when the TileAllocator deallocate it

                m_heapPageAllocator.Init(heapPageAllocatorDesc);

                TileAllocator::Descriptor tileAllocatorDesc;
                tileAllocatorDesc.m_tileSizeInBytes = TileSizeInBytes;
                // Tile allocator updates used resident memory
                tileAllocatorDesc.m_heapMemoryUsage = &GetDeviceHeapMemoryUsage();
                m_tileAllocator.Init(tileAllocatorDesc, m_heapPageAllocator);

                // Allocate one tile for default tile
                auto heapTiles = m_tileAllocator.Allocate(1);
                AZ_Assert(heapTiles.size() == 1, "Failed to allocate a default heap");
                m_defaultTile = heapTiles[0];
            }

            SetResolver(AZStd::make_unique<StreamingImagePoolResolver>());
            return RHI::ResultCode::Success;
        }

        void StreamingImagePool::ShutdownInternal()
        {
            if (m_enableTileResource)
            {
                m_tileAllocator.DeAllocate(AZStd::vector<HeapTiles>{m_defaultTile});
                m_defaultTile = HeapTiles{ };
                m_tileAllocator.Shutdown();
            }
        }

        void StreamingImagePool::AllocateImageTilesInternal(Image& image, uint32_t subresourceIndex)
        {
            uint32_t imageTileOffset = 0;
            CommandList::TileMapRequest request;
            request.m_sourceMemory = image.GetMemoryView().GetMemory();
            image.m_tileLayout.GetSubresourceTileInfo(subresourceIndex, imageTileOffset, request.m_sourceCoordinate, request.m_sourceRegionSize);

            AZ_Assert(image.m_heapTiles[subresourceIndex].size() == 0, "Stopping on an existing tile allocation. This will leak.");

            uint32_t totalTiles = request.m_sourceRegionSize.NumTiles;

            // protect access to m_tileAllocator
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_tileMutex);

                // Check if heap memory is enough for the tiles.
                RHI::HeapMemoryUsage& memoryAllocatorUsage = GetDeviceHeapMemoryUsage();
                size_t pageAllocationInBytes = m_tileAllocator.EvaluateMemoryAllocation(totalTiles);

                // Try to release some memory if there isn't enough memory available in the pool
                bool canAllocate = memoryAllocatorUsage.CanAllocate(pageAllocationInBytes);
                if (!canAllocate && m_memoryReleaseCallback)
                {
                    // only try to release tiles the resource need
                    uint32_t maxUsedTiles = m_tileAllocator.GetTotalTileCount() - totalTiles;
                    bool releaseSuccess = m_memoryReleaseCallback(maxUsedTiles * m_tileAllocator.GetDescriptor().m_tileSizeInBytes);

                    if (!releaseSuccess)
                    {
                        AZ_Warning(
                            "DX12::StreamingImagePool",
                            false,
                            "There isn't enough memory to allocate the image [%s]'s subresource %d. "
                            "Using the default tile for the subresource. Try increase the StreamingImagePool memory budget",
                            image.GetName().GetCStr(),
                            subresourceIndex);
                    }
                }

                image.m_heapTiles[subresourceIndex] = m_tileAllocator.Allocate(totalTiles);
            } // Unlock m_tileMutex.

            // If it failed to allocate tiles, use default tile for the sub-resource
            if (image.m_heapTiles[subresourceIndex].size() == 0)
            {
                request.m_rangeFlags.resize(1);
                request.m_rangeStartOffsets.resize(1);
                request.m_rangeTileCounts.resize(1);
                request.m_destinationHeap = m_defaultTile.m_heap.get();
                
                request.m_rangeFlags[0] = D3D12_TILE_RANGE_FLAG_REUSE_SINGLE_TILE;
                request.m_rangeStartOffsets[0] = 0;
                request.m_rangeTileCounts[0] = totalTiles;

                GetDevice().GetAsyncUploadQueue().QueueTileMapping(request);

                return;
            }

            // If the allocated tiles are spread across multiple heaps, we need one TileMapRequest for each heap.
            // In the TileMapRequest for the heap, it maps the tiles from the heap to a subset tiles of the subresource
            // and set the unmapped subresource tiles as skip.
            // Note: the mapped subset of tiles of the subresource will always be continues. So the skip range could only happen
            // in the front part of subresource tiles or the back part of them. 
            bool needSkipRange = image.m_heapTiles[subresourceIndex].size() > 1;

            uint32_t tileOffsetStart = 0;
            // send a tile map request for each heap
            for (const HeapTiles& heapTiles : image.m_heapTiles[subresourceIndex])
            {
                size_t rangeCount = heapTiles.m_tileSpanList.size();
                uint32_t startRangeIndex = 0;

                if (needSkipRange)
                {
                    if (tileOffsetStart == 0 || tileOffsetStart + heapTiles.m_totalTileCount == totalTiles)
                    {
                        // For the first heap, one extra range will indicate the subsequent subresource tiles that are not mapped in that heap.
                        // For the last heap, one extra range will indicate the preceding subresource tiles that are not mapped in that heap.
                        rangeCount += 1;
                    }
                    else
                    {
                        // For all other heaps, two extra ranges will indicate both the preceding and subsequent subresource tiles that are not mapped in that heap.
                        rangeCount += 2;
                    }

                    if (tileOffsetStart != 0)
                    {
                        startRangeIndex = 1;
                    }
                }

                request.m_rangeFlags.resize(rangeCount);
                request.m_rangeStartOffsets.resize(rangeCount);
                request.m_rangeTileCounts.resize(rangeCount);
                request.m_destinationHeap = heapTiles.m_heap.get();

                // skip tiles which are not mapped by current heap
                // [tileOffsetStart, tileOffsetStart + heapTiles.m_totalTileCount) 
                if (needSkipRange)
                {
                    // from 0 to current start tile
                    if (tileOffsetStart != 0)
                    {
                        request.m_rangeFlags[0] = D3D12_TILE_RANGE_FLAG_SKIP;
                        request.m_rangeStartOffsets[0] = 0;
                        request.m_rangeTileCounts[0] = tileOffsetStart;
                    }

                    if (tileOffsetStart + heapTiles.m_totalTileCount != totalTiles)
                    {
                        // from the last tile the current heap tiles mapped to the end
                        size_t lastIndex = rangeCount-1;
                        request.m_rangeFlags[lastIndex] = D3D12_TILE_RANGE_FLAG_SKIP;
                        request.m_rangeStartOffsets[lastIndex] = tileOffsetStart + heapTiles.m_totalTileCount;
                        request.m_rangeTileCounts[lastIndex] = totalTiles - request.m_rangeStartOffsets[lastIndex];
                    }
                }

                uint32_t rangeIdx = startRangeIndex;
                for (const auto& tiles : heapTiles.m_tileSpanList)
                {
                    request.m_rangeFlags[rangeIdx] = D3D12_TILE_RANGE_FLAG_NONE;
                    request.m_rangeStartOffsets[rangeIdx] = tiles.m_offset;
                    request.m_rangeTileCounts[rangeIdx] = tiles.m_tileCount;
                    rangeIdx++;
                }

                tileOffsetStart += heapTiles.m_totalTileCount;
                GetDevice().GetAsyncUploadQueue().QueueTileMapping(request);
            }
        }

        void StreamingImagePool::DeAllocateImageTilesInternal(Image& image, uint32_t subresourceIndex)
        {
            const AZStd::vector<HeapTiles>& heapTilesList = image.m_heapTiles[subresourceIndex];

            // The tile list can be empty if the subresource was using the default tile
            if (heapTilesList.size() == 0)
            {
                return;
            }

            // map all the tiles to NULL
            CommandList::TileMapRequest request;
            uint32_t imageTileOffset = 0;
            request.m_sourceMemory = image.GetMemoryView().GetMemory();
            image.m_tileLayout.GetSubresourceTileInfo(subresourceIndex, imageTileOffset, request.m_sourceCoordinate, request.m_sourceRegionSize);
            GetDevice().GetAsyncUploadQueue().QueueTileMapping(request);

            {
                AZStd::lock_guard<AZStd::mutex> lock(m_tileMutex);

                // deallocate tiles and update image's sub-resource info
                m_tileAllocator.DeAllocate(heapTilesList);
                image.m_heapTiles[subresourceIndex] = {};

                // Garbage collect the allocator immediately.
                m_tileAllocator.GarbageCollect();
            }
        }

        void StreamingImagePool::AllocatePackedImageTiles(Image& image)
        {
            AZ_PROFILE_FUNCTION(RHI);

            AZ_Assert(image.IsTiled(), "This method is only valid for tiled resources.");

            const ImageTileLayout& tileLayout = image.m_tileLayout;
            if (tileLayout.m_mipCountPacked)
            {
                AllocateImageTilesInternal(image, tileLayout.GetPackedSubresourceIndex());
                image.UpdateResidentTilesSizeInBytes(TileSizeInBytes);
            }
        }

        void StreamingImagePool::AllocateStandardImageTiles(Image& image, RHI::Interval mipInterval)
        {
            AZ_Assert(image.IsTiled(), "This method is only valid for tiled resources.");

            const RHI::ImageDescriptor& descriptor = image.GetDescriptor();
            const ImageTileLayout& tileLayout = image.m_tileLayout;

            // Clamp the mip chain to last standard mip. Packed mips are persistently mapped.
            mipInterval.m_min = AZStd::min(mipInterval.m_min, tileLayout.m_mipCountStandard);
            mipInterval.m_max = AZStd::min(mipInterval.m_max, tileLayout.m_mipCountStandard);

            // Only proceed if the interval is still valid.
            if (mipInterval.m_min < mipInterval.m_max)
            {
                for (uint32_t arrayIndex = 0; arrayIndex < descriptor.m_arraySize; ++arrayIndex)
                {
                    for (uint32_t mipIndex = mipInterval.m_min; mipIndex < mipInterval.m_max; ++mipIndex)
                    {
                        AllocateImageTilesInternal(image, RHI::GetImageSubresourceIndex(mipIndex, arrayIndex, descriptor.m_mipLevels));
                    }
                }
                
                image.UpdateResidentTilesSizeInBytes(TileSizeInBytes);
            }
        }

        void StreamingImagePool::DeAllocateStandardImageTiles(Image& image, RHI::Interval mipInterval)
        {
            AZ_Assert(image.IsTiled(), "This method is only valid for tiled resources.");

            const RHI::ImageDescriptor& descriptor = image.GetDescriptor();
            const ImageTileLayout& tileLayout = image.m_tileLayout;

            // Clamp the mip chain to last standard mip. Packed mips are persistently mapped.
            mipInterval.m_min = AZStd::min(mipInterval.m_min, tileLayout.m_mipCountStandard);
            mipInterval.m_max = AZStd::min(mipInterval.m_max, tileLayout.m_mipCountStandard);

            // Only proceed if the interval is still valid.
            if (mipInterval.m_min < mipInterval.m_max)
            {
                // add wait frame fence to async upload queue before queue tile mapping
                auto& device = static_cast<Device&>(GetDevice());
                CommandQueueContext& context = device.GetCommandQueueContext();
                const FenceSet &compiledFences = context.GetFrameFences(context.GetLastFrameIndex());
                const Fence& fence = compiledFences.GetFence(RHI::HardwareQueueClass::Graphics);
                GetDevice().GetAsyncUploadQueue().QueueWaitFence(fence, fence.GetPendingValue());

                for (uint32_t arrayIndex = 0; arrayIndex < descriptor.m_arraySize; ++arrayIndex)
                {
                    for (uint32_t mipIndex = mipInterval.m_min; mipIndex < mipInterval.m_max; ++mipIndex)
                    {
                        DeAllocateImageTilesInternal(image, RHI::GetImageSubresourceIndex(mipIndex, arrayIndex, descriptor.m_mipLevels));
                    }
                }
                image.UpdateResidentTilesSizeInBytes(TileSizeInBytes);
            }
        }

        bool StreamingImagePool::ShouldUseTileHeap(const RHI::ImageDescriptor& imageDescriptor) const
        {
            if (m_enableTileResource)
            {
                // D3D12_RESOURCE_DIMENSION_TEXTURE1D is not supported for tier 1 tile image resource
                if (imageDescriptor.m_dimension == RHI::ImageDimension::Image1D)
                {
                    return false;
                }

                // ID3D12Device::CreateReservedResource limitation
                // D3D12 ERROR: ID3D12Device::CreateReservedResource: On a device with Tier 2 & 3 Tiled Resources support, Tiled Resources cannot be created
                // with both more than one array slice and any mipmap that has a dimension less than a tile in extent.
                if (imageDescriptor.m_arraySize > 1)
                {
                    // get smallest mip size
                    RHI::Size formatDimensionAlignment = RHI::GetFormatDimensionAlignment(imageDescriptor.m_format);
                    uint32_t minMipWidth = AZStd::max(imageDescriptor.m_size.m_width >> (imageDescriptor.m_mipLevels-1), 1u);
                    uint32_t minMipHeight = AZStd::max(imageDescriptor.m_size.m_height >> (imageDescriptor.m_mipLevels-1), 1u);
                    uint32_t minMipSize =
                        AZ::DivideAndRoundUp(minMipWidth, formatDimensionAlignment.m_width) *
                        AZ::DivideAndRoundUp(minMipHeight, formatDimensionAlignment.m_height) *
                        RHI::GetFormatSize(imageDescriptor.m_format);
                    if (minMipSize < TileSizeInBytes)
                    {
                        return false;
                    }
                }

                return true;
            }
            return false;
        }

        RHI::ResultCode StreamingImagePool::InitImageInternal(const RHI::DeviceStreamingImageInitRequest& request)
        {
            AZ_PROFILE_FUNCTION(RHI);

            Image& image = static_cast<Image&>(*request.m_image);

            // Decide if we use tile heap for the image. It may effect allocation and memory usage.
            bool useTileHeap = ShouldUseTileHeap(image.GetDescriptor());

            MemoryView memoryView;

            if (useTileHeap)
            {
                // Note, the heap memory usage for reserved image are updated by the HeapAllocator and TileAllocator
                memoryView = GetDevice().CreateImageReserved(request.m_descriptor, D3D12_RESOURCE_STATE_COMMON, image.m_tileLayout);
                if (!memoryView.IsValid())
                {
                    // fall back to use non-tiled resource
                    useTileHeap = false;
                }
            }

            if (!useTileHeap)
            {
                // The committed image would allocate heap from the entire image
                // We would only need to update memory usage once at creating time and when resource is shutdown
                D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
                GetDevice().GetImageAllocationInfo(request.m_descriptor, allocationInfo);

                auto& memoryAllocatorUsage = GetDeviceHeapMemoryUsage();

                memoryView = GetDevice().CreateImageCommitted(request.m_descriptor, nullptr, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT);
                
                // Ensure the driver was able to make the allocation.
                if (!memoryView.IsValid())
                {
                    return RHI::ResultCode::Fail;
                }
                else
                {
                    // Update memory usage for committed resource
                    memoryAllocatorUsage.m_totalResidentInBytes += allocationInfo.SizeInBytes;
                    memoryAllocatorUsage.m_usedResidentInBytes += allocationInfo.SizeInBytes;
                    image.m_residentSizeInBytes = allocationInfo.SizeInBytes;
                }
            }

            memoryView.SetName(image.GetName().GetStringView());
            image.m_memoryView = AZStd::move(memoryView);
            image.GenerateSubresourceLayouts();
            image.m_streamedMipLevel = request.m_descriptor.m_mipLevels - static_cast<uint32_t>(request.m_tailMipSlices.size());

            // allocate tiles from heaps for reserved images
            if (useTileHeap)
            {
                // Allocate packed tiles for tail mips which are packed
                AllocatePackedImageTiles(image);

                // Allocate standard tile for mips from tail mip chain which are not included in packed tile
                const ImageTileLayout& tileLayout = image.m_tileLayout;
                if (tileLayout.m_mipCountPacked < static_cast<uint32_t>(request.m_tailMipSlices.size()))
                {
                    AllocateStandardImageTiles(image, RHI::Interval{ image.m_streamedMipLevel, request.m_descriptor.m_mipLevels - tileLayout.m_mipCountPacked });
                }
            }

            // update image's minimum resident size
            image.m_minimumResidentSizeInBytes = image.m_residentSizeInBytes;

            // Queue upload tail mip slices
            RHI::DeviceStreamingImageExpandRequest uploadMipRequest;
            uploadMipRequest.m_image = request.m_image;
            uploadMipRequest.m_mipSlices = request.m_tailMipSlices;
            uploadMipRequest.m_waitForUpload = true;
            GetDevice().GetAsyncUploadQueue().QueueUpload(uploadMipRequest, request.m_descriptor.m_mipLevels);

            GetResolver()->AddImageTransitionBarrier(image, request.m_descriptor.m_mipLevels, image.m_streamedMipLevel);

            return RHI::ResultCode::Success;
        }

        void StreamingImagePool::ShutdownResourceInternal(RHI::DeviceResource& resourceBase)
        {
            Image& image = static_cast<Image&>(resourceBase);

            // Wait for any upload of this image done.
            WaitFinishUploading(image);

            if (auto* resolver = GetResolver())
            {
                resolver->OnResourceShutdown(resourceBase);
            }

            if (image.IsTiled())
            {
                m_tileMutex.lock();                

                for (const auto& heapTiles : image.m_heapTiles)
                {
                    m_tileAllocator.DeAllocate(heapTiles.second);
                }
                m_tileAllocator.GarbageCollect();
                m_tileMutex.unlock();
                image.m_heapTiles.clear();
                image.m_tileLayout = ImageTileLayout();
            }
            else
            {
                auto& memoryAllocatorUsage = GetDeviceHeapMemoryUsage();
                memoryAllocatorUsage.m_totalResidentInBytes -= image.m_residentSizeInBytes;
                memoryAllocatorUsage.m_usedResidentInBytes -= image.m_residentSizeInBytes;
                memoryAllocatorUsage.Validate();
            }

            GetDevice().QueueForRelease(image.m_memoryView);
            image.m_memoryView = MemoryView();
            image.m_pendingResolves = 0;
        }

        RHI::ResultCode StreamingImagePool::ExpandImageInternal(const RHI::DeviceStreamingImageExpandRequest& request)
        {
            Image& image = static_cast<Image&>(*request.m_image);

            // Wait for any upload of this image done.
            WaitFinishUploading(image);

            const uint32_t residentMipLevelBefore = image.GetResidentMipLevel();
            const uint32_t residentMipLevelAfter = residentMipLevelBefore - static_cast<uint32_t>(request.m_mipSlices.size());

            if (image.IsTiled())
            {
                AllocateStandardImageTiles(image, RHI::Interval{ residentMipLevelAfter, residentMipLevelBefore });
            }

            // Create new expend request and append callback from the StreamingImagePool
            RHI::DeviceStreamingImageExpandRequest newRequest = request;
            newRequest.m_completeCallback = [=]() 
            {
                Image& dxImage = static_cast<Image&>(*request.m_image);
                dxImage.FinalizeAsyncUpload(residentMipLevelAfter);
                request.m_completeCallback();
                GetResolver()->AddImageTransitionBarrier(dxImage, residentMipLevelBefore, residentMipLevelAfter);
                
#if AZ_RHI_DX12_DEBUG_STREAMINGIMAGEPOOL
                AZ_TracePrintf("DX12 StreamingImagePool", "Image upload complete [%s]\n", request.m_image->GetName().GetCStr());
#endif
            };

            GetDevice().GetAsyncUploadQueue().QueueUpload(newRequest, residentMipLevelBefore);

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::TrimImageInternal(RHI::DeviceImage& image, uint32_t targetMipLevel)
        {
            Image& imageImpl = static_cast<Image&>(image);

            // Wait for any upload of this image done. 
            WaitFinishUploading(imageImpl);

            // Set streamed mip level to target mip level
            if (imageImpl.GetStreamedMipLevel() < targetMipLevel)
            {
                imageImpl.SetStreamedMipLevel(targetMipLevel);
            }

            const uint32_t residentMipLevelBefore = image.GetResidentMipLevel();

            if (imageImpl.IsTiled())
            {
                DeAllocateStandardImageTiles(imageImpl, RHI::Interval{ residentMipLevelBefore, targetMipLevel});
            }

#if AZ_RHI_DX12_DEBUG_STREAMINGIMAGEPOOL
            AZ_TracePrintf("DX12 StreamingImagePool", "Image mips were trimmer from %d to %d\n", residentMipLevelBefore, targetMipLevel);
#endif

            GetResolver()->AddImageTransitionBarrier(imageImpl, residentMipLevelBefore, targetMipLevel);

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::SetMemoryBudgetInternal(size_t newBudget)
        {
            RHI::HeapMemoryUsage& heapMemoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);

            if (newBudget == 0)
            {
                heapMemoryUsage.m_budgetInBytes = newBudget;
                return RHI::ResultCode::Success;
            }

            bool releaseSuccess = true;
            // If the new budget is smaller than the memory are in use, we need to release some memory
            if (newBudget < heapMemoryUsage.m_usedResidentInBytes)
            {
                releaseSuccess = m_memoryReleaseCallback(newBudget);
            }

            size_t resident = heapMemoryUsage.m_usedResidentInBytes;
            if (!releaseSuccess)
            {
                heapMemoryUsage.m_budgetInBytes = resident;
                AZ_Warning("StreamingImagePool", false, "Failed to set pool memory budget to %u, set to %u instead", newBudget, heapMemoryUsage.m_budgetInBytes);
            }
            else
            {
                heapMemoryUsage.m_budgetInBytes = newBudget;
            }
            return RHI::ResultCode::Success;
        }

        bool StreamingImagePool::SupportTiledImageInternal() const
        {
            return m_enableTileResource;
        }

        void StreamingImagePool::WaitFinishUploading(const Image& image)
        {
            GetDevice().GetAsyncUploadQueue().WaitForUpload(image.GetUploadFenceValue());
        }
    }
}

