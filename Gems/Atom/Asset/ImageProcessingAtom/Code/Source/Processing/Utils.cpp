/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/RPI.Reflect/Image/StreamingImageAsset.h>
#include <Atom/RPI.Reflect/Image/StreamingImageAssetHandler.h>
#include <Atom/Utils/DdsFile.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <Processing/ImageToProcess.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/Utils.h>
#include <Processing/ImageFlags.h>

namespace ImageProcessingAtom
{
    namespace Utils
    {
        using namespace AZ;

        EPixelFormat RHIFormatToPixelFormat(AZ::RHI::Format rhiFormat, bool& isSRGB)
        {
            isSRGB = false;
            switch (rhiFormat)
            {
            case AZ::RHI::Format::R8G8B8A8_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::R8G8B8A8_UNORM:
                return ePixelFormat_R8G8B8A8;
            case AZ::RHI::Format::R8G8_UNORM:
                return ePixelFormat_R8G8;
            case AZ::RHI::Format::R8_UNORM:
                return ePixelFormat_R8;
            case AZ::RHI::Format::A8_UNORM:
                return ePixelFormat_A8;
            case AZ::RHI::Format::R16G16B16A16_UNORM:
                return ePixelFormat_R16G16B16A16;
            case AZ::RHI::Format::R16G16_UNORM:
                return ePixelFormat_R16G16;
            case AZ::RHI::Format::R16_UNORM:
                return ePixelFormat_R16;

            case AZ::RHI::Format::ASTC_4x4_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_4x4_UNORM:
                return ePixelFormat_ASTC_4x4;
            case AZ::RHI::Format::ASTC_5x4_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_5x4_UNORM:
                return ePixelFormat_ASTC_5x4;
            case AZ::RHI::Format::ASTC_5x5_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_5x5_UNORM:
                return ePixelFormat_ASTC_5x5;
            case AZ::RHI::Format::ASTC_6x5_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_6x5_UNORM:
                return ePixelFormat_ASTC_6x5;
            case AZ::RHI::Format::ASTC_6x6_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_6x6_UNORM:
                return ePixelFormat_ASTC_6x6;
            case AZ::RHI::Format::ASTC_8x5_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_8x5_UNORM:
                return ePixelFormat_ASTC_8x5;
            case AZ::RHI::Format::ASTC_8x6_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_8x6_UNORM:
                return ePixelFormat_ASTC_8x6;
            case AZ::RHI::Format::ASTC_8x8_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_8x8_UNORM:
                return ePixelFormat_ASTC_8x8;
            case AZ::RHI::Format::ASTC_10x5_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_10x5_UNORM:
                return ePixelFormat_ASTC_10x5;
            case AZ::RHI::Format::ASTC_10x6_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_10x6_UNORM:
                return ePixelFormat_ASTC_10x6;
            case AZ::RHI::Format::ASTC_10x8_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_10x8_UNORM:
                return ePixelFormat_ASTC_10x8;
            case AZ::RHI::Format::ASTC_10x10_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_10x10_UNORM:
                return ePixelFormat_ASTC_10x10;
            case AZ::RHI::Format::ASTC_12x10_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_12x10_UNORM:
                return ePixelFormat_ASTC_12x10;
            case AZ::RHI::Format::ASTC_12x12_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::ASTC_12x12_UNORM:
                return ePixelFormat_ASTC_12x12;

            case AZ::RHI::Format::BC1_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::BC1_UNORM:
                return ePixelFormat_BC1;
            case AZ::RHI::Format::BC3_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::BC3_UNORM:
                return ePixelFormat_BC3;
            case AZ::RHI::Format::BC4_UNORM:
                return ePixelFormat_BC4;
            case AZ::RHI::Format::BC4_SNORM:
                return ePixelFormat_BC4s;
            case AZ::RHI::Format::BC5_UNORM:
                return ePixelFormat_BC5;
            case AZ::RHI::Format::BC5_SNORM:
                return ePixelFormat_BC5s;
            case AZ::RHI::Format::BC6H_UF16:
                return ePixelFormat_BC6UH;
            case AZ::RHI::Format::BC7_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::BC7_UNORM:
                return ePixelFormat_BC7;
            case AZ::RHI::Format::R9G9B9E5_SHAREDEXP:
                return ePixelFormat_R9G9B9E5;
            case AZ::RHI::Format::R32G32B32A32_FLOAT:
                return ePixelFormat_R32G32B32A32F;
            case AZ::RHI::Format::R32G32_FLOAT:
                return ePixelFormat_R32G32F;
            case AZ::RHI::Format::R32_FLOAT:
                return ePixelFormat_R32F;
            case AZ::RHI::Format::R16G16B16A16_FLOAT:
                return ePixelFormat_R16G16B16A16F;
            case AZ::RHI::Format::R16G16_FLOAT:
                return ePixelFormat_R16G16F;
            case AZ::RHI::Format::R16_FLOAT:
                return ePixelFormat_R16F;
            case AZ::RHI::Format::B8G8R8A8_UNORM_SRGB:
                isSRGB = true;
            case AZ::RHI::Format::B8G8R8A8_UNORM:
                return ePixelFormat_B8G8R8A8;
            case AZ::RHI::Format::R32_UINT:
                return ePixelFormat_R32;
            default:
                AZ_Warning("Image Processing", false, "Unknown pixel format");
                return ePixelFormat_Unknown;
            }
        }

