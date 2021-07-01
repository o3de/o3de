/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ImageProcessing_precompiled.h>

#include <Atom/ImageProcessing/ImageObject.h>
#include <Processing/ImageToProcess.h>
#include <Processing/PixelFormatInfo.h>
#include <Processing/ImageFlags.h>
#include <Atom/ImageProcessing/PixelFormats.h>

#include <Converters/FIR-Weights.h>
#include <Converters/PixelOperation.h>

namespace ImageProcessingAtom
{
    ///////////////////////////////////////////////////////////////////////////////////
    // Lookup table for a function 'float fn(float x)'.
    // Computed function values are stored in the table for x in [0.0; 1.0].
    //
    // If passed x is less than xMin (xMin must be >= 0) or greater than 1.0,
    // then the original function is called.
    // Otherwise, a value from the table (linearly interpolated)
    // is returned.
    template <int TABLE_SIZE>
    class FunctionLookupTable
    {
    public:
        FunctionLookupTable(float(*fn)(float x), float xMin, float maxAllowedDifference)
            : m_fn(fn)
            , m_xMin(xMin)
            , m_fMaxDiff(maxAllowedDifference)
        {
        }

        void Initialize() const
        {
            m_initialized = true;
            AZ_Assert(m_xMin >= 0.0f, "wrong initial data for m_xMin");
            for (int i = 0; i <= TABLE_SIZE; ++i)
            {
                const float x = i / (float)TABLE_SIZE;
                const float y = (*m_fn)(x);
                m_table[i] = y;
            }
        }

        inline float compute(float x) const
        {
            if (x < m_xMin || x > 1)
            {
                return m_fn(x);
            }

            const float f = x * TABLE_SIZE;

            const int i = int(f);

            if (!m_initialized)
            {
                Initialize();
            }

            if (i >= TABLE_SIZE)
            {
                return m_table[TABLE_SIZE];
            }

            const float alpha = f - i;
            return (1 - alpha) * m_table[i] + alpha * m_table[i + 1];
        }

    public:
        bool Test(const float maxDifferenceAllowed) const
        {
            if (int(-0.99f) != 0 ||
                int(+0.00f) != 0 ||
                int(+0.01f) != 0 ||
                int(+0.99f) != 0 ||
                int(+1.00f) != 1 ||
                int(+1.01f) != 1 ||
                int(+1.99f) != 1 ||
                int(+2.00f) != 2 ||
                int(+2.01f) != 2)
            {
                return false;
            }

            if (m_xMin < 0)
            {
                return false;
            }

            const int n = 1000000;
            for (int i = 0; i <= n; ++i)
            {
                const float x = 1.1f * (i / (float)n);
                const float resOriginal = m_fn(x);
                const float resTable = compute(x);
                const float difference = resOriginal - resTable;

                if (fabs(difference) > maxDifferenceAllowed)
                {
                    return false;
                }
            }
            return true;
        }

    private:
        float(* m_fn)(float x);
        float m_xMin;
        mutable float m_table[TABLE_SIZE + 1];
        mutable bool m_initialized = false;
        float m_fMaxDiff = 0.0f;
    };


    static float GammaToLinear(float x)
    {
        return (x <= 0.04045f) ? x / 12.92f : powf((x + 0.055f) / 1.055f, 2.4f);
    }

    static float LinearToGamma(float x)
    {
        return (x <= 0.0031308f) ? x * 12.92f : 1.055f * powf(x, 1.0f / 2.4f) - 0.055f;
    }

    static FunctionLookupTable<1024> s_lutGammaToLinear(GammaToLinear, 0.04045f, 0.00001f);
    static FunctionLookupTable<1024> s_lutLinearToGamma(LinearToGamma, 0.05f, 0.00001f);

    ///////////////////////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////////////////////
    bool ImageToProcess::GammaToLinearRGBA32F(bool bDeGamma)
    {
        // return immediately if there is no need to de-gamma image and the source is in the desired format
        EPixelFormat srcFmt = m_img->GetPixelFormat();
        if (!bDeGamma && (srcFmt == ePixelFormat_R32G32B32A32F))
        {
            return true;
        }

        //convert to 32F first
        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(srcFmt))
        {
            ConvertFormat(ePixelFormat_R32G32B32A32F);
            srcFmt = ePixelFormat_R32G32B32A32F;
        }

        IImageObjectPtr srcImage = m_img;
        EPixelFormat dstFmt = ePixelFormat_R32G32B32A32F;
        IImageObjectPtr dstImage(m_img->AllocateImage(dstFmt));

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
                if (bDeGamma)
                {
                    r = s_lutGammaToLinear.compute(r);
                    g = s_lutGammaToLinear.compute(g);
                    b = s_lutGammaToLinear.compute(b);
                }

                dstOp->SetRGBA(dstPixelBuf, r, g, b, a);
            }
        }

        m_img = dstImage;

        if (bDeGamma)
        {
            m_img->RemoveImageFlags(EIF_SRGBRead);
        }
        return true;
    }

    void ImageToProcess::LinearToGamma()
    {
        if (Get()->HasImageFlags(EIF_SRGBRead))
        {
            AZ_Assert(false, "%s: input image is already SRGB", __FUNCTION__);
            return;
        }

        if (!CPixelFormats::GetInstance().IsPixelFormatUncompressed(m_img->GetPixelFormat()))
        {
            AZ_Assert(false, "This is not common user case with compressed format input. But it may continue");
            ConvertFormat(ePixelFormat_R32G32B32A32F);
        }

        EPixelFormat srcFmt = m_img->GetPixelFormat();
        IImageObjectPtr srcImage = m_img;

        IImageObjectPtr dstImage(m_img->AllocateImage(srcFmt));

        //create pixel operation function
        IPixelOperationPtr pixelOp = CreatePixelOperation(srcFmt);

        //get count of bytes per pixel for both src and dst images
        uint32 pixelBytes = CPixelFormats::GetInstance().GetPixelFormatInfo(srcFmt)->bitsPerBlock / 8;

        const uint32 dwMips = srcImage->GetMipCount();
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

            for (uint32 i = 0; i < pixelCount; ++i, srcPixelBuf += pixelBytes, dstPixelBuf += pixelBytes)
            {
                pixelOp->GetRGBA(srcPixelBuf, r, g, b, a);
                r = s_lutLinearToGamma.compute(r);
                g = s_lutLinearToGamma.compute(g);
                b = s_lutLinearToGamma.compute(b);
                pixelOp->SetRGBA(dstPixelBuf, r, g, b, a);
            }
        }

        m_img = dstImage;
        Get()->AddImageFlags(EIF_SRGBRead);
    }
} // namespace ImageProcessingAtom
