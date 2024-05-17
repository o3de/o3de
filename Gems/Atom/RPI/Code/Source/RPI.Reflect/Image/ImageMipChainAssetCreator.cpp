/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>

#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace RPI
    {
        bool ImageMipChainAssetCreator::IsBuildingMip() const
        {
            return m_mipLevelsPending != m_mipLevelsCompleted;
        }

        bool ImageMipChainAssetCreator::ValidateIsBuildingMip()
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (!IsBuildingMip())
            {
                AZ_Assert(false, "BeginMip() was not called");
                return false;
            }

            return true;
        }

        void ImageMipChainAssetCreator::Begin(const Data::AssetId& assetId, uint16_t mipLevels, uint16_t arraySize)
        {
            BeginCommon(assetId);

            m_mipLevelsPending = 0;
            m_mipLevelsCompleted = 0;
            m_subImageOffset = 0;

            if (ValidateIsReady())
            {
                // An extra value is added at the end to avoid branching in GetSubImageData()
                m_asset->m_subImageDataOffsets.resize(mipLevels * arraySize + 1);
                m_asset->m_mipLevels = mipLevels;
                m_asset->m_arraySize = arraySize;
            }
        }

        void ImageMipChainAssetCreator::BeginMip(const RHI::DeviceImageSubresourceLayout& layout)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            if (IsBuildingMip())
            {
                AZ_Assert(false, "Already building a mip. You must call EndMip() first.");
                return;
            }

            if (m_mipLevelsCompleted == m_asset->m_mipLevels)
            {
                ReportError("Reached the maximum number of declared mip levels.");
                return;
            }

            m_asset->m_mipToSubImageOffset[m_mipLevelsPending] = m_subImageOffset;
            m_asset->m_subImageLayouts[m_mipLevelsPending] = layout;

            ++m_mipLevelsPending;
        }

        void ImageMipChainAssetCreator::AddSubImage(const void* data, size_t dataSize)
        {
            if (!ValidateIsBuildingMip())
            {
                return;
            }

            if (!data || !dataSize)
            {
                ReportError("You must supply a valid data payload.");
                return;
            }

            if (m_arraySlicesCompleted == m_asset->m_arraySize)
            {
                ReportError("Exceeded the %d array slices declared in Begin().", m_asset->m_arraySize);
                return;
            }

            // Resize the payload and copy in the new layout.
            auto& flatData = m_asset->m_imageData;
            const size_t dataOffset = flatData.size();
            flatData.resize(dataOffset + dataSize);
            memcpy(&flatData[dataOffset], data, dataSize);

            m_asset->m_subImageDataOffsets[m_subImageOffset] = dataOffset;

            ++m_arraySlicesCompleted;
            ++m_subImageOffset;
        }

        void ImageMipChainAssetCreator::EndMip()
        {
            if (!ValidateIsBuildingMip())
            {
                return;
            }

            if (m_arraySlicesCompleted != m_asset->m_arraySize)
            {
                ReportError("Expected %d sub-images in mip, but got %d.", m_asset->m_arraySize, m_arraySlicesCompleted);
                return;
            }

            ++m_mipLevelsCompleted;
            m_arraySlicesCompleted = 0;
        }

        bool ImageMipChainAssetCreator::End(Data::Asset<ImageMipChainAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (IsBuildingMip())
            {
                ReportError("You must call EndMip() before calling End().");
                return false;
            }

            if (m_mipLevelsCompleted != m_asset->m_mipLevels)
            {
                ReportError("The number of completed mip levels (%d) does not match the number of declared mip levels (%d).",
                    m_mipLevelsCompleted, m_asset->m_mipLevels);
                return false;
            }

            // Assign the last offset (the sentinel value) to be the size of the full image data.
            m_asset->m_subImageDataOffsets.back() = m_asset->m_imageData.size();

            m_asset->Init();
            m_asset->SetReady();
            return EndCommon(result);
        }
    }
}
