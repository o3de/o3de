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
#include "MotionBlur.h"
#include "DriverD3D.h"
#include "D3DPostProcess.h"

//////////////////////////////////////////////////////////////////////////
// Old Pipeline Pass

AZStd::unique_ptr<CMotionBlur::ObjectMap> CMotionBlur::m_Objects[CMotionBlur::s_maxObjectBuffers];
CThreadSafeRendererContainer<CMotionBlur::ObjectMap::value_type> CMotionBlur::m_FillData[RT_COMMAND_BUF_COUNT];

void CMotionBlur::GetPrevObjToWorldMat(CRenderObject* renderObject, Matrix44A& worldMatrix)
{
    assert(renderObject);

    if (renderObject->m_ObjFlags & FOB_HAS_PREVMATRIX)
    {
        const SRenderObjData* renderObjectData = renderObject->GetObjData();
        const uintptr_t objectId = renderObjectData ? renderObjectData->m_uniqueObjectId : 0;
        const AZ::u32 frameId = gRenDev->GetFrameID(false);
        const AZ::u32 objectIndex = (frameId - 1) % CMotionBlur::s_maxObjectBuffers;

        auto it = m_Objects[objectIndex]->find(objectId);
        if (it != m_Objects[objectIndex]->end())
        {
            worldMatrix = it->second.m_worldMatrix;
            return;
        }
    }

    worldMatrix = renderObject->m_II.m_Matrix;
}

void CMotionBlur::OnBeginFrame()
{
    assert(!gRenDev->m_pRT || gRenDev->m_pRT->IsMainThread());
    const AZ::u32 frameId = gRenDev->GetFrameID(false);
    const AZ::u32 objectIndex = frameId % CMotionBlur::s_maxObjectBuffers;

    m_Objects[objectIndex]->erase_if([frameId](const VectorMap<uintptr_t, MotionBlurObjectParameters >::value_type& object)
    {
        return (frameId - object.second.m_updateFrameId) > s_discardThreshold;
    });
}

void CMotionBlur::InsertNewElements()
{
    AZ::u32 nThreadID = gRenDev->m_RP.m_nProcessThreadID;
    if (m_FillData[nThreadID].empty())
    {
        return;
    }

    const AZ::u32 nFrameID = gRenDev->GetFrameID(false);
    const AZ::u32 nObjFrameWriteID = (nFrameID - 1) % CMotionBlur::s_maxObjectBuffers;

    m_FillData[nThreadID].CoalesceMemory();
    m_Objects[nObjFrameWriteID]->insert(&m_FillData[nThreadID][0], &m_FillData[nThreadID][0] + m_FillData[nThreadID].size());
    m_FillData[nThreadID].resize(0);
}

void CMotionBlur::FreeData()
{
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        m_FillData[i].clear();
    }

    for (size_t i = 0; i < CMotionBlur::s_maxObjectBuffers; ++i)
    {
        // m_Objects is a static object that is initialized in CMotionBlur::CMotionBlur, which is not guaranteed to be called by an application.
        // CMotionBlur::FreeData is a static cleanup function that is called regardless if CMotionBlur was created or not, so check to verify
        // we have valid data in m_Objects before trying to destruct the containers.
        if (m_Objects[i].get())
        {
            stl::reconstruct((*m_Objects[i]));
        }
    }
}

bool CD3D9Renderer::FX_MotionVectorGeneration(bool bEnable)
{
    bool bQualityCheck = CPostEffectsMgr::CheckPostProcessQuality(eRQ_Medium, eSQ_Medium);

    if (!bQualityCheck)
    {
        return false;
    }

    if IsCVarConstAccess(constexpr) (!CV_r_MotionVectors)
    {
        return false;
    }

    if (bEnable)
    {
        GetUtils().Log(" +++ Begin object motion vector generation +++ \n");

        // Re-use scene target rendertarget for velocity buffer
        RT_SetViewport(0, 0, CTexture::s_ptexSceneTarget->GetWidth(), CTexture::s_ptexSceneTarget->GetHeight());

        m_RP.m_PersFlags2 |= RBPF2_MOTIONBLURPASS;
    }
    else
    {
        FX_ResetPipe();
        gcpRendD3D->RT_SetViewport(0, 0, gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());

        m_RP.m_PersFlags2 &= ~RBPF2_MOTIONBLURPASS;

        GetUtils().Log(" +++ End object motion vector generation +++ \n");
    }

    return true;
}

