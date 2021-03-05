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
#include "../../Cry3DEngine/Environment/OceanEnvironmentBus.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////

const char* CSceneSnow::GetName() const
{
    return "SceneSnow";
}

bool CSceneSnow::Preprocess()
{
    //bool bSnowActive = IsActiveSnow();

    // HDR now.
    return false;//bSnowActive;
}

int CSceneSnow::CreateResources()
{
    Release();

    // Already generated ? No need to proceed
    if (!m_pClusterList.empty())
    {
        return 1;
    }

    m_nNumClusters = max<int> (1, CRenderer::CV_r_snowFlakeClusters);
    m_pClusterList.reserve(m_nNumClusters);
    for (int p = 0; p < m_nNumClusters; p++)
    {
        SSnowCluster* pDrop = new SSnowCluster;
        m_pClusterList.push_back(pDrop);
    }

    return 1;
}

void CSceneSnow::Release()
{
    if (m_pClusterList.empty())
    {
        return;
    }

    SSnowClusterItor pItor, pItorEnd = m_pClusterList.end();
    for (pItor = m_pClusterList.begin(); pItor != pItorEnd; ++pItor)
    {
        SAFE_DELETE((*pItor));
    }
    m_pClusterList.clear();

    m_pSnowFlakeMesh = NULL;
}


void CSceneSnow::Reset([[maybe_unused]] bool bOnSpecChange)
{
    m_pActive->ResetParam(0.0f);
    m_nAliveClusters = 0;

    m_pSnowFlakeMesh = NULL;
}


bool CSceneSnow::IsActiveSnow()
{
    //static float s_fLastSpawnTime = -1.0f;

    if ((m_pActive->GetParam() > 0.09f && CRenderer::CV_r_snow && m_SnowVolParams.m_fSnowFallBrightness > 0.005f && m_SnowVolParams.m_nSnowFlakeCount > 0) || m_nAliveClusters)
    {
        //s_fLastSpawnTime = 0.0f;
        return true;
    }

    /*if( s_fLastSpawnTime == 0.0f)
    {
        s_fLastSpawnTime = PostProcessUtils().m_pTimer->GetCurrTime();
    }

    if( fabs(PostProcessUtils().m_pTimer->GetCurrTime() - s_fLastSpawnTime) < 1.0f )
    {
        return true;
    }*/

    return false;
}

bool CSceneSnow::GenerateClusterMesh()
{
    int iRTWidth  = gcpRendD3D->GetWidth();
    int iRTHeight = gcpRendD3D->GetHeight();
    float fAspect = float(iRTWidth) / iRTHeight;

    // Vertex offsets for sprite expansion.
    static const Vec2 vVertOffsets[6] =
    {
        Vec2(1,  1),
        Vec2(-1,  1),
        Vec2(1, -1),
        Vec2(1, -1),
        Vec2(-1,  1),
        Vec2(-1, -1)
    };

    // Create mesh if there isn't one or if the count has changed.
    if (!m_pSnowFlakeMesh || (m_nFlakesPerCluster != m_SnowVolParams.m_nSnowFlakeCount))
    {
        m_pSnowFlakeMesh = NULL;

        m_nFlakesPerCluster = m_SnowVolParams.m_nSnowFlakeCount;
        m_nSnowFlakeVertCount = m_nFlakesPerCluster * 6;

        SVF_P3F_T2F_T3F* pSnowFlakes = new SVF_P3F_T2F_T3F[m_nSnowFlakeVertCount];

        float fUserSize = 0.0075f;
        float fUserSizeVar = 0.0025f;

        // Loop through number of flake sprites (6 verts per sprite).
        for (int p = 0; p < m_nSnowFlakeVertCount; p += 6)
        {
            Vec3 vPosition = Vec3(cry_random(-10.0f, 10.0f), cry_random(-10.0f, 10.0f), cry_random(-10.0f, 10.0f));
            float fSize = fUserSize + fUserSizeVar * cry_random(-1.0f, 1.0f);
            float fRandPhase = cry_random(0.0f, 10.0f);

            // Triangle strip, each vertex gets it's own offset for sprite expansion.
            for (int i = 0; i < 6; ++i)
            {
                int nIdx = p + i;
                pSnowFlakes[nIdx].p = vPosition;
                pSnowFlakes[nIdx].st0 = Vec2(vVertOffsets[i].x, vVertOffsets[i].y);
                pSnowFlakes[nIdx].st1 = Vec3(fSize, fSize * fAspect, fRandPhase);
            }
        }

        m_pSnowFlakeMesh = gRenDev->CreateRenderMeshInitialized(pSnowFlakes, m_nSnowFlakeVertCount, eVF_P3F_T2F_T3F, 0, 0, prtTriangleList, "SnowFlakeBuffer", "SnowFlakeBuffer");

        delete [] pSnowFlakes;

        // If the mesh wasn't initialized properly, return false.
        if (!m_pSnowFlakeMesh)
        {
            return false;
        }
    }

    return true;
}

