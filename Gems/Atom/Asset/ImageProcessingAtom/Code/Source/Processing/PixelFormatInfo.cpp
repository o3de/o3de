/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Processing/PixelFormatInfo.h>
#include <Processing/DDSHeader.h>

#include <AzCore/Math/MathUtils.h>

namespace ImageProcessingAtom
{
    CPixelFormats* CPixelFormats::s_instance = nullptr;

    CPixelFormats& CPixelFormats::GetInstance()
    {
        if (s_instance == nullptr)
        {
            s_instance = new CPixelFormats();
        }

        return *s_instance;
    }

    void CPixelFormats::DestroyInstance()
    {
        delete s_instance;
        s_instance = nullptr;
    }

    bool IsASTCFormat(EPixelFormat fmt)
    {
        if (fmt == ePixelFormat_ASTC_4x4
            || fmt == ePixelFormat_ASTC_5x4
            || fmt == ePixelFormat_ASTC_5x5
            || fmt == ePixelFormat_ASTC_6x5
            || fmt == ePixelFormat_ASTC_6x6
            || fmt == ePixelFormat_ASTC_8x5
            || fmt == ePixelFormat_ASTC_8x6
            || fmt == ePixelFormat_ASTC_8x8
            || fmt == ePixelFormat_ASTC_10x5
            || fmt == ePixelFormat_ASTC_10x6
            || fmt == ePixelFormat_ASTC_10x8
            || fmt == ePixelFormat_ASTC_10x10
            || fmt == ePixelFormat_ASTC_12x10
            || fmt == ePixelFormat_ASTC_12x12)
        {
            return true;
        }
        return false;
    }

    bool IsHDRFormat(EPixelFormat fmt)
    {
        switch (fmt)
        {
        case ePixelFormat_BC6UH:
        case ePixelFormat_R9G9B9E5:
        case ePixelFormat_R32G32B32A32F:
        case ePixelFormat_R32G32F:
        case ePixelFormat_R32F:
        case ePixelFormat_R16G16B16A16F:
        case ePixelFormat_R16G16F:
        case ePixelFormat_R16F:
            return true;
        default:
            return false;
        }
    }

    PixelFormatInfo::PixelFormatInfo(
        uint32_t    a_bitsPerPixel,
        uint32_t    a_Channels,
        bool        a_Alpha,
        const char* a_szAlpha,
        uint32      a_minWidth,
        uint32      a_minHeight,
        uint32_t    a_blockWidth,
        uint32_t    a_blockHeight,
        uint32_t    a_bitsPerBlock,
        bool        a_bSquarePow2,
        DXGI_FORMAT a_d3d10Format,
        uint32_t    a_fourCC,
        ESampleType a_eSampleType,
        const char* a_szName,
        const char* a_szDescription,
        bool        a_bCompressed,
        bool        a_bSelectable)
        : nChannels(a_Channels)
        , bHasAlpha(a_Alpha)
        , minWidth(a_minWidth)
        , minHeight(a_minHeight)
        , blockWidth(a_blockWidth)
        , blockHeight(a_blockHeight)
        , bitsPerBlock(a_bitsPerBlock)
        , bSquarePow2(a_bSquarePow2)
        , szAlpha(a_szAlpha)
        , d3d10Format(a_d3d10Format)
        , fourCC(a_fourCC)
        , eSampleType(a_eSampleType)
        , szName(a_szName)
        , szDescription(a_szDescription)
        , bCompressed(a_bCompressed)
        , bSelectable(a_bSelectable)
    {
        //validate pixel format
        //a_bitsPerPixel could be 0 if it's ACTC format since the actual bits per-pixel could be 6.4, 5.12 etc.
        if (a_bitsPerPixel)
        {
            AZ_Assert(a_bitsPerPixel * blockWidth * blockHeight == bitsPerBlock, "PixelFormatInfo: Wrong block setting");
        }

        AZ_Assert(szName, "szName can't be nullptr");
        AZ_Assert(nChannels > 0 && nChannels <= 4, "unreasonable channel count %d", nChannels);
        AZ_Assert(a_szDescription, "szDescription can't be nullptr");
        AZ_Assert(blockWidth > 0 && blockHeight > 0, "blcok size need to be larger than 0: %d x %d", blockWidth, blockHeight);
        AZ_Assert(minWidth > 0 && minHeight > 0, "piexel required mininum image size need to be larger than 0: %d x %d", minWidth, minHeight);
        if (!bCompressed)
        {
            AZ_Assert(blockWidth == 1 && blockHeight == 1, "Uncompressed format shouldn't have block which size > 1");
        }
    }

