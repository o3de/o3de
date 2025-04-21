/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/Utils/DdsFile.h>
#include <Atom/RHI.Reflect/ImageSubresource.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>

namespace AZ
{
    namespace
    {
        // Flags to indicate which members contain valid data
        enum DdsFlags : uint32_t
        {
            Unknown = 0x00000000,
            Caps = 0x00000001, // DDSD_CAPS
            Height = 0x00000002, // DDSD_HEIGHT
            Width = 0x00000004, // DDSD_WIDTH
            Pitch = 0x00000008, // DDSD_PITCH
            PixelFormat = 0x00001000, // DDSD_PIXELFORMAT
            MipmapCount = 0x00020000, // DDSD_MIPMAPCOUNT
            LinearSize = 0x00080000, // DDSD_LINEARSIZE
            Depth = 0x00800000, // DDSD_DEPTH
        };

        // Specifies the complexity of the surfaces stored
        enum DdsCaps : uint32_t
        {
            Complex = 0x00000008, // DDSCAPS_COMPLEX
            Mipmap = 0x00400000, // DDSCAPS_MIPMAP
            Texture = 0x00001000, // DDSCAPS_TEXTURE
        };

        // Additional detail about the surfaces stored.
        enum DdsCaps2 : uint32_t
        {
            Cubemap = 0x00000200, // DDSCAPS2_CUBEMAP
            PositiveX = 0x00000400, // DDSCAPS2_CUBEMAP_POSITIVEX
            NegativeX = 0x00000800, // DDSCAPS2_CUBEMAP_NEGATIVEX
            PositiveY = 0x00001000, // DDSCAPS2_CUBEMAP_POSITIVEY
            NegativeY = 0x00002000, // DDSCAPS2_CUBEMAP_NEGATIVEY
            PositiveZ = 0x00004000, // DDSCAPS2_CUBEMAP_POSITIVEZ
            NegativeZ = 0x00008000, // DDSCAPS2_CUBEMAP_NEGATIVEZ
            CubemapAll = Cubemap | PositiveX | NegativeX | PositiveY | NegativeY | PositiveZ | NegativeZ, //DDS_CUBEMAP_ALLFACES 
            Volume = 0x00200000, // DDSCAPS2_VOLUME
        };

        enum PixelFormatFlags : uint32_t
        {
            AlphaPixels = 0x00000001, // DDPF_ALPHAPIXELS
            Alpha = 0x00000002, // DDPF_ALPHA
            FourCC = 0x00000004, // DDPF_FOURCC
            RGB = 0x00000040, // DDPF_RGB
            YUV = 0x00000200, // DDPF_YUV
            Luminance = 0x00020000, // DDPF_LUMINANCE
        };

        enum class ResourceDimension : uint32_t
        {
            Unknown = 0,   // D3D10_RESOURCE_DIMENSION_UNKNOWN
            Buffer = 1,    // D3D10_RESOURCE_DIMENSION_BUFFER
            Texture1D = 2, // D3D10_RESOURCE_DIMENSION_TEXTURE1D
            Texture2D = 3, // D3D10_RESOURCE_DIMENSION_TEXTURE2D
            Texture3D = 4, // D3D10_RESOURCE_DIMENSION_TEXTURE3D
        };

        enum Dx10MiscFlags : uint32_t
        {
            TextureCube = 0x4, // DDS_RESOURCE_MISC_TEXTURECUBE
        };

