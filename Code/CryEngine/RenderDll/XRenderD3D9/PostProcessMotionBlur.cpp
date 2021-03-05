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

// All code here, besides CMotionBlur::Preprocess is a direct copy & modification of the CMotionBlurPass code found in GraphicsPipeline/MotionBlur.cpp.

void CMotionBlur::CopyAndScaleDoFBuffer(CTexture* pSrcTex, CTexture* pDestTex)
{
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    if (pSrcTex == NULL || pDestTex == NULL)
    {
        return;
    }

    PROFILE_LABEL_SCOPE("Motion Blur - Scale & Copy DoF");

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

    gcpRendD3D->FX_PushRenderTarget(0, pDestTex, NULL);
    gcpRendD3D->RT_SetViewport(0, 0, pDestTex->GetWidth(), pDestTex->GetHeight());
    PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffects, bResample ? techTexToTexResampled : techTexToTex, 0);
    gRenDev->FX_SetState(GS_NODEPTHTEST);

    int texFilter = CTexture::GetTexState(STexState(bResample ? FILTER_LINEAR : FILTER_POINT, true));
    PostProcessUtils().SetTexture(pSrcTex, 0, texFilter);

    static CCryNameR param0Name("texToTexParams0");
    static CCryNameR param1Name("texToTexParams1");

    const bool bBigDownsample = false;  // TODO
    CTexture* pOffsetTex = bBigDownsample ? pDestTex : pSrcTex;

    float s1 = 0.5f / (float)pOffsetTex->GetWidth();  // 2.0 better results on lower res images resizing
    float t1 = 0.5f / (float)pOffsetTex->GetHeight();

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

    CShaderMan::s_shPostEffects->FXSetPSFloat(param0Name, &params0, 1);
    CShaderMan::s_shPostEffects->FXSetPSFloat(param1Name, &params1, 1);

    PostProcessUtils().DrawFullScreenTri(pDestTex->GetWidth(), pDestTex->GetHeight());
    PostProcessUtils().ShEndPass();
    gcpRendD3D->FX_PopRenderTarget(0);
}

float CMotionBlur::ComputeMotionScale()
{
    static float storedMotionScale = 0.0f;
    if (gEnv->pTimer->IsTimerPaused(ITimer::ETIMER_GAME))
    {
        return storedMotionScale;
    }

    // The length of the generated motion vectors is proportional to the current time step, so we need
    // to rescale the motion vectors to simulate a constant camera exposure time

    float exposureTime = 1.0f / std::max(CRenderer::CV_r_MotionBlurShutterSpeed, 1e-6f);
    float timeStep = std::max(gEnv->pTimer->GetFrameTime(), 1e-6f);

    exposureTime *= gEnv->pTimer->GetTimeScale();

    storedMotionScale = exposureTime / timeStep;
    return storedMotionScale;
}

