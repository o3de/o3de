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

// NOTE: Tiled resources are currently disabled, because RenderDoc does not support them. (this may not be the case anymore)
#define AZ_RHI_USE_TILED_RESOURCES

namespace AZ
{
    namespace DX12
    {

        // constants for heap page allocation 
        namespace
        {
            const static int MaxTextureResolution = 8*1024;
            // Maxinum page size should be able to accommodate one slice of texture with maxinum size
            const static int MaxPageSize = MaxTextureResolution*MaxTextureResolution*4*4; //4 channel 4 bytes/channel
        }

        // The StreamingImagePoolResolver adds streaming image transition barriers when scope starts
        // The streaming image transition barriers are added when image was initlized and image mip got expanded or trimmed.
        class StreamingImagePoolResolver
            : public ResourcePoolResolver
        {
        public:
            AZ_CLASS_ALLOCATOR(StreamingImagePoolResolver, AZ::SystemAllocator, 0);
            AZ_RTTI(StreamingImagePoolResolver, "{C69BD5E1-15CD-4F60-A899-29E9DEDFA056}", ResourcePoolResolver);

            void QueuePrologueTransitionBarriers(CommandList& commandList) const override
            {
                for (const auto& barrier : m_prologueBarriers)
                {
                    commandList.QueueTransitionBarrier(barrier.second);
                }
            }

            void Deactivate() override
            {
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
                    }
                }
            }

            void OnResourceShutdown(const RHI::Resource& resource) override
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

