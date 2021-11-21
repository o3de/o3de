/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Processing/ImageObjectImpl.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/ImageFlags.h>
#include <Converters/PixelOperation.h>
#include <AzCore/std/string/conversions.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/GenericStreams.h>

#include <AzFramework/StringFunc/StringFunc.h>

// Indicates a 2D texture is a cube-map texture.
#define DDS_RESOURCE_MISC_TEXTURECUBE   0x4

namespace ImageProcessingAtom
{
    using namespace AZ;

    IImageObject* IImageObject::CreateImage(AZ::u32 width, AZ::u32 height,
        AZ::u32 maxMipCount, EPixelFormat pixelFormat)
    {
        return aznew CImageObject(width, height, maxMipCount, pixelFormat);
    }

    CImageObject::CImageObject(AZ::u32 width, AZ::u32 height, AZ::u32 maxMipCount, EPixelFormat pixelFormat)
        : m_pixelFormat(pixelFormat)
        , m_colMinARGB(0.0f, 0.0f, 0.0f, 0.0f)
        , m_colMaxARGB(1.0f, 1.0f, 1.0f, 1.0f)
        , m_averageBrightness(0.63f)
        , m_imageFlags(0)
        , m_numPersistentMips(0)
    {
        ResetImage(width, height, maxMipCount, pixelFormat);
    }

    EPixelFormat CImageObject::GetPixelFormat() const
    {
        return m_pixelFormat;
    }

    AZ::u32 CImageObject::GetPixelCount(AZ::u32 mip) const
    {
        AZ_Assert(mip < (AZ::u32)m_mips.size() && m_mips[mip], "Mip doesn't exist: %d", mip);

        return m_mips[mip]->m_width * m_mips[mip]->m_height;
    }

    AZ::u32 CImageObject::GetWidth(AZ::u32 mip) const
    {
        AZ_Assert(mip < (AZ::u32)m_mips.size() && m_mips[mip], "Mip doesn't exist: %d", mip);

        return m_mips[mip]->m_width;
    }

    AZ::u32 CImageObject::GetHeight(AZ::u32 mip) const
    {
        AZ_Assert(mip < (AZ::u32)m_mips.size() && m_mips[mip], "Mip doesn't exist: %d", mip);

        return m_mips[mip]->m_height;
    }

    AZ::u32 CImageObject::GetMipCount() const
    {
        return (AZ::u32)m_mips.size();
    }

    void CImageObject::ResetImage(AZ::u32 width, AZ::u32 height, AZ::u32 maxMipCount, EPixelFormat pixelFormat)
    {
        //check input
        AZ_Assert(width > 0 && height > 0, "image width and height need to larger than 0. width: %d, height: %d", width, height);

        //clean up mipmaps
        for (AZ::u32 mip = 0; mip < AZ::u32(m_mips.size()); ++mip)
        {
            delete m_mips[mip];
        }

        m_pixelFormat = pixelFormat;
        m_colMinARGB = AZ::Color(0.0f, 0.0f, 0.0f, 0.0f);
        m_colMaxARGB = AZ::Color(1.0f, 1.0f, 1.0f, 1.0f);
        m_averageBrightness = 0.0f;
        m_imageFlags = 0;
        m_numPersistentMips = 0;
        m_mips.clear();

        const PixelFormatInfo* const pFmt = CPixelFormats::GetInstance().GetPixelFormatInfo(m_pixelFormat);
        AZ_Assert(pFmt, "can't find pixe format info for %d", m_pixelFormat);

        const AZ::u32 mipCount = AZStd::min<AZ::u32>(maxMipCount,
            CPixelFormats::GetInstance().ComputeMaxMipCount(m_pixelFormat, width, height));

        m_mips.reserve(mipCount);

        for (AZ::u32 mip = 0; mip < mipCount; ++mip)
        {
            MipLevel* const pEntry = aznew MipLevel;

            AZ::u32 localWidth = width >> mip;
            AZ::u32 localHeight = height >> mip;
            if (localWidth < 1)
            {
                localWidth = 1;
            }
            if (localHeight < 1)
            {
                localHeight = 1;
            }

            pEntry->m_width = localWidth;
            pEntry->m_height = localHeight;

            if (pFmt->bCompressed)
            {
                const AZ::u32 blocksInRow = (pEntry->m_width + (pFmt->blockWidth - 1)) / pFmt->blockWidth;
                pEntry->m_pitch = (blocksInRow * pFmt->bitsPerBlock) / 8;
                pEntry->m_rowCount = (localHeight + (pFmt->blockHeight - 1)) / pFmt->blockHeight;
            }
            else
            {
                pEntry->m_pitch = (pEntry->m_width * pFmt->bitsPerBlock) / 8;
                pEntry->m_rowCount = localHeight;
            }

            pEntry->Alloc();

            m_mips.push_back(pEntry);
        }
    }

