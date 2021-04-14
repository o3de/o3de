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

// Description : shadows support.


#include "RenderDll_precompiled.h"
#include "DriverD3D.h"
#include <IEntityRenderState.h>
#include "../Common/Shadow_Renderer.h"
#include "../Common/ReverseDepth.h"
#include "D3DPostProcess.h"

#include "I3DEngine.h"

#include "GraphicsPipeline/Common/GraphicsPipelinePass.h"

#include "Common/RenderView.h"

#if defined(FEATURE_SVO_GI)
#include "D3D_SVO.h"
#endif

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(D3DShadows_cpp)
#endif

namespace
{
    CryCriticalSection g_cDynTexLock;
}

void CD3D9Renderer::EF_PrepareShadowGenRenderList()
{
    AZ_TRACE_METHOD();
    int nThreadID = m_RP.m_nFillThreadID;
    int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nCurRecLevel >= 0);
    int NumDynLights = m_RP.m_DLights[nThreadID][nCurRecLevel].Num();

    //TFIX nCurRecLevel+1 is incorrect
    TArray<SRenderLight>&  arrDeferLights = CDeferredShading::Instance().GetLights(nThreadID, nCurRecLevel);

    RegisterFinalizeShadowJobs(nThreadID);

    if (NumDynLights <= 0 && arrDeferLights.Num() <= 0)
    {
        return;
    }

    for (int nLightID = 0; nLightID < NumDynLights; nLightID++)
    {
        SRenderLight* pLight = &m_RP.m_DLights[nThreadID][nCurRecLevel][nLightID];
        EF_PrepareShadowGenForLight(pLight, nLightID);
    }

    for (uint32 nDeferLightID = 0; nDeferLightID < arrDeferLights.Num(); nDeferLightID++)
    {
        SRenderLight* pLight = &arrDeferLights[nDeferLightID];
        EF_PrepareShadowGenForLight(pLight, (NumDynLights + nDeferLightID));
    }

    // add custom frustums
    {
        ShadowMapFrustum* arrCustomFrustums;
        int nFrustumCount;
        gEnv->p3DEngine->GetCustomShadowMapFrustums(arrCustomFrustums, nFrustumCount);

        for (uint32 i = 0; i < nFrustumCount; ++i)
        {
            if (PrepareShadowGenForFrustum(&arrCustomFrustums[i], NULL, 0, i))
            {
                SRenderPipeline::SShadowFrustumToRender* pToRender = m_RP.SShadowFrustumToRenderList[nThreadID].AddIndex(1);
                pToRender->pFrustum = &arrCustomFrustums[i];
                pToRender->nRecursiveLevel = nCurRecLevel;
                pToRender->nLightID = 0;
                pToRender->pLight = NULL;
            }
        }
    }
}

bool CD3D9Renderer::EF_PrepareShadowGenForLight(SRenderLight* pLight, int nLightID)
{
    assert((unsigned int) nLightID < (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS));
    if ((unsigned int) nLightID >= (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS))
    {
        //Warning("CRenderer::EF_PrepareShadowGenForLight: Too many light sources used ..."); // redundant
        return false;
    }
    int nThreadID  = m_RP.m_nFillThreadID;
    int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nCurRecLevel >= 0);
    if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS))
    {
        return false;
    }

    const int nLightFrustumBaseID = nLightID * MAX_SHADOWMAP_LOD;

    ShadowMapFrustum** ppSMFrustumList = pLight->m_pShadowMapFrustums;
    if (!ppSMFrustumList)
    {
        return false;
    }


    int32 nCurLOD = 0;

    for (int nCaster = 0; *ppSMFrustumList && nCaster != MAX_GSM_LODS_NUM; ++ppSMFrustumList, ++nCaster)
    {
        //use pools
        if (CV_r_UseShadowsPool && pLight->m_Flags & DLF_DEFERRED_LIGHT)
        {
            (*ppSMFrustumList)->bUseShadowsPool = true;
        }
        else
        {
            (*ppSMFrustumList)->bUseShadowsPool = false;
        }

        if (PrepareShadowGenForFrustum(*ppSMFrustumList, pLight, nLightFrustumBaseID, nCurLOD))
        {
            SRenderPipeline::SShadowFrustumToRender* pToRender = m_RP.SShadowFrustumToRenderList[nThreadID].AddIndex(1);
            pToRender->pFrustum = (*ppSMFrustumList);
            pToRender->nRecursiveLevel = nCurRecLevel;
            pToRender->nLightID = nLightID;
            pToRender->pLight = pLight;
            nCurLOD++;
        }
    }

    return true;
}

bool CD3D9Renderer::PrepareShadowGenForFrustum(ShadowMapFrustum* pCurFrustum, SRenderLight* pLight, [[maybe_unused]] int nLightFrustumBaseID, [[maybe_unused]] int nLOD)
{
    int nThreadID  = m_RP.m_nFillThreadID;
    assert(SRendItem::m_RecurseLevel[nThreadID] == 0);

    PROFILE_FRAME(PrepareShadowGenForFrustum);

    //validate shadow frustum
    assert(pCurFrustum);
    if (!pCurFrustum)
    {
        return false;
    }
    if (pCurFrustum->m_castersList.IsEmpty() && pCurFrustum->m_jobExecutedCastersList.IsEmpty() &&
        !pCurFrustum->IsCached() && pCurFrustum->m_eFrustumType != ShadowMapFrustum::e_GsmDynamicDistance)
    {
        return false;
    }
    if (pCurFrustum->IsCached() && pCurFrustum->nTexSize == 0)
    {
        return false;
    }
    //////////////////////////////////////////////////////////////////////////

    //regenerate on reset device
    if (pCurFrustum->nResetID != m_nFrameReset)
    {
        pCurFrustum->nResetID = m_nFrameReset;
        pCurFrustum->RequestUpdate();
    }

    int nShadowGenGPU = 0;

    if (GetActiveGPUCount() > 1 && CV_r_ShadowGenMode == 1)
    {
        //TOFIx: make m_nFrameSwapID - double buffered
        nShadowGenGPU = gRenDev->RT_GetCurrGpuID();

        pCurFrustum->nOmniFrustumMask = 0x3F;
        //in case there was switch on the fly - regenerate all faces
        if (pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] > 0)
        {
            pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] = 0x3F;
        }
    }



    bool bNotNeedUpdate = false;
    if (pCurFrustum->bOmniDirectionalShadow)
    {
        bNotNeedUpdate = !(pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] & pCurFrustum->nOmniFrustumMask);
    }
    else
    {
        bNotNeedUpdate = !pCurFrustum->isUpdateRequested(nShadowGenGPU);
    }

    if (bNotNeedUpdate && !pCurFrustum->bUseShadowsPool)
    {
        memset(pCurFrustum->nShadowGenID[nThreadID], 0xFF, sizeof(pCurFrustum->nShadowGenID[nThreadID]));
        return pCurFrustum->nShadowGenMask != 0;
    }


    if (pCurFrustum->bUseShadowsPool)
    {
        pCurFrustum->nShadowPoolUpdateRate = min((uint32)CRenderer::CV_r_ShadowPoolMaxFrames, (uint32)pCurFrustum->nShadowPoolUpdateRate);
        if (!bNotNeedUpdate)
        {
            pCurFrustum->nShadowPoolUpdateRate >>= 2;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    //  update is requested - we should generate new shadow map
    //////////////////////////////////////////////////////////////////////////
    //force unwrap frustum
    if (pCurFrustum->bOmniDirectionalShadow)
    {
        pCurFrustum->bUnwrapedOmniDirectional = true;
    }
    else
    {
        pCurFrustum->bUnwrapedOmniDirectional = false;
    }

    ETEX_Type eTT = (pCurFrustum->bOmniDirectionalShadow && !pCurFrustum->bUnwrapedOmniDirectional) ? eTT_Cube : eTT_2D;

    //////////////////////////////////////////////////////////////////////////
    //recalculate LOF rendering params
    //////////////////////////////////////////////////////////////////////////

    //calc texture resolution and frustum resolution
    pCurFrustum->nTexSize = max(pCurFrustum->nTexSize, 32);
    pCurFrustum->nTextureWidth = pCurFrustum->nTexSize;
    pCurFrustum->nTextureHeight = pCurFrustum->nTexSize;
    pCurFrustum->nShadowMapSize = pCurFrustum->nTexSize;

    if (pCurFrustum->bUnwrapedOmniDirectional)
    {
        pCurFrustum->nTextureWidth = pCurFrustum->nTexSize * 3;
        pCurFrustum->nTextureHeight = pCurFrustum->nTexSize * 2;
    }

    //////////////////////////////////////////////////////////////////////////
    //Select shadow buffers format
    //////////////////////////////////////////////////////////////////////////
    ETEX_Format eTF = eTF_D32F;
    if (pCurFrustum->IsCached())
    {
        eTF = CV_r_ShadowsCacheFormat == 0 ? eTF_D32F : eTF_D16;
    }
    else if IsCVarConstAccess(constexpr) (CV_r_shadowtexformat == 0)
    {
        eTF = eTF_D32F;
    }
    else if IsCVarConstAccess(constexpr) (CV_r_shadowtexformat == 1)
    {
        eTF = eTF_D16;
    }
    else
    {
        eTF = eTF_D24S8;
    }

    if (pCurFrustum->bOmniDirectionalShadow && !(pCurFrustum->bUnwrapedOmniDirectional))
    {
        pCurFrustum->bHWPCFCompare = false;
    }
    else
    {
        const bool bSun = pLight && (pLight->m_Flags & DLF_SUN) != 0;
        pCurFrustum->bHWPCFCompare = !bSun || (CV_r_ShadowsPCFiltering != 0);
    }
    //assign requested texture format
    pCurFrustum->m_eReqTF = eTF;
    pCurFrustum->m_eReqTT = eTT;

    //assign owners
    pCurFrustum->pFrustumOwner = pCurFrustum;

    //actual view camera position
    Vec3 vCamOrigin = iSystem->GetViewCamera().GetPosition();

    CCamera tmpCamera;

    int nSides = 1;
    if (pCurFrustum->bOmniDirectionalShadow)
    {
        nSides = OMNI_SIDES_NUM;
    }

    // Static shadow map might not have any active casters, so don't reset nShadowGenMask every frame
    if (!pCurFrustum->IsCached())
    {
        pCurFrustum->nShadowGenMask = pCurFrustum->m_eFrustumType == ShadowMapFrustum::e_GsmDynamicDistance ? 1 : 0;
    }
    Matrix44 m;

    for (int nS = 0; nS < nSides; nS++)
    {
        //update check for shadow frustums
        if (pCurFrustum->bOmniDirectionalShadow && !pCurFrustum->bUseShadowsPool)
        {
            if (!((pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] & pCurFrustum->nOmniFrustumMask) & (1 << nS)))
            {
                continue;
            }
            else
            {
                pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] &= ~(1 << nS);
            }
        }
        else
        {
            pCurFrustum->nInvalidatedFrustMask[nShadowGenGPU] = 0;
        }

        //////////////////////////////////////////////////////////////////////////
        // Calc frustum CCamera for current frustum
        //////////////////////////////////////////////////////////////////////////
        if (!pCurFrustum->bOmniDirectionalShadow)
        {
            if (pCurFrustum->m_Flags & (DLF_PROJECT | DLF_AREA_LIGHT))
            {
                SRenderLight instLight = *pLight;
                if (pLight->m_Flags & DLF_AREA_LIGHT)
                {
                    // Pull the shadow frustum back to encompas the area of the light source.
                    float fMaxSize = max(pLight->m_fAreaWidth, pLight->m_fAreaHeight);
                    float fScale = fMaxSize / max(tanf(DEG2RAD(pLight->m_fLightFrustumAngle)), 0.0001f);

                    Vec3 vOffsetDir = fScale * pLight->m_ObjMatrix.GetColumn0().GetNormalized();
                    instLight.SetPosition(instLight.m_Origin - vOffsetDir);
                    instLight.m_fProjectorNearPlane = fScale;
                }

                // Frustum angle is clamped to prevent projection matrix problems.
                // We clamp here because area lights and non-shadow casting lights can cast 180 degree light.
                CShadowUtils::GetCubemapFrustumForLight(&instLight, 0, min(2 * pLight->m_fLightFrustumAngle, 175.0f), &(pCurFrustum->mLightProjMatrix), &(pCurFrustum->mLightViewMatrix), false);
            }
            else
            {
                if (pCurFrustum->m_eFrustumType == ShadowMapFrustum::e_PerObject)
                {
                    AABB lsBounds = CShadowUtils::GetShadowMatrixForCasterBox(pCurFrustum->mLightProjMatrix, pCurFrustum->mLightViewMatrix, pCurFrustum, 20.f);

                    // normalize filter filter kernel size to dimensions of light space bounding box
                    pCurFrustum->fWidthS *= pCurFrustum->nTextureWidth  / (lsBounds.max.x - lsBounds.min.x);
                    pCurFrustum->fWidthT *= pCurFrustum->nTextureHeight / (lsBounds.max.y - lsBounds.min.y);
                }
                else if (pCurFrustum->m_eFrustumType != ShadowMapFrustum::e_HeightMapAO)
                {
                    CShadowUtils::GetShadowMatrixOrtho(pCurFrustum->mLightProjMatrix,  pCurFrustum->mLightViewMatrix, m_CameraMatrix, pCurFrustum, false);
                }
            }

            //Pre-multiply matrices
            Matrix44 mViewProj =  Matrix44(pCurFrustum->mLightViewMatrix) * Matrix44(pCurFrustum->mLightProjMatrix);
            pCurFrustum->mLightViewMatrix = mViewProj;
            pCurFrustum->mLightProjMatrix.SetIdentity();

            tmpCamera = gEnv->p3DEngine->GetRenderingCamera();
        }
        else
        {
            tmpCamera = pCurFrustum->FrustumPlanes[nS];
        }


        //////////////////////////////////////////////////////////////////////////
        // Invoke IRenderNode::Render Jobs
        //////////////////////////////////////////////////////////////////////////
        uint32 nShadowGenID = m_nShadowGenId[nThreadID];
        m_nShadowGenId[nThreadID] += 1;

        pCurFrustum->nShadowGenID[nThreadID][nS] = nShadowGenID;

        uint32 nRenderingFlags = SRenderingPassInfo::DEFAULT_FLAGS;

#if defined(FEATURE_SVO_GI)
        if (CSvoRenderer::GetRsmColorMap(*pCurFrustum))
        {
            // we need correct diffuse texture for every chunk
            nRenderingFlags |= SRenderingPassInfo::DISABLE_RENDER_CHUNK_MERGE;
        }
#endif
        // create a matching rendering pass info for shadows
        SRenderingPassInfo passInfo = SRenderingPassInfo::CreateShadowPassRenderingInfo(tmpCamera, pCurFrustum->m_Flags, pCurFrustum->nShadowMapLod,
                pCurFrustum->IsCached(), pCurFrustum->bIsMGPUCopy, &pCurFrustum->nShadowGenMask, nS, nShadowGenID, nRenderingFlags);

        StartInvokeShadowMapRenderJobs(pCurFrustum, passInfo);
    }//nSides

    return true;
}

