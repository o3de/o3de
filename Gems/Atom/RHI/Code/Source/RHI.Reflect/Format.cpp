/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI.Reflect/Format.h>
#include <Atom/RHI.Reflect/Bits.h>
#include <AzCore/std/algorithm.h>

namespace AZ::RHI
{
    uint32_t GetFormatSize(Format format)
    {
        switch (format)
        {
        case Format::Unknown:
            return 0;

        case Format::R32G32B32A32_FLOAT:
        case Format::R32G32B32A32_UINT:
        case Format::R32G32B32A32_SINT:
            return 16;

        case Format::R32G32B32_FLOAT:
        case Format::R32G32B32_UINT:
        case Format::R32G32B32_SINT:
            return 12;

        case Format::R16G16B16A16_FLOAT:
        case Format::R16G16B16A16_UNORM:
        case Format::R16G16B16A16_UINT:
        case Format::R16G16B16A16_SNORM:
        case Format::R16G16B16A16_SINT:
        case Format::R32G32_FLOAT:
        case Format::R32G32_UINT:
        case Format::R32G32_SINT:
        case Format::D32_FLOAT_S8X24_UINT:
        case Format::Y416:
        case Format::Y210:
        case Format::Y216:
            return 8;

        case Format::R10G10B10A2_UNORM:
        case Format::R10G10B10A2_UINT:
        case Format::R11G11B10_FLOAT:
        case Format::R8G8B8A8_UNORM:
        case Format::R8G8B8A8_UNORM_SRGB:
        case Format::R8G8B8A8_UINT:
        case Format::R8G8B8A8_SNORM:
        case Format::R8G8B8A8_SINT:
        case Format::R16G16_FLOAT:
        case Format::R16G16_UNORM:
        case Format::R16G16_UINT:
        case Format::R16G16_SNORM:
        case Format::R16G16_SINT:
        case Format::D32_FLOAT:
        case Format::R32_FLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT:
        case Format::D24_UNORM_S8_UINT:
        case Format::R9G9B9E5_SHAREDEXP:
        case Format::R8G8_B8G8_UNORM:
        case Format::G8R8_G8B8_UNORM:
        case Format::B8G8R8A8_UNORM:
        case Format::B8G8R8A8_SNORM:
        case Format::B8G8R8X8_UNORM:
        case Format::R10G10B10_XR_BIAS_A2_UNORM:
        case Format::B8G8R8A8_UNORM_SRGB:
        case Format::B8G8R8X8_UNORM_SRGB:
        case Format::AYUV:
        case Format::Y410:
        case Format::YUY2:
            return 4;

        case Format::P010:
        case Format::P016:
            return 24;

        case Format::R8G8_UNORM:
        case Format::R8G8_UINT:
        case Format::R8G8_SNORM:
        case Format::R8G8_SINT:
        case Format::R16_FLOAT:
        case Format::D16_UNORM:
        case Format::R16_UNORM:
        case Format::R16_UINT:
        case Format::R16_SNORM:
        case Format::R16_SINT:
        case Format::R5G6B5_UNORM:
        case Format::B5G6R5_UNORM:
        case Format::B5G5R5A1_UNORM:
        case Format::A8P8:
        case Format::B4G4R4A4_UNORM:
            return 2;

        case Format::NV12:
        case Format::NV11:
            return 12;

        case Format::R8_UNORM:
        case Format::R8_UINT:
        case Format::R8_SNORM:
        case Format::R8_SINT:
        case Format::A8_UNORM:
        case Format::AI44:
        case Format::IA44:
        case Format::P8:
        case Format::R1_UNORM:
            return 1;

        case Format::BC1_UNORM:
        case Format::BC1_UNORM_SRGB:
        case Format::BC4_UNORM:
        case Format::BC4_SNORM:
        case Format::EAC_R11_UNORM:
        case Format::EAC_R11_SNORM:
        case Format::ETC2_UNORM:
        case Format::ETC2_UNORM_SRGB:
        case Format::ETC2A1_UNORM:
        case Format::ETC2A1_UNORM_SRGB:
            return 8;

        case Format::BC2_UNORM:
        case Format::BC2_UNORM_SRGB:
        case Format::BC3_UNORM:
        case Format::BC3_UNORM_SRGB:
        case Format::BC5_UNORM:
        case Format::BC5_SNORM:
        case Format::BC6H_UF16:
        case Format::BC6H_SF16:
        case Format::BC7_UNORM:
        case Format::BC7_UNORM_SRGB:
        case Format::EAC_RG11_UNORM:
        case Format::EAC_RG11_SNORM:
        case Format::ETC2A_UNORM:
        case Format::ETC2A_UNORM_SRGB:
        case Format::ASTC_4x4_UNORM:
        case Format::ASTC_4x4_UNORM_SRGB:
        case Format::ASTC_5x4_UNORM:
        case Format::ASTC_5x4_UNORM_SRGB:
        case Format::ASTC_5x5_UNORM:
        case Format::ASTC_5x5_UNORM_SRGB:
        case Format::ASTC_6x5_UNORM:
        case Format::ASTC_6x5_UNORM_SRGB:
        case Format::ASTC_6x6_UNORM:
        case Format::ASTC_6x6_UNORM_SRGB:
        case Format::ASTC_8x5_UNORM:
        case Format::ASTC_8x5_UNORM_SRGB:
        case Format::ASTC_8x6_UNORM:
        case Format::ASTC_8x6_UNORM_SRGB:
        case Format::ASTC_8x8_UNORM:
        case Format::ASTC_8x8_UNORM_SRGB:
        case Format::ASTC_10x5_UNORM:
        case Format::ASTC_10x5_UNORM_SRGB:
        case Format::ASTC_10x6_UNORM:
        case Format::ASTC_10x6_UNORM_SRGB:
        case Format::ASTC_10x8_UNORM:
        case Format::ASTC_10x8_UNORM_SRGB:
        case Format::ASTC_10x10_UNORM:
        case Format::ASTC_10x10_UNORM_SRGB:
        case Format::ASTC_12x10_UNORM:
        case Format::ASTC_12x10_UNORM_SRGB:
        case Format::ASTC_12x12_UNORM:
        case Format::ASTC_12x12_UNORM_SRGB:
            return 16;

        default:
            AZ_Assert(false, "Unimplemented format");
            return 0;
        }
    }