    bool CImageObject::CompareImage(const IImageObjectPtr otherImage) const
    {
        CImageObject* other = static_cast<CImageObject*>(otherImage.get());
        if (other == nullptr)
        {
            return false;
        }

        if (m_pixelFormat == other->m_pixelFormat
            && m_colMinARGB == other->m_colMinARGB
            && m_colMaxARGB == other->m_colMaxARGB
            && m_averageBrightness == other->m_averageBrightness
            && m_imageFlags == other->m_imageFlags
            && m_numPersistentMips == other->m_numPersistentMips
            && m_mips.size() == other->m_mips.size())
        {
            for (int mip = 0; mip < m_mips.size(); mip++)
            {
                if (!(*m_mips[mip] == *other->m_mips[mip]))
                {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    uint32_t CImageObject::GetTextureMemory() const
    {
        int totalSize = 0;
        for (int mip = 0; mip < m_mips.size(); mip++)
        {
            totalSize += CPixelFormats::GetInstance().EvaluateImageDataSize(m_pixelFormat,
                m_mips[mip]->m_width, m_mips[mip]->m_height);
        }

        return totalSize;
    }

    EAlphaContent CImageObject::GetAlphaContent() const
    {
        if (CPixelFormats::GetInstance().IsPixelFormatWithoutAlpha(m_pixelFormat))
        {
            return EAlphaContent::eAlphaContent_Absent;
        }

        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(m_pixelFormat))
        {
            AZ_TracePrintf("Image processing", "GetAlphaContent() was called for compressed format\n");
            return EAlphaContent::eAlphaContent_Indeterminate;
        }

        //go though alpha channel of first mip
        //create pixel operation function to access pixel data
        IPixelOperationPtr pixelOp = CreatePixelOperation(m_pixelFormat);

        //get count of bytes per pixel for images
        AZ::u32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(m_pixelFormat)->bitsPerBlock / 8;

        // counts of blacks and white
        uint32_t nBlacks = 0;
        uint32_t nWhites = 0;

        float r, g, b, a;
        AZ::u8* pixelBuf;
        AZ::u32 pitch;
        GetImagePointer(0, pixelBuf, pitch);

        const AZ::u32 pixelCount = GetPixelCount(0);

        for (AZ::u32 i = 0; i < pixelCount; ++i, pixelBuf += pixelBytes)
        {
            pixelOp->GetRGBA(pixelBuf, r, g, b, a);
            if (a == 0.0f)
            {
                ++nBlacks;
            }
            else if (a == 1.0f)
            {
                ++nWhites;
            }
            else
            {
                return EAlphaContent::eAlphaContent_Greyscale;
            }
        }

        if (nBlacks == 0)
        {
            return EAlphaContent::eAlphaContent_OnlyWhite;
        }

        if (nWhites == 0)
        {
            return EAlphaContent::eAlphaContent_OnlyBlack;
        }

        return EAlphaContent::eAlphaContent_OnlyBlackAndWhite;
    }

    // clone this image-object's contents
    IImageObject* CImageObject::Clone(uint32_t maxMipCount) const
    {
        IImageObject* outImage = AllocateImage(maxMipCount);
        AZ::u32 mips = outImage->GetMipCount();

        for (AZ::u32 mip = 0; mip < mips; ++mip)
        {
            AZ::u8* dstMem;
            AZ::u32 pitch;
            outImage->GetImagePointer(mip, dstMem, pitch);

            memcpy(dstMem, m_mips[mip]->m_pData, GetMipBufSize(mip));
        }
        return outImage;
    }

    void CImageObject::ClearColor(float r, float g, float b, float a)
    {
        //if it's compressed format, return directly
        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(m_pixelFormat))
        {
            AZ_Assert(false, "The %s function only works with uncompressed formats", __FUNCTION__);
            return;
        }
        //create pixel operation function to access pixel data
        IPixelOperationPtr pixelOp = CreatePixelOperation(m_pixelFormat);

        //get count of bytes per pixel for images
        AZ::u32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(m_pixelFormat)->bitsPerBlock / 8;

        AZ::u8* pixelBuf;
        AZ::u32 pitch;

        AZ::u32 mips = GetMipCount();
        for (AZ::u32 mip = 0; mip < mips; ++mip)
        {
            GetImagePointer(mip, pixelBuf, pitch);
            const AZ::u32 pixelCount = GetPixelCount(mip);
            for (AZ::u32 i = 0; i < pixelCount; ++i, pixelBuf += pixelBytes)
            {
                pixelOp->SetRGBA(pixelBuf, r, g, b, a);
            }
        }
    }

    // allocate an empty image with the same properties as the given image and the requested format
    IImageObject* CImageObject::AllocateImage(EPixelFormat pixelFormat, uint32_t maxMipCount) const
    {
        AZ::u32 width = GetWidth(0);
        AZ::u32 height = GetHeight(0);

        if (!CPixelFormats::GetInstance().IsImageSizeValid(pixelFormat, width, height, false))
        {
            AZ_Assert(false, "Cann't allocate image with format: %d", pixelFormat);
            return nullptr;
        }

        maxMipCount = AZ::GetMin(maxMipCount, GetMipCount());
        CImageObject* pRet = aznew CImageObject(width, height, maxMipCount, pixelFormat);
        pRet->CopyPropertiesFrom(this);
        return pRet;
    }

    IImageObject* CImageObject::AllocateImage(uint32_t maxMipCount) const
    {
        return AllocateImage(m_pixelFormat, maxMipCount);
    }

    CImageObject::~CImageObject()
    {
        for (size_t i = 0; i < m_mips.size(); ++i)
        {
            delete m_mips[i];
        }
        m_mips.clear();
    }

    float CImageObject::CalculateAverageBrightness() const
    {
        //if it's compressed format, return a default value
        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(m_pixelFormat))
        {
            return 0.5f;
        }

        // Accumulate pixel colors of the top mip
        double avgOverall[3] = { 0.0, 0.0, 0.0 };

        //create pixel operation function to access pixel data
        IPixelOperationPtr pixelOp = CreatePixelOperation(m_pixelFormat);

        //get count of bytes per pixel for images
        AZ::u32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(m_pixelFormat)->bitsPerBlock / 8;

        //only calculate mip 0
        AZ::u32 mip = 0;
        float color[4];
        AZ::u8* pixelBuf;
        AZ::u32 pitch;
        GetImagePointer(mip, pixelBuf, pitch);
        const AZ::u32 pixelCount = GetPixelCount(mip);
        for (AZ::u32 i = 0; i < pixelCount; ++i, pixelBuf += pixelBytes)
        {
            pixelOp->GetRGBA(pixelBuf, color[0], color[1], color[2], color[3]);
            avgOverall[0] += color[0];
            avgOverall[1] += color[1];
            avgOverall[2] += color[2];
        }

        const double avg = (avgOverall[0] + avgOverall[1] + avgOverall[2]) / (3 * pixelCount);

        return (float)avg;
    }

    bool CImageObject::BuildSurfaceHeader(DDS_HEADER_LEGACY& header) const
    {
        AZ::u32 dwWidth, dwMips, dwHeight;
        GetExtent(dwWidth, dwHeight, dwMips);

        if (dwMips <= 0)
        {
            AZ_Error("Image Processing", false, "%s: dwMips is %u", __FUNCTION__, (unsigned)dwMips);
            return false;
        }

        const EPixelFormat format = GetPixelFormat();
        if ((format < 0) || (format >= ePixelFormat_Count))
        {
            AZ_Error("Image Processing", false, "%s: Bad format %d", __FUNCTION__, (int)format);
            return false;
        }

        const PixelFormatInfo* const pPixelFormatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(format);

        memset(&header, 0, sizeof(DDS_HEADER_LEGACY));

        header.dwSize = sizeof(DDS_HEADER_LEGACY);
        header.dwHeaderFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT;
        header.dwWidth = dwWidth;
        header.dwHeight = dwHeight;

        if (HasImageFlags(EIF_Cubemap))
        {
            header.dwSurfaceFlags |= DDS_SURFACE_FLAGS_CUBEMAP;
            header.dwCubemapFlags |= DDS_CUBEMAP_ALLFACES;
            //save face size instead of image size.
            header.dwHeight /= 6;
        }

        header.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);
        header.ddspf.dwFlags = DDS_FOURCC;

        header.ddspf.dwFourCC = pPixelFormatInfo->fourCC;

        header.dwSurfaceFlags |= DDS_SURFACE_FLAGS_TEXTURE;

        if (dwMips > 1)
        {
            header.dwHeaderFlags |= DDS_HEADER_FLAGS_MIPMAP;
            header.dwMipMapCount = dwMips;
            header.dwSurfaceFlags |= DDS_SURFACE_FLAGS_MIPMAP;
        }

        // non standardized way to expose some features in the header (same information is in attached chunk but then
        // streaming would need to find this spot in the file)
        // if this is causing problems we need to change it
        header.dwTextureStage = FOURCC_FYRC;
        header.dwReserved1 = GetImageFlags();
        header.bNumPersistentMips = (AZ::u8)GetNumPersistentMips();

        //tile mode for some platform native texture
        if (HasImageFlags(EIF_RestrictedPlatformDNative))
        {
            header.tileMode = eTM_LinearPadded;
        }
        else if (HasImageFlags(EIF_RestrictedPlatformONative))
        {
            header.tileMode = eTM_Optimal;
        }

        // setting up min and max colors
        for (int i = 0; i < 4; i++)
        {
            header.cMinColor[i] = m_colMinARGB.GetElement(i);
            header.cMaxColor[i] = m_colMaxARGB.GetElement(i);
        }

        // set avg brightness
        header.fAvgBrightness = GetAverageBrightness();

        return true;
    }

