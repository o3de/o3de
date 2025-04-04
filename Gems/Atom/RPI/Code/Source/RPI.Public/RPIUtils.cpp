/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RHI/RHISystemInterface.h>

#include <Atom/RPI.Reflect/Asset/AssetUtils.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>

#include <Atom/RPI.Public/BlockCompression.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/RPIUtils.h>
#include <Atom/RPI.Public/Shader/Shader.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/WindowContext.h>

#include <AzCore/Math/Color.h>
#include <AzCore/std/containers/array.h>
#include <AzFramework/Asset/AssetSystemBus.h>

namespace AZ
{
    namespace RPI
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

            float ScaleValue(float value, float origMin, float origMax, float scaledMin, float scaledMax)
            {
                // This method assumes that origMin <= value <= origMax. Since this is a private helper method only
                // used by ScaleSNorm8Value and ScaleSNorm16Value below, we'll avoid using asserts to verify the
                // condition in the interest of performance in assert-enabled builds.
                return ((value - origMin) / (origMax - origMin)) * (scaledMax - scaledMin) + scaledMin;
            }

            float ScaleSNorm8Value(int8_t value)
            {
                // Scale the value from AZ::s8 min/max to -1 to 1
                // We need to treat -128 and -127 the same, so that we get a symmetric
                // range of -127 to 127 with complementary scaled values of -1 to 1
                constexpr float signedMax = static_cast<float>(AZStd::numeric_limits<int8_t>::max());
                constexpr float signedMin = -signedMax;
                return ScaleValue(AZStd::max(aznumeric_cast<float>(value), signedMin), signedMin, signedMax, -1.0f, 1.0f);
            }

            float ScaleSNorm16Value(int16_t value)
            {
                // Scale the value from AZ::s16 min/max to -1 to 1
                // We need to treat -32768 and -32767 the same, so that we get a symmetric
                // range of -32767 to 32767 with complementary scaled values of -1 to 1
                constexpr float signedMax = static_cast<float>(AZStd::numeric_limits<int16_t>::max());
                constexpr float signedMin = -signedMax;
                return ScaleValue(AZStd::max(aznumeric_cast<float>(value), signedMin), signedMin, signedMax, -1.0f, 1.0f);
            }

            // Pre-compute a lookup table for converting SRGB gamma to linear
            // by specifying the AZ::u8 so we don't have to do the computation
            // when retrieving pixels
            using ConversionLookupTable = AZStd::array<float, 256>;
            ConversionLookupTable CreateSrgbGammaToLinearLookupTable()
            {
                ConversionLookupTable lookupTable;

                for (size_t i = 0; i < lookupTable.array_size; ++i)
                {
                    float srgbValue = i / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max());
                    lookupTable[i] = AZ::Color::ConvertSrgbGammaToLinear(srgbValue);
                }

