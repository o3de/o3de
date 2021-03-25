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

#if defined(FEATURE_SVO_GI)

#include "DriverD3D.h"
#include "I3DEngine.h"
#include "D3DPostProcess.h"
#include "D3D_SVO.h"
#include "D3DTiledShading.h"
#include "../Common/Include_HLSL_CPP_Shared.h"
#include "../Common/TypedConstantBuffer.h"
#include "../Common/Textures/TextureManager.h"

#include <AzFramework/Terrain/TerrainDataRequestBus.h>


CSvoRenderer* CSvoRenderer::s_pInstance = 0;

CSvoRenderer::CSvoRenderer()
{
    m_pRT_AIR_MIN =
        m_pRT_AIR_MAX =
        m_pRT_AIR_SHAD =
        m_pRT_NID_0 = NULL;

    m_nTexStateTrilinear = CTexture::GetTexState(STexState(FILTER_TRILINEAR, true));
    m_nTexStateLinear = CTexture::GetTexState(STexState(FILTER_LINEAR, true));
    m_nTexStatePoint = CTexture::GetTexState(STexState(FILTER_POINT, true));
    m_nTexStateLinearWrap = CTexture::GetTexState(STexState(FILTER_LINEAR, false));

    m_pNoiseTex = nullptr; 
    m_pRsmNormlMap = m_pRsmColorMap = 0;
    m_pRsmPoolNor = m_pRsmPoolCol = 0;

    m_pShader = nullptr;

    ZeroStruct(m_arrNodesForUpdate);
    ZeroStruct(m_nCurPropagationPassID);

    SVOGILegacyRequestBus::Broadcast(&SVOGILegacyRequests::RegisterMutex, &m_renderMutex);
}

void SSvoTargetsSet::Release()
{
    SAFE_RELEASE(pRT_RGB_DEM_MIN_0);
    SAFE_RELEASE(pRT_ALD_DEM_MIN_0);
    SAFE_RELEASE(pRT_RGB_DEM_MAX_0);
    SAFE_RELEASE(pRT_ALD_DEM_MAX_0);
    SAFE_RELEASE(pRT_RGB_DEM_MIN_1);
    SAFE_RELEASE(pRT_ALD_DEM_MIN_1);
    SAFE_RELEASE(pRT_RGB_DEM_MAX_1);
    SAFE_RELEASE(pRT_ALD_DEM_MAX_1);
    SAFE_RELEASE(pRT_FIN_OUT_0);
    SAFE_RELEASE(pRT_FIN_OUT_1);
    SAFE_RELEASE(pRT_ALD_0);
    SAFE_RELEASE(pRT_ALD_1);
    SAFE_RELEASE(pRT_RGB_0);
    SAFE_RELEASE(pRT_RGB_1);
}

CSvoRenderer::~CSvoRenderer()
{
    m_tsDiff.Release();
    m_tsSpec.Release();

    SAFE_RELEASE(m_pRT_AIR_MIN);
    SAFE_RELEASE(m_pRT_AIR_MAX);
    SAFE_RELEASE(m_pRT_AIR_SHAD);
    SAFE_RELEASE(m_pRT_NID_0);
    SAFE_RELEASE(m_pRsmColorMap);
    SAFE_RELEASE(m_pRsmNormlMap);
    SAFE_RELEASE(m_pRsmPoolCol);
    SAFE_RELEASE(m_pRsmPoolNor);
    SVOGILegacyRequestBus::Broadcast(&SVOGILegacyRequests::UnregisterMutex);
}

CSvoRenderer* CSvoRenderer::GetInstance(bool bCheckAlloce)
{
    if (!s_pInstance && bCheckAlloce)
    {
        s_pInstance = new CSvoRenderer();
    }

    return s_pInstance;
}

void CSvoRenderer::Release()
{
    SAFE_DELETE(s_pInstance);
}

void CSvoRenderer::UpdateCompute()
{
    if (!IsActive())
    {
        return;
    }

    UpdatePassConstantBuffer();

    static int nTI_Compute_FrameId = -1;
    if (nTI_Compute_FrameId == gRenDev->GetFrameID(false))
    {
        return;
    }
    nTI_Compute_FrameId = gRenDev->GetFrameID(false);

    gEnv->p3DEngine->GetSvoStaticTextures(m_texInfo, &m_arrLightsStatic, &m_arrLightsDynamic);
    if (m_texInfo.pTexTree == nullptr)
    {
        return;
    }

    CD3D9Renderer* rd = gcpRendD3D;

    if (!m_pShader)
    {
        gcpRendD3D->m_cEF.mfRefreshSystemShader("Total_Illumination", m_pShader);
    }

    m_nodesForStaticUpdate.Clear();
    m_nodesForDynamicUpdate.Clear();


    if (GetIntegratioMode())
    {
        gEnv->p3DEngine->GetSvoBricksForUpdate(m_arrNodeInfo, false);

        for (int n = 0; n < m_arrNodeInfo.Count(); n++)
        {
            m_nodesForStaticUpdate.Add(m_arrNodeInfo[n]);
        }

        m_arrNodeInfo.Clear();
    }

    if (GetIntegratioMode())
    {
        gEnv->p3DEngine->GetSvoBricksForUpdate(m_arrNodeInfo, true);

        for (int n = 0; n < m_arrNodeInfo.Count(); n++)
        {
            m_nodesForDynamicUpdate.Add(m_arrNodeInfo[n]);
        }

        m_arrNodeInfo.Clear();
    }

    {
        // get UAV access
        vp_RGB0.Init(m_texInfo.pTexRgb0);
        vp_RGB1.Init(m_texInfo.pTexRgb1);
        vp_DYNL.Init(m_texInfo.pTexDynl);
        vp_RGB2.Init(m_texInfo.pTexRgb2);
        vp_RGB3.Init(m_texInfo.pTexRgb3);
        vp_NORM.Init(m_texInfo.pTexNorm);
        vp_ALDI.Init(m_texInfo.pTexAldi);
        vp_OPAC.Init(m_texInfo.pTexOpac);
    }

    if (!IsActive() || !m_texInfo.bSvoReady || !GetIntegratioMode())
    {
        return;
    }

    // force sync shaders compiling
    int nPrevAsync = CRenderer::CV_r_shadersasynccompiling;
    CRenderer::CV_r_shadersasynccompiling = 0;

    // clear pass
    {
        PROFILE_LABEL_SCOPE("TI_INJECT_CLEAR");

        for (int nNodesForUpdateStartIndex = 0; nNodesForUpdateStartIndex < m_nodesForStaticUpdate.Count(); )
        {
            ExecuteComputeShader(m_pShader, "ComputeClearBricks", eCS_ClearBricks, &nNodesForUpdateStartIndex, 0, m_nodesForStaticUpdate);
        }
    }

    {
        PROFILE_LABEL_SCOPE("TI_INJECT_LIGHT");

        for (int nNodesForUpdateStartIndex = 0; nNodesForUpdateStartIndex < m_nodesForStaticUpdate.Count(); )
        {
            ExecuteComputeShader(m_pShader, "ComputeDirectStaticLighting", eCS_InjectStaticLights, &nNodesForUpdateStartIndex, 0, m_nodesForStaticUpdate);
        }
    }

    if (gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->GetFVal())
    {
        if (gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->GetIVal() > 1)
        {
            PROFILE_LABEL_SCOPE("TI_INJECT_REFL0");

            m_nCurPropagationPassID = 0;
            for (int nNodesForUpdateStartIndex = 0; nNodesForUpdateStartIndex < m_nodesForStaticUpdate.Count(); )
            {
                ExecuteComputeShader(m_pShader, "ComputePropagateLighting", eCS_PropagateLighting_1to2, &nNodesForUpdateStartIndex, 0, m_nodesForStaticUpdate);
            }
        }

        if (gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->GetIVal() > 2)
        {
            PROFILE_LABEL_SCOPE("TI_INJECT_REFL1");

            m_nCurPropagationPassID++;
            for (int nNodesForUpdateStartIndex = 0; nNodesForUpdateStartIndex < m_nodesForStaticUpdate.Count(); )
            {
                ExecuteComputeShader(m_pShader, "ComputePropagateLighting", eCS_PropagateLighting_2to3, &nNodesForUpdateStartIndex, 0, m_nodesForStaticUpdate);
            }
        }
    }

    static int nLightsDynamicCountPrevFrame = 0;

    if ((m_arrLightsDynamic.Count() || nLightsDynamicCountPrevFrame))
    {
        PROFILE_LABEL_SCOPE("TI_INJECT_DYNL");

        // TODO: cull not affected nodes

        for (int nNodesForUpdateStartIndex = 0; nNodesForUpdateStartIndex < m_nodesForDynamicUpdate.Count(); )
        {
            ExecuteComputeShader(m_pShader, "ComputeDirectDynamicLighting", eCS_InjectDynamicLights, &nNodesForUpdateStartIndex, 0, m_nodesForDynamicUpdate);
        }
    }

    nLightsDynamicCountPrevFrame = m_arrLightsDynamic.Count();

    CRenderer::CV_r_shadersasynccompiling = nPrevAsync;

}

