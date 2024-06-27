/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <RHI/Conversions.h>
#include <RHI/Image.h>
#include <RHI/ImageView.h>
#include <Atom/RHI/MemoryStatisticsBuilder.h>

namespace AZ
{
    namespace DX12
    {
        RHI::Ptr<Image> Image::Create()
        {
            return aznew Image();
        }

        const MemoryView& Image::GetMemoryView() const
        {
            return m_memoryView;
        }

        MemoryView& Image::GetMemoryView()
        {
            return m_memoryView;
        }

        bool Image::IsTiled() const
        {
            return m_tileLayout.m_tileCount > 0;
        }

        void Image::SetNameInternal(const AZStd::string_view& name)
        {
            m_memoryView.SetName(name);
        }

        void Image::ReportMemoryUsage(RHI::MemoryStatisticsBuilder& builder) const
        {
            const RHI::ImageDescriptor& descriptor = GetDescriptor();

            RHI::MemoryStatistics::Image* imageStats = builder.AddImage();
            imageStats->m_name = GetName();
            imageStats->m_bindFlags = descriptor.m_bindFlags;
            imageStats->m_sizeInBytes = m_residentSizeInBytes;
            imageStats->m_minimumSizeInBytes = m_minimumResidentSizeInBytes;
        }

        void Image::UpdateResidentTilesSizeInBytes(uint32_t sizePerTile)
        {
            if (IsTiled())
            {
                uint32_t tileCount = 0;
                for (const auto& heapTilesGroups : m_heapTiles)
                {
                    for (const HeapTiles& heapTiles: heapTilesGroups.second)
                    {
                        tileCount += heapTiles.m_totalTileCount;
                    }
                }

                m_residentSizeInBytes = tileCount * sizePerTile;
            }
            else
            {
                AZ_Assert(IsTiled(), "Size won't be updated for non-tiled image ");
            }
        }

        void Image::GenerateSubresourceLayouts()
        {
            for (uint16_t mipSlice = 0; mipSlice < GetDescriptor().m_mipLevels; ++mipSlice)
            {
                RHI::DeviceImageSubresourceLayout& subresourceLayout = m_subresourceLayoutsPerMipChain[mipSlice];

                RHI::ImageSubresource subresource;
                subresource.m_mipSlice = mipSlice;
                subresourceLayout = RHI::GetImageSubresourceLayout(GetDescriptor(), subresource);

                // Align the row size to match the DX12 row pitch alignment.
                subresourceLayout.m_bytesPerRow = RHI::AlignUp(subresourceLayout.m_bytesPerRow, DX12_TEXTURE_DATA_PITCH_ALIGNMENT);
                subresourceLayout.m_bytesPerImage = subresourceLayout.m_rowCount * subresourceLayout.m_bytesPerRow;
            }
        }

        void Image::GetSubresourceLayoutsInternal(
            const RHI::ImageSubresourceRange& subresourceRange,
            RHI::DeviceImageSubresourceLayout* subresourceLayouts,
            size_t* totalSizeInBytes) const
        {
            const RHI::ImageDescriptor& imageDescriptor = GetDescriptor();

            uint32_t byteOffset = 0;

            if (subresourceLayouts)
            {
                for (uint16_t arraySlice = subresourceRange.m_arraySliceMin; arraySlice <= subresourceRange.m_arraySliceMax; ++arraySlice)
                {
                    for (uint16_t mipSlice = subresourceRange.m_mipSliceMin; mipSlice <= subresourceRange.m_mipSliceMax; ++mipSlice)
                    {
                        const RHI::DeviceImageSubresourceLayout& subresourceLayout = m_subresourceLayoutsPerMipChain[mipSlice];
                        const uint32_t subresourceIndex = RHI::GetImageSubresourceIndex(mipSlice, arraySlice, imageDescriptor.m_mipLevels);
                        subresourceLayouts[subresourceIndex] = subresourceLayout;
                        subresourceLayouts[subresourceIndex].m_offset = byteOffset;
                        byteOffset = RHI::AlignUp(byteOffset + subresourceLayout.m_bytesPerImage * subresourceLayout.m_size.m_depth, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
                    }
                }
            }
            else
            {
                for (uint16_t arraySlice = subresourceRange.m_arraySliceMin; arraySlice <= subresourceRange.m_arraySliceMax; ++arraySlice)
                {
                    for (uint16_t mipSlice = subresourceRange.m_mipSliceMin; mipSlice <= subresourceRange.m_mipSliceMax; ++mipSlice)
                    {
                        const RHI::DeviceImageSubresourceLayout& subresourceLayout = m_subresourceLayoutsPerMipChain[mipSlice];
                        byteOffset = RHI::AlignUp(byteOffset + subresourceLayout.m_bytesPerImage * subresourceLayout.m_size.m_depth, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
                    }
                }
            }

            if (totalSizeInBytes)
            {
                *totalSizeInBytes = byteOffset;
            }
        }

        bool Image::IsStreamableInternal() const
        {
            return IsTiled();
        }
        
        void Image::SetDescriptor(const RHI::ImageDescriptor& descriptor)
        {
            RHI::DeviceImage::SetDescriptor(descriptor);

            m_initialResourceState = D3D12_RESOURCE_STATE_COMMON;

            const RHI::ImageBindFlags bindFlags = descriptor.m_bindFlags;

            // Write only states
            const bool renderTarget = RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::Color);
            const bool copyDest = RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::CopyWrite);
            const bool depthTarget = RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::DepthStencil);

