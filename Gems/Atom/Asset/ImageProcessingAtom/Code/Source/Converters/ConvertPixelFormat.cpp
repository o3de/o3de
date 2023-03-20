/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/time.h>

#include <Processing/ImageFlags.h>
#include <Processing/ImageObjectImpl.h>
#include <Processing/ImageToProcess.h>
#include <Processing/PixelFormatInfo.h>

#include <Compressors/Compressor.h>
#include <Converters/PixelOperation.h>

///////////////////////////////////////////////////////////////////////////////////
//functions for maintaining alpha coverage.

namespace ImageProcessingAtom
{
    void ImageToProcess::ConvertFormat(EPixelFormat fmtDst)
    {
        //pixel format before convertion
        EPixelFormat fmtSrc = Get()->GetPixelFormat();

        //return directly if the image already has the desired pixel format
        if (fmtDst == fmtSrc)
        {
            return;
        }

        uint32 dwWidth, dwHeight;
        dwWidth = Get()->GetWidth(0);
        dwHeight = Get()->GetHeight(0);

        //if the output image size doesn't work the desired pixel format. set to fallback format
        const PixelFormatInfo* dstFmtInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(fmtDst);
        if (!CPixelFormats::GetInstance().IsImageSizeValid(fmtDst, dwWidth, dwHeight, true))
        {
            AZ_Warning("Image Processing", false, "Output pixel format %d doesn't work with output image size %d x %d",
                fmtDst, dwWidth, dwHeight);

            //fall back to safe texture format
            if (dstFmtInfo->nChannels == 1)
            {
                fmtDst = dstFmtInfo->bHasAlpha ? ePixelFormat_A8 : ePixelFormat_R8;
            }
            else if (dstFmtInfo->nChannels == 2)
            {
                fmtDst = ePixelFormat_R8G8;
            }
            else
            {
                fmtDst = dstFmtInfo->bHasAlpha ? ePixelFormat_R8G8B8A8 : ePixelFormat_R8G8B8X8;
            }
        }

        //convert src image to uncompressed formats if it's compressed format
        bool isSrcUncompressed = CPixelFormats::GetInstance().IsPixelFormatUncompressed(fmtSrc);
        bool isDstUncompressed = CPixelFormats::GetInstance().IsPixelFormatUncompressed(fmtDst);

        if (isSrcUncompressed && isDstUncompressed)
        {//both are uncompressed
            ConvertFormatUncompressed(fmtDst);
        }
        else if (!isSrcUncompressed && !isDstUncompressed)
        { //both are compressed
            AZ_Assert(false, "unusual user case. but we can still handle it");
        }
        else
        {   //one fmt is compressed format
            //use the compressed format to find right compressor
            EPixelFormat compressedFmt = isSrcUncompressed ? fmtDst : fmtSrc;
            EPixelFormat uncompressedFmt = isSrcUncompressed ? fmtSrc : fmtDst;
            ColorSpace sourceColorSpace = Get()->HasImageFlags(EIF_SRGBRead) ? ColorSpace::sRGB : ColorSpace::linear;

            ICompressorPtr compressor = ICompressor::FindCompressor(compressedFmt, sourceColorSpace, isSrcUncompressed);

            if (compressor == nullptr)
            {
                //no avaible compressor for compressed format
                AZ_Warning("Image Processing", false, "No avaliable compressor for pixel format %d", compressedFmt);
                return;
            }

            //check if the uncompressed fmt also supported by the compressor
            EPixelFormat desiredUncompressedFmt = compressor->GetSuggestedUncompressedFormat(compressedFmt, uncompressedFmt);
            if (desiredUncompressedFmt != uncompressedFmt)
            {
                //we need to do intermedia convertion to convert to the temperory format
                ConvertFormat(desiredUncompressedFmt);
                ConvertFormat(fmtDst);
            }
            else
            {
                IImageObjectPtr dstImage = nullptr;
                [[maybe_unused]] const PixelFormatInfo* compressedInfo = CPixelFormats::GetInstance().GetPixelFormatInfo(compressedFmt);
                if (isSrcUncompressed)
                {
                    AZ::u64 startTime = AZStd::GetTimeUTCMilliSecond();
                    dstImage = compressor->CompressImage(Get(), fmtDst, &m_compressOption);
                    AZ::u64 endTime = AZStd::GetTimeUTCMilliSecond();
                    [[maybe_unused]] double processTime = static_cast<double>(endTime - startTime) / 1000.0;
                    if (dstImage)
                    {
                        AZ_TracePrintf("Image Processing", "Image [%dx%d] was compressed to [%s] format by [%s] in %.3f seconds\n",
                            Get()->GetWidth(0), Get()->GetHeight(0), compressedInfo->szName, compressor->GetName(), processTime);
                    }
                }
                else
                {
                    dstImage = compressor->DecompressImage(Get(), fmtDst);
                }

                Set(dstImage);
                
                if (dstImage == nullptr)
                {
                    AZ_Error("Image Processing", false, "Failed to use [%s] to %s [%s] format", compressor->GetName(),
                        isSrcUncompressed ? "compress" : "decompress",
                        compressedInfo->szName);
                }
            }
        }
    }

    void ImageToProcess::ConvertFormatUncompressed(EPixelFormat fmtTo)
    {
        IImageObjectPtr srcImage = m_img;
        EPixelFormat srcFmt = srcImage->GetPixelFormat();
        EPixelFormat dstFmt = fmtTo;

        if (!(CPixelFormats::GetInstance().IsPixelFormatUncompressed(srcFmt)
              && CPixelFormats::GetInstance().IsPixelFormatUncompressed(dstFmt)))
        {
            AZ_Assert(false, "both source and dest images' pixel format need to be uncompressed");
            return;
        }

        IImageObjectPtr dstImage(m_img->AllocateImage(fmtTo));

        AZ_Assert(srcImage->GetPixelCount(0) == dstImage->GetPixelCount(0), "dest image has different size than source image");

        //create pixel operation function for src and dst images
        IPixelOperationPtr srcOp = CreatePixelOperation(srcFmt);
        IPixelOperationPtr dstOp = CreatePixelOperation(dstFmt);

        //get count of bytes per pixel for both src and dst images
        uint32 srcPixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(srcFmt)->bitsPerBlock / 8;
        uint32 dstPixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(dstFmt)->bitsPerBlock / 8;

        const uint32 dwMips = dstImage->GetMipCount();
        float r, g, b, a;
        for (uint32 dwMip = 0; dwMip < dwMips; ++dwMip)
        {
            uint8* srcPixelBuf;
            uint32 srcPitch;
            srcImage->GetImagePointer(dwMip, srcPixelBuf, srcPitch);
            uint8* dstPixelBuf;
            uint32 dstPitch;
            dstImage->GetImagePointer(dwMip, dstPixelBuf, dstPitch);

            const uint32 pixelCount = srcImage->GetPixelCount(dwMip);

            for (uint32 i = 0; i < pixelCount; ++i, srcPixelBuf += srcPixelBytes, dstPixelBuf += dstPixelBytes)
            {
                srcOp->GetRGBA(srcPixelBuf, r, g, b, a);
                dstOp->SetRGBA(dstPixelBuf, r, g, b, a);
            }
        }

        m_img = dstImage;
    }
} // namespace ImageProcessingAtom