    uint32_t GetFormatComponentCount(Format format)
    {
        switch (format)
        {
        case Format::Unknown:
            return 0;

        case Format::R32G32B32A32_FLOAT:
        case Format::R32G32B32A32_UINT:
        case Format::R32G32B32A32_SINT:
            return 4;

        case Format::R32G32B32_FLOAT:
        case Format::R32G32B32_UINT:
        case Format::R32G32B32_SINT:
            return 3;

        case Format::R16G16B16A16_FLOAT:
        case Format::R16G16B16A16_UNORM:
        case Format::R16G16B16A16_UINT:
        case Format::R16G16B16A16_SNORM:
        case Format::R16G16B16A16_SINT:
            return 4;

        case Format::R32G32_FLOAT:
        case Format::R32G32_UINT:
        case Format::R32G32_SINT:
            return 2;

        case Format::D32_FLOAT_S8X24_UINT:
            return 3;

        case Format::Y416:
        case Format::Y210:
        case Format::Y216:
            return 4;

        case Format::R10G10B10A2_UNORM:
        case Format::R10G10B10A2_UINT:
            return 4;

        case Format::R11G11B10_FLOAT:
            return 3;

        case Format::R8G8B8A8_UNORM:
        case Format::R8G8B8A8_UNORM_SRGB:
        case Format::R8G8B8A8_UINT:
        case Format::R8G8B8A8_SNORM:
        case Format::R8G8B8A8_SINT:
        case Format::A8B8G8R8_UNORM:
        case Format::A8B8G8R8_UNORM_SRGB:
        case Format::A8B8G8R8_SNORM:
            return 4;

        case Format::R16G16_FLOAT:
        case Format::R16G16_UNORM:
        case Format::R16G16_UINT:
        case Format::R16G16_SNORM:
        case Format::R16G16_SINT:
            return 2;

        case Format::D32_FLOAT:
        case Format::R32_FLOAT:
        case Format::R32_UINT:
        case Format::R32_SINT:
            return 1;

        case Format::D24_UNORM_S8_UINT:
            return 2;

        case Format::R9G9B9E5_SHAREDEXP:
            return 3;

        case Format::R8G8_B8G8_UNORM:
        case Format::G8R8_G8B8_UNORM:
            return 3; // One entry represents a pair of RGB colors, where R/B are repeated.

        case Format::B8G8R8A8_UNORM:
            return 4;

        case Format::B8G8R8A8_SNORM:
            return 4;

        case Format::B8G8R8X8_UNORM:
            return 3;

        case Format::R10G10B10_XR_BIAS_A2_UNORM:
            return 4;

        case Format::B8G8R8A8_UNORM_SRGB:
            return 4;

        case Format::B8G8R8X8_UNORM_SRGB:
            return 3;

        case Format::AYUV:
        case Format::Y410:
        case Format::YUY2:
            return 4;

        case Format::P010:
        case Format::P016:
            return 3; // Not sure this is right

        case Format::R8G8_UNORM:
        case Format::R8G8_UINT:
        case Format::R8G8_SNORM:
        case Format::R8G8_SINT:
            return 2;

        case Format::R16_FLOAT:
        case Format::D16_UNORM:
        case Format::R16_UNORM:
        case Format::R16_UINT:
        case Format::R16_SNORM:
        case Format::R16_SINT:
            return 1;

        case Format::R5G6B5_UNORM:
        case Format::B5G6R5_UNORM:
            return 3;

        case Format::B5G5R5A1_UNORM:
            return 4;

        case Format::A8P8:
            return 2;

        case Format::B4G4R4A4_UNORM:
            return 4;

        case Format::NV12:
        case Format::NV11:
            return 3;

        case Format::R8_UNORM:
        case Format::R8_UINT:
        case Format::R8_SNORM:
        case Format::R8_SINT:
        case Format::A8_UNORM:
            return 1;

        case Format::AI44:
        case Format::IA44:
        case Format::P8:
            return 1; // Not sure this is right. These are palletized formats.

        case Format::R1_UNORM:
            return 1;

        case Format::BC1_UNORM:
        case Format::BC1_UNORM_SRGB:
            return 4;

        case Format::BC4_UNORM:
        case Format::BC4_SNORM:
            return 1;

        case Format::BC2_UNORM:
        case Format::BC2_UNORM_SRGB:
        case Format::BC3_UNORM:
        case Format::BC3_UNORM_SRGB:
            return 4;

        case Format::BC5_UNORM:
        case Format::BC5_SNORM:
            return 2;

        case Format::BC6H_UF16:
        case Format::BC6H_SF16:
            return 3;

        case Format::BC7_UNORM:
        case Format::BC7_UNORM_SRGB:
            return 4; // May actually represent 3 or 4 elements depending on the compression settings

        case Format::ASTC_4x4_UNORM:
        case Format::ASTC_4x4_UNORM_SRGB:
        case Format::ASTC_5x4_UNORM:
        case Format::ASTC_5x4_UNORM_SRGB:
        case Format::ASTC_5x5_UNORM:
        case Format::ASTC_5x5_UNORM_SRGB:
        case Format::ASTC_6x5_UNORM:
        case Format::ASTC_6x5_UNORM_SRGB:
        case Format::ASTC_6x6_UNORM:
        case Format::ASTC_6x6_UNORM_SRGB:
        case Format::ASTC_8x5_UNORM:
        case Format::ASTC_8x5_UNORM_SRGB:
        case Format::ASTC_8x6_UNORM:
        case Format::ASTC_8x6_UNORM_SRGB:
        case Format::ASTC_8x8_UNORM:
        case Format::ASTC_8x8_UNORM_SRGB:
        case Format::ASTC_10x5_UNORM:
        case Format::ASTC_10x5_UNORM_SRGB:
        case Format::ASTC_10x6_UNORM:
        case Format::ASTC_10x6_UNORM_SRGB:
        case Format::ASTC_10x8_UNORM:
        case Format::ASTC_10x8_UNORM_SRGB:
        case Format::ASTC_10x10_UNORM:
        case Format::ASTC_10x10_UNORM_SRGB:
        case Format::ASTC_12x10_UNORM:
        case Format::ASTC_12x10_UNORM_SRGB:
        case Format::ASTC_12x12_UNORM:
        case Format::ASTC_12x12_UNORM_SRGB:
            return 4;

        case Format::ETC2_UNORM:
        case Format::ETC2_UNORM_SRGB:
            return 3;

        case Format::ETC2A_UNORM:
        case Format::ETC2A_UNORM_SRGB:
        case Format::ETC2A1_UNORM:
        case Format::ETC2A1_UNORM_SRGB:
            return 4;

        default:
            AZ_Assert(false, "Unimplemented format");
            return 0;
        }
    }