                return lookupTable;
            }

            static ConversionLookupTable s_SrgbGammaToLinearLookupTable = CreateSrgbGammaToLinearLookupTable();

            float RetrieveFloatValue(
                const AZ::u8* mem, AZStd::pair<size_t, size_t> indices, uint32_t componentIndex, AZ::RHI::Format format)
            {
                switch (format)
                {
                case AZ::RHI::Format::R8_UNORM:
                case AZ::RHI::Format::A8_UNORM:
                case AZ::RHI::Format::R8G8_UNORM:
                case AZ::RHI::Format::R8G8B8A8_UNORM:
                case AZ::RHI::Format::A8B8G8R8_UNORM:
                    {
                        return mem[indices.first + componentIndex] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max());
                    }
                case AZ::RHI::Format::R8_UNORM_SRGB:
                case AZ::RHI::Format::R8G8_UNORM_SRGB:
                case AZ::RHI::Format::R8G8B8A8_UNORM_SRGB:
                case AZ::RHI::Format::A8B8G8R8_UNORM_SRGB:
                    {
                        // Use a lookup table that takes an AZ::u8 instead of a float
                        // for better performance
                        return s_SrgbGammaToLinearLookupTable[mem[indices.first + componentIndex]];
                    }
                case AZ::RHI::Format::R8_SNORM:
                case AZ::RHI::Format::R8G8_SNORM:
                case AZ::RHI::Format::R8G8B8A8_SNORM:
                case AZ::RHI::Format::A8B8G8R8_SNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s8*>(mem);
                        return ScaleSNorm8Value(actualMem[indices.first + componentIndex]);
                    }
                case AZ::RHI::Format::D16_UNORM:
                case AZ::RHI::Format::R16_UNORM:
                case AZ::RHI::Format::R16G16_UNORM:
                case AZ::RHI::Format::R16G16B16A16_UNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::u16*>(mem);
                        return actualMem[indices.first + componentIndex] / static_cast<float>(AZStd::numeric_limits<AZ::u16>::max());
                    }
                case AZ::RHI::Format::R16_SNORM:
                case AZ::RHI::Format::R16G16_SNORM:
                case AZ::RHI::Format::R16G16B16A16_SNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s16*>(mem);
                        return ScaleSNorm16Value(actualMem[indices.first + componentIndex]);
                    }
                case AZ::RHI::Format::R16_FLOAT:
                case AZ::RHI::Format::R16G16_FLOAT:
                case AZ::RHI::Format::R16G16B16A16_FLOAT:
                    {
                        auto actualMem = reinterpret_cast<const float*>(mem);
                        return SHalf(actualMem[indices.first + componentIndex]);
                    }
                case AZ::RHI::Format::D32_FLOAT:
                case AZ::RHI::Format::R32_FLOAT:
                case AZ::RHI::Format::R32G32_FLOAT:
                case AZ::RHI::Format::R32G32B32_FLOAT:
                case AZ::RHI::Format::R32G32B32A32_FLOAT:
                    {
                        auto actualMem = reinterpret_cast<const float*>(mem);
                        return actualMem[indices.first + componentIndex];
                    }
                case AZ::RHI::Format::BC1_UNORM:
                    {
                        auto actualMem = reinterpret_cast<const BC1Block*>(mem);
                        return actualMem[indices.first].GetBlockColor(indices.second).GetElement(componentIndex);
                    }
                case AZ::RHI::Format::BC1_UNORM_SRGB:
                    {
                        auto actualMem = reinterpret_cast<const BC1Block*>(mem);
                        float color = actualMem[indices.first].GetBlockColor(indices.second).GetElement(componentIndex);
                        return s_SrgbGammaToLinearLookupTable[aznumeric_cast<uint8_t>(color * AZStd::numeric_limits<AZ::u8>::max())];
                    }
                case AZ::RHI::Format::BC4_UNORM:
                    {
                        auto actualMem = reinterpret_cast<const BC4Block*>(mem);
                        return actualMem[indices.first].GetBlockColor(indices.second).GetElement(componentIndex);
                    }
                default:
                    AZ_Assert(false, "Unsupported pixel format: %s", AZ::RHI::ToString(format));
                    return 0.0f;
                }
            }

            AZ::Color RetrieveColorValue(const AZ::u8* mem, AZStd::pair<size_t, size_t> indices, AZ::RHI::Format format)
            {
                switch (format)
                {
                case AZ::RHI::Format::R8_UNORM:
                    {
                        return AZ::Color(mem[indices.first] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()), 0.0f, 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::A8_UNORM:
                    {
                        return AZ::Color(0.0f, 0.0f, 0.0f, mem[indices.first] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()));
                    }
                case AZ::RHI::Format::R8G8_UNORM:
                    {
                        return AZ::Color(
                            mem[indices.first + 0] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()),
                            mem[indices.first + 1] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()),
                            0.0f,
                            1.0f);
                    }
                case AZ::RHI::Format::R8G8B8A8_UNORM:
                    {
                        return AZ::Color(
                            mem[indices.first + 0] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()),
                            mem[indices.first + 1] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()),
                            mem[indices.first + 2] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()),
                            mem[indices.first + 3] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()));
                    }
                case AZ::RHI::Format::A8B8G8R8_UNORM:
                    {
                        return AZ::Color(
                            mem[indices.first + 3] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()),
                            mem[indices.first + 2] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()),
                            mem[indices.first + 1] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()),
                            mem[indices.first + 0] / static_cast<float>(AZStd::numeric_limits<AZ::u8>::max()));
                    }
                case AZ::RHI::Format::R8_UNORM_SRGB:
                    {
                        return AZ::Color(s_SrgbGammaToLinearLookupTable[mem[indices.first]], 0.0f, 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::R8G8_UNORM_SRGB:
                    {
                        return AZ::Color(
                            s_SrgbGammaToLinearLookupTable[mem[indices.first + 0]],
                            s_SrgbGammaToLinearLookupTable[mem[indices.first + 1]],
                            0.0f,
                            1.0f);
                    }
                case AZ::RHI::Format::R8G8B8A8_UNORM_SRGB:
                    {
                        return AZ::Color(
                            s_SrgbGammaToLinearLookupTable[mem[indices.first + 0]],
                            s_SrgbGammaToLinearLookupTable[mem[indices.first + 1]],
                            s_SrgbGammaToLinearLookupTable[mem[indices.first + 2]],
                            s_SrgbGammaToLinearLookupTable[mem[indices.first + 3]]);
                    }
                case AZ::RHI::Format::A8B8G8R8_UNORM_SRGB:
                    {
                        return AZ::Color(
                            s_SrgbGammaToLinearLookupTable[mem[indices.first + 3]],
                            s_SrgbGammaToLinearLookupTable[mem[indices.first + 2]],
                            s_SrgbGammaToLinearLookupTable[mem[indices.first + 1]],
                            s_SrgbGammaToLinearLookupTable[mem[indices.first + 0]]);
                    }
                case AZ::RHI::Format::R8_SNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s8*>(mem);
                        return AZ::Color(ScaleSNorm8Value(actualMem[indices.first]), 0.0f, 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::R8G8_SNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s8*>(mem);
                        return AZ::Color(
                            ScaleSNorm8Value(actualMem[indices.first + 0]), ScaleSNorm8Value(actualMem[indices.first + 1]), 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::R8G8B8A8_SNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s8*>(mem);
                        return AZ::Color(
                            ScaleSNorm8Value(actualMem[indices.first + 0]),
                            ScaleSNorm8Value(actualMem[indices.first + 1]),
                            ScaleSNorm8Value(actualMem[indices.first + 2]),
                            ScaleSNorm8Value(actualMem[indices.first + 3]));
                    }
                case AZ::RHI::Format::A8B8G8R8_SNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s8*>(mem);
                        return AZ::Color(
                            ScaleSNorm8Value(actualMem[indices.first + 3]),
                            ScaleSNorm8Value(actualMem[indices.first + 2]),
                            ScaleSNorm8Value(actualMem[indices.first + 1]),
                            ScaleSNorm8Value(actualMem[indices.first + 0]));
                    }
                case AZ::RHI::Format::D16_UNORM:
                case AZ::RHI::Format::R16_UNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::u16*>(mem);
                        return AZ::Color(
                            actualMem[indices.first] / static_cast<float>(AZStd::numeric_limits<AZ::u16>::max()), 0.0f, 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::R16G16_UNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::u16*>(mem);
                        return AZ::Color(
                            actualMem[indices.first + 0] / static_cast<float>(AZStd::numeric_limits<AZ::u16>::max()),
                            actualMem[indices.first + 1] / static_cast<float>(AZStd::numeric_limits<AZ::u16>::max()),
                            0.0f,
                            1.0f);
                    }
                case AZ::RHI::Format::R16G16B16A16_UNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::u16*>(mem);
                        return AZ::Color(
                            actualMem[indices.first + 0] / static_cast<float>(AZStd::numeric_limits<AZ::u16>::max()),
                            actualMem[indices.first + 1] / static_cast<float>(AZStd::numeric_limits<AZ::u16>::max()),
                            actualMem[indices.first + 2] / static_cast<float>(AZStd::numeric_limits<AZ::u16>::max()),
                            actualMem[indices.first + 3] / static_cast<float>(AZStd::numeric_limits<AZ::u16>::max()));
                    }
                case AZ::RHI::Format::R16_SNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s16*>(mem);
                        return AZ::Color(ScaleSNorm16Value(actualMem[indices.first]), 0.0f, 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::R16G16_SNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s16*>(mem);
                        return AZ::Color(
                            ScaleSNorm16Value(actualMem[indices.first + 0]), ScaleSNorm16Value(actualMem[indices.first + 1]), 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::R16G16B16A16_SNORM:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s16*>(mem);
                        return AZ::Color(
                            ScaleSNorm16Value(actualMem[indices.first + 0]),
                            ScaleSNorm16Value(actualMem[indices.first + 1]),
                            ScaleSNorm16Value(actualMem[indices.first + 2]),
                            ScaleSNorm16Value(actualMem[indices.first + 3]));
                    }
                case AZ::RHI::Format::R16_FLOAT:
                    {
                        auto actualMem = reinterpret_cast<const float*>(mem);
                        return AZ::Color(SHalf(actualMem[indices.first]), 0.0f, 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::R16G16_FLOAT:
                    {
                        auto actualMem = reinterpret_cast<const float*>(mem);
                        return AZ::Color(SHalf(actualMem[indices.first + 0]), SHalf(actualMem[indices.first + 1]), 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::R16G16B16A16_FLOAT:
                    {
                        auto actualMem = reinterpret_cast<const float*>(mem);
                        return AZ::Color(
                            SHalf(actualMem[indices.first + 0]),
                            SHalf(actualMem[indices.first + 1]),
                            SHalf(actualMem[indices.first + 2]),
                            SHalf(actualMem[indices.first + 3]));
                    }
                case AZ::RHI::Format::D32_FLOAT:
                case AZ::RHI::Format::R32_FLOAT:
                    {
                        auto actualMem = reinterpret_cast<const float*>(mem);
                        return AZ::Color(actualMem[indices.first], 0.0f, 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::R32G32_FLOAT:
                    {
                        auto actualMem = reinterpret_cast<const float*>(mem);
                        return AZ::Color(actualMem[indices.first + 0], actualMem[indices.first + 1], 0.0f, 1.0f);
                    }
                case AZ::RHI::Format::R32G32B32_FLOAT:
                    {
                        auto actualMem = reinterpret_cast<const float*>(mem);
                        return AZ::Color(actualMem[indices.first + 0], actualMem[indices.first + 1], actualMem[indices.first + 2], 1.0f);
                    }
                case AZ::RHI::Format::R32G32B32A32_FLOAT:
                    {
                        auto actualMem = reinterpret_cast<const float*>(mem);
                        return AZ::Color(
                            actualMem[indices.first + 0],
                            actualMem[indices.first + 1],
                            actualMem[indices.first + 2],
                            actualMem[indices.first + 3]);
                    }
                case AZ::RHI::Format::BC1_UNORM:
                    {
                        auto actualMem = reinterpret_cast<const BC1Block*>(mem);
                        return actualMem[indices.first].GetBlockColor(indices.second);
                    }
                case AZ::RHI::Format::BC1_UNORM_SRGB:
                    {
                        auto actualMem = reinterpret_cast<const BC1Block*>(mem);
                        AZ::Color color = actualMem[indices.first].GetBlockColor(indices.second);
                        return AZ::Color(
                            s_SrgbGammaToLinearLookupTable[aznumeric_cast<uint8_t>(color.GetR() * AZStd::numeric_limits<AZ::u8>::max())],
                            s_SrgbGammaToLinearLookupTable[aznumeric_cast<uint8_t>(color.GetG() * AZStd::numeric_limits<AZ::u8>::max())],
                            s_SrgbGammaToLinearLookupTable[aznumeric_cast<uint8_t>(color.GetB() * AZStd::numeric_limits<AZ::u8>::max())],
                            s_SrgbGammaToLinearLookupTable[aznumeric_cast<uint8_t>(color.GetA() * AZStd::numeric_limits<AZ::u8>::max())]);
                    }
                case AZ::RHI::Format::BC4_UNORM:
                    {
                        auto actualMem = reinterpret_cast<const BC4Block*>(mem);
                        return actualMem[indices.first].GetBlockColor(indices.second);
                    }
                default:
                    AZ_Assert(false, "Unsupported pixel format: %s", AZ::RHI::ToString(format));
                    return AZ::Color::CreateZero();
                }
            }

            AZ::u32 RetrieveUintValue(
                const AZ::u8* mem, AZStd::pair<size_t, size_t> indices, uint32_t componentIndex, AZ::RHI::Format format)
            {
                switch (format)
                {
                case AZ::RHI::Format::R8_UINT:
                case AZ::RHI::Format::R8G8_UINT:
                case AZ::RHI::Format::R8G8B8A8_UINT:
                    {
                        return mem[indices.first + componentIndex] / static_cast<AZ::u32>(AZStd::numeric_limits<AZ::u8>::max());
                    }
                case AZ::RHI::Format::R16_UINT:
                case AZ::RHI::Format::R16G16_UINT:
                case AZ::RHI::Format::R16G16B16A16_UINT:
                    {
                        auto actualMem = reinterpret_cast<const AZ::u16*>(mem);
                        return actualMem[indices.first + componentIndex] / static_cast<AZ::u32>(AZStd::numeric_limits<AZ::u16>::max());
                    }
                case AZ::RHI::Format::R32_UINT:
                case AZ::RHI::Format::R32G32_UINT:
                case AZ::RHI::Format::R32G32B32_UINT:
                case AZ::RHI::Format::R32G32B32A32_UINT:
                    {
                        auto actualMem = reinterpret_cast<const AZ::u32*>(mem);
                        return actualMem[indices.first + componentIndex];
                    }
                default:
                    AZ_Assert(false, "Unsupported pixel format: %s", AZ::RHI::ToString(format));
                    return 0;
                }
            }

            AZ::s32 RetrieveIntValue(
                const AZ::u8* mem, AZStd::pair<size_t, size_t> indices, uint32_t componentIndex, AZ::RHI::Format format)
            {
                switch (format)
                {
                case AZ::RHI::Format::R8_SINT:
                case AZ::RHI::Format::R8G8_SINT:
                case AZ::RHI::Format::R8G8B8A8_SINT:
                    {
                        return mem[indices.first + componentIndex] / static_cast<AZ::s32>(AZStd::numeric_limits<AZ::s8>::max());
                    }
                case AZ::RHI::Format::R16_SINT:
                case AZ::RHI::Format::R16G16_SINT:
                case AZ::RHI::Format::R16G16B16A16_SINT:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s16*>(mem);
                        return actualMem[indices.first + componentIndex] / static_cast<AZ::s32>(AZStd::numeric_limits<AZ::s16>::max());
                    }
                case AZ::RHI::Format::R32_SINT:
                case AZ::RHI::Format::R32G32_SINT:
                case AZ::RHI::Format::R32G32B32_SINT:
                case AZ::RHI::Format::R32G32B32A32_SINT:
                    {
                        auto actualMem = reinterpret_cast<const AZ::s32*>(mem);
                        return actualMem[indices.first + componentIndex];
                    }
                default:
                    AZ_Assert(false, "Unsupported pixel format: %s", AZ::RHI::ToString(format));
                    return 0;
                }
            }

            // Given an XY position, return a pair of indices that can be used to decode an individual pixel.
            // For uncompressed formats:
            //   The first index points to the start of the pixel when indexing by component type
            //   The second index is 0 (unused)
            //   Ex: an input XY of (2, 0) for an R16G16B16 format returns (6,0) because the requested pixel starts at
            //   the 6th 16-bit entry in the pixel data.
            // For compressed formats:
            //   The first index points to the start of the compressed block
            //   The second index is a relative pixel index within that block
            //   Ex: an input XY of (6, 0) with a 4x4 compressed format produces an output of (1, 2), which means use block[1] to
            //   decompress and pixel[2] within that decompressed block.
            AZStd::pair<size_t, size_t> GetImageDataIndex(const AZ::RHI::ImageDescriptor& imageDescriptor, uint32_t x, uint32_t y)
            {
                auto width = imageDescriptor.m_size.m_width;
                const uint32_t numComponents = AZ::RHI::GetFormatComponentCount(imageDescriptor.m_format);

                switch (imageDescriptor.m_format)
                {
                case AZ::RHI::Format::BC1_UNORM:
                case AZ::RHI::Format::BC1_UNORM_SRGB:
                    return BC1Block::GetBlockIndices(width, x, y);
                case AZ::RHI::Format::BC4_UNORM:
                    return BC4Block::GetBlockIndices(width, x, y);
                default:
                    return AZStd::pair<size_t, size_t>((y * width + x) * numComponents, 0);
                }
            }
        } // namespace Internal

        ViewportContextPtr GetDefaultViewportContext()
        {
            RPI::ViewportContextRequestsInterface* viewportContextManager = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get();
            if (viewportContextManager)
            {
                return viewportContextManager->GetDefaultViewportContext();
            }
            return nullptr;
        }

        WindowContextSharedPtr GetDefaultWindowContext()
        {
            ViewportContextPtr viewportContext = GetDefaultViewportContext();
            if (viewportContext)
            {
                return viewportContext->GetWindowContext();
            }
            return nullptr;
        }

        bool IsNullRenderer()
        {
            return RPI::RPISystemInterface::Get()->IsNullRenderer();
        }

        Data::AssetId GetShaderAssetId(const AZStd::string& shaderFilePath, bool isCritical)
        {
            Data::AssetId shaderAssetId;

            Data::AssetCatalogRequestBus::BroadcastResult(
                shaderAssetId,
                &Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
                shaderFilePath.c_str(),
                azrtti_typeid<ShaderAsset>(),
                false);

            if (!shaderAssetId.IsValid())
            {
                if (isCritical)
                {
                    Data::Asset<RPI::ShaderAsset> shaderAsset = RPI::AssetUtils::LoadCriticalAsset<RPI::ShaderAsset>(shaderFilePath);
                    if (shaderAsset.IsReady())
                    {
                        return shaderAsset.GetId();
                    }
                    else
                    {
                        AZ_Error("RPI Utils", false, "Could not load critical shader [%s]", shaderFilePath.c_str());
                    }
                }

                AZ_Error("RPI Utils", false, "Failed to get asset id for shader [%s]", shaderFilePath.c_str());
            }

            return shaderAssetId;
        }

        Data::Asset<ShaderAsset> FindShaderAsset(Data::AssetId shaderAssetId, [[maybe_unused]] const AZStd::string& shaderFilePath)
        {
            if (!shaderAssetId.IsValid())
            {
                return Data::Asset<ShaderAsset>();
            }

            auto shaderAsset = Data::AssetManager::Instance().GetAsset<ShaderAsset>(shaderAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

            shaderAsset.BlockUntilLoadComplete();

            if (!shaderAsset.IsReady())
            {
                AZ_Error(
                    "RPI Utils",
                    false,
                    "Failed to find shader asset [%s] with asset ID [%s]",
                    shaderFilePath.c_str(),
                    shaderAssetId.ToString<AZStd::string>().c_str());
                return Data::Asset<ShaderAsset>();
            }

            return shaderAsset;
        }

        Data::Instance<Shader> LoadShader(
            Data::AssetId shaderAssetId, const AZStd::string& shaderFilePath, const AZStd::string& supervariantName)
        {
            auto shaderAsset = FindShaderAsset(shaderAssetId, shaderFilePath);
            if (!shaderAsset.IsReady())
            {
                return nullptr;
            }

            Data::Instance<Shader> shader = Shader::FindOrCreate(shaderAsset, AZ::Name(supervariantName));
            if (!shader)
            {
                AZ_Error(
                    "RPI Utils",
                    false,
                    "Failed to find or create a shader instance from shader asset [%s] with asset ID [%s]",
                    shaderFilePath.c_str(),
                    shaderAssetId.ToString<AZStd::string>().c_str());
                return nullptr;
            }

            return shader;
        }

        Data::Asset<ShaderAsset> FindShaderAsset(const AZStd::string& shaderFilePath)
        {
            return FindShaderAsset(GetShaderAssetId(shaderFilePath), shaderFilePath);
        }

        Data::Asset<ShaderAsset> FindCriticalShaderAsset(const AZStd::string& shaderFilePath)
        {
            const bool isCritical = true;
            return FindShaderAsset(GetShaderAssetId(shaderFilePath, isCritical), shaderFilePath);
        }

        Data::Instance<Shader> LoadShader(const AZStd::string& shaderFilePath, const AZStd::string& supervariantName)
        {
            return LoadShader(GetShaderAssetId(shaderFilePath), shaderFilePath, supervariantName);
        }

        Data::Instance<Shader> LoadCriticalShader(const AZStd::string& shaderFilePath, const AZStd::string& supervariantName)
        {
            const bool isCritical = true;
            return LoadShader(GetShaderAssetId(shaderFilePath, isCritical), shaderFilePath, supervariantName);
        }

        AZ::Data::Instance<RPI::StreamingImage> LoadStreamingTexture(AZStd::string_view path)
        {
            auto streamingImageAsset = RPI::AssetUtils::LoadCriticalAsset<RPI::StreamingImageAsset>(path);

            if (!streamingImageAsset.IsReady())
            {
                AZ_Error("RPI Utils", false, "Failed to get streaming image asset: " AZ_STRING_FORMAT, AZ_STRING_ARG(path));
                return AZ::Data::Instance<RPI::StreamingImage>();
            }

            return RPI::StreamingImage::FindOrCreate(streamingImageAsset);
        }

        // Find a format for formats with two planars (DepthStencil) based on its ImageView's aspect flag
        RHI::Format FindFormatForAspect(RHI::Format format, RHI::ImageAspect imageAspect)
        {
            RHI::ImageAspectFlags imageAspectFlags = RHI::GetImageAspectFlags(format);

            // only need to convert if the source contains two aspects
            if (imageAspectFlags == RHI::ImageAspectFlags::DepthStencil)
            {
                switch (imageAspect)
                {
                case RHI::ImageAspect::Stencil:
                    return RHI::Format::R8_UINT;
                case RHI::ImageAspect::Depth:
                {
                    switch (format)
                    {
                    case RHI::Format::D32_FLOAT_S8X24_UINT:
                        return RHI::Format::R32_FLOAT;
                    case RHI::Format::D24_UNORM_S8_UINT:
                        return RHI::Format::R32_UINT;
                    case RHI::Format::D16_UNORM_S8_UINT:
                        return RHI::Format::R16_UNORM;
                    default:
                        AZ_Assert(false, "Unknown DepthStencil format. Please update this function");
                        return RHI::Format::R32_FLOAT;
                    }
                }
                }
            }
            return format;
        }

        //! A helper function for GetComputeShaderNumThreads(), to consolidate error messages, etc.
        static bool GetAttributeArgumentByIndex(
            const Data::Asset<ShaderAsset>& shaderAsset,
            const AZ::Name& attributeName,
            const RHI::ShaderStageAttributeArguments& args,
            const size_t argIndex,
            uint16_t* value,
            AZStd::string& errorMsg)
        {
            if (value)
            {
                const auto numArguments = args.size();
                if (numArguments > argIndex)
                {
                    if (args[argIndex].type() == azrtti_typeid<int>())
                    {
                        *value = aznumeric_caster(AZStd::any_cast<int>(args[argIndex]));
                    }
                    else
                    {
                        errorMsg = AZStd::string::format(
                            "Was expecting argument '%zu' in attribute '%s' to be of type 'int' from shader asset '%s'",
                            argIndex,
                            attributeName.GetCStr(),
                            shaderAsset.GetHint().c_str());
                        return false;
                    }
                }
                else
                {
                    errorMsg = AZStd::string::format(
                        "Was expecting at least '%zu' arguments in attribute '%s' from shader asset '%s'",
                        argIndex + 1,
                        attributeName.GetCStr(),
                        shaderAsset.GetHint().c_str());
                    return false;
                }
            }
            return true;
        }

        AZ::Outcome<void, AZStd::string> GetComputeShaderNumThreads(
            const Data::Asset<ShaderAsset>& shaderAsset,
            const AZ::Name& attributeName,
            uint16_t* numThreadsX,
            uint16_t* numThreadsY,
            uint16_t* numThreadsZ)
        {
            // Set default 1, 1, 1 now. In case of errors later this is what the caller will get.
            if (numThreadsX)
            {
                *numThreadsX = 1;
            }
            if (numThreadsY)
            {
                *numThreadsY = 1;
            }
            if (numThreadsZ)
            {
                *numThreadsZ = 1;
            }
            const auto numThreads = shaderAsset->GetAttribute(RHI::ShaderStage::Compute, attributeName);
            if (!numThreads)
            {
                return AZ::Failure(AZStd::string::format(
                    "Couldn't find attribute '%s' in shader asset '%s'", attributeName.GetCStr(), shaderAsset.GetHint().c_str()));
            }
            const RHI::ShaderStageAttributeArguments& args = *numThreads;
            AZStd::string errorMsg;
            if (!GetAttributeArgumentByIndex(shaderAsset, attributeName, args, 0, numThreadsX, errorMsg))
            {
                return AZ::Failure(errorMsg);
            }
            if (!GetAttributeArgumentByIndex(shaderAsset, attributeName, args, 1, numThreadsY, errorMsg))
            {
                return AZ::Failure(errorMsg);
            }
            if (!GetAttributeArgumentByIndex(shaderAsset, attributeName, args, 2, numThreadsZ, errorMsg))
            {
                return AZ::Failure(errorMsg);
            }
            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> GetComputeShaderNumThreads(
            const Data::Asset<ShaderAsset>& shaderAsset, uint16_t* numThreadsX, uint16_t* numThreadsY, uint16_t* numThreadsZ)
        {
            return GetComputeShaderNumThreads(shaderAsset, Name{ "numthreads" }, numThreadsX, numThreadsY, numThreadsZ);
        }

        AZ::Outcome<void, AZStd::string> GetComputeShaderNumThreads(
            const Data::Asset<ShaderAsset>& shaderAsset, RHI::DispatchDirect& dispatchDirect)
        {
            return GetComputeShaderNumThreads(
                shaderAsset, &dispatchDirect.m_threadsPerGroupX, &dispatchDirect.m_threadsPerGroupY, &dispatchDirect.m_threadsPerGroupZ);
        }

        bool IsImageDataPixelAPISupported(AZ::RHI::Format format)
        {
            switch (format)
            {
            // Float types
            case AZ::RHI::Format::R8_UNORM:
            case AZ::RHI::Format::A8_UNORM:
            case AZ::RHI::Format::R8G8_UNORM:
            case AZ::RHI::Format::R8G8B8A8_UNORM:
            case AZ::RHI::Format::A8B8G8R8_UNORM:
            case AZ::RHI::Format::R8_UNORM_SRGB:
            case AZ::RHI::Format::R8G8_UNORM_SRGB:
            case AZ::RHI::Format::R8G8B8A8_UNORM_SRGB:
            case AZ::RHI::Format::A8B8G8R8_UNORM_SRGB:
            case AZ::RHI::Format::R8_SNORM:
            case AZ::RHI::Format::R8G8_SNORM:
            case AZ::RHI::Format::R8G8B8A8_SNORM:
            case AZ::RHI::Format::A8B8G8R8_SNORM:
            case AZ::RHI::Format::D16_UNORM:
            case AZ::RHI::Format::R16_UNORM:
            case AZ::RHI::Format::R16G16_UNORM:
            case AZ::RHI::Format::R16G16B16A16_UNORM:
            case AZ::RHI::Format::R16_SNORM:
            case AZ::RHI::Format::R16G16_SNORM:
            case AZ::RHI::Format::R16G16B16A16_SNORM:
            case AZ::RHI::Format::R16_FLOAT:
            case AZ::RHI::Format::R16G16_FLOAT:
            case AZ::RHI::Format::R16G16B16A16_FLOAT:
            case AZ::RHI::Format::D32_FLOAT:
            case AZ::RHI::Format::R32_FLOAT:
            case AZ::RHI::Format::R32G32_FLOAT:
            case AZ::RHI::Format::R32G32B32_FLOAT:
            case AZ::RHI::Format::R32G32B32A32_FLOAT:
            // Unsigned integer types
            case AZ::RHI::Format::R8_UINT:
            case AZ::RHI::Format::R8G8_UINT:
            case AZ::RHI::Format::R8G8B8A8_UINT:
            case AZ::RHI::Format::R16_UINT:
            case AZ::RHI::Format::R16G16_UINT:
            case AZ::RHI::Format::R16G16B16A16_UINT:
            case AZ::RHI::Format::R32_UINT:
            case AZ::RHI::Format::R32G32_UINT:
            case AZ::RHI::Format::R32G32B32_UINT:
            case AZ::RHI::Format::R32G32B32A32_UINT:
            // Signed integer types
            case AZ::RHI::Format::R8_SINT:
            case AZ::RHI::Format::R8G8_SINT:
            case AZ::RHI::Format::R8G8B8A8_SINT:
            case AZ::RHI::Format::R16_SINT:
            case AZ::RHI::Format::R16G16_SINT:
            case AZ::RHI::Format::R16G16B16A16_SINT:
            case AZ::RHI::Format::R32_SINT:
            case AZ::RHI::Format::R32G32_SINT:
            case AZ::RHI::Format::R32G32B32_SINT:
            case AZ::RHI::Format::R32G32B32A32_SINT:
            // Compressed types
            case AZ::RHI::Format::BC1_UNORM:
            case AZ::RHI::Format::BC1_UNORM_SRGB:
            case AZ::RHI::Format::BC4_UNORM:
                return true;
            }

            return false;
        }

        template<>
        AZ_DLL_EXPORT AZ::Color GetImageDataPixelValue<AZ::Color>(
            AZStd::span<const uint8_t> imageData,
            const AZ::RHI::ImageDescriptor& imageDescriptor,
            uint32_t x,
            uint32_t y,
            [[maybe_unused]] uint32_t componentIndex)
        {
            auto imageDataIndices = Internal::GetImageDataIndex(imageDescriptor, x, y);
            return Internal::RetrieveColorValue(imageData.data(), imageDataIndices, imageDescriptor.m_format);
        }

        template<>
        AZ_DLL_EXPORT float GetImageDataPixelValue<float>(
            AZStd::span<const uint8_t> imageData,
            const AZ::RHI::ImageDescriptor& imageDescriptor,
            uint32_t x,
            uint32_t y,
            uint32_t componentIndex)
        {
            auto imageDataIndices = Internal::GetImageDataIndex(imageDescriptor, x, y);
            return Internal::RetrieveFloatValue(imageData.data(), imageDataIndices, componentIndex, imageDescriptor.m_format);
        }

        template<>
        AZ_DLL_EXPORT AZ::u32 GetImageDataPixelValue<AZ::u32>(
            AZStd::span<const uint8_t> imageData,
            const AZ::RHI::ImageDescriptor& imageDescriptor,
            uint32_t x,
            uint32_t y,
            uint32_t componentIndex)
        {
            auto imageDataIndices = Internal::GetImageDataIndex(imageDescriptor, x, y);
            return Internal::RetrieveUintValue(imageData.data(), imageDataIndices, componentIndex, imageDescriptor.m_format);
        }

        template<>
        AZ_DLL_EXPORT AZ::s32 GetImageDataPixelValue<AZ::s32>(
            AZStd::span<const uint8_t> imageData,
            const AZ::RHI::ImageDescriptor& imageDescriptor,
            uint32_t x,
            uint32_t y,
            uint32_t componentIndex)
        {
            auto imageDataIndices = Internal::GetImageDataIndex(imageDescriptor, x, y);
            return Internal::RetrieveIntValue(imageData.data(), imageDataIndices, componentIndex, imageDescriptor.m_format);
        }

        template<typename T>
        T GetSubImagePixelValueInternal(
            const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset,
            uint32_t x,
            uint32_t y,
            uint32_t componentIndex,
            uint32_t mip,
            uint32_t slice)
        {
            if (!imageAsset.IsReady())
            {
                return aznumeric_cast<T>(0);
            }

            auto imageData = imageAsset->GetSubImageData(mip, slice);
            if (imageData.empty())
            {
                return aznumeric_cast<T>(0);
            }

            auto imageDescriptor = imageAsset->GetImageDescriptorForMipLevel(mip);
            return GetImageDataPixelValue<T>(imageData, imageDescriptor, x, y, componentIndex);
        }

        template<>
        AZ_DLL_EXPORT float GetSubImagePixelValue<float>(
            const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset,
            uint32_t x,
            uint32_t y,
            uint32_t componentIndex,
            uint32_t mip,
            uint32_t slice)
        {
            return GetSubImagePixelValueInternal<float>(imageAsset, x, y, componentIndex, mip, slice);
        }

        template<>
        AZ_DLL_EXPORT AZ::u32 GetSubImagePixelValue<AZ::u32>(
            const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset,
            uint32_t x,
            uint32_t y,
            uint32_t componentIndex,
            uint32_t mip,
            uint32_t slice)
        {
            return GetSubImagePixelValueInternal<AZ::u32>(imageAsset, x, y, componentIndex, mip, slice);
        }

        template<>
        AZ_DLL_EXPORT AZ::s32 GetSubImagePixelValue<AZ::s32>(
            const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset,
            uint32_t x,
            uint32_t y,
            uint32_t componentIndex,
            uint32_t mip,
            uint32_t slice)
        {
            return GetSubImagePixelValueInternal<AZ::s32>(imageAsset, x, y, componentIndex, mip, slice);
        }

        bool GetSubImagePixelValues(
            const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset,
            AZStd::pair<uint32_t, uint32_t> topLeft,
            AZStd::pair<uint32_t, uint32_t> bottomRight,
            AZStd::function<void(const AZ::u32& x, const AZ::u32& y, const float& value)> callback,
            uint32_t componentIndex,
            uint32_t mip,
            uint32_t slice)
        {
            if (!imageAsset.IsReady())
            {
                return false;
            }

            auto imageData = imageAsset->GetSubImageData(mip, slice);
            if (imageData.empty())
            {
                return false;
            }

            auto imageDescriptor = imageAsset->GetImageDescriptorForMipLevel(mip);

            for (uint32_t y = topLeft.second; y < bottomRight.second; ++y)
            {
                for (uint32_t x = topLeft.first; x < bottomRight.first; ++x)
                {
                    auto imageDataIndices = Internal::GetImageDataIndex(imageDescriptor, x, y);

                    float value =
                        Internal::RetrieveFloatValue(imageData.data(), imageDataIndices, componentIndex, imageDescriptor.m_format);
                    callback(x, y, value);
                }
            }

            return true;
        }

        bool GetSubImagePixelValues(
            const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset,
            AZStd::pair<uint32_t, uint32_t> topLeft,
            AZStd::pair<uint32_t, uint32_t> bottomRight,
            AZStd::function<void(const AZ::u32& x, const AZ::u32& y, const AZ::u32& value)> callback,
            uint32_t componentIndex,
            uint32_t mip,
            uint32_t slice)
        {
            if (!imageAsset.IsReady())
            {
                return false;
            }

            auto imageData = imageAsset->GetSubImageData(mip, slice);
            if (imageData.empty())
            {
                return false;
            }

            auto imageDescriptor = imageAsset->GetImageDescriptorForMipLevel(mip);

            for (uint32_t y = topLeft.second; y < bottomRight.second; ++y)
            {
                for (uint32_t x = topLeft.first; x < bottomRight.first; ++x)
                {
                    auto imageDataIndices = Internal::GetImageDataIndex(imageDescriptor, x, y);

                    AZ::u32 value =
                        Internal::RetrieveUintValue(imageData.data(), imageDataIndices, componentIndex, imageDescriptor.m_format);
                    callback(x, y, value);
                }
            }

            return true;
        }

        bool GetSubImagePixelValues(
            const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset,
            AZStd::pair<uint32_t, uint32_t> topLeft,
            AZStd::pair<uint32_t, uint32_t> bottomRight,
            AZStd::function<void(const AZ::u32& x, const AZ::u32& y, const AZ::s32& value)> callback,
            uint32_t componentIndex,
            uint32_t mip,
            uint32_t slice)
        {
            if (!imageAsset.IsReady())
            {
                return false;
            }

            auto imageData = imageAsset->GetSubImageData(mip, slice);
            if (imageData.empty())
            {
                return false;
            }

            auto imageDescriptor = imageAsset->GetImageDescriptorForMipLevel(mip);

            for (uint32_t y = topLeft.second; y < bottomRight.second; ++y)
            {
                for (uint32_t x = topLeft.first; x < bottomRight.first; ++x)
                {
                    auto imageDataIndices = Internal::GetImageDataIndex(imageDescriptor, x, y);

                    AZ::s32 value =
                        Internal::RetrieveIntValue(imageData.data(), imageDataIndices, componentIndex, imageDescriptor.m_format);
                    callback(x, y, value);
                }
            }

            return true;
        }

        AZStd::optional<RenderPipelineDescriptor> GetRenderPipelineDescriptorFromAsset(
            const Data::AssetId& pipelineAssetId, AZStd::string_view nameSuffix)
        {
            AZ::Data::Asset<AZ::RPI::AnyAsset> pipelineAsset =
                AssetUtils::LoadAssetById<AZ::RPI::AnyAsset>(pipelineAssetId, AssetUtils::TraceLevel::Error);
            if (!pipelineAsset.IsReady())
            {
                // Error already reported by LoadAssetByProductPath
                return AZStd::nullopt;
            }
            const RenderPipelineDescriptor* assetPipelineDesc = GetDataFromAnyAsset<AZ::RPI::RenderPipelineDescriptor>(pipelineAsset);
            if (!assetPipelineDesc)
            {
                AZ_Error("RPIUtils", false, "Invalid render pipeline descriptor from asset %s", pipelineAssetId.ToString<AZStd::string>().c_str());
                return AZStd::nullopt;
            }

            RenderPipelineDescriptor pipelineDesc = *assetPipelineDesc;
            pipelineDesc.m_name += nameSuffix;

            return {AZStd::move(pipelineDesc)};
        }

        AZStd::optional<RenderPipelineDescriptor> GetRenderPipelineDescriptorFromAsset(
            const AZStd::string& pipelineAssetPath, AZStd::string_view nameSuffix)
        {
            Data::AssetId assetId = AssetUtils::GetAssetIdForProductPath(pipelineAssetPath.c_str(), AssetUtils::TraceLevel::Error);
            if (assetId.IsValid())
            {
                return GetRenderPipelineDescriptorFromAsset(assetId, nameSuffix);
            }
            else
            {
                return AZStd::nullopt;
            }
        }

        void AddPassRequestToRenderPipeline(
            AZ::RPI::RenderPipeline* renderPipeline,
            const char* passRequestAssetFilePath,
            const char* referencePass,
            bool beforeReferencePass)
        {
            auto passRequestAsset = AZ::RPI::AssetUtils::LoadAssetByProductPath<AZ::RPI::AnyAsset>(
                passRequestAssetFilePath, AZ::RPI::AssetUtils::TraceLevel::Warning);
            const AZ::RPI::PassRequest* passRequest = nullptr;
            if (passRequestAsset->IsReady())
            {
                passRequest = passRequestAsset->GetDataAs<AZ::RPI::PassRequest>();
            }
            if (!passRequest)
            {
                AZ_Error("RPIUtils", false, "Can't load PassRequest from %s", passRequestAssetFilePath);
                return;
            }

            // Return if the pass to be created already exists
            AZ::RPI::PassFilter passFilter = AZ::RPI::PassFilter::CreateWithPassName(passRequest->m_passName, renderPipeline);
            AZ::RPI::Pass* existingPass = AZ::RPI::PassSystemInterface::Get()->FindFirstPass(passFilter);
            if (existingPass)
            {
                return;
            }

            // Create the pass
            AZ::RPI::Ptr<AZ::RPI::Pass> newPass = AZ::RPI::PassSystemInterface::Get()->CreatePassFromRequest(passRequest);
            if (!newPass)
            {
                AZ_Error("RPIUtils", false, "Failed to create the pass from pass request [%s].", passRequest->m_passName.GetCStr());
                return;
            }

            // Add the pass to render pipeline
            bool success;
            if (beforeReferencePass)
            {
                success = renderPipeline->AddPassBefore(newPass, AZ::Name(referencePass));
            }
            else
            {
                success = renderPipeline->AddPassAfter(newPass, AZ::Name(referencePass));
            }
            // only create pass resources if it was success
            if (!success)
            {
                AZ_Error(
                    "RPIUtils",
                    false,
                    "Failed to add pass [%s] to render pipeline [%s].",
                    newPass->GetName().GetCStr(),
                    renderPipeline->GetId().GetCStr());
            }
        }
    } // namespace RPI
} // namespace AZ
