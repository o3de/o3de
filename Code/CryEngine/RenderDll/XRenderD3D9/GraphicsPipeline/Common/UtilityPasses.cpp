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
#include "UtilityPasses.h"
#include "FullscreenPass.h"
#include "DriverD3D.h"


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CStretchRectPass
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CStretchRectPass::Execute(CTexture* pSrcTex, CTexture* pDestTex)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    if (pSrcTex == NULL || pDestTex == NULL)
    {
        return;
    }

    PROFILE_LABEL_SCOPE("STRETCHRECT");

    bool bResample = pSrcTex->GetWidth() != pDestTex->GetWidth() || pSrcTex->GetHeight() != pDestTex->GetHeight();
    const D3DFormat destFormat = CTexture::DeviceFormatFromTexFormat(pDestTex->GetDstFormat());
    const D3DFormat srcFormat = CTexture::DeviceFormatFromTexFormat(pSrcTex->GetDstFormat());

    if (!bResample && destFormat == srcFormat)
    {
        rd->GetDeviceContext().CopyResource(pDestTex->GetDevTexture()->GetBaseTexture(), pSrcTex->GetDevTexture()->GetBaseTexture());
        return;
    }

    static CCryNameTSCRC techTexToTex("TextureToTexture");
    static CCryNameTSCRC techTexToTexResampled("TextureToTextureResampled");

    m_pass.SetRenderTarget(0, pDestTex);
    m_pass.SetTechnique(CShaderMan::s_shPostEffects, bResample ? techTexToTexResampled : techTexToTex, 0);
    m_pass.SetState(GS_NODEPTHTEST);
    int texFilter = CTexture::GetTexState(STexState(bResample ? FILTER_LINEAR : FILTER_POINT, true));
    m_pass.SetTextureSamplerPair(0, pSrcTex, texFilter);

    static CCryNameR param0Name("texToTexParams0");
    static CCryNameR param1Name("texToTexParams1");

    const bool bBigDownsample = false;  // TODO
    CTexture* pOffsetTex = bBigDownsample ? pDestTex : pSrcTex;

    float s1 = 0.5f / (float) pOffsetTex->GetWidth();  // 2.0 better results on lower res images resizing
    float t1 = 0.5f / (float) pOffsetTex->GetHeight();

    Vec4 params0, params1;
    if (bBigDownsample)
    {
        // Use rotated grid + middle sample (~Quincunx)
        params0 = Vec4(s1 * 0.96f, t1 * 0.25f, -s1 * 0.25f, t1 * 0.96f);
        params1 = Vec4(-s1 * 0.96f, -t1 * 0.25f, s1 * 0.25f, -t1 * 0.96f);
    }
    else
    {
        // Use box filtering (faster - can skip bilinear filtering, only 4 taps)
        params0 = Vec4(-s1, -t1, s1, -t1);
        params1 = Vec4(s1, t1, -s1, t1);
    }

    m_pass.BeginConstantUpdate();
    CShaderMan::s_shPostEffects->FXSetPSFloat(param0Name, &params0, 1);
    CShaderMan::s_shPostEffects->FXSetPSFloat(param1Name, &params1, 1);
    m_pass.Execute();
}

