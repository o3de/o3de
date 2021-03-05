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

#include <ImageProcessing/PixelFormats.h>
#include <ImageProcessing/ImageObject.h>
#include <Converters/PixelOperation.h>
#include <Processing/PixelFormatInfo.h>

#include <Converters/Histogram.h>

///////////////////////////////////////////////////////////////////////////////////

namespace ImageProcessing
{
    float GetLuminance(const float& r, const float& g, const float& b)
    {
        return (r * 0.30f + g * 0.59f + b * 0.11f);
    }

    bool ComputeLuminanceHistogram(IImageObjectPtr imageObject, Histogram<256>& histogram)
    {
        EPixelFormat pixelFormat = imageObject->GetPixelFormat();
        if (!(CPixelFormats::GetInstance().IsPixelFormatUncompressed(pixelFormat)))
        {
            AZ_Assert(false, "%s function only works with uncompressed pixel format", __FUNCTION__);
            return false;
        }

        //create pixel operation function
        IPixelOperationPtr pixelOp = CreatePixelOperation(pixelFormat);

        //setup histogram bin
        static const size_t binCount = 256;
        Histogram<binCount>::Bins bins;
        Histogram<binCount>::clearBins(bins);

        //get count of bytes per pixel 
        AZ::u32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(pixelFormat)->bitsPerBlock / 8;

        const  AZ::u32 mipCount = imageObject->GetMipCount();
        float color[4];
        for (uint32 mip = 0; mip < mipCount; ++mip)
        {
            AZ::u8* pixelBuf;
            AZ::u32 pitch;
            imageObject->GetImagePointer(mip, pixelBuf, pitch);
            const uint32 pixelCount = imageObject->GetPixelCount(mip);

            for (uint32 i = 0; i < pixelCount; ++i, pixelBuf += pixelBytes)
            {
                pixelOp->GetRGBA(pixelBuf, color[0], color[1], color[2], color[3]);

                const float luminance = AZ::GetClamp(GetLuminance(color[0], color[1], color[2]), 0.0f, 1.0f);
                const float f = luminance * binCount;
                if (f <= 0)
                {
                    ++bins[0];
                }
                else
                {
                    const int bin = int(f);
                    ++bins[(bin < binCount) ? bin : binCount - 1];
                }
            }
        }

        histogram.set(bins);
        return true;
    }
} // end namespace ImageProcessing
