/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GradientSignal_precompiled.h"

#include <GradientSignal/GradientImageConversion.h>
#include <Atom/ImageProcessing/PixelFormats.h>

namespace
{
    namespace ChannelID
    {
        constexpr auto R = 0;
        constexpr auto G = 1;
        constexpr auto B = 2;
        constexpr auto A = 3;
    }

    ImageProcessingAtom::EPixelFormat ExportFormatToPixelFormat(GradientSignal::ExportFormat format)
    {
        using namespace GradientSignal;
        using namespace ImageProcessingAtom;

        switch (format)
        {
        case ExportFormat::U8:
            return ePixelFormat_R8;

        case ExportFormat::U16:
            return ePixelFormat_R16;

        case ExportFormat::U32:
            return ePixelFormat_R32;

        case ExportFormat::F32:
            return ePixelFormat_R32F;

        default:
            return EPixelFormat::ePixelFormat_Unknown;
        }
    }

    template <ImageProcessingAtom::EPixelFormat>
    struct Underlying {};

    template <>
    struct Underlying<ImageProcessingAtom::EPixelFormat::ePixelFormat_R8>
    {
        using type = AZ::u8;
    };

    template <>
    struct Underlying<ImageProcessingAtom::EPixelFormat::ePixelFormat_R16>
    {
        using type = AZ::u16;
    };

    template <>
    struct Underlying<ImageProcessingAtom::EPixelFormat::ePixelFormat_R32>
    {
        using type = AZ::u32;
    };

    template <>
    struct Underlying<ImageProcessingAtom::EPixelFormat::ePixelFormat_R32F>
    {
        using type = float;
    };

    template <typename A, typename B, typename C>
    A Lerp(A a, B b, C c)
    {
        return aznumeric_cast<A>((1 - c) * a + (c * b));
    }

    template <typename Target>
    Target ScaleToTypeRange(double scaleFactor)
    {
        scaleFactor = AZStd::clamp(scaleFactor, 0.0, 1.0);

        if constexpr (AZStd::is_floating_point_v<Target>)
        {
            return aznumeric_cast<Target>(scaleFactor);
        }
        else
        {
            return aznumeric_cast<Target>(Lerp(aznumeric_cast<double>(std::numeric_limits<Target>::lowest()),
                aznumeric_cast<double>(std::numeric_limits<Target>::max()), scaleFactor));
        }
    }

    template <typename Target, typename T>
    Target GetNormalScaled(T value, AZStd::pair<float, float> range)
    {
        // Normalize a value between 0 and 1
        double scaleFactor = (value - aznumeric_cast<double>(range.first)) / (aznumeric_cast<double>(range.second) - aznumeric_cast<double>(range.first));
        return ScaleToTypeRange<Target>(scaleFactor);
    }

    template <typename T>
    AZStd::pair<float, float> GetRange(const AZStd::vector<AZ::u8>& buffer)
    {
        auto bPtr = reinterpret_cast<const T*>(buffer.data());
        auto bEndPtr = reinterpret_cast<const T*>(buffer.data() + buffer.size());
        auto [min, max] = AZStd::minmax_element(bPtr, bEndPtr);
        return AZStd::make_pair(aznumeric_cast<float>(*min), aznumeric_cast<float>(*max));
    }

    template <typename Old, typename New>
    void ConvertBufferType(AZStd::vector<AZ::u8>& buffer, bool autoScale, AZStd::pair<float, float> userRange)
    {
        AZStd::size_t numElems = buffer.size() / sizeof(Old);
        AZStd::vector<AZ::u8> newBuffer(numElems * sizeof(New));

        auto bPtr = reinterpret_cast<const Old*>(buffer.data());
        auto bEndPtr = bPtr + numElems;
        auto nbPtr = reinterpret_cast<New*>(newBuffer.data());

        if (autoScale)
        {
            userRange = GetRange<Old>(buffer);
        }
        else
        {
            // Validate user input
            userRange.first = AZStd::clamp(userRange.first, aznumeric_cast<float>(std::numeric_limits<Old>::lowest()),
                aznumeric_cast<float>(std::numeric_limits<Old>::max()));
            userRange.second = AZStd::clamp(userRange.second, aznumeric_cast<float>(std::numeric_limits<Old>::lowest()),
                aznumeric_cast<float>(std::numeric_limits<Old>::max()));
            userRange.second = AZStd::max(userRange.second, userRange.first);
        }

        // Account for case where range.first equals range.second to prevent division by 0.
        if (AZ::GetAbs(userRange.second - userRange.first) < std::numeric_limits<float>::epsilon())
        {
            if (!autoScale)
            {
                AZ_Warning("Buffer Type Conversion", false, "Check min and max ranges! Max cannot be less than or equal to min.");
            }

            while (bPtr != bEndPtr)
            {
                *nbPtr = ScaleToTypeRange<New>(1.0);
                ++bPtr;
                ++nbPtr;
            }
        }
        else
        {
            while (bPtr != bEndPtr)
            {
                *nbPtr = GetNormalScaled<New>(*bPtr, userRange);
                ++bPtr;
                ++nbPtr;
            }
        }

        buffer = AZStd::move(newBuffer);
    }