void CSvoRenderer::ExecuteComputeShader(CShader* pSH, const char* szTechFinalName, EComputeStages eRenderStage, int* pnNodesForUpdateStartIndex, [[maybe_unused]] int nObjPassId, PodArray<I3DEngine::SSvoNodeInfo>& arrNodesForUpdate)
{

    FUNCTION_PROFILER_RENDERER;

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    SetShaderFlags(true, false);

    SD3DPostEffectsUtils::ShBeginPass(pSH, szTechFinalName, FEF_DONTSETSTATES);

    if (!gRenDev->m_RP.m_pShader)
    {
        gEnv->pLog->LogWarning("%s: Technique not found: %s", __FUNC__, szTechFinalName);
        (*pnNodesForUpdateStartIndex) += 1000;
        return;
    }

    {
        static CCryNameR param("SVO_PropagationPass");
        Vec4 value((float)(m_nCurPropagationPassID), 0, 0, 0);
        m_pShader->FXSetCSFloat(param, (Vec4*)&value, 1);
    }

    {
        static CCryNameR parameterName6("SVO_FrameIdByte");
        Vec4 ttt((float)(rd->GetFrameID(false) & 255), (float)rd->GetFrameID(false), 3, 4);
        if (rd->GetActiveGPUCount() > 1)
        {
            ttt.x = 0;
        }
        m_pShader->FXSetCSFloat(parameterName6, (Vec4*)&ttt, 1);
    }

    {
        static CCryNameR parameterName6("SVO_CamPos");
        Vec4 ttt(gEnv->pSystem->GetViewCamera().GetPosition(), 0);
        m_pShader->FXSetCSFloat(parameterName6, (Vec4*)&ttt, 1);
    }

    {
        Matrix44A mViewProj;
        mViewProj = gcpRendD3D->m_ViewProjMatrix;
        mViewProj.Transpose();

        static CCryNameR paramName("g_mViewProj");
        m_pShader->FXSetCSFloat(paramName, alias_cast<Vec4*>(&mViewProj), 4);

        static CCryNameR paramNamePrev("g_mViewProjPrev");
        m_pShader->FXSetPSFloat(paramNamePrev, alias_cast<Vec4*>(&m_matViewProjPrev), 4);
    }

    {
        const CameraViewParameters& rc = gcpRendD3D->GetViewParameters();
        float zn = rc.fNear;
        float zf = rc.fFar;
        float hfov = gcpRendD3D->GetCamera().GetHorizontalFov();
        Vec4 f;
        f[0] = zf / (zf - zn);
        f[1] = zn / (zn - zf);
        f[2] = 1.0f / hfov;
        f[3] = 1.0f;
        static CCryNameR paramName("SVO_ProjRatio");
        m_pShader->FXSetCSFloat(paramName, alias_cast<Vec4*>(&f), 1);
    }

    // setup SVO textures

    ID3D11DeviceContext* pDeviceCtx = &gcpRendD3D->GetDeviceContext();
    UINT UAVInitialCounts = 0;

    if (eRenderStage == eCS_InjectStaticLights)
    {
        SetupLightSources(m_arrLightsStatic, m_pShader, false);
        SetupNodesForUpdate(*pnNodesForUpdateStartIndex, arrNodesForUpdate);

        // update RGB1
        pDeviceCtx->CSSetUnorderedAccessViews(2, 1, &vp_RGB0.pUAV, &UAVInitialCounts);
        pDeviceCtx->CSSetUnorderedAccessViews(7, 1, &vp_RGB1.pUAV, &UAVInitialCounts);
        pDeviceCtx->CSSetUnorderedAccessViews(5, 1, &vp_NORM.pUAV, &UAVInitialCounts);

        if (vp_DYNL.pUAV)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(6, 1, &vp_DYNL.pUAV, &UAVInitialCounts);
        }

        SetupSvoTexturesForRead(m_texInfo, eHWSC_Compute, 0);

        SetupRsmSun(eHWSC_Compute);
    }
    else if (eRenderStage == eCS_InjectDynamicLights)
    {
        BindTiledLights(m_arrLightsDynamic, eHWSC_Compute);
        SetupLightSources(m_arrLightsDynamic, m_pShader, false);
        SetupNodesForUpdate(*pnNodesForUpdateStartIndex, arrNodesForUpdate);

        // update RGB
        pDeviceCtx->CSSetUnorderedAccessViews(2, 1, &vp_RGB0.pUAV, &UAVInitialCounts);

        if (gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->GetIVal() == 1)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(7, 1, &vp_RGB1.pUAV, &UAVInitialCounts);
        }
        if (gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->GetIVal() == 2)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(7, 1, &vp_RGB2.pUAV, &UAVInitialCounts);
        }
        if (gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->GetIVal() == 3)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(7, 1, &vp_RGB3.pUAV, &UAVInitialCounts);
        }

        pDeviceCtx->CSSetUnorderedAccessViews(5, 1, &vp_DYNL.pUAV, &UAVInitialCounts);

        SetupSvoTexturesForRead(m_texInfo, eHWSC_Compute, 0);
        if (!m_pNoiseTex)
        {
            m_pNoiseTex = CTextureManager::Instance()->GetDefaultTexture("DissolveNoiseMap");
        }
        m_pNoiseTex->Apply(15, m_nTexStateLinearWrap, -1, -1, -1, eHWSC_Compute);

        SetupRsmSun(eHWSC_Compute);
    }
    else if (eRenderStage == eCS_PropagateLighting_1to2)
    {
        SetupLightSources(m_arrLightsStatic, m_pShader, false);
        SetupNodesForUpdate(*pnNodesForUpdateStartIndex, arrNodesForUpdate);

        // update RGB2
        if (vp_RGB0.pUAV)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(0, 1, &vp_RGB0.pUAV, &UAVInitialCounts);
        }
        SetupSvoTexturesForRead(m_texInfo, eHWSC_Compute, 1); // input
        if (vp_RGB2.pUAV)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(5, 1, &vp_RGB2.pUAV, &UAVInitialCounts);
        }
        if (vp_ALDI.pUAV)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(6, 1, &vp_ALDI.pUAV, &UAVInitialCounts);
        }
        if (vp_DYNL.pUAV)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(7, 1, &vp_DYNL.pUAV, &UAVInitialCounts);
        }
    }
    else if (eRenderStage == eCS_PropagateLighting_2to3)
    {
        SetupLightSources(m_arrLightsStatic, m_pShader, false);
        SetupNodesForUpdate(*pnNodesForUpdateStartIndex, arrNodesForUpdate);

        // update RGB3
        if (vp_RGB0.pUAV)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(0, 1, &vp_RGB0.pUAV, &UAVInitialCounts);
        }
        SetupSvoTexturesForRead(m_texInfo, eHWSC_Compute, 2); // input
        if (vp_RGB3.pUAV)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(5, 1, &vp_RGB3.pUAV, &UAVInitialCounts);
        }
        if (vp_ALDI.pUAV)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(6, 1, &vp_ALDI.pUAV, &UAVInitialCounts);
        }
        if (vp_DYNL.pUAV)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(7, 1, &vp_DYNL.pUAV, &UAVInitialCounts);
        }
    }
    else if (eRenderStage == eCS_ClearBricks)
    {
        SetupNodesForUpdate(*pnNodesForUpdateStartIndex, arrNodesForUpdate);

        if (vp_RGB3.pUAV)
        {
            pDeviceCtx->CSSetUnorderedAccessViews(6, 1, &vp_RGB3.pUAV, &UAVInitialCounts);
        }
        pDeviceCtx->CSSetUnorderedAccessViews(7, 1, &vp_RGB1.pUAV, &UAVInitialCounts);
        pDeviceCtx->CSSetUnorderedAccessViews(5, 1, &vp_RGB2.pUAV, &UAVInitialCounts);
        pDeviceCtx->CSSetUnorderedAccessViews(0, 1, &vp_ALDI.pUAV, &UAVInitialCounts);
    }

    {
        rd->FX_Commit();

        uint32 nDispatchSizeX = 128;
        uint32 nDispatchSizeY = 16;

#ifdef CINEBOX_APP
        pDeviceCtx->Dispatch(nDispatchSizeX, nDispatchSizeY, 1);
#else
        rd->m_DevMan.Dispatch(nDispatchSizeX, nDispatchSizeY, 1);
#endif

        SD3DPostEffectsUtils::ShEndPass();
    }

    D3DUnorderedAccessView* pUAVNULL = NULL;

    pDeviceCtx->CSSetUnorderedAccessViews(7, 1, &pUAVNULL, &UAVInitialCounts);
    pDeviceCtx->CSSetUnorderedAccessViews(6, 1, &pUAVNULL, &UAVInitialCounts);
    pDeviceCtx->CSSetUnorderedAccessViews(5, 1, &pUAVNULL, &UAVInitialCounts);
    pDeviceCtx->CSSetUnorderedAccessViews(2, 1, &pUAVNULL, &UAVInitialCounts);
    pDeviceCtx->CSSetUnorderedAccessViews(1, 1, &pUAVNULL, &UAVInitialCounts);
    pDeviceCtx->CSSetUnorderedAccessViews(0, 1, &pUAVNULL, &UAVInitialCounts);

    CTexture::GetByID(vp_RGB0.nTexId)->Unbind();
    CTexture::GetByID(vp_RGB1.nTexId)->Unbind();
    CTexture::GetByID(vp_DYNL.nTexId)->Unbind();
    CTexture::GetByID(vp_RGB2.nTexId)->Unbind();
    CTexture::GetByID(vp_RGB3.nTexId)->Unbind();
    CTexture::GetByID(vp_NORM.nTexId)->Unbind();
    CTexture::GetByID(vp_ALDI.nTexId)->Unbind();
    CTexture::GetByID(vp_OPAC.nTexId)->Unbind();

    gcpRendD3D->GetTiledShading().UnbindForwardShadingResources(eHWSC_Compute);

    pSH->FXEnd();

}

