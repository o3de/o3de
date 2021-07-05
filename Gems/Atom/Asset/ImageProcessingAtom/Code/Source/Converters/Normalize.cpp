/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <ImageProcessing_precompiled.h>

#include <Processing/ImageObjectImpl.h>
#include <Processing/ImageFlags.h>


namespace ImageProcessingAtom
{
    template<const int qBits>
    static void AdjustScaleForQuantization(float fBaseValue, float fBaseLine, float& cScale, float& cMinColor, float& cMaxColor)
    {
        const int qOne = (1 << qBits) - 1;
        const int qUpperBits = (8 - qBits);
        const int qLowerBits = qBits - qUpperBits;

        const int v = int(floor(fBaseValue * qOne));

        int v0 = v - (v != 0);
        int v1 = v + 0;
        int v2 = v + (v != qOne);

        v0 = (v0 << qUpperBits) | (v0 >> qLowerBits);
        v1 = (v1 << qUpperBits) | (v1 >> qLowerBits);
        v2 = (v2 << qUpperBits) | (v2 >> qLowerBits);

        const float f0 = v0 / 255.0f;
        const float f1 = v1 / 255.0f;
        const float f2 = v2 / 255.0f;

        float fBaseLock = -1;

        if (fabsf(f0 - fBaseValue) < fabsf(fBaseLock - fBaseValue))
        {
            fBaseLock = f0;
        }
        if (fabsf(f1 - fBaseValue) < fabsf(fBaseLock - fBaseValue))
        {
            fBaseLock = f1;
        }
        if (fabsf(f2 - fBaseValue) < fabsf(fBaseLock - fBaseValue))
        {
            fBaseLock = f2;
        }

        float lScale = (1.0f - fBaseLock) / (1.0f - fBaseLine);
        float vScale = (1.0f - fBaseValue) / (1.0f - fBaseLine);
        float sScale = lScale / vScale;

        float csScale = (cScale / sScale);
        float csBias = cMinColor - (1.0f - sScale) * (cScale / sScale);

        if ((csBias > 0.0f) && ((csScale + csBias) < 1.0f))
        {
            cMinColor = csBias;
            cScale = csScale;
            cMaxColor = csScale + csBias;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////

    void CImageObject::NormalizeImageRange(EColorNormalization eColorNorm, EAlphaNormalization eAlphaNorm, bool bMaintainBlack, int nExponentBits)
    {
        if (GetPixelFormat() != ePixelFormat_R32G32B32A32F)
        {
            AZ_Assert(false, "%s: unsupported source format", __FUNCTION__);
            return;
        }

        uint32 dwWidth, dwHeight, dwMips;
        GetExtent(dwWidth, dwHeight, dwMips);

        // find image's range, can be negative
        float cMinColor[4] = { FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX };
        float cMaxColor[4] = { -FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX };

        for (uint32 dwMip = 0; dwMip < dwMips; ++dwMip)
        {
            uint8* pSrcMem;
            uint32 dwSrcPitch;
            GetImagePointer(dwMip, pSrcMem, dwSrcPitch);

            dwHeight = GetHeight(dwMip);
            dwWidth = GetWidth(dwMip);
            for (uint32 dwY = 0; dwY < dwHeight; ++dwY)
            {
                const float* pSrcPix = (float*)&pSrcMem[dwY * dwSrcPitch];
                for (uint32 dwX = 0; dwX < dwWidth; ++dwX)
                {
                    cMinColor[0] = AZ::GetMin(cMinColor[0], pSrcPix[0]);
                    cMinColor[1] = AZ::GetMin(cMinColor[1], pSrcPix[1]);
                    cMinColor[2] = AZ::GetMin(cMinColor[2], pSrcPix[2]);
                    cMinColor[3] = AZ::GetMin(cMinColor[3], pSrcPix[3]);

                    cMaxColor[0] = AZ::GetMax(cMaxColor[0], pSrcPix[0]);
                    cMaxColor[1] = AZ::GetMax(cMaxColor[1], pSrcPix[1]);
                    cMaxColor[2] = AZ::GetMax(cMaxColor[2], pSrcPix[2]);
                    cMaxColor[3] = AZ::GetMax(cMaxColor[3], pSrcPix[3]);

                    pSrcPix += 4;
                }
            }
        }

        if (bMaintainBlack)
        {
            cMinColor[0] = AZ::GetMin(0.f, cMinColor[0]);
            cMinColor[1] = AZ::GetMin(0.f, cMinColor[1]);
            cMinColor[2] = AZ::GetMin(0.f, cMinColor[2]);
            cMinColor[3] = AZ::GetMin(0.f, cMinColor[3]);
        }

        AZ_Assert(cMaxColor[0] >= cMinColor[0] && cMaxColor[1] >= cMinColor[1] &&
            cMaxColor[2] >= cMinColor[2] && cMaxColor[3] >= cMinColor[3], "bad color range");

        // some graceful threshold to avoid extreme cases
        if (cMaxColor[0] - cMinColor[0] < (3.f / 255))
        {
            cMinColor[0] = AZ::GetMax(0.f, cMinColor[0] - (2.f / 255));
            cMaxColor[0] = AZ::GetMin(1.f, cMaxColor[0] + (2.f / 255));
        }
        if (cMaxColor[1] - cMinColor[1] < (3.f / 255))
        {
            cMinColor[1] = AZ::GetMax(0.f, cMinColor[1] - (2.f / 255));
            cMaxColor[1] = AZ::GetMin(1.f, cMaxColor[1] + (2.f / 255));
        }
        if (cMaxColor[2] - cMinColor[2] < (3.f / 255))
        {
            cMinColor[2] = AZ::GetMax(0.f, cMinColor[2] - (2.f / 255));
            cMaxColor[2] = AZ::GetMin(1.f, cMaxColor[2] + (2.f / 255));
        }
        if (cMaxColor[3] - cMinColor[3] < (3.f / 255))
        {
            cMinColor[3] = AZ::GetMax(0.f, cMinColor[3] - (2.f / 255));
            cMaxColor[3] = AZ::GetMin(1.f, cMaxColor[3] + (2.f / 255));
        }

        // calculate range to normalize to
        const float fMaxExponent = powf(2.0f, (float)nExponentBits) - 1.0f;
        const float cUprValue = powf(2.0f, fMaxExponent);

        if (eColorNorm == eColorNormalization_PassThrough)
        {
            cMinColor[0] = cMinColor[1] = cMinColor[2] = 0.f;
            cMaxColor[0] = cMaxColor[1] = cMaxColor[2] = 1.f;
        }

        // don't touch alpha channel if not used
        if (eAlphaNorm == eAlphaNormalization_SetToZero)
        {
            // Store the range explicitly into the structure for read-back.
            // The formats which request range expansion don't support alpha.
            cMinColor[3] = 0.f;
            cMaxColor[3] = cUprValue;
        }
        else if (eAlphaNorm == eAlphaNormalization_PassThrough)
        {
            cMinColor[3] = 0.f;
            cMaxColor[3] = 1.f;
        }

        // get the origins of the color model's lattice for the range of values
        // these values need to be encoded as precise as possible under quantization
        AZ::Vector4 cBaseLines = AZ::Vector4(0.0f, 0.0f, 0.0f, 0.0f);
        AZ::Vector4 cScale = AZ::Vector4(cMaxColor[0] - cMinColor[0], cMaxColor[1] - cMinColor[1],
            cMaxColor[2] - cMinColor[2], cMaxColor[3] - cMinColor[3]);

#if 0
        // NOTE: disabled for now, in the future we can turn this on to force availability
        // of value to guarantee for example perfect grey-scales (using YFF)
        switch (GetImageFlags() & EIF_Colormodel)
        {
        case EIF_Colormodel_RGB:
            cBaseLines = Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            break;
        case EIF_Colormodel_CIE:
            cBaseLines = Vec4(0.0f, 1.f / 3, 1.f / 3, 0.0f);
            break;
        case EIF_Colormodel_IRB:
            cBaseLines = Vec4(0.0f, 1.f / 2, 1.f / 2, 0.0f);
            break;
        case EIF_Colormodel_YCC:
        case EIF_Colormodel_YFF:
            cBaseLines = Vec4(1.f / 2, 0.0f, 1.f / 2, 0.0f);
            break;
        }

        Vec4 cBaseScale = cBaseLines;
        cBaseLines = cBaseLines - cMinColor;
        cBaseLines = cBaseLines / cScale;

        if ((cBaseLines.x > 0.0f) && (cBaseLines.x < 1.0f))
        {
            AdjustScaleForQuantization<5>(cBaseLines.x, cBaseScale.x, cScale.x, cMinColor.x, cMaxColor.x);
        }
        if ((cBaseLines.y > 0.0f) && (cBaseLines.y < 1.0f))
        {
            AdjustScaleForQuantization<6>(cBaseLines.y, cBaseScale.y, cScale.y, cMinColor.y, cMaxColor.y);
        }
        if ((cBaseLines.z > 0.0f) && (cBaseLines.z < 1.0f))
        {
            AdjustScaleForQuantization<5>(cBaseLines.z, cBaseScale.z, cScale.z, cMinColor.z, cMaxColor.z);
        }
#endif

        // normalize the image
        AZ::Vector4 vMin = AZ::Vector4(cMinColor[0], cMinColor[1], cMinColor[2], cMinColor[3]);
        for (uint32 dwMip = 0; dwMip < dwMips; ++dwMip)
        {
            uint8* pSrcMem;
            uint32 dwSrcPitch;
            GetImagePointer(dwMip, pSrcMem, dwSrcPitch);

            dwHeight = GetHeight(dwMip);
            dwWidth = GetWidth(dwMip);
            for (uint32 dwY = 0; dwY < dwHeight; ++dwY)
            {
                AZ::Vector4* pSrcPix = (AZ::Vector4*)&pSrcMem[dwY * dwSrcPitch];
                for (uint32 dwX = 0; dwX < dwWidth; ++dwX)
                {
                    *pSrcPix = *pSrcPix - vMin;
                    *pSrcPix = *pSrcPix / cScale;
                    *pSrcPix = *pSrcPix * cUprValue;

                    pSrcPix++;
                }
            }
        }

        // set up a range
        SetColorRange(AZ::Color(cMinColor[0], cMinColor[1], cMinColor[2], cMinColor[3]),
            AZ::Color(cMaxColor[0], cMaxColor[1], cMaxColor[2], cMaxColor[3]));

        // set up a flag
        AddImageFlags(EIF_RenormalizedTexture);
    }

    void CImageObject::ExpandImageRange([[maybe_unused]] EColorNormalization eColorMode, EAlphaNormalization eAlphaMode, int nExponentBits)
    {
        AZ_Assert(!((eAlphaMode != eAlphaNormalization_SetToZero) && (nExponentBits != 0)), "%s: Unexpected alpha mode", __FUNCTION__);

        if (!HasImageFlags(EIF_RenormalizedTexture))
        {
            return;
        }

        if (GetPixelFormat() != ePixelFormat_R32G32B32A32F)
        {
            AZ_Assert(false, "%s: only supports source format A32B32G32R32F", __FUNCTION__);
            return;
        }

        uint32 dwWidth, dwHeight, dwMips;
        GetExtent(dwWidth, dwHeight, dwMips);

        // calculate range to normalize to
        const float fMaxExponent = powf(2.0f, (float)nExponentBits) - 1.0f;
        float cUprValue = powf(2.0f, fMaxExponent);

        // find image's range, can be negative
        AZ::Color cMinColor = AZ::Color(FLT_MAX, FLT_MAX, FLT_MAX, FLT_MAX);
        AZ::Color cMaxColor = AZ::Color(-FLT_MAX, -FLT_MAX, -FLT_MAX, -FLT_MAX);

        GetColorRange(cMinColor, cMaxColor);

        // don't touch alpha channel if not used
        if (eAlphaMode == eAlphaNormalization_SetToZero)
        {
            // Overwrite the range explicitly into the structure.
            // The formats which request range expansion don't support alpha.
            cUprValue = cMaxColor.GetA();

            cMinColor.SetA(1.f);
            cMaxColor.SetA(1.f);
        }

        // expand the image
        const AZ::Vector4 cScale = cMaxColor.GetAsVector4() - cMinColor.GetAsVector4();
        for (uint32 dwMip = 0; dwMip < dwMips; ++dwMip)
        {
            uint8* pSrcMem;
            uint32 dwSrcPitch;
            GetImagePointer(dwMip, pSrcMem, dwSrcPitch);

            dwHeight = GetHeight(dwMip);
            dwWidth = GetWidth(dwMip);
            for (uint32 dwY = 0; dwY < dwHeight; ++dwY)
            {
                AZ::Vector4* pSrcPix = (AZ::Vector4*)&pSrcMem[dwY * dwSrcPitch];
                for (uint32 dwX = 0; dwX < dwWidth; ++dwX)
                {
                    *pSrcPix = *pSrcPix / cUprValue;
                    *pSrcPix = *pSrcPix * cScale;
                    *pSrcPix = *pSrcPix + cMinColor.GetAsVector4();

                    pSrcPix++;
                }
            }
        }

        // set up a range
        SetColorRange(AZ::Color(0.0f, 0.0f, 0.0f, 0.0f), AZ::Color(1.0f, 1.0f, 1.0f, 1.0f));

        // set up a flag
        RemoveImageFlags(EIF_RenormalizedTexture);
    }

    ///////////////////////////////////////////////////////////////////////////////////

    void CImageObject::NormalizeVectors(AZ::u32 firstMip, AZ::u32 maxMipCount)
    {
        if (GetPixelFormat() != ePixelFormat_R32G32B32A32F)
        {
            AZ_Assert(false, "%s: only supports source format A32B32G32R32F", __FUNCTION__);
            return;
        }

        uint32 lastMip = AZ::GetMin(firstMip + maxMipCount, GetMipCount());
        for (uint32 mip = firstMip; mip < lastMip; ++mip)
        {
            const uint32 pixelCount = GetPixelCount(mip);
            uint8* imageMem;
            uint32 pitch;
            GetImagePointer(mip, imageMem, pitch);
            float* pPixels = (float*)imageMem;

            for (uint32 i = 0; i < pixelCount; ++i, pPixels += 4)
            {
                AZ::Vector3 vNormal = AZ::Vector3(pPixels[0] * 2.0f - 1.0f, pPixels[1] * 2.0f - 1.0f, pPixels[2] * 2.0f - 1.0f);

                // TODO: every opposing vector addition produces the zero-vector for
                // normals on the entire sphere, in that case the forward vector [0,0,1]
                // isn't necessarily right and we should look at the adjacent normals
                // for a direction
                vNormal.NormalizeSafe();

                pPixels[0] = vNormal.GetX() * 0.5f + 0.5f;
                pPixels[1] = vNormal.GetY() * 0.5f + 0.5f;
                pPixels[2] = vNormal.GetZ() * 0.5f + 0.5f;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    void CImageObject::ScaleAndBiasChannels(AZ::u32 firstMip, AZ::u32 maxMipCount, const AZ::Vector4& scale, const AZ::Vector4& bias)
    {
        if (GetPixelFormat() != ePixelFormat_R32G32B32A32F)
        {
            AZ_Assert(false, "%s: only supports source format A32B32G32R32F", __FUNCTION__);
            return;
        }

        const uint32 lastMip = AZ::GetMin(firstMip + maxMipCount, GetMipCount());
        for (uint32 mip = firstMip; mip < lastMip; ++mip)
        {
            const uint32 pixelCount = GetPixelCount(mip);
            uint8* imageMem;
            uint32 pitch;
            GetImagePointer(mip, imageMem, pitch);
            float* pPixels = (float*)imageMem;

            for (uint32 i = 0; i < pixelCount; ++i, pPixels += 4)
            {
                pPixels[0] = pPixels[0] * scale.GetX() + bias.GetX();
                pPixels[1] = pPixels[1] * scale.GetY() + bias.GetY();
                pPixels[2] = pPixels[2] * scale.GetZ() + bias.GetZ();
                pPixels[3] = pPixels[3] * scale.GetW() + bias.GetW();
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////
    void CImageObject::ClampChannels(AZ::u32 firstMip, AZ::u32 maxMipCount, const AZ::Vector4& min, const AZ::Vector4& max)
    {
        if (GetPixelFormat() != ePixelFormat_R32G32B32A32F)
        {
            AZ_Assert(false, "%s: only supports source format A32B32G32R32F", __FUNCTION__);
            return;
        }

        const uint32 lastMip = AZ::GetMin(firstMip + maxMipCount, GetMipCount());
        for (uint32 mip = firstMip; mip < lastMip; ++mip)
        {
            const uint32 pixelCount = GetPixelCount(mip);
            uint8* imageMem;
            uint32 pitch;
            GetImagePointer(mip, imageMem, pitch);
            float* pPixels = (float*)imageMem;

            for (uint32 i = 0; i < pixelCount; ++i, pPixels += 4)
            {
                pPixels[0] = AZ::GetClamp(pPixels[0], float(min.GetX()), float(max.GetX()));
                pPixels[1] = AZ::GetClamp(pPixels[1], float(min.GetY()), float(max.GetY()));
                pPixels[2] = AZ::GetClamp(pPixels[2], float(min.GetZ()), float(max.GetZ()));
                pPixels[3] = AZ::GetClamp(pPixels[3], float(min.GetW()), float(max.GetW()));
            }
        }
    }
} //namespace ImageProcessingAtom
