/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>

#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AZ
{
    namespace RPI
    {
        const char* StreamingImageAsset::DisplayName = "StreamingImage";
        const char* StreamingImageAsset::Group = "Image";
        const char* StreamingImageAsset::Extension = "streamingimage";

        void StreamingImageAsset::Reflect(ReflectContext* context)
        {
            if (auto* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<MipChain>()
                    ->Field("m_mipOffset", &MipChain::m_mipOffset)
                    ->Field("m_mipCount", &MipChain::m_mipCount)
                    ->Field("m_asset", &MipChain::m_asset)
                    ;

                serializeContext->Class<StreamingImageAsset, ImageAsset>()
                    ->Version(2) // Added m_averageColor field
                    ->Field("m_mipLevelToChainIndex", &StreamingImageAsset::m_mipLevelToChainIndex)
                    ->Field("m_mipChains", &StreamingImageAsset::m_mipChains)
                    ->Field("m_flags", &StreamingImageAsset::m_flags)
                    ->Field("m_tailMipChain", &StreamingImageAsset::m_tailMipChain)
                    ->Field("m_totalImageDataSize", &StreamingImageAsset::m_totalImageDataSize)
                    ->Field("m_averageColor", &StreamingImageAsset::m_averageColor)
                    ->Field("m_tags", &StreamingImageAsset::m_tags)
                    ;
            }
        }

        const Data::Asset<ImageMipChainAsset>& StreamingImageAsset::GetMipChainAsset(size_t mipChainIndex) const
        {
            return m_mipChains[mipChainIndex].m_asset;
        }

        const ImageMipChainAsset& StreamingImageAsset::GetTailMipChain() const
        {
            return m_tailMipChain;
        }

        size_t StreamingImageAsset::GetMipChainCount() const
        {
            return m_mipChains.size();
        }

        size_t StreamingImageAsset::GetMipChainIndex(size_t mipLevel) const
        {
            if (mipLevel >= m_imageDescriptor.m_mipLevels)
            {
                AZ_Assert(false, "Input mipLevel doesn't exist");
                mipLevel = m_imageDescriptor.m_mipLevels - 1;
            }
            return m_mipLevelToChainIndex[mipLevel];
        }

        size_t StreamingImageAsset::GetMipLevel(size_t mipChainIndex) const
        {
            return m_mipChains[mipChainIndex].m_mipOffset;
        }

        size_t StreamingImageAsset::GetMipCount(size_t mipChainIndex) const
        {
            return m_mipChains[mipChainIndex].m_mipCount;
        }

        const Data::AssetId& StreamingImageAsset::GetPoolAssetId() const
        {
            return m_poolAssetId;
        }

        StreamingImageFlags StreamingImageAsset::GetFlags() const
        {
            return m_flags;
        }

        size_t StreamingImageAsset::GetTotalImageDataSize() const
        {
            return m_totalImageDataSize;
        }

        Color StreamingImageAsset::GetAverageColor() const
        {
            if (m_averageColor.IsFinite())
            {
                return m_averageColor;
            }
            else
            {
                AZ_Warning(
                    "Streaming Image", false,
                    "Non-finite average color, it probably was never initialized. Returning black.");
                return Color(0.0f);
            }
        }

        RHI::ImageDescriptor StreamingImageAsset::GetImageDescriptorForMipLevel(AZ::u32 mipLevel) const
        {
            RHI::ImageDescriptor imageDescriptor = GetImageDescriptor();

            // Retrieve the layout for the particular mip level.
            // The levels get stored in a series of ImageMipChainAssets, which keep a
            // record of an offset so that you can calculate the sub-image index
            // based on the actual mip level.
            const ImageMipChainAsset* mipChainAsset = GetImageMipChainAsset(mipLevel);
            if (mipChainAsset)
            {
                auto mipChainIndex = GetMipChainIndex(mipLevel);
                auto mipChainOffset = aznumeric_cast<AZ::u32>(GetMipLevel(mipChainIndex));
                auto layout = mipChainAsset->GetSubImageLayout(mipLevel - mipChainOffset);

                imageDescriptor.m_size = layout.m_size;
            }
            else
            {
                AZ_Warning("Streaming Image", false, "Mip level index (%u) out of bounds, only %u levels available for asset %s",
                    mipLevel, imageDescriptor.m_mipLevels, m_assetId.ToString<AZStd::string>().c_str());
                return RHI::ImageDescriptor();
            }

            return imageDescriptor;
        }

        const AZStd::vector<AZ::Name>& StreamingImageAsset::GetTags() const
        {
            return m_tags;
        }

        void StreamingImageAsset::RemoveFrontMipchains(size_t mipChainLevel)
        {
            mipChainLevel = AZStd::min(mipChainLevel, m_mipChains.size() - 1);
            if (mipChainLevel == 0)
            {
                return;
            }

            AZ::u16 mipmapShift = m_mipChains[mipChainLevel].m_mipOffset;

            AZStd::move(m_mipLevelToChainIndex.begin() + mipmapShift, m_mipLevelToChainIndex.end(), m_mipLevelToChainIndex.begin());
            AZ_Assert(m_mipLevelToChainIndex.front() == mipChainLevel, "unmatching mipchain index");

            for (AZ::u16& chainIndex : m_mipLevelToChainIndex)
            {
                chainIndex -= aznumeric_cast<AZ::u16>(mipChainLevel);
            }

            AZStd::move(m_mipChains.begin() + mipChainLevel, m_mipChains.end(), m_mipChains.begin());
            m_mipChains.resize(m_mipChains.size() - mipChainLevel);

            for (auto& mipChain : m_mipChains)
            {
                // Assert that the offset does not become negative after subtraction:
                AZ_Assert(mipChain.m_mipOffset >= mipmapShift, "unexpected mipoffset");
                mipChain.m_mipOffset -= mipmapShift;
            }

            m_imageDescriptor.m_mipLevels -= mipmapShift;
            m_imageDescriptor.m_size = m_imageDescriptor.m_size.GetReducedMip(mipmapShift);
        }

        AZStd::span<const uint8_t> StreamingImageAsset::GetSubImageData(uint32_t mip, uint32_t slice)
        {
            const ImageMipChainAsset* mipChainAsset = GetImageMipChainAsset(mip);

            if (mipChainAsset == nullptr)
            {
                AZ_Warning("Streaming Image", false, "MipChain asset wasn't loaded for assetId %s",
                    m_assetId.ToString<AZStd::string>().c_str());
                return AZStd::span<const uint8_t>();
            }

            auto mipChainIndex = GetMipChainIndex(mip);
            auto mipChainOffset = aznumeric_cast<AZ::u32>(GetMipLevel(mipChainIndex));
            return mipChainAsset->GetSubImageData(mip - mipChainOffset, slice);
        }

        const ImageMipChainAsset* StreamingImageAsset::GetImageMipChainAsset(AZ::u32 mipLevel) const
        {
            if (mipLevel >= m_mipLevelToChainIndex.size())
            {
                return nullptr;
            }

            size_t mipChainIndex = m_mipLevelToChainIndex[mipLevel];
            const MipChain& mipChain = m_mipChains[mipChainIndex];

            const ImageMipChainAsset* mipChainAsset = nullptr;

            // Use m_tailMipChain if it's the last mip chain
            if (mipChainIndex == m_mipChains.size() - 1)
            {
                mipChainAsset = &m_tailMipChain;
            }
            else if (mipChain.m_asset.IsReady())
            {
                mipChainAsset = mipChain.m_asset.Get();
            }

            return mipChainAsset;
        }

        bool StreamingImageAsset::HasFullMipChainAssets() const
        {
            for (const auto& mipChain : m_mipChains)
            {
                if (mipChain.m_asset.GetId().IsValid() && !mipChain.m_asset.GetData())
                {
                    // if the asset id is valid but the asset doesn't contain asset data, return false
                    return false;
                }
            }
            return true;
        }
    }
}
