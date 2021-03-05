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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Direct3D specific post processing special effects

#include "RenderDll_precompiled.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"
#include <Common/RenderCapabilities.h>
#include <Common/Textures/TextureManager.h>

#include <AzFramework/Input/Devices/Mouse/InputDeviceMouse.h>

#if defined(AZ_RESTRICTED_PLATFORM)
#undef AZ_RESTRICTED_SECTION
#define D3DPOSTPROCESS_CPP_SECTION_1 1
#define D3DPOSTPROCESS_CPP_SECTION_2 2
#endif

enum COLORSPACES
{
    CS_sRGB0 = 0,    //Most accurate sRGB curve
    CS_sRGB1 = 1,   //Cheap approximation - pow(col, 1/2.2)
    CS_sRGB2 = 2,   //cheaper approx - sqrt(col)
    CS_P3D65 = 3,
    CS_Rec709 = 4,
    CS_Rec2020 = 5
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

SDepthTexture* SD3DPostEffectsUtils::GetDepthSurface(CTexture* pTex)
{
    if (pTex->GetFlags() & FT_USAGE_MSAA && gRenDev->m_RP.m_MSAAData.Type)
    {
        return &gcpRendD3D->m_DepthBufferOrigMSAA;
    }

    return &gcpRendD3D->m_DepthBufferOrig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::ResolveRT(CTexture*& pDst, const RECT* pSrcRect)
{
    AZ_Assert(pDst, "Null texture passed in");
    if (!pDst)
    {
        return;
    }

    int iTempX, iTempY, iWidth, iHeight;
    gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    CDeviceTexture* pDstResource = pDst->GetDevTexture();
    ID3D11RenderTargetView* pOrigRT = gcpRendD3D->m_pNewTarget[0]->m_pTarget;
    if (pOrigRT && pDstResource)
    {
        D3D11_BOX box;
        ZeroStruct(box);
        if (pSrcRect)
        {
            box.left = pSrcRect->left;
            box.right = pSrcRect->right;
            box.top = pSrcRect->top;
            box.bottom = pSrcRect->bottom;
        }
        else
        {
            box.right = min(pDst->GetWidth(), gcpRendD3D->m_pNewTarget[0]->m_Width);
            box.bottom = min(pDst->GetHeight(), gcpRendD3D->m_pNewTarget[0]->m_Height);
        }
        box.back = 1;


#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DPOSTPROCESS_CPP_SECTION_1
    #include AZ_RESTRICTED_FILE(D3DPostProcess_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
    #else
        ID3D11Resource* pSrcResource;
        pOrigRT->GetResource(&pSrcResource);
    #endif

        HRESULT hr = 0;
        gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_RTCopied++;
        gcpRendD3D->m_RP.m_PS[gcpRendD3D->m_RP.m_nProcessThreadID].m_RTCopiedSize += pDst->GetDeviceDataSize();

        gcpRendD3D->GetDeviceContext().CopySubresourceRegion(pDstResource->Get2DTexture(), 0, 0, 0, 0, pSrcResource, 0, &box);
        SAFE_RELEASE(pSrcResource);
    }
}

void SD3DPostEffectsUtils::SetSRGBShaderFlags()
{
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SRGB0] | g_HWSR_MaskBit[HWSR_SRGB1] | g_HWSR_MaskBit[HWSR_SRGB2]);
    switch(CRenderer::CV_r_ColorSpace)
    {
        case COLORSPACES::CS_sRGB0:
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SRGB0];
            break;
        }
        case COLORSPACES::CS_sRGB1:
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SRGB1];
            break;
        }
        case COLORSPACES::CS_sRGB2:
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SRGB2];
            break;
        }
        case COLORSPACES::CS_P3D65:
        {
            //todo: Needs support
        }
        case COLORSPACES::CS_Rec709:
        {
            //todo: Add support for Rec709 related shader flags to allow control over color space conversion from c++ code
        }
        case COLORSPACES::CS_Rec2020:
        {
            //todo: Add support for Rec2020 related shader flags to allow control over color space conversion from c++ code
        }
        default:
        {
            CryWarning(VALIDATOR_MODULE_RENDERER, VALIDATOR_WARNING, "Color space not supported");
        }
    }
}

void SD3DPostEffectsUtils::CopyTextureToScreen(CTexture*& pSrc, const RECT* pSrcRegion, const int filterMode, bool sRGBLookup)
{
    CRenderer* rd = gRenDev;
    uint64 saveFlagsShader_RT = rd->m_RP.m_FlagsShader_RT;
    rd->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE3];
    if (sRGBLookup && !pSrc->IsSRGB() && !RenderCapabilities::SupportsTextureViews())
    {
        //Force SRGB conversion in the shader because the platform doesn't support texture views
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
        SetSRGBShaderFlags();
    }
    static CCryNameTSCRC pRestoreTechName("TextureToTexture");
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffects, pRestoreTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    gRenDev->FX_SetState(GS_NODEPTHTEST);
    PostProcessUtils().SetTexture(pSrc, 0, filterMode >= 0 ? filterMode : FILTER_POINT, 1, sRGBLookup);
    PostProcessUtils().DrawFullScreenTri(pSrc->GetWidth(), pSrc->GetHeight(), 0, pSrcRegion);
    PostProcessUtils().ShEndPass();
    rd->m_RP.m_FlagsShader_RT = saveFlagsShader_RT;
}