CTexture* CSvoRenderer::GetGBuffer(int nId) // simplify branch compatibility
{
    CTexture* pRes;

    if (nId == 0)
    {
        pRes = CTexture::s_ptexSceneDiffuse;
    }
    else if (nId == 1)
    {
        pRes = CTexture::s_ptexSceneNormalsMap;
    }
    else if (nId == 2)
    {
        pRes = CTexture::s_ptexSceneSpecular;
    }
    else
    {
        pRes = 0;
    }

    return pRes;
}

void CSvoRenderer::ConeTracePass(SSvoTargetsSet* pTS)
{
    CheckAllocateRT(pTS == &m_tsSpec);

    if (!IsActive() || !m_pShader)
    {
        return;
    }

    const char* szTechFinalName = "ConeTracePass";

    if (m_texInfo.bSvoFreeze)
    {
        return;
    }

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    rd->FX_PushRenderTarget(0, pTS->pRT_ALD_0, NULL);
    rd->FX_PushRenderTarget(1, pTS->pRT_RGB_0, NULL);

    if (m_texInfo.pTexTree)
    {
        SetShaderFlags(pTS == &m_tsDiff);

        SD3DPostEffectsUtils::ShBeginPass(m_pShader, szTechFinalName, FEF_DONTSETTEXTURES /*| FEF_DONTSETSTATES*/);

        {
            Matrix44A matView;
            matView = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_cam.GetViewMatrix();
            Vec3 zAxis = matView.GetRow(1);
            matView.SetRow(1, -matView.GetRow(2));
            matView.SetRow(2, zAxis);
            float z = matView.m13;
            matView.m13 = -matView.m23;
            matView.m23 = z;

            SetupRsmSun(eHWSC_Pixel);

            static int nReprojFrameId = -1;
            if ((pTS == &m_tsDiff) && nReprojFrameId != rd->GetFrameID(false))
            {
                nReprojFrameId = rd->GetFrameID(false);

                Matrix44A matProj;
                const CCamera& cam = rd->m_RP.m_TI[rd->m_RP.m_nProcessThreadID].m_cam;
                mathMatrixPerspectiveFov(&matProj, cam.GetFov(), cam.GetProjRatio(), cam.GetNearPlane(), cam.GetFarPlane());
                static Matrix44A matPrevView = matView;
                static Matrix44A matPrevProj = matProj;
                rd->GetReprojectionMatrix(m_matReproj, matView, matProj, matPrevView, matPrevProj, cam.GetFarPlane());
                matPrevView = matView;
                matPrevProj = matProj;
            }

            {
                static CCryNameR parameterName5("SVO_ReprojectionMatrix");
                m_pShader->FXSetPSFloat(parameterName5, (Vec4*)m_matReproj.GetData(), 3);
            }

            {
                static CCryNameR parameterName6("SVO_FrameIdByte");
                Vec4 ttt((float)(rd->GetFrameID(false) & 255), (float)rd->GetFrameID(false), 3, 4);
                if (rd->GetActiveGPUCount() > 1)
                {
                    ttt.x = 0;
                }
                m_pShader->FXSetPSFloat(parameterName6, (Vec4*)&ttt, 1);
            }

            {
                static CCryNameR parameterName6("SVO_CamPos");
                Vec4 ttt(gEnv->pSystem->GetViewCamera().GetPosition(), 0);
                m_pShader->FXSetPSFloat(parameterName6, (Vec4*)&ttt, 1);
            }

            {
                Matrix44A mViewProj;
                mViewProj = gcpRendD3D->m_ViewProjMatrix;
                mViewProj.Transpose();

                static CCryNameR paramName("g_mViewProj");
                m_pShader->FXSetPSFloat(paramName, alias_cast<Vec4*>(&mViewProj), 4);

                static CCryNameR paramNamePrev("g_mViewProjPrev");
                m_pShader->FXSetPSFloat(paramNamePrev, alias_cast<Vec4*>(&m_matViewProjPrev), 4);
            }

            {
                float fSizeRatioW = float(rd->GetWidth() / rd->m_RTStack[0][rd->m_nRTStackLevel[0]].m_Width);
                float fSizeRatioH = float(rd->GetHeight() / rd->m_RTStack[0][rd->m_nRTStackLevel[0]].m_Height);
                static CCryNameR parameterName6("SVO_TargetResScale");
                static int nPrevWidth = 0;
                Vec4 ttt(fSizeRatioW, fSizeRatioH, gEnv->pConsole->GetCVar("e_svoTemporalFilteringBase")->GetFVal(),
                    (float)(nPrevWidth != (pTS->pRT_ALD_0->GetWidth() + 1) || (rd->m_RP.m_nRendFlags & SHDF_CUBEMAPGEN) || (rd->GetActiveGPUCount() > 1)));
                m_pShader->FXSetPSFloat(parameterName6, (Vec4*)&ttt, 1);
                nPrevWidth = (pTS->pRT_ALD_0->GetWidth() + 1);
            }

            {
                Matrix44A matView2;
                matView2 = gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_cam.GetViewMatrix();
                Vec3 zAxis2 = matView2.GetRow(1);
                matView2.SetRow(1, -matView2.GetRow(2));
                matView2.SetRow(2, zAxis2);
                float z2 = matView2.m13;
                matView2.m13 = -matView2.m23;
                matView2.m23 = z2;
                static CCryNameR paramName2("TI_CameraMatrix");
                m_pShader->FXSetPSFloat(paramName2, (Vec4*)matView2.GetData(), 3);
            }

            {
                Vec3 pvViewFrust[8];
                const CameraViewParameters& rc = gcpRendD3D->GetViewParameters();
                rc.CalcVerts(pvViewFrust);

                static CCryNameR parameterName0("SVO_FrustumVerticesCam0");
                static CCryNameR parameterName1("SVO_FrustumVerticesCam1");
                static CCryNameR parameterName2("SVO_FrustumVerticesCam2");
                static CCryNameR parameterName3("SVO_FrustumVerticesCam3");
                Vec4 ttt0(pvViewFrust[4] - rc.vOrigin, 0);
                Vec4 ttt1(pvViewFrust[5] - rc.vOrigin, 0);
                Vec4 ttt2(pvViewFrust[6] - rc.vOrigin, 0);
                Vec4 ttt3(pvViewFrust[7] - rc.vOrigin, 0);
                m_pShader->FXSetPSFloat(parameterName0, (Vec4*)&ttt0, 1);
                m_pShader->FXSetPSFloat(parameterName1, (Vec4*)&ttt1, 1);
                m_pShader->FXSetPSFloat(parameterName2, (Vec4*)&ttt2, 1);
                m_pShader->FXSetPSFloat(parameterName3, (Vec4*)&ttt3, 1);
            }


            if (pTS->pRT_ALD_1 && pTS->pRT_ALD_1)
            {
                static int nPrevWidth = 0;
                if (nPrevWidth != (pTS->pRT_ALD_1->GetWidth()))
                {
                    CTextureManager::Instance()->GetWhiteTexture()->Apply(10, m_nTexStateLinear);
                    CTextureManager::Instance()->GetWhiteTexture()->Apply(11, m_nTexStateLinear);
                    nPrevWidth = pTS->pRT_ALD_1->GetWidth();
                }
                else
                {
                    pTS->pRT_ALD_1->Apply(10, m_nTexStateLinear);
                    pTS->pRT_RGB_1->Apply(11, m_nTexStateLinear);
                }
            }
        }

        rd->FX_SetState(GS_NODEPTHTEST);

        SetupSvoTexturesForRead(m_texInfo, eHWSC_Pixel, (gEnv->pConsole->GetCVar("e_svoTI_Active")->GetIVal() ? gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->GetIVal() : 0), 0, 0);

        CTexture::s_ptexZTarget->Apply(4, m_nTexStatePoint);

        {
            GetGBuffer(1)->Apply(5, m_nTexStatePoint);

            GetGBuffer(2)->Apply(7, m_nTexStatePoint);

            GetGBuffer(0)->Apply(14, m_nTexStatePoint);
        }

        if (!GetIntegratioMode() && gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->GetFVal() && m_arrLightsDynamic.Count())
        {
            BindTiledLights(m_arrLightsDynamic, eHWSC_Pixel);
            SetupLightSources(m_arrLightsDynamic, m_pShader, true);
        }

        {
            const bool setupCloudShadows = rd->m_bShadowsEnabled && rd->m_bCloudShadowsEnabled;
            if (setupCloudShadows)
            {
                // cloud shadow map
                CTexture* pCloudShadowTex(rd->GetCloudShadowTextureId() > 0 ? CTexture::GetByID(rd->GetCloudShadowTextureId()) : CTextureManager::Instance()->GetWhiteTexture());
                assert(pCloudShadowTex);

                STexState pTexStateLinearClamp;
                pTexStateLinearClamp.SetFilterMode(FILTER_LINEAR);
                pTexStateLinearClamp.SetClampMode(false, false, false);
                int nTexStateLinearClampID = CTexture::GetTexState(pTexStateLinearClamp);

                pCloudShadowTex->Apply(15, m_nTexStateLinearWrap);
            }
            else
            {
                CTextureManager::Instance()->GetWhiteTexture()->Apply(15, m_nTexStateLinearWrap);
            }
        }

        rd->FX_Commit();

        SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexCurrentSceneDiffuseAccMap->GetWidth(), CTexture::s_ptexCurrentSceneDiffuseAccMap->GetHeight());

        gcpRendD3D->GetTiledShading().UnbindForwardShadingResources(eHWSC_Pixel);

        SD3DPostEffectsUtils::ShEndPass();
    }

    rd->FX_PopRenderTarget(0);
    rd->FX_PopRenderTarget(1);
}

