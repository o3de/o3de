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

#include "RenderDll_precompiled.h"
#include "PostAA.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "DepthOfField.h"
#include "../../Common/Textures/TextureManager.h"
#include <Common/RenderCapabilities.h>

struct TemporalAAParameters
{
    TemporalAAParameters() {}

    Matrix44 m_reprojection;

    // Index ordering
    // 5  2  6
    // 1  0  3
    // 7  4  8
    float m_beckmannHarrisFilter[9];
    float m_useAntiFlickerFilter;
    float m_clampingFactor;
    float m_newFrameWeight;
};

bool CPostAA::Preprocess()
{
    // Disable PostAA for Dolby mode.
    static ICVar* DolbyCvar = gEnv->pConsole->GetCVar("r_HDRDolby");
    int DolbyCvarValue = DolbyCvar ? DolbyCvar->GetIVal() : eDVM_Disabled;
    return DolbyCvarValue == eDVM_Disabled;
}

void CPostAA::Render()
{
    gcpRendD3D->GetGraphicsPipeline().RenderPostAA();
}

PostAAPass::PostAAPass()
{
    AZ::RenderNotificationsBus::Handler::BusConnect();
}

PostAAPass::~PostAAPass()
{
    AZ::RenderNotificationsBus::Handler::BusDisconnect();
}

void PostAAPass::Init()
{
    m_TextureAreaSMAA = CTexture::ForName("EngineAssets/ScreenSpace/AreaTex.dds", FT_DONT_STREAM, eTF_Unknown);
    m_TextureSearchSMAA = CTexture::ForName("EngineAssets/ScreenSpace/SearchTex.dds", FT_DONT_STREAM, eTF_Unknown);
}

void PostAAPass::Shutdown()
{
    SAFE_RELEASE(m_TextureAreaSMAA);
    SAFE_RELEASE(m_TextureSearchSMAA);
}

void PostAAPass::OnRendererFreeResources(int flags)
{
    // If texture resources are about to be freed by the renderer
    if (flags & FRR_TEXTURES)
    {
        // Release the PostAA textures first so they do not leak
        Shutdown();
    }
}

void PostAAPass::Reset()
{
    
}

static bool IsTemporalRestartNeeded()
{
    // When we are activating a new viewport.
    static AZ::s32 s_LastViewportId = -1;
    if (gRenDev->m_CurViewportID != s_LastViewportId)
    {
        s_LastViewportId = gRenDev->m_CurViewportID;
        return true;
    }

    const AZ::s32 StaleFrameThresholdCount = 10;

    // When we exceed N frames without rendering TAA (e.g. we toggle it on and off).
    static AZ::s32 s_LastFrameCounter = 0;
    const bool bIsStale = (GetUtils().m_iFrameCounter - s_LastFrameCounter) > StaleFrameThresholdCount;
    s_LastFrameCounter = GetUtils().m_iFrameCounter;

    return bIsStale;
}

static float BlackmanHarris(Vec2 uv)
{
    return expf(-2.29f * (uv.x * uv.x + uv.y * uv.y));
}

static void BuildTemporalParameters(TemporalAAParameters& temporalAAParameters)
{
    Matrix44_tpl<f64> reprojection64;

    {
        Matrix44_tpl<f64> currViewProjMatrixInverse = Matrix44_tpl<f64>(gRenDev->m_ViewProjNoJitterMatrix).GetInverted();
        Matrix44_tpl<f64> prevViewProjMatrix = gRenDev->GetPreviousFrameMatrixSet().m_ViewProjMatrix;

        reprojection64 = currViewProjMatrixInverse * prevViewProjMatrix;

        Matrix44_tpl<f64> scaleBias1 = Matrix44_tpl<f64>(
            0.5, 0, 0, 0,
            0, -0.5, 0, 0,
            0, 0, 1, 0,
            0.5, 0.5, 0, 1);

        Matrix44_tpl<f64> scaleBias2 = Matrix44_tpl<f64>(
            2.0, 0, 0, 0,
            0, -2.0, 0, 0,
            0, 0, 1, 0,
            -1.0, 1.0, 0, 1);

        reprojection64 = scaleBias2 * reprojection64 * scaleBias1;
    }

    const size_t FILTER_WEIGHT_COUNT = 9;
    Vec2 filterWeights[FILTER_WEIGHT_COUNT] =
    {
        Vec2{  0.0f,  0.0f },
        Vec2{ -1.0f,  0.0f },
        Vec2{  0.0f, -1.0f },
        Vec2{  1.0f,  0.0f },
        Vec2{  0.0f,  1.0f },
        Vec2{ -1.0f, -1.0f },
        Vec2{  1.0f, -1.0f },
        Vec2{ -1.0f,  1.0f },
        Vec2{  1.0f,  1.0f }
    };

    const Vec2 temporalJitterOffset(gRenDev->m_TemporalJitterClipSpace.x * 0.5f, gRenDev->m_TemporalJitterClipSpace.y * 0.5f);
    for (size_t i = 0; i < FILTER_WEIGHT_COUNT; ++i)
    {
        temporalAAParameters.m_beckmannHarrisFilter[i] = BlackmanHarris((filterWeights[i] - temporalJitterOffset));
    }

    temporalAAParameters.m_reprojection = reprojection64;
    temporalAAParameters.m_useAntiFlickerFilter = (float)CRenderer::CV_r_AntialiasingTAAUseAntiFlickerFilter;
    temporalAAParameters.m_clampingFactor = CRenderer::CV_r_AntialiasingTAAClampingFactor;
    temporalAAParameters.m_newFrameWeight = AZStd::max(gRenDev->CV_r_AntialiasingTAANewFrameWeight, FLT_EPSILON);
}