    const char* ToString(Format format)
    {
        switch (format)
        {
        case Format::Unknown:                         return "Unknown";
        case Format::R32G32B32A32_FLOAT:              return "R32G32B32A32_FLOAT";
        case Format::R32G32B32A32_UINT:               return "R32G32B32A32_UINT";
        case Format::R32G32B32A32_SINT:               return "R32G32B32A32_SINT";
        case Format::R32G32B32_FLOAT:                 return "R32G32B32_FLOAT";
        case Format::R32G32B32_UINT:                  return "R32G32B32_UINT";
        case Format::R32G32B32_SINT:                  return "R32G32B32_SINT";
        case Format::R16G16B16A16_FLOAT:              return "R16G16B16A16_FLOAT";
        case Format::R16G16B16A16_UNORM:              return "R16G16B16A16_UNORM";
        case Format::R16G16B16A16_UINT:               return "R16G16B16A16_UINT";
        case Format::R16G16B16A16_SNORM:              return "R16G16B16A16_SNORM";
        case Format::R16G16B16A16_SINT:               return "R16G16B16A16_SINT";
        case Format::R32G32_FLOAT:                    return "R32G32_FLOAT";
        case Format::R32G32_UINT:                     return "R32G32_UINT";
        case Format::R32G32_SINT:                     return "R32G32_SINT";
        case Format::D32_FLOAT_S8X24_UINT:            return "D32_FLOAT_S8X24_UINT";
        case Format::Y416:                            return "Y416";
        case Format::Y210:                            return "Y210";
        case Format::Y216:                            return "Y216";
        case Format::R10G10B10A2_UNORM:               return "R10G10B10A2_UNORM";
        case Format::R10G10B10A2_UINT:                return "R10G10B10A2_UINT";
        case Format::R11G11B10_FLOAT:                 return "R11G11B10_FLOAT";
        case Format::R8G8B8A8_UNORM:                  return "R8G8B8A8_UNORM";
        case Format::R8G8B8A8_UNORM_SRGB:             return "R8G8B8A8_UNORM_SRGB";
        case Format::R8G8B8A8_UINT:                   return "R8G8B8A8_UINT";
        case Format::R8G8B8A8_SNORM:                  return "R8G8B8A8_SNORM";
        case Format::R8G8B8A8_SINT:                   return "R8G8B8A8_SINT";
        case Format::A8B8G8R8_UNORM:                  return "A8B8G8R8_UNORM";
        case Format::A8B8G8R8_UNORM_SRGB:             return "A8B8G8R8_UNORM_SRGB";
        case Format::A8B8G8R8_SNORM:                  return "A8B8G8R8_SNORM";
        case Format::R16G16_FLOAT:                    return "R16G16_FLOAT";
        case Format::R16G16_UNORM:                    return "R16G16_UNORM";
        case Format::R16G16_UINT:                     return "R16G16_UINT";
        case Format::R16G16_SNORM:                    return "R16G16_SNORM";
        case Format::R16G16_SINT:                     return "R16G16_SINT";
        case Format::D32_FLOAT:                       return "D32_FLOAT";
        case Format::R32_FLOAT:                       return "R32_FLOAT";
        case Format::R32_UINT:                        return "R32_UINT";
        case Format::R32_SINT:                        return "R32_SINT";
        case Format::D24_UNORM_S8_UINT:               return "D24_UNORM_S8_UINT";
        case Format::R9G9B9E5_SHAREDEXP:              return "R9G9B9E5_SHAREDEXP";
        case Format::R8G8_B8G8_UNORM:                 return "R8G8_B8G8_UNORM";
        case Format::G8R8_G8B8_UNORM:                 return "G8R8_G8B8_UNORM";
        case Format::B8G8R8A8_UNORM:                  return "B8G8R8A8_UNORM";
        case Format::B8G8R8A8_SNORM:                  return "B8G8R8A8_SNORM";
        case Format::B8G8R8X8_UNORM:                  return "B8G8R8X8_UNORM";
        case Format::R10G10B10_XR_BIAS_A2_UNORM:      return "R10G10B10_XR_BIAS_A2_UNORM";
        case Format::B8G8R8A8_UNORM_SRGB:             return "B8G8R8A8_UNORM_SRGB";
        case Format::B8G8R8X8_UNORM_SRGB:             return "B8G8R8X8_UNORM_SRGB";
        case Format::AYUV:                            return "AYUV";
        case Format::Y410:                            return "Y410";
        case Format::YUY2:                            return "YUY2";
        case Format::P010:                            return "P010";
        case Format::P016:                            return "P016";
        case Format::R8G8_UNORM:                      return "R8G8_UNORM";
        case Format::R8G8_UINT:                       return "R8G8_UINT";
        case Format::R8G8_SNORM:                      return "R8G8_SNORM";
        case Format::R8G8_SINT:                       return "R8G8_SINT";
        case Format::R16_FLOAT:                       return "R16_FLOAT";
        case Format::D16_UNORM:                       return "D16_UNORM";
        case Format::R16_UNORM:                       return "R16_UNORM";
        case Format::R16_UINT:                        return "R16_UINT";
        case Format::R16_SNORM:                       return "R16_SNORM";
        case Format::R16_SINT:                        return "R16_SINT";
        case Format::B5G6R5_UNORM:                    return "B5G6R5_UNORM";
        case Format::B5G5R5A1_UNORM:                  return "B5G5R5A1_UNORM";
        case Format::A8P8:                            return "A8P8";
        case Format::B4G4R4A4_UNORM:                  return "B4G4R4A4_UNORM";
        case Format::NV12:                            return "NV12";
        case Format::NV11:                            return "NV11";
        case Format::R8_UNORM:                        return "R8_UNORM";
        case Format::R8_UINT:                         return "R8_UINT";
        case Format::R8_SNORM:                        return "R8_SNORM";
        case Format::R8_SINT:                         return "R8_SINT";
        case Format::A8_UNORM:                        return "A8_UNORM";
        case Format::AI44:                            return "AI44";
        case Format::IA44:                            return "IA44";
        case Format::P8:                              return "P8";
        case Format::R1_UNORM:                        return "R1_UNORM";
        case Format::BC1_UNORM:                       return "BC1_UNORM";
        case Format::BC1_UNORM_SRGB:                  return "BC1_UNORM_SRGB";
        case Format::BC4_UNORM:                       return "BC4_UNORM";
        case Format::BC4_SNORM:                       return "BC4_SNORM";
        case Format::BC2_UNORM:                       return "BC2_UNORM";
        case Format::BC2_UNORM_SRGB:                  return "BC2_UNORM_SRGB";
        case Format::BC3_UNORM:                       return "BC3_UNORM";
        case Format::BC3_UNORM_SRGB:                  return "BC3_UNORM_SRGB";
        case Format::BC5_UNORM:                       return "BC5_UNORM";
        case Format::BC5_SNORM:                       return "BC5_SNORM";
        case Format::BC6H_UF16:                       return "BC6H_UF16";
        case Format::BC6H_SF16:                       return "BC6H_SF16";
        case Format::BC7_UNORM:                       return "BC7_UNORM";
        case Format::BC7_UNORM_SRGB:                  return "BC7_UNORM_SRGB";
        case Format::ASTC_4x4_UNORM:                  return "ASTC_4x4_UNORM";
        case Format::ASTC_4x4_UNORM_SRGB:             return "ASTC_4x4_UNORM_SRGB";
        case Format::ASTC_5x4_UNORM:                  return "ASTC_5x4_UNORM";
        case Format::ASTC_5x4_UNORM_SRGB:             return "ASTC_5x4_UNORM_SRGB";
        case Format::ASTC_5x5_UNORM:                  return "ASTC_5x5_UNORM";
        case Format::ASTC_5x5_UNORM_SRGB:             return "ASTC_5x5_UNORM_SRGB";
        case Format::ASTC_6x5_UNORM:                  return "ASTC_6x5_UNORM";
        case Format::ASTC_6x5_UNORM_SRGB:             return "ASTC_6x5_UNORM_SRGB";
        case Format::ASTC_6x6_UNORM:                  return "ASTC_6x6_UNORM";
        case Format::ASTC_6x6_UNORM_SRGB:             return "ASTC_6x6_UNORM_SRGB";
        case Format::ASTC_8x5_UNORM:                  return "ASTC_8x5_UNORM";
        case Format::ASTC_8x5_UNORM_SRGB:             return "ASTC_8x5_UNORM_SRGB";
        case Format::ASTC_8x6_UNORM:                  return "ASTC_8x6_UNORM";
        case Format::ASTC_8x6_UNORM_SRGB:             return "ASTC_8x6_UNORM_SRGB";
        case Format::ASTC_8x8_UNORM:                  return "ASTC_8x8_UNORM";
        case Format::ASTC_8x8_UNORM_SRGB:             return "ASTC_8x8_UNORM_SRGB";
        case Format::ASTC_10x5_UNORM:                 return "ASTC_10x5_UNORM";
        case Format::ASTC_10x5_UNORM_SRGB:            return "ASTC_10x5_UNORM_SRGB";
        case Format::ASTC_10x6_UNORM:                 return "ASTC_10x6_UNORM";
        case Format::ASTC_10x6_UNORM_SRGB:            return "ASTC_10x6_UNORM_SRGB";
        case Format::ASTC_10x8_UNORM:                 return "ASTC_10x8_UNORM";
        case Format::ASTC_10x8_UNORM_SRGB:            return "ASTC_10x8_UNORM_SRGB";
        case Format::ASTC_10x10_UNORM:                return "ASTC_10x10_UNORM";
        case Format::ASTC_10x10_UNORM_SRGB:           return "ASTC_10x10_UNORM_SRGB";
        case Format::ASTC_12x10_UNORM:                return "ASTC_12x10_UNORM";
        case Format::ASTC_12x10_UNORM_SRGB:           return "ASTC_12x10_UNORM_SRGB";
        case Format::ASTC_12x12_UNORM:                return "ASTC_12x12_UNORM";
        case Format::ASTC_12x12_UNORM_SRGB:           return "ASTC_12x12_UNORM_SRGB";
        case Format::ETC2_UNORM:                      return "ETC2_UNORM";
        case Format::ETC2_UNORM_SRGB:                 return "ETC2_UNORM_SRGB";
        case Format::ETC2A1_UNORM:                    return "ETC2A1_UNORM";
        case Format::ETC2A1_UNORM_SRGB:               return "ETC2A1_UNORM_SRGB";
        case Format::ETC2A_UNORM:                     return "ETC2A_UNORM";
        case Format::ETC2A_UNORM_SRGB:                return "ETC2A_UNORM_SRGB";
        }
            
        return "<<Format Value Error>>";
    }