        enum class DxgiFormat : uint32_t
        {
            UNKNOWN                      = 0,
            R32G32B32A32_TYPELESS        = 1,
            R32G32B32A32_FLOAT           = 2,
            R32G32B32A32_UINT            = 3,
            R32G32B32A32_SINT            = 4,
            R32G32B32_TYPELESS           = 5,
            R32G32B32_FLOAT              = 6,
            R32G32B32_UINT               = 7,
            R32G32B32_SINT               = 8,
            R16G16B16A16_TYPELESS        = 9,
            R16G16B16A16_FLOAT           = 10,
            R16G16B16A16_UNORM           = 11,
            R16G16B16A16_UINT            = 12,
            R16G16B16A16_SNORM           = 13,
            R16G16B16A16_SINT            = 14,
            R32G32_TYPELESS              = 15,
            R32G32_FLOAT                 = 16,
            R32G32_UINT                  = 17,
            R32G32_SINT                  = 18,
            R32G8X24_TYPELESS            = 19,
            D32_FLOAT_S8X24_UINT         = 20,
            R32_FLOAT_X8X24_TYPELESS     = 21,
            X32_TYPELESS_G8X24_UINT      = 22,
            R10G10B10A2_TYPELESS         = 23,
            R10G10B10A2_UNORM            = 24,
            R10G10B10A2_UINT             = 25,
            R11G11B10_FLOAT              = 26,
            R8G8B8A8_TYPELESS            = 27,
            R8G8B8A8_UNORM               = 28,
            R8G8B8A8_UNORM_SRGB          = 29,
            R8G8B8A8_UINT                = 30,
            R8G8B8A8_SNORM               = 31,
            R8G8B8A8_SINT                = 32,
            R16G16_TYPELESS              = 33,
            R16G16_FLOAT                 = 34,
            R16G16_UNORM                 = 35,
            R16G16_UINT                  = 36,
            R16G16_SNORM                 = 37,
            R16G16_SINT                  = 38,
            R32_TYPELESS                 = 39,
            D32_FLOAT                    = 40,
            R32_FLOAT                    = 41,
            R32_UINT                     = 42,
            R32_SINT                     = 43,
            R24G8_TYPELESS               = 44,
            D24_UNORM_S8_UINT            = 45,
            R24_UNORM_X8_TYPELESS        = 46,
            X24_TYPELESS_G8_UINT         = 47,
            R8G8_TYPELESS                = 48,
            R8G8_UNORM                   = 49,
            R8G8_UINT                    = 50,
            R8G8_SNORM                   = 51,
            R8G8_SINT                    = 52,
            R16_TYPELESS                 = 53,
            R16_FLOAT                    = 54,
            D16_UNORM                    = 55,
            R16_UNORM                    = 56,
            R16_UINT                     = 57,
            R16_SNORM                    = 58,
            R16_SINT                     = 59,
            R8_TYPELESS                  = 60,
            R8_UNORM                     = 61,
            R8_UINT                      = 62,
            R8_SNORM                     = 63,
            R8_SINT                      = 64,
            A8_UNORM                     = 65,
            R1_UNORM                     = 66,
            R9G9B9E5_SHAREDEXP           = 67,
            R8G8_B8G8_UNORM              = 68,
            G8R8_G8B8_UNORM              = 69,
            BC1_TYPELESS                 = 70,
            BC1_UNORM                    = 71,
            BC1_UNORM_SRGB               = 72,
            BC2_TYPELESS                 = 73,
            BC2_UNORM                    = 74,
            BC2_UNORM_SRGB               = 75,
            BC3_TYPELESS                 = 76,
            BC3_UNORM                    = 77,
            BC3_UNORM_SRGB               = 78,
            BC4_TYPELESS                 = 79,
            BC4_UNORM                    = 80,
            BC4_SNORM                    = 81,
            BC5_TYPELESS                 = 82,
            BC5_UNORM                    = 83,
            BC5_SNORM                    = 84,
            B5G6R5_UNORM                 = 85,
            B5G5R5A1_UNORM               = 86,
            B8G8R8A8_UNORM               = 87,
            B8G8R8X8_UNORM               = 88,
            R10G10B10_XR_BIAS_A2_UNORM   = 89,
            B8G8R8A8_TYPELESS            = 90,
            B8G8R8A8_UNORM_SRGB          = 91,
            B8G8R8X8_TYPELESS            = 92,
            B8G8R8X8_UNORM_SRGB          = 93,
            BC6H_TYPELESS                = 94,
            BC6H_UF16                    = 95,
            BC6H_SF16                    = 96,
            BC7_TYPELESS                 = 97,
            BC7_UNORM                    = 98,
            BC7_UNORM_SRGB               = 99,
            AYUV                         = 100,
            Y410                         = 101,
            Y416                         = 102,
            NV12                         = 103,
            P010                         = 104,
            P016                         = 105,
            _420_OPAQUE                  = 106,
            YUY2                         = 107,
            Y210                         = 108,
            Y216                         = 109,
            NV11                         = 110,
            AI44                         = 111,
            IA44                         = 112,
            P8                           = 113,
            A8P8                         = 114,
            B4G4R4A4_UNORM               = 115,
        
            // Hardware specific
            R10G10B10_7E3_A2_FLOAT       = 116,
            R10G10B10_6E4_A2_FLOAT       = 117,
            D16_UNORM_S8_UINT            = 118,
            X16_TYPELESS_G8_UINT         = 120,

            // Dxgi 3.1 specific
            P208                         = 130,
            V208                         = 131,
            V408                         = 132,
        };

        constexpr uint32_t MakeFourCC(char ch0 = ' ', char ch1 = ' ', char ch2 = ' ', char ch3 = ' ')
        {
            return ((uint32_t)(uint8_t)(ch0)
                | ((uint32_t)(uint8_t)(ch1) << 8)
                | ((uint32_t)(uint8_t)(ch2) << 16)
                | ((uint32_t)(uint8_t)(ch3) << 24));
        }
    }

