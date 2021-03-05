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
#include <Processing/ImageToProcess.h>
#include <Processing/ImageConvert.h>
#include <Processing/PixelFormatInfo.h>
#include <Converters/FIR-Windows.h>
#include <Converters/PixelOperation.h>

namespace ImageProcessing
{

    // higher mip level is subtracted by lower mip level when applying the [cheap] high pass filter
    void ImageToProcess::CreateHighPass(AZ::u32 dwMipDown)
    {
        //no need to convert if mip go down 0
        if (dwMipDown == 0)
        {
            return;
        }

        const EPixelFormat ePixelFormat = m_img->GetPixelFormat();

        if (ePixelFormat != ePixelFormat_R32G32B32A32F)
        {
            AZ_Assert(false, "You need convert the orginal image to ePixelFormat_R32G32B32A32F before call this function");
            return;
        }

        
        AZ::u32 dwWidth, dwHeight, dwMips;
        dwWidth = m_img->GetWidth(0);
        dwHeight = m_img->GetHeight(0);
        dwMips = m_img->GetMipCount();

        if (dwMipDown >= dwMips)
        {
            AZ_Warning("Image Processing", false, "CreateHighPass can't go down %i MIP levels for high pass as there are not\
 enough MIP levels available, going down by %i instead", dwMipDown, dwMips - 1);
            dwMipDown = dwMips - 1;
        }

        IImageObjectPtr newImage(IImageObject::CreateImage(dwWidth, dwHeight, dwMips, ePixelFormat));
        newImage->CopyPropertiesFrom(m_img);

        IPixelOperationPtr pixelOp = CreatePixelOperation(ePixelFormat);
        AZ::u32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(ePixelFormat)->bitsPerBlock / 8;

        AZ::u32 dstMips = newImage->GetMipCount();
        for (AZ::u32 dstMip = 0; dstMip < dwMipDown; ++dstMip)
        {
            // linear interpolation
            FilterImage(MipGenType::triangle, MipGenEvalType::sum, 0.0f, 0.0f, m_img, dwMipDown, newImage, dstMip, NULL, NULL);

            const AZ::u32 pixelCountIn = m_img->GetWidth(dstMip) *m_img->GetHeight(dstMip);
            const AZ::u32 pixelCountOut = newImage->GetWidth(dstMip) * newImage->GetHeight(dstMip);

            //substraction
            AZ::u8* srcPixelBuf;
            AZ::u32 srcPitch;
            m_img->GetImagePointer(dstMip, srcPixelBuf, srcPitch);
            AZ::u8* dstPixelBuf;
            AZ::u32 dstPitch;
            newImage->GetImagePointer(dstMip, dstPixelBuf, dstPitch);
            const AZ::u32 pixelCount = newImage->GetPixelCount(dstMip);

            for (AZ::u32 i = 0; i < pixelCount; ++i, srcPixelBuf += pixelBytes, dstPixelBuf += pixelBytes)
            {
                float r1, g1, b1, a1, r2, g2, b2, a2;
                pixelOp->GetRGBA(srcPixelBuf, r1, g1, b1, a1);
                pixelOp->GetRGBA(dstPixelBuf, r2, g2, b2, a2);

                r2 = AZ::GetClamp<float>(r1 - r2 + 0.5f, 0.0f, 1.0f);
                g2 = AZ::GetClamp<float>(g1 - g2 + 0.5f, 0.0f, 1.0f);
                b2 = AZ::GetClamp<float>(b1 - b2 + 0.5f, 0.0f, 1.0f);
                a2 = AZ::GetClamp<float>(a1 - a2 + 0.5f, 0.0f, 1.0f);
                pixelOp->SetRGBA(dstPixelBuf, r2, g2, b2, a2);
            }
        }

        // mips below the chosen highpass mip are grey
        for (AZ::u32 dstMip = dwMipDown; dstMip < dstMips; ++dstMip)
        {
            AZ::u8* dstPixelBuf;
            AZ::u32 dstPitch;
            newImage->GetImagePointer(dstMip, dstPixelBuf, dstPitch);
            const AZ::u32 pixelCount = newImage->GetPixelCount(dstMip);

            for (AZ::u32 i = 0; i < pixelCount; ++i, dstPixelBuf += pixelBytes)
            {
                pixelOp->SetRGBA(dstPixelBuf, 0.5f, 0.5f, 0.5f, 1.0f);
            }
        }

        m_img = newImage;
    }

} // namespace ImageProcessing