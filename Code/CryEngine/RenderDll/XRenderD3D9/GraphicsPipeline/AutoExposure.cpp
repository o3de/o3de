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
#include "AutoExposure.h"

#include "DriverD3D.h"


void CAutoExposurePass::Init()
{
    m_samplerPoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
    m_samplerLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
}

void CAutoExposurePass::Shutdown()
{
}

void CAutoExposurePass::Reset()
{
}

void GetSampleOffsets_Downscale4x4Bilinear(uint32 nWidth, uint32 nHeight, Vec4 avSampleOffsets[])
{
    float tU = 1.0f / (float)nWidth;
    float tV = 1.0f / (float)nHeight;

    // Sample from the 16 surrounding points.  Since bilinear filtering is being used, specific the coordinate
    // exactly halfway between the current texel center (k-1.5) and the neighboring texel center (k-0.5)

    int index = 0;
    for (int y = 0; y < 4; y += 2)
    {
        for (int x = 0; x < 4; x += 2, index++)
        {
            avSampleOffsets[index].x = (x - 1.f) * tU;
            avSampleOffsets[index].y = (y - 1.f) * tV;
            avSampleOffsets[index].z = 0;
            avSampleOffsets[index].w = 1;
        }
    }
}

void CAutoExposurePass::MeasureLuminance()
{
    PROFILE_LABEL_SCOPE("MEASURE_LUMINANCE");

    CD3D9Renderer* rd = gcpRendD3D;

    uint64 nFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE5]);

    int32 curTexture = NUM_HDR_TONEMAP_TEXTURES - 1;
    static CCryNameR Param1Name("SampleOffsets");

    float tU = 1.0f / (3.0f * CTexture::s_ptexHDRToneMaps[curTexture]->GetWidth());
    float tV = 1.0f / (3.0f * CTexture::s_ptexHDRToneMaps[curTexture]->GetHeight());

    Vec4 avSampleOffsets[16];
    uint32 index = 0;
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            avSampleOffsets[index].x = x * tU;
            avSampleOffsets[index].y = y * tV;
            avSampleOffsets[index].z = 0;
            avSampleOffsets[index].w = 1;
            index++;
        }
    }

    uint32 nPasses;

    rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRToneMaps[curTexture], NULL);
    rd->FX_SetActiveRenderTargets();
    rd->RT_SetViewport(0, 0, CTexture::s_ptexHDRToneMaps[curTexture]->GetWidth(), CTexture::s_ptexHDRToneMaps[curTexture]->GetHeight());

    CShader* pShader = CShaderMan::s_shHDRPostProcess;
    static CCryNameTSCRC TechName("HDRSampleLumInitial");
    pShader->FXSetTechnique(TechName);
    pShader->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
    pShader->FXBeginPass(0);

    CTexture::s_ptexHDRTargetScaled[1]->Apply(0, m_samplerLinear);
    CTexture::s_ptexSceneNormalsMap->Apply(1, m_samplerLinear);
    CTexture::s_ptexSceneDiffuse->Apply(2, m_samplerLinear);
    CTexture::s_ptexSceneSpecular->Apply(3, m_samplerLinear);

    float s1 = 1.0f / (float) CTexture::s_ptexHDRTargetScaled[1]->GetWidth();
    float t1 = 1.0f / (float) CTexture::s_ptexHDRTargetScaled[1]->GetHeight();

    // Use rotated grid
    Vec4 vSampleLumOffsets0 = Vec4(s1 * 0.95f, t1 * 0.25f, -s1 * 0.25f, t1 * 0.96f);
    Vec4 vSampleLumOffsets1 = Vec4(-s1 * 0.96f, -t1 * 0.25f, s1 * 0.25f, -t1 * 0.96f);

    static CCryNameR pSampleLumOffsetsName0("SampleLumOffsets0");
    static CCryNameR pSampleLumOffsetsName1("SampleLumOffsets1");

    pShader->FXSetPSFloat(pSampleLumOffsetsName0, &vSampleLumOffsets0, 1);
    pShader->FXSetPSFloat(pSampleLumOffsetsName1, &vSampleLumOffsets1, 1);

    bool ret = DrawFullScreenQuad(0.0f, 1.0f - 1.0f * gcpRendD3D->m_CurViewportScale.y, 1.0f * gcpRendD3D->m_CurViewportScale.x, 1.0f);

    // important that we always write out valid luminance, even if quad draw fails
    if (!ret)
    {
        rd->FX_ClearTarget(CTexture::s_ptexHDRToneMaps[curTexture], Clr_Dark);
    }

    pShader->FXEndPass();

    rd->FX_PopRenderTarget(0);

    curTexture--;

    // Initialize the sample offsets for the iterative luminance passes
    while (curTexture >= 0)
    {
        rd->FX_PushRenderTarget(0, CTexture::s_ptexHDRToneMaps[curTexture], NULL);
        rd->RT_SetViewport(0, 0, CTexture::s_ptexHDRToneMaps[curTexture]->GetWidth(), CTexture::s_ptexHDRToneMaps[curTexture]->GetHeight());

        if (!curTexture)
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
        }
        if (curTexture == 1)
        {
            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
        }

        static CCryNameTSCRC TechNameLI("HDRSampleLumIterative");
        pShader->FXSetTechnique(TechNameLI);
        pShader->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);
        pShader->FXBeginPass(0);

        GetSampleOffsets_Downscale4x4Bilinear(CTexture::s_ptexHDRToneMaps[curTexture + 1]->GetWidth(), CTexture::s_ptexHDRToneMaps[curTexture + 1]->GetHeight(), avSampleOffsets);
        pShader->FXSetPSFloat(Param1Name, avSampleOffsets, 4);
        CTexture::s_ptexHDRToneMaps[curTexture + 1]->Apply(0, m_samplerLinear);

        // Draw a fullscreen quad to sample the RT
        ret = DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);

        // important that we always write out valid luminance, even if quad draw fails
        if (!ret)
        {
            rd->FX_ClearTarget(CTexture::s_ptexHDRToneMaps[curTexture], Clr_Dark);
        }

        pShader->FXEndPass();

        rd->FX_PopRenderTarget(0);

        curTexture--;
    }

    gcpRendD3D->GetDeviceContext().CopyResource(
        CTexture::s_ptexHDRMeasuredLuminance[gcpRendD3D->RT_GetCurrGpuID()]->GetDevTexture()->GetBaseTexture(),
        CTexture::s_ptexHDRToneMaps[0]->GetDevTexture()->GetBaseTexture());

    gRenDev->m_RP.m_FlagsShader_RT = nFlagsShader_RT;
}