void PostAAPass::RenderTemporalAA(
    CTexture* sourceTexture,
    CTexture* outputTarget,
    const DepthOfFieldParameters& depthOfFieldParameters)
{
    CShader* pShader = CShaderMan::s_shPostAA;
    PROFILE_LABEL_SCOPE("TAA");

    uint64 saveFlags_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3]);

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_AntialiasingTAAUseVarianceClamping)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    if (CRenderer::CV_r_HDREyeAdaptationMode == 2)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    }

    // Filter the CoC's when depth of field is enabled.
    if (depthOfFieldParameters.m_bEnabled)
    {
        gcpRendD3D->FX_PushRenderTarget(2, GetUtils().GetCoCCurrentTarget(), nullptr);

        GetUtils().SetTexture(GetUtils().GetCoCHistoryTarget(), 4, FILTER_LINEAR);

        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
    }

    if (IsTemporalRestartNeeded())
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
    }

    CTexture* currentTarget = GetUtils().GetTemporalCurrentTarget();
    CTexture* historyTarget = GetUtils().GetTemporalHistoryTarget();

    gcpRendD3D->FX_PushRenderTarget(0, outputTarget, nullptr);
    gcpRendD3D->FX_PushRenderTarget(1, currentTarget, nullptr);

    static const CCryNameTSCRC TechNameTAA("TAA");
    GetUtils().ShBeginPass(pShader, TechNameTAA, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    Vec4 vHDRSetupParams[5];
    gEnv->p3DEngine->GetHDRSetupParams(vHDRSetupParams);

    {
        TemporalAAParameters temporalAAParameters;
        BuildTemporalParameters(temporalAAParameters);

        {
            const float sharpening = max(0.5f + CRenderer::CV_r_AntialiasingTAASharpening, 0.5f); // Catmull-rom sharpening baseline is 0.5.

            static CCryNameR paramName("TemporalParams");
            Vec4 temporalParams[4];
            temporalParams[0] = Vec4(
                temporalAAParameters.m_useAntiFlickerFilter,
                temporalAAParameters.m_clampingFactor,
                temporalAAParameters.m_newFrameWeight,
                sharpening);

            temporalParams[1] = Vec4(
                0.0f,
                0.0f,
                0.0f,
                temporalAAParameters.m_beckmannHarrisFilter[0]);

            temporalParams[2] = Vec4(
                temporalAAParameters.m_beckmannHarrisFilter[1],
                temporalAAParameters.m_beckmannHarrisFilter[2],
                temporalAAParameters.m_beckmannHarrisFilter[3],
                temporalAAParameters.m_beckmannHarrisFilter[4]);

            temporalParams[3] = Vec4(
                temporalAAParameters.m_beckmannHarrisFilter[5],
                temporalAAParameters.m_beckmannHarrisFilter[6],
                temporalAAParameters.m_beckmannHarrisFilter[7],
                temporalAAParameters.m_beckmannHarrisFilter[8]);

            pShader->FXSetPSFloat(paramName, (const Vec4*)temporalParams, 4);
        }

        static CCryNameR szReprojMatrix("ReprojectionMatrix");
        pShader->FXSetPSFloat(szReprojMatrix, (Vec4*)temporalAAParameters.m_reprojection.GetData(), 4);

        static CCryNameR HDREyeAdaptation("HDREyeAdaptation");
        pShader->FXSetPSFloat(HDREyeAdaptation, CRenderer::CV_r_HDREyeAdaptationMode == 2 ? &vHDRSetupParams[4] : &vHDRSetupParams[3], 1);

        static CCryNameR DOF_FocusParams0("DOF_FocusParams0");
        pShader->FXSetPSFloat(DOF_FocusParams0, &depthOfFieldParameters.m_FocusParams0, 1);

        static CCryNameR DOF_FocusParams1("DOF_FocusParams1");
        pShader->FXSetPSFloat(DOF_FocusParams1, &depthOfFieldParameters.m_FocusParams1, 1);
    }

    GetUtils().SetTexture(sourceTexture, 0, FILTER_POINT);
    GetUtils().SetTexture(historyTarget, 1, FILTER_LINEAR);

    if (CTexture::s_ptexCurLumTexture)
    {
        if (!gRenDev->m_CurViewportID)
        {
            GetUtils().SetTexture(CTexture::s_ptexCurLumTexture, 2, FILTER_LINEAR);
        }
        else
        {
            GetUtils().SetTexture(CTexture::s_ptexHDRToneMaps[0], 2, FILTER_LINEAR);
        }
    }
    else
    {
        GetUtils().SetTexture(CTextureManager::Instance()->GetWhiteTexture(), 2, FILTER_LINEAR);
    }

    GetUtils().SetTexture(GetUtils().GetVelocityObjectRT(), 3, FILTER_POINT);
    GetUtils().SetTexture(CTexture::s_ptexZTarget, 5, FILTER_POINT);

    D3DShaderResourceView* depthSRV[1] = { gcpRendD3D->m_pZBufferDepthReadOnlySRV };
    gcpRendD3D->m_DevMan.BindSRV(eHWSC_Pixel, depthSRV, 14, 1);
    gcpRendD3D->FX_Commit();

    SD3DPostEffectsUtils::DrawFullScreenTri(outputTarget->GetWidth(), outputTarget->GetHeight());

    depthSRV[0] = nullptr;
    gcpRendD3D->m_DevMan.BindSRV(eHWSC_Pixel, depthSRV, 14, 1);
    gcpRendD3D->FX_Commit();

    GetUtils().ShEndPass();

    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->FX_PopRenderTarget(1);

    if (depthOfFieldParameters.m_bEnabled)
    {
        gcpRendD3D->FX_PopRenderTarget(2);
    }

    gcpRendD3D->m_RP.m_PersFlags2 |= RBPF2_NOPOSTAA;
    gRenDev->m_RP.m_FlagsShader_RT = saveFlags_RT;
}