    RHI::Size GetFormatDimensionAlignment(Format format)
    {
        switch (format)
        {
        case Format::BC1_UNORM:
        case Format::BC1_UNORM_SRGB:
        case Format::BC4_UNORM:
        case Format::BC4_SNORM:
        case Format::BC2_UNORM:
        case Format::BC2_UNORM_SRGB:
        case Format::BC3_UNORM:
        case Format::BC3_UNORM_SRGB:
        case Format::BC5_UNORM:
        case Format::BC5_SNORM:
        case Format::BC6H_UF16:
        case Format::BC6H_SF16:
        case Format::BC7_UNORM:
        case Format::BC7_UNORM_SRGB:
        case Format::ETC2_UNORM:
        case Format::ETC2_UNORM_SRGB:
        case Format::ETC2A_UNORM:
        case Format::ETC2A_UNORM_SRGB:
        case Format::ETC2A1_UNORM:
        case Format::ETC2A1_UNORM_SRGB:
        case Format::EAC_RG11_UNORM:
        case Format::EAC_RG11_SNORM:
        case Format::EAC_R11_UNORM:
        case Format::EAC_R11_SNORM:
        case Format::ASTC_4x4_UNORM:
        case Format::ASTC_4x4_UNORM_SRGB:
            return RHI::Size(4, 4, 1);
        case Format::ASTC_5x4_UNORM:
        case Format::ASTC_5x4_UNORM_SRGB:
            return RHI::Size(5, 4, 1);
        case Format::ASTC_5x5_UNORM:
        case Format::ASTC_5x5_UNORM_SRGB:
            return RHI::Size(5, 5, 1);
        case Format::ASTC_6x5_UNORM:
        case Format::ASTC_6x5_UNORM_SRGB:
            return RHI::Size(6, 5, 1);
        case Format::ASTC_6x6_UNORM:
        case Format::ASTC_6x6_UNORM_SRGB:
            return RHI::Size(6, 6, 1);
        case Format::ASTC_8x5_UNORM:
        case Format::ASTC_8x5_UNORM_SRGB:
            return RHI::Size(8, 5, 1);
        case Format::ASTC_8x6_UNORM:
        case Format::ASTC_8x6_UNORM_SRGB:
            return RHI::Size(8, 6, 1);
        case Format::ASTC_8x8_UNORM:
        case Format::ASTC_8x8_UNORM_SRGB:
            return RHI::Size(8, 8, 1);
        case Format::ASTC_10x5_UNORM:
        case Format::ASTC_10x5_UNORM_SRGB:
            return RHI::Size(10, 5, 1);
        case Format::ASTC_10x6_UNORM:
        case Format::ASTC_10x6_UNORM_SRGB:
            return RHI::Size(10, 6, 1);
        case Format::ASTC_10x8_UNORM:
        case Format::ASTC_10x8_UNORM_SRGB:
            return RHI::Size(10, 8, 1);
        case Format::ASTC_10x10_UNORM:
        case Format::ASTC_10x10_UNORM_SRGB:
            return RHI::Size(10, 10, 1);
        case Format::ASTC_12x10_UNORM:
        case Format::ASTC_12x10_UNORM_SRGB:
            return RHI::Size(12, 10, 1);
        case Format::ASTC_12x12_UNORM:
        case Format::ASTC_12x12_UNORM_SRGB:
            return RHI::Size(12, 12, 1);
        case Format::R8G8_B8G8_UNORM:
        case Format::G8R8_G8B8_UNORM:
        case Format::YUY2:
        case Format::Y210:
        case Format::Y216:
        case Format::NV12:
        case Format::P010:
        case Format::P016:
            return RHI::Size(2, 2, 1);

        default:
            return RHI::Size(1, 1, 1);
        }
    }