void CStretchRectPass::Reset()
{
    m_pass.Reset();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CGaussianBlurPass
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

inline float CGaussianBlurPass::GaussianDistribution1D(float x, float rho)
{
    float g = 1.0f / (rho * sqrtf(2.0f * PI));
    g *= expf(-(x * x) / (2.0f * rho * rho));
    return g;
}

void CGaussianBlurPass::ComputeParams(int texWidth, int texHeight, int numSamples, float scale, float distribution)
{
    assert(numSamples <= 16);
    const int halfNumSamples = (numSamples >> 1);

    float s1 = 1.0f / (float)texWidth;
    float t1 = 1.0f / (float)texHeight;

    float weights[16];
    float weightSum = 0.0f;

    // Compute Gaussian weights
    for (int s = 0; s < numSamples; ++s)
    {
        if (distribution != 0.0f)
        {
            weights[s] = GaussianDistribution1D((float)(s - halfNumSamples), distribution);
        }
        else
        {
            weights[s] = 0.0f;
        }
        weightSum += weights[s];
    }

    // Normalize weights
    for (int s = 0; s < numSamples; ++s)
    {
        weights[s] /= weightSum;
    }

    // Compute bilinear offsets
    for (int s = 0; s < halfNumSamples; ++s)
    {
        float off_a = weights[s * 2];
        float off_b = ((s * 2 + 1) <= numSamples - 1) ? weights[s * 2 + 1] : 0;
        float a_plus_b = (off_a + off_b);
        if (a_plus_b == 0)
        {
            a_plus_b = 1.0f;
        }
        float offset = off_b / a_plus_b;

        weights[s] = off_a + off_b;
        weights[s] *= scale;
        m_weights[s] = Vec4(1, 1, 1, 1) * weights[s];

        float currOffset = (float)s * 2.0f + offset - (float)halfNumSamples;
        m_paramsH[s] = Vec4(s1 * currOffset, 0, 0, 0);
        m_paramsV[s] = Vec4(0, t1 * currOffset, 0, 0);
    }
}

void CGaussianBlurPass::Execute(CTexture* pTex, CTexture* pTempTex, float scale, float distribution)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    if (pTex == NULL || pTempTex == NULL)
    {
        return;
    }

    PROFILE_LABEL_SCOPE("TEXBLUR_GAUSSIAN");

    CShader* pShader = CShaderMan::s_shPostEffects;
    int texFilter = CTexture::GetTexState(STexState(FILTER_LINEAR, true));

    static CCryNameTSCRC techDefault("GaussBlurBilinear");
    static CCryNameR clampTCName("clampTC");
    static CCryNameR param0Name("psWeights");
    static CCryNameR param1Name("PI_psOffsets");

    Vec4 clampTC(0.0f, 1.0f, 0.0f, 1.0f);
    if (pTex->GetWidth() == rd->GetWidth() && pTex->GetHeight() == rd->GetHeight())
    {
        // Clamp manually in shader since texture clamp won't apply for smaller viewport
        clampTC = Vec4(0.0f, rd->m_RP.m_CurDownscaleFactor.x, 0.0f, rd->m_RP.m_CurDownscaleFactor.y);
    }

    const int numSamples = 16;
    if (m_scale != scale || m_distribution != distribution)
    {
        ComputeParams(pTex->GetWidth(), pTex->GetHeight(), numSamples, scale, distribution);
        m_scale = scale;
        m_distribution = distribution;
    }

    // Horizontal
    m_passH.SetRenderTarget(0, pTempTex);
    m_passH.SetTechnique(pShader, techDefault, 0);
    m_passH.SetState(GS_NODEPTHTEST);
    m_passH.SetTextureSamplerPair(0, pTex, texFilter);

    m_passH.BeginConstantUpdate();
    pShader->FXSetVSFloat(param1Name, m_paramsH, numSamples / 2);
    pShader->FXSetPSFloat(param0Name, m_weights, numSamples / 2);
    pShader->FXSetPSFloat(clampTCName, &clampTC, 1);
    m_passH.Execute();

    // Vertical
    m_passV.SetRenderTarget(0, pTex);
    m_passV.SetTechnique(pShader, techDefault, 0);
    m_passV.SetState(GS_NODEPTHTEST);
    m_passV.SetTextureSamplerPair(0, pTempTex, texFilter);

    m_passV.BeginConstantUpdate();
    pShader->FXSetVSFloat(param1Name, m_paramsV, numSamples / 2);
    pShader->FXSetPSFloat(param0Name, m_weights, numSamples / 2);
    pShader->FXSetPSFloat(clampTCName, &clampTC, 1);
    m_passV.Execute();
}

void CGaussianBlurPass::Reset()
{
    m_passH.Reset();
    m_passV.Reset();
}