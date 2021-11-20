/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
        bool IsExtensionSupported(const char* extension)
        {
            QString ext = QString(extension).toLower();
            // This is the list of file extensions supported by this loader
            return ext == "dds";
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
                        for (; i < ePixelFormat_Count; i++)
                        {
                            const PixelFormatInfo* info = CPixelFormats::GetInstance().GetPixelFormatInfo((EPixelFormat)i);
                            if (static_cast<uint32_t>(info->d3d10Format) == dxgiFormat)
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
