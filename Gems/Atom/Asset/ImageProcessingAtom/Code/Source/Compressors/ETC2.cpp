/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Atom/ImageProcessing/ImageObject.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/ImageFlags.h>
#include <Converters/PixelOperation.h>
#include <Compressors/ETC2.h>

#include <EtcConfig.h>
#include <Etc.h>
#include <EtcImage.h>
#include <EtcColorFloatRGBA.h>

namespace ImageProcessingAtom
{
    //limited to 1 thread because AP requires so. We may change to n when AP allocate n thread to a job in the furture
    static const int  MAX_COMP_JOBS = 1;
    static const int MIN_COMP_JOBS = 1;
    static const float  ETC_LOW_EFFORT_LEVEL = 25.0f;
    static const float  ETC_MED_EFFORT_LEVEL = 40.0f;
    static const float  ETC_HIGH_EFFORT_LEVEL = 80.0f;

    //Grab the Etc2Comp specific pixel format enum
    static Etc::Image::Format FindEtc2PixelFormat(EPixelFormat fmt)
    {
        switch (fmt)
        {
        case ePixelFormat_EAC_RG11:
            return Etc::Image::Format::RG11;
        case ePixelFormat_EAC_R11:
            return Etc::Image::Format::R11;
        case ePixelFormat_ETC2:
            return Etc::Image::Format::RGB8;
        case ePixelFormat_ETC2a1:
            return Etc::Image::Format::RGB8A1;
        case ePixelFormat_ETC2a:
            return Etc::Image::Format::RGBA8;
        default:
            return Etc::Image::Format::FORMATS;
        }
    }

    //Get the errmetric required for the compression
    static Etc::ErrorMetric FindErrMetric(Etc::Image::Format fmt)
    {
        switch (fmt)
        {
        case Etc::Image::Format::RG11:
            return Etc::ErrorMetric::NORMALXYZ;
        case Etc::Image::Format::R11:
            return Etc::ErrorMetric::NUMERIC;
        case Etc::Image::Format::RGB8:
            return Etc::ErrorMetric::RGBX;
        case  Etc::Image::Format::RGBA8:
        case  Etc::Image::Format::RGB8A1:
            return Etc::ErrorMetric::RGBA;
        default:
            return Etc::ErrorMetric::ERROR_METRICS;
        }
    }

    //Convert to sRGB format
    static Etc::Image::Format FindGammaEtc2PixelFormat(Etc::Image::Format fmt)
    {
        switch (fmt)
        {
        case Etc::Image::Format::RGB8:
            return Etc::Image::Format::SRGB8;
        case Etc::Image::Format::RGBA8:
            return Etc::Image::Format::SRGBA8;
        case Etc::Image::Format::RGB8A1:
            return Etc::Image::Format::SRGB8A1;
        default:
            return Etc::Image::Format::FORMATS;
        }
    }

    bool ETC2Compressor::IsCompressedPixelFormatSupported(EPixelFormat fmt)
    {
        return (FindEtc2PixelFormat(fmt) != Etc::Image::Format::FORMATS);
    }

    bool ETC2Compressor::IsUncompressedPixelFormatSupported(EPixelFormat fmt)
    {
        //for uncompress format
        if (fmt == ePixelFormat_R8G8B8A8)
        {
            return true;
        }
        return false;
    }

    EPixelFormat ETC2Compressor::GetSuggestedUncompressedFormat([[maybe_unused]] EPixelFormat compressedfmt, [[maybe_unused]] EPixelFormat uncompressedfmt) const
    {
        return ePixelFormat_R8G8B8A8;
    }

    bool ETC2Compressor::DoesSupportDecompress([[maybe_unused]] EPixelFormat fmtDst)
    {
        return false;
    }

    ColorSpace ETC2Compressor::GetSupportedColorSpace([[maybe_unused]] EPixelFormat compressFormat) const
    {
        return ColorSpace::autoSelect;
    }
        
    const char* ETC2Compressor::GetName() const
    {
        return "ETC2Compressor";
    }

