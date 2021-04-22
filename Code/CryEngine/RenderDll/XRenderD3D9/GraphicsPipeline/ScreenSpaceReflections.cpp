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
#include "ScreenSpaceReflections.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "../../Common/ReverseDepth.h"


void CScreenSpaceReflectionsPass::Init()
{
}

void CScreenSpaceReflectionsPass::Shutdown()
{
}

void CScreenSpaceReflectionsPass::Reset()
{
    m_passRaytracing.Reset();
    m_passComposition.Reset();
    m_passCopy.Reset();
    m_passDownsample0.Reset();
    m_passDownsample1.Reset();
    m_passDownsample2.Reset();
    m_passBlur0.Reset();
    m_passBlur1.Reset();
    m_passBlur2.Reset();
}

void CScreenSpaceReflectionsPass::Execute()
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    if (!CRenderer::CV_r_SSReflections || !CTexture::s_ptexHDRTarget)  // Sketch mode disables HDR rendering
    {
        return;
    }

    PROFILE_LABEL_SCOPE("SS_REFLECTIONS");

    if (CRenderer::CV_r_SlimGBuffer)
    {
        rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
    }

    // Store current state
    const uint32 prevPersFlags = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags;

    Matrix44 mViewProj = rd->m_ViewMatrix * rd->m_ProjMatrix;

    if (rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
    {
        mViewProj = ReverseDepthHelper::Convert(mViewProj);
        rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags &= ~RBPF_REVERSE_DEPTH;
        rd->GetGraphicsPipeline().UpdatePerViewConstantBuffer();
    }

    Matrix44 mViewport(0.5f, 0,    0,    0,
        0,   -0.5f, 0,    0,
        0,    0,    1.0f, 0,
        0.5f, 0.5f, 0,    1.0f);
    const uint32 numGPUs = rd->GetActiveGPUCount();

    const int frameID = SPostEffectsUtils::m_iFrameCounter;
    Matrix44 mViewProjPrev = m_prevViewProj[max((frameID - (int)numGPUs) % MAX_GPU_NUM, 0)] * mViewport;

    int texStateLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
    int texStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
    int texStateLinearBorder  = CTexture::GetTexState(STexState(FILTER_LINEAR, TADDR_BORDER, TADDR_BORDER, TADDR_BORDER, 0));

    CShader* pShader = CShaderMan::s_shDeferredShading;

    {
        PROFILE_LABEL_SCOPE("SSR_RAYTRACE");

        if (CRenderer::CV_r_SlimGBuffer)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
        }

        CCryNameTSCRC techRaytrace("SSR_Raytrace");
        static CCryNameR viewProjName("g_mViewProj");
        static CCryNameR viewProjprevName("g_mViewProjPrev");
        CTexture* destRT = CRenderer::CV_r_SSReflHalfRes ? CTexture::s_ptexHDRTargetScaled[0] : CTexture::s_ptexHDRTarget;

        m_passRaytracing.SetRenderTarget(0, destRT);
        m_passRaytracing.SetTechnique(pShader, techRaytrace, rd->m_RP.m_FlagsShader_RT);
        m_passRaytracing.SetState(GS_NODEPTHTEST);
        m_passRaytracing.SetTextureSamplerPair(0, CTexture::s_ptexZTarget, texStatePoint);
        m_passRaytracing.SetTextureSamplerPair(1, CTexture::s_ptexSceneNormalsMap, texStateLinear);
        m_passRaytracing.SetTextureSamplerPair(2, CTexture::s_ptexSceneSpecular, texStateLinear);
        m_passRaytracing.SetTextureSamplerPair(3, CTexture::s_ptexZTargetScaled, texStatePoint);
        m_passRaytracing.SetTextureSamplerPair(4, CTexture::s_ptexHDRTargetPrev, texStateLinearBorder);
        m_passRaytracing.SetTextureSamplerPair(5, CTexture::s_ptexHDRMeasuredLuminance[rd->RT_GetCurrGpuID()], texStatePoint);
        m_passRaytracing.SetRequireWorldPos(true);

        m_passRaytracing.BeginConstantUpdate();
        pShader->FXSetPSFloat(viewProjName, (Vec4*)mViewProj.GetData(), 4);
        pShader->FXSetPSFloat(viewProjprevName, (Vec4*)mViewProjPrev.GetData(), 4);
        m_passRaytracing.Execute();
    }

    if (!CRenderer::CV_r_SSReflHalfRes)
    {
        m_passCopy.Execute(CTexture::s_ptexHDRTarget, CTexture::s_ptexHDRTargetScaled[0]);
    }

    // Convolve sharp reflections
    m_passDownsample0.Execute(CTexture::s_ptexHDRTargetScaled[0], CTexture::s_ptexHDRTargetScaled[1]);
    m_passBlur0.Execute(CTexture::s_ptexHDRTargetScaled[1], CTexture::s_ptexHDRTargetScaledTempRT[1], 1.0f, 3.0f);

    m_passDownsample1.Execute(CTexture::s_ptexHDRTargetScaled[1], CTexture::s_ptexHDRTargetScaled[2]);
    m_passBlur1.Execute(CTexture::s_ptexHDRTargetScaled[2], CTexture::s_ptexHDRTargetScaledTempRT[2], 1.0f, 3.0f);

    m_passDownsample2.Execute(CTexture::s_ptexHDRTargetScaled[2], CTexture::s_ptexHDRTargetScaled[3]);
    m_passBlur2.Execute(CTexture::s_ptexHDRTargetScaled[3], CTexture::s_ptexHDRTargetScaledTempRT[3], 1.0f, 3.0f);

    {
        PROFILE_LABEL_SCOPE("SSR_COMPOSE");

        if (CRenderer::CV_r_SlimGBuffer)
        {
            rd->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SLIM_GBUFFER];
        }

        static CCryNameTSCRC techComposition("SSReflection_Comp");

        CTexture* destTex = CTexture::s_ptexHDRTargetScaledTmp[0];
        destTex->Unbind();

        m_passComposition.SetRenderTarget(0, destTex);
        m_passComposition.SetTechnique(pShader, techComposition, rd->m_RP.m_FlagsShader_RT);
        m_passComposition.SetState(GS_NODEPTHTEST);
        
        CTexture* smoothnessTex = CTexture::s_ptexSceneSpecular;
        
        // smoothness is encoded in the normal texture for slim GBuffer optimization
        if (CRenderer::CV_r_SlimGBuffer)
        {
            smoothnessTex = CTexture::s_ptexSceneNormalsMap;
        }

        m_passComposition.SetTextureSamplerPair(0, smoothnessTex, texStateLinear);
        m_passComposition.SetTextureSamplerPair(1, CTexture::s_ptexHDRTargetScaled[0], texStateLinear);
        m_passComposition.SetTextureSamplerPair(2, CTexture::s_ptexHDRTargetScaled[1], texStateLinear);
        m_passComposition.SetTextureSamplerPair(3, CTexture::s_ptexHDRTargetScaled[2], texStateLinear);
        m_passComposition.SetTextureSamplerPair(4, CTexture::s_ptexHDRTargetScaled[3], texStateLinear);

        m_passComposition.BeginConstantUpdate();
        m_passComposition.Execute();
    }

    // Update array used for MGPU support
    m_prevViewProj[frameID % MAX_GPU_NUM] = mViewProj;

    // Restore original state
    rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags = prevPersFlags;
    if (rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_PersFlags & RBPF_REVERSE_DEPTH)
    {
        uint32 depthState = ReverseDepthHelper::ConvertDepthFunc(rd->m_RP.m_CurState);
        rd->FX_SetState(rd->m_RP.m_CurState, rd->m_RP.m_CurAlphaRef, depthState);
        rd->GetGraphicsPipeline().UpdatePerViewConstantBuffer();
    }
}
