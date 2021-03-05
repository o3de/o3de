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
#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"
#include "D3DStereo.h"
#include "../Common/Textures/TextureManager.h"

void CFilterSharpening::Render()
{
    PROFILE_LABEL_SCOPE("SHARPENING");

    const f32 fSharpenAmount = max(m_pAmount->GetParam(), CRenderer::CV_r_Sharpening + 1.0f);
    if (fSharpenAmount > 1e-6f)
    {
        GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[0]);
    }

    static CCryNameTSCRC pTechName("CA_Sharpening");
    GetUtils().ShBeginPass(CShaderMan::s_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    Vec4 pParams = Vec4(CRenderer::CV_r_ChromaticAberration, 0, 0, fSharpenAmount);
    static CCryNameR pParamName("psParams");
    CShaderMan::s_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

    GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_POINT);
    GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[0], 1);
    GetUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0.0f, &gcpRendD3D->m_FullResRect);

    GetUtils().ShEndPass();
}

void CFilterBlurring::Render()
{
    PROFILE_LABEL_SCOPE("BLURRING");

    float fAmount = m_pAmount->GetParam();
    fAmount = clamp_tpl<float>(fAmount, 0.0f, 1.0f);

    // maximum blur amount to have nice results
    const float fMaxBlurAmount = 5.0f;

    GetUtils().StretchRect(CTexture::s_ptexBackBuffer, CTexture::s_ptexBackBufferScaled[0]);
    GetUtils().TexBlurGaussian(CTexture::s_ptexBackBufferScaled[0], 1, 1.0f, LERP(0.0f, fMaxBlurAmount, fAmount), false, 0, false, CTexture::s_ptexBackBufferScaledTemp[0]);

    static CCryNameTSCRC pTechName("BlurInterpolation");
    GetUtils().ShBeginPass(CShaderMan::s_shPostEffects, pTechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);

    // Set PS default params
    Vec4 pParams = Vec4(0, 0, 0, fAmount * fAmount);
    static CCryNameR pParamName("psParams");
    CShaderMan::s_shPostEffects->FXSetPSFloat(pParamName, &pParams, 1);

    GetUtils().SetTexture(CTexture::s_ptexBackBufferScaled[0], 0);
    GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 1, FILTER_POINT);
    GetUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0.0f, &gcpRendD3D->m_FullResRect);

    GetUtils().ShEndPass();
}