//-----------------------------------------------------------------------------
void CD3D9Renderer::PrepareShadowGenForFrustumNonJobs([[maybe_unused]] const int nFlags)
{
    int nThreadID  = m_RP.m_nFillThreadID;
#if !defined(NDEBUG)
    int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nCurRecLevel >= 0);
#endif

    CCamera tmpCamera;

    for (int i = 0; i < m_RP.SShadowFrustumToRenderList[nThreadID].size(); ++i)
    {
        SRenderPipeline::SShadowFrustumToRender& rFrustumToRender = m_RP.SShadowFrustumToRenderList[nThreadID][i];
        ShadowMapFrustum* pCurFrustum = rFrustumToRender.pFrustum;

        int nSides = pCurFrustum->bOmniDirectionalShadow ? OMNI_SIDES_NUM : 1;

        for (int nS = 0; nS < nSides; nS++)
        {
            if (pCurFrustum->nShadowGenID[nThreadID][nS] == 0xFFFFFFFF)
            {
                continue;
            }

            //////////////////////////////////////////////////////////////////////////
            // Calc frustum CCamera for current frustum
            //////////////////////////////////////////////////////////////////////////
            if (!pCurFrustum->bOmniDirectionalShadow)
            {
                tmpCamera = gEnv->p3DEngine->GetRenderingCamera();
            }
            else
            {
                tmpCamera = pCurFrustum->FrustumPlanes[nS];
            }
            //////////////////////////////////////////////////////////////////////////
            // Invoke IRenderNode::Render
            //////////////////////////////////////////////////////////////////////////


            uint32 nRenderingFlags = SRenderingPassInfo::DEFAULT_FLAGS;

#if defined(FEATURE_SVO_GI)
            if (CSvoRenderer::GetRsmColorMap(*pCurFrustum))
            {
                // we need correct diffuse texture for every chunk
                nRenderingFlags |= SRenderingPassInfo::DISABLE_RENDER_CHUNK_MERGE;
            }
#endif

            // create a matching rendering pass info for shadows
            SRenderingPassInfo passInfo = SRenderingPassInfo::CreateShadowPassRenderingInfo(tmpCamera, pCurFrustum->m_Flags, pCurFrustum->nShadowMapLod,
                    pCurFrustum->IsCached(), pCurFrustum->bIsMGPUCopy, &pCurFrustum->nShadowGenMask, nS, pCurFrustum->nShadowGenID[nThreadID][nS], nRenderingFlags);

            for (int casterIdx = 0; casterIdx < pCurFrustum->m_castersList.Count(); ++casterIdx)
            {
                IShadowCaster* pEnt  = pCurFrustum->m_castersList[casterIdx];

                //TOFIX reactivate OmniDirectionalShadow
                if (pCurFrustum->bOmniDirectionalShadow)
                {
                    AABB aabb = pEnt->GetBBoxVirtual();
                    //!!! Reactivate proper camera
                    bool bVis = tmpCamera.IsAABBVisible_F(aabb);
                    if (!bVis)
                    {
                        continue;
                    }
                }

                if ((pCurFrustum->m_Flags & DLF_DIFFUSEOCCLUSION) && pEnt->HasOcclusionmap(0, pCurFrustum->pLightOwner))
                {
                    continue;
                }

                gEnv->p3DEngine->RenderRenderNode_ShadowPass(pEnt, passInfo, NULL);
            }
        }
    }
}

//-----------------------------------------------------------------------------
// Name: GetTextureRect()
// Desc: Get the dimensions of the texture
//-----------------------------------------------------------------------------
HRESULT GetTextureRect(CTexture* pTexture, RECT* pRect)
{
    pRect->left = 0;
    pRect->top = 0;
    pRect->right = pTexture->GetWidth();
    pRect->bottom = pTexture->GetHeight();

    return S_OK;
}

void CD3D9Renderer::OnEntityDeleted(IRenderNode* pRenderNode)
{
    m_pRT->RC_EntityDelete(pRenderNode);
}

void _DrawText(ISystem* pSystem, int x, int y, const float fScale, const char* format, ...)
PRINTF_PARAMS(5, 6);

void _DrawText([[maybe_unused]] ISystem* pSystem, int x, int y, const float fScale, const char* format, ...)
{
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsprintf_s(buffer, format, args);
    va_end(args);

    float color[4] = {0, 1, 0, 1};
    gEnv->pRenderer->Draw2dLabel((float)x, (float)y, fScale, color, false, buffer);
}