void CMotionBlur::Render()
{
    PROFILE_LABEL_SCOPE("MB");

    CD3D9Renderer* rd = gcpRendD3D;
    CShader* pShader = CShaderMan::s_shPostMotionBlur;

    int vpX, vpY, vpWidth, vpHeight;
    rd->GetViewport(&vpX, &vpY, &vpWidth, &vpHeight);

    // Check if DOF is enabled
    CDepthOfField* pDofRenderTech = (CDepthOfField*)PostEffectMgr()->GetEffect(ePFX_eDepthOfField);
    DepthOfFieldParameters dofParameters = pDofRenderTech->GetParameters();
    const bool bGatherDofEnabled = CRenderer::CV_r_dof > 0 && dofParameters.m_bEnabled;

    Matrix44A mViewProjPrev = GetPrevView();
    Matrix44 mViewProj = PostProcessUtils().m_pView;
    mViewProjPrev = mViewProjPrev * PostProcessUtils().m_pProj * PostProcessUtils().m_pScaleBias;
    mViewProjPrev.Transpose();

    CTexture* pVelocityRT = CTexture::s_ptexVelocity;
    float tileCountX = (float)CTexture::s_ptexVelocityTiles[1]->GetWidth();
    float tileCountY = (float)CTexture::s_ptexVelocityTiles[1]->GetHeight();

    static CCryNameR motionBlurParamName("vMotionBlurParams");

    int texStateLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
    int texStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));

    {
        PROFILE_LABEL_SCOPE("PACK VELOCITY");

        static CCryNameTSCRC techPackVelocities("PackVelocities");
        static CCryNameR viewProjPrevName("mViewProjPrev");
        static CCryNameR dirBlurName("vDirectionalBlur");
        static CCryNameR radBlurName("vRadBlurParam");

        const float maxRange = 32.0f;
        const float amount = clamp_tpl<float>(m_pRadBlurAmount->GetParam() / maxRange, 0.0f, 1.0f);
        const float radius = 1.0f / clamp_tpl<float>(m_pRadBlurRadius->GetParam(), 1e-6f, 2.0f);
        const Vec4 blurDir = m_pDirectionalBlurVec->GetParamVec4();
        const Vec4 dirBlurParam = Vec4(blurDir.x * (maxRange / (float)vpWidth), blurDir.y * (maxRange / (float)vpHeight), (float)vpWidth / (float)vpHeight, 1.0f);
        const Vec4 radBlurParam = Vec4(m_pRadBlurScreenPosX->GetParam() * dirBlurParam.z, m_pRadBlurScreenPosY->GetParam(), radius * amount, amount);

        const bool bRadialBlur = amount + (blurDir.x * blurDir.x) + (blurDir.y * blurDir.y) > 1.0f / (float)vpWidth;

        uint64 mask = bRadialBlur ? g_HWSR_MaskBit[HWSR_SAMPLE0] : 0;
        AZ_Assert(((mask >> 32) & 0xFFFFFFFF) == 0, "Make sure we aren't trying to use the top 32 bits, as they will be lost");


        gcpRendD3D->FX_PushRenderTarget(0, pVelocityRT, NULL);
        gcpRendD3D->RT_SetViewport(0, 0, pVelocityRT->GetWidth(), pVelocityRT->GetHeight());
        PostProcessUtils().ShBeginPass(pShader, techPackVelocities, (uint32)mask);
        gRenDev->FX_SetState(GS_NODEPTHTEST);

        PostProcessUtils().SetTexture(CTexture::s_ptexZTarget, 0, texStatePoint);
        PostProcessUtils().SetTexture(CTexture::s_ptexHDRTarget, 1, texStatePoint);
        PostProcessUtils().SetTexture(PostProcessUtils().GetVelocityObjectRT(), 2, texStatePoint);

        mViewProjPrev.Transpose();
        pShader->FXSetPSFloat(viewProjPrevName, (Vec4*)mViewProjPrev.GetData(), 4);
        pShader->FXSetPSFloat(dirBlurName, &dirBlurParam, 1);
        pShader->FXSetPSFloat(radBlurName, &radBlurParam, 1);
        const Vec4 motionBlurParams = Vec4(ComputeMotionScale(), 1.0f / tileCountX, 1.0f / tileCountX * CRenderer::CV_r_MotionBlurCameraMotionScale, 0);
        pShader->FXSetPSFloat(motionBlurParamName, &motionBlurParams, 1);

        PostProcessUtils().DrawFullScreenTriWPOS(pVelocityRT->GetWidth(), pVelocityRT->GetHeight());
        PostProcessUtils().ShEndPass();
        gcpRendD3D->FX_PopRenderTarget(0);

    }

    {
        PROFILE_LABEL_SCOPE("VELOCITY TILES");

        static CCryNameTSCRC techVelocityTileGen("VelocityTileGen");
        static CCryNameTSCRC techTileNeighborhood("VelocityTileNeighborhood");

        // Tile generation first pass
        {
            CTexture* pVelocityTile = CTexture::s_ptexVelocityTiles[0];

            gcpRendD3D->FX_PushRenderTarget(0, pVelocityTile, NULL);
            gcpRendD3D->RT_SetViewport(0, 0, pVelocityTile->GetWidth(), pVelocityTile->GetHeight());
            PostProcessUtils().ShBeginPass(pShader, techVelocityTileGen, 0);
            gRenDev->FX_SetState(GS_NODEPTHTEST);
            PostProcessUtils().SetTexture(pVelocityRT, 0, texStatePoint);

            Vec4 params = Vec4((float)pVelocityRT->GetWidth(), (float)pVelocityRT->GetHeight(), ceilf((float)gcpRendD3D->GetWidth() / tileCountX), 0);
            pShader->FXSetPSFloat(motionBlurParamName, &params, 1);

            PostProcessUtils().DrawFullScreenTri(pVelocityTile->GetWidth(), pVelocityTile->GetHeight());
            PostProcessUtils().ShEndPass();
            gcpRendD3D->FX_PopRenderTarget(0);
        }

        // Tile generation second pass
        {
            CTexture* pVelocityTile = CTexture::s_ptexVelocityTiles[1];

            gcpRendD3D->FX_PushRenderTarget(0, pVelocityTile, NULL);
            gcpRendD3D->RT_SetViewport(0, 0, pVelocityTile->GetWidth(), pVelocityTile->GetHeight());
            PostProcessUtils().ShBeginPass(pShader, techVelocityTileGen, 0);
            gRenDev->FX_SetState(GS_NODEPTHTEST);
            PostProcessUtils().SetTexture(CTexture::s_ptexVelocityTiles[0], 0, texStatePoint);

            Vec4 params = Vec4((float)CTexture::s_ptexVelocityTiles[0]->GetWidth(), (float)CTexture::s_ptexVelocityTiles[0]->GetHeight(), ceilf((float)gcpRendD3D->GetHeight() / tileCountY), 1);
            pShader->FXSetPSFloat(motionBlurParamName, &params, 1);

            PostProcessUtils().DrawFullScreenTri(pVelocityTile->GetWidth(), pVelocityTile->GetHeight());
            PostProcessUtils().ShEndPass();
            gcpRendD3D->FX_PopRenderTarget(0);
        }

        // Neighborhood max
        {
            CTexture* pVelocityTile = CTexture::s_ptexVelocityTiles[2];

            gcpRendD3D->FX_PushRenderTarget(0, pVelocityTile, NULL);
            gcpRendD3D->RT_SetViewport(0, 0, pVelocityTile->GetWidth(), pVelocityTile->GetHeight());
            PostProcessUtils().ShBeginPass(pShader, techTileNeighborhood, 0);
            gRenDev->FX_SetState(GS_NODEPTHTEST);
            PostProcessUtils().SetTexture(CTexture::s_ptexVelocityTiles[1], 0, texStatePoint);

            Vec4 params = Vec4(1.0f / tileCountX, 1.0f / tileCountY, 0, 0);
            pShader->FXSetPSFloat(motionBlurParamName, &params, 1);

            PostProcessUtils().DrawFullScreenTri(pVelocityTile->GetWidth(), pVelocityTile->GetHeight());
            PostProcessUtils().ShEndPass();
            gcpRendD3D->FX_PopRenderTarget(0);
        }
    }

    {
        PROFILE_LABEL_SCOPE("MOTION VECTOR APPLY");

        static CCryNameTSCRC techMotionBlur("MotionBlur");

        if (bGatherDofEnabled)
        {
            CopyAndScaleDoFBuffer(CTexture::s_ptexHDRTarget, CTexture::s_ptexSceneTargetR11G11B10F[0]);
        }

        uint64 rtMask = 0;
        rtMask |= (CRenderer::CV_r_MotionBlurQuality >= 2) ? g_HWSR_MaskBit[HWSR_SAMPLE2] : 0;
        rtMask |= (CRenderer::CV_r_MotionBlurQuality == 1) ? g_HWSR_MaskBit[HWSR_SAMPLE1] : 0;

        CTexture* pHDRTarget = CTexture::s_ptexHDRTarget;
        AZ_Assert(((rtMask >> 32) & 0xFFFFFFFF) == 0, "Make sure we aren't trying to use the top 32 bits, as they will be lost");

        gcpRendD3D->FX_PushRenderTarget(0, pHDRTarget, NULL);
        gcpRendD3D->RT_SetViewport(0, 0, pHDRTarget->GetWidth(), pHDRTarget->GetHeight());
        PostProcessUtils().ShBeginPass(pShader, techMotionBlur, (uint32)rtMask);
        gRenDev->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA);
        PostProcessUtils().SetTexture(bGatherDofEnabled ? CTexture::s_ptexSceneTargetR11G11B10F[0] : CTexture::s_ptexHDRTargetPrev, 0, texStateLinear);
        PostProcessUtils().SetTexture(pVelocityRT, 1, texStatePoint);
        PostProcessUtils().SetTexture(CTexture::s_ptexVelocityTiles[2], 2, texStatePoint);

        Vec4 params = Vec4(1.0f / tileCountX, 1.0f / tileCountY, 0, 0);
        pShader->FXSetPSFloat(motionBlurParamName, &params, 1);


        PostProcessUtils().DrawFullScreenTri(pHDRTarget->GetWidth(), pHDRTarget->GetHeight());
        PostProcessUtils().ShEndPass();
        gcpRendD3D->FX_PopRenderTarget(0);
    }
}

bool CMotionBlur::Preprocess()
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);

    if (!bQualityCheck)
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_PostProcessFilters)
    {
        return false;
    }

    if (!CRenderer::CV_r_MotionBlur)
    {
        return false;
    }

    if (!CRenderer::CV_r_RenderMotionBlurAfterHDR)
    {
        return false;
    }

    return true;
}
