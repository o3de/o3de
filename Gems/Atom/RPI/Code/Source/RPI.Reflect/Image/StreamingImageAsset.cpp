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
    namespace Internal
    {
        template <AZ::RHI::Format>
        float RetrieveFloatValue(const AZ::u8* mem, size_t index)
        {
            static_assert(false, "Unsupported pixel format");
        }

        template <AZ::RHI::Format>
        AZ::u32 RetrieveUintValue(const AZ::u8* mem, size_t index)
        {
            static_assert(false, "Unsupported pixel format");
        }

        template <AZ::RHI::Format>
        AZ::s32 RetrieveIntValue(const AZ::u8* mem, size_t index)
        {
            static_assert(false, "Unsupported pixel format");
        }

        template <>
        float RetrieveFloatValue<AZ::RHI::Format::Unknown>([[maybe_unused]] const AZ::u8* mem, [[maybe_unused]] size_t index)
        {
            return 0.0f;
        }

        template <>
        AZ::u32 RetrieveUintValue<AZ::RHI::Format::Unknown>([[maybe_unused]] const AZ::u8* mem, [[maybe_unused]] size_t index)
        {
            return 0;
        }

        template <>
        AZ::s32 RetrieveIntValue<AZ::RHI::Format::Unknown>([[maybe_unused]] const AZ::u8* mem, [[maybe_unused]] size_t index)
        {
            return 0;
        }

        template <>
        AZ::u32 RetrieveUintValue<AZ::RHI::Format::R8_UNORM>(const AZ::u8* mem, size_t index)
        {
            return mem[index] / static_cast<AZ::u32>(std::numeric_limits<AZ::u8>::max());
        }

        template <>
        AZ::u32 RetrieveUintValue<AZ::RHI::Format::R16_UNORM>(const AZ::u8* mem, size_t index)
        {
            // 16 bits per channel
            auto actualMem = reinterpret_cast<const AZ::u16*>(mem);

            return actualMem[index] / static_cast<AZ::u32>(std::numeric_limits<AZ::u16>::max());
        }

        template <>
        AZ::s32 RetrieveIntValue<AZ::RHI::Format::R16_SINT>(const AZ::u8* mem, size_t index)
        {
            // 16 bits per channel
            auto actualMem = reinterpret_cast<const AZ::s16*>(mem);

            return actualMem[index] / static_cast<AZ::s32>(std::numeric_limits<AZ::s16>::max());
        }

        template <>
        float RetrieveFloatValue<AZ::RHI::Format::R32_UINT>(const AZ::u8* mem, size_t index)
        {
            // 32 bits per channel
            auto actualMem = reinterpret_cast<const float*>(mem);
            actualMem += index;

            return *actualMem;
        }

        template <>
        float RetrieveFloatValue<AZ::RHI::Format::R32_FLOAT>(const AZ::u8* mem, size_t index)
        {
            // 32 bits per channel
            auto actualMem = reinterpret_cast<const float*>(mem);
            actualMem += index;

            return *actualMem;
        }

        float RetrieveFloatValue(const AZ::u8* mem, size_t index, AZ::RHI::Format format)
        {
            switch (format)
            {
            case AZ::RHI::Format::R32_UINT:
                return RetrieveFloatValue<AZ::RHI::Format::R32_UINT>(mem, index);
            case AZ::RHI::Format::R32_FLOAT:
                return RetrieveFloatValue<AZ::RHI::Format::R32_FLOAT>(mem, index);
            default:
                return RetrieveFloatValue<AZ::RHI::Format::Unknown>(mem, index);
            }
        }

        AZ::u32 RetrieveUintValue(const AZ::u8* mem, size_t index, AZ::RHI::Format format)
        {
            switch (format)
            {
            case AZ::RHI::Format::R8_UNORM:
                return RetrieveUintValue<AZ::RHI::Format::R8_UNORM>(mem, index);
            case AZ::RHI::Format::R16_UNORM:
                return RetrieveUintValue<AZ::RHI::Format::R16_UNORM>(mem, index);
            default:
                return RetrieveUintValue<AZ::RHI::Format::Unknown>(mem, index);
            }
        }

        AZ::s32 RetrieveIntValue(const AZ::u8* mem, size_t index, AZ::RHI::Format format)
        {
            switch (format)
            {
            case AZ::RHI::Format::R16_SINT:
                return RetrieveIntValue<AZ::RHI::Format::R16_SINT>(mem, index);
            default:
                return RetrieveIntValue<AZ::RHI::Format::Unknown>(mem, index);
            }
        }
    }

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

        template<typename T>
        T StreamingImageAsset::GetSubImagePixelValueInternal(uint32_t x, uint32_t y, uint32_t componentIndex, uint32_t mip, uint32_t slice)
        {
            AZStd::vector<T> values;
            auto position = AZStd::make_pair(x, y);

            GetSubImagePixelValues(position, position, values, componentIndex, mip, slice);

            if (values.size() == 1)
            {
                return values[0];
            }

            return aznumeric_cast<T>(0);
        }

        template<>
        float StreamingImageAsset::GetSubImagePixelValue<float>(uint32_t x, uint32_t y, uint32_t componentIndex, uint32_t mip, uint32_t slice)
        {
            return GetSubImagePixelValueInternal<float>(x, y, componentIndex, mip, slice);
        }

        template<>
        AZ::u32 StreamingImageAsset::GetSubImagePixelValue<AZ::u32>(uint32_t x, uint32_t y, uint32_t componentIndex, uint32_t mip, uint32_t slice)
        {
            return GetSubImagePixelValueInternal<AZ::u32>(x, y, componentIndex, mip, slice);
        }

        template<>
        AZ::s32 StreamingImageAsset::GetSubImagePixelValue<AZ::s32>(uint32_t x, uint32_t y, uint32_t componentIndex, uint32_t mip, uint32_t slice)
        {
            return GetSubImagePixelValueInternal<AZ::s32>(x, y, componentIndex, mip, slice);
        }

        void StreamingImageAsset::GetSubImagePixelValues(AZStd::pair<uint32_t, uint32_t> topLeft, AZStd::pair<uint32_t, uint32_t> bottomRight, AZStd::array_view<float> outValues, uint32_t componentIndex, uint32_t mip, uint32_t slice)
        {
            // TODO: Use the component index
            (void)componentIndex;

            auto imageData = GetSubImageData(mip, slice);

            if (!imageData.empty())
            {
                const AZ::RHI::ImageDescriptor imageDescriptor = GetImageDescriptor();
                auto width = imageDescriptor.m_size.m_width;

                size_t outValuesIndex = 0;
                for (uint32_t x = topLeft.first; x < bottomRight.first; ++x)
                {
                    for (uint32_t y = topLeft.second; y < bottomRight.second; ++y)
                    {
                        size_t imageDataIndex = (y * width) + x;

                        auto& outValue = const_cast<float&>(outValues[outValuesIndex++]);
                        outValue = Internal::RetrieveFloatValue(imageData.data(), imageDataIndex, imageDescriptor.m_format);
                    }
                }
            }
        }

        void StreamingImageAsset::GetSubImagePixelValues(AZStd::pair<uint32_t, uint32_t> topLeft, AZStd::pair<uint32_t, uint32_t> bottomRight, AZStd::array_view<AZ::u32> outValues, uint32_t componentIndex, uint32_t mip, uint32_t slice)
        {
            // TODO: Use the component index
            (void)componentIndex;

            auto imageData = GetSubImageData(mip, slice);

            if (!imageData.empty())
            {
                const AZ::RHI::ImageDescriptor imageDescriptor = GetImageDescriptor();
                auto width = imageDescriptor.m_size.m_width;

                size_t outValuesIndex = 0;
                for (uint32_t x = topLeft.first; x < bottomRight.first; ++x)
                {
                    for (uint32_t y = topLeft.second; y < bottomRight.second; ++y)
                    {
                        size_t imageDataIndex = (y * width) + x;

                        auto& outValue = const_cast<AZ::u32&>(outValues[outValuesIndex++]);
                        outValue = Internal::RetrieveUintValue(imageData.data(), imageDataIndex, imageDescriptor.m_format);
                    }
                }
            }
        }

        void StreamingImageAsset::GetSubImagePixelValues(AZStd::pair<uint32_t, uint32_t> topLeft, AZStd::pair<uint32_t, uint32_t> bottomRight, AZStd::array_view<AZ::s32> outValues, uint32_t componentIndex, uint32_t mip, uint32_t slice)
        {
            // TODO: Use the component index
            (void)componentIndex;

            auto imageData = GetSubImageData(mip, slice);

            if (!imageData.empty())
            {
                const AZ::RHI::ImageDescriptor imageDescriptor = GetImageDescriptor();
                auto width = imageDescriptor.m_size.m_width;

                size_t outValuesIndex = 0;
                for (uint32_t x = topLeft.first; x < bottomRight.first; ++x)
                {
                    for (uint32_t y = topLeft.second; y < bottomRight.second; ++y)
                    {
                        size_t imageDataIndex = (y * width) + x;

                        auto& outValue = const_cast<AZ::s32&>(outValues[outValuesIndex++]);
                        outValue = Internal::RetrieveIntValue(imageData.data(), imageDataIndex, imageDescriptor.m_format);
                    }
                }
            }
        }
    }
}
