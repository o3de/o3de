/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <ImageProcessing_precompiled.h>

#include <Processing/ImageObjectImpl.h>
#include <Processing/PixelFormatInfo.h>
#include <Converters/PixelOperation.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/string/conversions.h>
#include <Processing/ImageFlags.h>

#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/GenericStreams.h>

#include <AzFramework/StringFunc/StringFunc.h>

// Indicates a 2D texture is a cube-map texture.
#define DDS_RESOURCE_MISC_TEXTURECUBE   0x4

namespace ImageProcessing
{

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
        AZ_Assert(maxMipCount > 0, "image mipmap count need to larger than 0. maxMipCount: %d", maxMipCount);

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

    uint CImageObject::GetTextureMemory() const
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

        //if it's compressed format, return indeterminate. if user really want to know the content, they may convert the format to ARGB8 first
        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(m_pixelFormat))
        {
            AZ_Assert(false, "the function only works right with uncompressed formats. convert to uncompressed format if you get accurate result");
            return EAlphaContent::eAlphaContent_Indeterminate;
        }
        
        //go though alpha channel of first mip
        //create pixel operation function to access pixel data
        IPixelOperationPtr pixelOp = CreatePixelOperation(m_pixelFormat);

        //get count of bytes per pixel for images
        AZ::u32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(m_pixelFormat)->bitsPerBlock / 8;

        // counts of blacks and white
        uint nBlacks = 0;
        uint nWhites = 0;

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
    IImageObject* CImageObject::Clone() const
    {
        const EPixelFormat srcPixelformat = GetPixelFormat();

        IImageObject* pRet = AllocateImage();

        AZ::u32 dwMips = pRet->GetMipCount();
        for (AZ::u32 dwMip = 0; dwMip < dwMips; ++dwMip)
        {
            //AZ::u32 dwLocalWidth = GetWidth(dwMip);          // we get error on NVidia with this (assumes input is 4x4 as well)
            AZ::u32 dwLocalHeight = GetHeight(dwMip);

            AZ::u32 dwLines = dwLocalHeight;

            if (CPixelFormats::GetInstance().IsPixelFormatUncompressed(srcPixelformat))
            {
                dwLines = m_mips[dwMip]->m_rowCount;
            }

            AZ::u8* pMem;
            AZ::u32 dwPitch;
            GetImagePointer(dwMip, pMem, dwPitch);

            AZ::u8* pDstMem;
            AZ::u32 dwDstPitch;
            pRet->GetImagePointer(dwMip, pDstMem, dwDstPitch);

            for (AZ::u32 dwY = 0; dwY < dwLines; ++dwY)
            {
                memcpy(&pDstMem[dwDstPitch * dwY], &pMem[dwPitch * dwY], AZStd::min<AZ::u32>(dwPitch, dwDstPitch));
            }
        }
        return pRet;
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
    IImageObject* CImageObject::AllocateImage(EPixelFormat pixelFormat) const
    {
        AZ::u32 width = GetWidth(0);
        AZ::u32 height = GetHeight(0);

        if (!CPixelFormats::GetInstance().IsImageSizeValid(pixelFormat, width, height, false))
        {
            AZ_Assert(false, "Cann't allocate image with format: %d", pixelFormat);
            return nullptr;
        }

        CImageObject* pRet = aznew CImageObject(width, height, GetMipCount(), pixelFormat);
        pRet->CopyPropertiesFrom(this);
        return pRet;
    }

    IImageObject* CImageObject::AllocateImage() const
    {
        return AllocateImage(m_pixelFormat);
    }
    
    CImageObject::~CImageObject()
    {
        for (size_t i = 0; i < m_mips.size(); ++i)
        {
            delete m_mips[i];
        }
        m_mips.clear();
    }

    //note: there are some unreasonable parts of the save files formats for cry textures. We might need to rethink about
    // it for new renderer
    bool CImageObject::SaveImage(const char* filename, IImageObjectPtr alphaImage, AZStd::vector<AZStd::string>& outFilePaths) const
    {
        AZ::IO::SystemFile file;
        file.Open(filename, AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);

        AZ::IO::SystemFileStream fileSaveStream(&file, true);
        if (!fileSaveStream.IsOpen())
        {

            AZ_Warning("Image Processing", false, "%s: failed to create file %s", __FUNCTION__, filename);
            return false;
        }
        
        if (alphaImage)
        {
            AZ_Assert(HasImageFlags(EIF_AttachedAlpha), "attached alpha image flag wasn't set");
            AZ_Assert(!alphaImage->HasImageFlags(EIF_AttachedAlpha), "alpha image shouldn't have attached alpha image flag");

            // inherit cubemap and decal image flags to attached alpha image
            alphaImage->AddImageFlags(GetImageFlags() & (EIF_Cubemap 
                | EIF_Decal | EIF_Splitted));
            alphaImage->SetNumPersistentMips(m_numPersistentMips);
        }

        bool bOk = SaveImage(fileSaveStream);
        bool hasSplitFlag = HasImageFlags(EIF_Splitted);

        //append alpha image data in the end if there is no split
        if (bOk && alphaImage && !hasSplitFlag)
        {
            //4 bytes extension tag, 4 bytes attached alpha tag, then 4 bytes of chunk size
            fileSaveStream.Write(sizeof(FOURCC_CExt), &FOURCC_CExt); // marker for the start of Crytek Extended data
            fileSaveStream.Write(sizeof(FOURCC_AttC), &FOURCC_AttC); // Attached Channel chunk

            AZ::u32 size = 0;
            AZ::u32 sizeBytes = sizeof(size);
            fileSaveStream.Write(sizeBytes, &size); //size of attached chunk

            //save alpha image and get the size
            AZ::IO::SizeType startPos = fileSaveStream.GetCurPos();
            bOk = alphaImage->SaveImage(fileSaveStream);
            AZ::IO::SizeType endPos = fileSaveStream.GetCurPos();
            size = aznumeric_cast<AZ::u32>(endPos - startPos);

            //move back to beginning of chunk and write chunk size then move back to end
            fileSaveStream.Seek(startPos - sizeBytes, AZ::IO::GenericStream::ST_SEEK_BEGIN);
            fileSaveStream.Write(sizeBytes, &size);
            fileSaveStream.Seek(endPos, AZ::IO::GenericStream::ST_SEEK_BEGIN);

            // marker for the end of Crytek Extended data
            fileSaveStream.Write(sizeof(FOURCC_CEnd), &FOURCC_CEnd);
        }
        
        if (!bOk)
        {
            AZ::IO::SystemFile::Delete(filename);
            return false;
        }

        // It's important to maintain the product output sequence. Asset Database/Browser will use the first product to determine the source type!
        outFilePaths.push_back(filename);

        // save stand alone products
        if (hasSplitFlag)
        {
            // alpha
            if (alphaImage)
            {
                AZStd::string alphaFile = AZStd::string::format("%s.a", filename);

                AZ::IO::SystemFile outAlphaFile;
                outAlphaFile.Open(alphaFile.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);

                AZ::IO::SystemFileStream alphaFileSaveStream(&outAlphaFile, true);
                
                if (alphaFileSaveStream.IsOpen())
                {
                    alphaImage->SaveImage(alphaFileSaveStream);
                    outFilePaths.push_back(alphaFile);
                }
                else
                {
                    AZ_Warning("Image Processing", false, "%s: failed to create file %s", __FUNCTION__, alphaFile.c_str());
                }
            }

            // mips
            AZ::u32 numStreamable = GetMipCount() - m_numPersistentMips;
            for (AZ::u32 mip = 0; mip < numStreamable; mip++)
            {
                AZ::u32 nameIdx = numStreamable - mip;
                AZStd::string mipFileName = AZStd::string::format("%s.%d", filename, nameIdx);
                SaveMipToFile(mip, mipFileName);
                outFilePaths.push_back(mipFileName);
                if (alphaImage)
                {
                    AZStd::string mipAlphaFileName = mipFileName + "a";
                    alphaImage->SaveMipToFile(mip, mipAlphaFileName);
                    outFilePaths.push_back(mipAlphaFileName);
                }
            }
        }

        return bOk;
    }

    bool CImageObject::SaveMipToFile(AZ::u32 mip, const AZStd::string& filename) const
    {
        AZ::IO::SystemFile saveFile;
        saveFile.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY);

        AZ::IO::SystemFileStream saveFileStream(&saveFile, true);

        if (!saveFileStream.IsOpen())
        {
            AZ_Warning("Image Processing", false, "%s: failed to create file %s", __FUNCTION__, filename.c_str());
            return false;
        }

        saveFileStream.Write(GetMipBufSize(mip), m_mips[mip]->m_pData);
        return true;
    }

    IImageObject* CreateImageFromHeader(DDS_HEADER& header, DDS_HEADER_DXT10& exthead)
    {
        EPixelFormat eFormat = ePixelFormat_Unknown;
        AZ::u32 dwWidth, dwMips, dwHeight;
        AZ::u32 imageFlags = header.dwReserved1;
        AZ::Color colMinARGB, colMaxARGB;

        dwWidth = header.dwWidth;
        dwHeight = header.dwHeight;
        dwMips = 1;
        if (header.dwHeaderFlags & DDS_HEADER_FLAGS_MIPMAP)
        {
            dwMips = header.dwMipMapCount;
        }
        if ((header.dwSurfaceFlags & DDS_SURFACE_FLAGS_CUBEMAP) && (header.dwCubemapFlags & DDS_CUBEMAP_ALLFACES))
        {
            AZ_Assert(header.dwReserved1&EIF_Cubemap, "Image flag should have cubemap flag");
            dwHeight *= 6;
        }

        colMinARGB = AZ::Color(header.cMinColor[0], header.cMinColor[1], header.cMinColor[2], header.cMinColor[3]);
        colMaxARGB = AZ::Color(header.cMaxColor[0], header.cMaxColor[1], header.cMaxColor[2], header.cMaxColor[3]); 
        
        //get pixel format
        {
            // DX10 formats
            if (header.ddspf.dwFourCC == FOURCC_DX10)
            {
                AZ::u32 dxgiFormat = exthead.dxgiFormat;

                //remove the SRGB from dxgi format and add sRGB to image flag
                if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
                {
                    dxgiFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
                }
                else if (dxgiFormat == DXGI_FORMAT_BC1_UNORM_SRGB)
                {
                    dxgiFormat = DXGI_FORMAT_BC1_UNORM;
                }
                else if (dxgiFormat == DXGI_FORMAT_BC2_UNORM_SRGB)
                {
                    dxgiFormat = DXGI_FORMAT_BC2_UNORM;
                }
                else if (dxgiFormat == DXGI_FORMAT_BC3_UNORM_SRGB)
                {
                    dxgiFormat = DXGI_FORMAT_BC3_UNORM;
                }
                else if (dxgiFormat == DXGI_FORMAT_BC7_UNORM_SRGB)
                {
                    dxgiFormat = DXGI_FORMAT_BC7_UNORM;
                }

                //add rgb flag if the dxgiformat was changed (which means it was sRGB format) above
                if (dxgiFormat != exthead.dxgiFormat)
                {
                    AZ_Assert(imageFlags&EIF_SRGBRead, "Image flags should have SRGBRead flag");
                    imageFlags |= EIF_SRGBRead;
                }

                //check all the pixel formats and find matching one
                if (dxgiFormat != DXGI_FORMAT_UNKNOWN)
                {
                    int i = 0;
                    for (i; i<ePixelFormat_Count; i++)
                    {
                        const PixelFormatInfo *info = CPixelFormats::GetInstance().GetPixelFormatInfo((EPixelFormat)i);
                        if (info->d3d10Format == dxgiFormat)
                        {
                            eFormat = (EPixelFormat)i;
                            break;
                        }
                    }
                    if (i == ePixelFormat_Count)
                    {
                        AZ_Error("Image Processing", false, "Unhandled d3d10 format: %d", dxgiFormat);
                        return nullptr;
                    }
                }
            }
            else
            {
                //for non-dx10 formats, use fourCC to find out its pixel formats
                //go through all pixel formats and find a match with the fourcc
                for (AZ::u32 formatIdx = 0; formatIdx < ePixelFormat_Count; formatIdx++)
                {
                    const PixelFormatInfo *info = CPixelFormats::GetInstance().GetPixelFormatInfo((EPixelFormat)formatIdx);
                    if (header.ddspf.dwFourCC == info->fourCC)
                    {
                        eFormat = (EPixelFormat)formatIdx;
                        break;
                    }
                }

                //legacy formats. This section is only used for load dds files converted by RC.exe
                //our save to dds file function won't use any of these fourcc
                if (eFormat == ePixelFormat_Unknown)
                {
                    if (header.ddspf.dwFourCC == FOURCC_DXT1)
                    {
                        eFormat = ePixelFormat_BC1;
                    }
                    else if (header.ddspf.dwFourCC == FOURCC_DXT5)
                    {
                        eFormat = ePixelFormat_BC3;
                    }
                    else if (header.ddspf.dwFourCC == FOURCC_3DCP)
                    {
                        eFormat = ePixelFormat_BC4;
                    }
                    else if (header.ddspf.dwFourCC == FOURCC_3DC)
                    {
                        eFormat = ePixelFormat_BC5;
                    }
                    else if (header.ddspf.dwFourCC == DDS_FOURCC_R32F)
                    {
                        eFormat = ePixelFormat_R32F;
                    }
                    else if (header.ddspf.dwFourCC == DDS_FOURCC_G32R32F)
                    {
                        eFormat = ePixelFormat_R32G32F;
                    }
                    else if (header.ddspf.dwFourCC == DDS_FOURCC_A32B32G32R32F)
                    {
                        eFormat = ePixelFormat_R32G32B32A32F;
                    }
                    else if (header.ddspf.dwFourCC == DDS_FOURCC_R16F)
                    {
                        eFormat = ePixelFormat_R16F;
                    }
                    else if (header.ddspf.dwFourCC == DDS_FOURCC_G16R16F)
                    {
                        eFormat = ePixelFormat_R16G16F;
                    }
                    else if (header.ddspf.dwFourCC == DDS_FOURCC_A16B16G16R16F)
                    {
                        eFormat = ePixelFormat_R16G16B16A16F;
                    }
                    else if (header.ddspf.dwFourCC == DDS_FOURCC_A16B16G16R16)
                    {
                        eFormat = ePixelFormat_R16G16B16A16;
                    }
                    else if ((header.ddspf.dwFlags == DDS_RGBA || header.ddspf.dwFlags == DDS_RGB)
                        && header.ddspf.dwRGBBitCount == 32)
                    {
                        if (header.ddspf.dwRBitMask == 0x00ff0000)
                        {
                            eFormat = ePixelFormat_B8G8R8A8;
                        }
                        else
                        {
                            eFormat = ePixelFormat_R8G8B8A8;
                        }
                    }
                    else if (header.ddspf.dwFlags == DDS_LUMINANCEA && header.ddspf.dwRGBBitCount == 8)
                    {
                        eFormat = ePixelFormat_R8G8;
                    }
                    else if (header.ddspf.dwFlags == DDS_LUMINANCE  && header.ddspf.dwRGBBitCount == 8)
                    {
                        eFormat = ePixelFormat_A8;
                    }
                    else if ((header.ddspf.dwFlags == DDS_A || header.ddspf.dwFlags == DDS_A_ONLY || header.ddspf.dwFlags == (DDS_A | DDS_A_ONLY)) && header.ddspf.dwRGBBitCount == 8)
                    {
                        eFormat = ePixelFormat_A8;
                    }
                }
            }
        }

        if (eFormat == ePixelFormat_Unknown)
        {
            AZ_Error("Image Processing", false, "Unhandled dds pixel format fourCC: %d, flags: %d",
                header.ddspf.dwFourCC, header.ddspf.dwFlags);
            return nullptr;
        }

        IImageObject* newImage = IImageObject::CreateImage(dwWidth, dwHeight, dwMips, eFormat);

        if (dwMips != newImage->GetMipCount())
        {
            AZ_Error("Image Processing", false, "Mipcount from image data doesn't match image size and pixelformat");
            delete newImage;
            return nullptr;
        }

        //set properties
        newImage->SetImageFlags(imageFlags);
        newImage->SetAverageBrightness(header.fAvgBrightness);
        newImage->SetColorRange(colMinARGB, colMaxARGB);
        newImage->SetNumPersistentMips(header.bNumPersistentMips);

        return newImage;
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

    bool CImageObject::BuildSurfaceHeader(DDS_HEADER& header) const
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

        memset(&header, 0, sizeof(DDS_HEADER));

        header.dwSize = sizeof(DDS_HEADER);
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
        for (int i = 0; i < 4; i ++)
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
     
    bool CImageObject::SaveImage(AZ::IO::SystemFileStream &saveFileStream) const
    {        
        DDS_FILE_DESC desc;
        DDS_HEADER_DXT10 exthead;

        desc.dwMagic = FOURCC_DDS;

        if (!BuildSurfaceHeader(desc.header))
        {
            return false;
        }

        if (desc.header.IsDX10Ext() && !BuildSurfaceExtendedHeader(exthead))
        {
            return false;
        }

        saveFileStream.Write(sizeof(desc), &desc);

        if (desc.header.IsDX10Ext())
        {
            saveFileStream.Write(sizeof(exthead), &exthead);
        }

        AZ::u32 faces = 1;

        //for cubemap. export each face and its mipmap
        if (HasImageFlags(EIF_Cubemap))
        {
            faces = 6;
        }

        AZ::u32 mipStart = 0;
        if (HasImageFlags(EIF_Splitted))
        {
            if (m_numPersistentMips < m_mips.size())
            {
                mipStart = (AZ::u32)m_mips.size() - m_numPersistentMips;
            }
            else
            {
                AZ_Assert(false, "numPersistentMips wasn't setup correctly");
            }
        }

        for (AZ::u32 face = 0; face < faces; face++)
        {
            for (AZ::u32 mip = mipStart; mip < m_mips.size(); ++mip)
            {
                const MipLevel& level = *m_mips[mip];
                AZ::u32 faceBufSize = level.m_pitch*level.m_rowCount / faces;
                saveFileStream.Write(faceBufSize, level.m_pData + faceBufSize*face);
            }
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

        return  m_mips[mip]->m_rowCount * m_mips[mip]->m_pitch;
    }


    void CImageObject::SetMipData(AZ::u32 mip, AZ::u8* mipBuf, AZ::u32 bufSize, AZ::u32 pitch)
    {
        if (mip >= m_mips.size())
        {
            return;
        }
        m_mips[mip]->m_pData = mipBuf;
        m_mips[mip]->m_pitch = pitch;
        m_mips[mip]->m_rowCount = bufSize/pitch;
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
        return ((w&(w - 1)) == 0) && ((h&(h - 1)) == 0);
    }
        
    // use when you convert an image to another one
    void CImageObject::CopyPropertiesFrom(const IImageObjectPtr src)
    {
        const CImageObject *imageObj = static_cast<CImageObject *>(src.get());
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

    void CImageObject::ConvertLegacyGloss()
    {
        if (!(CPixelFormats::GetInstance().IsPixelFormatUncompressed(m_pixelFormat)))
        {
            AZ_Assert(false, "%s function only works with uncompressed pixel format", __FUNCTION__);
            return;
        }
        
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
                // Convert from (1 - s * 0.7)^6 to (1 - s)^2
                color[3] = 1 - pow(1.0f - color[3] * 0.7f, 3.0f);
                pixelOp->SetRGBA(pixelBuf, color[0], color[1], color[2], color[3]);
            }
        }
    }

    IImageObject* LoadImageFromDdsFile(const AZStd::string& filename)
    {
        AZ::IO::SystemFile file;
        file.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);

        AZ::IO::SystemFileStream fileLoadStream(&file, true);
        if (!fileLoadStream.IsOpen())
        {
            AZ_Warning("Image Processing", false, "%s: failed to open file %s", __FUNCTION__, filename.c_str());
            return nullptr;
        }

        AZStd::string ext = "";
        AzFramework::StringFunc::Path::GetExtension(filename.c_str(), ext, false);
        bool isAlphaImage = (ext == "a");
        
        IImageObject* imageObj = LoadImageFromDdsFile(fileLoadStream);
                
        //load mips from seperated files if it's splitted
        if (imageObj && imageObj->HasImageFlags(EIF_Splitted))
        {
            AZStd::string baseName;
            if (isAlphaImage)
            {
                baseName = filename.substr(0, filename.size() - 2);
            }
            else
            {
                baseName = filename;
            }

            AZ::u32 externalMipCount = 0;
            if (imageObj->GetNumPersistentMips() < imageObj->GetMipCount())
            {
                externalMipCount = imageObj->GetMipCount() - imageObj->GetNumPersistentMips();
            }
            //load other mips from files with number extensions
            for (AZ::u32 mipIdx = 1; mipIdx <= externalMipCount; mipIdx++)
            {
                AZ::u32 mip = externalMipCount - mipIdx;
                AZStd::string mipFileName = AZStd::string::format("%s.%d%s", baseName.c_str(), mipIdx, isAlphaImage?"a":"");
                
                AZ::IO::SystemFile mipFile;
                mipFile.Open(mipFileName.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);

                AZ::IO::SystemFileStream mipFileLoadStream(&mipFile, true);

                if (!mipFileLoadStream.IsOpen())
                {
                    AZ_Warning("Image Processing", false, "%s: failed to open mip file %s", __FUNCTION__, mipFileName.c_str());
                    break;
                }

                AZ::u32 pitch;
                AZ::u8* mem;
                imageObj->GetImagePointer(mip, mem, pitch);
                AZ::u32 bufSize = imageObj->GetMipBufSize(mip);
                mipFileLoadStream.Read(bufSize, mem);
            }
        }

        return imageObj;
    }

    IImageObject* LoadImageFromDdsFile(AZ::IO::SystemFileStream& fileLoadStream)
    {
        if (fileLoadStream.GetLength() - fileLoadStream.GetCurPos() < sizeof(DDS_FILE_DESC))
        {
            AZ_Error("Image Processing", false, "%s: Trying to load a none-DDS file", __FUNCTION__);
            return nullptr;
        }

        DDS_FILE_DESC desc;
        DDS_HEADER_DXT10 exthead;
        
        AZ::IO::SizeType startPos = fileLoadStream.GetCurPos();
        fileLoadStream.Read(sizeof(desc.dwMagic), &desc.dwMagic);

        if (desc.dwMagic != FOURCC_DDS)
        {
            desc.dwMagic = FOURCC_DDS;
            //the old cry .a file doesn't have "DDS " in the beginning of the file. 
            //so reset to previous position
            fileLoadStream.Seek(startPos, AZ::IO::GenericStream::ST_SEEK_BEGIN);
        }

        fileLoadStream.Read(sizeof(desc.header), &desc.header);

        if (!desc.IsValid())
        {
            AZ_Error("Image Processing", false, "%s: Trying to load a none-DDS file", __FUNCTION__);
            return nullptr;
        }

        if (desc.header.IsDX10Ext())
        {
            fileLoadStream.Read(sizeof(exthead), &exthead);
        }

        IImageObject* outImage = CreateImageFromHeader(desc.header, exthead);
        
        if (outImage == nullptr)
        {
            return nullptr;
        }

        //load mip data
        AZ::u32 mipStart = 0;
        //There are at least three lowest mips are in the file if it was splitted. This is to load splitted dds file exported by legacy rc.exe 
        int numPersistentMips = outImage->GetNumPersistentMips();
        if (numPersistentMips == 0 && outImage->HasImageFlags(EIF_Splitted))
        {
            outImage->SetNumPersistentMips(3);
        }

        if (outImage->HasImageFlags(EIF_Splitted) 
            && outImage->GetMipCount() > outImage->GetNumPersistentMips())
        {
            mipStart = outImage->GetMipCount() - outImage->GetNumPersistentMips();
        }

        AZ::u32 faces = 1;
        if (outImage->HasImageFlags(EIF_Cubemap))
        {
            faces = 6;
        }

        for (AZ::u32 face = 0; face < faces; face++)
        {
            for (AZ::u32 mip = mipStart; mip < outImage->GetMipCount(); ++mip)
            {
                AZ::u32 pitch;
                AZ::u8* mem;
                outImage->GetImagePointer(mip, mem, pitch);
                AZ::u32 faceBufSize = outImage->GetMipBufSize(mip) / faces;
                fileLoadStream.Read(faceBufSize, mem + faceBufSize*face);
            }
        }

        return outImage;
    }

    IImageObject* LoadAttachedImageFromDdsFile(const AZStd::string& filename, IImageObjectPtr originImage)
    {
        if (originImage == nullptr)
        {
            return nullptr;
        }

        AZ_Assert(originImage->HasImageFlags(EIF_AttachedAlpha), 
            "this function should only be called for origin image loaded from same file with attached alpha flag");
        
        AZ::IO::SystemFile file;
        file.Open(filename.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);

        AZ::IO::SystemFileStream fileLoadStream(&file, true);
        if (!fileLoadStream.IsOpen())
        {
            AZ_Warning("Image Processing", false, "%s: failed to open file %s", __FUNCTION__, filename.c_str());
            return nullptr;
        }
        
        DDS_FILE_DESC desc;
        DDS_HEADER_DXT10 exthead;

        fileLoadStream.Read(sizeof(desc), &desc);
        if (desc.dwMagic != FOURCC_DDS)
        {
            AZ_Error("Image Processing", false, "%s:Trying to load a none-DDS file", __FUNCTION__);
            return nullptr;
        }

        if (desc.header.IsDX10Ext())
        {
            fileLoadStream.Read(sizeof(exthead), &exthead);
        }

        //skip size for originImage's mip data
        for (AZ::u32 mip = 0; mip < originImage->GetMipCount(); ++mip)
        {
            AZ::u32 bufSize = originImage->GetMipBufSize(mip);
            fileLoadStream.Seek(bufSize, AZ::IO::GenericStream::ST_SEEK_CUR);
        }

        IImageObject* alphaImage = nullptr;

        AZ::u32 marker = 0;
        fileLoadStream.Read(4, &marker);
        if (marker == FOURCC_CExt) // marker for the start of Crytek Extended data
        {
            fileLoadStream.Read(4, &marker);
            if (FOURCC_AttC == marker) // Attached Channel chunk
            {
                AZ::u32 size = 0;
                fileLoadStream.Read(4, &size);

                alphaImage = LoadImageFromDdsFile(fileLoadStream);
                fileLoadStream.Read(4, &marker);
            }

            if (FOURCC_CEnd == marker ) // marker for the end of Crytek Extended data
            {
                fileLoadStream.Read(4, &marker);
            }
        }

        return alphaImage;
    }
    
} // namespace ImageProcessing