    DxgiFormat DxgiFormatFromRhiFormat(const RHI::Format format)
    {
        switch (format)
        {
        case RHI::Format::R32G32B32A32_FLOAT:
            return DxgiFormat::R32G32B32A32_FLOAT;
        case RHI::Format::R32G32B32A32_UINT:
            return DxgiFormat::R32G32B32A32_UINT;
        case RHI::Format::R32G32B32A32_SINT:
            return DxgiFormat::R32G32B32A32_SINT;

        case RHI::Format::R32G32B32_FLOAT:
            return DxgiFormat::R32G32B32_FLOAT;
        case RHI::Format::R32G32B32_UINT:
            return DxgiFormat::R32G32B32_UINT;
        case RHI::Format::R32G32B32_SINT:
            return DxgiFormat::R32G32B32_SINT;

        case RHI::Format::R16G16B16A16_FLOAT:
            return DxgiFormat::R16G16B16A16_FLOAT;
        case RHI::Format::R16G16B16A16_UNORM:
            return DxgiFormat::R16G16B16A16_UNORM;
        case RHI::Format::R16G16B16A16_UINT:
            return DxgiFormat::R16G16B16A16_UINT;
        case RHI::Format::R16G16B16A16_SNORM:
            return DxgiFormat::R16G16B16A16_SNORM;
        case RHI::Format::R16G16B16A16_SINT:
            return DxgiFormat::R16G16B16A16_SINT;

        case RHI::Format::R32G32_FLOAT:
            return DxgiFormat::R32G32_FLOAT;
        case RHI::Format::R32G32_UINT:
            return DxgiFormat::R32G32_UINT;
        case RHI::Format::R32G32_SINT:
            return DxgiFormat::R32G32_SINT;

        case RHI::Format::D32_FLOAT_S8X24_UINT:
            return DxgiFormat::D32_FLOAT_S8X24_UINT;

        case RHI::Format::R10G10B10A2_UNORM:
            return DxgiFormat::R10G10B10A2_UNORM;
        case RHI::Format::R10G10B10A2_UINT:
            return DxgiFormat::R10G10B10A2_UINT;

        case RHI::Format::R11G11B10_FLOAT:
            return DxgiFormat::R11G11B10_FLOAT;

        case RHI::Format::R8G8B8A8_UNORM:
            return DxgiFormat::R8G8B8A8_UNORM;
        case RHI::Format::R8G8B8A8_UNORM_SRGB:
            return DxgiFormat::R8G8B8A8_UNORM_SRGB;
        case RHI::Format::R8G8B8A8_UINT:
            return DxgiFormat::R8G8B8A8_UINT;
        case RHI::Format::R8G8B8A8_SNORM:
            return DxgiFormat::R8G8B8A8_SNORM;
        case RHI::Format::R8G8B8A8_SINT:
            return DxgiFormat::R8G8B8A8_SINT;

        case RHI::Format::R16G16_FLOAT:
            return DxgiFormat::R16G16_FLOAT;
        case RHI::Format::R16G16_UNORM:
            return DxgiFormat::R16G16_UNORM;
        case RHI::Format::R16G16_UINT:
            return DxgiFormat::R16G16_UINT;
        case RHI::Format::R16G16_SNORM:
            return DxgiFormat::R16G16_SNORM;
        case RHI::Format::R16G16_SINT:
            return DxgiFormat::R16G16_SINT;

        case RHI::Format::D32_FLOAT:
            return DxgiFormat::D32_FLOAT;
        case RHI::Format::R32_FLOAT:
            return DxgiFormat::R32_FLOAT;
        case RHI::Format::R32_UINT:
            return DxgiFormat::R32_UINT;
        case RHI::Format::R32_SINT:
            return DxgiFormat::R32_SINT;

        case RHI::Format::D24_UNORM_S8_UINT:
            return DxgiFormat::D24_UNORM_S8_UINT;

        case RHI::Format::R8G8_UNORM:
            return DxgiFormat::R8G8_UNORM;
        case RHI::Format::R8G8_UINT:
            return DxgiFormat::R8G8_UINT;
        case RHI::Format::R8G8_SNORM:
            return DxgiFormat::R8G8_SNORM;
        case RHI::Format::R8G8_SINT:
            return DxgiFormat::R8G8_SINT;

        case RHI::Format::R16_FLOAT:
            return DxgiFormat::R16_FLOAT;
        case RHI::Format::D16_UNORM:
            return DxgiFormat::D16_UNORM;
        case RHI::Format::R16_UNORM:
            return DxgiFormat::R16_UNORM;
        case RHI::Format::R16_UINT:
            return DxgiFormat::R16_UINT;
        case RHI::Format::R16_SNORM:
            return DxgiFormat::R16_SNORM;
        case RHI::Format::R16_SINT:
            return DxgiFormat::R16_SINT;

        case RHI::Format::R8_UNORM:
            return DxgiFormat::R8_UNORM;
        case RHI::Format::R8_UINT:
            return DxgiFormat::R8_UINT;
        case RHI::Format::R8_SNORM:
            return DxgiFormat::R8_SNORM;
        case RHI::Format::R8_SINT:
            return DxgiFormat::R8_SINT;
        case RHI::Format::A8_UNORM:
            return DxgiFormat::A8_UNORM;
        case RHI::Format::R1_UNORM:
            return DxgiFormat::R1_UNORM;

        case RHI::Format::R9G9B9E5_SHAREDEXP:
            return DxgiFormat::R9G9B9E5_SHAREDEXP;

        case RHI::Format::R8G8_B8G8_UNORM:
            return DxgiFormat::R8G8_B8G8_UNORM;
        case RHI::Format::G8R8_G8B8_UNORM:
            return DxgiFormat::G8R8_G8B8_UNORM;

        case RHI::Format::BC1_UNORM:
            return DxgiFormat::BC1_UNORM;
        case RHI::Format::BC1_UNORM_SRGB:
            return DxgiFormat::BC1_UNORM_SRGB;
        case RHI::Format::BC2_UNORM:
            return DxgiFormat::BC2_UNORM;
        case RHI::Format::BC2_UNORM_SRGB:
            return DxgiFormat::BC2_UNORM_SRGB;
        case RHI::Format::BC3_UNORM:
            return DxgiFormat::BC3_UNORM;
        case RHI::Format::BC3_UNORM_SRGB:
            return DxgiFormat::BC3_UNORM_SRGB;
        case RHI::Format::BC4_UNORM:
            return DxgiFormat::BC4_UNORM;
        case RHI::Format::BC4_SNORM:
            return DxgiFormat::BC4_SNORM;
        case RHI::Format::BC5_UNORM:
            return DxgiFormat::BC5_UNORM;
        case RHI::Format::BC5_SNORM:
            return DxgiFormat::BC5_SNORM;

        case RHI::Format::B5G6R5_UNORM:
            return DxgiFormat::B5G6R5_UNORM;
        case RHI::Format::B5G5R5A1_UNORM:
            return DxgiFormat::B5G5R5A1_UNORM;
        case RHI::Format::B8G8R8A8_UNORM:
            return DxgiFormat::B8G8R8A8_UNORM;
        case RHI::Format::B8G8R8X8_UNORM:
            return DxgiFormat::B8G8R8X8_UNORM;
        case RHI::Format::R10G10B10_XR_BIAS_A2_UNORM:
            return DxgiFormat::R10G10B10_XR_BIAS_A2_UNORM;
        case RHI::Format::B8G8R8A8_UNORM_SRGB:
            return DxgiFormat::B8G8R8A8_UNORM_SRGB;
        case RHI::Format::B8G8R8X8_UNORM_SRGB:
            return DxgiFormat::B8G8R8X8_UNORM_SRGB;

        case RHI::Format::BC6H_UF16:
            return DxgiFormat::BC6H_UF16;
        case RHI::Format::BC6H_SF16:
            return DxgiFormat::BC6H_SF16;

        case RHI::Format::BC7_UNORM:
            return DxgiFormat::BC7_UNORM;
        case RHI::Format::BC7_UNORM_SRGB:
            return DxgiFormat::BC7_UNORM_SRGB;

        case RHI::Format::AYUV:
            return DxgiFormat::AYUV;
        case RHI::Format::Y410:
            return DxgiFormat::Y410;
        case RHI::Format::Y416:
            return DxgiFormat::Y416;
        case RHI::Format::NV12:
            return DxgiFormat::NV12;
        case RHI::Format::P010:
            return DxgiFormat::P010;
        case RHI::Format::P016:
            return DxgiFormat::P016;
        case RHI::Format::YUY2:
            return DxgiFormat::YUY2;
        case RHI::Format::Y210:
            return DxgiFormat::Y210;
        case RHI::Format::Y216:
            return DxgiFormat::Y216;
        case RHI::Format::NV11:
            return DxgiFormat::NV11;
        case RHI::Format::AI44:
            return DxgiFormat::AI44;
        case RHI::Format::IA44:
            return DxgiFormat::IA44;
        case RHI::Format::P8:
            return DxgiFormat::P8;
        case RHI::Format::A8P8:
            return DxgiFormat::A8P8;
        case RHI::Format::B4G4R4A4_UNORM:
            return DxgiFormat::B4G4R4A4_UNORM;
        case RHI::Format::R10G10B10_7E3_A2_FLOAT:
            return DxgiFormat::R10G10B10_7E3_A2_FLOAT;
        case RHI::Format::R10G10B10_6E4_A2_FLOAT:
            return DxgiFormat::R10G10B10_6E4_A2_FLOAT;
        case RHI::Format::D16_UNORM_S8_UINT:
            return DxgiFormat::D16_UNORM_S8_UINT;
        case RHI::Format::X16_TYPELESS_G8_UINT:
            return DxgiFormat::X16_TYPELESS_G8_UINT;
        case RHI::Format::P208:
            return DxgiFormat::P208;
        case RHI::Format::V208:
            return DxgiFormat::V208;
        case RHI::Format::V408:
            return DxgiFormat::V408;

        case RHI::Format::EAC_R11_UNORM:
        case RHI::Format::EAC_R11_SNORM:
        case RHI::Format::EAC_RG11_UNORM:
        case RHI::Format::EAC_RG11_SNORM:
        case RHI::Format::ETC2_UNORM:
        case RHI::Format::ETC2_UNORM_SRGB:
        case RHI::Format::ETC2A_UNORM:
        case RHI::Format::ETC2A_UNORM_SRGB:
        case RHI::Format::ETC2A1_UNORM:
        case RHI::Format::ETC2A1_UNORM_SRGB:
            return DxgiFormat::UNKNOWN; // Not supported by dds

        case RHI::Format::PVRTC2_UNORM:
        case RHI::Format::PVRTC2_UNORM_SRGB:
        case RHI::Format::PVRTC4_UNORM:
        case RHI::Format::PVRTC4_UNORM_SRGB:
            return DxgiFormat::UNKNOWN; // Not supported by dds

        case RHI::Format::ASTC_4x4_UNORM:
        case RHI::Format::ASTC_4x4_UNORM_SRGB:
        case RHI::Format::ASTC_5x4_UNORM:
        case RHI::Format::ASTC_5x4_UNORM_SRGB:
        case RHI::Format::ASTC_5x5_UNORM:
        case RHI::Format::ASTC_5x5_UNORM_SRGB:
        case RHI::Format::ASTC_6x5_UNORM:
        case RHI::Format::ASTC_6x5_UNORM_SRGB:
        case RHI::Format::ASTC_6x6_UNORM:
        case RHI::Format::ASTC_6x6_UNORM_SRGB:
        case RHI::Format::ASTC_8x5_UNORM:
        case RHI::Format::ASTC_8x5_UNORM_SRGB:
        case RHI::Format::ASTC_8x6_UNORM:
        case RHI::Format::ASTC_8x6_UNORM_SRGB:
        case RHI::Format::ASTC_8x8_UNORM:
        case RHI::Format::ASTC_8x8_UNORM_SRGB:
        case RHI::Format::ASTC_10x5_UNORM:
        case RHI::Format::ASTC_10x5_UNORM_SRGB:
        case RHI::Format::ASTC_10x6_UNORM:
        case RHI::Format::ASTC_10x6_UNORM_SRGB:
        case RHI::Format::ASTC_10x8_UNORM:
        case RHI::Format::ASTC_10x8_UNORM_SRGB:
        case RHI::Format::ASTC_10x10_UNORM:
        case RHI::Format::ASTC_10x10_UNORM_SRGB:
        case RHI::Format::ASTC_12x10_UNORM:
        case RHI::Format::ASTC_12x10_UNORM_SRGB:
        case RHI::Format::ASTC_12x12_UNORM:
        case RHI::Format::ASTC_12x12_UNORM_SRGB:
            return DxgiFormat::UNKNOWN; // Not supported by dds
        
        default:
            return DxgiFormat::UNKNOWN;
        }
    }