    CPixelFormats::CPixelFormats()
    {
        InitPixelFormats();
    }

    void CPixelFormats::InitPixelFormat(EPixelFormat format, const PixelFormatInfo& formatInfo)
    {
        AZ_Assert((format >= 0) && (format < ePixelFormat_Count), "Unsupport pixel format: %d", format);

        if (m_pixelFormatInfo[format].szName && m_pixelFormatNameMap.find(formatInfo.szName) != m_pixelFormatNameMap.end())
        {
            // double initialization
            AZ_Assert(false, "Pixel format already exist: %s", m_pixelFormatInfo[format].szName);
        }
        m_pixelFormatNameMap[formatInfo.szName] = format;
        m_pixelFormatInfo[format] = formatInfo;
    }

    void CPixelFormats::InitPixelFormats()
    {
        // Unsigned Formats
        // Data in an unsigned format must be positive. Unsigned formats use combinations of
        // (R)ed, (G)reen, (B)lue, (A)lpha, (L)uminance
        InitPixelFormat(ePixelFormat_R8G8B8A8,      PixelFormatInfo(32, 4,  true,  "8", 1, 1, 1, 1, 32, false, DXGI_FORMAT_R8G8B8A8_UNORM, FOURCC_DX10, ESampleType::eSampleType_Uint8, "R8G8B8A8", "32-bit RGBA pixel format with alpha, using 8 bits per channel", false, true));
        InitPixelFormat(ePixelFormat_R8G8B8X8,      PixelFormatInfo(32, 4, false,  "0", 1, 1, 1, 1, 32, false, DXGI_FORMAT_R8G8B8A8_UNORM, FOURCC_DX10, ESampleType::eSampleType_Uint8, "R8G8B8X8", "32-bit RGB pixel format, where 8 bits are reserved for each color", false, true));
        InitPixelFormat(ePixelFormat_R8G8,          PixelFormatInfo(16, 2, false,  "0", 1, 1, 1, 1, 16, false, DXGI_FORMAT_R8G8_UNORM, FOURCC_DX10, ESampleType::eSampleType_Uint8, "R8G8", "16-bit red/green, using 8 bits per channel", false, false));
        InitPixelFormat(ePixelFormat_R8,            PixelFormatInfo(8, 1, false,  "0", 1, 1, 1, 1,  8, false, DXGI_FORMAT_R8_UNORM, FOURCC_DX10, ESampleType::eSampleType_Uint8, "R8", "8-bit red only", false, false));
        InitPixelFormat(ePixelFormat_A8,            PixelFormatInfo(8, 1,  true,  "8", 1, 1, 1, 1,  8, false, DXGI_FORMAT_A8_UNORM, FOURCC_DX10, ESampleType::eSampleType_Uint8, "A8", "8-bit alpha only", false, true));
        InitPixelFormat(ePixelFormat_R16G16B16A16,  PixelFormatInfo(64, 4,  true, "16", 1, 1, 1, 1, 64, false, DXGI_FORMAT_R16G16B16A16_UNORM, FOURCC_DX10, ESampleType::eSampleType_Uint16, "R16G16B16A16", "64-bit ARGB pixel format with alpha, using 16 bits per channel", false, false));
        InitPixelFormat(ePixelFormat_R16G16,        PixelFormatInfo(32, 2, false,  "0", 1, 1, 1, 1, 32, false, DXGI_FORMAT_R16G16_UNORM, FOURCC_DX10, ESampleType::eSampleType_Uint16, "R16G16", "32-bit red/green, using 16 bits per channel", false, false));
        InitPixelFormat(ePixelFormat_R16,           PixelFormatInfo(16, 1, false,  "0", 1, 1, 1, 1, 16, false, DXGI_FORMAT_R16_UNORM, FOURCC_DX10, ESampleType::eSampleType_Uint16, "R16", "16-bit red only", false, false));

        // Custom FourCC Formats
        // Data in these FourCC formats is custom compressed data and only decodable by certain hardware.
        InitPixelFormat(ePixelFormat_ASTC_4x4, PixelFormatInfo(0, 4, true, "?", 16, 16, 4, 4, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_4x4, ESampleType::eSampleType_Compressed, "ASTC_4x4", "ASTC 4x4 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_5x4, PixelFormatInfo(0, 4, true, "?", 16, 16, 5, 4, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_5x4, ESampleType::eSampleType_Compressed, "ASTC_5x4", "ASTC 5x4 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_5x5, PixelFormatInfo(0, 4, true, "?", 16, 16, 5, 5, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_5x5, ESampleType::eSampleType_Compressed, "ASTC_5x5", "ASTC 5x5 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_6x5, PixelFormatInfo(0, 4, true, "?", 16, 16, 6, 5, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_6x5, ESampleType::eSampleType_Compressed, "ASTC_6x5", "ASTC 6x5 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_6x6, PixelFormatInfo(0, 4, true, "?", 16, 16, 6, 6, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_6x6, ESampleType::eSampleType_Compressed, "ASTC_6x6", "ASTC 6x6 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_8x5, PixelFormatInfo(0, 4, true, "?", 16, 16, 8, 5, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_8x5, ESampleType::eSampleType_Compressed, "ASTC_8x5", "ASTC 8x5 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_8x6, PixelFormatInfo(0, 4, true, "?", 16, 16, 8, 6, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_8x6, ESampleType::eSampleType_Compressed, "ASTC_8x6", "ASTC 8x6 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_8x8, PixelFormatInfo(0, 4, true, "?", 16, 16, 8, 8, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_8x8, ESampleType::eSampleType_Compressed, "ASTC_8x8", "ASTC 8x8 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_10x5, PixelFormatInfo(0, 4, true, "?", 16, 16, 10, 5, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_10x5, ESampleType::eSampleType_Compressed, "ASTC_10x5", "ASTC 10x5 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_10x6, PixelFormatInfo(0, 4, true, "?", 16, 16, 10, 6, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_10x6, ESampleType::eSampleType_Compressed, "ASTC_10x6", "ASTC 10x6 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_10x8, PixelFormatInfo(0, 4, true, "?", 16, 16, 10, 8, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_10x8, ESampleType::eSampleType_Compressed, "ASTC_10x8", "ASTC 10x8 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_10x10, PixelFormatInfo(0, 4, true, "?", 16, 16, 10, 10, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_10x10, ESampleType::eSampleType_Compressed, "ASTC_10x10", "ASTC 10x10 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_12x10, PixelFormatInfo(0, 4, true, "?", 16, 16, 12, 10, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_12x10, ESampleType::eSampleType_Compressed, "ASTC_12x10", "ASTC 12x10 compressed texture format", true, false));
        InitPixelFormat(ePixelFormat_ASTC_12x12, PixelFormatInfo(0, 4, true, "?", 16, 16, 12, 12, 128, false, DXGI_FORMAT_UNKNOWN, FOURCC_ASTC_12x12, ESampleType::eSampleType_Compressed, "ASTC_12x12", "ASTC 12x12 compressed texture format", true, false));

        // Standardized Compressed DXGI Formats (DX10+)
        // Data in these compressed formats is hardware decodable on all DX10 chips, and manageable with the DX10-API.
        InitPixelFormat(ePixelFormat_BC1, PixelFormatInfo(4, 3, false, "0", 4, 4, 4, 4, 64, false, DXGI_FORMAT_BC1_UNORM, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC1", "BC1 compressed texture format", true, true));
        InitPixelFormat(ePixelFormat_BC1a, PixelFormatInfo(4, 4, true, "1", 4, 4, 4, 4, 64, false, DXGI_FORMAT_BC1_UNORM, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC1a", "BC1a compressed texture format with transparency", true, true));
        InitPixelFormat(ePixelFormat_BC3, PixelFormatInfo(8, 4, true, "3of8", 4, 4, 4, 4, 128, false, DXGI_FORMAT_BC3_UNORM, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC3", "BC3 compressed texture format", true, true));
        InitPixelFormat(ePixelFormat_BC3t, PixelFormatInfo(8, 4, true, "3of8", 4, 4, 4, 4, 128, false, DXGI_FORMAT_BC3_UNORM, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC3t", "BC3t compressed texture format with transparency", true, true));
        InitPixelFormat(ePixelFormat_BC4, PixelFormatInfo(4, 1, false, "0", 4, 4, 4, 4, 64, false, DXGI_FORMAT_BC4_UNORM, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC4", "BC4 compressed texture format for single channel maps. 3DCp", true, true));
        InitPixelFormat(ePixelFormat_BC4s, PixelFormatInfo(4, 1, false, "0", 4, 4, 4, 4, 64, false, DXGI_FORMAT_BC4_SNORM, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC4s", "BC4 compressed texture format for signed single channel maps", true, true));
        InitPixelFormat(ePixelFormat_BC5, PixelFormatInfo(8, 2, false, "0", 4, 4, 4, 4, 128, false, DXGI_FORMAT_BC5_UNORM, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC5", "BC5 compressed texture format for two channel maps or normalmaps. 3DC", true, true));
        InitPixelFormat(ePixelFormat_BC5s, PixelFormatInfo(8, 2, false, "0", 4, 4, 4, 4, 128, false, DXGI_FORMAT_BC5_SNORM, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC5s", "BC5 compressed texture format for signed two channel maps or normalmaps", true, true));
        InitPixelFormat(ePixelFormat_BC6UH, PixelFormatInfo(8, 3, false, "0", 4, 4, 4, 4, 128, false, DXGI_FORMAT_BC6H_UF16, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC6UH", "BC6 compressed texture format, unsigned half", true, true));
        InitPixelFormat(ePixelFormat_BC7, PixelFormatInfo(8, 4, true, "8", 4, 4, 4, 4, 128, false, DXGI_FORMAT_BC7_UNORM, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC7", "BC7 compressed texture format", true, true));
        InitPixelFormat(ePixelFormat_BC7t, PixelFormatInfo(8, 4, true, "8", 4, 4, 4, 4, 128, false, DXGI_FORMAT_BC7_UNORM, FOURCC_DX10, ESampleType::eSampleType_Compressed, "BC7t", "BC7t compressed texture format with transparency", true, true));

        // Float formats
        // Data in a Float format is floating point data.
        InitPixelFormat(ePixelFormat_R9G9B9E5, PixelFormatInfo(32, 3, false, "0", 1, 1, 1, 1, 32, false, DXGI_FORMAT_R9G9B9E5_SHAREDEXP, FOURCC_DX10, ESampleType::eSampleType_Compressed, "R9G9B9E5", "32-bit RGB pixel format with shared exponent", false, true));
        InitPixelFormat(ePixelFormat_R32G32B32A32F, PixelFormatInfo(128, 4, true, "23", 1, 1, 1, 1, 128, false, DXGI_FORMAT_R32G32B32A32_FLOAT, FOURCC_DX10, ESampleType::eSampleType_Float, "R32G32B32A32F", "four float channels", false, false));
        InitPixelFormat(ePixelFormat_R32G32F, PixelFormatInfo(64, 2, false, "0", 1, 1, 1, 1, 64, false, DXGI_FORMAT_R32G32_FLOAT, FOURCC_DX10, ESampleType::eSampleType_Float, "R32G32F", "two float channels", false, false)); // FIXME: This should be eTF_R32G32F, but that enum is not in ITexture.h yet
        InitPixelFormat(ePixelFormat_R32F, PixelFormatInfo(32, 1, false, "0", 1, 1, 1, 1, 32, false, DXGI_FORMAT_R32_FLOAT, FOURCC_DX10, ESampleType::eSampleType_Float, "R32F", "one float channel", false, false));
        InitPixelFormat(ePixelFormat_R16G16B16A16F, PixelFormatInfo(64, 4, true, "10", 1, 1, 1, 1, 64, false, DXGI_FORMAT_R16G16B16A16_FLOAT, FOURCC_DX10, ESampleType::eSampleType_Half, "R16G16B16A16F", "four half channels", false, false));
        InitPixelFormat(ePixelFormat_R16G16F, PixelFormatInfo(32, 2, false, "0", 1, 1, 1, 1, 32, false, DXGI_FORMAT_R16G16_FLOAT, FOURCC_DX10, ESampleType::eSampleType_Half, "R16G16F", "two half channel", false, false));
        InitPixelFormat(ePixelFormat_R16F, PixelFormatInfo(16, 1, false, "0", 1, 1, 1, 1, 16, false, DXGI_FORMAT_R16_FLOAT, FOURCC_DX10, ESampleType::eSampleType_Half, "R16F", "one half channel", false, false));

        //legacy BGRA8
        InitPixelFormat(ePixelFormat_B8G8R8A8, PixelFormatInfo(32, 4, true, "8", 1, 1, 1, 1, 32, false, DXGI_FORMAT_B8G8R8A8_UNORM, FOURCC_DX10, ESampleType::eSampleType_Uint8, "B8G8R8A8", "32-bit BGRA pixel format with alpha, using 8 bits per channel", false, false));

        InitPixelFormat(ePixelFormat_B8G8R8, PixelFormatInfo(24, 3, true, "0", 1, 1, 1, 1, 24, false, DXGI_FORMAT_UNKNOWN, FOURCC_DX10, ESampleType::eSampleType_Uint8, "B8G8R8", "24-bit BGR pixel format, using 8 bits per channel", false, false));
        InitPixelFormat(ePixelFormat_R8G8B8, PixelFormatInfo(24, 3, true, "0", 1, 1, 1, 1, 24, false, DXGI_FORMAT_UNKNOWN, FOURCC_DX10, ESampleType::eSampleType_Uint8, "R8G8B8", "24-bit RGB pixel format, using 8 bits per channel", false, false));

        InitPixelFormat(ePixelFormat_R32, PixelFormatInfo(32, 1, false, "0", 1, 1, 1, 1, 32, false, DXGI_FORMAT_FORCE_UINT, FOURCC_DX10, ESampleType::eSampleType_Uint32, "R32", "32-bit red only", false, false));

        //validate all pixel formats are proper initialized
        for (int i = 0; i < ePixelFormat_Count; ++i)
        {
            if (m_pixelFormatInfo[i].szName == 0)
            {
                // Uninitialized entry. Should never happen. But, if it happened: make sure that entries from
                // the EPixelFormat enum and InitPixelFormat() calls match.
                AZ_Assert(false, "InitPixelFormats error: not all pixel formats have an implementation.");
            }
        }
    }


    EPixelFormat CPixelFormats::FindPixelFormatByName(const char* name)
    {
        if (m_pixelFormatNameMap.find(name) != m_pixelFormatNameMap.end())
        {
            return m_pixelFormatNameMap[name];
        }
        return ePixelFormat_Unknown;
    }

    const PixelFormatInfo* CPixelFormats::GetPixelFormatInfo(EPixelFormat format)
    {
        AZ_Assert((format >= 0) && (format < ePixelFormat_Count), "Unsupport pixel format: %d", format);
        return &m_pixelFormatInfo[format];
    }

    bool CPixelFormats::IsPixelFormatUncompressed(EPixelFormat format)
    {
        AZ_Assert((format >= 0) && (format < ePixelFormat_Count), "Unsupport pixel format: %d", format);
        return !m_pixelFormatInfo[format].bCompressed;
    }

    bool CPixelFormats::IsPixelFormatWithoutAlpha(EPixelFormat format)
    {
        AZ_Assert((format >= 0) && (format < ePixelFormat_Count), "Unsupport pixel format: %d", format);
        return !m_pixelFormatInfo[format].bHasAlpha;
    }

    uint32 CPixelFormats::ComputeMaxMipCount(EPixelFormat format, uint32 width, uint32 height, uint32_t depth)
    {
        const PixelFormatInfo* const pFormatInfo = GetPixelFormatInfo(format);

        AZ_Assert(pFormatInfo != nullptr, "ComputeMaxMipCount: unsupport pixel format %d", format);

        uint32 tmpWidth = width;
        uint32 tmpHeight = height;
        uint32 tmpDepth = depth;

        bool bIgnoreBlockSize = CanImageSizeIgnoreBlockSize(format);

        uint32 mipCountW = 0;
        while ((tmpWidth >= pFormatInfo->minWidth) && (bIgnoreBlockSize || (tmpWidth % pFormatInfo->blockWidth == 0)))
        {
            ++mipCountW;
            tmpWidth >>= 1;
        }

        uint32 mipCountH = 0;
        while ((tmpHeight >= pFormatInfo->minHeight) && (bIgnoreBlockSize || (tmpHeight % pFormatInfo->blockHeight == 0)))
        {
            ++mipCountH;
            tmpHeight >>= 1;
        }

        uint32 mipCountD = 0;
        while ((tmpDepth >= 1))
        {
            ++mipCountD;
            tmpDepth >>= 1;
        }

        //for compressed image, use minmum mip out of W and H because any size below won't be compressed properly
        //for non-compressed image. use maximum mip count. for example the lowest two mips of 128x64 would be 2x1 and 1x1
        const uint32 mipCount = (pFormatInfo->bCompressed)
            ? AZStd::min<uint32>(mipCountW, mipCountH)
            : AZStd::max(AZStd::max<uint32>(mipCountW, mipCountH), mipCountD);

        // In some cases, user may call this function for image size which is qualified for this pixel format,
        // the mipCount could be 0 for those cases. Round it to 1 if it happend.
        return AZStd::max<uint32>((uint32)1, mipCount);
    }

    bool CPixelFormats::CanImageSizeIgnoreBlockSize(EPixelFormat format)
    {
        // ASTC is a kind of block compression but it doesn't need the image size to be interger mutiples of block size.
        // reference: https://www.khronos.org/registry/OpenGL/extensions/KHR/KHR_texture_compression_astc_hdr.txt
        //"For images which are not an integer multiple of the block size, additional texels are added to the edges
        // with maximum X and Y.These texels may be any color, as they will not be accessed."
        bool bIgnoreBlockSize = IsASTCFormat(format);

        return bIgnoreBlockSize;
    }

    bool CPixelFormats::IsImageSizeValid(EPixelFormat format, uint32 imageWidth, uint32 imageHeight, [[maybe_unused]] bool logWarning)
    {
        const PixelFormatInfo* const pFormatInfo = GetPixelFormatInfo(format);
        AZ_Assert(pFormatInfo != nullptr, "IsImageSizeValid: unsupport pixel format %d", format);

        //if the format requires image to be sqaure and power of 2
        if (pFormatInfo->bSquarePow2 && ((imageWidth != imageHeight) || (imageWidth & (imageWidth - 1)) != 0))
        {
            AZ_Warning("ImageBuilder", !logWarning, "Image size need to be square and power of 2 for pixel format %s",
                pFormatInfo->szName);
            return false;
        }

        // minimum size required by the pixel format
        if (imageWidth < pFormatInfo->minWidth || imageHeight < pFormatInfo->minHeight)
        {
            AZ_Warning("ImageBuilder", !logWarning, "The image size (%dx%d) is smaller than minimum size (%dx%d) for pixel format %s",
                imageWidth, imageHeight, pFormatInfo->minWidth, pFormatInfo->minHeight, pFormatInfo->szName);
            return false;
        }

        //check image size againest block size
        if (!CanImageSizeIgnoreBlockSize(format))
        {
            if (imageWidth % pFormatInfo->blockWidth != 0 || imageHeight % pFormatInfo->blockHeight != 0)
            {
                AZ_Warning("ImageBuilder", !logWarning, "Image size (%dx%d) need to be integer multiplier of compression block size (%dx%d) for pixel format %s",
                    imageWidth, imageHeight, pFormatInfo->minWidth, pFormatInfo->minHeight, pFormatInfo->szName);
                return false;
            }
        }

        return true;
    }

    AZ::u32 NextPowOf2(AZ::u32 value)
    {
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value++;
        return value;
    }

    void CPixelFormats::GetSuitableImageSize(EPixelFormat format, AZ::u32 imageWidth, AZ::u32 imageHeight,
        AZ::u32& outWidth, AZ::u32& outHeight)
    {
        const PixelFormatInfo* const pFormatInfo = GetPixelFormatInfo(format);
        AZ_Assert(pFormatInfo != nullptr, "IsImageSizeValid: unsupport pixel format %d", format);

        outWidth = imageWidth;
        outHeight = imageHeight;

        // minimum size required by the pixel format
        if (outWidth < pFormatInfo->minWidth)
        {
            outWidth = pFormatInfo->minWidth;
        }
        if (outHeight < pFormatInfo->minHeight)
        {
            outHeight = pFormatInfo->minHeight;
        }

        if (pFormatInfo->bSquarePow2 && ((outWidth != outHeight) || (outWidth & (outWidth - 1)) != 0))
        {
            AZ::u32 sideSide = AZ::GetMax(outWidth, outHeight);
            outWidth = NextPowOf2(sideSide);
            outHeight = outWidth;
        }

        //check image size againest block size
        //if the format requires square and power of 2. we can skip this step
        if (!CanImageSizeIgnoreBlockSize(format) && !pFormatInfo->bSquarePow2)
        {
            if (outWidth % pFormatInfo->blockWidth != 0)
            {
                outWidth = AZ::RoundUpToMultiple(outWidth, pFormatInfo->blockWidth);
            }
            if (outHeight % pFormatInfo->blockHeight != 0)
            {
                outHeight = AZ::RoundUpToMultiple(outHeight, pFormatInfo->blockHeight);
            }
        }
    }

    uint32 CPixelFormats::EvaluateImageDataSize(EPixelFormat format, uint32 imageWidth, uint32 imageHeight)
    {
        const PixelFormatInfo* const pFormatInfo = GetPixelFormatInfo(format);
        AZ_Assert(pFormatInfo != nullptr, "IsImageSizeValid: unsupport pixel format %d", format);

        //the image should pass IsImageSizeValid test to be eavluated correctly
        if (!IsImageSizeValid(format, imageWidth, imageHeight, false))
        {
            return 0;
        }

        // get number of blocks and multiply with bits per block. Divided by 8 to get
        // final byte size
        return (AZ::DivideAndRoundUp(imageWidth, pFormatInfo->blockWidth) *
                AZ::DivideAndRoundUp(imageHeight, pFormatInfo->blockHeight) *
                pFormatInfo->bitsPerBlock) / 8;
    }

    bool CPixelFormats::IsFormatSingleChannel(EPixelFormat fmt)
    {
        return (m_pixelFormatInfo[fmt].nChannels == 1);
    }

    bool CPixelFormats::IsFormatSigned(EPixelFormat fmt)
    {
        // all these formats contain signed data, the FP-formats contain scale & biased unsigned data
        return (fmt == ePixelFormat_BC4s || fmt == ePixelFormat_BC5s /*|| fmt == ePixelFormat_BC6SH*/);
    }

    bool CPixelFormats::IsFormatFloatingPoint(EPixelFormat fmt, bool bFullPrecision)
    {
        // all these formats contain floating point data
        if (!bFullPrecision)
        {
            return ((fmt == ePixelFormat_R16F || fmt == ePixelFormat_R16G16F ||
                     fmt == ePixelFormat_R16G16B16A16F) || (fmt == ePixelFormat_BC6UH || fmt == ePixelFormat_R9G9B9E5));
        }
        else
        {
            return ((fmt == ePixelFormat_R32F || fmt == ePixelFormat_R32G32F || fmt == ePixelFormat_R32G32B32A32F));
        }
    }
} // namespace ImageProcessingAtom
