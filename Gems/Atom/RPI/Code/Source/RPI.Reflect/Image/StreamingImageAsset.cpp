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
        // The original implementation was from cryhalf's CryConvertFloatToHalf and CryConvertHalfToFloat function
        // Will be replaced with centralized half float API
        struct SHalf
        {
            explicit SHalf(float floatValue)
            {
                AZ::u32 Result;

                AZ::u32 intValue = ((AZ::u32*)(&floatValue))[0];
                AZ::u32 Sign = (intValue & 0x80000000U) >> 16U;
                intValue = intValue & 0x7FFFFFFFU;

                if (intValue > 0x47FFEFFFU)
                {
                    // The number is too large to be represented as a half.  Saturate to infinity.
                    Result = 0x7FFFU;
                }
                else
                {
                    if (intValue < 0x38800000U)
                    {
                        // The number is too small to be represented as a normalized half.
                        // Convert it to a denormalized value.
                        AZ::u32 Shift = 113U - (intValue >> 23U);
                        intValue = (0x800000U | (intValue & 0x7FFFFFU)) >> Shift;
                    }
                    else
                    {
                        // Rebias the exponent to represent the value as a normalized half.
                        intValue += 0xC8000000U;
                    }

                    Result = ((intValue + 0x0FFFU + ((intValue >> 13U) & 1U)) >> 13U) & 0x7FFFU;
                }
                h = static_cast<AZ::u16>(Result | Sign);
            }

            operator float() const
            {
                AZ::u32 Mantissa;
                AZ::u32 Exponent;
                AZ::u32 Result;

                Mantissa = h & 0x03FF;

                if ((h & 0x7C00) != 0)  // The value is normalized
                {
                    Exponent = ((h >> 10) & 0x1F);
                }
                else if (Mantissa != 0)     // The value is denormalized
                {
                    // Normalize the value in the resulting float
                    Exponent = 1;

                    do
                    {
                        Exponent--;
                        Mantissa <<= 1;
                    } while ((Mantissa & 0x0400) == 0);

                    Mantissa &= 0x03FF;
                }
                else                        // The value is zero
                {
                    Exponent = static_cast<AZ::u32>(-112);
                }

                Result = ((h & 0x8000) << 16) | // Sign
                    ((Exponent + 112) << 23) | // Exponent
                    (Mantissa << 13);          // Mantissa

                return *(float*)&Result;
            }

        private:
            AZ::u16 h;
        };

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

        float ScaleValue(float value, float origMin, float origMax, float scaledMin, float scaledMax)
        {
            return ((value - origMin) / (origMax - origMin)) * (scaledMax - scaledMin) + scaledMin;
        }

        float RetrieveFloatValue(const AZ::u8* mem, size_t index, AZ::RHI::Format format)
        {
            switch (format)
            {
            case AZ::RHI::Format::R8_UNORM:
            case AZ::RHI::Format::A8_UNORM:
            {
                return mem[index] / static_cast<float>(std::numeric_limits<AZ::u8>::max());
            }
            case AZ::RHI::Format::R8_SNORM:
            {
                // Scale the value from AZ::s8 min/max to -1 to 1
                // We need to treat -128 and -127 the same, so that we get a symmetric
                // range of -127 to 127 with complementary scaled values of -1 to 1
                auto actualMem = reinterpret_cast<const AZ::s8*>(mem);
                AZ::s8 signedMax = std::numeric_limits<AZ::s8>::max();
                AZ::s8 signedMin = aznumeric_cast<AZ::s8>(-signedMax);
                return ScaleValue(AZStd::max(actualMem[index], signedMin), signedMin, signedMax, -1.0f, 1.0f);
            }
            case AZ::RHI::Format::D16_UNORM:
            case AZ::RHI::Format::R16_UNORM:
            {
                return mem[index] / static_cast<float>(std::numeric_limits<AZ::u16>::max());
            }
            case AZ::RHI::Format::R16_SNORM:
            {
                // Scale the value from AZ::s16 min/max to -1 to 1
                // We need to treat -32768 and -32767 the same, so that we get a symmetric
                // range of -32767 to 32767 with complementary scaled values of -1 to 1
                auto actualMem = reinterpret_cast<const AZ::s16*>(mem);
                AZ::s16 signedMax = std::numeric_limits<AZ::s16>::max();
                AZ::s16 signedMin = aznumeric_cast<AZ::s16>(-signedMax);
                return ScaleValue(AZStd::max(actualMem[index], signedMin), signedMin, signedMax, -1.0f, 1.0f);
            }
            case AZ::RHI::Format::R16_FLOAT:
            {
                auto actualMem = reinterpret_cast<const float*>(mem);
                return SHalf(actualMem[index]);
            }
            case AZ::RHI::Format::D32_FLOAT:
            case AZ::RHI::Format::R32_FLOAT:
            {
                auto actualMem = reinterpret_cast<const float*>(mem);
                return actualMem[index];
            }
            default:
                return RetrieveFloatValue<AZ::RHI::Format::Unknown>(mem, index);
            }
        }

        AZ::u32 RetrieveUintValue(const AZ::u8* mem, size_t index, AZ::RHI::Format format)
        {
            switch (format)
            {
            case AZ::RHI::Format::R8_UINT:
            {
                return mem[index] / static_cast<AZ::u32>(std::numeric_limits<AZ::u8>::max());
            }
            case AZ::RHI::Format::R16_UINT:
            {
                auto actualMem = reinterpret_cast<const AZ::u16*>(mem);
                return actualMem[index] / static_cast<AZ::u32>(std::numeric_limits<AZ::u16>::max());
            }
            case AZ::RHI::Format::R32_UINT:
            {
                auto actualMem = reinterpret_cast<const AZ::u32*>(mem);
                return actualMem[index];
            }
            default:
                return RetrieveUintValue<AZ::RHI::Format::Unknown>(mem, index);
            }
        }

        AZ::s32 RetrieveIntValue(const AZ::u8* mem, size_t index, AZ::RHI::Format format)
        {
            switch (format)
            {
            case AZ::RHI::Format::R8_SINT:
            {
                return mem[index] / static_cast<AZ::s32>(std::numeric_limits<AZ::s8>::max());
            }
            case AZ::RHI::Format::R16_SINT:
            {
                auto actualMem = reinterpret_cast<const AZ::s16*>(mem);
                return actualMem[index] / static_cast<AZ::s32>(std::numeric_limits<AZ::s16>::max());
            }
            case AZ::RHI::Format::R32_SINT:
            {
                auto actualMem = reinterpret_cast<const AZ::s32*>(mem);
                return actualMem[index];
            }
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
            AZStd::array<T, 1> values = { aznumeric_cast<T>(0) };

            auto position = AZStd::make_pair(x, y);
            AZStd::span<T> valueSpan(values.begin(), values.size());
            GetSubImagePixelValues(position, position, valueSpan, componentIndex, mip, slice);

            return values[0];
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

        void StreamingImageAsset::GetSubImagePixelValues(AZStd::pair<uint32_t, uint32_t> topLeft, AZStd::pair<uint32_t, uint32_t> bottomRight, AZStd::span<float> outValues, uint32_t componentIndex, uint32_t mip, uint32_t slice)
        {
            // TODO: Use the component index
            (void)componentIndex;

            auto imageData = GetSubImageData(mip, slice);

            if (!imageData.empty())
            {
                const AZ::RHI::ImageDescriptor imageDescriptor = GetImageDescriptor();
                auto width = imageDescriptor.m_size.m_width;
                const uint32_t pixelSize = AZ::RHI::GetFormatSize(imageDescriptor.m_format);

                size_t outValuesIndex = 0;
                for (uint32_t y = topLeft.second; y <= bottomRight.second; ++y)
                {
                    for (uint32_t x = topLeft.first; x <= bottomRight.first; ++x)
                    {
                        size_t imageDataIndex = (y * width + x) * pixelSize;

                        auto& outValue = outValues[outValuesIndex++];
                        outValue = Internal::RetrieveFloatValue(imageData.data(), imageDataIndex, imageDescriptor.m_format);
                    }
                }
            }
        }

        void StreamingImageAsset::GetSubImagePixelValues(AZStd::pair<uint32_t, uint32_t> topLeft, AZStd::pair<uint32_t, uint32_t> bottomRight, AZStd::span<AZ::u32> outValues, uint32_t componentIndex, uint32_t mip, uint32_t slice)
        {
            // TODO: Use the component index
            (void)componentIndex;

            auto imageData = GetSubImageData(mip, slice);

            if (!imageData.empty())
            {
                const AZ::RHI::ImageDescriptor imageDescriptor = GetImageDescriptor();
                auto width = imageDescriptor.m_size.m_width;
                const uint32_t pixelSize = AZ::RHI::GetFormatSize(imageDescriptor.m_format);

                size_t outValuesIndex = 0;
                for (uint32_t y = topLeft.second; y <= bottomRight.second; ++y)
                {
                    for (uint32_t x = topLeft.first; x <= bottomRight.first; ++x)
                    {
                        size_t imageDataIndex = (y * width + x) * pixelSize;

                        auto& outValue = outValues[outValuesIndex++];
                        outValue = Internal::RetrieveUintValue(imageData.data(), imageDataIndex, imageDescriptor.m_format);
                    }
                }
            }
        }

        void StreamingImageAsset::GetSubImagePixelValues(AZStd::pair<uint32_t, uint32_t> topLeft, AZStd::pair<uint32_t, uint32_t> bottomRight, AZStd::span<AZ::s32> outValues, uint32_t componentIndex, uint32_t mip, uint32_t slice)
        {
            // TODO: Use the component index
            (void)componentIndex;

            auto imageData = GetSubImageData(mip, slice);

            if (!imageData.empty())
            {
                const AZ::RHI::ImageDescriptor imageDescriptor = GetImageDescriptor();
                auto width = imageDescriptor.m_size.m_width;
                const uint32_t pixelSize = AZ::RHI::GetFormatSize(imageDescriptor.m_format);

                size_t outValuesIndex = 0;
                for (uint32_t y = topLeft.second; y <= bottomRight.second; ++y)
                {
                    for (uint32_t x = topLeft.first; x <= bottomRight.first; ++x)
                    {
                        size_t imageDataIndex = (y * width + x) * pixelSize;

                        auto& outValue = outValues[outValuesIndex++];
                        outValue = Internal::RetrieveIntValue(imageData.data(), imageDataIndex, imageDescriptor.m_format);
                    }
                }
            }
        }
    }
}