void CMotionBlur::RenderObjectsVelocity()
{
    PROFILE_LABEL_SCOPE("OBJECTS VELOCITY");

    auto renderTarget = GetUtils().GetVelocityObjectRT();
    auto depthTarget = &gcpRendD3D->m_DepthBufferOrig;

    // Make sure the depth target is at least as large as the render target.
    // Since the render target lags behind by a frame this might not be the case
    // when resolution is changed from higher res to lower res.
    if (renderTarget != nullptr && depthTarget != nullptr &&
        renderTarget->GetWidth() <= depthTarget->nWidth &&
        renderTarget->GetHeight() <= depthTarget->nHeight)
    {
         // Render object velocities
        
        
        //The render targets are already in memory for gmem mode
        if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            gcpRendD3D->FX_PushRenderTarget(0, renderTarget, depthTarget);
        }
        
        uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
        int iTempX, iTempY, iWidth, iHeight;
        gcpRendD3D->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);
        const bool bAllowMotionVectors = CRenderer::CV_r_MotionVectors > 0;
        if (bAllowMotionVectors)
        {
            uint32 nBatchMask = 0;
            // Check for moving geometry
            if (!CRenderer::CV_r_MotionBlurGBufferVelocity)
            {
                nBatchMask |= SRendItem::BatchFlags(EFSLIST_GENERAL, gRenDev->m_RP.m_pRLD);
                nBatchMask |= SRendItem::BatchFlags(EFSLIST_SKIN, gRenDev->m_RP.m_pRLD);
            }

            nBatchMask |= SRendItem::BatchFlags(EFSLIST_TRANSP, gRenDev->m_RP.m_pRLD);
            if (nBatchMask & FB_MOTIONBLUR)
            {
                IRenderElement* pPrevRE = gRenDev->m_RP.m_pRE;
                gRenDev->m_RP.m_pRE = NULL;

                if (!gcpRendD3D->FX_MotionVectorGeneration(true))
                {
                    return;
                }

                if (!CRenderer::CV_r_MotionBlurGBufferVelocity)
                {
                    gcpRendD3D->FX_ProcessRenderList(EFSLIST_GENERAL, FB_MOTIONBLUR);
                    gcpRendD3D->FX_ProcessRenderList(EFSLIST_SKIN, FB_MOTIONBLUR);
                }

                gcpRendD3D->FX_ProcessRenderList(EFSLIST_TRANSP, FB_MOTIONBLUR);

                gcpRendD3D->FX_MotionVectorGeneration(false);

                gRenDev->m_RP.m_pRE = pPrevRE;
            }
        }

        gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
        
        if (!gcpRendD3D->FX_GetEnabledGmemPath(nullptr))
        {
            gcpRendD3D->FX_PopRenderTarget(0);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
// New Pipeline Pass

void CMotionBlurPass::Init()
{
}

void CMotionBlurPass::Shutdown()
{
    Reset();
}

void CMotionBlurPass::Reset()
{
    m_passMotionBlur.Reset();
    m_passCopy.Reset();
    m_passPacking.Reset();
    m_passTileGen1.Reset();
    m_passTileGen2.Reset();
    m_passNeighborMax.Reset();
}

float CMotionBlurPass::ComputeMotionScale()
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

void CMotionBlurPass::Execute()
{
    // Added a check to make sure we're only running the new pipeline motion blur while the new pipeline is enabled.
    if (CRenderer::CV_r_GraphicsPipeline <= 0)
    {
        return;
    }

    PROFILE_LABEL_SCOPE("MOTION_BLUR");

    CD3D9Renderer* rd = gcpRendD3D;
    CShader* pShader = CShaderMan::s_shPostMotionBlur;

    int vpX, vpY, vpWidth, vpHeight;
    rd->GetViewport(&vpX, &vpY, &vpWidth, &vpHeight);

    // Check if DOF is enabled
    CDepthOfField* pDofRenderTech = (CDepthOfField*)PostEffectMgr()->GetEffect(ePFX_eDepthOfField);
    DepthOfFieldParameters dofParameters = pDofRenderTech->GetParameters();
    const bool bGatherDofEnabled = CRenderer::CV_r_dof > 0 && dofParameters.m_bEnabled;

    Matrix44A mViewProjPrev = CMotionBlur::GetPrevView();
    Matrix44 mViewProj = GetUtils().m_pView;
    mViewProjPrev = mViewProjPrev * GetUtils().m_pProj * GetUtils().m_pScaleBias;
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

        CMotionBlur* pMB = (CMotionBlur*)PostEffectMgr()->GetEffect(ePFX_eMotionBlur);
        const float maxRange = 32.0f;
        const float amount = clamp_tpl<float>(pMB->m_pRadBlurAmount->GetParam() / maxRange, 0.0f, 1.0f);
        const float radius = 1.0f / clamp_tpl<float>(pMB->m_pRadBlurRadius->GetParam(), 1e-6f, 2.0f);
        const Vec4 blurDir = pMB->m_pDirectionalBlurVec->GetParamVec4();
        const Vec4 dirBlurParam = Vec4(blurDir.x * (maxRange / (float)vpWidth), blurDir.y * (maxRange / (float)vpHeight), (float)vpWidth / (float)vpHeight, 1.0f);
        const Vec4 radBlurParam = Vec4(pMB->m_pRadBlurScreenPosX->GetParam() * dirBlurParam.z, pMB->m_pRadBlurScreenPosY->GetParam(), radius * amount,  amount);

        const bool bRadialBlur = amount + (blurDir.x * blurDir.x) + (blurDir.y * blurDir.y) > 1.0f / (float)vpWidth;

        m_passPacking.SetRenderTarget(0, pVelocityRT);
        m_passPacking.SetTechnique(pShader, techPackVelocities, bRadialBlur ? g_HWSR_MaskBit[HWSR_SAMPLE0] : 0);
        m_passPacking.SetState(GS_NODEPTHTEST);
        m_passPacking.SetTextureSamplerPair(0, CTexture::s_ptexZTarget, texStatePoint);
        m_passPacking.SetTextureSamplerPair(1, CTexture::s_ptexHDRTarget, texStatePoint);
        m_passPacking.SetTextureSamplerPair(2, GetUtils().GetVelocityObjectRT(), texStatePoint);
        m_passPacking.SetRequireWorldPos(true);

        m_passPacking.BeginConstantUpdate();
        mViewProjPrev.Transpose();
        pShader->FXSetPSFloat(viewProjPrevName, (Vec4*)mViewProjPrev.GetData(), 4);
        pShader->FXSetPSFloat(dirBlurName, &dirBlurParam, 1);
        pShader->FXSetPSFloat(radBlurName, &radBlurParam, 1);
        const Vec4 motionBlurParams = Vec4(ComputeMotionScale(), 1.0f / tileCountX, 1.0f / tileCountX * CRenderer::CV_r_MotionBlurCameraMotionScale, 0);
        pShader->FXSetPSFloat(motionBlurParamName, &motionBlurParams, 1);
        m_passPacking.Execute();
    }

    {
        PROFILE_LABEL_SCOPE("VELOCITY TILES");

        static CCryNameTSCRC techVelocityTileGen("VelocityTileGen");
        static CCryNameTSCRC techTileNeighborhood("VelocityTileNeighborhood");

        // Tile generation first pass
        {
            m_passTileGen1.SetRenderTarget(0, CTexture::s_ptexVelocityTiles[0]);
            m_passTileGen1.SetTechnique(pShader, techVelocityTileGen, 0);
            m_passTileGen1.SetState(GS_NODEPTHTEST);
            m_passTileGen1.SetTextureSamplerPair(0, pVelocityRT, texStatePoint);

            m_passTileGen1.BeginConstantUpdate();
            Vec4 params = Vec4((float)pVelocityRT->GetWidth(), (float)pVelocityRT->GetHeight(), ceilf((float)gcpRendD3D->GetWidth() / tileCountX), 0);
            pShader->FXSetPSFloat(motionBlurParamName, &params, 1);
            m_passTileGen1.Execute();
        }

        // Tile generation second pass
        {
            m_passTileGen2.SetRenderTarget(0, CTexture::s_ptexVelocityTiles[1]);
            m_passTileGen2.SetTechnique(pShader, techVelocityTileGen, 0);
            m_passTileGen2.SetState(GS_NODEPTHTEST);
            m_passTileGen2.SetTextureSamplerPair(0, CTexture::s_ptexVelocityTiles[0], texStatePoint);

            m_passTileGen2.BeginConstantUpdate();
            Vec4 params = Vec4((float)CTexture::s_ptexVelocityTiles[0]->GetWidth(), (float)CTexture::s_ptexVelocityTiles[0]->GetHeight(), ceilf((float)gcpRendD3D->GetHeight() / tileCountY), 1);
            pShader->FXSetPSFloat(motionBlurParamName, &params, 1);
            m_passTileGen2.Execute();
        }

        // Neighborhood max
        {
            m_passNeighborMax.SetRenderTarget(0, CTexture::s_ptexVelocityTiles[2]);
            m_passNeighborMax.SetTechnique(pShader, techTileNeighborhood, 0);
            m_passNeighborMax.SetState(GS_NODEPTHTEST);
            m_passNeighborMax.SetTextureSamplerPair(0, CTexture::s_ptexVelocityTiles[1], texStatePoint);

            m_passNeighborMax.BeginConstantUpdate();
            Vec4 params = Vec4(1.0f / tileCountX, 1.0f / tileCountY, 0, 0);
            pShader->FXSetPSFloat(motionBlurParamName, &params, 1);
            m_passNeighborMax.Execute();
        }
    }

    {
        PROFILE_LABEL_SCOPE("MOTION VECTOR APPLY");

        static CCryNameTSCRC techMotionBlur("MotionBlur");

        if (bGatherDofEnabled)
        {
            m_passCopy.Execute(CTexture::s_ptexHDRTarget, CTexture::s_ptexSceneTargetR11G11B10F[0]);
        }

        uint64 rtMask = 0;
        rtMask |= (CRenderer::CV_r_MotionBlurQuality >= 2) ? g_HWSR_MaskBit[HWSR_SAMPLE2] : 0;
        rtMask |= (CRenderer::CV_r_MotionBlurQuality == 1) ? g_HWSR_MaskBit[HWSR_SAMPLE1] : 0;

        m_passMotionBlur.SetRenderTarget(0, CTexture::s_ptexHDRTarget);
        m_passMotionBlur.SetTechnique(pShader, techMotionBlur, rtMask);
        m_passMotionBlur.SetState(GS_NODEPTHTEST | GS_BLSRC_ONE | GS_BLDST_ONEMINUSSRCALPHA);
        m_passMotionBlur.SetTextureSamplerPair(0, bGatherDofEnabled ? CTexture::s_ptexSceneTargetR11G11B10F[0] : CTexture::s_ptexHDRTargetPrev, texStateLinear);
        m_passMotionBlur.SetTextureSamplerPair(1, pVelocityRT, texStatePoint);
        m_passMotionBlur.SetTextureSamplerPair(2, CTexture::s_ptexVelocityTiles[2], texStatePoint);

        m_passMotionBlur.BeginConstantUpdate();
        Vec4 params = Vec4(1.0f / tileCountX, 1.0f / tileCountY, 0, 0);
        pShader->FXSetPSFloat(motionBlurParamName, &params, 1);
        m_passMotionBlur.Execute();
    }
}