        AZ::RHI::Format PixelFormatToRHIFormat(EPixelFormat pixelFormat, bool isSrgb)
        {
            switch (pixelFormat)
            {
            case ePixelFormat_R8G8B8A8:
            case ePixelFormat_R8G8B8X8:
                return isSrgb ? RHI::Format::R8G8B8A8_UNORM_SRGB : RHI::Format::R8G8B8A8_UNORM;
            case ePixelFormat_R8G8:
                return RHI::Format::R8G8_UNORM;
            case ePixelFormat_R8:
                return RHI::Format::R8_UNORM;
            case ePixelFormat_A8:
                return RHI::Format::A8_UNORM;
            case ePixelFormat_R16G16B16A16:
                return RHI::Format::R16G16B16A16_UNORM;
            case ePixelFormat_R16G16:
                return RHI::Format::R16G16_UNORM;
            case ePixelFormat_R16:
                return RHI::Format::R16_UNORM;

            case ePixelFormat_ASTC_4x4:
                return isSrgb ? RHI::Format::ASTC_4x4_UNORM_SRGB : RHI::Format::ASTC_4x4_UNORM;
            case ePixelFormat_ASTC_5x4:
                return isSrgb ? RHI::Format::ASTC_5x4_UNORM_SRGB : RHI::Format::ASTC_5x4_UNORM;
            case ePixelFormat_ASTC_5x5:
                return isSrgb ? RHI::Format::ASTC_5x5_UNORM_SRGB : RHI::Format::ASTC_5x5_UNORM;
            case ePixelFormat_ASTC_6x5:
                return isSrgb ? RHI::Format::ASTC_6x5_UNORM_SRGB : RHI::Format::ASTC_6x5_UNORM;
            case ePixelFormat_ASTC_6x6:
                return isSrgb ? RHI::Format::ASTC_6x6_UNORM_SRGB : RHI::Format::ASTC_6x6_UNORM;
            case ePixelFormat_ASTC_8x5:
                return isSrgb ? RHI::Format::ASTC_8x5_UNORM_SRGB : RHI::Format::ASTC_8x5_UNORM;
            case ePixelFormat_ASTC_8x6:
                return isSrgb ? RHI::Format::ASTC_8x6_UNORM_SRGB : RHI::Format::ASTC_8x6_UNORM;
            case ePixelFormat_ASTC_8x8:
                return isSrgb ? RHI::Format::ASTC_8x8_UNORM_SRGB : RHI::Format::ASTC_8x8_UNORM;
            case ePixelFormat_ASTC_10x5:
                return isSrgb ? RHI::Format::ASTC_10x5_UNORM_SRGB : RHI::Format::ASTC_10x5_UNORM;
            case ePixelFormat_ASTC_10x6:
                return isSrgb ? RHI::Format::ASTC_10x6_UNORM_SRGB : RHI::Format::ASTC_10x6_UNORM;
            case ePixelFormat_ASTC_10x8:
                return isSrgb ? RHI::Format::ASTC_10x8_UNORM_SRGB : RHI::Format::ASTC_10x8_UNORM;
            case ePixelFormat_ASTC_10x10:
                return isSrgb ? RHI::Format::ASTC_10x10_UNORM_SRGB : RHI::Format::ASTC_10x10_UNORM;
            case ePixelFormat_ASTC_12x10:
                return isSrgb ? RHI::Format::ASTC_12x10_UNORM_SRGB : RHI::Format::ASTC_12x10_UNORM;
            case ePixelFormat_ASTC_12x12:
                return isSrgb ? RHI::Format::ASTC_12x12_UNORM_SRGB : RHI::Format::ASTC_12x12_UNORM;

            case ePixelFormat_BC1:
            case ePixelFormat_BC1a:
                return isSrgb ? RHI::Format::BC1_UNORM_SRGB : RHI::Format::BC1_UNORM;
            case ePixelFormat_BC3:
            case ePixelFormat_BC3t:
                return isSrgb ? RHI::Format::BC3_UNORM_SRGB : RHI::Format::BC3_UNORM;
            case ePixelFormat_BC4:
                return RHI::Format::BC4_UNORM;
            case ePixelFormat_BC4s:
                return RHI::Format::BC4_SNORM;
            case ePixelFormat_BC5:
                return RHI::Format::BC5_UNORM;
            case ePixelFormat_BC5s:
                return RHI::Format::BC5_SNORM;
            case ePixelFormat_BC6UH:
                return RHI::Format::BC6H_UF16;
            case ePixelFormat_BC7:
            case ePixelFormat_BC7t:
                return isSrgb ? RHI::Format::BC7_UNORM_SRGB : RHI::Format::BC7_UNORM;
            case ePixelFormat_R9G9B9E5:
                return RHI::Format::R9G9B9E5_SHAREDEXP;
            case ePixelFormat_R32G32B32A32F:
                return RHI::Format::R32G32B32A32_FLOAT;
            case ePixelFormat_R32G32F:
                return RHI::Format::R32G32_FLOAT;
            case ePixelFormat_R32F:
                return RHI::Format::R32_FLOAT;
            case ePixelFormat_R16G16B16A16F:
                return RHI::Format::R16G16B16A16_FLOAT;
            case ePixelFormat_R16G16F:
                return RHI::Format::R16G16_FLOAT;
            case ePixelFormat_R16F:
                return RHI::Format::R16_FLOAT;
            case ePixelFormat_B8G8R8A8:
                return isSrgb ? RHI::Format::B8G8R8A8_UNORM_SRGB : RHI::Format::B8G8R8A8_UNORM;
            case ePixelFormat_R32:
                return RHI::Format::R32_UINT;
            default:
                AZ_Warning("Image Processing", false, "Unknown pixel format");
                return RHI::Format::Unknown;
            }
        }