void CD3D9Renderer::DrawAllShadowsOnTheScreen()
{
    float width = 800;
    float height = 600;

    TransformationMatrices backupSceneMatrices;

    Set2DMode(static_cast<uint32>(width), static_cast<uint32>(height), backupSceneMatrices);

    EF_SetColorOp(eCO_MODULATE, eCO_MODULATE, DEF_TEXARG0, DEF_TEXARG0);
    EF_SetSrgbWrite(false);

    int nMaxCount = 16;

    float fArrDim = max(4.f, sqrtf((float)nMaxCount));
    float fPicDimX = width / fArrDim;
    float fPicDimY = height / fArrDim;
    int nShadowId = 0;
    SDynTexture_Shadow* pTX = SDynTexture_Shadow::s_RootShadow.m_NextShadow;
    for (float x = 0; nShadowId < nMaxCount && x < width - 10; x += fPicDimX)
    {
        for (float y = 0; nShadowId < nMaxCount && y < height - 10; y += fPicDimY)
        {
            static ICVar* pVar = iConsole->GetCVar("e_ShadowsDebug");
            if (pVar && pVar->GetIVal() == 1)
            {
                while (pTX->m_pTexture && (pTX->m_pTexture->m_nAccessFrameID + 2) < GetFrameID(false) && pTX != &SDynTexture_Shadow::s_RootShadow)
                {
                    pTX = pTX->m_NextShadow;
                }
            }

            if (pTX == &SDynTexture_Shadow::s_RootShadow)
            {
                break;
            }

            if (pTX->m_pTexture && pTX->pLightOwner)
            {
                CTexture* tp = pTX->m_pTexture;
                if (tp)
                {
                    int nSavedAccessFrameID = pTX->m_pTexture->m_nAccessFrameID;


                    SetState(/*GS_BLSRC_SRCALPHA | GS_BLDST_ONEMINUSSRCALPHA | */ GS_NODEPTHTEST);
                    if (tp->GetTextureType() == eTT_2D)
                    {
                        //Draw2dImage(x, y, fPicDimX-1, fPicDimY-1, tp->GetID(), 0,1,1,0,180);

                        // set shader
                        CShader* pSH(CShaderMan::s_ShaderShadowMaskGen);

                        uint32 nPasses = 0;
                        static CCryNameTSCRC TechName = "DebugShadowMap";
                        pSH->FXSetTechnique(TechName);
                        pSH->FXBegin(&nPasses, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);
                        pSH->FXBeginPass(0);

                        Matrix44A origMatProj = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj;
                        Matrix44A origMatView = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;
                        Matrix44A* m = &m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj;
                        mathMatrixOrthoOffCenterLH(m, 0.0f, (float)width, (float)height, 0.0f, -1e30f, 1e30f);
                        m = &m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;
                        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView.SetIdentity();

                        SetState(GS_NODEPTHTEST);
                        STexState ts(FILTER_LINEAR, false);
                        ts.m_nAnisotropy = 1;
                        tp->Apply(0, CTexture::GetTexState(ts));
                        D3DSetCull(eCULL_None);

                        TempDynVB<SVF_P3F_T3F> vb(gcpRendD3D);
                        vb.Allocate(4);
                        SVF_P3F_T3F* vQuad = vb.Lock();

                        vQuad[0].p = Vec3(x, y, 1);
                        vQuad[0].st = Vec3(0, 1, 1);
                        //vQuad[0].color.dcolor = (uint32)-1;
                        vQuad[1].p = Vec3(x + fPicDimX - 1, y, 1);
                        vQuad[1].st = Vec3(1, 1, 1);
                        //vQuad[1].color.dcolor = (uint32)-1;
                        vQuad[3].p = Vec3(x + fPicDimX - 1, y + fPicDimY - 1, 1);
                        vQuad[3].st = Vec3(1, 0, 1);
                        //vQuad[3].color.dcolor = (uint32)-1;
                        vQuad[2].p = Vec3(x, y + fPicDimY - 1, 1);
                        vQuad[2].st = Vec3(0, 0, 1);
                        //vQuad[2].color.dcolor = (uint32)-1;

                        vb.Unlock();
                        vb.Bind(0);
                        vb.Release();

                        if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
                        {
                            FX_Commit();
                            FX_DrawPrimitive(eptTriangleStrip, 0, 4);
                        }

                        pSH->FXEndPass();
                        pSH->FXEnd();

                        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView = origMatView;
                        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj = origMatProj;
                    }
                    else
                    {
                        // set shader
                        CShader* pSH(CShaderMan::s_ShaderShadowMaskGen);

                        uint32 nPasses = 0;
                        static CCryNameTSCRC TechName = "DebugCubeMap";
                        pSH->FXSetTechnique(TechName);
                        pSH->FXBegin(&nPasses, FEF_DONTSETSTATES | FEF_DONTSETTEXTURES);
                        pSH->FXBeginPass(0);

                        float fSizeX = fPicDimX / 3;
                        float fSizeY = fPicDimY / 2;
                        float fx = ScaleCoordX(x);
                        fSizeX = ScaleCoordX(fSizeX);
                        float fy = ScaleCoordY(y);
                        fSizeY = ScaleCoordY(fSizeY);
                        float fOffsX[] = {fx, fx + fSizeX, fx + fSizeX * 2, fx, fx + fSizeX, fx + fSizeX * 2};
                        float fOffsY[] = {fy, fy, fy, fy + fSizeY, fy + fSizeY, fy + fSizeY};
                        Vec3 vTC0[] = {Vec3(1, 1, 1), Vec3(-1, 1, -1), Vec3(-1, 1, -1), Vec3(-1, -1, 1), Vec3(-1, 1, 1), Vec3(1, 1, -1)};
                        Vec3 vTC1[] = {Vec3(1, 1, -1), Vec3(-1, 1, 1), Vec3(1, 1, -1), Vec3(1, -1, 1), Vec3(1, 1, 1), Vec3(-1, 1, -1)};
                        Vec3 vTC2[] = {Vec3(1, -1, -1), Vec3(-1, -1, 1), Vec3(1, 1, 1), Vec3(1, -1, -1), Vec3(1, -1, 1), Vec3(-1, -1, -1)};
                        Vec3 vTC3[] = {Vec3(1, -1, 1), Vec3(-1, -1, -1), Vec3(-1, 1, 1), Vec3(-1, -1, -1), Vec3(-1, -1, 1), Vec3(1, -1, -1)};

                        Matrix44A origMatProj = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj;
                        Matrix44A origMatView = m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;
                        Matrix44A* m = &m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj;
                        mathMatrixOrthoOffCenterLH(m, 0.0f, (float)m_width, (float)m_height, 0.0f, -1e30f, 1e30f);
                        m = &m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView;
                        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView.SetIdentity();

                        SetState(GS_NODEPTHTEST);
                        STexState ts(FILTER_LINEAR, false);
                        ts.m_nAnisotropy = 1;
                        tp->Apply(0, CTexture::GetTexState(ts));

                        D3DSetCull(eCULL_None);

                        for (int i = 0; i < 6; i++)
                        {
                            TempDynVB<SVF_P3F_T3F> vb(gcpRendD3D);
                            vb.Allocate(4);
                            SVF_P3F_T3F* vQuad = vb.Lock();

                            vQuad[0].p = Vec3(fOffsX[i], fOffsY[i], 1);
                            vQuad[0].st = vTC0[i];
                            //vQuad[0].color.dcolor = (uint32)-1;
                            vQuad[1].p = Vec3(fOffsX[i] + fSizeX - 1, fOffsY[i], 1);
                            vQuad[1].st = vTC1[i];
                            //vQuad[1].color.dcolor = (uint32)-1;
                            vQuad[3].p = Vec3(fOffsX[i] + fSizeX - 1, fOffsY[i] + fSizeY - 1, 1);
                            vQuad[3].st = vTC2[i];
                            //vQuad[3].color.dcolor = (uint32)-1;
                            vQuad[2].p = Vec3(fOffsX[i], fOffsY[i] + fSizeY - 1, 1);
                            vQuad[2].st = vTC3[i];
                            //vQuad[2].color.dcolor = (uint32)-1;

                            vb.Unlock();
                            vb.Bind(0);
                            vb.Release();

                            if (!FAILED(FX_SetVertexDeclaration(0, eVF_P3F_T3F)))
                            {
                                FX_Commit();
                                FX_DrawPrimitive(eptTriangleStrip, 0, 4);
                            }
                        }

                        pSH->FXEndPass();
                        pSH->FXEnd();

                        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matView = origMatView;
                        m_RP.m_TI[m_RP.m_nProcessThreadID].m_matProj = origMatProj;
                    }

                    pTX->m_pTexture->m_nAccessFrameID = nSavedAccessFrameID;
                    ILightSource* pLS = (ILightSource*)pTX->pLightOwner;

                    _DrawText(iSystem, (int)(x / width * 800.f), (int)(y / height * 600.f), 1.0f, "%s \n %d %d-%d %d x%d",
                        pTX->m_pTexture->GetSourceName(),
                        pTX->m_nUniqueID,
                        pTX->m_pTexture ? pTX->m_pTexture->m_nUpdateFrameID : 0,
                        pTX->m_pTexture ? pTX->m_pTexture->m_nAccessFrameID : 0,
                        pTX->nObjectsRenderedCount,
                        pTX->m_nWidth);

                    if (pLS->GetLightProperties().m_sName)
                    {
                        _DrawText(iSystem, (int)(x / width * 800.f), (int)(y / height * 600.f) + 32, 1.0f, "%s", pLS->GetLightProperties().m_sName);
                    }
                }
            }
            pTX = pTX->m_NextShadow;
        }
    }

    Unset2DMode(backupSceneMatrices);
}

Vec3 UnProject(ShadowMapFrustum* pFr, Vec3 vPoint)
{
    const int shadowViewport[4] = {0, 0, 1, 1};

    Vec3 vRes(0, 0, 0);
    gRenDev->UnProject(vPoint.x, vPoint.y, vPoint.z,
        &vRes.x, &vRes.y, &vRes.z,
        (float*)&pFr->mLightViewMatrix,
        (float*)&pFr->mLightProjMatrix,
        shadowViewport);

    return vRes;
}

//////////////////////////////////////////////////////////////////////////

namespace
{
    struct CompareRSMRendItem
    {
        bool operator()(const SRendItem& a, const SRendItem& b) const
        {
            // Decal objects should be rendered last
            int nDecalA = (a.ObjSort & FOB_DECAL_MASK);
            int nDecalB = (b.ObjSort & FOB_DECAL_MASK);
            if ((nDecalA == 0) != (nDecalB == 0))         // Sort by decal flag
            {
                return nDecalA < nDecalB;
            }

            if (nDecalA && nDecalB)
            {
                // decal sorting
                uint32 objSortA_Low(a.ObjSort & 0xFFFF);
                uint32 objSortA_High(a.ObjSort & ~0xFFFF);
                uint32 objSortB_Low(b.ObjSort & 0xFFFF);
                uint32 objSortB_High(b.ObjSort & ~0xFFFF);

                if (objSortA_Low != objSortB_Low)
                {
                    return objSortA_Low < objSortB_Low;
                }

                if (a.SortVal != b.SortVal)
                {
                    return a.SortVal < b.SortVal;
                }

                return objSortA_High < objSortB_High;
            }
            else
            {
                // usual sorting
                if (a.SortVal != b.SortVal)         // Sort by shaders
                {
                    return a.SortVal < b.SortVal;
                }

                if (a.pElem != b.pElem)               // Sort by geometry
                {
                    return a.pElem < b.pElem;
                }

                return a.ObjSort < b.ObjSort;
            }
        }
    };
}

