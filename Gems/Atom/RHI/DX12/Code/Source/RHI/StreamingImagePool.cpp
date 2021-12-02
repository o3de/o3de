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
#include <AzCore/Debug/EventTrace.h>

// NOTE: Tiled resources are currently disabled, because RenderDoc does not support them.
// #define AZ_RHI_USE_TILED_RESOURCES

namespace AZ
{
    namespace DX12
    {
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
            AZ_TRACE_METHOD();

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
            AZ_TRACE_METHOD();



#ifdef AZ_RHI_USE_TILED_RESOURCES
            {
                AZ_PROFILE_SCOPE(RHI, "StreamImagePool::CreateHeap");

                CD3DX12_HEAP_DESC heapDesc(descriptor.m_budgetInBytes, D3D12_HEAP_TYPE_DEFAULT, 0, D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES);

                Microsoft::WRL::ComPtr<ID3D12Heap> heap;
                AssertSuccess(device.GetDevice()->CreateHeap(&heapDesc, IID_GRAPHICS_PPV_ARGS(heap.GetAddressOf())));
                m_heap = heap.Get();
            }

            {
                const size_t tileCountTotal = descriptor.m_budgetInBytes / D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

                RHI::PoolAllocator::Descriptor allocatorDesc;
                allocatorDesc.m_elementSize = 1;
                allocatorDesc.m_alignmentInBytes = 1;
                allocatorDesc.m_capacityInBytes = tileCountTotal;
                m_tileAllocator.Init(allocatorDesc);
            }
#endif

            SetResolver(AZStd::make_unique<StreamingImagePoolResolver>());
            return RHI::ResultCode::Success;
        }

        void StreamingImagePool::ShutdownInternal()
        {
#ifdef AZ_RHI_USE_TILED_RESOURCES
            GetDevice().QueueForRelease(AZStd::move(m_heap));
            m_tileAllocator.Shutdown();
#endif
        }

        void StreamingImagePool::AllocateImageTilesInternal(Image& image, CommandList::TileMapRequest& request, uint32_t subresourceIndex)
        {
            AZ_Assert(request.m_destinationHeap, "Destination heap must be valid.");

            uint32_t imageTileOffset = 0;
            image.m_tileLayout.GetSubresourceTileInfo(subresourceIndex, imageTileOffset, request.m_sourceCoordinate, request.m_sourceRegionSize);

            request.m_destinationTileMap.resize(request.m_sourceRegionSize.NumTiles);

            // Allocate tiles from the tile pool.
            for (uint32_t subresourceTileIndex = 0; subresourceTileIndex < request.m_sourceRegionSize.NumTiles; ++subresourceTileIndex)
            {
                const uint32_t imageTileIndex = imageTileOffset + subresourceTileIndex;

                // Make sure the tile is actually null so we don't leak.
                AZ_Assert(image.m_tiles[imageTileIndex].IsNull(), "Stomping on an existing tile allocation. This will leak.");

                RHI::VirtualAddress address = m_tileAllocator.Allocate();
                AZ_Assert(address.IsValid(), "Exceeded size of tile heap. The budget checks should have caught this.");

                // Store the allocation in the image (using image-relative index).
                image.m_tiles[imageTileIndex] = address;

                // Store the allocation into the map request (using subresource-relative index).
                request.m_destinationTileMap[subresourceTileIndex] = static_cast<uint32_t>(address.m_ptr);
            }

            GetDevice().GetAsyncUploadQueue().QueueTileMapping(request);

            // Garbage collect the allocator immediately.
            m_tileAllocator.GarbageCollect();
        }

        void StreamingImagePool::DeAllocateImageTilesInternal(Image& image, uint32_t subresourceIndex)
        {
            D3D12_TILED_RESOURCE_COORDINATE sourceCoordinate;
            D3D12_TILE_REGION_SIZE sourceRegionSize;
            uint32_t imageTileOffset = 0;
            image.m_tileLayout.GetSubresourceTileInfo(subresourceIndex, imageTileOffset, sourceCoordinate, sourceRegionSize);

            for (uint32_t subresourceTileIndex = 0; subresourceTileIndex < sourceRegionSize.NumTiles; ++subresourceTileIndex)
            {
                const uint32_t imageTileIndex = imageTileOffset + subresourceTileIndex;

                // De-allocate the tile and reset it null.
                AZ_Assert(image.m_tiles[imageTileIndex].IsValid(), "Attempting to map a tile to null when it's already null.");
                m_tileAllocator.DeAllocate(image.m_tiles[imageTileIndex]);
                image.m_tiles[imageTileIndex] = {};
            }

            // Garbage collect the allocator immediately.
            m_tileAllocator.GarbageCollect();
        }

        void StreamingImagePool::AllocatePackedImageTiles(Image& image)
        {
            AZ_TRACE_METHOD();

            AZ_Assert(image.IsTiled(), "This method is only valid for tiled resources.");
            AZ_Assert(image.GetDescriptor().m_arraySize == 1, "Not implemented for image arrays.");

            const ImageTileLayout& tileLayout = image.m_tileLayout;
            if (tileLayout.m_mipCountPacked)
            {
                CommandList::TileMapRequest request;
                request.m_sourceMemory = image.GetMemoryView().GetMemory();
                request.m_destinationHeap = m_heap.get();
                AllocateImageTilesInternal(image, request, tileLayout.GetPackedSubresourceIndex());
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
                // Reuse the same request structure (avoids additional heap allocations).
                CommandList::TileMapRequest request;
                request.m_sourceMemory = image.GetMemoryView().GetMemory();
                request.m_destinationHeap = m_heap.get();

                AZStd::lock_guard<AZStd::mutex> lock(m_tileMutex);
                for (uint32_t arrayIndex = 0; arrayIndex < descriptor.m_arraySize; ++arrayIndex)
                {
                    for (uint32_t mipIndex = mipInterval.m_min; mipIndex < mipInterval.m_max; ++mipIndex)
                    {
                        AllocateImageTilesInternal(image, request, RHI::GetImageSubresourceIndex(mipIndex, arrayIndex, descriptor.m_mipLevels));
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
            AZ_TRACE_METHOD();

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

            image.m_tiles.resize(image.m_tileLayout.m_tileCount);
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
                for (RHI::VirtualAddress address : image.m_tiles)
                {
                    m_tileAllocator.DeAllocate(address);
                }
                m_tileAllocator.GarbageCollect();
                m_tileMutex.unlock();
                image.m_tiles.clear();
                image.m_tiles.shrink_to_fit();
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