    DdsFlags PitchOrLinearSizeFromFormat(const RHI::Format format)
    {
        switch (format)
        {
        case RHI::Format::R32G32B32A32_FLOAT:
        case RHI::Format::R32G32B32A32_UINT:
        case RHI::Format::R32G32B32A32_SINT:
        case RHI::Format::R32G32B32_FLOAT:
        case RHI::Format::R32G32B32_UINT:
        case RHI::Format::R32G32B32_SINT:
        case RHI::Format::R16G16B16A16_FLOAT:
        case RHI::Format::R16G16B16A16_UNORM:
        case RHI::Format::R16G16B16A16_UINT:
        case RHI::Format::R16G16B16A16_SNORM:
        case RHI::Format::R16G16B16A16_SINT:
        case RHI::Format::R32G32_FLOAT:
        case RHI::Format::R32G32_UINT:
        case RHI::Format::R32G32_SINT:
        case RHI::Format::D32_FLOAT_S8X24_UINT:
        case RHI::Format::R10G10B10A2_UNORM:
        case RHI::Format::R10G10B10A2_UINT:
        case RHI::Format::R11G11B10_FLOAT:
        case RHI::Format::R8G8B8A8_UNORM:
        case RHI::Format::R8G8B8A8_UNORM_SRGB:
        case RHI::Format::R8G8B8A8_UINT:
        case RHI::Format::R8G8B8A8_SNORM:
        case RHI::Format::R8G8B8A8_SINT:
        case RHI::Format::R16G16_FLOAT:
        case RHI::Format::R16G16_UNORM:
        case RHI::Format::R16G16_UINT:
        case RHI::Format::R16G16_SNORM:
        case RHI::Format::R16G16_SINT:
        case RHI::Format::D32_FLOAT:
        case RHI::Format::R32_FLOAT:
        case RHI::Format::R32_UINT:
        case RHI::Format::R32_SINT:
        case RHI::Format::D24_UNORM_S8_UINT:
        case RHI::Format::R8G8_UNORM:
        case RHI::Format::R8G8_UINT:
        case RHI::Format::R8G8_SNORM:
        case RHI::Format::R8G8_SINT:
        case RHI::Format::R16_FLOAT:
        case RHI::Format::D16_UNORM:
        case RHI::Format::R16_UNORM:
        case RHI::Format::R16_UINT:
        case RHI::Format::R16_SNORM:
        case RHI::Format::R16_SINT:
        case RHI::Format::R8_UNORM:
        case RHI::Format::R8_UINT:
        case RHI::Format::R8_SNORM:
        case RHI::Format::R8_SINT:
        case RHI::Format::A8_UNORM:
        case RHI::Format::R1_UNORM:
        case RHI::Format::R9G9B9E5_SHAREDEXP:
        case RHI::Format::R8G8_B8G8_UNORM:
        case RHI::Format::G8R8_G8B8_UNORM:
            return DdsFlags::Pitch;

        case RHI::Format::BC1_UNORM:
        case RHI::Format::BC1_UNORM_SRGB:
        case RHI::Format::BC2_UNORM:
        case RHI::Format::BC2_UNORM_SRGB:
        case RHI::Format::BC3_UNORM:
        case RHI::Format::BC3_UNORM_SRGB:
        case RHI::Format::BC4_UNORM:
        case RHI::Format::BC4_SNORM:
        case RHI::Format::BC5_UNORM:
        case RHI::Format::BC5_SNORM:
            return DdsFlags::LinearSize;

        case RHI::Format::B5G6R5_UNORM:
        case RHI::Format::B5G5R5A1_UNORM:
        case RHI::Format::B8G8R8A8_UNORM:
        case RHI::Format::B8G8R8X8_UNORM:
        case RHI::Format::R10G10B10_XR_BIAS_A2_UNORM:
        case RHI::Format::B8G8R8A8_UNORM_SRGB:
        case RHI::Format::B8G8R8X8_UNORM_SRGB:
            return DdsFlags::Pitch;

        case RHI::Format::BC6H_UF16:
        case RHI::Format::BC6H_SF16:
        case RHI::Format::BC7_UNORM:
        case RHI::Format::BC7_UNORM_SRGB:
            return DdsFlags::LinearSize;

        case RHI::Format::AYUV:
        case RHI::Format::Y410:
        case RHI::Format::Y416:
        case RHI::Format::NV12:
        case RHI::Format::P010:
        case RHI::Format::P016:
        case RHI::Format::YUY2:
        case RHI::Format::Y210:
        case RHI::Format::Y216:
        case RHI::Format::NV11:
        case RHI::Format::AI44:
        case RHI::Format::IA44:
        case RHI::Format::P8:
        case RHI::Format::A8P8:
        case RHI::Format::B4G4R4A4_UNORM:
        case RHI::Format::R10G10B10_7E3_A2_FLOAT:
        case RHI::Format::R10G10B10_6E4_A2_FLOAT:
        case RHI::Format::D16_UNORM_S8_UINT:
        case RHI::Format::X16_TYPELESS_G8_UINT:
        case RHI::Format::P208:
        case RHI::Format::V208:
        case RHI::Format::V408:
            return DdsFlags::Pitch;

        case RHI::Format::EAC_R11_UNORM:
        case RHI::Format::EAC_R11_SNORM:
        case RHI::Format::EAC_RG11_UNORM:
        case RHI::Format::EAC_RG11_SNORM:
        case RHI::Format::ETC2_UNORM:
        case RHI::Format::ETC2_UNORM_SRGB:
        case RHI::Format::ETC2A_UNORM:
        case RHI::Format::ETC2A_UNORM_SRGB:
        case RHI::Format::ETC2A1_UNORM:
        case RHI::Format::ETC2A1_UNORM_SRGB:
            return DdsFlags::Unknown; // Not supported by dds

        case RHI::Format::PVRTC2_UNORM:
        case RHI::Format::PVRTC2_UNORM_SRGB:
        case RHI::Format::PVRTC4_UNORM:
        case RHI::Format::PVRTC4_UNORM_SRGB:
            return DdsFlags::Unknown; // Not supported by dds

        case RHI::Format::ASTC_4x4_UNORM:
        case RHI::Format::ASTC_4x4_UNORM_SRGB:
        case RHI::Format::ASTC_5x4_UNORM:
        case RHI::Format::ASTC_5x4_UNORM_SRGB:
        case RHI::Format::ASTC_5x5_UNORM:
        case RHI::Format::ASTC_5x5_UNORM_SRGB:
        case RHI::Format::ASTC_6x5_UNORM:
        case RHI::Format::ASTC_6x5_UNORM_SRGB:
        case RHI::Format::ASTC_6x6_UNORM:
        case RHI::Format::ASTC_6x6_UNORM_SRGB:
        case RHI::Format::ASTC_8x5_UNORM:
        case RHI::Format::ASTC_8x5_UNORM_SRGB:
        case RHI::Format::ASTC_8x6_UNORM:
        case RHI::Format::ASTC_8x6_UNORM_SRGB:
        case RHI::Format::ASTC_8x8_UNORM:
        case RHI::Format::ASTC_8x8_UNORM_SRGB:
        case RHI::Format::ASTC_10x5_UNORM:
        case RHI::Format::ASTC_10x5_UNORM_SRGB:
        case RHI::Format::ASTC_10x6_UNORM:
        case RHI::Format::ASTC_10x6_UNORM_SRGB:
        case RHI::Format::ASTC_10x8_UNORM:
        case RHI::Format::ASTC_10x8_UNORM_SRGB:
        case RHI::Format::ASTC_10x10_UNORM:
        case RHI::Format::ASTC_10x10_UNORM_SRGB:
        case RHI::Format::ASTC_12x10_UNORM:
        case RHI::Format::ASTC_12x10_UNORM_SRGB:
        case RHI::Format::ASTC_12x12_UNORM:
        case RHI::Format::ASTC_12x12_UNORM_SRGB:
            return DdsFlags::Unknown; // Not supported by dds

        default:
            return DdsFlags::Unknown;
        }
    }

