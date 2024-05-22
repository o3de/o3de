/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <Atom/RHI.Reflect/ImageEnums.h>
#include <Atom/RHI.Reflect/Size.h>

namespace AZ::RHI
{
    enum class Format : uint32_t
    {
        Unknown = 0,

        R32G32B32A32_FLOAT,
        R32G32B32A32_UINT,
        R32G32B32A32_SINT,

        R32G32B32_FLOAT,
        R32G32B32_UINT,
        R32G32B32_SINT,

        R16G16B16A16_FLOAT,
        R16G16B16A16_UNORM,
        R16G16B16A16_UINT,
        R16G16B16A16_SNORM,
        R16G16B16A16_SINT,

        R32G32_FLOAT,
        R32G32_UINT,
        R32G32_SINT,

        D32_FLOAT_S8X24_UINT,

        R10G10B10A2_UNORM,
        R10G10B10A2_UINT,

        R11G11B10_FLOAT,

        R8G8B8A8_UNORM,
        R8G8B8A8_UNORM_SRGB,
        R8G8B8A8_UINT,
        R8G8B8A8_SNORM,
        R8G8B8A8_SINT,

        R16G16_FLOAT,
        R16G16_UNORM,
        R16G16_UINT,
        R16G16_SNORM,
        R16G16_SINT,

        D32_FLOAT,
        R32_FLOAT,
        R32_UINT,
        R32_SINT,

        D24_UNORM_S8_UINT,

        R8G8_UNORM,
        R8G8_UNORM_SRGB,
        R8G8_UINT,
        R8G8_SNORM,
        R8G8_SINT,

        R16_FLOAT,
        D16_UNORM,
        R16_UNORM,
        R16_UINT,
        R16_SNORM,
        R16_SINT,

        R8_UNORM,
        R8_UNORM_SRGB,
        R8_UINT,
        R8_SNORM,
        R8_SINT,
        A8_UNORM,
        R1_UNORM,

        R9G9B9E5_SHAREDEXP,

        R8G8_B8G8_UNORM,
        G8R8_G8B8_UNORM,

        BC1_UNORM,
        BC1_UNORM_SRGB,
        BC2_UNORM,
        BC2_UNORM_SRGB,
        BC3_UNORM,
        BC3_UNORM_SRGB,
        BC4_UNORM,
        BC4_SNORM,
        BC5_UNORM,
        BC5_SNORM,

        B5G6R5_UNORM,
        B5G5R5A1_UNORM,
        A1B5G5R5_UNORM,
        B8G8R8A8_UNORM,
        B8G8R8X8_UNORM,
        R10G10B10_XR_BIAS_A2_UNORM,
        B8G8R8A8_UNORM_SRGB,
        B8G8R8X8_UNORM_SRGB,

        BC6H_UF16,
        BC6H_SF16,

        BC7_UNORM,
        BC7_UNORM_SRGB,

        AYUV,
        Y410,
        Y416,
        NV12,
        P010,
        P016,
        YUY2,
        Y210,
        Y216,
        NV11,
        AI44,
        IA44,
        P8,
        A8P8,
        B4G4R4A4_UNORM,
        R4G4B4A4_UNORM,
        R10G10B10_7E3_A2_FLOAT,
        R10G10B10_6E4_A2_FLOAT,
        D16_UNORM_S8_UINT,
        X16_TYPELESS_G8_UINT,
        P208,
        V208,
        V408,

        EAC_R11_UNORM,
        EAC_R11_SNORM,
        EAC_RG11_UNORM,
        EAC_RG11_SNORM,
        ETC2_UNORM,
        ETC2_UNORM_SRGB,
        ETC2A_UNORM,
        ETC2A_UNORM_SRGB,
        ETC2A1_UNORM,
        ETC2A1_UNORM_SRGB,

        PVRTC2_UNORM,
        PVRTC2_UNORM_SRGB,
        PVRTC4_UNORM,
        PVRTC4_UNORM_SRGB,

