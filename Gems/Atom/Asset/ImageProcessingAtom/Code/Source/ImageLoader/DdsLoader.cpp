/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ImageProcessing_precompiled.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/MathUtils.h>

#include <AzFramework/StringFunc/StringFunc.h>

#include <ImageLoader/ImageLoaders.h>
#include <Atom/ImageProcessing/ImageObject.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/DDSHeader.h>
#include <Processing/ImageFlags.h>

#include <QString>

namespace ImageProcessingAtom
{
    namespace DdsLoader
    {
        IImageObject* CreateImageFromHeaderLegacy(DDS_HEADER_LEGACY& header, DDS_HEADER_DXT10& exthead)
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
                AZ_Assert(header.dwReserved1 & EIF_Cubemap, "Image flag should have cubemap flag");
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
                        AZ_Assert(imageFlags & EIF_SRGBRead, "Image flags should have SRGBRead flag");
                        imageFlags |= EIF_SRGBRead;
                    }

                    //check all the pixel formats and find matching one
                    if (dxgiFormat != DXGI_FORMAT_UNKNOWN)
                    {
                        int i = 0;
                        for (i; i < ePixelFormat_Count; i++)
                        {
                            const PixelFormatInfo* info = CPixelFormats::GetInstance().GetPixelFormatInfo((EPixelFormat)i);
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
                        const PixelFormatInfo* info = CPixelFormats::GetInstance().GetPixelFormatInfo((EPixelFormat)formatIdx);
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


        bool IsExtensionSupported(const char* extension)
        {
            QString ext = QString(extension).toLower();
            // This is the list of file extensions supported by this loader
            return ext == "dds";
        }

        IImageObject* LoadImageFromFileLegacy(const AZStd::string& filename)
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

            IImageObject* imageObj = LoadImageFromFileStreamLegacy(fileLoadStream);

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
                    AZStd::string mipFileName = AZStd::string::format("%s.%d%s", baseName.c_str(), mipIdx, isAlphaImage ? "a" : "");

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

        IImageObject* LoadImageFromFileStreamLegacy(AZ::IO::SystemFileStream& fileLoadStream)
        {
            if (fileLoadStream.GetLength() - fileLoadStream.GetCurPos() < sizeof(DDS_FILE_DESC_LEGACY))
            {
                AZ_Error("Image Processing", false, "%s: Trying to load a none-DDS file", __FUNCTION__);
                return nullptr;
            }

            DDS_FILE_DESC_LEGACY desc;
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

            IImageObject* outImage = CreateImageFromHeaderLegacy(desc.header, exthead);

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
                    fileLoadStream.Read(faceBufSize, mem + faceBufSize * face);
                }
            }

            return outImage;
        }

        IImageObject* LoadAttachedImageFromDdsFileLegacy(const AZStd::string& filename, IImageObjectPtr originImage)
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

            DDS_FILE_DESC_LEGACY desc;
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
            if (marker == FOURCC_CExt) // marker for the start of O3DE Extended data
            {
                fileLoadStream.Read(4, &marker);
                if (FOURCC_AttC == marker) // Attached Channel chunk
                {
                    AZ::u32 size = 0;
                    fileLoadStream.Read(4, &size);

                    alphaImage = LoadImageFromFileStreamLegacy(fileLoadStream);
                    fileLoadStream.Read(4, &marker);
                }

                if (FOURCC_CEnd == marker) // marker for the end of O3DE Extended data
                {
                    fileLoadStream.Read(4, &marker);
                }
            }