    // DdsFile function definitions

    DdsFile::DdsFile()
        : m_magic(MakeFourCC('D', 'D', 'S'))
    {

    }

    DdsFile::~DdsFile()
    {
    }

    void DdsFile::SetSize(RHI::Size size)
    {
        m_header.m_width = size.m_width;
        m_header.m_height = size.m_height;

        if (size.m_depth > 1)
        {
            m_header.m_flags |= DdsFlags::Depth;
            m_header.m_depth = size.m_depth;
            SetAsVolumeTexture();
        }
        else
        {
            m_header.m_flags &= ~DdsFlags::Depth;
            m_header.m_depth = 1;
        }

        m_headerDx10.m_arraySize = 1;

        if (size.m_width > 1 && size.m_height > 1 && size.m_depth > 1)
        {
            m_headerDx10.m_resourceDimension = static_cast<uint32_t>(ResourceDimension::Texture3D);
        }
        else if (size.m_width > 1 && size.m_height > 1)
        {
            m_headerDx10.m_resourceDimension = static_cast<uint32_t>(ResourceDimension::Texture2D);
        }
        else if (size.m_width > 1)
        {
            m_headerDx10.m_resourceDimension = static_cast<uint32_t>(ResourceDimension::Texture1D);
        }
        else
        {
            m_headerDx10.m_resourceDimension = static_cast<uint32_t>(ResourceDimension::Unknown);
        }

        ResolvePitch();
    }

