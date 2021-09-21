/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <astcenc.h>

#include <AzCore/Jobs/JobCompletion.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/PlatformIncl.h>

#include <Atom/ImageProcessing/ImageObject.h>
#include <Compressors/ASTCCompressor.h>
#include <Processing/ImageFlags.h>
#include <Processing/ImageToProcess.h>
#include <Processing/PixelFormatInfo.h>


namespace ImageProcessingAtom
{
    bool ASTCCompressor::IsCompressedPixelFormatSupported(EPixelFormat fmt)
    {
        return IsASTCFormat(fmt);
    }

    bool ASTCCompressor::IsUncompressedPixelFormatSupported(EPixelFormat fmt)
    {
        // astc encoder requires the compress input image or decompress output image to have four channels
        switch (fmt)
        {
        // uint 8
        case ePixelFormat_R8G8B8A8:
        case ePixelFormat_R8G8B8X8:
        // fp16
        case ePixelFormat_R16G16B16A16F:
        // fp32
        case ePixelFormat_R32G32B32A32F:
            return true;
        default:
            return false;
        }
    }

    EPixelFormat ASTCCompressor::GetSuggestedUncompressedFormat([[maybe_unused]] EPixelFormat compressedfmt, EPixelFormat uncompressedfmt) const
    {
        if (IsUncompressedPixelFormatSupported(uncompressedfmt))
        {
            return uncompressedfmt;
        }
        
        auto formatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(uncompressedfmt);
        switch (formatInfo->eSampleType)
        {
        case ESampleType::eSampleType_Half:
            return ePixelFormat_R16G16B16A16F;
        case ESampleType::eSampleType_Float:
            return ePixelFormat_R32G32B32A32F;
        }

        return ePixelFormat_R8G8B8A8;
    }

    ColorSpace ASTCCompressor::GetSupportedColorSpace([[maybe_unused]] EPixelFormat compressFormat) const
    {
        return ColorSpace::autoSelect;
    }

    const char* ASTCCompressor::GetName() const
    {
        return "ASTCCompressor";
    }

    bool ASTCCompressor::DoesSupportDecompress([[maybe_unused]] EPixelFormat fmtDst)
    {
        return true;
    }

    astcenc_profile GetAstcProfile(bool isSrgb, EPixelFormat pixelFormat)
    {
        // select profile depends on LDR or HDR, SRGB or Linear
        //      ASTCENC_PRF_LDR
        //      ASTCENC_PRF_LDR_SRGB
        //      ASTCENC_PRF_HDR_RGB_LDR_A
        //      ASTCENC_PRF_HDR
        
        auto formatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(pixelFormat);
        bool isHDR = formatInfo->eSampleType == ESampleType::eSampleType_Half || formatInfo->eSampleType == ESampleType::eSampleType_Float;
        astcenc_profile profile;
        if (isHDR)
        {
            // HDR is not support in core vulkan 1.1 for android.
            // https://arm-software.github.io/vulkan-sdk/_a_s_t_c.html
            profile = isSrgb?ASTCENC_PRF_HDR_RGB_LDR_A:ASTCENC_PRF_HDR;
        }
        else
        {
            
            profile = isSrgb?ASTCENC_PRF_LDR_SRGB:ASTCENC_PRF_LDR;
        }
        return profile;
    }

    astcenc_type GetAstcDataType(EPixelFormat pixelFormat)
    {        
        auto formatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(pixelFormat);
        astcenc_type dataType = ASTCENC_TYPE_U8;

        switch (formatInfo->eSampleType)
        {
        case ESampleType::eSampleType_Uint8:
            dataType = ASTCENC_TYPE_U8;
            break;
        case ESampleType::eSampleType_Half:
            dataType = ASTCENC_TYPE_F16;
            break;
        case ESampleType::eSampleType_Float:
            dataType = ASTCENC_TYPE_F32;
            break;
        default:
            dataType = ASTCENC_TYPE_U8;
            AZ_Assert(false, "Unsupport uncompressed format %s", formatInfo->szName);
            break;
        }

        return dataType;
    }

