/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/std/algorithm.h>
#include <ImageProcessing_Traits_Platform.h>
#include <ImageBuilderBaseType.h>

#define IMAGE_BUIDER_MAKEFOURCC(ch0, ch1, ch2, ch3)           \
    ((AZ::u32)(AZ::u8)(ch0) | ((AZ::u32)(AZ::u8)(ch1) << 8) | \
     ((AZ::u32)(AZ::u8)(ch2) << 16) | ((AZ::u32)(AZ::u8)(ch3) << 24))

// This header defines constants and structures that are useful when parsing
// DDS files.  DDS files were originally designed to use several structures
// and constants that are native to DirectDraw and are defined in ddraw.h,
// such as DDSURFACEDESC2 and DDSCAPS2.  This file defines similar
// (compatible) constants and structures so that one can use DDS files
// without needing to include ddraw.h.


//Needed to write out DDS files on Mac
#if AZ_TRAIT_IMAGEPROCESSING_DEFINE_DIRECT3D_CONSTANTS
#define    DDPF_ALPHAPIXELS     0x00000001  // Texture contains alpha data
#define    DDPF_ALPHA           0x00000002  // For alpha channel only uncompressed data
#define    DDPF_FOURCC          0x00000004  // Texture contains compressed RGB data
#define    DDPF_RGB             0x00000040  // Texture contains uncompressed RGB data
#define    DDPF_YUV             0x00000200  // For YUV uncompressed data
#define    DDPF_LUMINANCE       0x00020000  // For single channel color uncompressed data

#define    DDSCAPS_COMPLEX      0x00000008  // Must be used on any file that contains more than one surface
#define    DDSCAPS_MIPMAP       0x00400000  // Should be used for a mipmap
#define    DDSCAPS_TEXTURE      0x00001000  // Required
#endif

#define DDS_FOURCC      0x00000004  // DDPF_FOURCC
#define DDS_RGB         0x00000040  // DDPF_RGB
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_SIGNED      0x00080000  // DDPF_SIGNED; only used for lumbyard dds
#define DDS_RGBA        0x00000041  // DDPF_RGB | DDPF_ALPHAPIXELS
#define DDS_LUMINANCEA  0x00020001  // DDS_LUMINANCE | DDPF_ALPHAPIXELS
#define DDS_A           0x00000001  // DDPF_ALPHAPIXELS
#define DDS_A_ONLY      0x00000002  // DDPF_ALPHA

#define DDS_FOURCC_A16B16G16R16   0x00000024  // FOURCC A16B16G16R16
#define DDS_FOURCC_V16U16         0x00000040  // FOURCC V16U16
#define DDS_FOURCC_Q16W16V16U16   0x0000006E  // FOURCC Q16W16V16U16
#define DDS_FOURCC_R16F           0x0000006F  // FOURCC R16F
#define DDS_FOURCC_G16R16F        0x00000070  // FOURCC G16R16F
#define DDS_FOURCC_A16B16G16R16F  0x00000071  // FOURCC A16B16G16R16F
#define DDS_FOURCC_R32F           0x00000072  // FOURCC R32F
#define DDS_FOURCC_G32R32F        0x00000073  // FOURCC G32R32F
#define DDS_FOURCC_A32B32G32R32F  0x00000074  // FOURCC A32B32G32R32F

#define DDSD_CAPS         0x00000001l   // default
#define DDSD_PIXELFORMAT  0x00001000l
#define DDSD_WIDTH        0x00000004l
#define DDSD_HEIGHT       0x00000002l
#define DDSD_LINEARSIZE   0x00080000l

#define DDS_HEADER_FLAGS_TEXTURE    0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT
#define DDS_HEADER_FLAGS_MIPMAP     0x00020000  // DDSD_MIPMAPCOUNT
#define DDS_HEADER_FLAGS_VOLUME     0x00800000  // DDSD_DEPTH
#define DDS_HEADER_FLAGS_PITCH      0x00000008  // DDSD_PITCH
#define DDS_HEADER_FLAGS_LINEARSIZE 0x00080000  // DDSD_LINEARSIZE

#define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE
#define DDS_SURFACE_FLAGS_MIPMAP  0x00400008 // DDSCAPS_COMPLEX | DDSCAPS_MIPMAP
#define DDS_SURFACE_FLAGS_CUBEMAP 0x00000008 // DDSCAPS_COMPLEX

#define DDS_CUBEMAP           0x00000200 // DDSCAPS2_CUBEMAP
#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES (DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX | \
                              DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY | \
                              DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ)