    Format ConvertLinearToSRGB(Format format)
    {
        switch (format)
        {
        case Format::R8G8B8A8_UNORM:
            return Format::R8G8B8A8_UNORM_SRGB;

        case Format::A8B8G8R8_UNORM:
            return Format::A8B8G8R8_UNORM_SRGB;

        case Format::BC1_UNORM:
            return Format::BC1_UNORM_SRGB;

        case Format::BC2_UNORM:
            return Format::BC2_UNORM_SRGB;

        case Format::BC3_UNORM:
            return Format::BC3_UNORM_SRGB;

        case Format::B8G8R8A8_UNORM:
            return Format::B8G8R8A8_UNORM_SRGB;

        case Format::B8G8R8X8_UNORM:
            return Format::B8G8R8X8_UNORM_SRGB;

        case Format::BC7_UNORM:
            return Format::BC7_UNORM_SRGB;

        case Format::ASTC_4x4_UNORM:
            return Format::ASTC_4x4_UNORM_SRGB;

        case Format::ASTC_5x4_UNORM:
            return Format::ASTC_5x4_UNORM_SRGB;

        case Format::ASTC_5x5_UNORM:
            return Format::ASTC_5x5_UNORM_SRGB;

        case Format::ASTC_6x5_UNORM:
            return Format::ASTC_6x5_UNORM_SRGB;

        case Format::ASTC_6x6_UNORM:
            return Format::ASTC_6x6_UNORM_SRGB;

        case Format::ASTC_8x5_UNORM:
            return Format::ASTC_8x5_UNORM_SRGB;

        case Format::ASTC_8x6_UNORM:
            return Format::ASTC_8x6_UNORM_SRGB;

        case Format::ASTC_8x8_UNORM:
            return Format::ASTC_8x8_UNORM_SRGB;

        case Format::ASTC_10x5_UNORM:
            return Format::ASTC_10x5_UNORM_SRGB;

        case Format::ASTC_10x6_UNORM:
            return Format::ASTC_10x6_UNORM_SRGB;

        case Format::ASTC_10x8_UNORM:
            return Format::ASTC_10x8_UNORM_SRGB;

        case Format::ASTC_10x10_UNORM:
            return Format::ASTC_10x10_UNORM_SRGB;

        case Format::ASTC_12x10_UNORM:
            return Format::ASTC_12x10_UNORM_SRGB;

        case Format::ASTC_12x12_UNORM:
            return Format::ASTC_12x12_UNORM_SRGB;

        case Format::ETC2_UNORM:
            return Format::ETC2_UNORM_SRGB;

        case Format::ETC2A1_UNORM:
            return Format::ETC2A1_UNORM_SRGB;

        case Format::ETC2A_UNORM:
            return Format::ETC2A_UNORM_SRGB;

        default:
            return format;
        }
    }

