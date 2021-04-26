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

// Todo:
//  -   This is deprecated, will be refactored this into something like UberPostProcess
//  - Will contain all constant enabled post processes (edgeAA / sunShafts / color charts)


#include "RenderDll_precompiled.h"
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"

bool CSunShafts::Preprocess()
{
    return false;
}

bool CSunShafts::IsVisible()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);

    // We need to check every case and compare against merged post effects case

    m_bShaftsEnabled = true;

    if (!bQualityCheck)
    {
        m_bShaftsEnabled &= false;
    }

    if (gEnv->p3DEngine->GetSunColor().len2() < 0.01f)
    {
        m_bShaftsEnabled &= false;
    }

    if (m_pShaftsAmount->GetParam() < 0.01f && m_pRaysAmount->GetParam() < 0.01f)
    {
        m_bShaftsEnabled &= false;
    }

    // sun behind camera, can skip post process
    const float fSunVisThreshold = 0.45f;
    float fLdotV = gEnv->p3DEngine->GetSunDirNormalized().dot(gRenDev->GetViewParameters().vZ);
    if (fLdotV > fSunVisThreshold)
    {
        m_bShaftsEnabled &= false;
    }

    if (CRenderer::CV_r_sunshafts && IsActive())
    {
        // Disable for interiors
        uint32 nCamVisAreaFlags = gRenDev->m_p3DEngineCommon.m_pCamVisAreaInfo.nFlags;
        if ((nCamVisAreaFlags& S3DEngineCommon::VAF_EXISTS_FOR_POSITION) && !(nCamVisAreaFlags & (S3DEngineCommon::VAF_CONNECTED_TO_OUTDOOR | S3DEngineCommon::VAF_AFFECTED_BY_OUT_LIGHTS)))
        {
            m_bShaftsEnabled &= false;
        }
        else
        {
            m_bShaftsEnabled &= true;
        }
    }
    else
    {
        m_bShaftsEnabled &= false;
    }

    // Check if shafts occluded - if so skip them - todo: fade in/out shafts amount
    if (CRenderer::CV_r_sunshafts > 1 && m_bShaftsEnabled)
    {
        if (!m_pOcclQuery)
        {
            Initialize();
        }

        //bool bReady = m_pOcclQuery->IsReady();
        bool bSunShaftsVisible =  ((int)m_nVisSampleCount > (CTexture::s_ptexBackBuffer->GetWidth() * CTexture::s_ptexBackBuffer->GetHeight()  / 100)); // || !bReady;
        m_bShaftsEnabled &= bSunShaftsVisible;

        m_nVisSampleCount = m_pOcclQuery->GetVisibleSamples(CRenderer::CV_r_sunshafts == 2);
        if (!m_pOcclQuery->GetDrawFrame() || m_pOcclQuery->IsReady())
        {
            gRenDev->m_cEF.mfRefreshSystemShader("Sunshafts", CShaderMan::s_shPostSunShafts);

            PROFILE_LABEL_SCOPE("SUNSHAFTS OCCLUSION");

            static CCryNameTSCRC pTechName("OcclCheckTechnique");

            PostProcessUtils().ShBeginPass(CShaderMan::s_shPostSunShafts, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            gcpRendD3D->SetCullMode(R_CULL_NONE);
            gcpRendD3D->FX_SetState(GS_DEPTHFUNC_LEQUAL | GS_COLMASK_NONE);

            m_pOcclQuery->BeginQuery();
            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 1.0f);
            m_pOcclQuery->EndQuery();
            PostProcessUtils().ShEndPass();
        }
    }

    return m_bShaftsEnabled;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
