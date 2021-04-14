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
#include "DepthOfField.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"
#include "../../Common/Textures/TextureManager.h"
#include "Common/RenderCapabilities.h"

namespace
{
    const float Pi = (float)M_PI;

    float ngon_rad(float theta, float n)
    {
        return cosf(Pi / n) / cosf(theta - (2 * Pi / n) * floorf((n * theta + PI) / (2 * Pi)));
    }

    // Shirleys concentric mapping
    static Vec2 ToUnitDisk(Vec2 O, float blades, [[maybe_unused]] float fstop)
    {
        float normalizedStops = 1.0f;

        float phi, r;
        const float a = 2 * O.x - 1;
        const float b = 2 * O.y - 1;
        if (fabs(a) > fabs(b)) // use squares instead of absolute values
        {
            r = a;
            phi = (Pi / 4.0f) * (b / (a + 1e-6f));
        }
        else
        {
            r = b;
            phi = (Pi / 2.0f) - (Pi / 4.0f) * (a / (b + 1e-6f));
        }

        float rr = r * powf(ngon_rad(phi, blades), normalizedStops);
        rr = fabs(rr) * (rr > 0 ? 1.0f : -1.0f);

        return Vec2(rr * cosf(phi + normalizedStops), rr * sinf(phi + normalizedStops));
    }
}

void CDepthOfField::UpdateParameters()
{
    bool bOverrideActive = IsActive();
    bool bUseGameSettings = (m_pUserActive->GetParam()) ? true : false;

    float frameTime = clamp_tpl<float>(gEnv->pTimer->GetFrameTime() * 3.0f, 0.0f, 1.0f);

    {
        float fUserFocusRange = m_pUserFocusRange->GetParam();
        float fUserFocusDistance = m_pUserFocusDistance->GetParam();
        float fUserBlurAmount = m_pUserBlurAmount->GetParam();

        if (bOverrideActive)
        {
            fUserFocusRange = fUserFocusDistance = fUserBlurAmount = 0.0f;
        }

        m_fUserFocusRangeCurr += (fUserFocusRange - m_fUserFocusRangeCurr) * frameTime;
        m_fUserFocusDistanceCurr += (fUserFocusDistance - m_fUserFocusDistanceCurr) * frameTime;
        m_fUserBlurAmountCurr += (fUserBlurAmount - m_fUserBlurAmountCurr) * frameTime;
    }

    float focalDistance = 0.0f;
    float focalRange = 0.0f;
    float blurAmount = 0.0f;

    /// Override mode: full control over focal distance / range through parameters.
    if (bOverrideActive)
    {
        focalDistance = m_pFocusDistance->GetParam();
        focalRange = m_pFocusRange->GetParam();
        blurAmount = m_pBlurAmount->GetParam();
    }

    /// Blend of TOD settings with "user adjustments". Used by flowgraph / trackview.
    else if (bUseGameSettings)
    {
        m_todFocusRange += (m_fUserFocusRangeCurr - m_todFocusRange) * frameTime;
        m_todBlurAmount += (m_fUserBlurAmountCurr - m_todBlurAmount) * frameTime;

        focalDistance = m_fUserFocusDistanceCurr;
        focalRange = m_fUserFocusRangeCurr;
        blurAmount = m_fUserBlurAmountCurr;
    }

    /// Full TOD control.
    else
    {
        float fTodFocusRange = (CRenderer::CV_r_dof == 2) ? m_pTimeOfDayFocusRange->GetParam() : 0;
        float fTodBlurAmount = (CRenderer::CV_r_dof == 2) ? m_pTimeOfDayBlurAmount->GetParam() : 0;

        m_todFocusRange += (fTodFocusRange * 2.0f - m_todFocusRange) * frameTime;
        m_todBlurAmount += (fTodBlurAmount - m_todBlurAmount) * frameTime;

        focalDistance = 0.0f;
        focalRange = m_todFocusRange;
        blurAmount = m_todBlurAmount;
    }

    float focalMinDistance = -focalRange * 0.5f;
    float focalMaxDistance = focalRange * 0.5f;

    Vec4 focusParams;
    focusParams.x = 1.0f / (focalMaxDistance + 1e-6f);
    focusParams.y = -focalDistance / (focalMaxDistance + 1e-6f);
    focusParams.z = 1.0f / (focalMinDistance + 1e-6f);
    focusParams.w = -focalDistance / (focalMinDistance + 1e-6f);

    // Arbitrary scale added for compatibility with deprecated scatter depth of field. Should get removed
    // but will break existing content.
    blurAmount *= 2.0f;

    m_Parameters.m_FocusParams0 = focusParams;
    m_Parameters.m_FocusParams1 = Vec4(CRenderer::CV_r_dofMinZ + m_pFocusMinZ->GetParam(), CRenderer::CV_r_dofMinZScale + m_pFocusMinZScale->GetParam(), 0.0f, blurAmount);
    m_Parameters.m_bEnabled = blurAmount > 0.001f;
}