void SD3DPostEffectsUtils::CopyScreenToTexture(CTexture*& pDst, const RECT* pSrcRegion)
{
    int iTempX, iTempY, iWidth, iHeight;
    gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    CTexture* source = gcpRendD3D->FX_GetCurrentRenderTarget(0);
    if (source)
    {
        if (source->GetDstFormat() == pDst->GetDstFormat())
        {
            ResolveRT(pDst, pSrcRegion);
        }
        else
        {
            StretchRect(source, pDst, false, false, false, false, SPostEffectsUtils::eDepthDownsample_None, false, pSrcRegion);
        }
    }
    else
    {
        CDeviceTexture* dstResource = pDst->GetDevTexture();
        ID3D11RenderTargetView* sourceRT = gcpRendD3D->FX_GetCurrentRenderTargetSurface(0);
        if (sourceRT)
        {
            D3D11_RENDER_TARGET_VIEW_DESC backbufferDesc;
            sourceRT->GetDesc(&backbufferDesc);
            const D3DFormat dstFmt = CTexture::DeviceFormatFromTexFormat(pDst->GetDstFormat());
            const D3DFormat srcFmt = backbufferDesc.Format;

            if (dstFmt == srcFmt)
            {
                ID3D11Resource* srcResource;
                sourceRT->GetResource(&srcResource);

                ID3D11Texture2D* srcTex2D = static_cast<ID3D11Texture2D*>(srcResource);
                D3D11_TEXTURE2D_DESC srcTex2desc;
                srcTex2D->GetDesc(&srcTex2desc);

                if (pSrcRegion)
                {
                    D3D11_BOX box = { 0 };
                    box.left = pSrcRegion->left;
                    box.right = pSrcRegion->right;
                    box.top = pSrcRegion->top;
                    box.bottom = pSrcRegion->bottom;
                    box.front = 0;
                    box.back = 1;
                    gcpRendD3D->GetDeviceContext().CopySubresourceRegion(dstResource->Get2DTexture(), 0, 0, 0, 0, srcResource, 0, &box);
                }
                else
                {
                    gcpRendD3D->GetDeviceContext().CopySubresourceRegion(dstResource->Get2DTexture(), 0, 0, 0, 0, srcResource, 0, NULL);
                }
            }
            else
            {
                AZ_Assert(false, "Pixel formats differ");
            }
        }
        else
        {
            AZ_Assert(false, "No source texture present");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::StretchRect(CTexture* pSrc, CTexture*& pDst, bool bClearAlpha, bool bDecodeSrcRGBK, bool bEncodeDstRGBK, bool bBigDownsample, EDepthDownsample depthDownsample, bool bBindMultisampled, const RECT* srcRegion)
{
    if (!pSrc || !pDst)
    {
        return;
    }

    PROFILE_LABEL_SCOPE("STRETCHRECT");
    PROFILE_SHADER_SCOPE;

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] |
                                        g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5] | g_HWSR_MaskBit[HWSR_REVERSE_DEPTH]);

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
    gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
    bool bResample = 0;

    if (pSrc->GetWidth() != pDst->GetWidth() || pSrc->GetHeight() != pDst->GetHeight())
    {
        bResample = 1;
    }

    const D3DFormat dstFmt = CTexture::DeviceFormatFromTexFormat(pDst->GetDstFormat());
    const D3DFormat srcFmt = CTexture::DeviceFormatFromTexFormat(pSrc->GetDstFormat());

    bool destinationBaseTextureExists = pDst->GetDevTexture() && pDst->GetDevTexture()->GetBaseTexture();
    AZ_Error("Rendering", destinationBaseTextureExists, "'%s' used as destination texture in call to SD3DPostProcessUtils::StretchRect, but it does not have a valid device texture.", pDst->GetName());
    bool sourceBaseTextureExists = pSrc->GetDevTexture() && pSrc->GetDevTexture()->GetBaseTexture();
    AZ_Error("Rendering", sourceBaseTextureExists, "'%s' used as source texture in call to SD3DPostProcessUtils::StretchRect, but it does not have a valid device texture.", pSrc->GetName());

    if (bResample == false && gRenDev->m_RP.m_FlagsShader_RT == 0 && dstFmt == srcFmt)
    {
        if (sourceBaseTextureExists && destinationBaseTextureExists)
        {
            gcpRendD3D->GetDeviceContext().CopyResource(pDst->GetDevTexture()->GetBaseTexture(), pSrc->GetDevTexture()->GetBaseTexture());
            gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
        }
        return;
    }

    gcpRendD3D->FX_PushRenderTarget(0, pDst, NULL);

    gcpRendD3D->FX_SetColorDontCareActions(0, true, false);
    gcpRendD3D->FX_SetDepthDontCareActions(0, true, true);
    gcpRendD3D->FX_SetStencilDontCareActions(0, true, true);

    gcpRendD3D->FX_SetActiveRenderTargets();
    gcpRendD3D->RT_SetViewport(0, 0, pDst->GetWidth(), pDst->GetHeight());

    bool bEnableRTSample0 = bBindMultisampled;

    if (bEnableRTSample0)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    if (bClearAlpha)         // clear alpha to 0
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    }
    if (bDecodeSrcRGBK)  // decode RGBK src texture
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
    }
    if (depthDownsample != eDepthDownsample_None)  // take minimum/maximum depth from the 4 samples
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
        if (depthDownsample == eDepthDownsample_Min)
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_REVERSE_DEPTH];
        }
    }
    if (bEncodeDstRGBK)  // encode RGBK dst texture
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
    }

    static CCryNameTSCRC pTechTexToTex("TextureToTexture");
    static CCryNameTSCRC pTechTexToTexResampled("TextureToTextureResampled");
    ShBeginPass(CShaderMan::s_shPostEffects, bResample ? pTechTexToTexResampled : pTechTexToTex, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    gRenDev->FX_SetState(GS_NODEPTHTEST);

    // Get sample size ratio (based on empirical "best look" approach)
    float fSampleSize = ((float)pSrc->GetWidth() / (float)pDst->GetWidth()) * 0.5f;

    // Set samples position
    //float s1 = fSampleSize / (float) pSrc->GetWidth();  // 2.0 better results on lower res images resizing
    //float t1 = fSampleSize / (float) pSrc->GetHeight();

    CTexture* pOffsetTex = bBigDownsample ? pDst : pSrc;

    float s1 = 0.5f / (float) pOffsetTex->GetWidth(); // 2.0 better results on lower res images resizing
    float t1 = 0.5f / (float) pOffsetTex->GetHeight();

    Vec4 pParams0, pParams1;

    if (bBigDownsample)
    {
        // Use rotated grid + middle sample (~quincunx)
        pParams0 = Vec4(s1 * 0.96f, t1 * 0.25f, -s1 * 0.25f, t1 * 0.96f);
        pParams1 = Vec4(-s1 * 0.96f, -t1 * 0.25f, s1 * 0.25f, -t1 * 0.96f);
    }
    else
    {
        // Use box filtering (faster - can skip bilinear filtering, only 4 taps)
        pParams0 = Vec4(-s1, -t1, s1, -t1);
        pParams1 = Vec4(s1, t1, -s1, t1);
    }

    static CCryNameR pParam0Name("texToTexParams0");
    static CCryNameR pParam1Name("texToTexParams1");

    CShaderMan::s_shPostEffects->FXSetPSFloat(pParam0Name, &pParams0, 1);
    CShaderMan::s_shPostEffects->FXSetPSFloat(pParam1Name, &pParams1, 1);

    int nFilter = bResample ? FILTER_LINEAR : FILTER_POINT;
    pSrc->Apply(0, CTexture::GetTexState(STexState(nFilter, true)), EFTT_UNKNOWN, -1,
        (bBindMultisampled && gRenDev->m_RP.m_MSAAData.Type) ? SResourceView::DefaultViewMS : SResourceView::DefaultView);  // bind as msaa target (if valid)

    DrawFullScreenTri(pDst->GetWidth(), pDst->GetHeight(), 0, srcRegion);

    ShEndPass();

    // Restore previous viewport
    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

void SD3DPostEffectsUtils::SwapRedBlue([[maybe_unused]] CTexture* pSrc, [[maybe_unused]] CTexture* pDst)
{
#if defined(AZ_RESTRICTED_PLATFORM)
    #define AZ_RESTRICTED_SECTION D3DPOSTPROCESS_CPP_SECTION_2
    #include AZ_RESTRICTED_FILE(D3DPostProcess_cpp)
#endif
}

void SD3DPostEffectsUtils::DownsampleDepth(CTexture* pSrc, CTexture* pDst, bool bFromSingleChannel)
{
    PROFILE_LABEL_SCOPE("DOWNSAMPLE_DEPTH");
    PROFILE_SHADER_SCOPE;

    if (!pDst)
    {
        return;
    }

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    uint64 nSaveFlagsShader_RT = rd->m_RP.m_FlagsShader_RT;
    rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    // Get current viewport
    int prevVPX, prevVPY, prevVPWidth, prevVPHeight;
    rd->GetViewport(&prevVPX, &prevVPY, &prevVPWidth, &prevVPHeight);

    bool bUseDeviceDepth = (pSrc == NULL);

    rd->FX_PushRenderTarget(0, pDst, NULL);

    // Metal Load/Store Actions
    rd->FX_SetColorDontCareActions(0, true, false);
    rd->FX_SetDepthDontCareActions(0, true, true);
    rd->FX_SetStencilDontCareActions(0, true, true);

    int dstWidth  = pDst->GetWidth();
    int dstHeight = pDst->GetHeight();
    rd->RT_SetViewport(0, 0, dstWidth, dstHeight);

    if (bUseDeviceDepth)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    if (bFromSingleChannel)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    }

    static CCryNameTSCRC pTech("DownsampleDepth");
    ShBeginPass(CShaderMan::s_shPostEffects, pTech, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    rd->FX_SetState(GS_NODEPTHTEST);

    int srcWidth  = bUseDeviceDepth ? rd->GetWidth()  : pSrc->GetWidth();
    int srcHeight = bUseDeviceDepth ? rd->GetHeight() : pSrc->GetHeight();

    if (bUseDeviceDepth)
    {
        rd->m_DevMan.BindSRV(eHWSC_Pixel, &rd->m_pZBufferDepthReadOnlySRV, 0, 1);
    }
    else
    {
        SetTexture(pSrc, 0, FILTER_POINT, 1);
    }

#if defined(CRY_USE_METAL) || defined(ANDROID)
    const Vec2& vDownscaleFactor = gcpRendD3D->m_RP.m_CurDownscaleFactor;
    gRenDev->RT_SetScissor(true, 0, 0, pDst->GetWidth() * vDownscaleFactor.x + 0.5f, pDst->GetHeight() * vDownscaleFactor.y + 0.5f);
#endif

    if (GetShaderLanguage() == eSL_GLES3_0)
    {
        // There's a bug in Qualcomm OpenGL ES 3.0 drivers that cause the device
        // shader compiler to crash if we use "textureSize" in the shader to get the texture dimensions.
        Vec4 texSize = Vec4(srcWidth, srcHeight, 0, 0);
        static CCryNameR texSizeParam("DownsampleDepth_DepthTex_Dimensions");
        CShaderMan::s_shPostEffects->FXSetPSFloat(texSizeParam, &texSize, 1);
    }

    RECT source = { 0, 0, pDst->GetWidth(), pDst->GetHeight() };
    //Round up to even to handle uneven dimensions
    dstWidth = (dstWidth + 1) & ~1;
    dstHeight = (dstHeight + 1) & ~1;
    DrawFullScreenTri(dstWidth, dstHeight, 0.f, &source);

#if defined(CRY_USE_METAL) || defined(ANDROID)
    gRenDev->RT_SetScissor(false, 0, 0, 0, 0);
#endif

    ShEndPass();

    if (bUseDeviceDepth)
    {
        D3DShaderResourceView* pNullSRV[1] = { NULL };
        rd->m_DevMan.BindSRV(eHWSC_Pixel, pNullSRV, 0, 1);

        rd->FX_Commit();
    }

    // Restore previous viewport
    rd->FX_PopRenderTarget(0);
    rd->RT_SetViewport(prevVPX, prevVPY, prevVPWidth, prevVPHeight);

    rd->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
void SD3DPostEffectsUtils::DownsampleDepthUsingCompute(CTexture* pSrc, CTexture** pDstArr, bool bFromSingleChannel)
{
    PROFILE_LABEL_SCOPE("DOWNSAMPLE_DEPTHCS");
    PROFILE_SHADER_SCOPE;

    if (!pDstArr || !pDstArr[0])
    {
        return;
    }

    CTexture* pDst = pDstArr[0];

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    uint64 saveFlagsShader_RT = rd->m_RP.m_FlagsShader_RT;
    rd->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    bool useDeviceDepth = (pSrc == NULL);

    rd->RT_SetViewport(0, 0, pDst->GetWidth(), pDst->GetHeight());

    if (useDeviceDepth)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    if (bFromSingleChannel)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    }

    static CCryNameTSCRC pTech("DownsampleDepthCS");
    ShBeginPass(CShaderMan::s_shPostEffects, pTech, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    rd->FX_SetState(GS_NODEPTHTEST);

    int srcWidth  = useDeviceDepth ? rd->GetWidth()  : pSrc->GetWidth();
    int srcHeight = useDeviceDepth ? rd->GetHeight() : pSrc->GetHeight();

    if (useDeviceDepth)
    {
        rd->m_DevMan.BindSRV(eHWSC_Compute, &rd->m_pZBufferDepthReadOnlySRV, 0, 1);
    }
    else
    {
        D3DShaderResourceView* pShaderResrouce[1] = {
            (pSrc->GetShaderResourceView()),
        };
        rd->m_DevMan.BindSRV(eHWSC_Compute, pShaderResrouce, 0, 1);
    }

    static CCryNameR paramSrcSize("SrcTexSizeAndCount");
    Vec4 vParamSrcSize(srcWidth, srcHeight, 2, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(paramSrcSize, &vParamSrcSize, 1);
    rd->FX_Commit();

    D3DUnorderedAccessView* UAVs[2] = {
        (pDstArr[0]->GetDeviceUAV()),
        (pDstArr[1]->GetDeviceUAV()),
    };
    rd->GetDeviceContext().CSSetUnorderedAccessViews(0, 2, UAVs, NULL);

    uint32 dispatchSizeX = srcWidth / 8 + (srcWidth % 8 > 0 ? 1 : 0);
    uint32 dispatchSizeY = srcHeight / 8 + (srcHeight % 8 > 0 ? 1 : 0);
    rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, 1);
    ShEndPass();
    rd->m_RP.m_FlagsShader_RT = saveFlagsShader_RT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
void SD3DPostEffectsUtils::DownsampleUsingCompute(CTexture* pSrc, CTexture** pDstArr)
{
    AZ_Assert(pSrc && pDstArr[0], "Null textures passed in");

    const int maxIterations = 3;

    int numIters = 1;
    D3DUnorderedAccessView* pUAV[maxIterations];
    pUAV[0] = (pDstArr[0]->GetDeviceUAV());

    for (int i = 1; i < maxIterations; i++)
    {
        if (pDstArr[i])
        {
            numIters++;
            pUAV[i] = (pDstArr[i]->GetDeviceUAV());
        }
        else
        {
            // Need to bind a UAV or Metal will complain... even if not written to.
            pUAV[i] = pUAV[0];
            break;
        }
    }

    PROFILE_LABEL_SCOPE("DOWNSAMPLE_SCENE_CS");
    PROFILE_SHADER_SCOPE;

    CTexture* pSrcRT = pSrc;

    static CCryNameTSCRC pTech("TextureToTextureCS");
    ShBeginPass(CShaderMan::s_shPostEffects, pTech, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    int srcWidth  = pSrcRT->GetWidth();
    int srcHeight = pSrcRT->GetHeight();

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    D3DShaderResourceView* pSRV = (pSrcRT->GetShaderResourceView());
    rd->m_DevMan.BindSRV(eHWSC_Compute, &pSRV, 0, 1);

    static CCryNameR param("gNumIterations");
    Vec4 vParam(numIters, 0, 0, 0);
    CShaderMan::s_shDeferredShading->FXSetCSFloat(param, &vParam, 1);
    rd->FX_Commit();

    rd->GetDeviceContext().CSSetUnorderedAccessViews(0, 3, pUAV, NULL);

    // Grid dims must match in shader
    const uint32 kernelGridX = 8;
    const uint32 kernelGridY = 8;

    uint32 dispatchSizeX = srcWidth / kernelGridX + (srcWidth % kernelGridX > 0 ? 1 : 0);
    uint32 dispatchSizeY = srcHeight / kernelGridY + (srcHeight % kernelGridY > 0 ? 1 : 0);
    rd->m_DevMan.Dispatch(dispatchSizeX, dispatchSizeY, 1);
    ShEndPass();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::Downsample(CTexture* pSrc, CTexture* pDst, int nSrcW, int nSrcH, int nDstW, int nDstH, EFilterType eFilter, bool bSetTarget)
{
    if (!pSrc)
    {
        return;
    }

    PROFILE_LABEL_SCOPE("DOWNSAMPLE");
    PROFILE_SHADER_SCOPE;

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2]);

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
    gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    if (bSetTarget)
    {
        gcpRendD3D->FX_PushRenderTarget(0, pDst, NULL);
    }
    gcpRendD3D->RT_SetViewport(0, 0, nDstW, nDstH);

    // Currently only exact multiples supported
    Vec2 vSamples(float(nSrcW) / nDstW, float(nSrcH) / nDstH);
    const Vec2 vSampleSize(1.f / nSrcW, 1.f / nSrcH);
    const Vec2 vPixelSize(1.f / nDstW, 1.f / nDstH);
    // Adjust UV space if source rect smaller than texture
    const float fClippedRatioX = float(nSrcW) / pSrc->GetWidth();
    const float fClippedRatioY = float(nSrcH) / pSrc->GetHeight();

    // Base kernel size in pixels
    float fBaseKernelSize = 1.f;
    // How many lines of border samples to skip
    float fBorderSamplesToSkip = 0.f;

    switch (eFilter)
    {
    default:
    case FilterType_Box:
        fBaseKernelSize = 1.f;
        fBorderSamplesToSkip = 0.f;
        break;
    case FilterType_Tent:
        fBaseKernelSize = 2.f;
        fBorderSamplesToSkip = 0.f;
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
        break;
    case FilterType_Gauss:
        // The base kernel for Gaussian filter is 3x3 pixels [-1.5 .. 1.5]
        // Samples on the borders are ignored due to small contribution
        // so the actual kernel size is N*3 - 2 where N is number of samples per pixel
        fBaseKernelSize = 3.f;
        fBorderSamplesToSkip = 1.f;
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
        break;
    case FilterType_Lanczos:
        fBaseKernelSize = 3.f;
        fBorderSamplesToSkip = 0.f;
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
        break;
    }

    // Kernel position step
    const Vec2 vSampleStep(1.f / vSamples.x, 1.f / vSamples.y);
    // The actual kernel radius in pixels
    const Vec2 vKernelRadius = 0.5f * Vec2(fBaseKernelSize, fBaseKernelSize) - fBorderSamplesToSkip * vSampleStep;

    // UV offset from pixel center to first (top-left) sample
    const Vec2 vFirstSampleOffset(0.5f * vSampleSize.x - vKernelRadius.x * vPixelSize.x,
        0.5f * vSampleSize.y - vKernelRadius.y * vPixelSize.y);
    // Kernel position of first (top-left) sample
    const Vec2 vFirstSamplePos = -vKernelRadius + 0.5f * vSampleStep;

    static CCryNameTSCRC pTechName("TextureToTextureResampleFilter");
    ShBeginPass(CShaderMan::s_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    gRenDev->FX_SetState(GS_NODEPTHTEST);

    const Vec4 pParams0(vKernelRadius.x, vKernelRadius.y, fClippedRatioX, fClippedRatioY);
    const Vec4 pParams1(vSampleSize.x, vSampleSize.y, vFirstSampleOffset.x, vFirstSampleOffset.y);
    const Vec4 pParams2(vSampleStep.x, vSampleStep.y, vFirstSamplePos.x, vFirstSamplePos.y);

    static CCryNameR pParam0Name("texToTexParams0");
    static CCryNameR pParam1Name("texToTexParams1");
    static CCryNameR pParam2Name("texToTexParams2");
    CShaderMan::s_shPostEffects->FXSetPSFloat(pParam0Name, &pParams0, 1);
    CShaderMan::s_shPostEffects->FXSetPSFloat(pParam1Name, &pParams1, 1);
    CShaderMan::s_shPostEffects->FXSetPSFloat(pParam2Name, &pParams2, 1);

    SetTexture(pSrc, 0, FILTER_NONE);

    DrawFullScreenTri(nDstW, nDstH);

    ShEndPass();

    // Restore previous viewport
    if (bSetTarget)
    {
        gcpRendD3D->FX_PopRenderTarget(0);
    }
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::DownsampleStable(CTexture* pSrcRT, CTexture* pDstRT, bool bKillFireflies)
{
    PROFILE_LABEL_SCOPE("DOWNSAMPLE_STABLE");

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    int iTempX, iTempY, iWidth, iHeight;
    gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    gcpRendD3D->FX_PushRenderTarget(0, pDstRT, NULL);
    gcpRendD3D->RT_SetViewport(0, 0, pDstRT->GetWidth(), pDstRT->GetHeight());

    gcpRendD3D->FX_SetColorDontCareActions(0, true, false);
    gcpRendD3D->FX_SetDepthDontCareActions(0, true, true);
    gcpRendD3D->FX_SetStencilDontCareActions(0, true, true);

    if (bKillFireflies)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    static CCryNameTSCRC techName("DownsampleStable");
    ShBeginPass(CShaderMan::s_shPostEffects, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);
    pSrcRT->Apply(0, CTexture::GetTexState(STexState(FILTER_LINEAR, true)));

    DrawFullScreenTri(pDstRT->GetWidth(), pDstRT->GetHeight());

    ShEndPass();

    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::TexBlurIterative(CTexture* pTex, int nIterationsMul, bool bDilate, CTexture* pBlurTmp)
{
    if (!pTex)
    {
        return;
    }

    SDynTexture* tpBlurTemp = 0;

    if (!pBlurTmp)
    {
        tpBlurTemp = new SDynTexture(pTex->GetWidth(), pTex->GetHeight(), pTex->GetDstFormat(), eTT_2D, FT_STATE_CLAMP | FT_USAGE_RENDERTARGET, "TempBlurRT");
        if (tpBlurTemp)
        {
            tpBlurTemp->Update(pTex->GetWidth(), pTex->GetHeight());
        }

        if (!tpBlurTemp || !tpBlurTemp->m_pTexture)
        {
            SAFE_DELETE(tpBlurTemp);
            return;
        }
    }

    PROFILE_LABEL_SCOPE("TEXBLUR_16TAPS");
    PROFILE_SHADER_SCOPE;

    CTexture* pTempRT = pBlurTmp ? pBlurTmp : tpBlurTemp->m_pTexture;

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
    gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    // Iterative blur (aka Kawase): 4 taps, 16 taps, 64 taps, 256 taps, etc
    uint64 nFlagsShaderRT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1]);

    //// Dilate - use 2 passes, horizontal+vertical
    //if( bDilate )
    //{
    //  gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    //  nIterationsMul = 1;
    //}

    for (int i = 1; i <= nIterationsMul; ++i)
    {
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 1st iteration (4 taps)

        gcpRendD3D->FX_PushRenderTarget(0, pTempRT, NULL);
        gcpRendD3D->FX_SetActiveRenderTargets(false);// Avoiding invalid d3d error (due to deferred rt setup, when ping-pong'ing between RTs we can bump into RTs still bound when binding it as a SRV)
        gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());

        // only regular gaussian blur supporting masking
        static CCryNameTSCRC pTechName("Blur4Taps");

        uint32 nPasses;
        CShaderMan::s_shPostEffects->FXSetTechnique(pTechName);
        CShaderMan::s_shPostEffects->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        gRenDev->FX_SetState(GS_NODEPTHTEST);

        // setup texture offsets, for texture sampling
        // Get sample size ratio (based on empirical "best look" approach)
        float fSampleSize = 1.0f * ((float) i);//((float)pTex->GetWidth()/(float)pTex->GetWidth()) * 0.5f;

        // Set samples position
        float s1 = fSampleSize / (float) pTex->GetWidth();  // 2.0 better results on lower res images resizing
        float t1 = fSampleSize / (float) pTex->GetHeight();

        // Use rotated grid
        Vec4 pParams0 = Vec4(s1 * 0.96f, t1 * 0.25f, -s1 * 0.25f, t1 * 0.96f);//Vec4(-s1, -t1, s1, -t1);
        Vec4 pParams1 = Vec4(-s1 * 0.96f, -t1 * 0.25f, s1 * 0.25f, -t1 * 0.96f);//Vec4(s1, t1, -s1, t1);

        static CCryNameR pParam0Name("texToTexParams0");
        static CCryNameR pParam1Name("texToTexParams1");

        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam0Name, &pParams0, 1);
        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam1Name, &pParams1, 1);

        CShaderMan::s_shPostEffects->FXBeginPass(0);

        SetTexture(pTex, 0, FILTER_LINEAR);

        DrawFullScreenTri(pTex->GetWidth(), pTex->GetHeight());

        CShaderMan::s_shPostEffects->FXEndPass();
        CShaderMan::s_shPostEffects->FXEnd();

        gcpRendD3D->FX_PopRenderTarget(0);

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 2nd iteration (4 x 4 taps)
        if (bDilate)
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
        }

        gcpRendD3D->FX_PushRenderTarget(0, pTex, NULL);
        gcpRendD3D->FX_SetActiveRenderTargets(false);// Avoiding invalid d3d error (due to deferred rt setup, when ping-pong'ing between RTs we can bump into RTs still bound when binding it as a SRV)
        gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());

        CShaderMan::s_shPostEffects->FXSetTechnique(pTechName);
        CShaderMan::s_shPostEffects->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        gRenDev->FX_SetState(GS_NODEPTHTEST);
        // increase kernel size for second iteration
        fSampleSize = 2.0 * ((float) i);
        // Set samples position
        s1 = fSampleSize / (float) pTex->GetWidth();  // 2.0 better results on lower res images resizing
        t1 = fSampleSize / (float) pTex->GetHeight();

        // Use rotated grid
        pParams0 = Vec4(s1 * 0.96f, t1 * 0.25f, -s1 * 0.25f, t1 * 0.96f);//Vec4(-s1, -t1, s1, -t1);
        pParams1 = Vec4(-s1 * 0.96f, -t1 * 0.25f, s1 * 0.25f, -t1 * 0.96f);//Vec4(s1, t1, -s1, t1);

        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam0Name, &pParams0, 1);
        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam1Name, &pParams1, 1);

        CShaderMan::s_shPostEffects->FXBeginPass(0);

        SetTexture(pTempRT, 0, FILTER_LINEAR);

        DrawFullScreenTri(pTex->GetWidth(), pTex->GetHeight());

        CShaderMan::s_shPostEffects->FXEndPass();
        CShaderMan::s_shPostEffects->FXEnd();

        gcpRendD3D->FX_PopRenderTarget(0);
    }

    gRenDev->m_RP.m_FlagsShader_RT = nFlagsShaderRT;

    // Restore previous viewport
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    SAFE_DELETE(tpBlurTemp);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::TexBlurDirectional(CTexture* pTex, const Vec2& vDir, int nIterationsMul, CTexture* pBlurTmp)
{
    if (!pTex)
    {
        return;
    }

    SDynTexture* tpBlurTemp = 0;
    if (!pBlurTmp)
    {
        tpBlurTemp = new SDynTexture(pTex->GetWidth(), pTex->GetHeight(), pTex->GetDstFormat(), eTT_2D, FT_STATE_CLAMP | FT_USAGE_RENDERTARGET, "TempBlurRT");
        if (tpBlurTemp)
        {
            tpBlurTemp->Update(pTex->GetWidth(), pTex->GetHeight());
        }

        if (!tpBlurTemp || !tpBlurTemp->m_pTexture)
        {
            SAFE_DELETE(tpBlurTemp);
            return;
        }
    }

    PROFILE_LABEL_SCOPE("TEXBLUR_DIRECTIONAL");
    PROFILE_SHADER_SCOPE;

    CTexture* pTempRT = pBlurTmp;
    if (!pBlurTmp)
    {
        pTempRT = tpBlurTemp->m_pTexture;
    }

    static CCryNameTSCRC pTechName("BlurDirectional");
    static CCryNameR pParam0Name("texToTexParams0");
    static CCryNameR pParam1Name("texToTexParams1");
    static CCryNameR pParam2Name("texToTexParams2");
    static CCryNameR pParam3Name("texToTexParams3");

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
    gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    // Iterative directional blur: 1 iter: 8 taps, 64 taps, 2 iter: 512 taps, 4096 taps...
    float fSampleScale = 1.0f;
    for (int i = nIterationsMul; i >= 1; --i)
    {
        // 1st iteration (4 taps)

        gcpRendD3D->FX_PushRenderTarget(0, pTempRT, NULL);
        gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());

        uint32 nPasses;
        CShaderMan::s_shPostEffects->FXSetTechnique(pTechName);
        CShaderMan::s_shPostEffects->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        gRenDev->FX_SetState(GS_NODEPTHTEST);

        float fSampleSize = fSampleScale;

        // Set samples position
        float s1 = fSampleSize / (float) pTex->GetWidth();  // 2.0 better results on lower res images resizing
        float t1 = fSampleSize / (float) pTex->GetHeight();
        Vec2 vBlurDir;
        vBlurDir.x = s1 * vDir.x;
        vBlurDir.y = t1 * vDir.y;

        // Use rotated grid
        Vec4 pParams0 = Vec4(-vBlurDir.x * 4.0f, -vBlurDir.y * 4.0f, -vBlurDir.x * 3.0f, -vBlurDir.y * 3.0f);
        Vec4 pParams1 = Vec4(-vBlurDir.x * 2.0f, -vBlurDir.y * 2.0f, -vBlurDir.x, -vBlurDir.y);
        Vec4 pParams2 = Vec4(vBlurDir.x * 2.0f, vBlurDir.y * 2.0f, vBlurDir.x, vBlurDir.y);
        Vec4 pParams3 = Vec4(vBlurDir.x * 4.0f, vBlurDir.y * 4.0f, vBlurDir.x * 3.0f, vBlurDir.y * 3.0f);

        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam0Name, &pParams0, 1);
        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam1Name, &pParams1, 1);
        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam2Name, &pParams2, 1);
        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam3Name, &pParams3, 1);

        CShaderMan::s_shPostEffects->FXBeginPass(0);

        SetTexture(pTex, 0, FILTER_LINEAR, TADDR_BORDER);
        SetTexture(CTextureManager::Instance()->GetDefaultTexture("ScreenNoiseMap"), 1, FILTER_POINT, 0);

        DrawFullScreenTri(pTex->GetWidth(), pTex->GetHeight());

        CShaderMan::s_shPostEffects->FXEndPass();
        CShaderMan::s_shPostEffects->FXEnd();

        gcpRendD3D->FX_PopRenderTarget(0);

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        // 2nd iteration (4 x 4 taps)

        gcpRendD3D->FX_PushRenderTarget(0, pTex, NULL);
        gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());

        CShaderMan::s_shPostEffects->FXSetTechnique(pTechName);
        CShaderMan::s_shPostEffects->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        gRenDev->FX_SetState(GS_NODEPTHTEST);

        fSampleScale *= 0.75f;

        fSampleSize = fSampleScale;
        s1 = fSampleSize / (float) pTex->GetWidth();  // 2.0 better results on lower res images resizing
        t1 = fSampleSize / (float) pTex->GetHeight();
        vBlurDir.x = vDir.x * s1;
        vBlurDir.y = vDir.y * t1;

        pParams0 = Vec4(-vBlurDir.x * 4.0f, -vBlurDir.y * 4.0f, -vBlurDir.x * 3.0f, -vBlurDir.y * 3.0f);
        pParams1 = Vec4(-vBlurDir.x * 2.0f, -vBlurDir.y * 2.0f, -vBlurDir.x, -vBlurDir.y);
        pParams2 = Vec4(vBlurDir.x, vBlurDir.y, vBlurDir.x * 2.0f, vBlurDir.y * 2.0f);
        pParams3 = Vec4(vBlurDir.x * 3.0f, vBlurDir.y * 3.0f, vBlurDir.x * 4.0f, vBlurDir.y * 4.0f);

        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam0Name, &pParams0, 1);
        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam1Name, &pParams1, 1);
        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam2Name, &pParams2, 1);
        CShaderMan::s_shPostEffects->FXSetPSFloat(pParam3Name, &pParams3, 1);

        CShaderMan::s_shPostEffects->FXBeginPass(0);

        SetTexture(pTempRT, 0, FILTER_LINEAR, TADDR_BORDER);
        SetTexture(CTextureManager::Instance()->GetDefaultTexture("ScreenNoiseMap"), 1, FILTER_POINT, 0);

        DrawFullScreenTri(pTex->GetWidth(), pTex->GetHeight());

        CShaderMan::s_shPostEffects->FXEndPass();
        CShaderMan::s_shPostEffects->FXEnd();

        gcpRendD3D->FX_PopRenderTarget(0);

        fSampleScale *= 0.5f;
    }

    // Restore previous viewport
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    SAFE_DELETE(tpBlurTemp);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::TexBlurGaussian(CTexture* pTex, [[maybe_unused]] int nAmount, float fScale, float fDistribution, bool bAlphaOnly, CTexture* pMask, bool bSRGB, CTexture* pBlurTmp)
{
    if (!pTex)
    {
        return;
    }

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2]);

    PROFILE_LABEL_SCOPE("TEXBLUR_GAUSSIAN");

    if (bSRGB)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    CTexture* pTempRT = pBlurTmp;
    SDynTexture* tpBlurTemp = 0;
    if (!pBlurTmp)
    {
        tpBlurTemp = new SDynTexture(pTex->GetWidth(), pTex->GetHeight(), pTex->GetDstFormat(), eTT_2D,  FT_STATE_CLAMP | FT_USAGE_RENDERTARGET, "TempBlurRT");
        if (tpBlurTemp)
        {
            tpBlurTemp->Update(pTex->GetWidth(), pTex->GetHeight());
        }

        if (!tpBlurTemp || !tpBlurTemp->m_pTexture)
        {
            SAFE_DELETE(tpBlurTemp);
            return;
        }

        pTempRT = tpBlurTemp->m_pTexture;
    }

    PROFILE_SHADER_SCOPE;

    // Get current viewport
    int iTempX, iTempY, iWidth, iHeight;
    gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
    gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());

    Vec4 vWhite(1.0f, 1.0f, 1.0f, 1.0f);

    // TODO: Make test with Martin's idea about the horizontal blur pass with vertical offset.

    // only regular gaussian blur supporting masking
    static CCryNameTSCRC pTechName("GaussBlurBilinear");
    static CCryNameTSCRC pTechNameMasked("MaskedGaussBlurBilinear");
    static CCryNameTSCRC pTechName1("GaussAlphaBlur");

    //ShBeginPass(CShaderMan::m_shPostEffects, , FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    uint32 nPasses;
    CShaderMan::s_shPostEffects->FXSetTechnique((!bAlphaOnly) ? ((pMask) ? pTechNameMasked : pTechName) : pTechName1);
    CShaderMan::s_shPostEffects->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    gRenDev->FX_SetState(GS_NODEPTHTEST);

    // setup texture offsets, for texture sampling
    float s1 = 1.0f / (float) pTex->GetWidth();
    float t1 = 1.0f / (float) pTex->GetHeight();

    // Horizontal/Vertical pass params
    const int nSamples = 16;
    int nHalfSamples = (nSamples >> 1);

    Vec4 pHParams[32], pVParams[32], pWeightsPS[32];
    float pWeights[32], fWeightSum = 0;

    memset(pWeights, 0, sizeof(pWeights));

    int s;
    for (s = 0; s < nSamples; ++s)
    {
        if (fDistribution != 0.0f)
        {
            pWeights[s] = GaussianDistribution1D(s - nHalfSamples, fDistribution);
        }
        else
        {
            pWeights[s] = 0.0f;
        }
        fWeightSum += pWeights[s];
    }

    // normalize weights
    for (s = 0; s < nSamples; ++s)
    {
        pWeights[s] /= fWeightSum;
    }

    // set bilinear offsets
    for (s = 0; s < nHalfSamples; ++s)
    {
        float off_a = pWeights[s * 2];
        float off_b = ((s * 2 + 1) <= nSamples - 1) ? pWeights[s * 2 + 1] : 0;
        float a_plus_b = (off_a + off_b);
        if (a_plus_b == 0)
        {
            a_plus_b = 1.0f;
        }
        float offset = off_b / a_plus_b;

        pWeights[s] = off_a + off_b;
        pWeights[s] *= fScale;
        pWeightsPS[s] = vWhite * pWeights[s];

        float fCurrOffset = (float) s * 2 + offset - nHalfSamples;
        pHParams[s] = Vec4(s1 * fCurrOffset, 0, 0, 0);
        pVParams[s] = Vec4(0, t1 * fCurrOffset, 0, 0);
    }


    STexState sTexState = STexState(FILTER_LINEAR, true);
    static CCryNameR clampTCName("clampTC");
    static CCryNameR pParam0Name("psWeights");
    static CCryNameR pParam1Name("PI_psOffsets");

    Vec4 clampTC(0.0f, 1.0f, 0.0f, 1.0f);
    if (pTex->GetWidth() == gcpRendD3D->GetWidth() && pTex->GetHeight() == gcpRendD3D->GetHeight())
    {
        // clamp manually in shader since texture clamp won't apply for smaller viewport
        clampTC = Vec4(0.0f, gRenDev->m_RP.m_CurDownscaleFactor.x, 0.0f, gRenDev->m_RP.m_CurDownscaleFactor.y);
    }

    //SetTexture(pTex, 0, FILTER_LINEAR);
    CShaderMan::s_shPostEffects->FXSetPSFloat(clampTCName, &clampTC, 1);
    CShaderMan::s_shPostEffects->FXSetPSFloat(pParam0Name, pWeightsPS, nHalfSamples);

    //for(int p(1); p<= nAmount; ++p)
    {
        //Horizontal


        gcpRendD3D->FX_PushRenderTarget(0, pTempRT, NULL);
        gcpRendD3D->FX_SetActiveRenderTargets(false);// Avoiding invalid d3d error (due to deferred rt setup, when ping-pong'ing between RTs we can bump into RTs still bound when binding it as a SRV)
        gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());

        // !force updating constants per-pass! (dx10..)
        CShaderMan::s_shPostEffects->FXBeginPass(0);

        pTex->Apply(0, CTexture::GetTexState(sTexState));
        if (pMask)
        {
            pMask->Apply(1, CTexture::GetTexState(sTexState));
        }
        CShaderMan::s_shPostEffects->FXSetVSFloat(pParam1Name, pHParams, nHalfSamples);
        DrawFullScreenTri(pTex->GetWidth(), pTex->GetHeight());

        CShaderMan::s_shPostEffects->FXEndPass();

        gcpRendD3D->FX_PopRenderTarget(0);

        //Vertical
        gcpRendD3D->FX_PushRenderTarget(0, pTex, NULL);
        gcpRendD3D->FX_SetActiveRenderTargets(false);// Avoiding invalid d3d error (due to deferred rt setup, when ping-pong'ing between RTs we can bump into RTs still bound when binding it as a SRV)
        gcpRendD3D->RT_SetViewport(0, 0, pTex->GetWidth(), pTex->GetHeight());

        // !force updating constants per-pass! (dx10..)
        CShaderMan::s_shPostEffects->FXBeginPass(0);

        CShaderMan::s_shPostEffects->FXSetVSFloat(pParam1Name, pVParams, nHalfSamples);
        pTempRT->Apply(0, CTexture::GetTexState(sTexState));
        if (pMask)
        {
            pMask->Apply(1, CTexture::GetTexState(sTexState));
        }
        DrawFullScreenTri(pTex->GetWidth(), pTex->GetHeight());

        CShaderMan::s_shPostEffects->FXEndPass();

        gcpRendD3D->FX_PopRenderTarget(0);
    }

    CShaderMan::s_shPostEffects->FXEnd();

    //    ShEndPass( );

    // Restore previous viewport
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    //release dyntexture
    SAFE_DELETE(tpBlurTemp);

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::BeginStencilPrePass(const bool bAddToStencil, const bool bDebug, const bool bResetStencil, const uint8 nStencilRefReset)
{
    if (!bAddToStencil && !bResetStencil)
    {
        gcpRendD3D->m_nStencilMaskRef++;
    }

    if (gcpRendD3D->m_nStencilMaskRef > STENC_MAX_REF)
    {
        gcpRendD3D->EF_ClearTargetsImmediately(FRT_CLEAR_STENCIL);
        gcpRendD3D->m_nStencilMaskRef = 1;
    }

    gcpRendD3D->FX_SetStencilState(
        STENC_FUNC(FSS_STENCFUNC_ALWAYS) |
        STENCOP_FAIL(FSS_STENCOP_REPLACE) |
        STENCOP_ZFAIL(FSS_STENCOP_REPLACE) |
        STENCOP_PASS(FSS_STENCOP_REPLACE),
        bResetStencil ? nStencilRefReset : gcpRendD3D->m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFF
        );

    gcpRendD3D->FX_SetState(GS_STENCIL | GS_NODEPTHTEST | (!bDebug ? GS_COLMASK_NONE : 0));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::EndStencilPrePass()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::SetupStencilStates(int32 nStFunc)
{
    if (nStFunc < 0)
    {
        return;
    }

    gcpRendD3D->FX_SetStencilState(
        STENC_FUNC(nStFunc) |
        STENCOP_FAIL(FSS_STENCOP_KEEP) |
        STENCOP_ZFAIL(FSS_STENCOP_KEEP) |
        STENCOP_PASS(FSS_STENCOP_KEEP),
        gcpRendD3D->m_nStencilMaskRef, 0xFFFFFFFF, 0xFFFFFFFF);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::SetFillModeSolid(bool bEnable)
{
    if (bEnable)
    {
        if (gcpRendD3D->GetWireframeMode() > R_SOLID_MODE)
        {
            SStateRaster RS = gcpRendD3D->m_StatesRS[gcpRendD3D->m_nCurStateRS];
            RS.Desc.FillMode = D3D11_FILL_SOLID;
            gcpRendD3D->SetRasterState(&RS);
        }
    }
    else
    {
        if (gcpRendD3D->GetWireframeMode() == R_WIREFRAME_MODE)
        {
            SStateRaster RS = gcpRendD3D->m_StatesRS[gcpRendD3D->m_nCurStateRS];
            RS.Desc.FillMode = D3D11_FILL_WIREFRAME;
            gcpRendD3D->SetRasterState(&RS);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void SD3DPostEffectsUtils::DrawQuadFS(CShader* pShader, bool bOutputCamVec, int nWidth, int nHeight, float x0, float y0, float x1, float y1, float z)
{
    const Vec4 cQuadConst(1.0f / (float) nWidth, 1.0f / (float) nHeight, z, 1.0f);
    pShader->FXSetVSFloat(m_pQuadParams, &cQuadConst, 1);

    const Vec4 cQuadPosConst(x0, y0, x1, y1);
    pShader->FXSetVSFloat(m_pQuadPosParams, &cQuadPosConst, 1);

    if (bOutputCamVec)
    {
        UpdateFrustumCorners();
        const Vec4 vLT(m_vLT, 1.0f);
        pShader->FXSetVSFloat(m_pFrustumLTParams, &vLT, 1);
        const Vec4 vLB(m_vLB, 1.0f);
        pShader->FXSetVSFloat(m_pFrustumLBParams, &vLB, 1);
        const Vec4 vRT(m_vRT, 1.0f);
        pShader->FXSetVSFloat(m_pFrustumRTParams, &vRT, 1);
        const Vec4 vRB(m_vRB, 1.0f);
        pShader->FXSetVSFloat(m_pFrustumRBParams, &vRB, 1);
    }

    gcpRendD3D->FX_Commit();
    if (!FAILED(gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_C4B_T2F)))
    {
        gcpRendD3D->FX_SetVStream(0, gcpRendD3D->m_pQuadVB, 0, sizeof(SVF_P3F_C4B_T2F));
        gcpRendD3D->FX_DrawPrimitive(eptTriangleStrip, 0, gcpRendD3D->m_nQuadVBSize);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CD3D9Renderer::FX_PostProcessScene(bool bEnable)
{
    AZ_TRACE_METHOD();
    if (!gcpRendD3D)
    {
        return false;
    }

    if (bEnable)
    {
        PostProcessUtils().Create();
    }
    else
    if (!CRenderer::CV_r_PostProcess && CTexture::s_ptexBackBuffer)
    {
        PostProcessUtils().Release();
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

void CD3D9Renderer::GetReprojectionMatrix(Matrix44A& matReproj,
    const Matrix44A& matView,
    const Matrix44A& matProj,
    const Matrix44A& matPrevView,
    const Matrix44A& matPrevProj,
    float fFarPlane) const
{
    // Current camera screen-space to projection-space
    const Matrix44A matSc2Pc
        (2.f,  0.f, -1.f,  0.f,
        0.f,  2.f, -1.f,  0.f,
        0.f,  0.f,  1.f,  0.f,
        0.f,  0.f,  0.f,  1.f / fFarPlane);

    // Current camera view-space to projection-space
    const Matrix44A matVc2Pc
        (matProj.m00,  0.f,          0.f,         0.f,
        0.f,          matProj.m11,  0.f,         0.f,
        0.f,          0.f,          1.f,         0.f,
        0.f,          0.f,          0.f,         1.f);

    // Current camera projection-space to world-space
    const Matrix44A matPc2Wc = (matVc2Pc * matView).GetInverted();

    // Previous camera view-space to projection-space
    const Matrix44A matVp2Pp
        (matPrevProj.m00, 0.f,              0.f,                0.f,
        0.f,             matPrevProj.m11,  0.f,                0.f,
        0.f,             0.f,              1.f,                0.f,
        0.f,             0.f,              0.f,                1.f);

    // Previous camera world-space to projection-space
    const Matrix44A matWp2Pp = matVp2Pp * matPrevView;

    // Previous camera projection-space to texture-space
    const Matrix44A matPp2Tp
        (0.5f,         0.f,           0.5f,                     0.f,
        0.f,          0.5f,          0.5f,                     0.f,
        0.f,          0.f,           1.f,                      0.f,
        0.f,          0.f,           0.f,                      1.f);

    // Final reprojection matrix (from current camera screen-space to previous camera texture-space)
    matReproj = matPp2Tp * matWp2Pp * matPc2Wc * matSc2Pc;
}

void CD3D9Renderer::UpdatePreviousFrameMatrices()
{
    PreviousFrameMatrixSet& matrices = m_PreviousFrameMatrixSets[m_CurViewportID][m_CurRenderEye];

    matrices.m_WorldViewPosition = GetViewParameters().vOrigin;
    matrices.m_ViewMatrix = m_CameraMatrix;
    matrices.m_ViewNoTranslateMatrix = m_CameraZeroMatrix[m_RP.m_nProcessThreadID];
    matrices.m_ProjMatrix = m_ProjNoJitterMatrix;

    // Use next frame jitter so that motion vector calculation is stable with jitter. While it would
    // be best to remove jitter entirely, the shaders are already computing the clip space position of
    // the current fragment with jitter, so this a pragmatic compromise.
    SubpixelJitter::Sample sample = SubpixelJitter::EvaluateSample(SPostEffectsUtils::m_iFrameCounter + 1, (SubpixelJitter::Pattern)CV_r_AntialiasingTAAJitterPattern);

    Vec2 subpixelOffsetClipSpace;
    subpixelOffsetClipSpace.x = ((sample.m_subpixelOffset.x * 2.0) / (float)m_width) / m_RP.m_CurDownscaleFactor.x;
    subpixelOffsetClipSpace.y = ((sample.m_subpixelOffset.y * 2.0) / (float)m_height) / m_RP.m_CurDownscaleFactor.y;

    matrices.m_ProjMatrix.m20 += subpixelOffsetClipSpace.x;
    matrices.m_ProjMatrix.m21 += subpixelOffsetClipSpace.y;

    matrices.m_ViewProjMatrix = m_ViewProjNoJitterMatrix;
    matrices.m_ViewProjNoTranslateMatrix = m_CameraZeroMatrix[m_RP.m_nProcessThreadID] * matrices.m_ProjMatrix;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CPostEffectsMgr::Begin()
{
    PostProcessUtils().Log("### POST-PROCESSING BEGINS ### ");
    PostProcessUtils().m_pTimer = gEnv->pTimer;
    static EShaderQuality nPrevShaderQuality = eSQ_Low;
    static ERenderQuality nPrevRenderQuality = eRQ_Low;

    EShaderQuality nShaderQuality = (EShaderQuality) gcpRendD3D->EF_GetShaderQuality(eST_PostProcess);
    ERenderQuality nRenderQuality = gRenDev->m_RP.m_eQuality;
    if (nPrevShaderQuality != nShaderQuality || nPrevRenderQuality != nRenderQuality)
    {
        CPostEffectsMgr::Reset(true);
        nPrevShaderQuality = nShaderQuality;
        nPrevRenderQuality = nRenderQuality;
    }

    gcpRendD3D->ResetToDefault();

    SPostEffectsUtils::m_pCurDepthSurface = &gcpRendD3D->m_DepthBufferOrig;

    gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

    SPostEffectsUtils::m_fWaterLevel = gRenDev->m_p3DEngineCommon.m_OceanInfo.m_fWaterLevel;

    PostProcessUtils().SetFillModeSolid(true);
    PostProcessUtils().UpdateOverscanBorderAspectRatio();
}

void CPostEffectsMgr::End()
{
    gcpRendD3D->RT_SetViewport(PostProcessUtils().m_pScreenRect.left, PostProcessUtils().m_pScreenRect.top, PostProcessUtils().m_pScreenRect.right, PostProcessUtils().m_pScreenRect.bottom);

    gcpRendD3D->FX_ResetPipe();

    PostProcessUtils().SetFillModeSolid(false);

    const uint32 nThreadID = gRenDev->m_RP.m_nProcessThreadID;
    int recursiveLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(recursiveLevel >= 0);

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if (gRenDev->m_RP.m_TI[nThreadID].m_PersFlags & RBPF_RENDER_SCENE_TO_TEXTURE)
    {
        return;
    }
#endif // if AZ_RTT_ENABLE

    gcpRendD3D->UpdatePreviousFrameMatrices();

    const int kFloatMaxContinousInt = 0x1000000;  // 2^24
    const bool bStereo = gcpRendD3D->GetS3DRend().IsStereoEnabled();
    if (!bStereo || (bStereo && gRenDev->m_CurRenderEye == STEREO_EYE_RIGHT))
    {
        SPostEffectsUtils::m_iFrameCounter = (SPostEffectsUtils::m_iFrameCounter + 1) % kFloatMaxContinousInt;
    }

    PostProcessUtils().Log("### POST-PROCESSING ENDS ### ");
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CREPostProcess:: mfDraw([[maybe_unused]] CShader* ef, [[maybe_unused]] SShaderPass* sfm)
{
    CPostEffectsMgr* pPostMgr = PostEffectMgr();
    IF (!gcpRendD3D || !CRenderer::CV_r_PostProcess || pPostMgr->GetEffects().empty() || gcpRendD3D->GetWireframeMode() > R_SOLID_MODE, 0)
    {
        return 0;
    }

    // Skip hdr/post processing when rendering different camera views
    if ((gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_MIRRORCULL) || (gcpRendD3D->m_RP.m_nRendFlags & SHDF_CUBEMAPGEN))
    {
        return 0;
    }

    if (gcpRendD3D->m_bDeviceLost)
    {
        return 0;
    }

    IF (!CShaderMan::s_shPostEffects, 0)
    {
        return 0;
    }

    IF (!CTexture::IsTextureExist(CTexture::s_ptexBackBuffer), 0)
    {
        return 0;
    }

    IF (!CTexture::IsTextureExist(CTexture::s_ptexSceneTarget), 0)
    {
        return 0;
    }

    PROFILE_LABEL_SCOPE("POST EFFECTS");

    pPostMgr->Begin();

    gcpRendD3D->FX_ApplyShaderQuality(eST_PostProcess);

    for (CPostEffectItor pItor = pPostMgr->GetEffects().begin(), pItorEnd = pPostMgr->GetEffects().end(); pItor != pItorEnd; ++pItor)
    {
        CPostEffect* pCurrEffect = (*pItor);
        if (pCurrEffect->GetRenderFlags() & PSP_REQUIRES_UPDATE)
        {
            pCurrEffect->Update();
        }
    }

#ifndef _RELEASE
    CPostEffectDebugVec& activeEffects = pPostMgr->GetActiveEffectsDebug();
    CPostEffectDebugVec& activeParams = pPostMgr->GetActiveEffectsParamsDebug();
#endif

    AZStd::vector<CPostEffect*> effectsToRender;
    for (CPostEffectItor pItor = pPostMgr->GetEffects().begin(), pItorEnd = pPostMgr->GetEffects().end(); pItor != pItorEnd; ++pItor)
    {
        CPostEffect* effectToPreprocess = (*pItor);
        if (effectToPreprocess->Preprocess())
        {
            effectsToRender.push_back(effectToPreprocess);
        }
    }

    for (auto effectIter = effectsToRender.begin(); effectIter != effectsToRender.end(); ++effectIter)
    {
        CPostEffect* currentEffect = (*effectIter);
        uint32 nRenderFlags = currentEffect->GetRenderFlags();
        if (nRenderFlags & PSP_UPDATE_BACKBUFFER)
        {
            PostProcessUtils().CopyScreenToTexture(CTexture::s_ptexBackBuffer);
        }
        if (nRenderFlags & PSP_UPDATE_SCENE_SPECULAR)
        {
            bool optimizeRT = CRenderer::CV_r_SlimGBuffer == 1;
            // When optimization is on, we use a single channel format texture for specular. This copy requires full RGBA so use normal map render target instead.
            PostProcessUtils().CopyScreenToTexture(optimizeRT ? CTexture::s_ptexSceneNormalsMap : CTexture::s_ptexSceneSpecular);

        }
# ifndef _RELEASE
        SPostEffectsDebugInfo* pDebugInfo = NULL;
        for (uint32 i = 0, nNumEffects = activeEffects.size(); i < nNumEffects; ++i)
        {
            if ((pDebugInfo = &activeEffects[i]) && pDebugInfo->pEffect == currentEffect)
            {
                pDebugInfo->fTimeOut = POSTSEFFECTS_DEBUGINFO_TIMEOUT;
                break;
            }
            pDebugInfo = NULL;
        }
        if (pDebugInfo == NULL)
        {
            activeEffects.push_back(SPostEffectsDebugInfo(currentEffect));
        }
#endif
        if (CRenderer::CV_r_SkipNativeUpscale > 0 && AZStd::next(effectIter) == effectsToRender.end())
        {
            gcpRendD3D->FX_PopRenderTarget(0);
            gcpRendD3D->RT_SetViewport(0,0, gcpRendD3D->GetNativeWidth(), gcpRendD3D->GetNativeHeight());
            gcpRendD3D->FX_SetRenderTarget(0, gcpRendD3D->GetBackBuffer(), &gcpRendD3D->m_DepthBufferOrigMSAA);
            gcpRendD3D->FX_SetActiveRenderTargets();
        }
        currentEffect->Render();
    }

# ifndef _RELEASE
    if (CRenderer::CV_r_AntialiasingModeDebug > 0)
    {
        float mx = CTexture::s_ptexBackBuffer->GetWidth() >> 1, my = CTexture::s_ptexBackBuffer->GetHeight() >> 1;
        AZ::Vector2 systemCursorPositionNormalized = AZ::Vector2::CreateZero();
        AzFramework::InputSystemCursorRequestBus::EventResult(systemCursorPositionNormalized,
                                                              AzFramework::InputDeviceMouse::Id,
                                                              &AzFramework::InputSystemCursorRequests::GetSystemCursorPositionNormalized);
        mx = systemCursorPositionNormalized.GetX() * gEnv->pRenderer->GetWidth();
        my = systemCursorPositionNormalized.GetY() * gEnv->pRenderer->GetHeight();

        PostProcessUtils().CopyScreenToTexture(CTexture::s_ptexBackBuffer);
        static CCryNameTSCRC pszTechName("DebugPostAA");
        GetUtils().ShBeginPass(CShaderMan::s_shPostAA, pszTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        gcpRendD3D->SetCullMode(R_CULL_NONE);
        gcpRendD3D->FX_SetState(GS_NODEPTHTEST);
        const Vec4 vDebugParams(mx, my, 1.f, max(1.0f, (float) CRenderer::CV_r_AntialiasingModeDebug));
        static CCryNameR pszDebugParams("vDebugParams");
        CShaderMan::s_shPostAA->FXSetPSFloat(pszDebugParams, &vDebugParams, 1);
        GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);
        SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight());
        GetUtils().ShEndPass();
    }

    if (!activeEffects.empty() && CRenderer::CV_r_PostProcess >= 2) // Debug output for active post effects
    {
        SDrawTextInfo pDrawTexInfo;
        if (CRenderer::CV_r_PostProcess >= 2)
        {
            int nPosY = 20;
            pDrawTexInfo.color[0] = pDrawTexInfo.color[2] = 0.0f;
            pDrawTexInfo.color[1] = 1.0f;
            gcpRendD3D->Draw2dText(30, nPosY += 15, "Active post effects:", pDrawTexInfo);

            pDrawTexInfo.color[0] = pDrawTexInfo.color[1] = pDrawTexInfo.color[2] = 1.0f;
            for (uint32 i = 0, nNumEffects = activeEffects.size(); i < nNumEffects; ++i)
            {
                SPostEffectsDebugInfo& debugInfo = activeEffects[i];
                if (debugInfo.fTimeOut > 0.0f)
                {
                    gcpRendD3D->Draw2dText(30, nPosY += 10, debugInfo.pEffect->GetName(), pDrawTexInfo);
                }
                debugInfo.fTimeOut -= gEnv->pTimer->GetFrameTime();
            }
        }

        if (CRenderer::CV_r_PostProcess == 3)
        {
            StringEffectMap* pEffectsParamsUpdated = pPostMgr->GetDebugParamsUsedInFrame();
            if (pEffectsParamsUpdated)
            {
                if (!pEffectsParamsUpdated->empty())
                {
                    StringEffectMapItor pEnd = pEffectsParamsUpdated->end();
                    for (StringEffectMapItor pItor = pEffectsParamsUpdated->begin(); pItor != pEnd; ++pItor)
                    {
                        SPostEffectsDebugInfo* pDebugInfo = NULL;
                        for (uint32 p = 0, nNumParams = activeParams.size(); p < nNumParams; ++p)
                        {
                            if ((pDebugInfo = &activeParams[p]) && pDebugInfo->szParamName == (pItor->first))
                            {
                                pDebugInfo->fTimeOut = POSTSEFFECTS_DEBUGINFO_TIMEOUT;
                                break;
                            }
                            pDebugInfo = NULL;
                        }
                        if (pDebugInfo == NULL)
                        {
                            activeParams.push_back(SPostEffectsDebugInfo((pItor->first), (pItor->second != 0) ? pItor->second->GetParam() : 0.0f));
                        }
                    }
                    pEffectsParamsUpdated->clear();
                }

                int nPosX = 250, nPosY = 5;
                pDrawTexInfo.color[0] = pDrawTexInfo.color[2] = 0.0f;
                pDrawTexInfo.color[1] = 1.0f;
                gcpRendD3D->Draw2dText(nPosX, nPosY += 15, "Frame parameters:", pDrawTexInfo);

                pDrawTexInfo.color[0] = pDrawTexInfo.color[1] = pDrawTexInfo.color[2] = 1.0f;
                for (uint32 p = 0, nNumParams = activeParams.size(); p < nNumParams; ++p)
                {
                    SPostEffectsDebugInfo& debugInfo = activeParams[p];
                    if (debugInfo.fTimeOut > 0.0f)
                    {
                        char pNameAndValue[128];
                        sprintf_s(pNameAndValue, "%s: %.4f\n", debugInfo.szParamName.c_str(), debugInfo.fParamVal);
                        gcpRendD3D->Draw2dText(nPosX, nPosY += 10, pNameAndValue, pDrawTexInfo);
                    }
                    debugInfo.fTimeOut -= gEnv->pTimer->GetFrameTime();
                }
            }
        }
    }
#endif

    pPostMgr->End();

    return 1;
}