bool CSunShafts::MergedSceneDownsampleAndSunShaftsMaskGen(CTexture* pSceneSrc, CTexture* pSceneDst, CTexture* pSunShaftsMaskDst)
{
    if (!pSceneSrc ||
        !pSceneDst ||
        !pSunShaftsMaskDst)
    {
        return false;
    }

    PROFILE_LABEL_SCOPE("SCENE_DOWNSAMPLE_SUNSHAFTS_MASK_GEN");

    gRenDev->m_cEF.mfRefreshSystemShader("Sunshafts", CShaderMan::s_shPostSunShafts);

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= (g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    int iTempX, iTempY, iWidth, iHeight;
    gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    gcpRendD3D->FX_PushRenderTarget(0, pSceneDst, NULL);
    gcpRendD3D->FX_PushRenderTarget(1, pSunShaftsMaskDst, NULL);
    gcpRendD3D->FX_SetColorDontCareActions(0, true, false);
    gcpRendD3D->FX_SetColorDontCareActions(1, true, false);

    gcpRendD3D->RT_SetViewport(0, 0, pSceneDst->GetWidth(), pSceneDst->GetHeight());

    static CCryNameTSCRC pTech0Name("MergedTexToTexAndSunShaftsMaskGen");

    gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostSunShafts, pTech0Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    PostProcessUtils().SetTexture(pSceneSrc, 0, FILTER_LINEAR);
    PostProcessUtils().SetTexture(CTexture::s_ptexZTargetScaled, 1, FILTER_POINT);

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);
    PostProcessUtils().DrawFullScreenTri(pSceneDst->GetWidth(), pSceneDst->GetHeight());

    PostProcessUtils().ShEndPass();

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;

    // Restore previous viewport
    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->FX_PopRenderTarget(1);
    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

bool CSunShafts::SunShaftsGen(CTexture* pSunShafts, CTexture* pPingPongRT)
{
    PROFILE_LABEL_SCOPE("SUNSHAFTS_GEN");

    gRenDev->m_cEF.mfRefreshSystemShader("Sunshafts", CShaderMan::s_shPostSunShafts);

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= (g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    int iTempX, iTempY, iWidth, iHeight;
    gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);


    //moved here because used later on
    static CCryNameR pParam1Name("SunShafts_SunPos");
    Vec3 pSunPos = gEnv->p3DEngine->GetSunDir() * 1000.0f;
    Vec4 pParamSunPos = Vec4(pSunPos, 1.0f);

    // no need to waste gpu to compute sun screen pos
    Vec4 pSunPosScreen = PostProcessUtils().m_pViewProj * pParamSunPos;
    pSunPosScreen.x = ((pSunPosScreen.x + pSunPosScreen.w) * 0.5f) / (1e-6f + pSunPosScreen.w);
    pSunPosScreen.y = ((-pSunPosScreen.y + pSunPosScreen.w) * 0.5f) / (1e-6f + pSunPosScreen.w);
    pSunPosScreen.w = gEnv->p3DEngine->GetSunDirNormalized().dot(PostProcessUtils().m_pViewProj.GetRow(2));

    Vec4 pShaftParams(0, 0, 0, 0);
    static CCryNameR pParam2Name("PI_sunShaftsParams");

    /////////////////////////////////////////////
    // Create shafts mask texture. Gmem path has already this done since it merges scene downsample with SS mask gen.
    if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
    {
        gcpRendD3D->FX_PushRenderTarget(0, pSunShafts, NULL);
        gcpRendD3D->FX_SetColorDontCareActions(0, true, false);
        gcpRendD3D->RT_SetViewport(0, 0, pSunShafts->GetWidth(), pSunShafts->GetHeight());

        static CCryNameTSCRC pTech0Name("SunShaftsMaskGen");

        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

        PostProcessUtils().ShBeginPass(CShaderMan::s_shPostSunShafts, pTech0Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

        gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

        // Get sample size ratio (based on empirical "best look" approach)
        float fSampleSize = ((float)CTexture::s_ptexBackBuffer->GetWidth() / (float)pSunShafts->GetWidth()) * 0.5f;

        // Set samples position
        float s1 = fSampleSize / (float) CTexture::s_ptexBackBuffer->GetWidth();  // 2.0 better results on lower res images resizing
        float t1 = fSampleSize / (float) CTexture::s_ptexBackBuffer->GetHeight();
        // Use rotated grid
        Vec4 pParams0 = Vec4(s1 * 0.95f, t1 * 0.25f, -s1 * 0.25f, t1 * 0.96f);
        Vec4 pParams1 = Vec4(-s1 * 0.96f, -t1 * 0.25f, s1 * 0.25f, -t1 * 0.96f);

        static CCryNameR pParam3Name("texToTexParams0");
        static CCryNameR pParam4Name("texToTexParams1");

        CShaderMan::s_shPostSunShafts->FXSetPSFloat(pParam3Name, &pParams0, 1);
        CShaderMan::s_shPostSunShafts->FXSetPSFloat(pParam4Name, &pParams1, 1);

        PostProcessUtils().SetTexture(CTexture::s_ptexZTargetScaled, 0, FILTER_POINT);
        PostProcessUtils().SetTexture(CTexture::s_ptexHDRTargetScaled[0], 1, (gRenDev->m_RP.m_eQuality >= eRQ_High) ? FILTER_POINT : FILTER_LINEAR);

        gcpRendD3D->FX_SetState(GS_NODEPTHTEST);
        PostProcessUtils().DrawFullScreenTri(pSunShafts->GetWidth(), pSunShafts->GetHeight());

        PostProcessUtils().ShEndPass();

        gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;

        // Restore previous viewport
        gcpRendD3D->FX_PopRenderTarget(0);
        gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);
    }
    /////////////////////////////////////////////
    // Apply local radial blur to shafts mask

    gcpRendD3D->FX_SetActiveRenderTargets();
    gcpRendD3D->FX_PushRenderTarget(0, pPingPongRT, NULL);
    gcpRendD3D->FX_SetColorDontCareActions(0, true, false);
    gcpRendD3D->RT_SetViewport(0, 0, pSunShafts->GetWidth(), pSunShafts->GetHeight());
    //gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

    static CCryNameTSCRC pTech1Name("SunShaftsGen");
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostSunShafts, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    //moved to top from here
    static CCryNameR pParam0Name("SunShafts_ViewProj");
    CShaderMan::s_shPostSunShafts->FXSetVSFloat(pParam0Name, (Vec4*) PostProcessUtils().m_pViewProj.GetData(), 4);
    CShaderMan::s_shPostSunShafts->FXSetVSFloat(pParam1Name, &pParamSunPos, 1);

    // big radius, project until end of screen
    pShaftParams.x = 0.1f;
    pShaftParams.y = clamp_tpl<float>(m_pRaysAttenuation->GetParam(), 0.0f, 10.0f);

    CShaderMan::s_shPostSunShafts->FXSetPSFloat(pParam2Name, &pShaftParams, 1);
    CShaderMan::s_shPostSunShafts->FXSetPSFloat(pParam1Name, &pSunPosScreen, 1);
    PostProcessUtils().SetTexture(pSunShafts, 0, FILTER_LINEAR);
    PostProcessUtils().DrawFullScreenTri(pSunShafts->GetWidth(), pSunShafts->GetHeight());

    PostProcessUtils().ShEndPass();

    gcpRendD3D->FX_PopRenderTarget(0);

    gcpRendD3D->FX_SetActiveRenderTargets();
    gcpRendD3D->FX_PushRenderTarget(0, pSunShafts, NULL);
    gcpRendD3D->FX_SetColorDontCareActions(0, true, false);

    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostSunShafts, pTech1Name, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    // interpolate between projections
    pShaftParams.x = 0.025f;
    CShaderMan::s_shPostSunShafts->FXSetPSFloat(pParam2Name, &pShaftParams, 1);
    CShaderMan::s_shPostSunShafts->FXSetPSFloat(pParam1Name, &pSunPosScreen, 1);
    PostProcessUtils().SetTexture(pPingPongRT, 0, FILTER_LINEAR);
    PostProcessUtils().DrawFullScreenTri(pSunShafts->GetWidth(), pSunShafts->GetHeight());

    PostProcessUtils().ShEndPass();

    gcpRendD3D->FX_PopRenderTarget(0);

    gcpRendD3D->FX_SetActiveRenderTargets();

    gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];

    return true;
}