    template <ImageProcessingAtom::EPixelFormat Old>
    void ConvertBufferType(AZStd::vector<AZ::u8>& buffer, ImageProcessingAtom::EPixelFormat newFormat, bool autoScale, AZStd::pair<float, float> userRange)
    {
        using namespace ImageProcessingAtom;

        switch (newFormat)
        {
        case ePixelFormat_R8:
            ConvertBufferType<typename Underlying<Old>::type, Underlying<ePixelFormat_R8>::type>(buffer, autoScale, userRange);
            break;

        case ePixelFormat_R16:
            ConvertBufferType<typename Underlying<Old>::type, Underlying<ePixelFormat_R16>::type>(buffer, autoScale, userRange);
            break;

        case ePixelFormat_R32:
            ConvertBufferType<typename Underlying<Old>::type, Underlying<ePixelFormat_R32>::type>(buffer, autoScale, userRange);
            break;

        case ePixelFormat_R32F:
            ConvertBufferType<typename Underlying<Old>::type, Underlying<ePixelFormat_R32F>::type>(buffer, autoScale, userRange);
            break;

        default:
            break;
        }
    }

    ImageProcessingAtom::EPixelFormat ConvertBufferType(AZStd::vector<AZ::u8>& buffer, ImageProcessingAtom::EPixelFormat old, ImageProcessingAtom::EPixelFormat newFormat, bool autoScale, AZStd::pair<float, float> userRange)
    {
        using namespace ImageProcessingAtom;

        switch (old)
        {
        case ePixelFormat_R8:
            ConvertBufferType<ePixelFormat_R8>(buffer, newFormat, autoScale, userRange);
            break;

        case ePixelFormat_R16:
            ConvertBufferType<ePixelFormat_R16>(buffer, newFormat, autoScale, userRange);
            break;

        case ePixelFormat_R32:
            ConvertBufferType<ePixelFormat_R32>(buffer, newFormat, autoScale, userRange);
            break;

        case ePixelFormat_R32F:
            ConvertBufferType<ePixelFormat_R32F>(buffer, newFormat, autoScale, userRange);
            break;

        default:
            break;
        }

        return newFormat;
    }
    
    AZ::u8 IsActive(AZ::u8 index, AZ::u8 mask)
    {
        return (mask & (1 << index)) != 0;
    }

    template <typename T>
    T AlphaOp(T val, const T* buffer, GradientSignal::AlphaExportTransform op)
    {
        switch (op)
        {
        case GradientSignal::AlphaExportTransform::MULTIPLY:
        {
            if constexpr (AZStd::is_floating_point_v<T>)
            {
                return val * buffer[ChannelID::A];
            }
            else
            {
                return aznumeric_cast<T>(val * (aznumeric_cast<double>(buffer[ChannelID::A]) / std::numeric_limits<T>::max()));
            }
        }

        case GradientSignal::AlphaExportTransform::ADD:
            return aznumeric_cast<T>(val + buffer[ChannelID::A]);

        case GradientSignal::AlphaExportTransform::SUBTRACT:
            return aznumeric_cast<T>(val - buffer[ChannelID::A]);

        default:
            return val;
        }
    }

    template <typename T>
    T GetMin(const T* arr, GradientSignal::ChannelMask mask, AZStd::size_t channels)
    {
        T min = std::numeric_limits<T>::max();

        for (AZStd::size_t i = 0; i < channels; ++i)
        {
            min = AZStd::min(min, Lerp(min, 
                arr[i], IsActive(i, mask)));
        }

        return min;
    }

