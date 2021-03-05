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
#include "ScreenSpaceSSS.h"
#include "DriverD3D.h"


void CScreenSpaceSSSPass::Init()
{
}

void CScreenSpaceSSSPass::Shutdown()
{
}

void CScreenSpaceSSSPass::Reset()
{
    m_passH.Reset();
    m_passV.Reset();
}

void CScreenSpaceSSSPass::Execute(CTexture* pIrradianceTex)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    if (!CTexture::s_ptexHDRTarget)  // Sketch mode disables HDR rendering
    {
        return;
    }

    PROFILE_LABEL_SCOPE("SSSSS");

    static CCryNameTSCRC techBlur("SSSSS_Blur");
    static CCryNameR paramBlur("SSSBlurDir");
    static CCryNameR paramViewSpaceParams("ViewSpaceParams");

    CShader* pShader = CShaderMan::s_shDeferredShading;
    int texStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));

    Vec4 viewSpaceParams(2.0f / rd->m_ProjMatrix.m00, 2.0f / rd->m_ProjMatrix.m11, -1.0f / rd->m_ProjMatrix.m00, -1.0f / rd->m_ProjMatrix.m11);
    float fProjScaleX = 0.5f * rd->m_ProjMatrix.m00;
    float fProjScaleY = 0.5f * rd->m_ProjMatrix.m11;

    // Horizontal pass
    {
        m_passH.SetRenderTarget(0, CTexture::s_ptexSceneTargetR11G11B10F[1]);
        m_passH.SetTechnique(pShader, techBlur, 0);
        m_passH.SetState(GS_NODEPTHTEST);
        m_passH.SetTextureSamplerPair(0, pIrradianceTex, texStatePoint);
        m_passH.SetTextureSamplerPair(1, CTexture::s_ptexZTarget, texStatePoint);
        m_passH.SetTextureSamplerPair(2, CTexture::s_ptexSceneNormalsMap, texStatePoint);
        m_passH.SetTextureSamplerPair(3, CTexture::s_ptexSceneDiffuse, texStatePoint);
        m_passH.SetTextureSamplerPair(4, CTexture::s_ptexSceneSpecular, texStatePoint);

        m_passH.BeginConstantUpdate();
        pShader->FXSetPSFloat(paramViewSpaceParams, &viewSpaceParams, 1);
        Vec4 blurParam(fProjScaleX, 0, 0, 0);
        pShader->FXSetPSFloat(paramBlur, &blurParam, 1);
        m_passH.Execute();
    }

    // Vertical pass
    {
        m_passV.SetRenderTarget(0, CTexture::s_ptexHDRTarget);
        m_passV.SetTechnique(pShader, techBlur, g_HWSR_MaskBit[HWSR_SAMPLE0]);
        m_passV.SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONE);
        m_passV.SetTextureSamplerPair(0, CTexture::s_ptexSceneTargetR11G11B10F[1], texStatePoint);
        m_passV.SetTextureSamplerPair(1, CTexture::s_ptexZTarget, texStatePoint);
        m_passV.SetTextureSamplerPair(2, CTexture::s_ptexSceneNormalsMap, texStatePoint);
        m_passV.SetTextureSamplerPair(3, CTexture::s_ptexSceneDiffuse, texStatePoint);
        m_passV.SetTextureSamplerPair(4, CTexture::s_ptexSceneSpecular, texStatePoint);
        m_passV.SetTextureSamplerPair(5, pIrradianceTex, texStatePoint);

        m_passV.BeginConstantUpdate();
        pShader->FXSetPSFloat(paramViewSpaceParams, &viewSpaceParams, 1);
        Vec4 blurParam(0, fProjScaleY, 0, 0);
        pShader->FXSetPSFloat(paramBlur, &blurParam, 1);
        m_passV.Execute();
    }
}