        IImageObjectPtr LoadImageFromImageAsset(const AZ::Data::Asset<AZ::RPI::StreamingImageAsset>& imageAsset)
        {
            if (!imageAsset.IsReady())
            {
                return nullptr;
            }

            const AZ::RHI::ImageDescriptor imageDescriptor = imageAsset->GetImageDescriptor();

            // create an image object based on the image descriptor to store all the mip data
            bool isSRGB;
            EPixelFormat format = RHIFormatToPixelFormat(imageDescriptor.m_format, isSRGB);

            u32 width = imageDescriptor.m_size.m_width;
            u32 height = imageDescriptor.m_size.m_height;
            u32 mipLevels = imageDescriptor.m_mipLevels;
            u32 arraySize = imageDescriptor.m_arraySize;

            if (imageDescriptor.m_isCubemap)
            {
                height *= 6;
            }

            IImageObjectPtr outputImage = IImageObjectPtr(IImageObject::CreateImage(width, height, mipLevels, format));

            if (isSRGB)
            {
                outputImage->AddImageFlags(EIF_SRGBRead);
            }

            if (imageDescriptor.m_isCubemap)
            {
                outputImage->AddImageFlags(EIF_Cubemap);
            }

            // copy image data from asset to image object
            for (u32 mip = 0; mip < mipLevels; mip++)
            {
                AZ::u8* imageBuf;
                AZ::u32 pitch;
                outputImage->GetImagePointer(mip, imageBuf, pitch);

                for (u32 slice = 0; slice < arraySize; slice++)
                {
                    AZStd::array_view<uint8_t> imageData = imageAsset->GetSubImageData(mip, slice);
                    memcpy(imageBuf + slice * imageData.size(), imageData.data(), imageData.size());
                }
            }
            return outputImage;
        }

