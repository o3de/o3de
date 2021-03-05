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
#include "Bloom.h"
#include "DriverD3D.h"

void CBloomPass::Init()
{
}

void CBloomPass::Shutdown()
{
    Reset();
}

void CBloomPass::Reset()
{
    m_pass1H.Reset();
    m_pass1V.Reset();
    m_pass2H.Reset();
    m_pass2V.Reset();
}

void CBloomPass::Execute()
{
    // Approximate function (1 - r)^4 by a sum of Gaussians: 0.0174*G(0.008,r) + 0.192*G(0.0576,r)
    const float sigma1 = sqrtf(0.008f);
    const float sigma2 = sqrtf(0.0576f - 0.008f);

    PROFILE_LABEL_SCOPE("BLOOM_GEN");

    CD3D9Renderer* rd = gcpRendD3D;
    static CCryNameTSCRC techName("HDRBloomGaussian");
    static CCryNameR szHDRParam0("HDRParams0");

    int width = CTexture::s_ptexHDRFinalBloom->GetWidth();
    int height = CTexture::s_ptexHDRFinalBloom->GetHeight();

    // Note: Just scaling the sampling offsets depending on the resolution is not very accurate but works acceptably
    assert(CTexture::s_ptexHDRFinalBloom->GetWidth() == CTexture::s_ptexHDRTarget->GetWidth() / 4);
    float scaleW = ((float)width / 400.0f) / (float)width;
    float scaleH = ((float)height / 225.0f) / (float)height;

    int texStateLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
    int texStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
    int texFilter = (CTexture::s_ptexHDRFinalBloom->GetWidth() == 400 && CTexture::s_ptexHDRFinalBloom->GetHeight() == 225) ? texStatePoint : texStateLinear;

    rd->RT_SetViewport(0, 0, width, height);

    // Pass 1 Horizontal
    m_pass1H.SetRenderTarget(0, CTexture::s_ptexHDRTempBloom[1]);
    m_pass1H.SetTechnique(CShaderMan::s_shHDRPostProcess, techName, 0);
    m_pass1H.SetState(GS_NODEPTHTEST);
    m_pass1H.SetTextureSamplerPair(0, CTexture::s_ptexHDRTargetScaled[1], texFilter);
    m_pass1H.SetTextureSamplerPair(2, CTexture::s_ptexHDRToneMaps[0], texStatePoint);

    m_pass1H.BeginConstantUpdate();
    Vec4 v = Vec4(scaleW, 0, 0, 0);
    CShaderMan::s_shHDRPostProcess->FXSetPSFloat(szHDRParam0, &v, 1);
    m_pass1H.Execute();

    // Pass 1 Vertical
    m_pass1V.SetRenderTarget(0, CTexture::s_ptexHDRTempBloom[0]);
    m_pass1V.SetTechnique(CShaderMan::s_shHDRPostProcess, techName, 0);
    m_pass1V.SetState(GS_NODEPTHTEST);
    m_pass1V.SetTextureSamplerPair(0, CTexture::s_ptexHDRTempBloom[1], texFilter);
    m_pass1V.SetTextureSamplerPair(2, CTexture::s_ptexHDRToneMaps[0], texStatePoint);

    m_pass1V.BeginConstantUpdate();
    v = Vec4(0, scaleH, 0, 0);
    CShaderMan::s_shHDRPostProcess->FXSetPSFloat(szHDRParam0, &v, 1);
    m_pass1V.Execute();

    // Pass 2 Horizontal
    m_pass2H.SetRenderTarget(0, CTexture::s_ptexHDRTempBloom[1]);
    m_pass2H.SetTechnique(CShaderMan::s_shHDRPostProcess, techName, 0);
    m_pass2H.SetState(GS_NODEPTHTEST);
    m_pass2H.SetTextureSamplerPair(0, CTexture::s_ptexHDRTempBloom[0], texFilter);
    m_pass2H.SetTextureSamplerPair(2, CTexture::s_ptexHDRToneMaps[0], texStatePoint);

    m_pass2H.BeginConstantUpdate();
    v = Vec4((sigma2 / sigma1) * scaleW, 0, 0, 0);
    CShaderMan::s_shHDRPostProcess->FXSetPSFloat(szHDRParam0, &v, 1);
    m_pass2H.Execute();

    // Pass 2 Vertical
    m_pass2V.SetRenderTarget(0, CTexture::s_ptexHDRFinalBloom);
    m_pass2V.SetTechnique(CShaderMan::s_shHDRPostProcess, techName, g_HWSR_MaskBit[HWSR_SAMPLE0]);
    m_pass2V.SetState(GS_NODEPTHTEST);
    m_pass2V.SetTextureSamplerPair(0, CTexture::s_ptexHDRTempBloom[1], texFilter);
    m_pass2V.SetTextureSamplerPair(1, CTexture::s_ptexHDRTempBloom[0], texFilter);
    m_pass2V.SetTextureSamplerPair(2, CTexture::s_ptexHDRToneMaps[0], texStatePoint);

    m_pass2V.BeginConstantUpdate();
    v = Vec4(0, (sigma2 / sigma1) * scaleH, 0, 0);
    CShaderMan::s_shHDRPostProcess->FXSetPSFloat(szHDRParam0, &v, 1);
    m_pass2V.Execute();
}