#define DDS_FLAGS_VOLUME 0x00200000 // DDSCAPS2_VOLUME

#define DDS_RESF1_NORMALMAP 0x01000000
#define DDS_RESF1_DSDT      0x02000000

namespace ImageProcessingAtom
{
    const static AZ::u32 FOURCC_DX10 = IMAGE_BUIDER_MAKEFOURCC('D', 'X', '1', '0');
    const static AZ::u32 FOURCC_DDS = IMAGE_BUIDER_MAKEFOURCC('D', 'D', 'S', ' ');
    const static AZ::u32 FOURCC_FYRC = IMAGE_BUIDER_MAKEFOURCC('F', 'Y', 'R', 'C');

    //The values of each elements in this enum should be same as ITexture ETEX_TileMode enum.
    enum DDS_TileMode : AZ::u8
    {
        eTM_None = 0,
        eTM_LinearPadded,
        eTM_Optimal,
    };

    struct DDS_PIXELFORMAT
    {
        AZ::u32 dwSize;
        AZ::u32 dwFlags;
        AZ::u32 dwFourCC;
        AZ::u32 dwRGBBitCount;
        AZ::u32 dwRBitMask;
        AZ::u32 dwGBitMask;
        AZ::u32 dwBBitMask;
        AZ::u32 dwABitMask;

        const bool operator == (const DDS_PIXELFORMAT& fmt) const
        {
            return dwFourCC == fmt.dwFourCC &&
                   dwFlags == fmt.dwFlags &&
                   dwRGBBitCount == fmt.dwRGBBitCount &&
                   dwRBitMask == fmt.dwRBitMask &&
                   dwGBitMask == fmt.dwGBitMask &&
                   dwBBitMask == fmt.dwBBitMask &&
                   dwABitMask == fmt.dwABitMask &&
                   dwSize == fmt.dwSize;
        }
    };

    struct DDS_HEADER_DXT10
    {
        // we're unable to use native enums, so we use AZ::u32 instead.
        AZ::u32 /*DXGI_FORMAT*/ dxgiFormat;
        AZ::u32 /*D3D10_RESOURCE_DIMENSION*/ resourceDimension;
        AZ::u32 miscFlag;
        AZ::u32 arraySize;
        AZ::u32 reserved;
    };

    // Dds header for O3DE dds format.
    // It has same size as standard dds header but uses several reserved slots for customized information
    struct DDS_HEADER_LEGACY
    {
        AZ::u32 dwSize;
        AZ::u32 dwHeaderFlags;
        AZ::u32 dwHeight;
        AZ::u32 dwWidth;
        AZ::u32 dwPitchOrLinearSize;
        AZ::u32 dwDepth;        // only if DDS_HEADER_FLAGS_VOLUME is set in dwHeaderFlags
        AZ::u32 dwMipMapCount;
        AZ::u32 dwAlphaBitDepth;
        AZ::u32 dwReserved1;    // Image flags
        float fAvgBrightness;   // Average top mip brightness. Could be f16/half
        float cMinColor[4];
        float cMaxColor[4];
        DDS_PIXELFORMAT ddspf;
        AZ::u32 dwSurfaceFlags;
        AZ::u32 dwCubemapFlags;
        AZ::u8  bNumPersistentMips;
        AZ::u8  tileMode;      //DDS_TileMode
        AZ::u8  bReserved2[6];
        AZ::u32 dwTextureStage;


        inline const bool IsValid() const { return sizeof(*this) == dwSize; }
        inline const bool IsDX10Ext() const { return ddspf.dwFourCC == FOURCC_DX10; }
        inline const uint32 GetMipCount() const { return AZStd::max(1u, (uint32)dwMipMapCount); }

        inline const size_t GetFullHeaderSize() const
        {
            if (IsDX10Ext())
            {
                return sizeof(DDS_HEADER_LEGACY) + sizeof(DDS_HEADER_DXT10);
            }

            return sizeof(DDS_HEADER_LEGACY);
        }
    };

    // description of file header
    struct DDS_FILE_DESC_LEGACY
    {
        AZ::u32 dwMagic;
        DDS_HEADER_LEGACY header;

        inline const bool IsValid() const { return dwMagic == FOURCC_DDS && header.IsValid(); }
        inline const size_t GetFullHeaderSize() const { return sizeof(dwMagic) + header.GetFullHeaderSize(); }
    };