void CSvoRenderer::DrawPonts(PodArray<SVF_P3F_C4B_T2F>& arrVerts)
{
    SPostEffectsUtils::UpdateFrustumCorners();

    CVertexBuffer strip(arrVerts.GetElements(), eVF_P3F_C4B_T2F);

    gRenDev->DrawPrimitivesInternal(&strip, arrVerts.Count(), eptPointList);
}

void CSvoRenderer::UpdateRender()
{
    int nPrevAsync = CRenderer::CV_r_shadersasynccompiling;
    if (gEnv->IsEditor())
    {
        CRenderer::CV_r_shadersasynccompiling = 0;
    }

    {
        PROFILE_LABEL_SCOPE("TI_GEN_DIFF");

        ConeTracePass(&m_tsDiff);
    }
    if (GetIntegratioMode() == 2)
    {
        PROFILE_LABEL_SCOPE("TI_GEN_SPEC");

        ConeTracePass(&m_tsSpec);
    }

    {
        PROFILE_LABEL_SCOPE("TI_DEMOSAIC_DIFF");

        DemosaicPass(&m_tsDiff);
    }
    if (GetIntegratioMode() == 2)
    {
        PROFILE_LABEL_SCOPE("TI_DEMOSAIC_SPEC");

        DemosaicPass(&m_tsSpec);
    }

    {
        PROFILE_LABEL_SCOPE("TI_UPSCALE_DIFF");

        UpScalePass(&m_tsDiff);
    }
    if (GetIntegratioMode() == 2)
    {
        PROFILE_LABEL_SCOPE("TI_UPSCALE_SPEC");

        UpScalePass(&m_tsSpec);
    }

    {
        m_matViewProjPrev = gcpRendD3D->m_ViewProjMatrix;
        m_matViewProjPrev.Transpose();
    }

    if (gEnv->IsEditor())
    {
        CRenderer::CV_r_shadersasynccompiling = nPrevAsync;
    }
}

void CSvoRenderer::DemosaicPass(SSvoTargetsSet* pTS)
{
    const char* szTechFinalName = "DemosaicPass";

    if (!IsActive() || !m_pShader)
    {
        return;
    }

    if (m_texInfo.bSvoFreeze)
    {
        return;
    }

    { // SVO
        if (!pTS->pRT_ALD_0 || !pTS->pRT_ALD_0)
        {
            return;
        }

        CD3D9Renderer* const __restrict rd = gcpRendD3D;

        rd->FX_PushRenderTarget(0, pTS->pRT_RGB_DEM_MIN_0, NULL, -1, false, 1);
        rd->FX_PushRenderTarget(1, pTS->pRT_ALD_DEM_MIN_0, NULL, -1, false, 1);
        rd->FX_PushRenderTarget(2, pTS->pRT_RGB_DEM_MAX_0, NULL, -1, false, 1);
        rd->FX_PushRenderTarget(3, pTS->pRT_ALD_DEM_MAX_0, NULL, -1, false, 1);

        if (m_texInfo.pTexTree)
        {
            SetShaderFlags(pTS == &m_tsDiff);

            SD3DPostEffectsUtils::ShBeginPass(m_pShader, szTechFinalName, FEF_DONTSETTEXTURES);

            rd->FX_SetState(GS_NODEPTHTEST);

            CTexture::s_ptexZTarget->Apply(4, m_nTexStatePoint);

            GetGBuffer(1)->Apply(5, m_nTexStatePoint);

            GetGBuffer(2)->Apply(7, m_nTexStatePoint);

            GetGBuffer(0)->Apply(14, m_nTexStatePoint);

            {
                static CCryNameR parameterName5("SVO_ReprojectionMatrix");
                m_pShader->FXSetPSFloat(parameterName5, (Vec4*)m_matReproj.GetData(), 3);
            }

            {
                float fSizeRatioW = float(rd->GetWidth() / rd->m_RTStack[0][rd->m_nRTStackLevel[0]].m_Width);
                float fSizeRatioH = float(rd->GetHeight() / rd->m_RTStack[0][rd->m_nRTStackLevel[0]].m_Height);
                static CCryNameR parameterName6("SVO_TargetResScale");
                static int nPrevWidth = 0;
                Vec4 ttt(fSizeRatioW, fSizeRatioH, gEnv->pConsole->GetCVar("e_svoTemporalFilteringBase")->GetFVal(),
                    (float)(nPrevWidth != (pTS->pRT_ALD_0->GetWidth() + 1) || (rd->m_RP.m_nRendFlags & SHDF_CUBEMAPGEN) || (rd->GetActiveGPUCount() > 1)));
                m_pShader->FXSetPSFloat(parameterName6, (Vec4*)&ttt, 1);
                nPrevWidth = (pTS->pRT_ALD_0->GetWidth() + 1);
            }

            {
                static CCryNameR parameterName6("SVO_FrameIdByte");
                Vec4 ttt((float)(rd->GetFrameID(false) & 255), (float)rd->GetFrameID(false), 3, 4);
                if (rd->GetActiveGPUCount() > 1)
                {
                    ttt.x = 0;
                }
                m_pShader->FXSetPSFloat(parameterName6, (Vec4*)&ttt, 1);
            }

            {
                static CCryNameR parameterName6("SVO_CamPos");
                Vec4 ttt(gEnv->pSystem->GetViewCamera().GetPosition(), 0);
                m_pShader->FXSetPSFloat(parameterName6, (Vec4*)&ttt, 1);
            }

            {
                Matrix44A mViewProj;
                mViewProj = gcpRendD3D->m_ViewProjMatrix;
                mViewProj.Transpose();

                static CCryNameR paramName("g_mViewProj");
                m_pShader->FXSetPSFloat(paramName, alias_cast<Vec4*>(&mViewProj), 4);

                static CCryNameR paramNamePrev("g_mViewProjPrev");
                m_pShader->FXSetPSFloat(paramNamePrev, alias_cast<Vec4*>(&m_matViewProjPrev), 4);
            }

            {
                Matrix44A matView;
                matView = gcpRendD3D->m_RP.m_TI[gcpRendD3D->m_RP.m_nProcessThreadID].m_cam.GetViewMatrix();
                Vec3 zAxis = matView.GetRow(1);
                matView.SetRow(1, -matView.GetRow(2));
                matView.SetRow(2, zAxis);
                float z = matView.m13;
                matView.m13 = -matView.m23;
                matView.m23 = z;
                static CCryNameR paramName2("TI_CameraMatrix");
                m_pShader->FXSetPSFloat(paramName2, (Vec4*)matView.GetData(), 3);
            }

            pTS->pRT_ALD_0->Apply(10, m_nTexStateLinear);
            pTS->pRT_RGB_0->Apply(11, m_nTexStateLinear);

            pTS->pRT_RGB_DEM_MIN_1->Apply(0, m_nTexStateLinear);
            pTS->pRT_ALD_DEM_MIN_1->Apply(1, m_nTexStateLinear);
            pTS->pRT_RGB_DEM_MAX_1->Apply(2, m_nTexStateLinear);
            pTS->pRT_ALD_DEM_MAX_1->Apply(3, m_nTexStateLinear);


            SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexCurrentSceneDiffuseAccMap->GetWidth(), CTexture::s_ptexCurrentSceneDiffuseAccMap->GetHeight());

            SD3DPostEffectsUtils::ShEndPass();
        }

        rd->FX_PopRenderTarget(0);
        rd->FX_PopRenderTarget(1);
        rd->FX_PopRenderTarget(2);
        rd->FX_PopRenderTarget(3);
    }
}

void CSvoRenderer::SetupLightSources(PodArray<I3DEngine::SLightTI>& lightsTI, CShader* pShader, bool bPS)
{
    const int nLightGroupsNum = 2;

    static CCryNameR paramNamesLightPos[nLightGroupsNum] =
    {
        CCryNameR("SVO_LightPos0"),
        CCryNameR("SVO_LightPos1"),
    };
    static CCryNameR paramNamesLightDir[nLightGroupsNum] =
    {
        CCryNameR("SVO_LightDir0"),
        CCryNameR("SVO_LightDir1"),
    };
    static CCryNameR paramNamesLightCol[nLightGroupsNum] =
    {
        CCryNameR("SVO_LightCol0"),
        CCryNameR("SVO_LightCol1"),
    };

    for (int g = 0; g < nLightGroupsNum; g++)
    {
        Vec4 LightPos[4];
        Vec4 LightDir[4];
        Vec4 LightCol[4];

        for (int x = 0; x < 4; x++)
        {
            int nId = g * 4 + x;

            LightPos[x] = (nId < lightsTI.Count()) ? lightsTI[nId].vPosR : Vec4(0, 0, 0, 0);
            LightDir[x] = (nId < lightsTI.Count()) ? lightsTI[nId].vDirF : Vec4(0, 0, 0, 0);
            LightCol[x] = (nId < lightsTI.Count()) ? lightsTI[nId].vCol : Vec4(0, 0, 0, 0);
        }

        if (bPS)
        {
            pShader->FXSetPSFloat(paramNamesLightPos[g], alias_cast<Vec4*>(&LightPos[0][0]), 4);
            pShader->FXSetPSFloat(paramNamesLightDir[g], alias_cast<Vec4*>(&LightDir[0][0]), 4);
            pShader->FXSetPSFloat(paramNamesLightCol[g], alias_cast<Vec4*>(&LightCol[0][0]), 4);
        }
        else
        { // CS
            pShader->FXSetCSFloat(paramNamesLightPos[g], alias_cast<Vec4*>(&LightPos[0][0]), 4);
            pShader->FXSetCSFloat(paramNamesLightDir[g], alias_cast<Vec4*>(&LightDir[0][0]), 4);
            pShader->FXSetCSFloat(paramNamesLightCol[g], alias_cast<Vec4*>(&LightCol[0][0]), 4);
        }
    }
}