void CSceneSnow::SpawnCluster(SSnowCluster*& pCluster)
{
    static SSnowCluster pNewDrop;

    static float fLastSpawnTime = 0.0f;

    pCluster->m_pPos = gcpRendD3D->GetViewParameters().vOrigin;

    pCluster->m_pPos.x += cry_random(-1.0f, 1.0f) * 15;
    pCluster->m_pPos.y += cry_random(-1.0f, 1.0f) * 15;
    pCluster->m_pPos.z += cry_random(-1.0f, 1.0f) * 5 + 4;

    pCluster->m_pPosPrev = pCluster->m_pPos;
    pCluster->m_fLifeTime = (pNewDrop.m_fLifeTime / std::max(1.0f, m_SnowVolParams.m_fSnowFallGravityScale) + pNewDrop.m_fLifeTimeVar * cry_random(-1.0f, 1.0f));

    // Increase/decrease weight randomly
    pCluster->m_fWeight = clamp_tpl<float>(pNewDrop.m_fWeight + pNewDrop.m_fWeightVar * cry_random(-1.0f, 1.0f), 0.1f, 1.0f);

    pCluster->m_fSpawnTime = PostProcessUtils().m_pTimer->GetCurrTime();
    fLastSpawnTime = PostProcessUtils().m_pTimer->GetCurrTime();
}

void CSceneSnow::UpdateClusters()
{
    SSnowClusterItor pItor, pItorEnd = m_pClusterList.end();

    bool bAllowSpawn(m_pActive->GetParam() > 0.005f && m_SnowVolParams.m_fSnowFallBrightness > 0.005f);

    const float fCurrFrameTime = gEnv->pTimer->GetFrameTime();
    const float fGravity = fCurrFrameTime * m_SnowVolParams.m_fSnowFallGravityScale;
    const float fWind = m_SnowVolParams.m_fSnowFallWindScale;

    m_nAliveClusters = 0;

    Vec3 vCameraPos = gcpRendD3D->GetViewParameters().vOrigin;

    for (pItor = m_pClusterList.begin(); pItor != pItorEnd; ++pItor)
    {
        SSnowCluster* pCurr = (*pItor);

        float fCurrLifeTime = pCurr->m_fLifeTime > 0 ? (PostProcessUtils().m_pTimer->GetCurrTime() - pCurr->m_fSpawnTime) / pCurr->m_fLifeTime : 0;
        float fClusterDist = pCurr->m_pPos.GetDistance(vCameraPos);

        // Cluster is too small, died or is out of range, respawn.
        if (fabs(fCurrLifeTime) > 1.0f || pCurr->m_fLifeTime < 0 || fClusterDist > 30.0f)
        {
            if (bAllowSpawn)
            {
                SpawnCluster(pCurr);
            }
            else
            {
                pCurr->m_fLifeTime = -1.0f;
                continue;
            }
        }

        // Previous position (for blur).
        pCurr->m_pPosPrev = pCurr->m_pPos;

        // Apply gravity force.
        if (m_SnowVolParams.m_fSnowFallGravityScale)
        {
            Vec3 vGravity(0.0f, 0.0f, -9.8f);
            pCurr->m_pPos += vGravity * fGravity * pCurr->m_fWeight;
        }

        // Apply wind force.
        if (m_SnowVolParams.m_fSnowFallWindScale)
        {
            Vec3 vWindVec = gEnv->p3DEngine->GetWind(AABB(pCurr->m_pPos - Vec3(10, 10, 10), pCurr->m_pPos + Vec3(10, 10, 10)), false);
            pCurr->m_pPos += vWindVec * pCurr->m_fWeight * fWind;
        }

        // Increment count.
        m_nAliveClusters++;
    }
}

