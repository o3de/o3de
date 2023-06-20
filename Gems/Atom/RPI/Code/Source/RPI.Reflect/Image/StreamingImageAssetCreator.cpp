/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/ImageMipChainAssetCreator.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetCreator.h>

#include <AzCore/Asset/AssetManager.h>

namespace AZ
{
    namespace RPI
    {
        void StreamingImageAssetCreator::Begin(const Data::AssetId& assetId)
        {
            BeginCommon(assetId);

            m_mipLevels = 0;
        }

        void StreamingImageAssetCreator::SetImageDescriptor(const RHI::ImageDescriptor& descriptor)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            if (descriptor.m_mipLevels > RHI::Limits::Image::MipCountMax)
            {
                ReportError("Exceeded the maximum number of mip levels supported by the RHI.");
                return;
            }

            m_asset->m_imageDescriptor = descriptor;
        }

        void StreamingImageAssetCreator::SetImageViewDescriptor(const RHI::ImageViewDescriptor& descriptor)
        {
            if (ValidateIsReady())
            {
                m_asset->m_imageViewDescriptor = descriptor;
            }
        }

        void StreamingImageAssetCreator::AddMipChainAsset(ImageMipChainAsset& mipChainAsset)
        {
            if (!ValidateIsReady())
            {
                return;
            }

            if (!mipChainAsset.GetId().IsValid())
            {
                ReportError("ImageMipChainAsset does not have a valid id. A valid id is required.");
                return;
            }

            const uint16_t mipLevelOffset = m_mipLevels;
            const uint16_t localMipCount = mipChainAsset.GetMipLevelCount();
            const uint16_t localMipBegin = m_mipLevels;
            const uint16_t localMipEnd = m_mipLevels + localMipCount;
            const uint16_t mipChainIndex = static_cast<uint16_t>(m_asset->m_mipChains.size());

            if (localMipEnd > RHI::Limits::Image::MipCountMax)
            {
                ReportError("Exceeded the maximum number of mip levels supported by the RHI.");
                return;
            }

            // Map the image mip slice indices to mip chain asset. This is likely a many-to-one mapping.
            for (uint16_t localMipIndex = localMipBegin; localMipIndex < localMipEnd; ++localMipIndex)
            {
                m_asset->m_mipLevelToChainIndex[localMipIndex] = mipChainIndex;
            }

            StreamingImageAsset::MipChain mipChain;
            mipChain.m_mipOffset = mipLevelOffset;
            mipChain.m_mipCount = localMipCount;
            // Mipchain assets are not loaded by default
            mipChain.m_asset = { &mipChainAsset, AZ::Data::AssetLoadBehavior::NoLoad };

            m_asset->m_mipChains.push_back(mipChain);

            m_mipLevels += localMipCount;
        }

        void StreamingImageAssetCreator::SetPoolAssetId(const Data::AssetId& poolAssetId)
        {
            if (ValidateIsReady())
            {
                m_asset->m_poolAssetId = poolAssetId;
            }
        }

        void StreamingImageAssetCreator::SetFlags(StreamingImageFlags flag)
        {
            if (ValidateIsReady())
            {
                m_asset->m_flags = flag;
            }
        }

        void StreamingImageAssetCreator::SetAverageColor(Color avgColor)
        {
            if (ValidateIsReady())
            {
                m_asset->m_averageColor = avgColor;
            }
        }

        void StreamingImageAssetCreator::AddTag(AZ::Name tag)
        {
            if (ValidateIsReady())
            {
                // add tag if it doesn't already exist
                if (auto it = AZStd::find(m_asset->m_tags.begin(), m_asset->m_tags.end(), tag); it == m_asset->m_tags.end())
                {
                    m_asset->m_tags.push_back(AZStd::move(tag));
                }
            }
        }

        bool StreamingImageAssetCreator::End(Data::Asset<StreamingImageAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            const RHI::ImageDescriptor& imageDescriptor = m_asset->GetImageDescriptor();
            const uint16_t expectedMipLevels = imageDescriptor.m_mipLevels;

            if (m_mipLevels != expectedMipLevels)
            {
                ReportError("Expected %u mip levels, but %u were added through mip chains.", expectedMipLevels, m_mipLevels);
                return false;
            }

            size_t totalImageBytes = 0;
            for (const StreamingImageAsset::MipChain& mipChain : m_asset->m_mipChains)
            {
                totalImageBytes += mipChain.m_asset->GetImageDataSize();
            }
            m_asset->m_totalImageDataSize = aznumeric_cast<uint32_t>(totalImageBytes);

            Data::Asset<ImageMipChainAsset> mipChainTail(m_asset->m_mipChains.back().m_asset);
            m_asset->m_tailMipChain.CopyFrom(*mipChainTail.Get());
            m_asset->m_mipChains.back().m_asset = Data::Asset<ImageMipChainAsset>();
            
            // If the streaming image is not streamable, it should have one and only one mip chain
            if (RHI::CheckBitsAny(m_asset->GetFlags(), StreamingImageFlags::NotStreamable) && m_asset->GetMipChainCount() != 1)
            {
                ReportError("Expected only one mip chain asset for non-streamable streaming image");
                return false;
            }


            m_asset->SetReady();
            return EndCommon(result);
        }
    }
}