    RHI::Size DdsFile::GetSize()
    {
        return RHI::Size(m_header.m_width, m_header.m_height, m_header.m_depth);
    }

    void DdsFile::SetFormat(RHI::Format format)
    {
        m_externalFormat = format;
        m_headerDx10.m_dxgiFormat = static_cast<uint32_t>(DxgiFormatFromRhiFormat(format));

        ResolvePitch();
    }

    RHI::Format DdsFile::GetFormat()
    {
        return m_externalFormat;
    }

    void DdsFile::SetAsCubemap()
    {
        m_header.m_caps |= DdsCaps::Complex;
        m_header.m_caps_2 |= DdsCaps2::CubemapAll;
        m_headerDx10.m_miscFlag |= Dx10MiscFlags::TextureCube;
    }

    void DdsFile::SetAsVolumeTexture()
    {
        m_header.m_caps |= DdsCaps::Complex;
        m_header.m_caps_2 |= DdsCaps2::Volume;
    }

    void DdsFile::SetMipLevels(uint32_t mipLevels)
    {
        m_header.m_mipMapCount = mipLevels;
        if (mipLevels > 1)
        {
            m_header.m_mipMapCount = mipLevels;
            m_header.m_caps |= DdsCaps::Complex | DdsCaps::Mipmap;
            m_header.m_flags |= DdsFlags::MipmapCount;
        }
    }