    bool CImageObject::BuildSurfaceExtendedHeader(DDS_HEADER_DXT10& exthead) const
    {
        const EPixelFormat format = GetPixelFormat();

        const PixelFormatInfo* const pPixelFormatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(format);

        DXGI_FORMAT dxgiformat = pPixelFormatInfo->d3d10Format;

        // check if we hit a format which can't be stored into a DX10 DDS-file (fe. L8)
        if (dxgiformat == DXGI_FORMAT_UNKNOWN)
        {
            AZ_Error("Image Processing", false, "%s: Format can not be stored in a DDS-file %d", __FUNCTION__, dxgiformat);
            return false;
        }

        //the dxgi format are different for linear space or gamma space
        if (HasImageFlags(EIF_SRGBRead))
        {
            switch (dxgiformat)
            {
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                dxgiformat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                break;
            case DXGI_FORMAT_BC1_UNORM:
                dxgiformat = DXGI_FORMAT_BC1_UNORM_SRGB;
                break;
            case DXGI_FORMAT_BC2_UNORM:
                dxgiformat = DXGI_FORMAT_BC2_UNORM_SRGB;
                break;
            case DXGI_FORMAT_BC3_UNORM:
                dxgiformat = DXGI_FORMAT_BC3_UNORM_SRGB;
                break;
            case DXGI_FORMAT_BC7_UNORM:
                dxgiformat = DXGI_FORMAT_BC7_UNORM_SRGB;
                break;
            default:
                break;
            }
        }
        else
        {
            switch (dxgiformat)
            {
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                dxgiformat = DXGI_FORMAT_R8G8B8A8_UNORM;
                break;
            case DXGI_FORMAT_BC1_UNORM_SRGB:
                dxgiformat = DXGI_FORMAT_BC1_UNORM;
                break;
            case DXGI_FORMAT_BC2_UNORM_SRGB:
                dxgiformat = DXGI_FORMAT_BC2_UNORM;
                break;
            case DXGI_FORMAT_BC3_UNORM_SRGB:
                dxgiformat = DXGI_FORMAT_BC3_UNORM;
                break;
            case DXGI_FORMAT_BC7_UNORM_SRGB:
                dxgiformat = DXGI_FORMAT_BC7_UNORM;
                break;
            default:
                break;
            }
        }

        memset(&exthead, 0, sizeof(exthead));

        exthead.dxgiFormat = dxgiformat;
        exthead.resourceDimension = 3; //texture2d. not used

        if (HasImageFlags(EIF_Volumetexture))
        {
            AZ_Assert(false, "There isn't any support for volume texture");
        }
        else if (HasImageFlags(EIF_Cubemap))
        {
            exthead.miscFlag = DDS_RESOURCE_MISC_TEXTURECUBE;
            exthead.arraySize = 6;
        }
        else
        {
            exthead.miscFlag = 0;
            exthead.arraySize = 1;
        }

        return true;
    }