void CSvoRenderer::SetupNodesForUpdate(int& nNodesForUpdateStartIndex, PodArray<I3DEngine::SSvoNodeInfo>& arrNodesForUpdate)
{
    static CCryNameR paramNames[SVO_MAX_NODE_GROUPS] =
    {
        CCryNameR("SVO_NodesForUpdate0"),
        CCryNameR("SVO_NodesForUpdate1"),
        CCryNameR("SVO_NodesForUpdate2"),
        CCryNameR("SVO_NodesForUpdate3"),
    };

    for (int g = 0; g < SVO_MAX_NODE_GROUPS; g++)
    {
        float matVal[4][4];

        for (int x = 0; x < 4; x++)
        {
            for (int y = 0; y < 4; y++)
            {
                int nId = nNodesForUpdateStartIndex + g * 16 + x * 4 + y;
                matVal[x][y] = 0.1f + ((nId < arrNodesForUpdate.Count()) ? arrNodesForUpdate[nId].nAtlasOffset : -2);

                if (nId < arrNodesForUpdate.Count())
                {
                    float fNodeSize = arrNodesForUpdate[nId].wsBox.GetSize().x;
                }
            }
        }

        m_pShader->FXSetCSFloat(paramNames[g], alias_cast<Vec4*>(&matVal[0][0]), 4);
    }

    nNodesForUpdateStartIndex += 4 * 4 * SVO_MAX_NODE_GROUPS; // 128
}

void CSvoRenderer::SetupSvoTexturesForRead(I3DEngine::SSvoStaticTexInfo& texInfo, EHWShaderClass eShaderClass, int nStage, int nStageOpa, int nStageNorm)
{
    ((CTexture*)texInfo.pTexTree)->Apply(0, m_nTexStatePoint, -1, -1, -1, eShaderClass);

    CTextureManager::Instance()->GetBlackTexture()->Apply(1, m_nTexStateLinear, -1, -1, -1, eShaderClass);


    if (nStage == 0)
    {
        CTexture::GetByID(vp_RGB0.nTexId)->Apply(1, m_nTexStateLinear, -1, -1, -1, eShaderClass);
    }
    else if (nStage == 1)
    {
        CTexture::GetByID(vp_RGB1.nTexId)->Apply(1, m_nTexStateLinear, -1, -1, -1, eShaderClass);
    }
    else if (nStage == 2)
    {
        CTexture::GetByID(vp_RGB2.nTexId)->Apply(1, m_nTexStateLinear, -1, -1, -1, eShaderClass);
    }
    else if (nStage == 3)
    {
        CTexture::GetByID(vp_RGB3.nTexId)->Apply(1, m_nTexStateLinear, -1, -1, -1, eShaderClass);
    }

    if (nStageNorm == 0)
    {
        CTexture::GetByID(vp_NORM.nTexId)->Apply(2, m_nTexStateLinear, -1, -1, -1, eShaderClass);
    }


    if (nStageOpa == 0)
    {
        ((CTexture*)texInfo.pTexOpac)->Apply(3, m_nTexStateLinear, -1, -1, -1, eShaderClass);
    }


    if (texInfo.pGlobalSpecCM)
    {
        ((CTexture*)texInfo.pGlobalSpecCM)->Apply(6, m_nTexStateTrilinear, -1, -1, -1, eShaderClass);
    }

}