    template <typename T>
    T GetMax(const T* arr, GradientSignal::ChannelMask mask, AZStd::size_t channels)
    {
        T max = std::numeric_limits<T>::lowest();

        for (AZStd::size_t i = 0; i < channels; ++i)
        {
            max = AZStd::max(max, Lerp(max, 
                arr[i], IsActive(i, mask)));
        }

        return max;
    }

    template <typename T>
    T GetAverage(const T* arr, GradientSignal::ChannelMask mask, AZStd::size_t channels)
    {
        double total = 0.0;
        AZStd::size_t active = 0;

        for (AZStd::size_t i = 0; i < channels; ++i)
        {
            AZ::u8 result = IsActive(i, mask);
            total += result * arr[i];
            active += result;
        }

        total /= AZStd::max(active, aznumeric_cast<AZStd::size_t>(1));

        return aznumeric_cast<T>(total);
    }

    template <typename T>
    T GetTerrarium(const T* arr, GradientSignal::ChannelMask mask, AZStd::size_t channels)
    {
        if (channels < 3 || !IsActive(ChannelID::R, mask) || !IsActive(ChannelID::G, mask) || !IsActive(ChannelID::B, mask))
        {
            AZ_Error("GradientImageConversion", false, "RGB channels must be present and active!");
            return aznumeric_cast<T>(0);
        }
        else
        {
            /*
                "Terrarium" is an image-based terrain file format as defined here:  https://www.mapzen.com/blog/terrain-tile-service/
                According to the website:  "Terrarium format PNG tiles contain raw elevation data in meters, in Mercator projection (EPSG:3857).
                All values are positive with a 32,768 offset, split into the red, green, and blue channels, with 16 bits of integer and 8 bits of fraction. To decode:  (red * 256 + green + blue / 256) - 32768"
                This gives a range -32768 to 32768 meters at a constant 1/256 meter resolution. For reference, the lowest point on Earth (Mariana Trench) is at -10911 m, and the highest point (Mt Everest) is at 8848 m.
            */
            return aznumeric_cast<T>((arr[ChannelID::R] * 256.0) + arr[ChannelID::G] + (arr[ChannelID::B] / 256.0) - 32768.0);
        }
    }

    template <typename T>
    void TransformBuffer(AZStd::size_t channels, GradientSignal::ChannelMask mask, GradientSignal::AlphaExportTransform alphaTransform,
        AZStd::vector<AZ::u8>& mem, const AZStd::function<T (T* arr, GradientSignal::ChannelMask mask,
            AZStd::size_t channels)>& transformFunc)
    {
        auto begin = reinterpret_cast<T*>(mem.data());
        auto end = reinterpret_cast<T*>(mem.data() + mem.size());
        auto out = begin;

        auto activeChannels = AZStd::min(channels, aznumeric_cast<AZStd::size_t>(3));

        if (channels < 4 || !IsActive(ChannelID::A, mask))
        {
            while (begin < end)
            {
                *out = transformFunc(begin, mask, activeChannels);
                std::advance(out, 1);
                std::advance(begin, channels);
            }
        }
        else
        {
            while (begin < end)
            {
                *out = AlphaOp(transformFunc(begin, mask, activeChannels), begin, alphaTransform);
                std::advance(out, 1);
                std::advance(begin, channels);
            }
        }

        mem.resize(mem.size() / channels);
    }

    AZStd::size_t GetChannels(ImageProcessingAtom::EPixelFormat format)
    {
        using namespace ImageProcessingAtom;

        switch (format)
        {
        case ePixelFormat_R8G8:
        case ePixelFormat_R16G16:
        case ePixelFormat_R32G32F:
            return 2;

        case ePixelFormat_R16G16B16A16:
        case ePixelFormat_R8G8B8A8:
        case ePixelFormat_R8G8B8X8:
        case ePixelFormat_R32G32B32A32F:
            return 4;

        default:
            return 0;
        }
    }