    uint32_t DdsFile::GetMipLevels()
    {
        return m_header.m_mipMapCount;
    }

    AZ::Outcome<void, DdsFile::DdsFailure> DdsFile::WriteHeaderToStream(AZ::IO::GenericStream& stream)
    {
        size_t bytesWritten = 0;
        bytesWritten += stream.Write(sizeof(m_magic), &m_magic);
        bytesWritten += stream.Write(sizeof(m_header), &m_header);
        bytesWritten += stream.Write(sizeof(m_headerDx10), &m_headerDx10);

        if (bytesWritten != sizeof(m_magic) + sizeof(m_header) + sizeof(m_headerDx10))
        {
            DdsFailure failure;
            failure.m_code = DdsFailure::Code::WriteFailed;
            failure.m_message.format("Failed to write all data to the stream");
            return AZ::Failure(failure);
        }

        return AZ::Success();
    }

    void DdsFile::ResolvePitch()
    {
        if (m_externalFormat != RHI::Format::Unknown)
        {
            m_header.m_flags &= ~(DdsFlags::Pitch | DdsFlags::LinearSize); // clear out the flags first.
            DdsFlags pitchOrLinearSize = PitchOrLinearSizeFromFormat(m_externalFormat);
            m_header.m_flags |= pitchOrLinearSize;

            RHI::DeviceImageSubresourceLayout layout = RHI::GetImageSubresourceLayout(GetSize(), m_externalFormat);
            m_header.m_pitchOrLinearSize = layout.m_bytesPerRow;
        }
    }