void CSceneSnow::Render()
{
    // Number of clusters has changed, reallocate resources.
    const int nNumClusters = max<int>(1, CRenderer::CV_r_snowFlakeClusters);
    if (m_nNumClusters != nNumClusters)
    {
        CreateResources();
    }

    // Generate the cluster mesh.
    if (!GenerateClusterMesh())
    {
        return;
    }

    // If not enough flakes, skip.
    if (m_nFlakesPerCluster < 1)
    {
        return;
    }

    if (!PostProcessUtils().m_pTimer)
    {
        return;
    }

    gRenDev->m_cEF.mfRefreshSystemShader("PostEffectsGame", CShaderMan::s_shPostEffectsGame);

    PostProcessUtils().StretchRect(CTexture::s_ptexHDRTarget, CTexture::s_ptexSceneTarget);

    UpdateClusters();

    CTexture* pSceneSrc = CTexture::s_ptexHDRTarget;
    CTexture* pVelocitySrc = CTexture::s_ptexVelocity;
    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_snow_halfres)
    {
        pSceneSrc = CTexture::s_ptexHDRTargetScaledTmp[0];
        pVelocitySrc = CTexture::s_ptexBackBufferScaled[0];

        gcpRendD3D->FX_ClearTarget(pSceneSrc, Clr_Transparent);
        gcpRendD3D->FX_ClearTarget(pVelocitySrc, Clr_Static);
    }

    PROFILE_LABEL_PUSH("SCENE_SNOW_FLAKES");

    // Render to HDR and velocity.
    gcpRendD3D->FX_PushRenderTarget(0, pSceneSrc, CRenderer::CV_r_snow_halfres ? NULL : &gcpRendD3D->m_DepthBufferOrig);
    gcpRendD3D->FX_PushRenderTarget(1, pVelocitySrc, NULL);

    gcpRendD3D->FX_SetColorDontCareActions(0, false, false);
    gcpRendD3D->FX_SetColorDontCareActions(1, true, false);
    gcpRendD3D->FX_SetStencilDontCareActions(0, true, true);
    gcpRendD3D->FX_SetDepthDontCareActions(0, false, true);

    DrawClusters();

    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->FX_PopRenderTarget(1);

    PROFILE_LABEL_POP("SCENE_SNOW_FLAKES");

    if IsCVarConstAccess(constexpr) (CRenderer::CV_r_snow_halfres)
    {
        HalfResComposite();
    }

    gcpRendD3D->FX_Commit();
}

void CSceneSnow::HalfResComposite()
{
    PROFILE_LABEL_PUSH("SCENE_SNOW_FLAKES_HALFRES_COMPOSITE");
    gcpRendD3D->FX_PushRenderTarget(0, CTexture::s_ptexHDRTarget, NULL);
    gcpRendD3D->FX_PushRenderTarget(1, CTexture::s_ptexVelocity, NULL);

    static CCryNameTSCRC pTechNameComposite = "SnowHalfResComposite";
    SD3DPostEffectsUtils::ShBeginPass(CShaderMan::s_shPostEffectsGame, pTechNameComposite, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);
    gcpRendD3D->FX_SetState(GS_NODEPTHTEST | GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA);
    PostProcessUtils().SetTexture(CTexture::s_ptexHDRTargetScaledTmp[0], 0, FILTER_LINEAR);
    PostProcessUtils().SetTexture(CTexture::s_ptexBackBufferScaled[0], 1, FILTER_POINT);
    SD3DPostEffectsUtils::DrawFullScreenTri(gcpRendD3D->GetWidth(), gcpRendD3D->GetHeight());
    SD3DPostEffectsUtils::ShEndPass();

    gcpRendD3D->FX_PopRenderTarget(0);
    gcpRendD3D->FX_PopRenderTarget(1);
    PROFILE_LABEL_POP("SCENE_SNOW_FLAKES_HALFRES_COMPOSITE");
}