    template <template <typename> typename Op>
    ImageProcessingAtom::EPixelFormat CallHelper(ImageProcessingAtom::EPixelFormat format,
        GradientSignal::ChannelMask mask, GradientSignal::AlphaExportTransform alphaTransform, AZStd::vector<AZ::u8>& mem)
    {
        using namespace ImageProcessingAtom;

        switch (format)
        {
        case ePixelFormat_R8G8:
        case ePixelFormat_R8G8B8A8:
        case ePixelFormat_R8G8B8X8:
            TransformBuffer<AZ::u8>(GetChannels(format), mask, alphaTransform, mem, Op<AZ::u8>{});
            return ePixelFormat_R8;

        case ePixelFormat_R16G16B16A16:
        case ePixelFormat_R16G16:
            TransformBuffer<AZ::u16>(GetChannels(format), mask, alphaTransform, mem, Op<AZ::u16>{});
            return ePixelFormat_R16;

        case ePixelFormat_R32G32B32A32F:
        case ePixelFormat_R32G32F:
            TransformBuffer<float>(GetChannels(format), mask, alphaTransform, mem, Op<float>{});
            return ePixelFormat_R32F;

        default:
            return format;
        }
    }

    template <typename T>
    struct AverageFuncCaller
    {
        T operator()(T* arr, GradientSignal::ChannelMask mask,
            AZStd::size_t channels) const
        {
            return GetAverage(arr, mask, channels);
        }
    };

    template <typename T>
    struct MinFuncCaller
    {
        T operator()(T* arr, GradientSignal::ChannelMask mask,
            AZStd::size_t channels) const
        {
            return GetMin(arr, mask, channels);
        }
    };

    template <typename T>
    struct MaxFuncCaller
    {
        T operator()(T* arr, GradientSignal::ChannelMask mask,
            AZStd::size_t channels) const
        {
            return GetMax(arr, mask, channels);
        }
    };

    template <typename T>
    struct TerrariumFuncCaller
    {
        T operator()(T* arr, GradientSignal::ChannelMask mask,
            AZStd::size_t channels) const
        {
            return GetTerrarium(arr, mask, channels);
        }
    };

    ImageProcessingAtom::EPixelFormat OperationHelper(GradientSignal::ChannelExportTransform op, ImageProcessingAtom::EPixelFormat format,
        GradientSignal::ChannelMask mask, GradientSignal::AlphaExportTransform alphaTransform, AZStd::vector<AZ::u8>& mem)
    {
        switch (op)
        {
        case GradientSignal::ChannelExportTransform::AVERAGE:
            return CallHelper<AverageFuncCaller>(format, mask, alphaTransform, mem);

        case GradientSignal::ChannelExportTransform::MIN:
            return CallHelper<MinFuncCaller>(format, mask, alphaTransform, mem);

        case GradientSignal::ChannelExportTransform::MAX:
            return CallHelper<MaxFuncCaller>(format, mask, alphaTransform, mem);

        case GradientSignal::ChannelExportTransform::TERRARIUM:
            return CallHelper<TerrariumFuncCaller>(format, mask, alphaTransform, mem);

        default:
            return format;
        }
    }
}

namespace GradientSignal
{
    AZStd::unique_ptr<ImageAsset> ConvertImage(const ImageAsset& image, const ImageSettings& settings)
    {
        auto newAsset = AZStd::make_unique<ImageAsset>();

        newAsset->m_imageWidth = image.m_imageWidth;
        newAsset->m_imageHeight = image.m_imageHeight;
        newAsset->m_bytesPerPixel = image.m_bytesPerPixel;
        newAsset->m_imageFormat = image.m_imageFormat;
        newAsset->m_imageData = image.m_imageData;

        if (image.m_imageData.empty())
        {
            return newAsset;
        }

        // ChannelMask is 8 bits, and the bits are assumed to be as follows: 0b0000ABGR
        auto mask = static_cast<ChannelMask>
        (
            static_cast<AZ::u8>(settings.m_useA) << 3 |
            static_cast<AZ::u8>(settings.m_useB) << 2 |
            static_cast<AZ::u8>(settings.m_useG) << 1 |
            static_cast<AZ::u8>(settings.m_useR)
        );

        newAsset->m_imageFormat = OperationHelper(settings.m_rgbTransform, newAsset->m_imageFormat, mask, settings.m_alphaTransform, newAsset->m_imageData);
        newAsset->m_imageFormat = ConvertBufferType(newAsset->m_imageData, newAsset->m_imageFormat, ExportFormatToPixelFormat(settings.m_format), 
            settings.m_autoScale, AZStd::make_pair(settings.m_scaleRangeMin, settings.m_scaleRangeMax));
        newAsset->m_bytesPerPixel = newAsset->m_imageData.size() / aznumeric_cast<AZStd::size_t>(newAsset->m_imageWidth * newAsset->m_imageHeight);

        return newAsset;
    }
}