void PostAAPass::Execute()
{
    PROFILE_LABEL_SCOPE("POST_AA");
    PROFILE_SHADER_SCOPE;

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3]);
    CTexture* inOutBuffer = CTexture::s_ptexSceneSpecular;

    // Slimming GBuffer process is done by encoding normals into format that can fit in only two channels 
    // and then uses the third extra channel to encode specular's Y channel (in YPbPbr format). The CbCr channels can be  
    // compressed down to two channels due to requiring only 4 bit prcision for them. This means we can't use the specular
    // texture for temporary copies. Thus requiring the need to pick other unused textures to be the replacement.
    if (CRenderer::CV_r_SlimGBuffer == 1)
    {
        if (CRenderer::CV_r_AntialiasingMode == eAT_FXAA || CRenderer::CV_r_AntialiasingMode == eAT_SMAA1TX)
        {
            inOutBuffer = CTexture::s_ptexSceneDiffuse;
        }
        else
        {
            inOutBuffer = CTexture::s_ptexSceneNormalsMap;
        }
    }

    static ICVar* DolbyCvar = gEnv->pConsole->GetCVar("r_HDRDolby");
    const int DolbyCvarValue = DolbyCvar ? DolbyCvar->GetIVal() : eDVM_Disabled;
    const bool bDolbyHDRMode = DolbyCvarValue > eDVM_Disabled;
    
    CTexture* currentRT = gcpRendD3D->FX_GetCurrentRenderTarget(0);
    if (currentRT == SPostEffectsUtils::AcquireFinalCompositeTarget(bDolbyHDRMode) && CRenderer::CV_r_SkipNativeUpscale)
    {
        gcpRendD3D->FX_PopRenderTarget(0);
        gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetNativeWidth(), gcpRendD3D->GetNativeHeight());
        gcpRendD3D->FX_SetRenderTarget(0, gcpRendD3D->GetBackBuffer(), nullptr);
        gcpRendD3D->FX_SetActiveRenderTargets();
    }

    bool useCurrentRTForAAOutput = (CRenderer::CV_r_SkipRenderComposites == 1);
    switch (CRenderer::CV_r_AntialiasingMode)
    {
    case eAT_SMAA1TX:
        RenderSMAA(inOutBuffer, &inOutBuffer, useCurrentRTForAAOutput);
        break;
    case eAT_FXAA:
        RenderFXAA(inOutBuffer, &inOutBuffer, useCurrentRTForAAOutput);
        break;
    case eAT_NOAA:
        break;
    }
    
    if (!CRenderer::CV_r_SkipRenderComposites)
    {
        RenderComposites(inOutBuffer);
    }

    gcpRendD3D->m_RP.m_PersFlags2 |= RBPF2_NOPOSTAA;
    CTexture::s_ptexBackBuffer->SetResolved(true);

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