    void CImageObject::GetExtent(AZ::u32& width, AZ::u32& height, AZ::u32& mipCount) const
    {
        mipCount = (AZ::u32)m_mips.size();

        width = m_mips[0]->m_width;
        height = m_mips[0]->m_height;
    }

    AZ::u32 CImageObject::GetMipDataSize(const AZ::u32 mip) const
    {
        AZ_Assert(mip < m_mips.size(), "mip %d doesn't exist", mip);

        return m_mips[mip]->GetSize();
    }

    void CImageObject::GetImagePointer(const AZ::u32 mip, AZ::u8*& pMem, AZ::u32& pitch) const
    {
        AZ_Assert(mip < (AZ::u32)m_mips.size() && m_mips[mip], "requested mip doesn't exist");

        pMem = m_mips[mip]->m_pData;
        pitch = m_mips[mip]->m_pitch;
    }

    AZ::u32 CImageObject::GetMipBufSize(AZ::u32 mip) const
    {
        AZ_Assert(mip < (AZ::u32)m_mips.size() && m_mips[mip], "requested mip doesn't exist");

        return m_mips[mip]->m_rowCount * m_mips[mip]->m_pitch;
    }


    void CImageObject::SetMipData(AZ::u32 mip, AZ::u8* mipBuf, AZ::u32 bufSize, AZ::u32 pitch)
    {
        if (mip >= m_mips.size())
        {
            return;
        }
        m_mips[mip]->m_pData = mipBuf;
        m_mips[mip]->m_pitch = pitch;
        m_mips[mip]->m_rowCount = bufSize / pitch;
        AZ_Assert(bufSize == m_mips[mip]->m_rowCount * pitch, "Bad pitch size");
    }