            return alphaImage;
        }

        // Create an image object from standard dds header
        IImageObject* CreateImageFromHeader(DDS_HEADER& header, DDS_HEADER_DXT10& exthead)
        {
            if ((header.dwCaps & DDS_SURFACE_FLAGS_TEXTURE) != DDS_SURFACE_FLAGS_TEXTURE ||
                (header.dwFlags & DDS_HEADER_FLAGS_TEXTURE) != DDS_HEADER_FLAGS_TEXTURE)
            {
                AZ_Error("Image Processing", false, "This dds file is not a valid texture");
                return nullptr;
            }

            EPixelFormat format = ePixelFormat_Unknown;
            uint32_t imageFlags = 0;

            uint32_t width = header.dwWidth;
            uint32_t height = header.dwHeight;
            uint32_t mips = 1;
            if (header.dwFlags & DDS_HEADER_FLAGS_MIPMAP)
            {
                mips = header.dwMipMapCount;
            }
            if ((header.dwCaps & DDS_SURFACE_FLAGS_CUBEMAP) && (header.dwCaps_2 & DDS_CUBEMAP))
            {
                if ((header.dwCaps_2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
                {
                    AZ_Error("Image Processing", false, "Only support cubemap in dds file with all faces");
                    return nullptr;
                }
                imageFlags |= EIF_Cubemap;
                height *= 6;
            }

            // Get pixel format
            if (header.ddspf.dwFlags & DDS_FOURCC)
            {
                // dx10 formats
                if (header.ddspf.dwFourCC == FOURCC_DX10)
                {
                    uint32_t dxgiFormat = exthead.dxgiFormat;

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
                        imageFlags |= EIF_SRGBRead;
                    }

                    //check all the pixel formats and find matching one
                    if (dxgiFormat != DXGI_FORMAT_UNKNOWN)
                    {
                        uint32_t i = 0;
                        for (i; i < ePixelFormat_Count; i++)
                        {
                            const PixelFormatInfo* info = CPixelFormats::GetInstance().GetPixelFormatInfo((EPixelFormat)i);
                            if (info->d3d10Format == dxgiFormat)
                            {
                                format = (EPixelFormat)i;
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
                else if (header.ddspf.dwFourCC == FOURCC_DXT1)
                {
                    format = ePixelFormat_BC1;
                }
                else if (header.ddspf.dwFourCC == FOURCC_DXT5)
                {
                    format = ePixelFormat_BC3;
                }
                else if (header.ddspf.dwFourCC == FOURCC_3DCP)
                {
                    format = ePixelFormat_BC4;
                }
                else if (header.ddspf.dwFourCC == FOURCC_3DC)
                {
                    format = ePixelFormat_BC5;
                }
                else if (header.ddspf.dwFourCC == DDS_FOURCC_R32F)
                {
                    format = ePixelFormat_R32F;
                }
                else if (header.ddspf.dwFourCC == DDS_FOURCC_G32R32F)
                {
                    format = ePixelFormat_R32G32F;
                }
                else if (header.ddspf.dwFourCC == DDS_FOURCC_A32B32G32R32F)
                {
                    format = ePixelFormat_R32G32B32A32F;
                }
                else if (header.ddspf.dwFourCC == DDS_FOURCC_R16F)
                {
                    format = ePixelFormat_R16F;
                }
                else if (header.ddspf.dwFourCC == DDS_FOURCC_G16R16F)
                {
                    format = ePixelFormat_R16G16F;
                }
                else if (header.ddspf.dwFourCC == DDS_FOURCC_A16B16G16R16F)
                {
                    format = ePixelFormat_R16G16B16A16F;
                }
                else if (header.ddspf.dwFourCC == DDS_FOURCC_A16B16G16R16)
                {
                    format = ePixelFormat_R16G16B16A16;
                }
            }
            else
            {
                if ((header.ddspf.dwFlags == DDS_RGBA || header.ddspf.dwFlags == DDS_RGB))
                {
                    if (header.ddspf.dwRBitMask == 0x00ff0000 || header.ddspf.dwGBitMask == 0x00ff0000 || header.ddspf.dwBBitMask == 0x00ff0000)
                    {
                        format = (header.ddspf.dwRGBBitCount == 32) ? ePixelFormat_B8G8R8A8 : ePixelFormat_B8G8R8;
                    }
                    else if (header.ddspf.dwBBitMask == 0x00ff0000 || header.ddspf.dwGBitMask == 0x00ff0000 || header.ddspf.dwRBitMask == 0x00ff0000)
                    {
                        format = (header.ddspf.dwRGBBitCount == 32) ? ePixelFormat_R8G8B8A8 : ePixelFormat_R8G8B8;
                    }
                }
                else if (header.ddspf.dwFlags == DDS_LUMINANCEA && header.ddspf.dwRGBBitCount == 8)
                {
                    format = ePixelFormat_R8G8;
                }
                else if (header.ddspf.dwFlags == DDS_LUMINANCE && header.ddspf.dwRGBBitCount == 8)
                {
                    format = ePixelFormat_A8;
                }
                else if ((header.ddspf.dwFlags == DDS_A || header.ddspf.dwFlags == DDS_A_ONLY || header.ddspf.dwFlags == (DDS_A | DDS_A_ONLY)) && header.ddspf.dwRGBBitCount == 8)
                {
                    format = ePixelFormat_A8;
                }
            }

            if (format == ePixelFormat_Unknown)
            {
                AZ_Error("Image Processing", false, "Unhandled dds pixel format fourCC: %d, flags: %d",
                    header.ddspf.dwFourCC, header.ddspf.dwFlags);
                return nullptr;
            }

            // Resize to block size for compressed format. This could happened to those 1x1 bc1 dds files
            // [GFX TODO] [ATOM-181] We may want to add padding support for bc formats so we can remove this temporary fix
            const PixelFormatInfo* const pFormatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(format);
            if (width < pFormatInfo->blockWidth && height < pFormatInfo->blockHeight)
            {
                width = pFormatInfo->blockWidth;
                height = pFormatInfo->blockHeight;
            }

            IImageObject* newImage = IImageObject::CreateImage(width, height, mips, format);

            //set properties
            newImage->SetImageFlags(imageFlags);

            return newImage;
        }

        IImageObject* LoadImageFromFile(const AZStd::string& filename)
        {
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

            if (desc.dwMagic != FOURCC_DDS || !desc.IsValid())
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

            AZ::u32 faces = 1;
            if (outImage->HasImageFlags(EIF_Cubemap))
            {
                faces = 6;
            }

            for (AZ::u32 face = 0; face < faces; face++)
            {
                for (AZ::u32 mip = 0; mip < outImage->GetMipCount(); ++mip)
                {
                    AZ::u32 pitch;
                    AZ::u8* mem;
                    outImage->GetImagePointer(mip, mem, pitch);
                    AZ::u32 faceBufSize = outImage->GetMipBufSize(mip) / faces;

                    if (fileLoadStream.GetLength() - fileLoadStream.GetCurPos() < faceBufSize)
                    {
                        delete outImage;
                        AZ_Error("Image Processing", false, "DdsLoader: load mip data error");
                        return nullptr;
                    }
                    fileLoadStream.Read(faceBufSize, mem + faceBufSize * face);
                }
            }

            return outImage;
        }
    }// namespace ImageDDS
} //namespace ImageProcessingAtom