        ASTC_4x4_UNORM,
        ASTC_4x4_UNORM_SRGB,
        ASTC_5x4_UNORM,
        ASTC_5x4_UNORM_SRGB,
        ASTC_5x5_UNORM,
        ASTC_5x5_UNORM_SRGB,
        ASTC_6x5_UNORM,
        ASTC_6x5_UNORM_SRGB,
        ASTC_6x6_UNORM,
        ASTC_6x6_UNORM_SRGB,
        ASTC_8x5_UNORM,
        ASTC_8x5_UNORM_SRGB,
        ASTC_8x6_UNORM,
        ASTC_8x6_UNORM_SRGB,
        ASTC_8x8_UNORM,
        ASTC_8x8_UNORM_SRGB,
        ASTC_10x5_UNORM,
        ASTC_10x5_UNORM_SRGB,
        ASTC_10x6_UNORM,
        ASTC_10x6_UNORM_SRGB,
        ASTC_10x8_UNORM,
        ASTC_10x8_UNORM_SRGB,
        ASTC_10x10_UNORM,
        ASTC_10x10_UNORM_SRGB,
        ASTC_12x10_UNORM,
        ASTC_12x10_UNORM_SRGB,
        ASTC_12x12_UNORM,
        ASTC_12x12_UNORM_SRGB,

        A8B8G8R8_UNORM,
        A8B8G8R8_UNORM_SRGB,
        A8B8G8R8_SNORM,
        R5G6B5_UNORM,
        B8G8R8A8_SNORM,

        Count
    };

    /// Capabilities of a format.
    enum class FormatCapabilities : uint32_t
    {
        None = 0,
        VertexBuffer = AZ_BIT(0),   // The format can be used to create a vertex buffer.
        IndexBuffer = AZ_BIT(1),    // The format can be used to create an index buffer.
        RenderTarget = AZ_BIT(2),   // The format can be used to create a rendertarget image.
        DepthStencil = AZ_BIT(3),   // The format can be used to create a depth/stencil image.
        Blend = AZ_BIT(4),          // The format can be used to create a rendertarget image that supports blending operations.
        Sample = AZ_BIT(5),         // The format can be used to create an image that supports being sample from a shader.
        TypedLoadBuffer = AZ_BIT(6),// The format can be used to create a buffer that supports typed load operations.  
        TypedStoreBuffer = AZ_BIT(7),   // The format can be used to create a buffer that supports typed store operations.  
        AtomicBuffer = AZ_BIT(8),   // The format can be used to create a buffer that supports atomic operations.
        ShadingRate = AZ_BIT(9),    // The format can be used to create a shading rate image.
    };

    AZ_DEFINE_ENUM_BITWISE_OPERATORS(AZ::RHI::FormatCapabilities);

    //! @brief Returns the number of bytes it takes to represent an image element using the provided format.
    //! It returns the number of bytes in a block if the given format is a block compressed format like BC1,
    //! and it do the number of bytes in a pixel otherwise.
    uint32_t GetFormatSize(Format format);

    //! @brief Returns the number of elements represented by the provided format. For example, R32G32B32_FLOAT returns 3, 
    //!        and R32G32_UINT returns 2. This function is intended primarily for validation of vertex input stream formats.
    uint32_t GetFormatComponentCount(Format format);

    const char* ToString(Format format);

    //! @brief Returns the required alignment for width / height of an image for the given format. Block
    //! compressed formats will return the number of blocks (e.g 4x4, 8x8, etc). Certain Packed / Planar formats may return 2.
    //!  This is especially important for low level of detail mips in the chain, which will reduce down to the alignment instead
    //! of 1x1. The returned value only applies to image width and height. Depth is unaffected.
    RHI::Size GetFormatDimensionAlignment(Format format);

    //! @brief Returns the SRGB equivalent to the provided linear format, if it exists. If not, the provided
    //! format is returned unchanged.
    RHI::Format ConvertLinearToSRGB(Format format);

    //! @brief Returns the linear equivalent to the provided SRGB format. If no such equivalent exists,
    //! the provided format is returned unchanged.
    RHI::Format ConvertSRGBToLinear(Format format);

    //! @brief Returns the image aspect flags supported by the format.
    ImageAspectFlags GetImageAspectFlags(Format format);

    AZ_TYPE_INFO_SPECIALIZE(Format, "{92CC7BFB-4F2B-45F9-A951-B4EBBCD485B8}");
}