bool CD3D9Renderer::PrepareDepthMap(ShadowMapFrustum* lof, int nLightFrustumID, bool bClearPool)
{
    int nThreadList = m_RP.m_nProcessThreadID;
    assert(lof);
    if (!lof)
    {
        return false;
    }

    //select shadowgen gpu
    int nShadowGenGPU = 0;
    if (GetActiveGPUCount() > 1 && CV_r_ShadowGenMode == 1)
    {
        //TOFIx: make m_nFrameSwapID - double buffered
        nShadowGenGPU = gRenDev->RT_GetCurrGpuID();
    }

    if (GetActiveGPUCount() > 1 && lof->IsCached())
    {
        ShadowFrustumMGPUCache* pFrustumCache = GetShadowFrustumMGPUCache();
        pFrustumCache->nUpdateMaskRT &= ~(1 << gRenDev->RT_GetCurrGpuID());
    }

    // Save previous camera
    int vX, vY, vWidth, vHeight;
    GetViewport(&vX, &vY, &vWidth, &vHeight);
    Matrix44 camMatr = m_CameraMatrix;

    // Setup matrices
    Matrix44A origMatView = m_RP.m_TI[nThreadList].m_matView;
    Matrix44A origMatProj = m_RP.m_TI[nThreadList].m_matProj;

    Matrix44A* m = &m_RP.m_TI[nThreadList].m_matProj;
    m->SetIdentity();
    m = &m_RP.m_TI[nThreadList].m_matView;
    m->SetIdentity();

    lof->mLightProjMatrix.SetIdentity();

    //////////////////////////////////////////////////////////////////////////
    //  Assign RTs
    //////////////////////////////////////////////////////////////////////////
    bool bTextureFromDynPool = false;

    if (lof->bUseShadowsPool)
    {
        lof->nTextureWidth = m_nShadowPoolWidth;
        lof->nTextureHeight = m_nShadowPoolHeight;

        //current eTF should be stored in the shadow frustum
        ETEX_Format ePoolTF = lof->m_eReqTF;
        CTexture::s_ptexRT_ShadowPool->Invalidate(m_nShadowPoolWidth, m_nShadowPoolHeight, ePoolTF);

        if (!CTexture::IsTextureExist(CTexture::s_ptexRT_ShadowPool))
        {
#if !defined(_RELEASE) && !defined(WIN32) && !defined(WIN64)
            __debugbreak(); // don't want any realloc on consoles
#endif
            CTexture::s_ptexRT_ShadowPool->CreateRenderTarget(eTF_Unknown, Clr_FarPlane);
        }

        lof->pDepthTex = CTexture::s_ptexRT_ShadowPool;
    }
    else
    {
        if (lof->m_eFrustumType == ShadowMapFrustum::e_Nearest)
        {
            CTexture* pTX = CTexture::s_ptexNearestShadowMap;
            if (!CTexture::IsTextureExist(pTX))
            {
                pTX->CreateRenderTarget(lof->m_eReqTF, Clr_FarPlane);
            }
            lof->pDepthTex = pTX;

            lof->fWidthS *= lof->nTextureWidth / (float)pTX->GetWidth();
            lof->fWidthT *= lof->nTextureHeight / (float)pTX->GetHeight();

            lof->nTextureWidth = pTX->GetWidth();
            lof->nTextureHeight = pTX->GetHeight();
        }

        else if (lof->IsCached())
        {
            if (lof->m_eFrustumType != ShadowMapFrustum::e_HeightMapAO)
            {
                assert(CV_r_ShadowsCache > 0 && CV_r_ShadowsCache <= MAX_GSM_LODS_NUM);
                const int nStaticMapIndex = clamp_tpl(lof->nShadowMapLod - (CV_r_ShadowsCache - 1), 0, MAX_GSM_LODS_NUM - 1);
                lof->pDepthTex = CTexture::s_ptexCachedShadowMap[nStaticMapIndex];
            }
            else
            {
                lof->pDepthTex = CTexture::s_ptexHeightMapAODepth[0];
            }
        }
        else if (lof->m_eFrustumType != ShadowMapFrustum::e_GsmDynamicDistance)
        {
            bTextureFromDynPool = true;
            SDynTexture_Shadow* pDynTX = SDynTexture_Shadow::GetForFrustum(lof);
            lof->pDepthTex = pDynTX->m_pTexture;
        }
    }

    if (CTexture::IsTextureExist(lof->pDepthTex))
    {
        int nSides = 1;
        if (lof->bOmniDirectionalShadow)
        {
            nSides = OMNI_SIDES_NUM;
        }

        int sideIndex;
        int nFirstShadowGenRI, nLastShadowGenRI;

        int nOldScissor = CV_r_scissor;
        int old_CV_r_nodrawnear = CV_r_nodrawnear;
        int nPersFlags = m_RP.m_TI[nThreadList].m_PersFlags;
        int nPersFlags2 = m_RP.m_PersFlags2;
        int nStateAnd = m_RP.m_StateAnd;
        m_RP.m_TI[nThreadList].m_PersFlags &= ~(RBPF_HDR | RBPF_MIRRORCULL); // In a mirrorcull pass (i.e. cubemap gen), remove mirrorculling for shadow gen (omni shadows should re-enable it later on)
        m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_SHADOWGEN;

        if (!(lof->m_Flags & DLF_DIRECTIONAL))
        {
            m_RP.m_PersFlags2 |= RBPF2_DRAWTOCUBE;
        }

        //hack remove texkill for eTF_DF24 and eTF_D24S8
        if (lof->m_eReqTF == eTF_R32F || lof->m_eReqTF == eTF_R16G16F || lof->m_eReqTF == eTF_R16F || lof->m_eReqTF == eTF_R16G16B16A16F ||
            lof->m_eReqTF == eTF_D16 || lof->m_eReqTF == eTF_D24S8 || lof->m_eReqTF == eTF_D32F || lof->m_eReqTF == eTF_D32FS8)
        {
            m_RP.m_PersFlags2 |= RBPF2_NOALPHABLEND;
            m_RP.m_StateAnd &= ~GS_BLEND_MASK;
        }

        if (lof->m_eReqTF == eTF_R32F || lof->m_eReqTF == eTF_R16G16F || lof->m_eReqTF == eTF_R16F || lof->m_eReqTF == eTF_R16G16B16A16F)
        {
            m_RP.m_PersFlags2 |= RBPF2_NOALPHATEST;
            m_RP.m_StateAnd &= ~GS_ALPHATEST_MASK;
        }
        CCamera saveCam = GetCamera();
        Vec3 vPos = lof->vLightSrcRelPos + lof->vProjTranslation;

        SDepthTexture depthTarget;
        ZeroStruct(depthTarget);

        for (sideIndex = 0; sideIndex < nSides; sideIndex++)
        {
            if (nLightFrustumID >= 0)//skip for custom fustum
            {
                //////////////////////////////////////////////////////////////////////////
                //compute shadow recursive level
                //////////////////////////////////////////////////////////////////////////
                const int nThreadID  = m_RP.m_nProcessThreadID;
                const int nShadowRecur = lof->nShadowGenID[nThreadID][sideIndex];

                if (nShadowRecur == 0xFFFFFFFF)
                {
                    continue;
                }

                assert(nShadowRecur >= 0 && nShadowRecur < MAX_SHADOWMAP_FRUSTUMS);

                assert(nThreadID >= 0 && nThreadID < 2);
                nFirstShadowGenRI = SRendItem::m_ShadowsStartRI[nThreadID][nShadowRecur];
                nLastShadowGenRI = SRendItem::m_ShadowsEndRI[nThreadID][nShadowRecur];
                const bool bClearRequired = lof->IsCached() && !lof->bIncrementalUpdate;
                // If there's something to render, sort by lights.
                if (nLastShadowGenRI - nFirstShadowGenRI > 0)
                {
                    auto& renderItems = CRenderView::CurrentRenderView()->GetRenderItems(SG_SORT_GROUP, EFSLIST_SHADOW_GEN);
                    SRendItem* pFirst = &renderItems[nFirstShadowGenRI];
                    SRendItem::mfSortByLight(pFirst, nLastShadowGenRI - nFirstShadowGenRI, true, false, false);
                }
                else if (!bClearRequired)
                {
                    // Nothing to render and there's no need to clear, so we can just continue.
                    continue;
                }
            }

            depthTarget.nWidth = lof->nTextureWidth;
            depthTarget.nHeight = lof->nTextureHeight;
            depthTarget.nFrameAccess = -1;
            depthTarget.bBusy = false;
            depthTarget.pTex = lof->pDepthTex;
            depthTarget.pTarget = lof->pDepthTex->GetDevTexture()->Get2DTexture();
            depthTarget.pSurf = lof->bOmniDirectionalShadow && !(lof->bUnwrapedOmniDirectional)
                ? static_cast<D3DDepthSurface*>(lof->pDepthTex->GetDeviceDepthStencilSurf(sideIndex, 1))
                : static_cast<D3DDepthSurface*>(lof->pDepthTex->GetDeviceDepthStencilSurf());

            CCamera tmpCamera;
            if (!lof->bOmniDirectionalShadow)
            {
                *m =  lof->mLightViewMatrix;
                m_RP.m_TI[nThreadList].m_PersFlags &= ~RBPF_REVERSE_DEPTH;

                uint32 depthState = ReverseDepthHelper::ConvertDepthFunc(m_RP.m_CurState);
                FX_SetState(m_RP.m_CurState, m_RP.m_CurAlphaRef, depthState);

#if defined(FEATURE_SVO_GI)
                if (!(lof->m_Flags & DLF_DIRECTIONAL) && CSvoRenderer::GetRsmColorMap(*lof))
                {
                    m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_MIRRORCULL;
                }
#endif
            }
            else
            {
                const Matrix34& m34 = lof->FrustumPlanes[sideIndex].GetMatrix();
                CameraViewParameters c;
                c.Perspective(tmpCamera.GetFov(), tmpCamera.GetProjRatio(), tmpCamera.GetNearPlane(), tmpCamera.GetFarPlane());
                Vec3 vEyeC = tmpCamera.GetPosition();
                Vec3 vAtC = vEyeC + Vec3(m34(0, 1), m34(1, 1), m34(2, 1));
                Vec3 vUpC = Vec3(m34(0, 2), m34(1, 2), m34(2, 2));
                c.LookAt(vEyeC, vAtC, vUpC);
                ApplyViewParameters(c);
                CShadowUtils::GetCubemapFrustum(FTYP_SHADOWOMNIPROJECTION, lof, sideIndex, &m_RP.m_TI[nThreadList].m_matProj, &m_RP.m_TI[nThreadList].m_matView, NULL);

                //enable back facing for omni lights for now
                m_RP.m_TI[nThreadList].m_PersFlags |= RBPF_MIRRORCULL;
                if (lof->m_Flags & DLF_AREA_LIGHT)
                {
                    m_RP.m_TI[nThreadList].m_PersFlags &= ~RBPF_MIRRORCULL;
                }

#if defined(FEATURE_SVO_GI)
                if (!(lof->m_Flags & DLF_DIRECTIONAL) && CSvoRenderer::GetRsmColorMap(*lof))
                {
                    m_RP.m_TI[nThreadList].m_PersFlags &= ~RBPF_MIRRORCULL;
                }
#endif
            }

            EF_SetCameraInfo();

            //assign for shader's parameters
            SRenderPipeline::ShadowInfo& shadowInfo = m_RP.m_ShadowInfo;
            shadowInfo.m_pCurShadowFrustum = lof;
            shadowInfo.m_nOmniLightSide = sideIndex;
            shadowInfo.vViewerPos = saveCam.GetPosition();

            {
                CStandardGraphicsPipeline::ShadowParameters shadowParams;
                shadowParams.m_ShadowFrustum = lof;
                shadowParams.m_OmniLightSideIndex = sideIndex;
                shadowParams.m_ViewerPos = shadowInfo.vViewerPos;
                GetGraphicsPipeline().UpdatePerShadowConstantBuffer(shadowParams);

                AzRHI::ConstantBuffer* perShadow = GetGraphicsPipeline().GetPerShadowConstantBuffer().get();
                m_DevMan.BindConstantBuffer(eHWSC_Vertex, perShadow, eConstantBufferShaderSlot_PerPass);
                m_DevMan.BindConstantBuffer(eHWSC_Pixel, perShadow, eConstantBufferShaderSlot_PerPass);
                m_DevMan.BindConstantBuffer(eHWSC_Geometry, perShadow, eConstantBufferShaderSlot_PerPass);
                m_DevMan.BindConstantBuffer(eHWSC_Hull, perShadow, eConstantBufferShaderSlot_PerPass);
                m_DevMan.BindConstantBuffer(eHWSC_Domain, perShadow, eConstantBufferShaderSlot_PerPass);
                m_DevMan.BindConstantBuffer(eHWSC_Compute, perShadow, eConstantBufferShaderSlot_PerPass);
            }

#if defined(FEATURE_SVO_GI)
            if (CSvoRenderer::GetRsmColorMap(*lof, true) && CSvoRenderer::GetRsmNormlMap(*lof, true))
            {
                FX_PushRenderTarget(0, CSvoRenderer::GetInstance()->GetRsmColorMap(*lof), &depthTarget, lof->m_eReqTT == eTT_Cube ? sideIndex : -1);
                FX_PushRenderTarget(1, CSvoRenderer::GetInstance()->GetRsmNormlMap(*lof), NULL, lof->m_eReqTT == eTT_Cube ? sideIndex : -1);
            }
            else
#endif
            {
                FX_PushRenderTarget(0, nullptr, &depthTarget, lof->m_eReqTT == eTT_Cube ? sideIndex : -1);
                FX_SetColorDontCareActions(0, true, true);
            }

            //SDW-GEN_REND_PATH
            //////////////////////////////////////////////////////////////////////////
            // clear frame buffer after RT push
            //////////////////////////////////////////////////////////////////////////
            if (!lof->bIncrementalUpdate)
            {
                uint32_t clearFlags = 0;

                const bool bReverseDepth = (m_RP.m_TI[nThreadList].m_PersFlags & RBPF_REVERSE_DEPTH) != 0;

                if (lof->bUseShadowsPool)
                {
                    if (bClearPool)
                    {
                        const RECT rect = { lof->packX[sideIndex], lof->packY[sideIndex], lof->packX[sideIndex] + lof->packWidth[sideIndex], lof->packY[sideIndex] + lof->packHeight[sideIndex] };

                        FX_ClearTarget(&depthTarget, CLEAR_ZBUFFER, Clr_FarPlane_R.r, 0, 1, &rect, false);
                        clearFlags |= CLEAR_ZBUFFER;
                    }
                }
                else
                {
#if defined(FEATURE_SVO_GI)
                    if (CSvoRenderer::GetRsmColorMap(*lof, true) && CSvoRenderer::GetRsmNormlMap(*lof, true))
                    {
                        FX_ClearTarget(CSvoRenderer::GetInstance()->GetRsmColorMap(*lof), Clr_Transparent);
                        FX_ClearTarget(CSvoRenderer::GetInstance()->GetRsmNormlMap(*lof), Clr_Transparent);
                        clearFlags |= CLEAR_RTARGET;
                    }
#endif
                    FX_ClearTarget(&depthTarget, CLEAR_ZBUFFER | CLEAR_STENCIL, Clr_FarPlane_R.r, 0);
                    clearFlags |= CLEAR_ZBUFFER | CLEAR_STENCIL;
#if defined(CRY_USE_METAL)
                    //Clear calls are cached until a draw call is made. If there is nothing in the caster list no draw calls will be made.
                    //Hence make a draw call to clear the render targets.
                    if (lof->m_castersList.IsEmpty() && lof->m_jobExecutedCastersList.IsEmpty())
                    {
                        FX_Commit();
                        FX_ClearTargetRegion();
                    }
#endif
                }

                m_pNewTarget[0]->m_ClearFlags = 0;

                FX_SetColorDontCareActions(0, !(clearFlags & CLEAR_RTARGET), false);
                FX_SetColorDontCareActions(1, !(clearFlags & CLEAR_RTARGET), false);
                FX_SetColorDontCareActions(2, !(clearFlags & CLEAR_RTARGET), false);
#if !defined(OPENGL_ES) // some drivers don't play well with the following
                FX_SetStencilDontCareActions(0, !(clearFlags & CLEAR_STENCIL), true);
                FX_SetStencilDontCareActions(1, !(clearFlags & CLEAR_STENCIL), true);
                FX_SetStencilDontCareActions(2, !(clearFlags & CLEAR_STENCIL), true);
#endif
            }
            else
            {
                // CONFETTI BEGIN: David Srour
                // Metal Load/Store Actions
                FX_SetColorDontCareActions(0, true, true);
                FX_SetStencilDontCareActions(0, true, true);
                FX_SetColorDontCareActions(1, true, true);
                FX_SetStencilDontCareActions(1, true, true);
                FX_SetColorDontCareActions(2, true, true);
                FX_SetStencilDontCareActions(2, true, true);
                // CONFETTI END
            }

            //set proper side-viewport
            if (lof->bUnwrapedOmniDirectional || lof->bUseShadowsPool)
            {
                int arrViewport[4];
                lof->GetSideViewport(sideIndex, arrViewport);
                RT_SetViewport(arrViewport[0], arrViewport[1], arrViewport[2], arrViewport[3]);
            }

            FX_Commit(true);

#if defined(FEATURE_SVO_GI)
            if (!CSvoRenderer::GetRsmColorMap(*lof))
#endif
            {
                FX_SetState(GS_COLMASK_NONE, -1);
                m_RP.m_PersFlags2 |= RBPF2_DISABLECOLORWRITES;
                m_RP.m_StateOr |= GS_COLMASK_NONE;
            }

            if (lof->fDepthSlopeBias > 0.0f && (lof->m_Flags & DLF_DIRECTIONAL))
            {
                float fShadowsBias = CV_r_ShadowsBias;
                float fShadowsSlopeScaleBias = lof->fDepthSlopeBias;

                //adjust nearest slope fer nearest custom frustum
                if (lof->m_eFrustumType == ShadowMapFrustum::e_Nearest)
                {
                    fShadowsSlopeScaleBias *= 7.0f;
                }

                SStateRaster CurRS = m_StatesRS[m_nCurStateRS];
                CurRS.Desc.DepthBias = 0;
                CurRS.Desc.DepthBiasClamp = fShadowsBias * 20;
                CurRS.Desc.SlopeScaledDepthBias = fShadowsSlopeScaleBias;
                SetRasterState(&CurRS);
            }


            if (!(lof->m_Flags & DLF_LIGHT_BEAM))
            {
                D3DSetCull(eCULL_None);
            }
            else
            {
                D3DSetCull(eCULL_Back);
                m_RP.m_PersFlags2 |= RBPF2_LIGHTSHAFTS;
            }

            if (nLightFrustumID < 0)
            {
                FX_ProcessRenderList(EFSLIST_GENERAL, 0, FX_FlushShader_ShadowGen, false);
                FX_ProcessRenderList(EFSLIST_GENERAL, 1, FX_FlushShader_ShadowGen, false);
            }
            else
            if (!lof->m_castersList.IsEmpty() || !lof->m_jobExecutedCastersList.IsEmpty())
            {
                FX_ProcessRenderList(nFirstShadowGenRI,
                    nLastShadowGenRI,
                    EFSLIST_SHADOW_GEN, 0, FX_FlushShader_ShadowGen, false);
            }

            FX_PopRenderTarget(0);

#if defined(FEATURE_SVO_GI)
            if (CSvoRenderer::GetRsmColorMap(*lof) && CSvoRenderer::GetRsmNormlMap(*lof))
            {
                FX_PopRenderTarget(1);
            }
#endif

            m_RP.m_PersFlags2 &= ~RBPF2_DISABLECOLORWRITES;
            m_RP.m_StateOr &= ~GS_COLMASK_NONE;
            if (CV_r_ShadowsBias > 0.0f)
            {
                SStateRaster CurRS = m_StatesRS[m_nCurStateRS];
                CurRS.Desc.DepthBias = 0;
                CurRS.Desc.DepthBiasClamp = 0;
                CurRS.Desc.SlopeScaledDepthBias = 0;
                SetRasterState(&CurRS);
            }
        }

        m_RP.m_TI[nThreadList].m_PersFlags &= ~RBPF_SHADOWGEN;
        SetCamera(saveCam);
        if (lof->m_eReqTT == eTT_Cube)
        {
            lof->mLightViewMatrix.SetIdentity();
            lof->mLightViewMatrix.SetTranslation(vPos);
            lof->mLightViewMatrix.Transpose();
        }

        if (!(lof->m_Flags & DLF_DIRECTIONAL))
        {
            m_RP.m_PersFlags2 &= ~RBPF2_DRAWTOCUBE;
        }

        m_RP.m_TI[nThreadList].m_PersFlags &= ~RBPF_MIRRORCULL;
        m_RP.m_PersFlags2 &= ~RBPF2_LIGHTSHAFTS;

        CV_r_nodrawnear = old_CV_r_nodrawnear;
        CV_r_scissor = nOldScissor;

        m_RP.m_TI[nThreadList].m_PersFlags = nPersFlags;
        m_RP.m_PersFlags2 = nPersFlags2;
        m_RP.m_StateAnd = nStateAnd;

        EF_Scissor(false, 0, 0, 0, 0);
    }
    else if (bTextureFromDynPool)
    {
        iLog->Log("Error: cannot create depth texture  for frustum '%d' (skipping)", lof->nShadowMapLod);
    }

    m_RP.m_TI[nThreadList].m_matView = origMatView;
    m_RP.m_TI[nThreadList].m_matProj = origMatProj;
    m_CameraMatrix = camMatr;
    EF_SetCameraInfo();
    RT_SetViewport(vX, vY, vWidth, vHeight);

    return true;
}