void CSceneSnow::DrawClusters()
{
    // Previous view projection matrix for motion blur.
    float fCurrFrameTime = gEnv->pTimer->GetFrameTime();
    Matrix44A mViewProjPrev = gRenDev->GetPreviousFrameMatrixSet().m_ViewMatrix;
    mViewProjPrev = mViewProjPrev * GetUtils().m_pProj;
    mViewProjPrev.Transpose();

    //clear and set render flags
    uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;
    gRenDev->m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0]);

    if (m_RainVolParams.bApplyOcclusion)
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    static CCryNameTSCRC pTech0Name("SceneSnow");

    static CCryNameR pSnowFlakeParamName("vSnowFlakeParams");
    Vec4 vSnowFlakeParams = Vec4(m_SnowVolParams.m_fSnowFallBrightness, min(10.0f, m_SnowVolParams.m_fSnowFlakeSize), m_SnowVolParams.m_fSnowFallTurbulence, m_SnowVolParams.m_fSnowFallTurbulenceFreq);

    SSnowClusterItor pItor, pItorEnd = m_pClusterList.end();
    for (pItor = m_pClusterList.begin(); pItor != pItorEnd; ++pItor)
    {
        SSnowCluster* pCurr = (*pItor);
        if (pCurr->m_fLifeTime < 0)
        {
            continue;
        }

        // Don't render if indoors or under water.
        const float oceanLevel = OceanToggle::IsActive() ? OceanRequest::GetWaterLevel(pCurr->m_pPos) : gEnv->p3DEngine->GetWaterLevel(&pCurr->m_pPos);
        if (gEnv->p3DEngine->GetVisAreaFromPos(pCurr->m_pPos) || pCurr->m_pPos.z < oceanLevel)
        {
            continue;
        }

        PostProcessUtils().ShBeginPass(CShaderMan::s_shPostEffectsGame, pTech0Name, 0);

        // Snowflake params.
        CShaderMan::s_shPostEffectsGame->FXSetVSFloat(pSnowFlakeParamName, &vSnowFlakeParams, 1);
        CShaderMan::s_shPostEffectsGame->FXSetPSFloat(pSnowFlakeParamName, &vSnowFlakeParams, 1);

        // Cluster params.
        static CCryNameR vSnowClusterPosName("vSnowClusterPos");
        Vec4 vSnowClusterPos = Vec4(pCurr->m_pPos, 1.0f);
        CShaderMan::s_shPostEffectsGame->FXSetVSFloat(vSnowClusterPosName, &vSnowClusterPos, 1);

        static CCryNameR vSnowClusterPosPrevName("vSnowClusterPosPrev");
        Vec4 vSnowClusterPosPrev = Vec4(pCurr->m_pPosPrev, 1.0f);
        CShaderMan::s_shPostEffectsGame->FXSetVSFloat(vSnowClusterPosPrevName, &vSnowClusterPosPrev, 1);

        // Motion blur params.
        static CCryNameR pViewProjPrevName("mViewProjPrev");
        CShaderMan::s_shPostEffectsGame->FXSetVSFloat(pViewProjPrevName, (Vec4*)mViewProjPrev.GetData(), 4);

        // Occlusion params.
        if (m_RainVolParams.bApplyOcclusion)
        {
            static CCryNameR mSnowOccMatrName("mSnowOccMatr");
            CShaderMan::s_shPostEffectsGame->FXSetVSFloat(mSnowOccMatrName, (Vec4*)m_RainVolParams.matOccTransRender.GetData(), 3);
        }

        gcpRendD3D->FX_Commit();
        if (!FAILED(gcpRendD3D->FX_SetVertexDeclaration(0, eVF_P3F_T2F_T3F)))
        {
            size_t offset(0);
            CRenderMesh* pSnowFlakeMesh = static_cast<CRenderMesh*>(m_pSnowFlakeMesh.get());
            //CRenderMesh* pSnowFlakeMesh((CRenderMesh*) m_pSnowFlakeMesh);
            pSnowFlakeMesh->CheckUpdate(0);
            D3DBuffer* pVB = gcpRendD3D->m_DevBufMan.GetD3D(pSnowFlakeMesh->GetVBStream(VSF_GENERAL), &offset);
            gcpRendD3D->FX_SetVStream(0, pVB, offset, pSnowFlakeMesh->GetStreamStride(VSF_GENERAL));
            gcpRendD3D->FX_SetIStream(0, 0, Index16);
            //STtryfix  D3DIndexBuffer *pIB = gcpRendD3D->m_DevBufMan.GetD3DIB(pSnowFlakeMesh->GetIBStream(), &offset);

            gcpRendD3D->FX_DrawPrimitive(eptTriangleList, 0, m_nSnowFlakeVertCount);
        }

        PostProcessUtils().ShEndPass();
    }
    gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
}
