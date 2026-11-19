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
#include <AzCore/Math/MathUtils.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

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
        // ASTC requires four components per uncompressed pixel.
        switch (fmt)
        {
        case ePixelFormat_R8G8B8A8:
        case ePixelFormat_R8G8B8X8:
        case ePixelFormat_R16G16B16A16F:
        case ePixelFormat_R32G32B32A32F:
            return true;
        default:
            return false;
        }
    }

    EPixelFormat ASTCCompressor::GetSuggestedUncompressedFormat(
        [[maybe_unused]] EPixelFormat compressedfmt,
        EPixelFormat uncompressedfmt) const
    {
        if (IsUncompressedPixelFormatSupported(uncompressedfmt))
        {
            return uncompressedfmt;
        }

        const auto* formatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(uncompressedfmt);
        switch (formatInfo->eSampleType)
        {
        case ESampleType::eSampleType_Half:
            return ePixelFormat_R16G16B16A16F;
        case ESampleType::eSampleType_Float:
            return ePixelFormat_R32G32B32A32F;
        default:
            return ePixelFormat_R8G8B8A8;
        }
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

    static astcenc_profile GetAstcProfile(bool isSrgb, bool isHdr)
    {
        if (isHdr)
        {
            if (isSrgb)
            {
                return ASTCENC_PRF_HDR_RGB_LDR_A;
            }

            return ASTCENC_PRF_HDR;
        }

        if (isSrgb)
        {
            return ASTCENC_PRF_LDR_SRGB;
        }

        return ASTCENC_PRF_LDR;
    }

    static astcenc_type GetAstcDataType(EPixelFormat pixelFormat)
    {
        const auto* formatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(pixelFormat);
        switch (formatInfo->eSampleType)
        {
        case ESampleType::eSampleType_Uint8:
            return ASTCENC_TYPE_U8;
        case ESampleType::eSampleType_Half:
            return ASTCENC_TYPE_F16;
        case ESampleType::eSampleType_Float:
            return ASTCENC_TYPE_F32;
        default:
            AZ_Assert(false, "Unsupported uncompressed format %s", formatInfo->szName);
            return ASTCENC_TYPE_U8;
        }
    }

    static float GetAstcCompressQuality(ICompressor::EQuality quality)
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

    using AstcContextPtr = AZStd::unique_ptr<astcenc_context, decltype(&astcenc_context_free)>;

    static AstcContextPtr CreateAstcContext(
        astcenc_profile profile,
        const PixelFormatInfo& format,
        float quality,
        AZ::u32 flags,
        AZ::u32 threadCount)
    {
        astcenc_config config;
        astcenc_error status = astcenc_config_init(profile, format.blockWidth, format.blockHeight, 1, quality, flags, &config);
        if (status != ASTCENC_SUCCESS)
        {
            AZ_Error("Image Processing", false, "astcenc_config_init failed: %s", astcenc_get_error_string(status));
            return AstcContextPtr(nullptr, astcenc_context_free);
        }

        astcenc_context* context = nullptr;
        status = astcenc_context_alloc(&config, threadCount, &context, nullptr);
        if (status != ASTCENC_SUCCESS)
        {
            AZ_Error("Image Processing", false, "astcenc_context_alloc failed: %s", astcenc_get_error_string(status));
            return AstcContextPtr(nullptr, astcenc_context_free);
        }

        return AstcContextPtr(context, astcenc_context_free);
    }

    static AZ::u32 GetAstcWorkChunkCount(const PixelFormatInfo& format, AZ::u32 width, AZ::u32 height)
    {
        // astcenc 5.7.1 assigns compression work in groups of 16 blocks.
        constexpr AZ::u32 blocksPerChunk = 16;
        const AZ::u32 blockCount = AZ::DivideAndRoundUp(width, format.blockWidth)
            * AZ::DivideAndRoundUp(height, format.blockHeight);
        return AZ::DivideAndRoundUp(blockCount, blocksPerChunk);
    }

    IImageObjectPtr ASTCCompressor::CompressImage(
        IImageObjectPtr srcImage, EPixelFormat fmtDst, const CompressOption* compressOption) const
    {
        if (!srcImage || srcImage->GetMipCount() == 0 || !IsCompressedPixelFormatSupported(fmtDst))
        {
            return nullptr;
        }

        EPixelFormat fmtSrc = srcImage->GetPixelFormat();
        if (!IsUncompressedPixelFormatSupported(fmtSrc))
        {
            return nullptr;
        }

        EQuality quality = eQuality_Normal;
        astcenc_swizzle swizzle{ ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };
        if (compressOption)
        {
            quality = compressOption->compressQuality;
            if (compressOption->discardAlpha)
            {
                swizzle.a = ASTCENC_SWZ_1;
            }
        }

        AZ::u32 flags = 0;
        if (srcImage->HasImageFlags(EIF_RenormalizedTexture))
        {
            // The normal map swizzle only reads R and G. Byte formats already have the right values.
            if (fmtSrc != ePixelFormat_R8G8B8A8 && fmtSrc != ePixelFormat_R8G8B8X8)
            {
                ImageToProcess imageToProcess(srcImage);
                imageToProcess.ConvertFormatUncompressed(ePixelFormat_R8G8B8X8);
                srcImage = imageToProcess.Get();
                fmtSrc = srcImage->GetPixelFormat();
            }

            flags = ASTCENC_FLG_MAP_NORMAL;
            swizzle = astcenc_swizzle{ ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_R, ASTCENC_SWZ_G };
        }

        IImageObjectPtr dstImage(srcImage->AllocateImage(fmtDst));
        if (!dstImage)
        {
            return nullptr;
        }

        const auto* dstFormatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(fmtDst);
        const AZ::u32 topMipChunks = GetAstcWorkChunkCount(*dstFormatInfo, srcImage->GetWidth(0), srcImage->GetHeight(0));
        const AZ::u32 threadCount = AZStd::max(1u, AZStd::min(AZStd::thread::hardware_concurrency() / 2, topMipChunks));

        const astcenc_profile profile = GetAstcProfile(srcImage->HasImageFlags(EIF_SRGBRead), srcImage->HasImageFlags(EIF_HDR));
        const auto context = CreateAstcContext(profile, *dstFormatInfo, GetAstcCompressQuality(quality), flags, threadCount);
        if (!context)
        {
            return nullptr;
        }

        const astcenc_type dataType = GetAstcDataType(fmtSrc);
        const AZ::u32 dstMips = dstImage->GetMipCount();
        AZStd::vector<astcenc_error> workerStatus;

        for (AZ::u32 mip = 0; mip < dstMips; ++mip)
        {
            AZ::u8* srcMem;
            AZ::u32 srcPitch;
            srcImage->GetImagePointer(mip, srcMem, srcPitch);
            void* slice = srcMem;
            astcenc_image image{ srcImage->GetWidth(mip), srcImage->GetHeight(mip), 1, dataType, &slice };

            AZ::u8* dstMem;
            AZ::u32 dstPitch;
            dstImage->GetImagePointer(mip, dstMem, dstPitch);
            const AZ::u32 dataSize = dstImage->GetMipBufSize(mip);

            const AZ::u32 mipChunks = GetAstcWorkChunkCount(*dstFormatInfo, image.dim_x, image.dim_y);
            const AZ::u32 mipThreadCount = AZStd::max(1u, AZStd::min(threadCount, mipChunks));

            astcenc_error status;
            if (mipThreadCount == 1)
            {
                status = astcenc_compress_image(context.get(), &image, &swizzle, dstMem, dataSize, 0);
            }
            else
            {
                AZ::Job* currentJob = AZ::JobContext::GetGlobalContext()->GetJobManager().GetCurrentJob();
                AZStd::optional<AZ::JobCompletion> completionJob;
                if (!currentJob)
                {
                    completionJob.emplace();
                }

                workerStatus.assign(mipThreadCount, ASTCENC_SUCCESS);
                astcenc_context* const codecContext = context.get();
                for (AZ::u32 threadIdx = 1; threadIdx < mipThreadCount; ++threadIdx)
                {
                    const auto jobLambda = [&workerStatus, codecContext, &image, &swizzle, dstMem, dataSize, threadIdx]()
                    {
                        workerStatus[threadIdx] = astcenc_compress_image(codecContext, &image, &swizzle, dstMem, dataSize, threadIdx);
                    };

                    AZ::Job* workerJob = AZ::CreateJobFunction(AZStd::move(jobLambda), true);
                    if (currentJob)
                    {
                        currentJob->StartAsChild(workerJob);
                    }
                    else
                    {
                        workerJob->SetDependent(&*completionJob);
                        workerJob->Start();
                    }
                }

                workerStatus[0] = astcenc_compress_image(context.get(), &image, &swizzle, dstMem, dataSize, 0);
                if (currentJob)
                {
                    currentJob->WaitForChildren();
                }
                else
                {
                    completionJob->StartAndWaitForCompletion();
                }

                status = ASTCENC_SUCCESS;
                for (astcenc_error workerError : workerStatus)
                {
                    if (workerError != ASTCENC_SUCCESS)
                    {
                        status = workerError;
                        break;
                    }
                }
            }

            if (status != ASTCENC_SUCCESS)
            {
                AZ_Error("Image Processing", false, "ASTCCompressor::CompressImage failed: %s\n", astcenc_get_error_string(status));
                return nullptr;
            }

            // The codec tracks completed work until the context is reset.
            status = astcenc_compress_reset(context.get());
            if (status != ASTCENC_SUCCESS)
            {
                AZ_Error("Image Processing", false, "astcenc_compress_reset failed: %s", astcenc_get_error_string(status));
                return nullptr;
            }
        }

        return dstImage;
    }

    static bool HasHdrAstcBlocks(const IImageObject& image, astcenc_context* hdrContext)
    {
        constexpr AZ::u32 blockSize = 16;

        // astcenc_get_block_info leaves is_hdr_block clear for constant blocks. Their dynamic range is stored in bit 9.
        constexpr AZ::u8 constantBlockHdrFlag = 1 << 1;
        astcenc_block_info blockInfo;

        for (AZ::u32 mip = 0; mip < image.GetMipCount(); ++mip)
        {
            AZ::u8* data = nullptr;
            AZ::u32 pitch = 0;
            image.GetImagePointer(mip, data, pitch);
            const AZ::u32 blockCount = image.GetMipBufSize(mip) / blockSize;
            for (AZ::u32 blockIndex = 0; blockIndex < blockCount; ++blockIndex)
            {
                const AZ::u8* block = data + blockIndex * blockSize;
                if (astcenc_get_block_info(hdrContext, block, &blockInfo) == ASTCENC_SUCCESS
                    && (blockInfo.is_hdr_block || (blockInfo.is_constant_block && (block[1] & constantBlockHdrFlag))))
                {
                    return true;
                }
            }
        }

        return false;
    }

    IImageObjectPtr ASTCCompressor::DecompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst) const
    {
        if (!srcImage || srcImage->GetMipCount() == 0)
        {
            return nullptr;
        }

        const EPixelFormat fmtSrc = srcImage->GetPixelFormat();
        if (!IsCompressedPixelFormatSupported(fmtSrc) || !IsUncompressedPixelFormatSupported(fmtDst))
        {
            return nullptr;
        }

        astcenc_swizzle swizzle{ ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };
        if (srcImage->HasImageFlags(EIF_RenormalizedTexture))
        {
            swizzle = astcenc_swizzle{ ASTCENC_SWZ_R, ASTCENC_SWZ_A, ASTCENC_SWZ_Z, ASTCENC_SWZ_1 };
        }

        const auto* srcFormatInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(fmtSrc);
        const bool isSrgb = srcImage->HasImageFlags(EIF_SRGBRead);
        const bool hasHdrFlag = srcImage->HasImageFlags(EIF_HDR);

        // Legacy decoding accepted HDR for RGBX8 and float output. RGBA8 remains the fast preview path.
        const bool checkLegacyHdr = isSrgb && !hasHdrFlag && fmtDst != ePixelFormat_R8G8B8A8;
        // Linear HDR can decode linear LDR blocks. Reloaded sRGB assets can lack the HDR flag.
        const astcenc_profile profile = GetAstcProfile(isSrgb, !isSrgb || hasHdrFlag || checkLegacyHdr);
        auto context = CreateAstcContext(profile, *srcFormatInfo, ASTCENC_PRE_MEDIUM, ASTCENC_FLG_DECOMPRESS_ONLY, 1);
        if (!context)
        {
            return nullptr;
        }

        if (checkLegacyHdr && !HasHdrAstcBlocks(*srcImage, context.get()))
        {
            context.reset();
            context = CreateAstcContext(ASTCENC_PRF_LDR_SRGB, *srcFormatInfo, ASTCENC_PRE_MEDIUM, ASTCENC_FLG_DECOMPRESS_ONLY, 1);
            if (!context)
            {
                return nullptr;
            }
        }

        IImageObjectPtr dstImage(srcImage->AllocateImage(fmtDst));
        if (!dstImage)
        {
            return nullptr;
        }

        const astcenc_type dataType = GetAstcDataType(fmtDst);
        const AZ::u32 dstMips = dstImage->GetMipCount();
        for (AZ::u32 mip = 0; mip < dstMips; ++mip)
        {
            AZ::u8* srcMem;
            AZ::u32 srcPitch;
            srcImage->GetImagePointer(mip, srcMem, srcPitch);
            const AZ::u32 srcDataSize = srcImage->GetMipBufSize(mip);

            AZ::u8* dstMem;
            AZ::u32 dstPitch;
            dstImage->GetImagePointer(mip, dstMem, dstPitch);
            void* slice = dstMem;
            astcenc_image image{ srcImage->GetWidth(mip), srcImage->GetHeight(mip), 1, dataType, &slice };

            const astcenc_error status = astcenc_decompress_image(context.get(), srcMem, srcDataSize, &image, &swizzle, 0);
            if (status != ASTCENC_SUCCESS)
            {
                AZ_Error("Image Processing", false, "ASTCCompressor::DecompressImage failed: %s\n", astcenc_get_error_string(status));
                return nullptr;
            }
        }

        return dstImage;
    }
} // namespace ImageProcessingAtom