        AZ::Data::Asset<AZ::RPI::StreamingImageAsset> LoadImageAsset(const AZ::Data::AssetId& imageAssetId)
        {
            // Blocking loading streaming image asset with its mipchain assets
            AZ::Data::Asset<AZ::RPI::StreamingImageAsset> imageAsset = AZ::Data::AssetManager::Instance().GetAsset<AZ::RPI::StreamingImageAsset>(imageAssetId, AZ::Data::AssetLoadBehavior::PreLoad);

            imageAsset.BlockUntilLoadComplete();

            return imageAsset;
        }

        IImageObjectPtr LoadImageFromImageAsset(const AZ::Data::AssetId& imageAssetId)
        {
            auto imageAsset = LoadImageAsset(imageAssetId);
            return LoadImageFromImageAsset(imageAsset);
        }

        bool SaveImageToDdsFile(IImageObjectPtr image, AZStd::string_view filePath)
        {
            RHI::Format format = PixelFormatToRHIFormat(image->GetPixelFormat(), image->HasImageFlags(EIF_SRGBRead));
            IImageObjectPtr imageToSave = image;

            //Some compressed formats such as astc formats are not supported by dds. We need do some decompressing first.
            if (!AZ::DdsFile::DoesSupportFormat(format))
            {
                // Convert to some uncompressed format
                ImageToProcess imageToProcess(image);
                imageToProcess.ConvertFormat(ePixelFormat_R8G8B8A8);
                imageToSave = imageToProcess.Get();
            }

            DdsFile::DdsFileData ddsFileData;
            ddsFileData.m_size.m_width = imageToSave->GetWidth(0);
            ddsFileData.m_size.m_height = imageToSave->GetHeight(0);
            ddsFileData.m_format = PixelFormatToRHIFormat(imageToSave->GetPixelFormat(), imageToSave->HasImageFlags(EIF_SRGBRead));
            ddsFileData.m_mipLevels = imageToSave->GetMipCount();

            if (image->HasImageFlags(EIF_Cubemap))
            {
                ddsFileData.m_size.m_height /= 6;
                ddsFileData.m_isCubemap = true;
            }
            
            AZ::u32 bufferSize = 0;
            for (AZ::u32 mip = 0; mip < ddsFileData.m_mipLevels; mip++)
            {
                bufferSize += imageToSave->GetMipBufSize(mip);
            }

            AZStd::vector<uint8_t> buffer;
            buffer.resize_no_construct(bufferSize);

            ddsFileData.m_buffer = &buffer;

            AZ::u32 faces = 1;
            if (imageToSave->HasImageFlags(EIF_Cubemap))
            {
                faces = 6;
            }

            // The sub images are saved with this order:
            // face 1: mip 0, mip 1, mip2 ...
            // face 2: mip 0, mip 1, mip2 ...
            // ...
            // face 6: mip 0, mip 1, mip2 ...

            AZ::u8* currentPos = buffer.data();

            for (AZ::u32 face = 0; face < faces; face++)
            {
                for (AZ::u32 mip = 0; mip < ddsFileData.m_mipLevels; mip++)
                {
                    AZ::u32 mipBufferSize = imageToSave->GetMipBufSize(mip);
                    AZ::u32 subImageSize = mipBufferSize / faces;

                    AZ::u8* imageBuffer = nullptr;
                    AZ::u32 pitch = 0;

                    imageToSave->GetImagePointer(mip, imageBuffer, pitch);

                    memcpy(currentPos, imageBuffer + subImageSize * face, subImageSize);
                    currentPos += subImageSize;
                }
            }            

            auto outcome = AZ::DdsFile::WriteFile(filePath, ddsFileData);
            if (!outcome)
            {
                AZ_Warning("WriteDds", false, "%s", outcome.GetError().m_message.c_str());
                return false;
            }
            return true;
        }
    }

} // namespace ImageProcessingAtom