void CD3D9Renderer::ConfigShadowTexgen(int Num, ShadowMapFrustum* pFr, int nFrustNum, [[maybe_unused]] bool bScreenToLocalBasis, bool bUseComparisonSampling)
{
    Matrix44A shadowMat, mTexScaleBiasMat, mLightView, mLightProj, mLightViewProj;
    bool bGSM = false;

    //check for successful PrepareDepthMap
    if (!pFr->pDepthTex && !pFr->bUseShadowsPool)
    {
        return;
    }

    float fOffsetX = 0.5f;
    float fOffsetY = 0.5f;
    const Matrix44A mClipToTexSpace = Matrix44(0.5f,     0.0f,     0.0f,    0.0f,
            0.0f,    -0.5f,     0.0f,    0.0f,
            0.0f,     0.0f,     1.0f,    0.0f,
            fOffsetX, fOffsetY, 0.0f,    1.0f);

    mTexScaleBiasMat = mClipToTexSpace;

    if ((pFr->bOmniDirectionalShadow || pFr->bUseShadowsPool) && nFrustNum > -1)
    {
        if (!pFr->bOmniDirectionalShadow)
        {
            mLightView = pFr->mLightViewMatrix;
            mLightProj.SetIdentity();
        }
        else
        {
            CShadowUtils::GetCubemapFrustum(FTYP_SHADOWOMNIPROJECTION, pFr, nFrustNum,  &mLightProj, &mLightView);
        }

        float arrOffs[2];
        float arrScale[2];
        pFr->GetTexOffset(nFrustNum, arrOffs, arrScale, m_nShadowPoolWidth, m_nShadowPoolHeight);

        //calculate crop matrix for  frustum
        //TD: investigate proper half-texel offset with mCropView
        Matrix44 mCropView(arrScale[0],     0.0f,  0.0f,   0.0f,
            0.0f,  arrScale[1],  0.0f,   0.0f,
            0.0f,     0.0f,  1.0f,   0.0f,
            arrOffs[0], arrOffs[1],  0.0f,   1.0f);

        // multiply the projection matrix with it
        mTexScaleBiasMat = mTexScaleBiasMat * mCropView;

        //constants for gsm atlas
        m_cEF.m_TempVecs[6].x = arrOffs[0];
        m_cEF.m_TempVecs[6].y = arrOffs[1];
    }
    else
    {
        mLightView = pFr->mLightViewMatrix;

        if (pFr->m_eFrustumType == ShadowMapFrustum::e_GsmDynamicDistance)
        {
            Matrix44 mCropView(IDENTITY);
            mCropView.m00 = (float)pFr->packWidth[0]  / pFr->pDepthTex->GetWidth();
            mCropView.m11 = (float)pFr->packHeight[0] / pFr->pDepthTex->GetHeight();
            mCropView.m30 = (float)pFr->packX[0] / pFr->pDepthTex->GetWidth();
            mCropView.m31 = (float)pFr->packY[0] / pFr->pDepthTex->GetHeight();

            mTexScaleBiasMat = mTexScaleBiasMat * mCropView;
        }

        mLightProj.SetIdentity();
        bGSM = true;
    }

    mLightViewProj = mLightView * mLightProj;
    shadowMat = mLightViewProj * mTexScaleBiasMat;

    //set shadow matrix
    gRenDev->m_TempMatrices[Num][0] = shadowMat.GetTransposed();
    m_cEF.m_TempVecs[5] = Vec4(mLightViewProj.m30, mLightViewProj.m31, mLightViewProj.m32, 1);

    //////////////////////////////////////////////////////////////////////////
    // Deferred shadow pass setup
    //////////////////////////////////////////////////////////////////////////
    Matrix44 mScreenToShadow;
    int vpX, vpY, vpWidth, vpHeight;
    GetViewport(&vpX, &vpY, &vpWidth, &vpHeight);
    FX_DeferredShadowPassSetup(gRenDev->m_TempMatrices[Num][0], pFr, (float)vpWidth, (float)vpHeight,
        mScreenToShadow, pFr->m_eFrustumType == ShadowMapFrustum::e_Nearest);

#if defined(VOLUMETRIC_FOG_SHADOWS)
    //use cur TexGen for homogeneous position reconstruction
    if (bScreenToLocalBasis && CRenderer::CV_r_FogShadowsMode == 1)
    {
        Matrix44A mLocalScale;
        mLocalScale.SetIdentity();
        gRenDev->m_TempMatrices[Num][0] = mScreenToShadow.GetTransposed();
        float fScreenScale = (CV_r_FogShadows == 2) ? 4.0f : 2.0f;
        mLocalScale.m00 = fScreenScale;
        mLocalScale.m11 = fScreenScale;
        gRenDev->m_TempMatrices[Num][0] = gRenDev->m_TempMatrices[Num][0] * mLocalScale;
    }
#endif

    gRenDev->m_TempMatrices[Num][2].m33 = 0.0f;
    if (bGSM && pFr->bBlendFrustum)
    {
        const float fBlendVal = pFr->fBlendVal;

        m_cEF.m_TempVecs[15][0] = fBlendVal;
        m_cEF.m_TempVecs[15][1] = 1.0f / (1.0f - fBlendVal);
        m_cEF.m_TempVecs[15][2] = 0.0f;
        m_cEF.m_TempVecs[15][3] = 0.0f;

        m_cEF.m_TempVecs[6] = Vec4(1.f, 1.f, 0.f, 0.f);
        if (pFr->m_eFrustumType == ShadowMapFrustum::e_GsmDynamicDistance)
        {
            m_cEF.m_TempVecs[6].x = pFr->pDepthTex->GetWidth() /  float(pFr->packWidth[0]);
            m_cEF.m_TempVecs[6].y = pFr->pDepthTex->GetHeight() / float(pFr->packHeight[0]);
            m_cEF.m_TempVecs[6].z = -pFr->packX[0] / float(pFr->packWidth[0]);
            m_cEF.m_TempVecs[6].w = -pFr->packY[0] / float(pFr->packHeight[0]);
        }

        const ShadowMapFrustum* pPrevFr = pFr->pPrevFrustum;
        if (pPrevFr)
        {
            Matrix44A mLightViewPrev = pPrevFr->mLightViewMatrix;
            Matrix44A shadowMatPrev =  mLightViewPrev * mClipToTexSpace; // NOTE: no sub-rect here as blending code assumes full [0-1] UV range;

            FX_DeferredShadowPassSetupBlend(shadowMatPrev.GetTransposed(), Num, (float)vpWidth, (float)vpHeight);

            m_cEF.m_TempVecs[2][2] = 1.f / (pPrevFr->fFarDist);

            float fBlendValPrev = pPrevFr->fBlendVal;

            m_cEF.m_TempVecs[15][2] = fBlendValPrev;
            m_cEF.m_TempVecs[15][3] = 1.0f / (1.0f - fBlendValPrev);
        }
    }

    Matrix33 mRotMatrix(mLightView);
    mRotMatrix.OrthonormalizeFast();
    gRenDev->m_TempMatrices[0][1] =  Matrix44(mRotMatrix).GetTransposed();

    if (Num >= 0)
    {
        if (!pFr->pDepthTex && !pFr->bUseShadowsPool)
        {
            Warning("Warning: CD3D9Renderer::ConfigShadowTexgen: pFr->depth_tex_id not set");
        }
        else
        {
            int nID = 0;
            int nIDBlured = 0;
            if (pFr->bUseShadowsPool)
            {
                nID = CTexture::s_ptexRT_ShadowPool->GetID();
            }
            else
            if (pFr->pDepthTex != NULL)
            {
                nID = pFr->pDepthTex->GetID();
            }

            m_RP.m_ShadowCustomTexBind[Num * 2 + 0] = nID;
            m_RP.m_ShadowCustomTexBind[Num * 2 + 1] = nIDBlured;
            m_RP.m_ShadowCustomComparisonSampling[Num * 2 + 0] = bUseComparisonSampling;

            m_cEF.m_TempVecs[8][0] = pFr->fShadowFadingDist;

            assert(Num < 4);
            if (pFr->bHWPCFCompare)
            {
                if (pFr->m_Flags & DLF_DIRECTIONAL)
                {
                    //linear case + constant offset
                    m_cEF.m_TempVecs[1][Num] = pFr->fDepthConstBias;
                    if (pFr->m_eFrustumType == ShadowMapFrustum::e_Nearest)
                    {
                        m_cEF.m_TempVecs[1][Num] *= 3.0f;
                    }
                }
                else
                if (pFr->m_Flags & DLF_PROJECT)
                {
                    //non-linear case
                    m_cEF.m_TempVecs[1][Num] = pFr->fDepthConstBias;
                }
                else
                {
                    m_cEF.m_TempVecs[1][Num] = pFr->fDepthConstBias;
                }
            }
            else
            {
                //linear case
                m_cEF.m_TempVecs[1][Num] = pFr->fDepthTestBias;
            }

            m_cEF.m_TempVecs[2][Num] = 1.f / (pFr->fFarDist);
            m_cEF.m_TempVecs[9][Num] = 1.f / pFr->nTexSize;
            m_cEF.m_TempVecs[3][Num] = 0.0f;

            float fShadowJitter = m_shadowJittering;

            if (pFr->m_Flags & DLF_DIRECTIONAL)
            {
                float fFilteredArea = fShadowJitter * (pFr->fWidthS + pFr->fBlurS);

                if (pFr->m_eFrustumType == ShadowMapFrustum::e_Nearest)
                {
                    fFilteredArea *= 0.1f;
                }

                m_cEF.m_TempVecs[4].x = fFilteredArea;
                m_cEF.m_TempVecs[4].y = fFilteredArea;
            }
            else
            {
                fShadowJitter = 2.0;
                m_cEF.m_TempVecs[4].x = fShadowJitter;
                ;
                m_cEF.m_TempVecs[4].y = fShadowJitter;

                if (pFr->bOmniDirectionalShadow)
                {
                    m_cEF.m_TempVecs[4].x *= 1.0f / 3.0f;
                    m_cEF.m_TempVecs[4].y *= 1.0f / 2.0f;
                }
            }
        }
    }
}

