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
#include <Processing/ImageConvert.h>
#include <Processing/PixelFormatInfo.h>
#include <Converters/PixelOperation.h>

///////////////////////////////////////////////////////////////////////////////////
//functions for maintaining alpha coverage.

namespace ImageProcessing
{
    void CImageObject::TransferAlphaCoverage(const TextureSettings* textureSetting, const IImageObjectPtr srcImg)
    {
        EPixelFormat srcFmt = srcImg->GetPixelFormat();
        //both this image and src image need to be uncompressed
        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(m_pixelFormat) 
            || !CPixelFormats::GetInstance().IsPixelFormatUncompressed(srcFmt))
        {
            AZ_Assert(false, "Both source image and dest image need to be uncompressed");
            return;
        }
        
        const float fAlphaRef = 0.5f;  // Seems to give good overall results
        const float fDesiredAlphaCoverage = srcImg->ComputeAlphaCoverage(0, fAlphaRef);
        
        //create pixel operation function
        IPixelOperationPtr pixelOp = CreatePixelOperation(m_pixelFormat);
        //get count of bytes per pixel 
        AZ::u32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(m_pixelFormat)->bitsPerBlock / 8;

        for (uint32 mip = 0; mip < GetMipCount(); mip++)
        {
            const float fAlphaOffset = textureSetting->ComputeMIPAlphaOffset(mip);
            const float fAlphaScale = ComputeAlphaCoverageScaleFactor(mip, fDesiredAlphaCoverage, fAlphaRef);

            AZ::u8* pixelBuf = m_mips[mip]->m_pData;
            const AZ::u32 pixelCount = GetPixelCount(mip);

            for (AZ::u32 i = 0; i < pixelCount; ++i, pixelBuf += pixelBytes)
            {
                float r, g, b, a;
                pixelOp->GetRGBA(pixelBuf, r, g, b, a);
                a = AZ::GetMin(a * fAlphaScale + fAlphaOffset, 1.0f);
                pixelOp->SetRGBA(pixelBuf, r, g, b, a);
            }
        }
    }

    float CImageObject::ComputeAlphaCoverageScaleFactor(AZ::u32 mip, float fDesiredCoverage, float fAlphaRef) const
    {
        float minAlphaRef = 0.0f;
        float maxAlphaRef = 1.0f;
        float midAlphaRef = 0.5f;

        // Find best alpha test reference value using a binary search
        for (int i = 0; i < 10; i++)
        {
            const float currentCoverage = ComputeAlphaCoverage(mip, midAlphaRef);

            if (currentCoverage > fDesiredCoverage)
            {
                minAlphaRef = midAlphaRef;
            }
            else if (currentCoverage < fDesiredCoverage)
            {
                maxAlphaRef = midAlphaRef;
            }
            else
            {
                break;
            }

            midAlphaRef = (minAlphaRef + maxAlphaRef) * 0.5f;
        }

        return fAlphaRef / midAlphaRef;
    }

    float CImageObject::ComputeAlphaCoverage(AZ::u32 mip, float fAlphaRef) const
    {
        //This function only works with uncompressed image
        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(m_pixelFormat))
        {
            AZ_Assert(false, "This image need to be uncompressed");
            return 0;
        }

        uint32 coverage = 0;

        //create pixel operation function
        IPixelOperationPtr pixelOp = CreatePixelOperation(m_pixelFormat);
        //get count of bytes per pixel 
        AZ::u32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(m_pixelFormat)->bitsPerBlock / 8;

        AZ::u8* pixelBuf = m_mips[mip]->m_pData;
        const AZ::u32 pixelCount = GetPixelCount(mip);
        
        for (AZ::u32 i = 0; i < pixelCount; ++i, pixelBuf += pixelBytes)
        {
            float r, g, b, a;
            pixelOp->GetRGBA(pixelBuf, r, g, b, a);
            coverage += a > fAlphaRef;
        }

        return (float)coverage / (float)(pixelCount);
    }

} // namespace ImageProcessing