    IImageObjectPtr ETC2Compressor::CompressImage(IImageObjectPtr srcImage, EPixelFormat fmtDst,
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
        ICompressor::EQuality quality = ICompressor::eQuality_Normal;
        //get setting from compression option
        if (compressOption)
        {
            quality = compressOption->compressQuality;
        }

        float qualityEffort = 0.0f;
        switch (quality)
        {
        case eQuality_Preview:
        case eQuality_Fast:
        {
            qualityEffort = ETC_LOW_EFFORT_LEVEL;
            break;
        }
        case eQuality_Normal:
        {
            qualityEffort = ETC_MED_EFFORT_LEVEL;
            break;
        }
        default:
        {
            qualityEffort = ETC_HIGH_EFFORT_LEVEL;
        }
        }

        Etc::Image::Format dstEtc2Format = FindEtc2PixelFormat(fmtDst);
        if (srcImage->GetImageFlags() & EIF_SRGBRead)
        {
            dstEtc2Format = FindGammaEtc2PixelFormat(dstEtc2Format);
        }

        //use to read pixel data from src image
        IPixelOperationPtr pixelOp = CreatePixelOperation(fmtSrc);
        //get count of bytes per pixel for images
        AZ::u32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(fmtSrc)->bitsPerBlock / 8;

        const AZ::u32 mipCount = dstImage->GetMipCount();
        for (AZ::u32 mip = 0; mip < mipCount; ++mip)
        {
            const AZ::u32 width = srcImage->GetWidth(mip);
            const AZ::u32 height = srcImage->GetHeight(mip);

            // Prepare source data
            AZ::u8* srcMem;
            AZ::u32 srcPitch;
            srcImage->GetImagePointer(mip, srcMem, srcPitch);
            const AZ::u32 pixelCount = srcImage->GetPixelCount(mip);

            Etc::ColorFloatRGBA* rgbaPixels = new Etc::ColorFloatRGBA[pixelCount];
            Etc::ColorFloatRGBA* rgbaPixelPtr = rgbaPixels;
            float r, g, b, a;
            for (AZ::u32 pixelIdx = 0; pixelIdx < pixelCount; pixelIdx++, srcMem += pixelBytes, rgbaPixelPtr++)
            {
                pixelOp->GetRGBA(srcMem, r, g, b, a);
                rgbaPixelPtr->fA = a;
                rgbaPixelPtr->fR = r;
                rgbaPixelPtr->fG = g;
                rgbaPixelPtr->fB = b;
            }

            //Call into etc2Comp lib to compress. https://medium.com/@duhroach/building-a-blazing-fast-etc2-compressor-307f3e9aad99
            Etc::ErrorMetric errMetric = FindErrMetric(dstEtc2Format);
            unsigned char* paucEncodingBits;
            unsigned int uiEncodingBitsBytes;
            unsigned int uiExtendedWidth;
            unsigned int uiExtendedHeight;
            int iEncodingTime_ms;

            Etc::Encode(reinterpret_cast<float*>(rgbaPixels),
                width, height,
                dstEtc2Format,
                errMetric,
                qualityEffort,
                MIN_COMP_JOBS,
                MAX_COMP_JOBS,
                &paucEncodingBits, &uiEncodingBitsBytes,
                &uiExtendedWidth, &uiExtendedHeight,
                &iEncodingTime_ms);

            AZ::u8* dstMem;
            AZ::u32 dstPitch;
            dstImage->GetImagePointer(mip, dstMem, dstPitch);

            memcpy(dstMem, paucEncodingBits, uiEncodingBitsBytes);
            delete[] rgbaPixels;
        }

        return dstImage;
    }

    IImageObjectPtr ETC2Compressor::DecompressImage(IImageObjectPtr srcImage, [[maybe_unused]] EPixelFormat fmtDst) const
    {
        //etc2Comp doesn't support decompression
        //Since PVRTexLib support ETC formats too. It may take over the decompression.
        return nullptr;
    }
} // namespace ImageProcessingAtom