void CSvoRenderer::CheckAllocateRT(bool bSpecPass)
{
    int nWidth = gcpRendD3D->GetWidth();
    int nHeight = gcpRendD3D->GetHeight();

    int nResScaleBase = 2;
    int nInW = (nWidth / nResScaleBase);
    int nInH = (nHeight / nResScaleBase);

    if (!bSpecPass)
    {
        CheckCreateUpdateRT(m_tsDiff.pRT_ALD_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_DIFF_ALD");
        CheckCreateUpdateRT(m_tsDiff.pRT_ALD_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_DIFF_ALD");
        CheckCreateUpdateRT(m_tsDiff.pRT_RGB_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_DIFF_RGB");
        CheckCreateUpdateRT(m_tsDiff.pRT_RGB_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_DIFF_RGB");

        CheckCreateUpdateRT(m_tsSpec.pRT_ALD_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_SPEC_ALD");
        CheckCreateUpdateRT(m_tsSpec.pRT_ALD_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_SPEC_ALD");
        CheckCreateUpdateRT(m_tsSpec.pRT_RGB_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_SPEC_RGB");
        CheckCreateUpdateRT(m_tsSpec.pRT_RGB_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_SPEC_RGB");


        CheckCreateUpdateRT(m_tsDiff.pRT_RGB_DEM_MIN_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_DIFF_FIN_RGB_MIN");
        CheckCreateUpdateRT(m_tsDiff.pRT_ALD_DEM_MIN_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_DIFF_FIN_ALD_MIN");
        CheckCreateUpdateRT(m_tsDiff.pRT_RGB_DEM_MAX_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_DIFF_FIN_RGB_MAX");
        CheckCreateUpdateRT(m_tsDiff.pRT_ALD_DEM_MAX_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_DIFF_FIN_ALD_MAX");
        CheckCreateUpdateRT(m_tsDiff.pRT_RGB_DEM_MIN_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_DIFF_FIN_RGB_MIN");
        CheckCreateUpdateRT(m_tsDiff.pRT_ALD_DEM_MIN_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_DIFF_FIN_ALD_MIN");
        CheckCreateUpdateRT(m_tsDiff.pRT_RGB_DEM_MAX_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_DIFF_FIN_RGB_MAX");
        CheckCreateUpdateRT(m_tsDiff.pRT_ALD_DEM_MAX_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_DIFF_FIN_ALD_MAX");

        CheckCreateUpdateRT(m_tsDiff.pRT_FIN_OUT_0, nWidth, nHeight, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_FIN_DIFF_OUT");
        CheckCreateUpdateRT(m_tsDiff.pRT_FIN_OUT_1, nWidth, nHeight, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_FIN_DIFF_OUT");

        CheckCreateUpdateRT(m_tsSpec.pRT_RGB_DEM_MIN_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_SPEC_FIN_RGB_MIN");
        CheckCreateUpdateRT(m_tsSpec.pRT_ALD_DEM_MIN_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_SPEC_FIN_ALD_MIN");
        CheckCreateUpdateRT(m_tsSpec.pRT_RGB_DEM_MAX_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_SPEC_FIN_RGB_MAX");
        CheckCreateUpdateRT(m_tsSpec.pRT_ALD_DEM_MAX_0, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_SPEC_FIN_ALD_MAX");
        CheckCreateUpdateRT(m_tsSpec.pRT_RGB_DEM_MIN_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_SPEC_FIN_RGB_MIN");
        CheckCreateUpdateRT(m_tsSpec.pRT_ALD_DEM_MIN_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_SPEC_FIN_ALD_MIN");
        CheckCreateUpdateRT(m_tsSpec.pRT_RGB_DEM_MAX_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_SPEC_FIN_RGB_MAX");
        CheckCreateUpdateRT(m_tsSpec.pRT_ALD_DEM_MAX_1, nInW, nInH, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_SPEC_FIN_ALD_MAX");

        CheckCreateUpdateRT(m_tsSpec.pRT_FIN_OUT_0, nWidth, nHeight, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SVO_FIN_SPEC_OUT");
        CheckCreateUpdateRT(m_tsSpec.pRT_FIN_OUT_1, nWidth, nHeight, eTF_R16G16B16A16F, eTT_2D, FT_STATE_CLAMP, "SV1_FIN_SPEC_OUT");

        // swap ping-pong RT
        std::swap(m_tsDiff.pRT_ALD_0, m_tsDiff.pRT_ALD_1);
        std::swap(m_tsDiff.pRT_RGB_0, m_tsDiff.pRT_RGB_1);
        std::swap(m_tsDiff.pRT_RGB_DEM_MIN_0, m_tsDiff.pRT_RGB_DEM_MIN_1);
        std::swap(m_tsDiff.pRT_ALD_DEM_MIN_0, m_tsDiff.pRT_ALD_DEM_MIN_1);
        std::swap(m_tsDiff.pRT_RGB_DEM_MAX_0, m_tsDiff.pRT_RGB_DEM_MAX_1);
        std::swap(m_tsDiff.pRT_ALD_DEM_MAX_0, m_tsDiff.pRT_ALD_DEM_MAX_1);
        std::swap(m_tsDiff.pRT_FIN_OUT_0, m_tsDiff.pRT_FIN_OUT_1);

        std::swap(m_tsSpec.pRT_ALD_0, m_tsSpec.pRT_ALD_1);
        std::swap(m_tsSpec.pRT_RGB_0, m_tsSpec.pRT_RGB_1);
        std::swap(m_tsSpec.pRT_RGB_DEM_MIN_0, m_tsSpec.pRT_RGB_DEM_MIN_1);
        std::swap(m_tsSpec.pRT_ALD_DEM_MIN_0, m_tsSpec.pRT_ALD_DEM_MIN_1);
        std::swap(m_tsSpec.pRT_RGB_DEM_MAX_0, m_tsSpec.pRT_RGB_DEM_MAX_1);
        std::swap(m_tsSpec.pRT_ALD_DEM_MAX_0, m_tsSpec.pRT_ALD_DEM_MAX_1);
        std::swap(m_tsSpec.pRT_FIN_OUT_0, m_tsSpec.pRT_FIN_OUT_1);
    }
}

bool CSvoRenderer::IsShaderItemUsedForVoxelization(SShaderItem& rShaderItem, [[maybe_unused]] IRenderNode* pRN)
{
    CShader* pS = (CShader*)rShaderItem.m_pShader;
    CShaderResources* pR = (CShaderResources*)rShaderItem.m_pShaderResources;

    // skip some objects marked by level designer
    //  if(pRN && pRN->IsRenderNode() && pRN->GetIntegrationType())
    //  return false;

    // skip transparent meshes except decals
    //  if((pR->Opacity() != 1.f) && !(pS->GetFlags()&EF_DECAL))
    //  return false;

    // skip windows
    if (pS && pS->GetShaderType() == eST_Glass)
    {
        return false;
    }

    // skip water
    if (pS && pS->GetShaderType() == eST_Water)
    {
        return false;
    }

    return true;
}

namespace
{
    const float ACTIVATION_MODE_DISABLED = -1.0f;

    float GetActivationModeAsFloat()
    {
        auto* instance = CSvoRenderer::GetInstance();

        float modeAsFloat = ACTIVATION_MODE_DISABLED;
        int32 modeAsInt = instance->GetIntegratioMode();

        if (instance->IsActive())
        {
            if (modeAsInt == 0 && gEnv->pConsole->GetCVar("e_svoTI_UseLightProbes")->GetIVal())
            { // AO modulates diffuse and specular
                modeAsFloat = 0.0f;
            }
            else if (modeAsInt <= 1)
            { // GI replaces diffuse and modulates specular
                modeAsFloat = 1.0f;
            }
            else if (modeAsInt == 2)
            { // GI replaces diffuse and specular
                modeAsFloat = 2.0f;
            }
        }

        return modeAsFloat;
    }
}

void CSvoRenderer::UpdatePassConstantBuffer()
{
    CSvoRenderer* svoRenderer = CSvoRenderer::GetInstance();

    CTypedConstantBuffer<HLSL_PerPassConstantBuffer_Svo> cb(m_PassConstantBuffer);

    // SvoParams4 is used by forward tiled shaders even if GI is disabled, it contains info about GI mode and also is GI active or not
    cb->PerPass_SvoParams4 = Vec4(0.0f, -1.0f, 0.0f, 0.0f);

    if (svoRenderer)
    {
        AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
        AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
        const float terrainSizeX = terrainAabb.GetXExtent();

        cb->PerPass_SvoTreeSettings0 = Vec4(
            (float)terrainSizeX,
            gEnv->pConsole->GetCVar("e_svoMinNodeSize")->GetFVal(),
            (float)svoRenderer->m_texInfo.nBrickSize,
            128.f);

        cb->PerPass_SvoTreeSettings1 = Vec4(
            gEnv->pConsole->GetCVar("e_svoMaxNodeSize")->GetFVal(),
            0.0f,
            0.0f,
            0.0f);
        cb->PerPass_SvoParams0 = Vec4(
            1.0f,
            0.0f,
            0.0f,
            0.0f);

        cb->PerPass_SvoParams1 = Vec4(
            1.f / (gEnv->pConsole->GetCVar("e_svoTI_DiffuseConeWidth")->GetFVal() + 0.00001f),
            gEnv->pConsole->GetCVar("e_svoTI_ConeMaxLength")->GetFVal(),
            0.0f,
            (svoRenderer->m_texInfo.bSvoReady && gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->GetIVal()) ? 1.0f : 0.0f);

        cb->PerPass_SvoParams2 = Vec4(
            gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->GetFVal(),
            0.0f,
            gEnv->pConsole->GetCVar("e_svoTI_Saturation")->GetFVal(),
            0.0f);

        cb->PerPass_SvoParams3 = Vec4(
            1.0f,
            0.0f,
            0.0f,
            (svoRenderer->m_texInfo.bSvoReady && gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->GetIVal()) ? 1.0f : 0.0f);


        {
            cb->PerPass_SvoParams4.x = gEnv->pConsole->GetCVar("e_svoTI_AmbientOffsetRed")->GetFVal();
            cb->PerPass_SvoParams4.y = gEnv->pConsole->GetCVar("e_svoTI_AmbientOffsetGreen")->GetFVal();
            cb->PerPass_SvoParams4.z = gEnv->pConsole->GetCVar("e_svoTI_AmbientOffsetBlue")->GetFVal();
            cb->PerPass_SvoParams4.w = gEnv->pConsole->GetCVar("e_svoTI_AmbientOffsetBias")->GetFVal();
        }

        cb->PerPass_SvoParams5 = Vec4(
            0.0f,
            0.0f,
            gEnv->pConsole->GetCVar("e_svoTI_NumberOfBounces")->GetFVal() - 1.f,
            svoRenderer->m_texInfo.fGlobalSpecCM_Mult);

        cb->PerPass_SvoParams6 = Vec4(
            1.0f,
            gEnv->IsEditing() ? 0 : (gEnv->pConsole->GetCVar("e_svoTemporalFilteringMinDistance")->GetFVal() / gcpRendD3D->GetViewParameters().fFar),
            0.0f,
            0.0f);
    }

    m_PassConstantBuffer = cb.GetDeviceConstantBuffer();
    cb.CopyToDevice();

    auto& devManager = gcpRendD3D->m_DevMan;
    devManager.BindConstantBuffer(eHWSC_Compute, m_PassConstantBuffer.get(), eConstantBufferShaderSlot_PerPass);
    devManager.BindConstantBuffer(eHWSC_Pixel, m_PassConstantBuffer.get(), eConstantBufferShaderSlot_PerPass);
}

Vec4 CSvoRenderer::GetDisabledPerFrameShaderParameters()
{
    Vec4 params;
    params.x = 0.0f;
    params.y = ACTIVATION_MODE_DISABLED;
    params.z = 0.0f;
    params.z = 0.0f;
    return params;
}

Vec4 CSvoRenderer::GetPerFrameShaderParameters() const
{
    Vec4 params;
    params.x = 1.f;
    params.y = GetActivationModeAsFloat();
    params.z = 0.0f;
    params.w = 0.0f;
    return params;
}

void CSvoRenderer::DebugDrawStats(const RPProfilerStats* pBasicStats, float& ypos, const float ystep, float xposms)
{
    ColorF color = Col_Yellow;
    const EDrawTextFlags  txtFlags = (EDrawTextFlags)(eDrawText_2D | eDrawText_800x600 | eDrawText_FixedSize | eDrawText_Monospace);

#define SVO_Draw2dLabel(labelName)                                                                                        \
    gRenDev->Draw2dLabel(60, ypos += ystep, 2, &color.r, false, (const char*)(((const char*)(#labelName)) + 10));         \
    if (pBasicStats[labelName].gpuTimeMax > 0.01) {                                                                       \
        gRenDev->Draw2dLabelEx(xposms, ypos, 2, color, txtFlags, "%5.2f Aver=%5.2f Max=%5.2f",                            \
            pBasicStats[labelName].gpuTime, pBasicStats[labelName].gpuTimeSmoothed, pBasicStats[labelName].gpuTimeMax); } \
    else{                                                                                                                 \
        gRenDev->Draw2dLabelEx(xposms, ypos, 2, color, txtFlags, "%5.2f", pBasicStats[labelName].gpuTime); }              \

    SVO_Draw2dLabel(eRPPSTATS_TI_INJECT_CLEAR);
    SVO_Draw2dLabel(eRPPSTATS_TI_VOXELIZE);
    SVO_Draw2dLabel(eRPPSTATS_TI_INJECT_AIR);
    SVO_Draw2dLabel(eRPPSTATS_TI_INJECT_LIGHT);
    SVO_Draw2dLabel(eRPPSTATS_TI_INJECT_REFL0);
    SVO_Draw2dLabel(eRPPSTATS_TI_INJECT_REFL1);
    SVO_Draw2dLabel(eRPPSTATS_TI_INJECT_DYNL);
    SVO_Draw2dLabel(eRPPSTATS_TI_NID_DIFF);
    SVO_Draw2dLabel(eRPPSTATS_TI_GEN_DIFF);
    SVO_Draw2dLabel(eRPPSTATS_TI_GEN_SPEC);
    SVO_Draw2dLabel(eRPPSTATS_TI_GEN_AIR);
    SVO_Draw2dLabel(eRPPSTATS_TI_DEMOSAIC_DIFF);
    SVO_Draw2dLabel(eRPPSTATS_TI_DEMOSAIC_SPEC);
    SVO_Draw2dLabel(eRPPSTATS_TI_UPSCALE_DIFF);
    SVO_Draw2dLabel(eRPPSTATS_TI_UPSCALE_SPEC);
}

void CSvoRenderer::SetShaderFlags(bool bDiffuseMode, bool bPixelShader)
{
    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE0];
    }

    if (m_texInfo.pGlobalSpecCM && GetIntegratioMode()) // use global env CM
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE1];
    }
    else
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE1];

    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE2];
    }

    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE3];
    }

    if (bDiffuseMode) // diffuse or specular rendering
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE4];
    }
    else
    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE4];
    }

    if (GetIntegratioMode()) // ignore colors and normals for AO only mode
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE5];
    }
    else
    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_SAMPLE5];
    }

    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_HW_PCF_COMPARE];
    }

    if (bPixelShader && !GetIntegratioMode() && gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->GetFVal()) // read sun light and shadow map during final cone tracing
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ];
    }
    else
    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ];
    }

    if (bPixelShader && !GetIntegratioMode() && gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->GetFVal() && m_arrLightsDynamic.Count()) // read sun light and shadow map during final cone tracing
    {
        gRenDev->m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_POINT_LIGHT];
    }
    else
    {
        gRenDev->m_RP.m_FlagsShader_RT &= ~g_HWSR_MaskBit[HWSR_POINT_LIGHT];
    }
}

int CSvoRenderer::GetIntegratioMode()
{
    return gEnv->pConsole->GetCVar("e_svoTI_IntegrationMode")->GetIVal();
}

bool CSvoRenderer::IsActive()
{
    static ICVar* e_GI = gEnv->pConsole->GetCVar("e_GI");
    static ICVar* e_svoTI_Active = gEnv->pConsole->GetCVar("e_svoTi_Active");

    //Unable to find cvars, clearly can't be active.
    if (!e_GI || !e_svoTI_Active)
    {
        return false;
    }

    return e_GI->GetIVal() && CSvoRenderer::s_pInstance && e_svoTI_Active->GetIVal();
}

bool CSvoRenderer::SetSamplers(int nCustomID, EHWShaderClass eSHClass, int nTUnit, int nTState, int nTexMaterialSlot, int nSUnit)
{
    CSvoRenderer* pSR = CSvoRenderer::GetInstance();

    if (!pSR)
    {
        return false;
    }

    switch (nCustomID)
    {
    case TO_SVOTREE:
    case TO_SVOTRIS:
    case TO_SVOGLCM:
    case TO_SVORGBS:
    case TO_SVONORM:
    case TO_SVOOPAC:
    {
        CTexture* pTex = CTextureManager::Instance()->GetBlackTexture();

        if (pSR->m_texInfo.pTexTree)
        {
            if (nCustomID == TO_SVOTREE)
            {
                nCustomID = pSR->m_texInfo.pTexTree->GetTextureID();
            }
            if (nCustomID == TO_SVORGBS)
            {
                if (pSR->m_texInfo.pTexRgb0)
                {
                    nCustomID = pSR->m_texInfo.pTexRgb0->GetTextureID();
                }
            }
            if (nCustomID == TO_SVONORM)
            {
                nCustomID = pSR->m_texInfo.pTexNorm->GetTextureID();
            }
            if (nCustomID == TO_SVOOPAC)
            {
                nCustomID = pSR->m_texInfo.pTexOpac->GetTextureID();
            }

            if (nCustomID > 0)
            {
                pTex = CTexture::GetByID(nCustomID);
            }
        }

        pTex->Apply(nTUnit, nTState, nTexMaterialSlot, nSUnit, -1, eSHClass);

        return true;
    }
    }

    return false;
}

CTexture* CSvoRenderer::GetDiffuseFinRT()
{
    return m_tsDiff.pRT_FIN_OUT_0;
}

CTexture* CSvoRenderer::GetSpecularFinRT()
{
    return m_tsSpec.pRT_FIN_OUT_0;
}

void CSvoRenderer::UpScalePass(SSvoTargetsSet* pTS)
{

    const char* szTechFinalName = "UpScalePass";

    if (!IsActive() || !m_pShader)
    {
        return;
    }

    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    rd->FX_PushRenderTarget(0, pTS->pRT_FIN_OUT_0, NULL, -1, false, 1);

    SetShaderFlags(pTS == &m_tsDiff);

    SD3DPostEffectsUtils::ShBeginPass(m_pShader, szTechFinalName, FEF_DONTSETTEXTURES);

    if (!gRenDev->m_RP.m_pShader)
    {
        gEnv->pLog->LogWarning("Error: %s: Technique not found: %s", __FUNC__, szTechFinalName);
        rd->FX_PopRenderTarget(0);
        return;
    }

    CTexture::s_ptexZTarget->Apply(4, m_nTexStatePoint);
    GetGBuffer(1)->Apply(5, m_nTexStatePoint);
    GetGBuffer(2)->Apply(7, m_nTexStatePoint);
    GetGBuffer(0)->Apply(14, m_nTexStatePoint);

    pTS->pRT_ALD_DEM_MIN_0->Apply(10, m_nTexStatePoint);
    pTS->pRT_RGB_DEM_MIN_0->Apply(11, m_nTexStatePoint);
    pTS->pRT_ALD_DEM_MAX_0->Apply(12, m_nTexStatePoint);
    pTS->pRT_RGB_DEM_MAX_0->Apply(13, m_nTexStatePoint);

    pTS->pRT_FIN_OUT_1->Apply(9, m_nTexStatePoint);

    if (pTS == &m_tsSpec && m_tsDiff.pRT_FIN_OUT_0)
    {
        m_tsDiff.pRT_FIN_OUT_0->Apply(15, m_nTexStatePoint);
    }
    else
    {
        CTextureManager::Instance()->GetBlackTexture()->Apply(15, m_nTexStatePoint);
    }

    {
        static CCryNameR parameterName6("SVO_SrcPixSize");
        Vec4 ttt(0, 0, 0, 0);
        ttt.x = 1.f / float(pTS->pRT_ALD_DEM_MIN_0->GetWidth());
        ttt.y = 1.f / float(pTS->pRT_ALD_DEM_MIN_0->GetHeight());
        m_pShader->FXSetPSFloat(parameterName6, (Vec4*)&ttt, 1);
    }

    {
        static CCryNameR parameterName5("SVO_ReprojectionMatrix");
        m_pShader->FXSetPSFloat(parameterName5, (Vec4*)m_matReproj.GetData(), 3);
    }

    {
        float fSizeRatioW = float(rd->GetWidth() / rd->m_RTStack[0][rd->m_nRTStackLevel[0]].m_Width);
        float fSizeRatioH = float(rd->GetHeight() / rd->m_RTStack[0][rd->m_nRTStackLevel[0]].m_Height);
        static CCryNameR parameterName6("SVO_TargetResScale");
        static int nPrevWidth = 0;
        Vec4 ttt(fSizeRatioW, fSizeRatioH, gEnv->pConsole->GetCVar("e_svoTemporalFilteringBase")->GetFVal(),
            (float)(nPrevWidth != (pTS->pRT_ALD_0->GetWidth() + 1) || (rd->m_RP.m_nRendFlags & SHDF_CUBEMAPGEN) || (rd->GetActiveGPUCount() > 1)));
        m_pShader->FXSetPSFloat(parameterName6, (Vec4*)&ttt, 1);
        nPrevWidth = (pTS->pRT_ALD_0->GetWidth() + 1);
    }

    {
        Matrix44A mViewProj;
        mViewProj = gcpRendD3D->m_ViewProjMatrix;
        mViewProj.Transpose();

        static CCryNameR paramName("g_mViewProj");
        m_pShader->FXSetPSFloat(paramName, alias_cast<Vec4*>(&mViewProj), 4);

        static CCryNameR paramNamePrev("g_mViewProjPrev");
        m_pShader->FXSetPSFloat(paramNamePrev, alias_cast<Vec4*>(&m_matViewProjPrev), 4);
    }

    SD3DPostEffectsUtils::DrawFullScreenTriWPOS(CTexture::s_ptexCurrentSceneDiffuseAccMap->GetWidth(), CTexture::s_ptexCurrentSceneDiffuseAccMap->GetHeight());

    SD3DPostEffectsUtils::ShEndPass();

    rd->FX_PopRenderTarget(0);
}

void CSvoRenderer::SetupRsmSun(const EHWShaderClass eShClass)
{
    
    CD3D9Renderer* const __restrict rd = gcpRendD3D;

    int nLightID = 0;

    threadID m_nThreadID = gcpRendD3D->m_RP.m_nProcessThreadID;
    int m_nRecurseLevel = SRendItem::m_RecurseLevel[m_nThreadID];

    int nDLights = rd->m_RP.m_DLights[m_nThreadID][m_nRecurseLevel].Num();
    int nFrustumIdx = nLightID;// + nDLights;

    int nStartIdx = SRendItem::m_StartFrust[m_nThreadID][nFrustumIdx];
    int nEndIdx = SRendItem::m_EndFrust[m_nThreadID][nFrustumIdx];

    static CCryNameR lightProjParamName("SVO_RsmSunShadowProj");
    static CCryNameR rsmSunColParameterName("SVO_RsmSunCol");
    static CCryNameR rsmSunDirParameterName("SVO_RsmSunDir");
    Matrix44A shadowMat;
    shadowMat.SetIdentity();

    int nFrIdx = 0;
    for (nFrIdx = nStartIdx; nFrIdx < nEndIdx; nFrIdx++)
    {
        ShadowMapFrustum& firstFrustum = rd->m_RP.m_SMFrustums[m_nThreadID][m_nRecurseLevel][nFrIdx];
        if (firstFrustum.nShadowMapLod == 2)
        {
            break;
        }
    }

    if ((nEndIdx > nFrIdx) && GetRsmColorMap(rd->m_RP.m_SMFrustums[m_nThreadID][m_nRecurseLevel][nFrIdx]))
    {
        ShadowMapFrustum& firstFrustum = rd->m_RP.m_SMFrustums[m_nThreadID][m_nRecurseLevel][nFrIdx];
        rd->ConfigShadowTexgen(0, &firstFrustum, 0);

        if (firstFrustum.bUseShadowsPool)
        {
            STexState TS;
            TS.SetFilterMode(FILTER_POINT);
            TS.SetClampMode(TADDR_CLAMP, TADDR_CLAMP, TADDR_CLAMP);
            TS.m_bSRGBLookup = false;
            CTexture::s_ptexRT_ShadowPool->Apply(12, CTexture::GetTexState(TS), EFTT_UNKNOWN, -1, -1, eShClass);
        }
        else
        {
            firstFrustum.pDepthTex->Apply(12, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1, eShClass);
            GetRsmColorMap(firstFrustum)->Apply(13, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1, eShClass);
            GetRsmNormlMap(firstFrustum)->Apply(9, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1, eShClass);
        }

        // set up shadow matrix
        shadowMat = gRenDev->m_TempMatrices[0][0];
        const Vec4 vEye(gRenDev->GetViewParameters().vOrigin, 0.f);
        Vec4 vecTranslation(vEye.Dot((Vec4&)shadowMat.m00), vEye.Dot((Vec4&)shadowMat.m10), vEye.Dot((Vec4&)shadowMat.m20), vEye.Dot((Vec4&)shadowMat.m30));
        shadowMat.m03 += vecTranslation.x;
        shadowMat.m13 += vecTranslation.y;
        shadowMat.m23 += vecTranslation.z;
        shadowMat.m33 += vecTranslation.w;
        (Vec4&)shadowMat.m20 *= gRenDev->m_cEF.m_TempVecs[2].x;
        SetShaderFloat(eShClass, lightProjParamName, alias_cast<Vec4*>(&shadowMat), 4);

        Vec4 ttt(gEnv->p3DEngine->GetSunColor(), gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->GetFVal());
        SetShaderFloat(eShClass, rsmSunColParameterName, (Vec4*)&ttt, 1);
        Vec4 ttt2(gEnv->p3DEngine->GetSunDirNormalized(), 0);
        SetShaderFloat(eShClass, rsmSunDirParameterName, (Vec4*)&ttt2, 1);
    }
    else
    {
        CTextureManager::Instance()->GetBlackTexture()->Apply(12, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1, eShClass);
        CTextureManager::Instance()->GetBlackTexture()->Apply(13, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1, eShClass);
        CTextureManager::Instance()->GetBlackTexture()->Apply(9, m_nTexStatePoint, EFTT_UNKNOWN, -1, -1, eShClass);
        SetShaderFloat(eShClass, lightProjParamName, alias_cast<Vec4*>(&shadowMat), 4);

        Vec4 ttt(0, 0, 0, 0);
        SetShaderFloat(eShClass, rsmSunColParameterName, (Vec4*)&ttt, 1);
        Vec4 ttt2(0, 0, 0, 0);
        SetShaderFloat(eShClass, rsmSunDirParameterName, (Vec4*)&ttt2, 1);
    }
}

ISvoRenderer* CD3D9Renderer::GetISvoRenderer()
{
    return CSvoRenderer::GetInstance(true);
}

void CSvoRenderer::SetShaderFloat(const EHWShaderClass eShClass, const CCryNameR& NameParam, const Vec4* fParams, int nParams)
{
    if (eShClass == eHWSC_Pixel)
    {
        m_pShader->FXSetPSFloat(NameParam, fParams, nParams);
    }
    else if (eShClass == eHWSC_Compute)
    {
        m_pShader->FXSetCSFloat(NameParam, fParams, nParams);
    }
    else if (eShClass == eHWSC_Vertex)
    {
        m_pShader->FXSetVSFloat(NameParam, fParams, nParams);
    }
}

void CSvoRenderer::BindTiledLights(PodArray<I3DEngine::SLightTI>& lightsTI, EHWShaderClass shaderType)
{
    gcpRendD3D->GetTiledShading().BindForwardShadingResources(NULL, shaderType);

    STiledLightShadeInfo* tiledLightShadeInfo = gcpRendD3D->GetTiledShading().GetTiledLightShadeInfo();

    for (int l = 0; l < lightsTI.Count(); l++)
    {
        I3DEngine::SLightTI& svoLight = lightsTI[l];

        if (!svoLight.vDirF.w)
        {
            continue;
        }

        const int tlTypeRegularProjector = 6;

        Vec4 worldViewPos = Vec4(gcpRendD3D->GetViewParameters().vOrigin, 0);

        for (uint32 lightIdx = 0; lightIdx <= 255 && tiledLightShadeInfo[lightIdx].posRad != Vec4(0, 0, 0, 0); ++lightIdx)
        {
            if ((tiledLightShadeInfo[lightIdx].lightType == tlTypeRegularProjector) && svoLight.vPosR.IsEquivalent(tiledLightShadeInfo[lightIdx].posRad + worldViewPos, .5f))
            {
                if (svoLight.vCol.w > 0)
                {
                    svoLight.vCol.w = ((float)lightIdx + 100);
                }
                else
                {
                    svoLight.vCol.w = -((float)lightIdx + 100);
                }
            }
        }
    }
}

CTexture* CSvoRenderer::GetRsmColorMap(ShadowMapFrustum& rFr, bool bCheckUpdate)
{
    if (IsActive() && (rFr.nShadowMapLod == 2) && gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->GetFVal())
    {
        if (bCheckUpdate)
        {
            CSvoRenderer::GetInstance()->CheckCreateUpdateRT(CSvoRenderer::GetInstance()->m_pRsmColorMap, rFr.nShadowMapSize, rFr.nShadowMapSize, eTF_R8G8B8A8, eTT_2D, FT_STATE_CLAMP, "SVO_SUN_RSM_COLOR");
        }

        return CSvoRenderer::GetInstance()->m_pRsmColorMap;
    }

    if (IsActive() && rFr.bUseShadowsPool && gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->GetFVal() && rFr.m_Flags & DLF_USE_FOR_SVOGI)
    {
        if (bCheckUpdate)
        {
            CSvoRenderer::GetInstance()->CheckCreateUpdateRT(CSvoRenderer::GetInstance()->m_pRsmPoolCol, gcpRendD3D->m_nShadowPoolWidth, gcpRendD3D->m_nShadowPoolHeight, eTF_R8G8B8A8, eTT_2D, FT_STATE_CLAMP, "SVO_PRJ_RSM_COLOR");
        }

        return CSvoRenderer::GetInstance()->m_pRsmPoolCol;
    }

    return NULL;
}

CTexture* CSvoRenderer::GetRsmNormlMap(ShadowMapFrustum& rFr, bool bCheckUpdate)
{
    if (IsActive() &&  (rFr.nShadowMapLod == 2) && gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->GetFVal())
    {
        if (bCheckUpdate)
        {
            CSvoRenderer::GetInstance()->CheckCreateUpdateRT(CSvoRenderer::GetInstance()->m_pRsmNormlMap, rFr.nShadowMapSize, rFr.nShadowMapSize, eTF_R8G8B8A8, eTT_2D, FT_STATE_CLAMP, "SVO_SUN_RSM_NORMAL");
        }

        return CSvoRenderer::GetInstance()->m_pRsmNormlMap;
    }

    if (IsActive() && rFr.bUseShadowsPool && gEnv->pConsole->GetCVar("e_svoTI_InjectionMultiplier")->GetFVal() && rFr.m_Flags & DLF_USE_FOR_SVOGI)
    {
        if (bCheckUpdate)
        {
            CSvoRenderer::GetInstance()->CheckCreateUpdateRT(CSvoRenderer::GetInstance()->m_pRsmPoolNor, gcpRendD3D->m_nShadowPoolWidth, gcpRendD3D->m_nShadowPoolHeight, eTF_R8G8B8A8, eTT_2D, FT_STATE_CLAMP, "SVO_PRJ_RSM_NORMAL");
        }

        return CSvoRenderer::GetInstance()->m_pRsmPoolNor;
    }

    return NULL;
}

void CSvoRenderer::CheckCreateUpdateRT(CTexture*& pTex, int nWidth, int nHeight, ETEX_Format eTF, [[maybe_unused]] ETEX_Type eTT, [[maybe_unused]] int nTexFlags, const char* szName)
{
    if ((!pTex) || (pTex->GetWidth() != nWidth) || (pTex->GetHeight() != nHeight) || (pTex->GetTextureDstFormat() != eTF))
    {
        SAFE_RELEASE(pTex);

        char szNameEx[256] = "";
        sprintf_s(szNameEx, "%s_%d_%d", szName, nWidth, nHeight); // workaround for RT management bug

        SD3DPostEffectsUtils::CreateRenderTarget(szNameEx, pTex, nWidth, nHeight, Clr_Unknown, false, false, eTF);

        //iLog->Log("Realloc RT %dx%d, %s, %s", nWidth, nHeight, CTexture::NameForTextureFormat(eTF), szName);
    }
}


void CSvoRenderer::SVoxPool::Init(ITexture* _pTex)
{
    CTexture* pTex = (CTexture*)_pTex;

    if (pTex)
    {
        pUAV = (D3DUnorderedAccessView*)((CTexture*)pTex)->GetDeviceUAV();
        nTexId = pTex->GetTextureID();
    }
}
#endif