bool CColorGrading::UpdateParams(SColorGradingMergeParams& pMergeParams)
{
    float fSharpenAmount = max(m_pSharpenAmount->GetParam(), 0.0f);

    // add cvar color_grading/color_grading_levels/color_grading_selectivecolor/color_grading_filters

    // Clamp to same Photoshop min/max values
    float fMinInput = clamp_tpl<float>(m_pMinInput->GetParam(), 0.0f, 255.0f);
    float fGammaInput = clamp_tpl<float>(m_pGammaInput->GetParam(), 0.0f, 10.0f);
    float fMaxInput = clamp_tpl<float>(m_pMaxInput->GetParam(), 0.0f, 255.0f);
    float fMinOutput = clamp_tpl<float>(m_pMinOutput->GetParam(), 0.0f, 255.0f);

    float fMaxOutput = clamp_tpl<float>(m_pMaxOutput->GetParam(), 0.0f, 255.0f);

    float fBrightness = m_pBrightness->GetParam();
    float fContrast = m_pContrast->GetParam();
    float fSaturation = m_pSaturation->GetParam() + m_pSaturationOffset->GetParam();
    Vec4  pFilterColor = m_pPhotoFilterColor->GetParamVec4() + m_pPhotoFilterColorOffset->GetParamVec4();
    float fFilterColorDensity = clamp_tpl<float>(m_pPhotoFilterColorDensity->GetParam() + m_pPhotoFilterColorDensityOffset->GetParam(), 0.0f, 1.0f);
    float fGrain = min(m_pGrainAmount->GetParam() + m_pGrainAmountOffset->GetParam(), 1.0f);

    Vec4 pSelectiveColor = m_pSelectiveColor->GetParamVec4();
    float fSelectiveColorCyans = clamp_tpl<float>(m_pSelectiveColorCyans->GetParam() * 0.01f, -1.0f, 1.0f);
    float fSelectiveColorMagentas = clamp_tpl<float>(m_pSelectiveColorMagentas->GetParam() * 0.01f, -1.0f, 1.0f);
    float fSelectiveColorYellows = clamp_tpl<float>(m_pSelectiveColorYellows->GetParam() * 0.01f, -1.0f, 1.0f);
    float fSelectiveColorBlacks = clamp_tpl<float>(m_pSelectiveColorBlacks->GetParam() * 0.01f, -1.0f, 1.0f);

    // Saturation matrix
    Matrix44 pSaturationMat;
    {
        float y = 0.3086f, u = 0.6094f, v = 0.0820f, s = clamp_tpl<float>(fSaturation, -1.0f, 100.0f);

        float a = (1.0f - s) * y + s;
        float b = (1.0f - s) * y;
        float c = (1.0f - s) * y;
        float d = (1.0f - s) * u;
        float e = (1.0f - s) * u + s;
        float f = (1.0f - s) * u;
        float g = (1.0f - s) * v;
        float h = (1.0f - s) * v;
        float i = (1.0f - s) * v + s;

        pSaturationMat.SetIdentity();
        pSaturationMat.SetRow(0, Vec3(a, d, g));
        pSaturationMat.SetRow(1, Vec3(b, e, h));
        pSaturationMat.SetRow(2, Vec3(c, f, i));
    }

    //  Brightness matrix
    Matrix44 pBrightMat;
    fBrightness = clamp_tpl<float>(fBrightness, 0.0f, 100.0f);
    pBrightMat.SetIdentity();
    pBrightMat.SetRow(0, Vec3(fBrightness, 0, 0));
    pBrightMat.SetRow(1, Vec3(0, fBrightness, 0));
    pBrightMat.SetRow(2, Vec3(0, 0, fBrightness));

    // Create Contrast matrix
    Matrix44 pContrastMat;
    {
        float c = clamp_tpl<float>(fContrast, -1.0f, 100.0f);
        pContrastMat.SetIdentity();
        pContrastMat.SetRow(0, Vec3(c, 0, 0));
        pContrastMat.SetRow(1, Vec3(0, c, 0));
        pContrastMat.SetRow(2, Vec3(0, 0, c));
        pContrastMat.SetColumn(3, 0.5f * Vec3(1.0f - c, 1.0f - c, 1.0f - c));
    }

    // Compose final color matrix and set fragment program constants
    Matrix44 pColorMat = pSaturationMat * (pBrightMat * pContrastMat);

    Vec4 pParams0 = Vec4(fMinInput, fGammaInput, fMaxInput, fMinOutput);
    Vec4 pParams1 = Vec4(fMaxOutput, fGrain, cry_random(0, 1023), cry_random(0, 1023));
    Vec4 pParams2 = Vec4(pFilterColor.x, pFilterColor.y, pFilterColor.z, fFilterColorDensity);
    Vec4 pParams3 = Vec4(pSelectiveColor.x, pSelectiveColor.y, pSelectiveColor.z, fSharpenAmount + 1.0f);
    Vec4 pParams4 = Vec4(fSelectiveColorCyans, fSelectiveColorMagentas, fSelectiveColorYellows, fSelectiveColorBlacks);

    // Enable corresponding shader variation
    pMergeParams.nFlagsShaderRT = gRenDev->m_RP.m_FlagsShader_RT;
    pMergeParams.nFlagsShaderRT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] | g_HWSR_MaskBit[HWSR_SAMPLE4] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    if (CRenderer::CV_r_colorgrading_levels && (fMinInput || fGammaInput || fMaxInput || fMinOutput || fMaxOutput))
    {
        pMergeParams.nFlagsShaderRT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    if (CRenderer::CV_r_colorgrading_filters && (fFilterColorDensity || fGrain || fSharpenAmount))
    {
        if (fFilterColorDensity)
        {
            pMergeParams.nFlagsShaderRT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
        }
        if (fGrain || fSharpenAmount)
        {
            pMergeParams.nFlagsShaderRT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
        }
    }

    if (CRenderer::CV_r_colorgrading_selectivecolor && (fSelectiveColorCyans || fSelectiveColorMagentas || fSelectiveColorYellows || fSelectiveColorBlacks))
    {
        pMergeParams.nFlagsShaderRT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
    }

    Matrix44 pColorMatFromUserAndTOD = GetUtils().GetColorMatrix();
    pColorMat = pColorMat  * pColorMatFromUserAndTOD;

    Vec4 pColorMatrix[3] =
    {
        Vec4(pColorMat.m00, pColorMat.m01, pColorMat.m02, pColorMat.m03),
        Vec4(pColorMat.m10, pColorMat.m11, pColorMat.m12, pColorMat.m13),
        Vec4(pColorMat.m20, pColorMat.m21, pColorMat.m22, pColorMat.m23),
    };

    pMergeParams.pColorMatrix[0] = pColorMatrix[0];
    pMergeParams.pColorMatrix[1] = pColorMatrix[1];
    pMergeParams.pColorMatrix[2] = pColorMatrix[2];
    pMergeParams.pLevels[0] = pParams0;
    pMergeParams.pLevels[1] = pParams1;
    pMergeParams.pFilterColor = pParams2;
    pMergeParams.pSelectiveColor[0] = pParams3;
    pMergeParams.pSelectiveColor[1] = pParams4;

    // Always using color charts

    if (gcpRendD3D->m_pColorGradingControllerD3D && (gEnv->IsCutscenePlaying() || (gRenDev->GetFrameID(false) % max(1, CRenderer::CV_r_ColorgradingChartsCache)) == 0))
    {
        if (!gcpRendD3D->m_pColorGradingControllerD3D->Update(&pMergeParams))
        {
            return false;
        }
    }

    // If using merged color grading with color chart disable regular color transformations in display - only need to use color chart
    pMergeParams.nFlagsShaderRT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE4]);

    //// Always using color charts - %SAMPLE5 for defining volume lookup
    //if( gcpRendD3D->m_pColorGradingControllerD3D && gcpRendD3D->m_pColorGradingControllerD3D->GetColorChart() && gcpRendD3D->m_pColorGradingControllerD3D->GetColorChart()->GetTexType() == eTT_3D)
    //  pMergeParams.nFlagsShaderRT |= g_HWSR_MaskBit[HWSR_SAMPLE5];

    return true;
}