    // ARGB
    void CImageObject::GetColorRange(AZ::Color& minColor, AZ::Color& maxColor) const
    {
        minColor = m_colMinARGB;
        maxColor = m_colMaxARGB;
    }

    // ARGB
    void CImageObject::SetColorRange(const AZ::Color& minColor, const AZ::Color& maxColor)
    {
        m_colMinARGB = minColor;
        m_colMaxARGB = maxColor;
    }

    float CImageObject::GetAverageBrightness() const
    {
        return m_averageBrightness;
    }

    void CImageObject::SetAverageBrightness(const float avgBrightness)
    {
        m_averageBrightness = avgBrightness;
    }

    AZ::u32 CImageObject::GetImageFlags() const
    {
        return m_imageFlags;
    }

    void CImageObject::SetImageFlags(const AZ::u32 imageFlags)
    {
        m_imageFlags = imageFlags;
    }

    void CImageObject::AddImageFlags(const AZ::u32 imageFlags)
    {
        m_imageFlags |= imageFlags;
    }

    void CImageObject::RemoveImageFlags(const AZ::u32 imageFlags)
    {
        m_imageFlags &= ~imageFlags;
    }

    bool CImageObject::HasImageFlags(const AZ::u32 imageFlags) const
    {
        return (m_imageFlags & imageFlags) != 0;
    }

    AZ::u32 CImageObject::GetNumPersistentMips() const
    {
        return m_numPersistentMips;
    }

    void CImageObject::SetNumPersistentMips(AZ::u32 nMips)
    {
        m_numPersistentMips = nMips;
    }

    bool CImageObject::HasPowerOfTwoSizes() const
    {
        AZ::u32 w, h, mips;
        GetExtent(w, h, mips);
        return ((w & (w - 1)) == 0) && ((h & (h - 1)) == 0);
    }

    // use when you convert an image to another one
    void CImageObject::CopyPropertiesFrom(const IImageObjectPtr src)
    {
        const CImageObject* imageObj = static_cast<CImageObject*>(src.get());
        CopyPropertiesFrom(imageObj);
    }

    void CImageObject::CopyPropertiesFrom(const CImageObject* src)
    {
        m_colMinARGB = src->m_colMinARGB;
        m_colMaxARGB = src->m_colMaxARGB;
        m_averageBrightness = src->m_averageBrightness;
        m_imageFlags = src->GetImageFlags();
    }