void CAutoExposurePass::AdjustExposure()
{
    PROFILE_LABEL_SCOPE("EYEADAPTATION");

    CD3D9Renderer* rd = gcpRendD3D;

    // Swap current & last luminance
    const int lumMask = (int32)(sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur) / sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur[0])) - 1;
    const int32 numTextures = (int32)max(min(gRenDev->GetActiveGPUCount(), (uint32)(sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur) / sizeof(CTexture::s_ptexHDRAdaptedLuminanceCur[0]))), 1u);

    CTexture::s_nCurLumTextureIndex++;
    CTexture* pTexPrev = CTexture::s_ptexHDRAdaptedLuminanceCur[(CTexture::s_nCurLumTextureIndex - numTextures) & lumMask];
    CTexture* pTexCur = CTexture::s_ptexHDRAdaptedLuminanceCur[CTexture::s_nCurLumTextureIndex & lumMask];
    CTexture::s_ptexCurLumTexture = pTexCur;
    assert(pTexCur);

    CShader* pShader = CShaderMan::s_shHDRPostProcess;
    uint32 nPasses;
    static CCryNameTSCRC TechName("HDRCalculateAdaptedLum");
    pShader->FXSetTechnique(TechName);
    pShader->FXBegin(&nPasses, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

    rd->FX_PushRenderTarget(0, pTexCur, NULL);
    rd->RT_SetViewport(0, 0, pTexCur->GetWidth(), pTexCur->GetHeight());

    pShader->FXBeginPass(0);

    {
        Vec4 elapsedTime;
        elapsedTime[0] = iTimer->GetFrameTime() * numTextures;
        elapsedTime[1] = 1.0f - expf(-CRenderer::CV_r_HDREyeAdaptationSpeed * elapsedTime[0]);
        elapsedTime[2] = 0;
        elapsedTime[3] = 0;

        if (rd->GetCamera().IsJustActivated() || rd->m_nDisableTemporalEffects > 0)
        {
            elapsedTime[1] = 1.0f;
            elapsedTime[2] = 1.0f;
        }

        static CCryNameR Param1Name("ElapsedTime");
        pShader->FXSetPSFloat(Param1Name, &elapsedTime, 1);
    }

    pTexPrev->Apply(0, m_samplerPoint);
    CTexture::s_ptexHDRToneMaps[0]->Apply(1, m_samplerPoint);

    // Draw a fullscreen quad to sample the RT
    DrawFullScreenQuad(0.0f, 0.0f, 1.0f, 1.0f);

    pShader->FXEndPass();

    rd->FX_PopRenderTarget(0);
}

void CAutoExposurePass::Execute()
{
    MeasureLuminance();
    AdjustExposure();
}