void PostAAPass::RenderSMAA(CTexture* sourceTexture, CTexture** outputTexture, bool useCurrentRT)
{
    CTexture* pEdgesTex = CTexture::s_ptexSceneNormalsMap; // Reusing esram resident target
    
    // Need to use a different temporary texture for edge detection since it is using the normal map
    // as inout for slimming GBuffer 
    if(CRenderer::CV_r_SlimGBuffer == 1)
    {
        pEdgesTex = CTexture::s_ptexSceneNormalsBent;
    }
    
    CTexture* pBlendTex = CTexture::s_ptexSceneDiffuse;     // Reusing esram resident target (note that we access this FP16 RT using point filtering - full rate on GCN)
    if(CRenderer::CV_r_SlimGBuffer == 1)
    {
        pBlendTex = CTexture::s_ptexSceneSpecularAccMap;
    }

    CShader* pShader = CShaderMan::s_shPostAA;

    if (pEdgesTex && pBlendTex)
    {
        PROFILE_LABEL_SCOPE("SMAA1tx");
        const int iWidth = gcpRendD3D->GetWidth();
        const int iHeight = gcpRendD3D->GetHeight();

        ////////////////////////////////////////////////////////////////////////////////////////////////
        // 1st pass: generate edges texture
        {
            PROFILE_LABEL_SCOPE("Edge Generation");
            gcpRendD3D->FX_ClearTarget(pEdgesTex, Clr_Transparent);
            gcpRendD3D->FX_PushRenderTarget(0, pEdgesTex, &gcpRendD3D->m_DepthBufferOrig);
            gcpRendD3D->FX_SetActiveRenderTargets();

            static CCryNameTSCRC pszLumaEdgeDetectTechName("LumaEdgeDetectionSMAA");
            static const CCryNameR pPostAAParams("PostAAParams");

            gcpRendD3D->RT_SetViewport(0, 0, iWidth, iHeight);

            GetUtils().ShBeginPass(pShader, pszLumaEdgeDetectTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            gcpRendD3D->FX_SetState(GS_NODEPTHTEST);
            GetUtils().BeginStencilPrePass(false, true);

            GetUtils().SetTexture(sourceTexture, 0, FILTER_POINT);
            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(iWidth, iHeight);

            GetUtils().ShEndPass();

            GetUtils().EndStencilPrePass();

            gcpRendD3D->FX_PopRenderTarget(0);
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        // 2nd pass: generate blend texture
        {
            PROFILE_LABEL_SCOPE("Blend Weight Generation");
            gcpRendD3D->FX_ClearTarget(pBlendTex, Clr_Transparent);
            gcpRendD3D->FX_PushRenderTarget(0, pBlendTex, &gcpRendD3D->m_DepthBufferOrig);
            gcpRendD3D->FX_SetActiveRenderTargets();

            static CCryNameTSCRC pszBlendWeightTechName("BlendWeightSMAA");
            GetUtils().ShBeginPass(pShader, pszBlendWeightTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            gcpRendD3D->FX_SetState(GS_NODEPTHTEST);
            gcpRendD3D->FX_StencilTestCurRef(true, false);

            GetUtils().SetTexture(pEdgesTex, 0, FILTER_LINEAR);
            GetUtils().SetTexture(m_TextureAreaSMAA, 1, FILTER_LINEAR);
            GetUtils().SetTexture(m_TextureSearchSMAA, 2, FILTER_POINT);

            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(iWidth, iHeight);

            GetUtils().ShEndPass();

            gcpRendD3D->FX_PopRenderTarget(0);
        }

        CTexture* pDstRT = CTexture::s_ptexSceneNormalsMap;

        // Need to use a different temporary texture for edge detection since it is using the normal map
        // as inout for slimming GBuffer 
        if (CRenderer::CV_r_SlimGBuffer == 1)
        {
            pDstRT = pEdgesTex;
        }

        ////////////////////////////////////////////////////////////////////////////////////////////////
        // Final pass - blend neighborhood pixels
        {
            PROFILE_LABEL_SCOPE("Composite");
            gcpRendD3D->FX_PushRenderTarget(0, pDstRT, NULL);
            gcpRendD3D->FX_SetActiveRenderTargets();

            gcpRendD3D->FX_StencilTestCurRef(false);

            static CCryNameTSCRC pszBlendNeighborhoodTechName("NeighborhoodBlendingSMAA");
            GetUtils().ShBeginPass(pShader, pszBlendNeighborhoodTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            gcpRendD3D->FX_SetState(GS_NODEPTHTEST);
            GetUtils().SetTexture(pBlendTex, 0, FILTER_POINT);
            GetUtils().SetTexture(sourceTexture, 1, FILTER_LINEAR);

            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(iWidth, iHeight);

            GetUtils().ShEndPass();

            gcpRendD3D->FX_PopRenderTarget(0);
        }

        //////////////////////////////////////////////////////////////////////////
        // TEMPORAL SMAA 1TX
        {
            PROFILE_LABEL_SCOPE("TAA");
            CTexture* currentTarget = GetUtils().GetTemporalCurrentTarget();
            CTexture* historyTarget = GetUtils().GetTemporalHistoryTarget();

            if (useCurrentRT)
            {
                currentTarget = gcpRendD3D->FX_GetCurrentRenderTarget(0);
            }
            else
            {
                gcpRendD3D->FX_PushRenderTarget(0, currentTarget, nullptr);
            }

            static CCryNameTSCRC TechNameTAA("SMAA_TAA");
            GetUtils().ShBeginPass(pShader, TechNameTAA, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            {
                TemporalAAParameters temporalAAParameters;
                BuildTemporalParameters(temporalAAParameters);

                const float sharpening = max(1.0f + CRenderer::CV_r_AntialiasingNonTAASharpening, 1.0f);

                static CCryNameR szReprojMatrix("ReprojectionMatrix");
                pShader->FXSetPSFloat(szReprojMatrix, (Vec4*)temporalAAParameters.m_reprojection.GetData(), 4);

                Vec4 temporalParams(
                    temporalAAParameters.m_useAntiFlickerFilter,
                    temporalAAParameters.m_clampingFactor,
                    temporalAAParameters.m_newFrameWeight,
                    sharpening);

                static CCryNameR paramName("TemporalParams");
                pShader->FXSetPSFloat(paramName, &temporalParams, 1);
            }

            GetUtils().SetTexture(pDstRT, 0, FILTER_POINT);
            GetUtils().SetTexture(historyTarget, 1, FILTER_LINEAR);
            GetUtils().SetTexture(GetUtils().GetVelocityObjectRT(), 3, FILTER_POINT);
            GetUtils().SetTexture(CTexture::s_ptexZTarget, 5, FILTER_POINT);

            D3DShaderResourceView* depthSRV[1] = { gcpRendD3D->m_pZBufferDepthReadOnlySRV };
            gcpRendD3D->m_DevMan.BindSRV(eHWSC_Pixel, depthSRV, 14, 1);
            gcpRendD3D->FX_Commit();

            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(currentTarget->GetWidth(), currentTarget->GetHeight());

            depthSRV[0] = nullptr;
            gcpRendD3D->m_DevMan.BindSRV(eHWSC_Pixel, depthSRV, 14, 1);
            gcpRendD3D->FX_Commit();

            GetUtils().ShEndPass();

            if (!useCurrentRT)
            {
                gcpRendD3D->FX_PopRenderTarget(0);
            }

            *outputTexture = currentTarget;
        }
    }
}

void PostAAPass::RenderFXAA(CTexture* sourceTexture, CTexture** outputTexture, bool useCurrentRT)
{
    PROFILE_LABEL_SCOPE("FXAA");
    CTexture* currentTarget = CTexture::s_ptexSceneNormalsMap;
    if (useCurrentRT)
    {
        currentTarget = gcpRendD3D->FX_GetCurrentRenderTarget(0);
    }
    else
    {
        gcpRendD3D->FX_PushRenderTarget(0, currentTarget, nullptr);
    }

    CShader* pShader = CShaderMan::s_shPostAA;
    const f32 fWidthRcp = 1.0f / (float)gcpRendD3D->GetWidth();
    const f32 fHeightRcp = 1.0f / (float)gcpRendD3D->GetHeight();

    static CCryNameTSCRC TechNameFXAA("FXAA");
    GetUtils().ShBeginPass(pShader, TechNameFXAA, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    const Vec4 vRcpFrameOpt(-0.33f * fWidthRcp, -0.33f * fHeightRcp, 0.33f * fWidthRcp, 0.33f * fHeightRcp);// (1.0/sz.xy) * -0.33, (1.0/sz.xy) * 0.33. 0.5 -> softer
    const Vec4 vRcpFrameOpt2(-2.0f * fWidthRcp, -2.0f * fHeightRcp, 2.0f * fWidthRcp, 2.0f  * fHeightRcp);// (1.0/sz.xy) * -2.0, (1.0/sz.xy) * 2.0
    static CCryNameR pRcpFrameOptParam("RcpFrameOpt");
    pShader->FXSetPSFloat(pRcpFrameOptParam, &vRcpFrameOpt, 1);
    static CCryNameR pRcpFrameOpt2Param("RcpFrameOpt2");
    pShader->FXSetPSFloat(pRcpFrameOpt2Param, &vRcpFrameOpt2, 1);

    GetUtils().SetTexture(sourceTexture, 0, FILTER_LINEAR);

    SD3DPostEffectsUtils::DrawFullScreenTriWPOS(sourceTexture->GetWidth(), sourceTexture->GetHeight());
    gcpRendD3D->FX_Commit();

    GetUtils().ShEndPass();
    if (!useCurrentRT)
    {
        gcpRendD3D->FX_PopRenderTarget(0);
    }
    *outputTexture = currentTarget;
}

void PostAAPass::RenderComposites(CTexture* sourceTexture)
{
    PROFILE_LABEL_SCOPE("FLARES, GRAIN");

    CD3D9Renderer* rd = gcpRendD3D;
    
    rd->FX_SetStencilDontCareActions(0, true, true);
    bool isAfterPostProcessBucketEmpty = SRendItem::IsListEmpty(EFSLIST_AFTER_POSTPROCESS, rd->m_RP.m_nProcessThreadID, rd->m_RP.m_pRLD);
    
    bool isAuxGeomEnabled = false;
#if defined(ENABLE_RENDER_AUX_GEOM)
    isAuxGeomEnabled = CRenderer::CV_r_enableauxgeom == 1;
#endif
    
    //We may need to preserve the depth buffer in case there is something to render in the EFSLIST_AFTER_POSTPROCESS bucket.
    //It could be UI in the 3d world. If the bucket is empty ignore the depth buffer as it is not needed.
    //Also check if Auxgeom rendering is enabled in which case we preserve depth buffer.
    if (isAfterPostProcessBucketEmpty && !isAuxGeomEnabled)
    {
        rd->FX_SetDepthDontCareActions(0, true, true);
    }
    else
    {
        rd->FX_SetDepthDontCareActions(0, false, false);
    }
    
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    // enable sharpening controlled by r_AntialiasingNonTAASharpening here 
    // TAA applies sharpening in a different shader stage (TAAGatherHistory)
    if (!(gcpRendD3D->FX_GetAntialiasingType() & eAT_TAA_MASK) &&
        CRenderer::CV_r_AntialiasingNonTAASharpening > 0.f)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
    }

    if (gcpRendD3D->m_RP.m_PersFlags2 & RBPF2_LENS_OPTICS_COMPOSITE)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
        if (CRenderer::CV_r_FlaresChromaShift > 0.5f / (float)gcpRendD3D->GetWidth())  // only relevant if bigger than half pixel
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE3];
        }
    }

    if (CRenderer::CV_r_colorRangeCompression)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
    }
    else
    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE4];
    }

    if (!RenderCapabilities::SupportsTextureViews())
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
    }
    
    PostProcessUtils().SetSRGBShaderFlags();
    
    static const CCryNameTSCRC TechNameComposites("PostAAComposites");
    static const CCryNameTSCRC TechNameDebugMotion("PostAADebugMotion");

    CCryNameTSCRC techName = TechNameComposites;
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_MotionVectorsDebug)
    {
        techName = TechNameDebugMotion;
    }

    GetUtils().ShBeginPass(CShaderMan::s_shPostAA, techName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    {
        STexState texStateLinerSRGB(FILTER_LINEAR, true);
        texStateLinerSRGB.m_bSRGBLookup = true;

        bool bResolutionScaling = false;

#if defined(CRY_USE_METAL) || defined(ANDROID)
        {
            const Vec2& vDownscaleFactor = gcpRendD3D->m_RP.m_CurDownscaleFactor;
            bResolutionScaling = (vDownscaleFactor.x < .999999f) || (vDownscaleFactor.y < .999999f) == false;
            gcpRendD3D->SetCurDownscaleFactor(Vec2(1, 1));
        }
#endif
        if (!bResolutionScaling)
        {
            texStateLinerSRGB.SetFilterMode(FILTER_POINT);
        }

        sourceTexture->Apply(0, CTexture::GetTexState(texStateLinerSRGB));
    }

    gcpRendD3D->FX_PushWireframeMode(R_SOLID_MODE);
    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_MotionVectorsDebug)
    {
        // This is necessary because the depth target is currently bound, and we are reading from it
        // in this pass. Therefore, this pushes the same target without the depth buffer and then pops
        // it at the end.
        CTexture* texture = gcpRendD3D->FX_GetCurrentRenderTarget(0);
        AZ_Assert(texture, "No render target is bound.");
        gcpRendD3D->FX_PushRenderTarget(0, texture, nullptr);
        gcpRendD3D->FX_SetActiveRenderTargets();

        TemporalAAParameters temporalAAParameters;
        BuildTemporalParameters(temporalAAParameters);

        static CCryNameR szReprojMatrix("ReprojectionMatrix");
        CShaderMan::s_shPostAA->FXSetPSFloat(szReprojMatrix, (Vec4*)temporalAAParameters.m_reprojection.GetData(), 4);

        GetUtils().SetTexture(GetUtils().GetVelocityObjectRT(), 3, FILTER_POINT);
        GetUtils().SetTexture(CTexture::s_ptexZTarget, 5, FILTER_POINT);

        D3DShaderResourceView* depthSRV[1] = { gcpRendD3D->m_pZBufferDepthReadOnlySRV };
        gcpRendD3D->m_DevMan.BindSRV(eHWSC_Pixel, depthSRV, 14, 1);
        gcpRendD3D->FX_Commit();

        SPostEffectsUtils::DrawFullScreenTri(gcpRendD3D->GetOverlayWidth(), gcpRendD3D->GetOverlayHeight());

        depthSRV[0] = nullptr;
        gcpRendD3D->m_DevMan.BindSRV(eHWSC_Pixel, depthSRV, 14, 1);
        gcpRendD3D->FX_Commit();

        gcpRendD3D->FX_PopRenderTarget(0);
        gcpRendD3D->FX_SetActiveRenderTargets();
    }
    else
    {
        const Vec4 temporalParams(0, 0, 0, max(1.0f + CRenderer::CV_r_AntialiasingNonTAASharpening, 1.0f));
        static CCryNameR paramName("TemporalParams");
        CShaderMan::s_shPostAA->FXSetPSFloat(paramName, &temporalParams, 1);

        CTexture* pLensOpticsComposite = CTexture::s_ptexSceneTargetR11G11B10F[0];
        GetUtils().SetTexture(pLensOpticsComposite, 5, FILTER_POINT);
        if (gRenDev->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_SAMPLE3])
        {
            const Vec4 vLensOptics(1.0f, 1.0f, 1.0f, CRenderer::CV_r_FlaresChromaShift);
            static CCryNameR pLensOpticsParam("vLensOpticsParams");
            CShaderMan::s_shPostAA->FXSetPSFloat(pLensOpticsParam, &vLensOptics, 1);
        }

        // Apply grain (unfortunately final luminance texture doesn't get its final value baked, so have to replicate entire hdr eye adaption)
        {
            Vec4 vHDRSetupParams[5];
            gEnv->p3DEngine->GetHDRSetupParams(vHDRSetupParams);

            CEffectParam* m_pFilterGrainAmount = PostEffectMgr()->GetByName("FilterGrain_Amount");
            CEffectParam* m_pFilterArtifactsGrain = PostEffectMgr()->GetByName("FilterArtifacts_Grain");
            const float fFiltersGrainAmount = max(m_pFilterGrainAmount->GetParam(), m_pFilterArtifactsGrain->GetParam());
            const Vec4 v = Vec4(0, 0, 0, max(fFiltersGrainAmount, max(vHDRSetupParams[1].w, CRenderer::CV_r_HDRGrainAmount)));
            static CCryNameR szHDRParam("HDRParams");
            CShaderMan::s_shPostAA->FXSetPSFloat(szHDRParam, &v, 1);
            static CCryNameR szHDREyeAdaptationParam("HDREyeAdaptation");
            CShaderMan::s_shPostAA->FXSetPSFloat(szHDREyeAdaptationParam, &vHDRSetupParams[3], 1);

            GetUtils().SetTexture(CTextureManager::Instance()->GetDefaultTexture("FilmGrainMap"), 6, FILTER_POINT, 0);

            if (CTexture::s_ptexCurLumTexture)
            {
                GetUtils().SetTexture(CTexture::s_ptexCurLumTexture, 7, FILTER_POINT);
            }
#ifdef CRY_USE_METAL // Metal still expects a bound texture here!
            else
            {
                CTextureManager::Instance()->GetWhiteTexture()->Apply(7, FILTER_POINT);
            }
#endif
        }

        SPostEffectsUtils::DrawFullScreenTri(gcpRendD3D->GetOverlayWidth(), gcpRendD3D->GetOverlayHeight());
    }

    gcpRendD3D->FX_PopWireframeMode();

    GetUtils().ShEndPass();
    
    //UI should be coming in next. Since its in a gem we cant set loadactions in lyshine.
    //Hence we are setting it here. Stencil is setup as DoCare for load and store as it gets cleared at the start of UI rendering
    rd->FX_SetDepthDontCareActions(0, true, true); //We set this again here as all the actions get reset to conservative settings (do care) after the draw call
    rd->FX_SetStencilDontCareActions(0, false, false);
}

