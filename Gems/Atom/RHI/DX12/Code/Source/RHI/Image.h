/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <RHI/TileAllocator.h>
#include <RHI/MemoryView.h>
#include <Atom/RHI/DeviceImage.h>
#include <Atom/RHI/Allocator.h>
#include <Atom/RHI/ImageProperty.h>
#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RHI.Reflect/Limits.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/parallel/atomic.h>

namespace AZ
{
    namespace DX12
    {
        /**
         * Contains the tiled resource layout for an image. More than one sub-resources can be packed
         * into one or more tiles. The lowest N mips are typically packed into one or two tiles. The rest
         * of the mips are considered 'standard' and are composed of one or more tiles.
         */
        struct ImageTileLayout
        {
            // Returns whether the subresource is packed into a tile with other subresources.
            bool IsPacked(uint32_t subresourceIndex) const;

            // Returns the first subresource index associated with packed mips.
            uint32_t GetPackedSubresourceIndex() const;

            // Returns the tile offset relative to the image.
            uint32_t GetTileOffset(uint32_t subresourceIndex) const;

            /**
             * Given a subresource index, returns the tile offset of the subresource from the total
             * image tile set. The coordinate and region size are used to describe how the tiles map
             * to the source image. Packed mips are treated as a simple region of flat tiles.
             */
            void GetSubresourceTileInfo(
                uint32_t subresourceIndex,
                uint32_t& imageTileOffset,
                D3D12_TILED_RESOURCE_COORDINATE& coordinate,
                D3D12_TILE_REGION_SIZE& regionSize) const;

            RHI::Size m_tileSize;
            uint32_t m_tileCount = 0;
            uint32_t m_tileCountStandard = 0;
            uint32_t m_tileCountPacked = 0;
            uint32_t m_mipCount = 0;
            uint32_t m_mipCountStandard = 0;
            uint32_t m_mipCountPacked = 0;
            AZStd::vector<D3D12_SUBRESOURCE_TILING> m_subresourceTiling;
        };

        class Image final
            : public RHI::DeviceImage
        {
            using Base = RHI::DeviceImage;
        public:
            AZ_CLASS_ALLOCATOR(Image, AZ::SystemAllocator);
            AZ_RTTI(Image, "{D2B32EE2-2ED5-477A-8346-95AF0D11DAC8}", Base);
            ~Image() = default;

            static RHI::Ptr<Image> Create();

            // Returns the memory view allocated to this buffer.
            const MemoryView& GetMemoryView() const;
            MemoryView& GetMemoryView();

            // Call when an asynchronous upload has completed.
            void FinalizeAsyncUpload(uint32_t newStreamedMipLevels);

            // Get mip level uploaded to GPU
            uint32_t GetStreamedMipLevel() const;

            void SetStreamedMipLevel(uint32_t streamedMipLevel);

            // Returns whether the image is using a tiled resource.
            bool IsTiled() const;
            
            void SetUploadFenceValue(uint64_t fenceValue);
            uint64_t GetUploadFenceValue() const;

            // Describes the state of a subresource by index.
            struct SubresourceAttachmentState
            {
                uint32_t m_subresourceIndex = 0;
                D3D12_RESOURCE_STATES m_state = D3D12_RESOURCE_STATE_COMMON;
            };

            // Describes the resource state of a range of subresources.
            using SubresourceRangeAttachmentState = RHI::ImageProperty<D3D12_RESOURCE_STATES>::PropertyRange;

            // Set the attachment state of the image subresources. If argument "range" is nullptr, then the new state will be applied to all subresources.
            void SetAttachmentState(D3D12_RESOURCE_STATES state, const RHI::ImageSubresourceRange* range = nullptr);

            // Set the attachment state of the image subresources using the subresource index.
            void SetAttachmentState(D3D12_RESOURCE_STATES state, uint32_t subresourceIndex);

            // Get the attachment state of some of the subresources of the image by their RHI::ImageSubresourceRange.
            // If argument "range" is nullptr, then the state for all subresource will be return.
            AZStd::vector<SubresourceRangeAttachmentState> GetAttachmentStateByRange(const RHI::ImageSubresourceRange* range = nullptr) const;

            // Get the attachment state of some of the subresources of the image by their subresource index.
            // If argument "range" is nullptr, then the state for all subresource will be return.
            AZStd::vector<SubresourceAttachmentState> GetAttachmentStateByIndex(const RHI::ImageSubresourceRange* range = nullptr) const;

            // Return the initial state of this image (the one used when it was created).
            D3D12_RESOURCE_STATES GetInitialResourceState() const;

        private:
            Image() = default;

            friend class SwapChain;
            friend class ImagePool;
            friend class StreamingImagePool;
            friend class AliasedHeap;
            friend class ImagePoolResolver;
            friend class StreamingImagePoolResolver;

            //////////////////////////////////////////////////////////////////////////
            // RHI::Object
            void SetNameInternal(const AZStd::string_view& name) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceResource
            void ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // RHI::DeviceImage
            void GetSubresourceLayoutsInternal(
                const RHI::ImageSubresourceRange& subresourceRange,
                RHI::DeviceImageSubresourceLayout* subresourceLayouts,
                size_t* totalSizeInBytes) const override;
                                
            bool IsStreamableInternal() const override;

            void SetDescriptor(const RHI::ImageDescriptor& descriptor) override;
            //////////////////////////////////////////////////////////////////////////

            // Calculate the size of all the tiles allocated for this image and save the number in m_residentSizeInBytes
            void UpdateResidentTilesSizeInBytes(uint32_t sizePerTile);

            void GenerateSubresourceLayouts();
            
            // The memory view allocated to this image.
            MemoryView m_memoryView;

            // The number of bytes actually resident.
            // For tiled resources, this size is same as the memory of tiles are used for mipmaps which are resident. It would be updated every time the image's mipmap
            // is expanded or trimmed.
            // For committed resources, this size won't change after image is initialized. 
            size_t m_residentSizeInBytes = 0;

            // The minimum resident size of this image. The size is the same as resident size when image was initialized.
            size_t m_minimumResidentSizeInBytes = 0;

            AZStd::array<RHI::DeviceImageSubresourceLayout, RHI::Limits::Image::MipCountMax> m_subresourceLayoutsPerMipChain;

            // The layout of tiles with respect to each subresource in the image.
            ImageTileLayout m_tileLayout;

            // The map of heap tiles allocated for each subresources
            // Note: the tiles allocated for each subresource may come from multiple heap pages 
            AZStd::unordered_map<uint32_t, AZStd::vector<HeapTiles>> m_heapTiles;

            // Tracking the actual mip level data uploaded. It's also used for invalidate image view. 
            uint32_t m_streamedMipLevel = 0;

            // The queue fence value of latest async upload request 
            uint64_t m_uploadFenceValue = 0;

            // The initial state for the graph compiler to use when compiling the resource transition chain.
            RHI::ImageProperty<D3D12_RESOURCE_STATES> m_attachmentState;

            // The initial state used when creating this image.
            D3D12_RESOURCE_STATES m_initialResourceState = D3D12_RESOURCE_STATE_COMMON;

            // The number of resolve operations pending for this image.
            AZStd::atomic<uint32_t> m_pendingResolves = 0;
        };
    }
}