// DEPRECATED
void CSunShafts::Render()
{
    PROFILE_SHADER_SCOPE;
    PROFILE_LABEL_SCOPE("MERGED_SUNSHAFTS_COLORCORRECTION");

    gRenDev->m_cEF.mfRefreshSystemShader("Sunshafts", CShaderMan::s_shPostSunShafts);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Update color grading

    bool bColorGrading = false;

    SColorGradingMergeParams pMergeParams;
    if (CRenderer::CV_r_colorgrading && CRenderer::CV_r_colorgrading_charts)
    {
        CColorGrading* pColorGrad = 0;
        if (!PostEffectMgr()->GetEffects().empty())
        {
            pColorGrad =  static_cast<CColorGrading*>(PostEffectMgr()->GetEffect(ePFX_ColorGrading));
        }

        if (pColorGrad && pColorGrad->UpdateParams(pMergeParams))
        {
            bColorGrading = true;
        }
    }

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= (g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    int iTempX, iTempY, iWidth, iHeight;
    gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

    CTexture* pBackBufferTex = CTexture::s_ptexBackBuffer;

    static CCryNameR pParam1Name("SunShafts_SunPos");
    Vec3 pSunPos = gEnv->p3DEngine->GetSunDir() * 1000.0f;
    Vec4 pParamSunPos = Vec4(pSunPos, 1.0f);

    // Compute sun screen pos on cpu side
    Vec4 pSunPosScreen = PostProcessUtils().m_pViewProj * pParamSunPos;
    pSunPosScreen.x = ((pSunPosScreen.x + pSunPosScreen.w) * 0.5f) / pSunPosScreen.w;
    pSunPosScreen.y = ((-pSunPosScreen.y + pSunPosScreen.w) * 0.5f) / pSunPosScreen.w;
    pSunPosScreen.w = gEnv->p3DEngine->GetSunDirNormalized().dot(PostProcessUtils().m_pViewProj.GetRow(2));

    /////////////////////////////////////////////
    // Create shafts mask texture

    uint32 nWidth = CTexture::s_ptexBackBufferScaled[1]->GetWidth();
    uint32 nHeight = CTexture::s_ptexBackBufferScaled[1]->GetHeight();
    if (gRenDev->m_RP.m_eQuality >= eRQ_High)
    {
        nWidth = CTexture::s_ptexBackBufferScaled[0]->GetWidth();
        nHeight = CTexture::s_ptexBackBufferScaled[0]->GetHeight();
    }

    SDynTexture* pSunShaftsRT = new SDynTexture(nWidth, nHeight, CTexture::s_ptexBackBufferScaled[1]->GetDstFormat(), eTT_2D, FT_STATE_CLAMP | FT_USAGE_RENDERTARGET, "TempBlurRT");
    if (!pSunShaftsRT)
    {
        return;
    }

    pSunShaftsRT->Update(nWidth, nHeight);
    if (!pSunShaftsRT->m_pTexture)
    {
        SAFE_DELETE(pSunShaftsRT);
        return;
    }

    Vec4 pShaftParams(0, 0, 0, 0);
    static CCryNameR pParam2Name("PI_sunShaftsParams");
    if (m_bShaftsEnabled)
    {
        SunShaftsGen(pSunShaftsRT->m_pTexture);
    }

    CTexture* pSunShafts = pSunShaftsRT->m_pTexture;

    /////////////////////////////////////////////
    // Display sun shafts

    gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

    CColorGradingControllerD3D* pCtrl = gcpRendD3D->m_pColorGradingControllerD3D;
    CTexture* pTexColorChar = pCtrl ? pCtrl->GetColorChart() : 0;

    // todo: should always use volume lookup (1 less shader combination)
    if (bColorGrading && pTexColorChar)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
        if (pTexColorChar->GetTexType() == eTT_3D)
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
        }
    }

    static CCryNameTSCRC pTech2Name("SunShaftsDisplay");
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostSunShafts, pTech2Name, FEF_DONTSETSTATES);   //FEF_DONTSETTEXTURES |

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    const float fSunVisThreshold = 0.45f;
    float fLdotV = clamp_tpl<float>(-(gEnv->p3DEngine->GetSunDirNormalized().dot(gRenDev->GetViewParameters().vZ) - fSunVisThreshold) * 4.0f, 0.0f, 1.0f);
    pShaftParams.x = clamp_tpl<float>(m_pShaftsAmount->GetParam() * fLdotV, 0.0f, 1.0f);
    pShaftParams.y = clamp_tpl<float>(m_pRaysAmount->GetParam(), 0.0f, 10.0f);
    CShaderMan::s_shPostSunShafts->FXSetPSFloat(pParam2Name, &pShaftParams, 1);

    static CCryNameR pParam5Name("SunShafts_SunCol");
    Vec4 pRaysCustomCol = m_pRaysCustomCol->GetParamVec4();
    Vec3 pSunColor = gEnv->p3DEngine->GetSunColor();
    pSunColor.Normalize();
    pSunColor.SetLerp(Vec3(pRaysCustomCol.x, pRaysCustomCol.y, pRaysCustomCol.z), pSunColor, m_pRaysSunColInfluence->GetParam());

    Vec4 pShaftsSunCol(pSunColor, 1);
    CShaderMan::s_shPostSunShafts->FXSetPSFloat(pParam5Name, &pShaftsSunCol, 1);

    if (pTexColorChar)
    {
        PostProcessUtils().SetTexture(pTexColorChar, 0, FILTER_LINEAR);
    }

    PostProcessUtils().SetTexture(pBackBufferTex, 1, (gRenDev->m_RP.m_FlagsShader_RT & g_HWSR_MaskBit[HWSR_SAMPLE1]) ? FILTER_LINEAR : FILTER_POINT);
    if (pSunShafts)
    {
        PostProcessUtils().SetTexture(pSunShafts, 2, FILTER_LINEAR);
    }

    PostProcessUtils().SetTexture(CTexture::s_ptexZTarget, 4, FILTER_POINT);

    CShaderMan::s_shPostSunShafts->FXSetPSFloat(pParam1Name, &pParamSunPos, 1);

    PostProcessUtils().DrawFullScreenTri(pBackBufferTex->GetWidth(), pBackBufferTex->GetHeight());


    PostProcessUtils().ShEndPass();

    SAFE_DELETE(pSunShaftsRT);
    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