void DepthOfFieldPass::UpdatePassConstants(const DepthOfFieldParameters& dofParams)
{
    m_PassConstantBuffer->m_focusParams0 = dofParams.m_FocusParams0;
    m_PassConstantBuffer->m_focusParams1 = dofParams.m_FocusParams1;
    m_PassConstantBuffer.CopyToDevice();

    AzRHI::ConstantBuffer* constantBuffer = m_PassConstantBuffer.GetDeviceConstantBuffer().get();
    gcpRendD3D->m_DevMan.BindConstantBuffer(eHWSC_Pixel, constantBuffer, eConstantBufferShaderSlot_PerPass);
}

void DepthOfFieldPass::UpdateGatherSubPassConstants(AZ::u32 targetWidth, AZ::u32 targetHeight, AZ::u32 squareTapCount)
{
    const float fFNumber = 8;
    const float fNumApertureSides = 8;
    const float recipTapCount = 1.0f / ((float)squareTapCount - 1.0f);

    for (AZ::u32 y = 0; y < squareTapCount; ++y)
    {
        for (AZ::u32 x = 0; x < squareTapCount; ++x)
        {
            Vec2 t = Vec2(x * recipTapCount, y * recipTapCount);
            Vec2 result = ToUnitDisk(t, fNumApertureSides, fFNumber);
            m_GatherSubPassConstantBuffer->m_taps[x + y * squareTapCount] = Vec4(result.x, result.y, 0, 0);
        }
    }
    m_GatherSubPassConstantBuffer->m_ScreenSize = Vec4(
        (float)targetWidth,
        (float)targetHeight,
        1.0f / (float)targetWidth,
        1.0f / (float)targetHeight);
    m_GatherSubPassConstantBuffer->m_tapCount = Vec4((float)(squareTapCount * squareTapCount), 0, 0, 0);    
    m_GatherSubPassConstantBuffer.CopyToDevice();

    AzRHI::ConstantBuffer* constantBuffer = m_GatherSubPassConstantBuffer.GetDeviceConstantBuffer().get();
    gcpRendD3D->m_DevMan.BindConstantBuffer(eHWSC_Pixel, constantBuffer, eConstantBufferShaderSlot_PerSubPass);
}

void DepthOfFieldPass::UpdateMinCoCSubPassConstants(AZ::u32 targetWidth, AZ::u32 targetHeight)
{
    m_MinCoCSubPassConstantBuffer->m_ScreenSize = Vec4(
        (float)targetWidth,
        (float)targetHeight,
        1.0f / (float)targetWidth,
        1.0f / (float)targetHeight);
    m_MinCoCSubPassConstantBuffer.CopyToDevice();

    AzRHI::ConstantBuffer* constantBuffer = m_MinCoCSubPassConstantBuffer.GetDeviceConstantBuffer().get();
    gcpRendD3D->m_DevMan.BindConstantBuffer(eHWSC_Pixel, constantBuffer, eConstantBufferShaderSlot_PerSubPass);
}

