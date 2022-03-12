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
                    ->Version(1)
                    ->Field("m_mipLevelToChainIndex", &StreamingImageAsset::m_mipLevelToChainIndex)
                    ->Field("m_mipChains", &StreamingImageAsset::m_mipChains)
                    ->Field("m_flags", &StreamingImageAsset::m_flags)
                    ->Field("m_tailMipChain", &StreamingImageAsset::m_tailMipChain)
                    ->Field("m_totalImageDataSize", &StreamingImageAsset::m_totalImageDataSize)
                    ;
            }
        }
        
        const Data::Asset<ImageMipChainAsset>& StreamingImageAsset::GetMipChainAsset(size_t mipChainIndex) const
        {
            return m_mipChains[mipChainIndex].m_asset;
        }

        void StreamingImageAsset::ReleaseMipChainAssets()
        {
            // Release loaded mipChain asset
            for (uint32_t mipChainIndex = 0; mipChainIndex < m_mipChains.size() - 1; mipChainIndex++)
            {
                m_mipChains[mipChainIndex].m_asset.Release();
            }
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
        
        AZStd::array_view<uint8_t> StreamingImageAsset::GetSubImageData(uint32_t mip, uint32_t slice)
        {
            if (mip >= m_mipLevelToChainIndex.size())
            {
                return AZStd::array_view<uint8_t>();
            }

            size_t mipChainIndex = m_mipLevelToChainIndex[mip];
            MipChain& mipChain = m_mipChains[mipChainIndex];

            ImageMipChainAsset* mipChainAsset = nullptr;

            // Use m_tailMipChain if it's the last mip chain
            if (mipChainIndex == m_mipChains.size() - 1)
            {
                mipChainAsset = &m_tailMipChain;
            }
            else if (mipChain.m_asset.IsReady())
            {
                mipChainAsset = mipChain.m_asset.Get();
            }

            if (mipChainAsset == nullptr)
            {
                AZ_Warning("Streaming Image", false, "MipChain asset wasn't loaded");
                return AZStd::array_view<uint8_t>();
            }

            return mipChainAsset->GetSubImageData(mip - mipChain.m_mipOffset, slice);
        }
    }
}