void CColorGrading::Render()
{
    // Depreceated: to be removed / replaced by UberPostProcess shader
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CImageGhosting::Render()
{
    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    CTexture* pPrevFrame = CTexture::s_ptexPrevFrameScaled;
    CTexture* pPrevFrameRead = CTexture::s_ptexPrevFrameScaled;

    if (m_bInit)
    {
        m_bInit = false;
        ColorF clearColor(0, 0, 0, 0);

        gcpRendD3D->FX_ClearTarget(pPrevFrame, Clr_Transparent);
    }

    PROFILE_LABEL_SCOPE("IMAGE_GHOSTING");

    // 0.25ms / 0.4ms
    static CCryNameTSCRC TechName("ImageGhosting");
    GetUtils().ShBeginPass(CShaderMan::s_shPostEffects, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    // Update ghosting
    gcpRendD3D->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);

    static CCryNameR pParamNamePS("ImageGhostingParamsPS");
    Vec4 vParamsPS = Vec4(1, 1, max(0.0f, 1 - m_pAmount->GetParam()), gEnv->pTimer->GetFrameTime());
    CShaderMan::s_shPostEffects->FXSetPSFloat(pParamNamePS, &vParamsPS, 1);

    GetUtils().SetTexture(pPrevFrameRead, 0, FILTER_LINEAR);

    GetUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0.0f, &gcpRendD3D->m_FullResRect);

    GetUtils().ShEndPass();

    GetUtils().CopyScreenToTexture(CTexture::s_ptexBackBuffer);         // 0.25ms / 0.4 ms
    GetUtils().StretchRect(CTexture::s_ptexBackBuffer, pPrevFrame); // 0.25ms
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CUberGamePostProcess::Render()
{
    PROFILE_LABEL_SCOPE("UBER_GAME_POSTPROCESS");

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2]);

    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    if (m_nCurrPostEffectsMask & ePE_ChromaShift)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }
    //if( m_nCurrPostEffectsMask & ePE_RadialBlur )
    //  gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    if (m_nCurrPostEffectsMask & ePE_SyncArtifacts)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
    }

    static CCryNameTSCRC TechName("UberGamePostProcess");
    GetUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, TechName, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    CTexture* pMaskTex = static_cast<CParamTexture*>(m_pMask)->GetParamTexture();

    int32 nRenderState = GS_NODEPTHTEST;

    // Blend with backbuffer when user sets a mask
    if (pMaskTex)
    {
        nRenderState |= GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA;
    }

    gcpRendD3D->FX_SetState(nRenderState);

    Vec4 cColor = m_pColorTint->GetParamVec4();
    static CCryNameR pParamNamePS0("UberPostParams0");
    static CCryNameR pParamNamePS1("UberPostParams1");
    static CCryNameR pParamNamePS2("UberPostParams2");
    static CCryNameR pParamNamePS3("UberPostParams3");
    static CCryNameR pParamNamePS4("UberPostParams4");
    static CCryNameR pParamNamePS5("UberPostParams5");

    Vec4 vParamsPS[6] =
    {
        Vec4(m_pVSyncAmount->GetParam(), m_pInterlationAmount->GetParam(), m_pInterlationTiling->GetParam(), m_pInterlationRotation->GetParam()),
        //Vec4(m_pVSyncFreq->GetParam(), 1.0f + max(0.0f, m_pPixelationScale->GetParam()*0.25f), m_pNoise->GetParam()*0.25f, m_pChromaShiftAmount->GetParam() + m_pFilterChromaShiftAmount->GetParam()),
        Vec4(m_pVSyncFreq->GetParam(), 1.0f, m_pNoise->GetParam() * 0.25f, m_pChromaShiftAmount->GetParam() + m_pFilterChromaShiftAmount->GetParam()),
        Vec4(min(1.0f, m_pGrainAmount->GetParam() * 0.1f * 0.25f), m_pGrainTile->GetParam(), m_pSyncWavePhase->GetParam(), m_pSyncWaveFreq->GetParam()),
        Vec4(cColor.x, cColor.y, cColor.z, min(1.0f, m_pSyncWaveAmplitude->GetParam() * 0.01f)),
        Vec4(cry_random(0.0f, 1.0f), cry_random(0.0f, 1.0f), cry_random(0.0f, 1.0f),  cry_random(0.0f, 1.0f)),
        Vec4(0, 0, 0, 0)
    };

    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamNamePS0, &vParamsPS[0], 1);
    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamNamePS1, &vParamsPS[1], 1);
    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamNamePS2, &vParamsPS[2], 1);
    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamNamePS3, &vParamsPS[3], 1);
    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamNamePS4, &vParamsPS[4], 1);
    CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pParamNamePS5, &vParamsPS[5], 1);


    GetUtils().SetTexture(CTexture::s_ptexBackBuffer, 0, FILTER_LINEAR);
    GetUtils().SetTexture(CTextureManager::Instance()->GetDefaultTexture("ScreenNoiseMap"), 1, FILTER_LINEAR, 0);

    if (pMaskTex)
    {
        GetUtils().SetTexture(pMaskTex, 2, FILTER_LINEAR);
    }
    else
    {
        GetUtils().SetTexture(CTextureManager::Instance()->GetWhiteTexture(), 2, FILTER_LINEAR);
    }

    GetUtils().DrawFullScreenTri(CTexture::s_ptexBackBuffer->GetWidth(), CTexture::s_ptexBackBuffer->GetHeight(), 0.0f, &gcpRendD3D->m_FullResRect);

    GetUtils().ShEndPass();

    m_nCurrPostEffectsMask = 0;
    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

void CSoftAlphaTest::Render()
{
    PROFILE_LABEL_SCOPE("SOFT ALPHA TEST");

    IRenderElement* pPrevRE = gRenDev->m_RP.m_pRE;
    gRenDev->m_RP.m_pRE = NULL;

    PostProcessUtils().CopyScreenToTexture(CTexture::s_ptexSceneNormalsMap);

    gcpRendD3D->FX_ProcessSoftAlphaTestRenderLists();

    gRenDev->m_RP.m_pRE = pPrevRE;
}