void DepthOfFieldPass::Init()
{
    m_PassConstantBuffer.CreateDeviceBuffer();
    m_GatherSubPassConstantBuffer.CreateDeviceBuffer();
    m_MinCoCSubPassConstantBuffer.CreateDeviceBuffer();
}

void DepthOfFieldPass::Shutdown()
{
}

void DepthOfFieldPass::Reset()
{
}

void DepthOfFieldPass::Execute()
{
    PROFILE_SHADER_SCOPE;
    PROFILE_LABEL_SCOPE("DOF");

    CDepthOfField* depthOfField = (CDepthOfField*)PostEffectMgr()->GetEffect(ePFX_eDepthOfField);
    const DepthOfFieldParameters& dofParams = depthOfField->GetParameters();
    if (!dofParams.m_bEnabled)
    {
        return;
    }

    UpdatePassConstants(dofParams);

    gRenDev->m_cEF.mfRefreshSystemShader("DepthOfField", CShaderMan::s_shPostDepthOfField);

    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2]);

    CTexture* cocCurrent = SPostEffectsUtils::GetCoCCurrentTarget();
    if (CRenderer::CV_r_AntialiasingMode == eAT_TAA)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE2];
    }

    gcpRendD3D->FX_SetState(GS_NODEPTHTEST);
    gcpRendD3D->SetCullMode(R_CULL_NONE);

    // For better blending later.
    // We skip this on mobile as to reduce memory bandwidth and fetch from the RT instead using GMEM
    bool sampleSceneFromRenderTarget = gcpRendD3D->FX_GetEnabledGmemPath(nullptr) && RenderCapabilities::GetFrameBufferFetchCapabilities().test(RenderCapabilities::FBF_COLOR0);
    if (!sampleSceneFromRenderTarget)
    {
        GetUtils().StretchRect(CTexture::s_ptexHDRTarget, CTexture::s_ptexSceneTarget);
    }

    CTexture* nearFarLayersTemp[2] = { CTexture::s_ptexHDRTargetScaledTmp[0], CTexture::s_ptexHDRTargetScaledTempRT[0] };

    assert(nearFarLayersTemp[0]->GetWidth() == CTexture::s_ptexHDRDofLayers[0]->GetWidth() && nearFarLayersTemp[0]->GetHeight() == CTexture::s_ptexHDRDofLayers[0]->GetHeight());
    assert(nearFarLayersTemp[1]->GetWidth() == CTexture::s_ptexHDRDofLayers[1]->GetWidth() && nearFarLayersTemp[1]->GetHeight() == CTexture::s_ptexHDRDofLayers[1]->GetHeight());
    assert(nearFarLayersTemp[0]->GetPixelFormat() == CTexture::s_ptexHDRDofLayers[0]->GetPixelFormat() && nearFarLayersTemp[1]->GetPixelFormat() == CTexture::s_ptexHDRDofLayers[1]->GetPixelFormat());

    {
        // 1st downscale stage
        {
            PROFILE_LABEL_SCOPE("DOWNSCALE LAYERS");

            gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexHDRDofLayers[0], NULL); // near
            gcpRendD3D->FX_PushRenderTarget(1, CTexture::s_ptexHDRDofLayers[1], NULL); // far
            gcpRendD3D->FX_PushRenderTarget(2, CTexture::s_ptexSceneCoC[0], NULL); // CoC near/far

            gcpRendD3D->FX_SetColorDontCareActions(0, true, false);
            gcpRendD3D->FX_SetColorDontCareActions(1, true, false);
            gcpRendD3D->FX_SetColorDontCareActions(2, true, false);

            static CCryNameTSCRC techNameDownscaleDof("DownscaleDof");
            GetUtils().ShBeginPass(CShaderMan::s_shPostDepthOfField, techNameDownscaleDof, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            GetUtils().SetTexture(CTexture::s_ptexZTarget, 0, FILTER_POINT);
            GetUtils().SetTexture(CTexture::s_ptexHDRTarget, 1, FILTER_LINEAR);
            GetUtils().SetTexture(cocCurrent, 2, FILTER_POINT);
            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexHDRDofLayers[0]->GetWidth(), CTexture::s_ptexHDRDofLayers[0]->GetHeight());

            GetUtils().ShEndPass();
            gcpRendD3D->FX_PopRenderTarget(0);
            gcpRendD3D->FX_PopRenderTarget(1);
            gcpRendD3D->FX_PopRenderTarget(2);

            // Avoiding false d3d error (due to deferred rt setup, when ping-pong'ing between RTs we can bump into RTs still bound when binding it as a SRV)
            gcpRendD3D->FX_SetActiveRenderTargets();
        }

        // 2nd downscale stage (tile min CoC)
        {
            PROFILE_LABEL_SCOPE("MIN COC DOWNSCALE");
            uint startingDownscaleIter = 1;
            for (uint32 i = startingDownscaleIter; i < MIN_DOF_COC_K; i++)
            {
                uint32 cocArrayLastElement = i - 1;
                if (i == startingDownscaleIter)
                {
                    cocArrayLastElement = 0;
                }

                gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexSceneCoC[i], NULL); // near
                gcpRendD3D->FX_SetColorDontCareActions(0, true, false);

                static CCryNameTSCRC techNameTileMinCoC("TileMinCoC");
                GetUtils().ShBeginPass(CShaderMan::s_shPostDepthOfField, techNameTileMinCoC, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

                UpdateMinCoCSubPassConstants(
                    CTexture::s_ptexSceneCoC[cocArrayLastElement]->GetWidth(),
                    CTexture::s_ptexSceneCoC[cocArrayLastElement]->GetHeight());

                GetUtils().SetTexture(CTexture::s_ptexSceneCoC[cocArrayLastElement], 0, FILTER_LINEAR);
                SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexSceneCoC[i]->GetWidth(), CTexture::s_ptexSceneCoC[i]->GetHeight());

                GetUtils().ShEndPass();
                gcpRendD3D->FX_PopRenderTarget(0);
                gcpRendD3D->FX_SetActiveRenderTargets();
            }
        }
    }

    {
        // 1st gather pass
        {
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(DepthOfField_cpp)
#endif
#if defined(AZ_RESTRICTED_SECTION_IMPLEMENTED)
#undef AZ_RESTRICTED_SECTION_IMPLEMENTED
#else
            AZ::u32 squareTapCount = 7;
#endif
            if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
            {
                squareTapCount = CRenderer::CV_r_GMEM_DOF_Gather1_Quality;
            }
            UpdateGatherSubPassConstants(
                nearFarLayersTemp[0]->GetWidth(),
                nearFarLayersTemp[0]->GetHeight(),
                squareTapCount);

            PROFILE_LABEL_SCOPE("FAR/NEAR LAYER");
            gcpRendD3D->FX_PushRenderTarget(0, nearFarLayersTemp[0], NULL);
            gcpRendD3D->FX_PushRenderTarget(1, nearFarLayersTemp[1], NULL);
            gcpRendD3D->FX_PushRenderTarget(2, CTexture::s_ptexSceneCoCTemp, NULL);

            gcpRendD3D->FX_SetColorDontCareActions(0, true, false);
            gcpRendD3D->FX_SetColorDontCareActions(1, true, false);
            gcpRendD3D->FX_SetColorDontCareActions(2, true, false);

            gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];
            static CCryNameTSCRC techNameDOF("Dof");
            GetUtils().ShBeginPass(CShaderMan::s_shPostDepthOfField, techNameDOF, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            GetUtils().SetTexture(CTexture::s_ptexZTargetScaled, 0, FILTER_POINT);
            GetUtils().SetTexture(CTexture::s_ptexHDRDofLayers[0], 1, FILTER_LINEAR);
            GetUtils().SetTexture(CTexture::s_ptexHDRDofLayers[1], 2, FILTER_LINEAR);
            GetUtils().SetTexture(CTexture::s_ptexSceneCoC[0], 3, FILTER_LINEAR);
            GetUtils().SetTexture(CTexture::s_ptexSceneCoC[MIN_DOF_COC_K - 1], 4, FILTER_POINT);

            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(nearFarLayersTemp[0]->GetWidth(), nearFarLayersTemp[0]->GetHeight());

            GetUtils().ShEndPass();
            gcpRendD3D->FX_PopRenderTarget(2);
            gcpRendD3D->FX_PopRenderTarget(1);
            gcpRendD3D->FX_PopRenderTarget(0);
            gcpRendD3D->FX_SetActiveRenderTargets();
        }

        // 2nd gather iteration
        {
            AZ::u32 squareTapCount = 3;
            if (gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
            {
                squareTapCount = CRenderer::CV_r_GMEM_DOF_Gather2_Quality;
            }
            UpdateGatherSubPassConstants(
                CTexture::s_ptexHDRDofLayers[0]->GetWidth(),
                CTexture::s_ptexHDRDofLayers[0]->GetHeight(),
                squareTapCount);

            PROFILE_LABEL_SCOPE("FAR/NEAR LAYER ITERATION");
            gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexHDRDofLayers[0], NULL);
            gcpRendD3D->FX_PushRenderTarget(1, CTexture::s_ptexHDRDofLayers[1], NULL);

            gcpRendD3D->FX_SetColorDontCareActions(0, true, false);
            gcpRendD3D->FX_SetColorDontCareActions(1, true, false);

            gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
            static CCryNameTSCRC techNameDOF("Dof");
            GetUtils().ShBeginPass(CShaderMan::s_shPostDepthOfField, techNameDOF, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            GetUtils().SetTexture(nearFarLayersTemp[0], 1, FILTER_LINEAR);
            GetUtils().SetTexture(nearFarLayersTemp[1], 2, FILTER_LINEAR);
            GetUtils().SetTexture(CTexture::s_ptexSceneCoCTemp, 3, FILTER_POINT);
            GetUtils().SetTexture(CTexture::s_ptexSceneCoC[MIN_DOF_COC_K - 1], 4, FILTER_POINT);
            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexHDRDofLayers[0]->GetWidth(), CTexture::s_ptexHDRDofLayers[0]->GetHeight());

            GetUtils().ShEndPass();
            gcpRendD3D->FX_PopRenderTarget(1);
            gcpRendD3D->FX_PopRenderTarget(0);
            gcpRendD3D->FX_SetActiveRenderTargets();
        }

        // Final composition
        {
            PROFILE_LABEL_SCOPE("COMPOSITE");
            gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexHDRTarget, NULL);

            static CCryNameTSCRC techNameCompositeDof("CompositeDof");
            GetUtils().ShBeginPass(CShaderMan::s_shPostDepthOfField, techNameCompositeDof, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            GetUtils().SetTexture(CTexture::s_ptexZTarget, 0, FILTER_POINT);
            GetUtils().SetTexture(CTexture::s_ptexHDRDofLayers[0], 1, FILTER_LINEAR);
            GetUtils().SetTexture(CTexture::s_ptexHDRDofLayers[1], 2, FILTER_LINEAR);
            GetUtils().SetTexture(CTextureManager::Instance()->GetNoTexture(), 3, FILTER_LINEAR);

            if (!sampleSceneFromRenderTarget)
            {
                GetUtils().SetTexture(CTexture::s_ptexSceneTarget, 4, FILTER_POINT);
            }

            GetUtils().SetTexture(cocCurrent, 5, FILTER_POINT);
            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexHDRTarget->GetWidth(), CTexture::s_ptexHDRTarget->GetHeight());

            GetUtils().ShEndPass();

            gcpRendD3D->FX_PopRenderTarget(0);
        }

        CTexture::s_ptexHDRTarget->SetResolved(true);
        gcpRendD3D->FX_SetActiveRenderTargets();
    }

    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}