//=============================================================================================================
void CD3D9Renderer::FX_SetupForwardShadows(bool bUseShaderPermutations)
{
    const int FORWARD_SHADOWS_CASCADE0_SINGLE_TAP = 0x10;
    const int FORWARD_SHADOWS_CLOUD_SHADOWS       = 0x20;

    const int nThreadID = m_RP.m_nProcessThreadID;
    const int nSunFrustumID = 0;

    const int nStartIdx = SRendItem::m_StartFrust[nThreadID][nSunFrustumID];
    const int nEndIdx = SRendItem::m_EndFrust[nThreadID][nSunFrustumID];

    if (bUseShaderPermutations)
    {
        m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_SAMPLE0] | g_HWSR_MaskBit[HWSR_SAMPLE1] | g_HWSR_MaskBit[HWSR_SAMPLE2] | g_HWSR_MaskBit[HWSR_SAMPLE3] |
                                   g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ]);
    }

    uint32 nCascadeMask = 0;
    for (int a = nStartIdx, cascadeCount = 0; a < nEndIdx && cascadeCount < 4; ++a)
    {
        ShadowMapFrustum* pFr = &m_RP.m_SMFrustums[nThreadID][nSunFrustumID][a];
        nCascadeMask |= 0x1 << a;

        ConfigShadowTexgen(cascadeCount, pFr, -1, true);

        if (bUseShaderPermutations)
        {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SAMPLE0 + cascadeCount];
        }

        ++cascadeCount;
    }

    // only do full pcf filtering on nearest shadow cascade
    if (nCascadeMask > 0 && m_RP.m_SMFrustums[nThreadID][nSunFrustumID][nStartIdx].nShadowMapLod != 0)
    {
        nCascadeMask |= FORWARD_SHADOWS_CASCADE0_SINGLE_TAP;
    }

    if (m_bCloudShadowsEnabled && m_cloudShadowTexId > 0)
    {
        nCascadeMask |= FORWARD_SHADOWS_CLOUD_SHADOWS;

        if (bUseShaderPermutations)
        {
            m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_LIGHT_TEX_PROJ];
        }
    }

    // store cascade mask in m_TempVecs[4].z
    *alias_cast<uint32*>(&m_cEF.m_TempVecs[4].z) = nCascadeMask;
}