void PostAAPass::RenderFinalComposite(CTexture* sourceTexture)
{
    if (CShaderMan::s_shPostAA == NULL)
    {
        return;
    }

    PROFILE_LABEL_SCOPE("NATIVE_UPSCALE");
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE5]);
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    const bool renderSceneToTexture = (gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_RENDER_SCENE_TO_TEXTURE) != 0;
    if ((sourceTexture->GetWidth() != gRenDev->GetOverlayWidth() || sourceTexture->GetHeight() != gRenDev->GetOverlayHeight()) && !renderSceneToTexture)
#else
    if (sourceTexture->GetWidth() != gRenDev->GetOverlayWidth() || sourceTexture->GetHeight() != gRenDev->GetOverlayHeight())
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    if (!RenderCapabilities::SupportsTextureViews())
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
    }
    
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if (CRenderer::CV_r_FinalOutputAlpha == static_cast<int>(AzRTT::AlphaMode::ALPHA_DEPTH_BASED))
    {
        // enable sampling of depth target for alpha value
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    
    PostProcessUtils().SetSRGBShaderFlags();
    
    gcpRendD3D->FX_PushWireframeMode(R_SOLID_MODE);
    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    static CCryNameTSCRC pTechName("UpscaleImage");
    SPostEffectsUtils::ShBeginPass(CShaderMan::s_shPostAA, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    STexState texStateLinerSRGB(FILTER_LINEAR, true);
    texStateLinerSRGB.m_bSRGBLookup = true;
    sourceTexture->Apply(0, CTexture::GetTexState(texStateLinerSRGB));

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    if (CRenderer::CV_r_FinalOutputAlpha == static_cast<int>(AzRTT::AlphaMode::ALPHA_DEPTH_BASED))
    {
        CTexture::s_ptexZTarget->Apply(1, CTexture::GetTexState(STexState(FILTER_POINT, true)));
    }
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    SPostEffectsUtils::DrawFullScreenTri(gcpRendD3D->GetOverlayWidth(), gcpRendD3D->GetOverlayHeight());
    SPostEffectsUtils::ShEndPass();

    gcpRendD3D->FX_PopWireframeMode();
}