    // Standard dds header
    struct DDS_HEADER
    {
        uint32_t dwSize;
        uint32_t dwFlags;
        uint32_t dwHeight;
        uint32_t dwWidth;
        uint32_t dwPitchOrLinearSize;
        uint32_t dwDepth;
        uint32_t dwMipMapCount;
        uint32_t dwReserved1[11];
        DDS_PIXELFORMAT ddspf;
        uint32_t dwCaps;
        uint32_t dwCaps_2;
        uint32_t dwCaps_3;
        uint32_t dwCaps_4;
        uint32_t dwReserved2;

        inline const bool IsValid() const { return sizeof(*this) == dwSize; }
        inline const bool IsDX10Ext() const { return ddspf.dwFourCC == FOURCC_DX10; }
        inline const uint32 GetMipCount() const { return AZStd::max(1u, (uint32_t)dwMipMapCount); }

        inline const size_t GetFullHeaderSize() const
        {
            if (IsDX10Ext())
            {
                return sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10);
            }

            return sizeof(DDS_HEADER);
        }
    };

    // Standard description of file header
    struct DDS_FILE_DESC
    {
        AZ::u32 dwMagic;
        DDS_HEADER header;

        inline const bool IsValid() const { return dwMagic == FOURCC_DDS && header.IsValid(); }
        inline const size_t GetFullHeaderSize() const { return sizeof(dwMagic) + header.GetFullHeaderSize(); }
    };

    // chunk identifier
    const static AZ::u32 FOURCC_CExt = IMAGE_BUIDER_MAKEFOURCC('C', 'E', 'x', 't');    // O3DE extension start
    const static AZ::u32 FOURCC_CEnd = IMAGE_BUIDER_MAKEFOURCC('C', 'E', 'n', 'd');    // O3DE extension end
    const static AZ::u32 FOURCC_AttC = IMAGE_BUIDER_MAKEFOURCC('A', 't', 't', 'C');    // Chunk Attached Channel

    //Fourcc for pixel formats which aren't supported by dx10, such as astc formats
    //They are used for dwFourCC of dds header's DDS_PIXELFORMAT to identify non-dx10 pixel formats
    const static AZ::u32 FOURCC_ASTC_4x4 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', '4', '4');
    const static AZ::u32 FOURCC_ASTC_5x4 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', '5', '4');
    const static AZ::u32 FOURCC_ASTC_5x5 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', '5', '5');
    const static AZ::u32 FOURCC_ASTC_6x5 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', '6', '5');
    const static AZ::u32 FOURCC_ASTC_6x6 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', '6', '6');
    const static AZ::u32 FOURCC_ASTC_8x5 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', '8', '5');
    const static AZ::u32 FOURCC_ASTC_8x6 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', '8', '6');
    const static AZ::u32 FOURCC_ASTC_10x5 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', 'A', '5');
    const static AZ::u32 FOURCC_ASTC_10x6 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', 'A', '6');
    const static AZ::u32 FOURCC_ASTC_8x8 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', '8', '8');
    const static AZ::u32 FOURCC_ASTC_10x8 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', 'A', '8');
    const static AZ::u32 FOURCC_ASTC_10x10 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', 'A', 'A');
    const static AZ::u32 FOURCC_ASTC_12x10 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', 'C', 'A');
    const static AZ::u32 FOURCC_ASTC_12x12 = IMAGE_BUIDER_MAKEFOURCC('A', 'S', 'C', 'C');

    //legacy formats names. they are only used for load old dds formats. 
    const static AZ::u32 FOURCC_DXT1 = IMAGE_BUIDER_MAKEFOURCC('D', 'X', 'T', '1');
    const static AZ::u32 FOURCC_DXT2 = IMAGE_BUIDER_MAKEFOURCC('D', 'X', 'T', '2');
    const static AZ::u32 FOURCC_DXT3 = IMAGE_BUIDER_MAKEFOURCC('D', 'X', 'T', '3');
    const static AZ::u32 FOURCC_DXT4 = IMAGE_BUIDER_MAKEFOURCC('D', 'X', 'T', '4');
    const static AZ::u32 FOURCC_DXT5 = IMAGE_BUIDER_MAKEFOURCC('D', 'X', 'T', '5');
    const static AZ::u32 FOURCC_3DCP = IMAGE_BUIDER_MAKEFOURCC('A', 'T', 'I', '1');
    const static AZ::u32 FOURCC_3DC = IMAGE_BUIDER_MAKEFOURCC('A', 'T', 'I', '2');
}