            // Read Only States
            const bool shaderResource = RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShaderRead);
            const bool copySource = RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::CopyRead);

            const bool writeState = renderTarget || copyDest || depthTarget;
            const bool readState = shaderResource || copySource;

            // If any write only state is set, only write only resource states can be applied
            if (writeState)
            {
                if (renderTarget)
                {
                    m_initialResourceState |= D3D12_RESOURCE_STATE_RENDER_TARGET;
                }
                else if (copyDest)
                {
                    m_initialResourceState |= D3D12_RESOURCE_STATE_COPY_DEST;
                }
                else if (depthTarget)
                {
                    m_initialResourceState |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
                }
            }
            // If any read only state is set, only read only resource states can be applied
            else if (readState)
            {
                if (shaderResource)
                {
                    if (RHI::CheckBitsAny(descriptor.m_sharedQueueMask, RHI::HardwareQueueClassMask::Graphics))
                    {
                        m_initialResourceState |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
                    }
                    if (RHI::CheckBitsAny(descriptor.m_sharedQueueMask, RHI::HardwareQueueClassMask::Compute))
                    {
                        m_initialResourceState |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
                    }
                }

                if (copySource)
                {
                    m_initialResourceState |= D3D12_RESOURCE_STATE_COPY_SOURCE;
                }
            }
            // If neither a read only or write only state is set, we can set a read/write state
            else
            {
                if (RHI::CheckBitsAny(bindFlags, RHI::ImageBindFlags::ShaderWrite))
                {
                    m_initialResourceState |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
                }
            }

            m_attachmentState.Init(descriptor);
            SetAttachmentState(m_initialResourceState);
        }

        void Image::FinalizeAsyncUpload(uint32_t newStreamedMipLevels)
        {
            AZ_Assert(newStreamedMipLevels <= m_streamedMipLevel, "Expanded mip levels can't be more than streamed mip level");

            if (newStreamedMipLevels < m_streamedMipLevel)
            {
                m_streamedMipLevel = newStreamedMipLevels;
                InvalidateViews();
            }
        }
        
        void Image::SetUploadFenceValue(uint64_t fenceValue)
        {
            AZ_Assert(fenceValue > m_uploadFenceValue, "New fence value should always larger than previous fence value");
            m_uploadFenceValue = fenceValue;
        }

        uint64_t Image::GetUploadFenceValue() const
        {
            return m_uploadFenceValue;
        }

        uint32_t Image::GetStreamedMipLevel() const
        {
            return m_streamedMipLevel;
        }
        
        void Image::SetStreamedMipLevel(uint32_t streamedMipLevel)
        {
            if (m_streamedMipLevel != streamedMipLevel)
            {
                m_streamedMipLevel = streamedMipLevel;
                InvalidateViews();
            }
        }

        void Image::SetAttachmentState(D3D12_RESOURCE_STATES state, const RHI::ImageSubresourceRange* range /*= nullptr*/)
        {
            m_attachmentState.Set(range ? *range : RHI::ImageSubresourceRange(GetDescriptor()), state);
        }

        void Image::SetAttachmentState(D3D12_RESOURCE_STATES state, uint32_t subresourceIndex)
        {
            const auto& descriptor = GetDescriptor();
            RHI::ImageSubresourceRange range;
            if (subresourceIndex == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
            {
                range = RHI::ImageSubresourceRange(descriptor);
            }
            else
            {
                uint16_t mipSlice;
                uint16_t arraySlice;
                uint16_t planeSlice;
                D3D12DecomposeSubresource(
                    subresourceIndex, descriptor.m_mipLevels, descriptor.m_arraySize, mipSlice, arraySlice, planeSlice);
                range = RHI::ImageSubresourceRange(mipSlice, mipSlice, arraySlice, arraySlice);
                range.m_aspectFlags = ConvertPlaneSliceToImageAspectFlags(planeSlice);
            }

            m_attachmentState.Set(range, state);
        }

        AZStd::vector<Image::SubresourceRangeAttachmentState> Image::GetAttachmentStateByRange(const RHI::ImageSubresourceRange* range /*= nullptr*/) const
        {
            return m_attachmentState.Get(range ? *range : RHI::ImageSubresourceRange(GetDescriptor()));
        }

        AZStd::vector<Image::SubresourceAttachmentState> Image::GetAttachmentStateByIndex(const RHI::ImageSubresourceRange* range /*= nullptr*/) const
        {
            AZStd::vector<SubresourceAttachmentState> result;
            auto initialStatesRange = GetAttachmentStateByRange(range);
            if (initialStatesRange.empty())
            {
                return result;
            }

            // First check if we can use the D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES index by counting
            // the subresources that have the same attachment state.
            uint32_t subresourcesCount = 0;
            D3D12_RESOURCE_STATES lastState = initialStatesRange.front().m_property;
            for (const auto& initialState : initialStatesRange)
            {
                // Check if the new subresource range has the same attachment state as all previous subresources.
                // If it's different, then we just exit because we can't use the D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES index.
                if (lastState != initialState.m_property)
                {
                    break;
                }

                uint32_t mipCount = initialState.m_range.m_mipSliceMax - initialState.m_range.m_mipSliceMin + 1;
                uint32_t arraySize = initialState.m_range.m_arraySliceMax - initialState.m_range.m_arraySliceMin + 1;
                uint32_t planeCount = RHI::CountBitsSet(static_cast<uint32_t>(GetAspectFlags() & initialState.m_range.m_aspectFlags));
                subresourcesCount += planeCount * mipCount * arraySize;
            }

            // Compare the subresource count to the total subresources of the image to see if we can use the D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES special index.
            const auto& descriptor = GetDescriptor();
            uint32_t totalSubresources = descriptor.m_arraySize * descriptor.m_mipLevels * RHI::CountBitsSet(static_cast<uint32_t>(GetAspectFlags()));
            if (totalSubresources == subresourcesCount)
            {
                SubresourceAttachmentState state{};
                state.m_state = lastState;
                state.m_subresourceIndex = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
                return { state };
            }

            // Since not all subresources have the same attachment state, we need to add each subresource as a separate entry.
            for (const auto& initialState : initialStatesRange)
            {
                const RHI::ImageSubresourceRange& subresourceRange = initialState.m_range;
                for (uint16_t aspectIndex = 0; aspectIndex < RHI::ImageAspectCount; ++aspectIndex)
                {
                    if (RHI::CheckBitsAll(subresourceRange.m_aspectFlags, static_cast<RHI::ImageAspectFlags>(AZ_BIT(aspectIndex))))
                    {
                        uint16_t planeSlice = ConvertImageAspectToPlaneSlice(static_cast<RHI::ImageAspect>(aspectIndex));
                        for (uint16_t mipLevel = subresourceRange.m_mipSliceMin; mipLevel < subresourceRange.m_mipSliceMax + 1; ++mipLevel)
                        {
                            for (uint16_t arraySlice = subresourceRange.m_arraySliceMin; arraySlice < subresourceRange.m_arraySliceMax + 1; ++arraySlice)
                            {
                                result.emplace_back();
                                SubresourceAttachmentState& subresourceState = result.back();
                                subresourceState.m_state = initialState.m_property;
                                subresourceState.m_subresourceIndex = D3D12CalcSubresource(
                                    mipLevel,
                                    arraySlice,
                                    planeSlice,
                                    descriptor.m_mipLevels,
                                    descriptor.m_arraySize);
                            }
                        }
                    }
                }
            }
            return result;
        }

        D3D12_RESOURCE_STATES Image::GetInitialResourceState() const
        {
            return m_initialResourceState;
        }

        bool ImageTileLayout::IsPacked(uint32_t subresourceIndex) const
        {
            return m_subresourceTiling[subresourceIndex].StartTileIndexInOverallResource == D3D12_PACKED_TILE;
        }

        uint32_t ImageTileLayout::GetPackedSubresourceIndex() const
        {
            return m_mipCountStandard;
        }

        uint32_t ImageTileLayout::GetTileOffset(uint32_t subresourceIndex) const
        {
            uint32_t tileOffset = m_subresourceTiling[subresourceIndex].StartTileIndexInOverallResource;
            return tileOffset != D3D12_PACKED_TILE ? tileOffset : m_tileCountStandard;
        }

        void ImageTileLayout::GetSubresourceTileInfo(uint32_t subresourceIndex, uint32_t& imageTileOffset, D3D12_TILED_RESOURCE_COORDINATE& coordinate, D3D12_TILE_REGION_SIZE& regionSize) const
        {
            if (IsPacked(subresourceIndex))
            {
                // Packed mips are only supported when the array count is 1. The subresource is
                // equal to the first non-standard mip.
                coordinate = CD3DX12_TILED_RESOURCE_COORDINATE(0, 0, 0, m_mipCountStandard);

                // The region is a flat list of tiles.
                regionSize = CD3DX12_TILE_REGION_SIZE(m_tileCountPacked, 0, 0, 0, 0);

                // Assign the offset of the tile relative to the image.
                imageTileOffset = m_tileCountStandard;
            }
            else
            {
                coordinate = CD3DX12_TILED_RESOURCE_COORDINATE(0, 0, 0, subresourceIndex);

                // The region is a box covering all the tiles in the subresource.
                const D3D12_SUBRESOURCE_TILING& tiling = m_subresourceTiling[subresourceIndex];
                regionSize = CD3DX12_TILE_REGION_SIZE(
                    tiling.WidthInTiles * tiling.HeightInTiles * tiling.DepthInTiles, TRUE,
                    tiling.WidthInTiles, tiling.HeightInTiles, tiling.DepthInTiles);

                imageTileOffset = tiling.StartTileIndexInOverallResource;
            }
        }
    }
}