    AZ::Outcome<void, DdsFile::DdsFailure> DdsFile::WriteFile(const AZStd::string& filePath, const DdsFileData& ddsFiledata)
    {
        DdsFile ddsFile;
        ddsFile.SetSize(ddsFiledata.m_size);
        ddsFile.SetFormat(ddsFiledata.m_format);
        if (ddsFiledata.m_isCubemap)
        {
            ddsFile.SetAsCubemap();
        }
        ddsFile.SetMipLevels(ddsFiledata.m_mipLevels);

        IO::FileIOStream fileStream(filePath.c_str(), IO::OpenMode::ModeWrite| IO::OpenMode::ModeCreatePath);

        if (!fileStream.IsOpen())
        {
            DdsFailure failure;
            failure.m_code = DdsFailure::Code::OpenFileFailed;
            failure.m_message.format("Failed to open \"%s\" for writing.", filePath.c_str());
            return AZ::Failure(failure);
        }

        auto outcome = ddsFile.WriteHeaderToStream(fileStream);
        size_t bytesWritten = fileStream.Write(ddsFiledata.m_buffer->size(), ddsFiledata.m_buffer->data());

        if (!outcome || bytesWritten != ddsFiledata.m_buffer->size())
        {
            DdsFailure failure;
            failure.m_code = DdsFailure::Code::WriteFailed;
            failure.m_message.format("Failed to write all data to \"%s\"", filePath.c_str());
            return AZ::Failure(failure);
        }

        return AZ::Success();
    }

    bool DdsFile::DoesSupportFormat(RHI::Format format)
    {
        return DxgiFormatFromRhiFormat(format) != DxgiFormat::UNKNOWN;
    }

    DdsFile::DdsPixelFormat::DdsPixelFormat()
        // By default, the pixel format struct is only there to point to the dx10 header by setting the fourCC to 'DX10'
        : m_fourCC(MakeFourCC('D', 'X', '1', '0'))
        , m_flags(PixelFormatFlags::FourCC)
    {
    }

    DdsFile::DdsHeader::DdsHeader()
        : m_flags(DdsFlags::Caps | DdsFlags::Height | DdsFlags::Width | DdsFlags::PixelFormat) // Set flags required by the DDS spec
        , m_caps(DdsCaps::Texture) // the texture flag is required by the DDS spec
    {

    }
    
}// namespace AZ