void CD3D9Renderer::FX_SetupShadowsForTransp()
{
    PROFILE_FRAME(SetupShadowsForTransp);

    m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_POINT_LIGHT] | g_HWSR_MaskBit[HWSR_SHADOW_MIXED_MAP_G16R16]);


    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_PARTICLE_SHADOW];

    if (m_shadowJittering > 0.0f)
    {
        m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_SHADOW_JITTERING];
    }

    // Always use PCF for shadows for transparent
    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[ HWSR_HW_PCF_COMPARE ];

    FX_SetupForwardShadows(true);
}

void CD3D9Renderer::FX_SetupShadowsForFog()
{
    PROFILE_FRAME(FX_SetupShadowsForFog);

    m_RP.m_FlagsShader_RT &= ~(g_HWSR_MaskBit[HWSR_POINT_LIGHT] | g_HWSR_MaskBit[HWSR_HW_PCF_COMPARE] | g_HWSR_MaskBit[HWSR_SHADOW_JITTERING] |
                               g_HWSR_MaskBit[HWSR_SHADOW_MIXED_MAP_G16R16]);

    m_RP.m_FlagsShader_RT |= g_HWSR_MaskBit[HWSR_HW_PCF_COMPARE] | g_HWSR_MaskBit[HWSR_PARTICLE_SHADOW];

    FX_SetupForwardShadows();
}


bool CD3D9Renderer::FX_PrepareDepthMapsForLight(const SRenderLight& rLight, int nLightID, bool bClearPool)
{
    int nThreadID = m_RP.m_nProcessThreadID;
    int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nCurRecLevel >= 0);

    if ((m_RP.m_TI[nThreadID].m_PersFlags & RBPF_NO_SHADOWGEN) != 0)
    {
        return false;
    }

    //render all valid shadow frustums
    const int nStartIdx = SRendItem::m_StartFrust[nThreadID][nLightID];
    const int nEndIdx = SRendItem::m_EndFrust[nThreadID][nLightID];
    if (nStartIdx == nEndIdx)
    {
        return false;
    }

    AZ_Assert((nEndIdx - nStartIdx) <= MAX_GSM_LODS_NUM, "Number of shadow frustums is more than max GSM LODs supported.");

    bool processedAtLeastOneShadow = false;

    for (int nFrustIdx = nEndIdx - 1; nFrustIdx >= nStartIdx; --nFrustIdx)
    {
        ShadowMapFrustum* pCurFrustum = &m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nFrustIdx];

        int nLightFrustumID = nLightID * MAX_SHADOWMAP_LOD + (nFrustIdx - nStartIdx);
        if (!pCurFrustum)
        {
            continue;
        }

        if (pCurFrustum->m_eReqTT == eTT_1D || pCurFrustum->m_eReqTF == eTF_Unknown)
        {
            // looks like unitialized shadow frustum for 1 frame - some mt issue
            continue;
        }

        const bool bSun = (rLight.m_Flags & DLF_SUN) != 0;

        // Per-object shadows are added to the "custom" shadow list in CRenderer::FinalizeRendItems_FindShadowFrustums.
        // Do not render them twice.
        if (pCurFrustum->m_eFrustumType != ShadowMapFrustum::e_PerObject)
        {
#ifndef _RELEASE
            char frustumLabel[64];

            if (bSun)
            {
                const int nShadowRecur = pCurFrustum->nShadowGenID[nThreadID][0];
                const int nRendItemCount = nShadowRecur != 0xFFFFFFFF ? (SRendItem::m_ShadowsEndRI[nThreadID][nShadowRecur] - SRendItem::m_ShadowsStartRI[nThreadID][nShadowRecur]) : 0;

                const char* frustumTextSun[] =
                {
                    "GSM FRUSTUM %i",
                    "GSM DISTANCE FRUSTUM %i",
                    "GSM CACHED FRUSTUM %i",
                    "HEIGHT MAP AO FRUSTUM %i",
                    "NEAREST FRUSTUM",
                    "UNKNOWN"
                };

                frustumLabel[0] = 0;

                if (!pCurFrustum->IsCached() || nRendItemCount > 0)
                {
                    sprintf_s(frustumLabel, frustumTextSun[pCurFrustum->m_eFrustumType], m_RP.m_SMFrustums[nThreadID][nCurRecLevel][nFrustIdx].nShadowMapLod);
                }
            }
            else
            {
                sprintf_s(frustumLabel, "FRUSTUM %i", nFrustIdx - nStartIdx);
            }

            if (frustumLabel[0])
            {
                PROFILE_LABEL_PUSH(frustumLabel);
            }
#endif
            // merge cached shadow maps and corresponding dynamic shadow maps.
            if (bSun && pCurFrustum->m_eFrustumType == ShadowMapFrustum::e_GsmDynamicDistance)
            {
                assert(0 <= nStartIdx && nStartIdx <= nEndIdx && nEndIdx <= m_RP.m_SMFrustums[nThreadID][nCurRecLevel].size());
                auto pBegin = m_RP.m_SMFrustums[nThreadID][nCurRecLevel].Data() + nStartIdx;
                auto pEnd = m_RP.m_SMFrustums[nThreadID][nCurRecLevel].Data() + nEndIdx;
                auto pCachedFrustum = std::find_if(pBegin, pEnd,
                        [=](ShadowMapFrustum& fr)
                        {
                            return fr.nShadowMapLod == pCurFrustum->nShadowMapLod && fr.m_eFrustumType == ShadowMapFrustum::e_GsmCached;
                        });

                pCurFrustum->pDepthTex = NULL;
                if (pCachedFrustum != pEnd)
                {
                    FX_MergeShadowMaps(pCurFrustum, pCachedFrustum);
                    processedAtLeastOneShadow = true;
                }
            }

            if (PrepareDepthMap(pCurFrustum, nLightFrustumID, bClearPool))
            {
                bClearPool = false;
                processedAtLeastOneShadow = true;
            }

#ifndef _RELEASE
            if (frustumLabel[0])
            {
                PROFILE_LABEL_POP(frustumLabel);
            }
#endif
        }
    }

    return processedAtLeastOneShadow;
}

void CD3D9Renderer::EF_PrepareCustomShadowMaps()
{
    int nThreadID = m_RP.m_nProcessThreadID;
    int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nCurRecLevel >= 0);

    if ((m_RP.m_TI[nThreadID].m_PersFlags & RBPF_NO_SHADOWGEN) != 0)
    {
        return;
    }

    int NumDynLights = m_RP.m_DLights[nThreadID][nCurRecLevel].Num();

    TArray<SRenderLight>&  arrDeferLights = CDeferredShading::Instance().GetLights(nThreadID, nCurRecLevel);

    if (NumDynLights <= 0 && arrDeferLights.Num() <= 0)
    {
        return;
    }

    // Find AABB of all nearest objects. Compute once for all lights as this can be slow
    AABB aabbCasters(AABB::RESET);
    m_RP.m_arrCustomShadowMapFrustumData[nThreadID].CoalesceMemory();
    size_t shadowMapArraySize = m_RP.m_arrCustomShadowMapFrustumData[nThreadID].size();
    for (size_t i = 0; i < shadowMapArraySize; ++i)
    {
        aabbCasters.Add(m_RP.m_arrCustomShadowMapFrustumData[nThreadID][i].aabb);
    }

    // AABBs are added in world space but without camera position applied
    aabbCasters.min += GetCamera().GetPosition();
    aabbCasters.max += GetCamera().GetPosition();

    // add nearest frustum if it has been set up
    for (int nLightID = 0; nLightID < NumDynLights; nLightID++)
    {
        SRenderLight* pLight =  &m_RP.m_DLights[nThreadID][nCurRecLevel][nLightID];

        if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS))
        {
            continue;
        }

        //shadows for nearest objects
        if (CV_r_DrawNearShadows && pLight->m_Flags & DLF_DIRECTIONAL)
        {
            int nStartIdx = SRendItem::m_StartFrust[nThreadID][nLightID];
            TArray<ShadowMapFrustum>& arrFrustums = m_RP.m_SMFrustums[nThreadID][nCurRecLevel];
            if (!arrFrustums.empty() && nStartIdx >= 0 && nStartIdx < arrFrustums.size())
            {
                //////////////////////////////////////////////////////////////////////////
                //prepare custom frustums for sun
                if (!m_RP.m_arrCustomShadowMapFrustumData[nThreadID].empty())
                {
                    //copy sun frustum
                    ShadowMapFrustum* pCustomFrustum = arrFrustums.AddIndex(1);
                    ShadowMapFrustum* pSunFrustum =  &arrFrustums[nStartIdx];
                    memcpy(pCustomFrustum, pSunFrustum, sizeof(ShadowMapFrustum));

                    const int nFrustumIndex = arrFrustums.Num() - 1;
                    m_RP.m_SMCustomFrustumIDs[nThreadID][nCurRecLevel].Add(nFrustumIndex);

                    pCustomFrustum->m_eFrustumType = ShadowMapFrustum::e_Nearest;
                    pCustomFrustum->bUseShadowsPool = false;
                    pCustomFrustum->bUseAdditiveBlending = true;
                    pCustomFrustum->fShadowFadingDist = 1.0f;
                    pCustomFrustum->fDepthConstBias = 0.0001f;

                    Matrix44A& mPrj = pCustomFrustum->mLightProjMatrix;
                    Matrix44A& mView = pCustomFrustum->mLightViewMatrix;
                    pCustomFrustum->aabbCasters = aabbCasters;
                    CShadowUtils::GetShadowMatrixForObject(mPrj, mView, pCustomFrustum);
                    pCustomFrustum->mLightViewMatrix = pCustomFrustum->mLightViewMatrix * pCustomFrustum->mLightProjMatrix;
                }
                //////////////////////////////////////////////////////////////////////////
            }
        }
    }

    // prepare depth maps for all custom frustums
    for (int32* pID = m_RP.m_SMCustomFrustumIDs[nThreadID][nCurRecLevel].begin(); pID != m_RP.m_SMCustomFrustumIDs[nThreadID][nCurRecLevel].end(); ++pID)
    {
        ShadowMapFrustum& curFrustum = m_RP.m_SMFrustums[nThreadID][nCurRecLevel][*pID];
        PrepareDepthMap(&curFrustum, curFrustum.m_eFrustumType == ShadowMapFrustum::e_Nearest ? -1 : *pID, true);
    }
}