    Format ConvertSRGBToLinear(Format format)
    {
        switch (format)
        {
        case Format::A8B8G8R8_UNORM_SRGB:
            return Format::A8B8G8R8_UNORM;

        case Format::R8G8B8A8_UNORM_SRGB:
            return Format::R8G8B8A8_UNORM;

        case Format::BC1_UNORM_SRGB:
            return Format::BC1_UNORM;

        case Format::BC2_UNORM_SRGB:
            return Format::BC2_UNORM;

        case Format::BC3_UNORM_SRGB:
            return Format::BC3_UNORM;

        case Format::B8G8R8A8_UNORM_SRGB:
            return Format::B8G8R8A8_UNORM;

        case Format::B8G8R8X8_UNORM_SRGB:
            return Format::B8G8R8X8_UNORM;

        case Format::BC7_UNORM_SRGB:
            return Format::BC7_UNORM;

        case Format::ASTC_4x4_UNORM_SRGB:
            return Format::ASTC_4x4_UNORM;

        case Format::ASTC_5x4_UNORM_SRGB:
            return Format::ASTC_5x4_UNORM;

        case Format::ASTC_5x5_UNORM_SRGB:
            return Format::ASTC_5x5_UNORM;

        case Format::ASTC_6x5_UNORM_SRGB:
            return Format::ASTC_6x5_UNORM;

        case Format::ASTC_6x6_UNORM_SRGB:
            return Format::ASTC_6x6_UNORM;

        case Format::ASTC_8x5_UNORM_SRGB:
            return Format::ASTC_8x5_UNORM;

        case Format::ASTC_8x6_UNORM_SRGB:
            return Format::ASTC_8x6_UNORM;

        case Format::ASTC_8x8_UNORM_SRGB:
            return Format::ASTC_8x8_UNORM;

        case Format::ASTC_10x5_UNORM_SRGB:
            return Format::ASTC_10x5_UNORM;

        case Format::ASTC_10x6_UNORM_SRGB:
            return Format::ASTC_10x6_UNORM;

        case Format::ASTC_10x8_UNORM_SRGB:
            return Format::ASTC_10x8_UNORM;

        case Format::ASTC_10x10_UNORM_SRGB:
            return Format::ASTC_10x10_UNORM;

        case Format::ASTC_12x10_UNORM_SRGB:
            return Format::ASTC_12x10_UNORM;

        case Format::ASTC_12x12_UNORM_SRGB:
            return Format::ASTC_12x12_UNORM;

        case Format::ETC2_UNORM_SRGB:
            return Format::ETC2_UNORM;

        case Format::ETC2A1_UNORM_SRGB:
            return Format::ETC2A1_UNORM;

        case Format::ETC2A_UNORM_SRGB:
            return Format::ETC2A_UNORM;

        default:
            return format;
        }
    }

    ImageAspectFlags GetImageAspectFlags(Format format)
    {
        switch (format)
        {
        case Format::D32_FLOAT_S8X24_UINT:
        case Format::D16_UNORM_S8_UINT:
        case Format::D24_UNORM_S8_UINT:
            return ImageAspectFlags::DepthStencil;
        case Format::D32_FLOAT:
        case Format::D16_UNORM:
            return ImageAspectFlags::Depth;
        default:
            return ImageAspectFlags::Color;
        }
    }
}
