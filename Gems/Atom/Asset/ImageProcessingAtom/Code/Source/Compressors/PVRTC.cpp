/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ImageProcessing_precompiled.h>
#include <ImageProcessing_Traits_Platform.h>
#include <Atom/ImageProcessing/ImageObject.h>
#include <Processing/ImageFlags.h>
#include <Processing/PixelFormatInfo.h>
#include <Compressors/PVRTC.h>

#if AZ_TRAIT_IMAGEPROCESSING_PVRTEXLIB_USE_WINDLL_IMPORT
//_WINDLL_IMPORT need to be defined before including PVRTexLib header files to avoid linking error on windows.
#define _WINDLL_IMPORT
// NOMINMAX needs to be defined before including PVRTexLib header files (which include Windows.h)
// so that Windows.h doesn't define min/max. Otherwise, a compile error may arise in Uber builds
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <PVRTexture.h>
#include <PVRTextureUtilities.h>


namespace ImageProcessingAtom
{
    // Note: PVRTexLib supports ASTC formats, ETC formats, PVRTC formats and BC formats
    // We haven't tested the performace to compress BC formats compare to CTSquisher
    // For PVRTC formats, we only added PVRTC 1 support for now
    // The compression for ePVRTPF_EAC_R11 and ePVRTPF_EAC_RG11 are very slow. It takes 7 and 14 minutes for a 2048x2048 texture.
    EPVRTPixelFormat FindPvrPixelFormat(EPixelFormat fmt)
    {
        switch (fmt)
        {
        case ePixelFormat_ASTC_4x4:
            return ePVRTPF_ASTC_4x4;
        case ePixelFormat_ASTC_5x4:
            return ePVRTPF_ASTC_5x4;
        case ePixelFormat_ASTC_5x5:
            return ePVRTPF_ASTC_5x5;
        case ePixelFormat_ASTC_6x5:
            return ePVRTPF_ASTC_6x5;
        case ePixelFormat_ASTC_6x6:
            return ePVRTPF_ASTC_6x6;
        case ePixelFormat_ASTC_8x5:
            return ePVRTPF_ASTC_8x5;
        case ePixelFormat_ASTC_8x6:
            return ePVRTPF_ASTC_8x6;
        case ePixelFormat_ASTC_8x8:
            return ePVRTPF_ASTC_8x8;
        case ePixelFormat_ASTC_10x5:
            return ePVRTPF_ASTC_10x5;
        case ePixelFormat_ASTC_10x6:
            return ePVRTPF_ASTC_10x6;
        case ePixelFormat_ASTC_10x8:
            return ePVRTPF_ASTC_10x8;
        case ePixelFormat_ASTC_10x10:
            return ePVRTPF_ASTC_10x10;
        case ePixelFormat_ASTC_12x10:
            return ePVRTPF_ASTC_12x10;
        case ePixelFormat_ASTC_12x12:
            return ePVRTPF_ASTC_12x12;
        case ePixelFormat_PVRTC2:
            return ePVRTPF_PVRTCI_2bpp_RGBA;
        case ePixelFormat_PVRTC4:
            return ePVRTPF_PVRTCI_4bpp_RGBA;
        case ePixelFormat_EAC_R11:
            return ePVRTPF_EAC_R11;
        case ePixelFormat_EAC_RG11:
            return ePVRTPF_EAC_RG11;
        case ePixelFormat_ETC2:
            return ePVRTPF_ETC2_RGB;
        case ePixelFormat_ETC2a1:
            return ePVRTPF_ETC2_RGB_A1;
        case ePixelFormat_ETC2a:
            return ePVRTPF_ETC2_RGBA;
        default:
            return ePVRTPF_NumCompressedPFs;
        }
    }

    bool PVRTCCompressor::IsCompressedPixelFormatSupported(EPixelFormat fmt)
    {
        return (FindPvrPixelFormat(fmt) != ePVRTPF_NumCompressedPFs);
    }

    bool PVRTCCompressor::IsUncompressedPixelFormatSupported(EPixelFormat fmt)
    {
        //for uncompress format
        if (fmt == ePixelFormat_R8G8B8A8)
        {
            return true;
        }
        return false;
    }

    EPixelFormat PVRTCCompressor::GetSuggestedUncompressedFormat([[maybe_unused]] EPixelFormat compressedfmt, [[maybe_unused]] EPixelFormat uncompressedfmt) const
    {
        return ePixelFormat_R8G8B8A8;
    }

    ColorSpace PVRTCCompressor::GetSupportedColorSpace([[maybe_unused]] EPixelFormat compressFormat) const
    {
        return ColorSpace::autoSelect;
    }

    bool PVRTCCompressor::DoesSupportDecompress([[maybe_unused]] EPixelFormat fmtDst)
    {
        return true;
    }