void CD3D9Renderer::EF_PrepareAllDepthMaps()
{
    int nThreadID = m_RP.m_nProcessThreadID;
    int nCurRecLevel = SRendItem::m_RecurseLevel[nThreadID];
    assert(nCurRecLevel >= 0);

    int NumDynLights = m_RP.m_DLights[nThreadID][nCurRecLevel].Num();

    TArray<SRenderLight>&  arrDeferLights = CDeferredShading::Instance().GetLights(nThreadID, nCurRecLevel);

    if (NumDynLights <= 0 && arrDeferLights.Num() <= 0)
    {
        return;
    }

    bool haveShadows = false;
    for (int nLightID = 0; nLightID < NumDynLights; nLightID++)
    {
        SRenderLight* pLight =  &m_RP.m_DLights[nThreadID][SRendItem::m_RecurseLevel[nThreadID]][nLightID];

        if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS))
        {
            continue;
        }

        haveShadows |= FX_PrepareDepthMapsForLight(*pLight, nLightID);
    }

    if (!haveShadows && m_clearShadowMaskTexture)
    {
        FX_ClearShadowMaskTexture();
        m_clearShadowMaskTexture = false;
    }
    else
    {
        m_clearShadowMaskTexture = true;
    }

    //////////////////////////////////////////////////////////////////////////
    if IsCVarConstAccess(constexpr) (!CV_r_UseShadowsPool)
    {
        for (uint32 nDeferLightID = 0; nDeferLightID < arrDeferLights.Num(); nDeferLightID++)
        {
            SRenderLight* pLight =  &arrDeferLights[nDeferLightID];
            if (!(pLight->m_Flags & DLF_CASTSHADOW_MAPS))
            {
                continue;
            }

            int nDeferLightIdx = nDeferLightID + NumDynLights;
            assert((unsigned int) nDeferLightIdx < (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS));
            if ((unsigned int) nDeferLightIdx >= (MAX_REND_LIGHTS + MAX_DEFERRED_LIGHTS))
            {
                Warning("CD3D9Renderer::EF_PrepareAllDepthMaps: Too many light sources used ...");
                return;
            }

            FX_PrepareDepthMapsForLight(*pLight, nDeferLightIdx);
        }
    }

    // prepare custom depth maps
    {
        PROFILE_LABEL_SCOPE("CUSTOM MAPS");
        const uint64 nPrevRTFlags = m_RP.m_FlagsShader_RT;
        uint32 nPrefRendFlags = m_RP.m_nRendFlags;
        m_RP.m_nRendFlags = 0;

        EF_PrepareCustomShadowMaps();

        m_RP.m_nRendFlags = nPrefRendFlags;
        m_RP.m_FlagsShader_RT = nPrevRTFlags;
    }
}

void CD3D9Renderer::FX_MergeShadowMaps(ShadowMapFrustum* pDst, const ShadowMapFrustum* pSrc)
{
    if (!pDst || !pSrc)
    {
        return;
    }
    AZ_TRACE_METHOD();

    CRY_ASSERT(pSrc->m_eFrustumType == ShadowMapFrustum::e_GsmCached);
    CRY_ASSERT(pDst->m_eFrustumType == ShadowMapFrustum::e_GsmDynamicDistance);
    CRY_ASSERT(pDst->nShadowMapLod == pSrc->nShadowMapLod);

    const int nThreadID = m_RP.m_nProcessThreadID;
    const int nShadowRecur = pDst->nShadowGenID[nThreadID][0];
    const int nRendItemCount = nShadowRecur != 0xFFFFFFFF ? (SRendItem::m_ShadowsEndRI[nThreadID][nShadowRecur] - SRendItem::m_ShadowsStartRI[nThreadID][nShadowRecur]) : 0;

    // get crop rectangle for projection
    Matrix44r mReproj = Matrix44r(pDst->mLightViewMatrix).GetInverted() * Matrix44r(pSrc->mLightViewMatrix);
    Vec4r srcClipPosTL = Vec4r(-1, -1, 0, 1) * mReproj;
    srcClipPosTL /= srcClipPosTL.w;

    const float fSnap = 2.0f / pSrc->pDepthTex->GetWidth();
    Vec4 crop = Vec4(
            crop.x = fSnap * int(srcClipPosTL.x / fSnap),
            crop.y = fSnap * int(srcClipPosTL.y / fSnap),
            crop.z = 2.0f * pDst->nTextureWidth / float(pSrc->nTextureWidth),
            crop.w = 2.0f * pDst->nTextureHeight / float(pSrc->nTextureHeight)
            );

    Matrix44 cropMatrix(IDENTITY);
    cropMatrix.m00 = 2.f / crop.z;
    cropMatrix.m11 = 2.f / crop.w;
    cropMatrix.m30 = -(1.0f + cropMatrix.m00 * crop.x);
    cropMatrix.m31 = -(1.0f + cropMatrix.m11 * crop.y);

    const bool bOutsideFrustum = abs(crop.x) > 1.0f || abs(crop.x + crop.z) > 1.0f || abs(crop.y) > 1.0f || abs(crop.y + crop.w) > 1.0f;
    const bool bEmptyCachedFrustum = pSrc->nShadowGenMask == 0;
    const bool bRequireCopy = bOutsideFrustum || bEmptyCachedFrustum || nRendItemCount > 0;

    pDst->pDepthTex = NULL;
    pDst->bIncrementalUpdate = true;
    pDst->mLightViewMatrix = pSrc->mLightViewMatrix * cropMatrix;

    // do we need to merge static shadows into the dynamic shadow map?
    if (bRequireCopy)
    {

        SDynTexture_Shadow* pDynTex = SDynTexture_Shadow::GetForFrustum(pDst);
        pDst->pDepthTex = pDynTex->m_pTexture;

        SDepthTexture depthSurface;

        depthSurface.nWidth = pDst->nTextureWidth;
        depthSurface.nHeight = pDst->nTextureHeight;
        depthSurface.nFrameAccess = -1;
        depthSurface.bBusy = false;
        depthSurface.pTex = pDst->pDepthTex;
        depthSurface.pSurf = pDst->pDepthTex->GetDeviceDepthStencilSurf();
        depthSurface.pTarget = pDst->pDepthTex->GetDevTexture()->Get2DTexture();

        if (bEmptyCachedFrustum)
        {
            gcpRendD3D->FX_ClearTarget(&depthSurface, CLEAR_ZBUFFER | CLEAR_STENCIL, Clr_FarPlane.r, 0);
        }
        else
        {
            uint64 nSaveFlagsShader_RT = gRenDev->m_RP.m_FlagsShader_RT;

            int iTempX, iTempY, iWidth, iHeight;
            gRenDev->GetViewport(&iTempX, &iTempY, &iWidth, &iHeight);

            gcpRendD3D->FX_PushRenderTarget(0, (CTexture*)NULL, &depthSurface);
            gcpRendD3D->FX_SetActiveRenderTargets();
            gcpRendD3D->RT_SetViewport(0, 0,  pDst->pDepthTex->GetWidth(),  pDst->pDepthTex->GetHeight());

            FX_SetStencilDontCareActions(0, true, true);

            static CCryNameTSCRC pTechCopyShadowMap("ReprojectShadowMap");
            SPostEffectsUtils::ShBeginPass(CShaderMan::s_ShaderShadowMaskGen, pTechCopyShadowMap, FEF_DONTSETTEXTURES | FEF_DONTSETSTATES);

            gRenDev->FX_SetState(GS_DEPTHWRITE | GS_DEPTHFUNC_NOTEQUAL);

            Matrix44 mReprojDstToSrc = pDst->mLightViewMatrix.GetInverted() * pSrc->mLightViewMatrix;

            static CCryNameR paramReprojMatDstToSrc("g_mReprojDstToSrc");
            CShaderMan::s_ShaderShadowMaskGen->FXSetPSFloat(paramReprojMatDstToSrc, (Vec4*) mReprojDstToSrc.GetData(), 4);

            Matrix44 mReprojSrcToDst = pSrc->mLightViewMatrix.GetInverted() * pDst->mLightViewMatrix;
            static CCryNameR paramReprojMatSrcToDst("g_mReprojSrcToDst");
            CShaderMan::s_ShaderShadowMaskGen->FXSetPSFloat(paramReprojMatSrcToDst, (Vec4*) mReprojSrcToDst.GetData(), 4);

            pSrc->pDepthTex->Apply(0, CTexture::GetTexState(STexState(FILTER_POINT, true)));

            SPostEffectsUtils::DrawFullScreenTri(depthSurface.nWidth, depthSurface.nHeight);
            SPostEffectsUtils::ShEndPass();

            gcpRendD3D->FX_PopRenderTarget(0);
            gcpRendD3D->RT_SetViewport(iTempX, iTempY, iWidth, iHeight);

            gRenDev->m_RP.m_FlagsShader_RT = nSaveFlagsShader_RT;
        }

        pDst->packWidth[0] = pDst->nTextureWidth;
        pDst->packHeight[0] = pDst->nTextureHeight;

        pDst->packX[0] = pDst->packY[0] = 0;
    }
    else
    {
        pDst->packX[0] = int((crop.x * 0.5f + 0.5f) * pSrc->pDepthTex->GetWidth() + 0.5f);
        pDst->packY[0] = int((-(crop.y + crop.w) * 0.5f + 0.5f) * pSrc->pDepthTex->GetHeight() + 0.5f);
        pDst->packWidth[0]  = pDst->nTextureWidth;
        pDst->packHeight[0] = pDst->nTextureHeight;

        pDst->pDepthTex = pSrc->pDepthTex;
        pDst->nTexSize = pSrc->nTexSize;
        pDst->nTextureWidth = pSrc->nTextureWidth;
        pDst->nTextureHeight = pSrc->nTextureHeight;
    }

    pDst->fNearDist = pSrc->fNearDist;
    pDst->fFarDist = pSrc->fFarDist;
    pDst->fDepthConstBias = pSrc->fDepthConstBias;
    pDst->fDepthTestBias = pSrc->fDepthTestBias;
    pDst->fDepthSlopeBias = pSrc->fDepthSlopeBias;
}

void CD3D9Renderer::FX_ClearShadowMaskTexture()
{
    const int arraySize = CTexture::s_ptexShadowMask->StreamGetNumSlices();
    SResourceView curSliceRVDesc = SResourceView::RenderTargetView(CTexture::s_ptexShadowMask->GetTextureDstFormat(), 0, 1);
    for (int i = 0; i < arraySize; ++i)
    {
        curSliceRVDesc.m_Desc.nFirstSlice = i;
        SResourceView& firstSliceRV = CTexture::s_ptexShadowMask->GetResourceView(curSliceRVDesc);

#if defined(CRY_USE_METAL)
        static ICVar* pVar = iConsole->GetCVar("e_ShadowsClearShowMaskAtLoad");
        if (pVar && pVar->GetIVal())
        {
            FX_ClearTarget(static_cast<D3DSurface*>(firstSliceRV.m_pDeviceResourceView), Clr_Transparent, 0, nullptr);
            
            // On metal we have to submit a draw call in order for a clear to take affect.
            // Doing the commit/clear target region will produce the needed draw call for the clear.
            FX_PushRenderTarget(0, static_cast<D3DSurface*>(firstSliceRV.m_pDeviceResourceView), nullptr);
            
            RT_SetViewport(0, 0, CTexture::s_ptexShadowMask->GetWidth(), CTexture::s_ptexShadowMask->GetHeight());
            FX_Commit();
            FX_ClearTargetRegion();
            FX_PopRenderTarget(0);
        }
#else
        FX_ClearTarget(static_cast<D3DSurface*>(firstSliceRV.m_pDeviceResourceView), Clr_Transparent, 0, nullptr);
#endif
    }
}