    float GetAstcCompressQuality(ICompressor::EQuality quality)
    {
        switch (quality)
        {
        case ICompressor::EQuality::eQuality_Fast:
            return ASTCENC_PRE_FAST;
        case ICompressor::EQuality::eQuality_Slow:
            return ASTCENC_PRE_THOROUGH;
        case ICompressor::EQuality::eQuality_Preview:
        case ICompressor::EQuality::eQuality_Normal:
        default:
            return ASTCENC_PRE_MEDIUM;
        }
    }

    IImageObjectPtr ASTCCompressor::CompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst, const CompressOption* compressOption) const
    {
        //validate input
        EPixelFormat fmtSrc = srcImage->GetPixelFormat();

        //src format need to be uncompressed and dst format need to compressed.
        if (!IsUncompressedPixelFormatSupported(fmtSrc) || !IsCompressedPixelFormatSupported(fmtDst))
        {
            return nullptr;
        }

        astcenc_swizzle swizzle {ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, compressOption->discardAlpha? ASTCENC_SWZ_1:ASTCENC_SWZ_A};
        AZ::u32 flags = 0;
        if (srcImage->HasImageFlags(EIF_RenormalizedTexture))
        {
            ImageToProcess imageToProcess(srcImage);
            imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
            srcImage = imageToProcess.Get();
            fmtSrc = srcImage->GetPixelFormat();

            flags = ASTCENC_FLG_MAP_NORMAL;
            swizzle = astcenc_swizzle{ ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_G };
        }

        auto dstFormatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(fmtDst);

        const float quality = GetAstcCompressQuality(compressOption->compressQuality);
        const astcenc_profile profile = GetAstcProfile(srcImage->HasImageFlags(EIF_SRGBRead), fmtSrc);

        astcenc_config config;
        astcenc_error status;
        status = astcenc_config_init(profile, dstFormatInfo->blockWidth, dstFormatInfo->blockHeight, 1, quality, flags, &config);

        //ASTCENC_FLG_MAP_NORMAL
        AZ_Assert( status == ASTCENC_SUCCESS, "ERROR: Codec config init failed: %s\n", astcenc_get_error_string(status));

        // Create a context based on the configuration
        astcenc_context* context;
        AZ::u32 blockCount = ((srcImage->GetWidth(0)+ dstFormatInfo->blockWidth-1)/dstFormatInfo->blockWidth) * ((srcImage->GetHeight(0) + dstFormatInfo->blockHeight-1)/dstFormatInfo->blockHeight);
        AZ::u32 threadCount = AZStd::min(AZStd::thread::hardware_concurrency(), blockCount);
        status = astcenc_context_alloc(&config, threadCount, &context);
        AZ_Assert( status == ASTCENC_SUCCESS, "ERROR: Codec context alloc failed: %s\n", astcenc_get_error_string(status));

        const astcenc_type dataType =GetAstcDataType(fmtSrc);

        // Compress the image for each mips
        IImageObjectPtr dstImage(srcImage->AllocateImage(fmtDst));
        const AZ::u32 dstMips = dstImage->GetMipCount();
        for (AZ::u32 mip = 0; mip < dstMips; ++mip)
        {
            astcenc_image image;
            image.dim_x = srcImage->GetWidth(mip);
            image.dim_y = srcImage->GetHeight(mip);
            image.dim_z = 1;
            image.data_type = dataType;
                        
            AZ::u8* srcMem;
            AZ::u32 srcPitch;
            srcImage->GetImagePointer(mip, srcMem, srcPitch);
            image.data = reinterpret_cast<void**>(&srcMem);
                        
            AZ::u8* dstMem;
            AZ::u32 dstPitch;
            dstImage->GetImagePointer(mip, dstMem, dstPitch);
            AZ::u32 dataSize = dstImage->GetMipBufSize(mip);

            // Create jobs for each compression thread
            auto completionJob = aznew AZ::JobCompletion();
            for (AZ::u32 threadIdx = 0; threadIdx < threadCount; threadIdx++)
            {
                const auto jobLambda = [&status, context, &image, &swizzle, dstMem, dataSize, threadIdx]()
                {
                    astcenc_error error = astcenc_compress_image(context, &image, &swizzle, dstMem, dataSize, threadIdx);
                    if (error != ASTCENC_SUCCESS)
                    {
                        status = error;
                    }
                };

                AZ::Job* simulationJob = AZ::CreateJobFunction(AZStd::move(jobLambda), true, nullptr);  //auto-deletes
                simulationJob->SetDependent(completionJob);
                simulationJob->Start();
            }

            if (completionJob)
            {
                completionJob->StartAndWaitForCompletion();
                delete completionJob;
                completionJob = nullptr;
            }

            if (status != ASTCENC_SUCCESS)
            {
                AZ_Error("Image Processing", false, "ASTCCompressor::CompressImage failed: %s\n", astcenc_get_error_string(status));
                astcenc_context_free(context);
                return nullptr;
            }

            // Need to reset to compress next mip
            astcenc_compress_reset(context);
        }
        astcenc_context_free(context);

        return dstImage;
    }

    IImageObjectPtr ASTCCompressor::DecompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst) const
    {
        //validate input
        EPixelFormat fmtSrc = srcImage->GetPixelFormat(); //compressed
        auto srcFormatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(fmtSrc);

        if (!IsCompressedPixelFormatSupported(fmtSrc) || !IsUncompressedPixelFormatSupported(fmtDst))
        {
            return nullptr;
        }

        const float quality = ASTCENC_PRE_MEDIUM;
        astcenc_swizzle swizzle {ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A};
        if (srcImage->HasImageFlags(EIF_RenormalizedTexture))
        {
            swizzle = astcenc_swizzle{ASTCENC_SWZ_R, ASTCENC_SWZ_A, ASTCENC_SWZ_Z, ASTCENC_SWZ_1};
        }

        astcenc_config config;
        astcenc_error status;
        astcenc_profile profile = GetAstcProfile(srcImage->HasImageFlags(EIF_SRGBRead), fmtDst);
        AZ::u32 flags = ASTCENC_FLG_DECOMPRESS_ONLY;
        status = astcenc_config_init(profile, srcFormatInfo->blockWidth, srcFormatInfo->blockHeight, 1, quality, flags, &config);

        //ASTCENC_FLG_MAP_NORMAL
        AZ_Assert( status == ASTCENC_SUCCESS, "astcenc_config_init failed: %s\n", astcenc_get_error_string(status));

        // Create a context based on the configuration
        const AZ::u32 threadCount = 1; // Decompress function doesn't support multiple threads
        astcenc_context* context;
        status = astcenc_context_alloc(&config, threadCount, &context);
        AZ_Assert( status == ASTCENC_SUCCESS, "astcenc_context_alloc failed: %s\n", astcenc_get_error_string(status));

        astcenc_type dataType =GetAstcDataType(fmtDst);

        // Decompress the image for each mips
        IImageObjectPtr dstImage(srcImage->AllocateImage(fmtDst));
        const AZ::u32 dstMips = dstImage->GetMipCount();
        for (AZ::u32 mip = 0; mip < dstMips; ++mip)
        {
            astcenc_image image;
            image.dim_x = srcImage->GetWidth(mip);
            image.dim_y = srcImage->GetHeight(mip);
            image.dim_z = 1;
            image.data_type = dataType;
                        
            AZ::u8* srcMem;
            AZ::u32 srcPitch;
            srcImage->GetImagePointer(mip, srcMem, srcPitch);
            AZ::u32 srcDataSize = srcImage->GetMipBufSize(mip);
                        
            AZ::u8* dstMem;
            AZ::u32 dstPitch;
            dstImage->GetImagePointer(mip, dstMem, dstPitch);
            image.data = reinterpret_cast<void**>(&dstMem);

            status = astcenc_decompress_image(context, srcMem, srcDataSize, &image, &swizzle, 0);

            if (status != ASTCENC_SUCCESS)
            {
                AZ_Error("Image Processing", false, "ASTCCompressor::DecompressImage failed: %s\n", astcenc_get_error_string(status));
                astcenc_context_free(context);
                return nullptr;
            }
        }

        astcenc_context_free(context);

        return dstImage;
    }
} //namespace ImageProcessingAtom