                m_prologueBarriers.erase(AZStd::remove_if(m_prologueBarriers.begin(), m_prologueBarriers.end(), predicateBarriers), m_prologueBarriers.end());
            }

        private:
            using ImageBarrier = AZStd::pair<Image*, D3D12_RESOURCE_TRANSITION_BARRIER>;
            AZStd::vector<ImageBarrier> m_prologueBarriers;
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

        D3D12_RESOURCE_ALLOCATION_INFO StreamingImagePool::GetAllocationInfo(const RHI::ImageDescriptor& imageDescriptor, uint32_t residentMipLevel)
        {
            AZ_PROFILE_FUNCTION(RHI);

            uint32_t alignment = GetFormatDimensionAlignment(imageDescriptor.m_format);

            RHI::ImageDescriptor residentImageDescriptor = imageDescriptor;
            residentImageDescriptor.m_size = imageDescriptor.m_size.GetReducedMip(residentMipLevel);
            residentImageDescriptor.m_size.m_width = RHI::AlignUp(residentImageDescriptor.m_size.m_width, alignment);
            residentImageDescriptor.m_size.m_height = RHI::AlignUp(residentImageDescriptor.m_size.m_height, alignment);
            residentImageDescriptor.m_mipLevels = static_cast<uint16_t>(imageDescriptor.m_mipLevels - residentMipLevel);

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo;
            GetDevice().GetImageAllocationInfo(residentImageDescriptor, allocationInfo);
            return allocationInfo;
        }

        RHI::ResultCode StreamingImagePool::InitInternal([[maybe_unused]] RHI::Device& deviceBase, [[maybe_unused]] const RHI::StreamingImagePoolDescriptor& descriptor)
        {
            AZ_PROFILE_FUNCTION(RHI);

#ifdef AZ_RHI_USE_TILED_RESOURCES
            {
                Device& device = static_cast<Device&>(deviceBase);
                
                m_memoryAllocatorUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
                
                HeapAllocator::Descriptor heapPageAllocatorDesc;
                heapPageAllocatorDesc.m_device = &device;
                heapPageAllocatorDesc.m_pageSizeInBytes = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT * 256; // 16M per page, 256 tiles
                heapPageAllocatorDesc.m_getHeapMemoryUsageFunction = [this]() { return &m_memoryAllocatorUsage;};
                heapPageAllocatorDesc.m_resourceTypeFlags = ImageResourceTypeFlags::Image;
                heapPageAllocatorDesc.m_collectLatency = 0;

                m_heapPageAllocator.Init(heapPageAllocatorDesc);

                TileAllocator::Descriptor tileAllocatorDesc;
                tileAllocatorDesc.m_tileSizeInBytes = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
                m_tileAllocator.Init(tileAllocatorDesc, m_heapPageAllocator);
            }
#endif

            SetResolver(AZStd::make_unique<StreamingImagePoolResolver>());
            return RHI::ResultCode::Success;
        }

        void StreamingImagePool::ShutdownInternal()
        {
#ifdef AZ_RHI_USE_TILED_RESOURCES
            m_tileAllocator.Shutdown();
#endif
        }

        void StreamingImagePool::AllocateImageTilesInternal(Image& image, uint32_t subresourceIndex)
        {
            uint32_t imageTileOffset = 0;
            CommandList::TileMapRequest request;
            request.m_sourceMemory = image.GetMemoryView().GetMemory();
            image.m_tileLayout.GetSubresourceTileInfo(subresourceIndex, imageTileOffset, request.m_sourceCoordinate, request.m_sourceRegionSize);

            AZ_Assert(image.m_heapTiles[subresourceIndex].size() == 0, "Stoping on an existing tile allocation. This will leak.");

            uint32_t totalTiles = request.m_sourceRegionSize.NumTiles;
            image.m_heapTiles[subresourceIndex] = m_tileAllocator.Allocate(totalTiles);

            // send tile map request for each heap
            bool needSkipRange = image.m_heapTiles[subresourceIndex].size() > 1;
            uint32_t tileOffsetStart = 0;
            for (auto& heapTiles : image.m_heapTiles[subresourceIndex])
            {
                size_t rangeCount = heapTiles.m_tilesList.size();
                uint32_t startRangeIndex = 0;
                if (needSkipRange)
                {
                    rangeCount += 2;
                    startRangeIndex = 1;
                }

                request.m_rangeFlags.resize(rangeCount);
                request.m_rangeStartOffsets.resize(rangeCount);
                request.m_rangeTileCounts.resize(rangeCount);
                request.m_destinationHeap = heapTiles.m_heap.get();

                // skip tiles which are not mapped by current heap
                if (needSkipRange)
                {
                    request.m_rangeFlags[0] = D3D12_TILE_RANGE_FLAG_SKIP;
                    request.m_rangeStartOffsets[0] = 0;
                    request.m_rangeTileCounts[0] = tileOffsetStart;

                    size_t lastIndex = rangeCount-1;
                    request.m_rangeFlags[lastIndex] = D3D12_TILE_RANGE_FLAG_SKIP;
                    request.m_rangeStartOffsets[lastIndex] = tileOffsetStart;
                    request.m_rangeTileCounts[lastIndex] = totalTiles - tileOffsetStart;
                }

                uint32_t rangeIdx = startRangeIndex;
                for (auto& tiles : heapTiles.m_tilesList)
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
            auto& heapTiles = image.m_heapTiles[subresourceIndex];
            AZ_Assert(heapTiles.size() > 0, "Attempting to map a tile to null when it's already null.");

            // map all the tiles to NULL
            CommandList::TileMapRequest request;
            uint32_t imageTileOffset = 0;
            request.m_sourceMemory = image.GetMemoryView().GetMemory();
            image.m_tileLayout.GetSubresourceTileInfo(subresourceIndex, imageTileOffset, request.m_sourceCoordinate, request.m_sourceRegionSize);
            GetDevice().GetAsyncUploadQueue().QueueTileMapping(request);

            // deallocate tiles and update image's sub-resource info
            m_tileAllocator.DeAllocate(heapTiles);
            image.m_heapTiles[subresourceIndex] = {};

            // Garbage collect the allocator immediately.
            m_tileAllocator.GarbageCollect();
        }

        void StreamingImagePool::AllocatePackedImageTiles(Image& image)
        {
            AZ_PROFILE_FUNCTION(RHI);

            AZ_Assert(image.IsTiled(), "This method is only valid for tiled resources.");
            AZ_Assert(image.GetDescriptor().m_arraySize == 1, "Not implemented for image arrays.");

            const ImageTileLayout& tileLayout = image.m_tileLayout;
            if (tileLayout.m_mipCountPacked)
            {
                AllocateImageTilesInternal(image, tileLayout.GetPackedSubresourceIndex());
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
                AZStd::lock_guard<AZStd::mutex> lock(m_tileMutex);
                for (uint32_t arrayIndex = 0; arrayIndex < descriptor.m_arraySize; ++arrayIndex)
                {
                    for (uint32_t mipIndex = mipInterval.m_min; mipIndex < mipInterval.m_max; ++mipIndex)
                    {
                        AllocateImageTilesInternal(image, RHI::GetImageSubresourceIndex(mipIndex, arrayIndex, descriptor.m_mipLevels));
                    }
                }
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
                AZStd::lock_guard<AZStd::mutex> lock(m_tileMutex);
                for (uint32_t arrayIndex = 0; arrayIndex < descriptor.m_arraySize; ++arrayIndex)
                {
                    for (uint32_t mipIndex = mipInterval.m_min; mipIndex < mipInterval.m_max; ++mipIndex)
                    {
                        DeAllocateImageTilesInternal(image, RHI::GetImageSubresourceIndex(mipIndex, arrayIndex, descriptor.m_mipLevels));
                    }
                }
            }
        }
        

        RHI::ResultCode StreamingImagePool::InitImageInternal(const RHI::StreamingImageInitRequest& request)
        {
            AZ_PROFILE_FUNCTION(RHI);

            Image& image = static_cast<Image&>(*request.m_image);

            uint32_t expectedResidentMipLevel = request.m_descriptor.m_mipLevels - static_cast<uint32_t>(request.m_tailMipSlices.size());
            D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = GetAllocationInfo(request.m_descriptor, expectedResidentMipLevel);

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            if (!memoryUsage.TryReserveMemory(allocationInfo.SizeInBytes))
            {
                return RHI::ResultCode::OutOfMemory;
            }

            #if defined (AZ_RHI_USE_TILED_RESOURCES)
                /**
                 * DirectX has weird limitation in the implementation where texture arrays are not currently supported
                 * (despite the spec saying they are):
                 *
                 * 'On a device with Tier 2 Tiled Resources support, Tiled Resources cannot be created with both more
                 *  than one array slice and any mipmap that has a dimension less than a tile in extent. 
                 *  Hardware support in this area was not able to be standardized in time to be included in D3D'
                 *
                 * Need to follow up to figure out if this is a permanent limitation or whether it will be lifted in an
                 * upcoming version of windows. For now, a committed resource is created when texture arrays are in use.
                 */
                const bool useTileHeap = request.m_descriptor.m_arraySize == 1;
            #else
                const bool useTileHeap = false;
            #endif

            MemoryView memoryView;
            if (useTileHeap)
            {
                memoryView = GetDevice().CreateImageReserved(
                    request.m_descriptor, D3D12_RESOURCE_STATE_COMMON, image.m_tileLayout);
            }
            else
            {
                memoryView = GetDevice().CreateImageCommitted(
                    request.m_descriptor, nullptr, D3D12_RESOURCE_STATE_COMMON, D3D12_HEAP_TYPE_DEFAULT);
            }

            // Ensure the driver was able to make the allocation.
            if (!memoryView.IsValid())
            {
                memoryUsage.m_reservedInBytes -= allocationInfo.SizeInBytes;
                return RHI::ResultCode::OutOfMemory;
            }

            memoryView.SetName(image.GetName().GetStringView());

            image.m_residentSizeInBytes = allocationInfo.SizeInBytes;
            image.m_memoryView = AZStd::move(memoryView);
            image.GenerateSubresourceLayouts();
            image.m_streamedMipLevel = request.m_descriptor.m_mipLevels - static_cast<uint32_t>(request.m_tailMipSlices.size());

            if (useTileHeap)
            {
                AllocatePackedImageTiles(image);
                // Allocate standard tile for mips from tail mipchain which are not included in packed tile
                const ImageTileLayout& tileLayout = image.m_tileLayout;
                if (tileLayout.m_mipCountPacked < static_cast<uint32_t>(request.m_tailMipSlices.size()))
                {
                    AllocateStandardImageTiles(image, RHI::Interval{ image.m_streamedMipLevel, request.m_descriptor.m_mipLevels - tileLayout.m_mipCountPacked });
                }
            }

            // Queue upload tail mip slices
            RHI::StreamingImageExpandRequest uploadMipRequest;
            uploadMipRequest.m_image = &image;
            uploadMipRequest.m_mipSlices = request.m_tailMipSlices;
            uploadMipRequest.m_waitForUpload = true;
            GetDevice().GetAsyncUploadQueue().QueueUpload(uploadMipRequest, request.m_descriptor.m_mipLevels);

            memoryUsage.m_residentInBytes += allocationInfo.SizeInBytes;
            memoryUsage.Validate();

            GetResolver()->AddImageTransitionBarrier(image, request.m_descriptor.m_mipLevels, image.m_streamedMipLevel);

            return RHI::ResultCode::Success;
        }

        void StreamingImagePool::ShutdownResourceInternal(RHI::Resource& resourceBase)
        {
            Image& image = static_cast<Image&>(resourceBase);

            // Wait for any upload of this image done. 
            GetDevice().GetAsyncUploadQueue().WaitForUpload(image.GetUploadFenceValue());

            if (auto* resolver = GetResolver())
            {
                resolver->OnResourceShutdown(resourceBase);
            }


            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            memoryUsage.m_residentInBytes -= image.m_residentSizeInBytes;
            memoryUsage.m_reservedInBytes -= image.m_residentSizeInBytes;
            memoryUsage.Validate();

            if (image.IsTiled())
            {
                m_tileMutex.lock();
                for (auto& heapTiles : image.m_heapTiles)
                {
                    m_tileAllocator.DeAllocate(heapTiles.second);
                }
                m_tileAllocator.GarbageCollect();
                m_tileMutex.unlock();
                image.m_heapTiles.clear();
                image.m_tileLayout = ImageTileLayout();
            }

            GetDevice().QueueForRelease(image.m_memoryView);
            image.m_memoryView = MemoryView();
            image.m_pendingResolves = 0;
        }

        RHI::ResultCode StreamingImagePool::ExpandImageInternal(const RHI::StreamingImageExpandRequest& request)
        {
            Image& image = static_cast<Image&>(*request.m_image);

            const uint32_t residentMipLevelBefore = image.GetResidentMipLevel();
            const uint32_t residentMipLevelAfter = residentMipLevelBefore - static_cast<uint32_t>(request.m_mipSlices.size());
            D3D12_RESOURCE_ALLOCATION_INFO allocationInfoAfter = GetAllocationInfo(image.GetDescriptor(), residentMipLevelAfter);

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            const size_t imageSizeBefore = image.m_residentSizeInBytes;
            const size_t imageSizeAfter = allocationInfoAfter.SizeInBytes;
            
            // Try reserve memory for the increased size.
            if (!memoryUsage.TryReserveMemory(imageSizeAfter - imageSizeBefore))
            {
                return RHI::ResultCode::OutOfMemory;
            }

            if (image.IsTiled())
            {
                AllocateStandardImageTiles(image, RHI::Interval{ residentMipLevelAfter, residentMipLevelBefore });
            }

            // Update resident memory size
            memoryUsage.m_residentInBytes += imageSizeAfter - imageSizeBefore;
            memoryUsage.Validate();
            image.m_residentSizeInBytes = imageSizeAfter;

            // Create new expend request and append callback from the StreamingImagePool
            RHI::StreamingImageExpandRequest newRequest = request;
            newRequest.m_completeCallback = [=]() 
            {
                Image& dxImage = static_cast<Image&>(*request.m_image);
                dxImage.FinalizeAsyncUpload(residentMipLevelAfter);
                GetResolver()->AddImageTransitionBarrier(dxImage, residentMipLevelBefore, residentMipLevelAfter);

                request.m_completeCallback();
            };

            GetDevice().GetAsyncUploadQueue().QueueUpload(newRequest, residentMipLevelBefore);

            return RHI::ResultCode::Success;
        }

        RHI::ResultCode StreamingImagePool::TrimImageInternal(RHI::Image& image, uint32_t targetMipLevel)
        {
            Image& imageImpl = static_cast<Image&>(image);

            // Wait for any upload of this image done. 
            GetDevice().GetAsyncUploadQueue().WaitForUpload(imageImpl.GetUploadFenceValue());

            // Set streamed mip level to target mip level
            if (imageImpl.GetStreamedMipLevel() < targetMipLevel)
            {
                imageImpl.SetStreamedMipLevel(targetMipLevel);
            }

            D3D12_RESOURCE_ALLOCATION_INFO allocationInfoAfter = GetAllocationInfo(image.GetDescriptor(), targetMipLevel);

            const uint32_t residentMipLevelBefore = image.GetResidentMipLevel();

            RHI::HeapMemoryUsage& memoryUsage = m_memoryUsage.GetHeapMemoryUsage(RHI::HeapMemoryLevel::Device);
            const size_t imageSizeBefore = imageImpl.m_residentSizeInBytes;
            const size_t imageSizeAfter = allocationInfoAfter.SizeInBytes;

            if (imageImpl.IsTiled())
            {
                DeAllocateStandardImageTiles(imageImpl, RHI::Interval{ residentMipLevelBefore, targetMipLevel});
            }

            const size_t sizeDiffInBytes = imageSizeBefore - imageSizeAfter;
            memoryUsage.m_residentInBytes -= sizeDiffInBytes;
            memoryUsage.m_reservedInBytes -= sizeDiffInBytes;
            imageImpl.m_residentSizeInBytes = imageSizeAfter;
            memoryUsage.Validate();

            GetResolver()->AddImageTransitionBarrier(imageImpl, residentMipLevelBefore, targetMipLevel);

            return RHI::ResultCode::Success;
        }
    }
}