    IImageObjectPtr PVRTCCompressor::CompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst,
        const CompressOption* compressOption) const
    {
        //validate input
        EPixelFormat fmtSrc = srcImage->GetPixelFormat();

        //src format need to be uncompressed and dst format need to compressed.
        if (!IsUncompressedPixelFormatSupported(fmtSrc) || !IsCompressedPixelFormatSupported(fmtDst))
        {
            return nullptr;
        }

        IImageObjectPtr dstImage(srcImage->AllocateImage(fmtDst));

        //determinate compression quality
        pvrtexture::ECompressorQuality internalQuality = pvrtexture::eETCFast;
        ICompressor::EQuality quality = ICompressor::eQuality_Normal;
        AZ::Vector3 uniformWeights = AZ::Vector3(0.3333f, 0.3334f, 0.3333f);
        AZ::Vector3 weights = uniformWeights;
        bool isUniform = true;
        //get setting from compression option
        if (compressOption)
        {
            quality = compressOption->compressQuality;
            weights = compressOption->rgbWeight;
            isUniform = (weights == uniformWeights);
        }

        if (IsETCFormat(fmtDst))
        {
            if ((quality <= eQuality_Normal) && isUniform)
            {
                internalQuality = pvrtexture::eETCFast;
            }
            else if (quality <= eQuality_Normal)
            {
                internalQuality = pvrtexture::eETCNormal;
            }
            else if (isUniform)
            {
                internalQuality = pvrtexture::eETCSlow;
            }
            else
            {
                internalQuality = pvrtexture::eETCSlow;
            }
        }
        else if (IsASTCFormat(fmtDst))
        {
            if (quality == eQuality_Preview)
            {
                internalQuality = pvrtexture::eASTCVeryFast;
            }
            else if (quality == eQuality_Fast)
            {
                internalQuality = pvrtexture::eASTCFast;
            }
            else if (quality == eQuality_Normal)
            {
                internalQuality = pvrtexture::eASTCMedium;
            }
            else
            {
                internalQuality = pvrtexture::eASTCThorough;
            }
        }
        else
        {
            if (quality == eQuality_Preview)
            {
                internalQuality = pvrtexture::ePVRTCFastest;
            }
            else if (quality == eQuality_Fast)
            {
                internalQuality = pvrtexture::ePVRTCFast;
            }
            else if (quality == eQuality_Normal)
            {
                internalQuality = pvrtexture::ePVRTCNormal;
            }
            else
            {
                internalQuality = pvrtexture::ePVRTCHigh;
            }
        }

        // setup color space
        EPVRTColourSpace cspace = ePVRTCSpacelRGB;
        if (srcImage->GetImageFlags() & EIF_SRGBRead)
        {
            cspace = ePVRTCSpacesRGB;
        }

        //setup src texture for compression
        const pvrtexture::PixelType srcPixelType('r', 'g', 'b', 'a', 8, 8, 8, 8);
        const AZ::u32 dstMips = dstImage->GetMipCount();
        for (AZ::u32 mip = 0; mip < dstMips; ++mip)
        {
            const AZ::u32 width = srcImage->GetWidth(mip);
            const AZ::u32 height = srcImage->GetHeight(mip);

            // Prepare source data
            AZ::u8* srcMem;
            uint32 srcPitch;
            srcImage->GetImagePointer(mip, srcMem, srcPitch);

            const pvrtexture::CPVRTextureHeader srcHeader(
                srcPixelType.PixelTypeID,           // AZ::u64            u64PixelFormat,
                width,                              // uint32            u32Height=1,
                height,                             // uint32            u32Width=1,
                1,                                  // uint32            u32Depth=1,
                1,                                  // uint32            u32NumMipMaps=1,
                1,                                  // uint32            u32NumArrayMembers=1,
                1,                                  // uint32            u32NumFaces=1,
                cspace,                             // EPVRTColourSpace  eColourSpace=ePVRTCSpacelRGB,
                ePVRTVarTypeUnsignedByteNorm,       // EPVRTVariableType eChannelType=ePVRTVarTypeUnsignedByteNorm,
                false);                             // bool              bPreMultiplied=false);

            pvrtexture::CPVRTexture compressTexture(srcHeader, srcMem);

            //compressing
            bool isSuccess = false;
#if AZ_TRAIT_IMAGEPROCESSING_SUPPORT_TRY_CATCH
            try
#endif // AZ_TRAIT_IMAGEPROCESSING_SUPPORT_TRY_CATCH
            {
                isSuccess = pvrtexture::Transcode(
                    compressTexture,
                    pvrtexture::PixelType(FindPvrPixelFormat(fmtDst)),
                    ePVRTVarTypeUnsignedByteNorm,
                    cspace,
                    internalQuality);
            }
#if AZ_TRAIT_IMAGEPROCESSING_SUPPORT_TRY_CATCH
            catch (...)
            {
                AZ_Error("Image Processing", false, "Unknown exception in PVRTexLib");
                return nullptr;
            }
#endif // AZ_TRAIT_IMAGEPROCESSING_SUPPORT_TRY_CATCH

            if (!isSuccess)
            {
                AZ_Error("Image Processing", false, "Failed to compress image with PVRTexLib. You may not have astcenc.exe for compressing ASTC formates");
                return nullptr;
            }

            // Getting compressed data
            const void* const compressedData = compressTexture.getDataPtr();
            if (!compressedData)
            {
                AZ_Error("Image Processing", false, "Failed to obtain compressed image data by using PVRTexLib");
                return nullptr;
            }

            const AZ::u32 compressedDataSize = compressTexture.getDataSize();
            if (dstImage->GetMipBufSize(mip) != compressedDataSize)
            {
                AZ_Error("Image Processing", false, "Compressed image data size mismatch while using PVRTexLib");
                return nullptr;
            }

            //save compressed data to dst image
            AZ::u8* dstMem;
            AZ::u32 dstPitch;
            dstImage->GetImagePointer(mip, dstMem, dstPitch);
            memcpy(dstMem, compressedData, compressedDataSize);
        }

        return dstImage;
    }

    IImageObjectPtr PVRTCCompressor::DecompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst) const
    {
        //validate input
        EPixelFormat fmtSrc = srcImage->GetPixelFormat(); //compressed

        if (!IsCompressedPixelFormatSupported(fmtSrc) || !IsUncompressedPixelFormatSupported(fmtDst))
        {
            return nullptr;
        }

        EPVRTColourSpace colorSpace = ePVRTCSpacelRGB;
        if (srcImage->GetImageFlags() & EIF_SRGBRead)
        {
            colorSpace = ePVRTCSpacesRGB;
        }

        IImageObjectPtr dstImage(srcImage->AllocateImage(fmtDst));

        const AZ::u32 mipCount = dstImage->GetMipCount();
        for (AZ::u32 mip = 0; mip < mipCount; ++mip)
        {
            const AZ::u32 width = srcImage->GetWidth(mip);
            const AZ::u32 height = srcImage->GetHeight(mip);

            // Preparing source compressed data
            const pvrtexture::CPVRTextureHeader compressedHeader(
                FindPvrPixelFormat(fmtSrc),         // AZ::u64         u64PixelFormat,
                width,                              // uint32            u32Height=1,
                height,                             // uint32            u32Width=1,
                1,                                  // uint32            u32Depth=1,
                1,                                  // uint32            u32NumMipMaps=1,
                1,                                  // uint32            u32NumArrayMembers=1,
                1,                                  // uint32            u32NumFaces=1,
                colorSpace,                         // EPVRTColourSpace  eColourSpace=ePVRTCSpacelRGB,
                ePVRTVarTypeUnsignedByteNorm,       // EPVRTVariableType eChannelType=ePVRTVarTypeUnsignedByteNorm,
                false);                             // bool              bPreMultiplied=false);

            const AZ::u32 compressedDataSize = compressedHeader.getDataSize();
            if (srcImage->GetMipBufSize(mip) != compressedDataSize)
            {
                AZ_Error("Image Processing", false, "Decompressed image data size mismatch while using PVRTexLib");
                return nullptr;
            }

            AZ::u8* srcMem;
            AZ::u32 srcPitch;
            srcImage->GetImagePointer(mip, srcMem, srcPitch);
            pvrtexture::CPVRTexture cTexture(compressedHeader, srcMem);

            // Decompress
            bool bOk = false;
#if AZ_TRAIT_IMAGEPROCESSING_SUPPORT_TRY_CATCH
            try
            {
#endif // AZ_TRAIT_IMAGEPROCESSING_SUPPORT_TRY_CATCH
                bOk = pvrtexture::Transcode(
                    cTexture,
                    pvrtexture::PVRStandard8PixelType,
                    ePVRTVarTypeUnsignedByteNorm,
                    colorSpace,
                    pvrtexture::ePVRTCHigh);

#if AZ_TRAIT_IMAGEPROCESSING_SUPPORT_TRY_CATCH
            }
            catch (...)
            {
                AZ_Error("Image Processing", false, "Unknown exception in PVRTexLib when decompressing");
                return nullptr;
            }
#endif // AZ_TRAIT_IMAGEPROCESSING_SUPPORT_TRY_CATCH

            if (!bOk)
            {
                AZ_Error("Image Processing", false, "Failed to decompress an image by using PVRTexLib");
                return nullptr;
            }

            // Getting decompressed data
            const void* const pDecompressedData = cTexture.getDataPtr();
            if (!pDecompressedData)
            {
                AZ_Error("Image Processing", false, "Failed to obtain decompressed image data by using PVRTexLib");
                return nullptr;
            }

            const AZ::u32 decompressedDataSize = cTexture.getDataSize();
            if (dstImage->GetMipBufSize(mip) != decompressedDataSize)
            {
                AZ_Error("Image Processing", false, "Decompressed image data size mismatch while using PVRTexLib");
                return nullptr;
            }

            //save decompressed image to dst image
            AZ::u8* dstMem;
            AZ::u32 dstPitch;
            dstImage->GetImagePointer(mip, dstMem, dstPitch);
            memcpy(dstMem, pDecompressedData, decompressedDataSize);
        }

        return dstImage;
    }
} //namespace ImageProcessingAtom