    void CImageObject::Swizzle(const char channels[4])
    {
        if (!(CPixelFormats::GetInstance().IsPixelFormatUncompressed(m_pixelFormat)))
        {
            AZ_Assert(false, "%s function only works with uncompressed pixel format", __FUNCTION__);
            return;
        }

        const AZ::u8 channelCnt = 4;

        enum Channel_Id
        {
            ChannelR = 0,
            ChannelG,
            ChannelB,
            ChannelA,
            ChannelVal0,
            ChannelVal1,
            ChannelTypeCount
        };

        float values[ChannelTypeCount];
        values[ChannelVal0] = 0.f;
        values[ChannelVal1] = 1.f;

        AZ::u8 channelIndics[channelCnt];
        for (AZ::u8 idx = 0; idx < channelCnt; idx++)
        {
            switch (channels[idx])
            {
            case 'a':
                channelIndics[idx] = ChannelA;
                break;
            case 'r':
                channelIndics[idx] = ChannelR;
                break;
            case 'g':
                channelIndics[idx] = ChannelG;
                break;
            case 'b':
                channelIndics[idx] = ChannelB;
                break;
            case '0':
                channelIndics[idx] = ChannelVal0;
                break;
            case '1':
                channelIndics[idx] = ChannelVal1;
                break;
            default:
                AZ_Assert(false, "%s function only works with channel name \"rgba01\"", __FUNCTION__);
                return;
            }
        }

        //create pixel operation function
        IPixelOperationPtr pixelOp = CreatePixelOperation(m_pixelFormat);

        //get count of bytes per pixel
        uint32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(m_pixelFormat)->bitsPerBlock / 8;

        const uint32 mips = (uint32)m_mips.size();
        for (uint32 mip = 0; mip < mips; ++mip)
        {
            uint8* pixelBuf = m_mips[mip]->m_pData;
            const uint32 pixelCount = GetPixelCount(mip);

            for (uint32 i = 0; i < pixelCount; ++i, pixelBuf += pixelBytes)
            {
                pixelOp->GetRGBA(pixelBuf, values[ChannelR], values[ChannelG], values[ChannelB], values[ChannelA]);
                pixelOp->SetRGBA(pixelBuf, values[channelIndics[0]], values[channelIndics[1]],
                    values[channelIndics[2]], values[channelIndics[3]]);
            }
        }
    }

    void CImageObject::GlossFromNormals(bool hasAuthoredGloss)
    {
        if (!(CPixelFormats::GetInstance().IsPixelFormatUncompressed(m_pixelFormat)))
        {
            AZ_Assert(false, "%s function only works with uncompressed pixel format", __FUNCTION__);
            return;
        }

        // Derive new roughness from normal variance to preserve the bumpiness of normal map mips and to reduce specular aliasing.
        // The derived roughness is combined with the artist authored roughness stored in the alpha channel of the normal map.
        // The algorithm is based on the Frequency Domain Normal Mapping implementation presented by Neubelt and Pettineo at Siggraph 2013.

        //create pixel operation function
        IPixelOperationPtr pixelOp = CreatePixelOperation(m_pixelFormat);

        //get count of bytes per pixel
        AZ::u32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(m_pixelFormat)->bitsPerBlock / 8;

        const AZ::u32 mips = (AZ::u32)m_mips.size();
        float color[4];
        for (AZ::u32 mip = 0; mip < mips; ++mip)
        {
            AZ::u8* pixelBuf = m_mips[mip]->m_pData;
            const AZ::u32 pixelCount = GetPixelCount(mip);

            for (AZ::u32 i = 0; i < pixelCount; ++i, pixelBuf += pixelBytes)
            {
                pixelOp->GetRGBA(pixelBuf, color[0], color[1], color[2], color[3]);

                // Get length of the averaged normal
                AZ::Vector3 normal(color[0] * 2.0f - 1.0f, color[1] * 2.0f - 1.0f, color[2] * 2.0f - 1.0f);

                float len = AZ::GetMax<float>(normal.GetLength(), 1.0f / (1 << 15));

                float authoredSmoothness = hasAuthoredGloss ? color[3] : 1.0f;
                float finalSmoothness = authoredSmoothness;

                if (len < 1.0f)
                {
                    // Convert from smoothness to roughness (needs to match shader code)
                    float authoredRoughness = (1.0f - authoredSmoothness) * (1.0f - authoredSmoothness);

                    // Derive new roughness based on normal variance
                    float kappa = (3.0f * len - len * len * len) / (1.0f - len * len);
                    float variance = 1.0f / (2.0f * kappa);
                    float finalRoughness = AZ::GetMin(sqrtf(authoredRoughness * authoredRoughness + variance), 1.0f);

                    // Convert roughness back to smoothness
                    finalSmoothness = 1.0f - sqrtf(finalRoughness);
                }

                pixelOp->SetRGBA(pixelBuf, color[0], color[1], color[2], finalSmoothness);
            }
        }
    }
} // namespace ImageProcessingAtom
