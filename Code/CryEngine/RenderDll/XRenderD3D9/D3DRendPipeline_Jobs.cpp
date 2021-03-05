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
#include <I3DEngine.h>
#include <IMovieSystem.h>
#include <CryHeaders.h>

#include "../Common/PostProcess/PostProcessUtils.h"
#include "D3DPostProcess.h"
#include "D3DStereo.h"
#include "D3DHWShader.h"

#include "Common/RenderView.h"

///////////////////////////////////////////////////////////////////////////////
void CRenderer::RegisterFinalizeShadowJobs(int nThreadID)
{
    // Init post job
    // legacy job priority: JobManager::eLowPriority
    m_generateShadowRendItemJobExecutor.SetPostJob(
        m_finalizeShadowRendItemsJobExecutor[nThreadID],
        [this, nThreadID]
        {
            this->FinalizeShadowRendItems(nThreadID);
        }
    );
    m_generateShadowRendItemJobExecutor.PushCompletionFence();
    
    m_finalizeShadowRendItemsJobExecutor[nThreadID].PushCompletionFence();
}

///////////////////////////////////////////////////////////////////////////////
// sort SRendItem lists
void CD3D9Renderer::EF_SortRenderList(int nList, int nAW, SRenderListDesc* pRLD, int nThread, bool bUseDist)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

    int nStart = pRLD->m_nStartRI[nAW][nList];
    int nEnd   = pRLD->m_nEndRI[nAW][nList];
    int n = nEnd - nStart;

    if (!n)
    {
        return;
    }

    auto& renderItems = CRenderView::GetRenderViewForThread(nThread)->GetRenderItems(nAW, nList);
    switch (nList)
    {
    case EFSLIST_PREPROCESS:
    {
        PROFILE_FRAME(State_SortingPre);
        SRendItem::mfSortPreprocess(&renderItems[nStart], n);
    }
    break;

    case EFSLIST_DEFERRED_PREPROCESS:
    case EFSLIST_HDRPOSTPROCESS:
    case EFSLIST_POSTPROCESS:
    case EFSLIST_FOG_VOLUME:
        break;

    case EFSLIST_WATER_VOLUMES:
    case EFSLIST_REFRACTIVE_SURFACE:
    case EFSLIST_TRANSP:
    case EFSLIST_WATER:
    case EFSLIST_HALFRES_PARTICLES:
    case EFSLIST_LENSOPTICS:
    case EFSLIST_EYE_OVERLAY:
    {
        PROFILE_FRAME(State_SortingDist);
        SRendItem::mfSortByDist(&renderItems[nStart], n, false);
    }
    break;
    case EFSLIST_DECAL:
    {
        PROFILE_FRAME(State_SortingDecals);
        SRendItem::mfSortByDist(&renderItems[nStart], n, true);
    }
    break;

    case EFSLIST_GENERAL:
    case EFSLIST_SKIN:
    {
        if (bUseDist && m_bUseGPUFriendlyBatching[nThread])
        {
            PROFILE_FRAME(State_SortingZPass);
            if IsCVarConstAccess(constexpr) (CRenderer::CV_r_ZPassDepthSorting == 1)
            {
                SRendItem::mfSortForZPass(&renderItems[nStart], n);
            }
            else if IsCVarConstAccess(constexpr) (CRenderer::CV_r_ZPassDepthSorting == 2)
            {
                SRendItem::mfSortByDist(&renderItems[nStart], n, false, true);
            }
        }
        else
        {
            PROFILE_FRAME(State_SortingLight);
            SRendItem::mfSortByLight(&renderItems[nStart], n, true, false, false);
        }
    }
    break;

    case EFSLIST_AFTER_POSTPROCESS:
    case EFSLIST_AFTER_HDRPOSTPROCESS:
    {
        PROFILE_FRAME(State_SortingLight);
        SRendItem::mfSortByLight(&renderItems[nStart], n, true, false, nList == EFSLIST_DECAL);
    }
    break;
    case EFSLIST_GPU_PARTICLE_CUBEMAP_COLLISION:
        break;

    default:
        AZ_Assert(0, "Not handled");
    }
}

///////////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::EF_SortRenderLists(SRenderListDesc* pRLD, int nThreadID, bool bUseDist, bool bUseJobSystem)
{
    PROFILE_FRAME(Sort_Lists);

    int i, j;
    for (j = 0; j < MAX_LIST_ORDER; j++)
    {
        for (i = 1; i < EFSLIST_NUM; i++)
        {
            // EFSLIST_SHADOW_GEN is handled in Shadow Pass
            // EFSLIST_PREPROCESS, EFSLIST_WATER, EFSLIST_WATER_VOLUMES are handled in EF_ProcessRenderLists
            if (i == EFSLIST_SHADOW_GEN || i == EFSLIST_PREPROCESS || i == EFSLIST_WATER || i == EFSLIST_WATER_VOLUMES)
            {
                continue;
            }
            if (bUseJobSystem)
            {
                int nStart = pRLD->m_nStartRI[j][i];
                int nEnd   = pRLD->m_nEndRI[j][i];

                if ((nEnd - nStart) > 0)
                {
                    // legacy job priority: JobManager::eHighPriority
                    m_finalizeRendItemsJobExecutor[nThreadID].StartJob(
                        [this, i, j, pRLD, nThreadID, bUseDist]
                        {
                            this->EF_SortRenderList(i, j, pRLD, nThreadID, bUseDist);
                        }
                    );
                }
            }
            else
            {
                EF_SortRenderList(i, j, pRLD, nThreadID, bUseDist);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::BeginSpawningGeneratingRendItemJobs(int nThreadID)
{
    AZ_TRACE_METHOD();

    // Register post job
    // legacy job priority: JobManager::eHighPriority
    m_generateRendItemJobExecutor.SetPostJob(
        m_finalizeRendItemsJobExecutor[nThreadID],
        [this, nThreadID]
        {
            this->FinalizeRendItems(nThreadID);
        }
    );

    // Push completion fences accross all groups to prevent false (race-condition) reports of "completion" before all jobs are started.  These are then popped after we are done starting jobs.
    // there will be decreased again after the main thread has passed all job creating parts
    m_generateRendItemPreProcessJobExecutor.PushCompletionFence();
    m_generateRendItemJobExecutor.PushCompletionFence();
    m_finalizeRendItemsJobExecutor[nThreadID].PushCompletionFence();
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::BeginSpawningShadowGeneratingRendItemJobs(int nThreadID)
{
    RegisterFinalizeShadowJobs(nThreadID);
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::EndSpawningGeneratingRendItemJobs()
{
    m_generateRendItemJobExecutor.PopCompletionFence();
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::FinalizeRendItems(int nThreadID)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

    FinalizeRendItems_ReorderRendItems(nThreadID);
    FinalizeRendItems_SortRenderLists(nThreadID);

    m_finalizeRendItemsJobExecutor[nThreadID].PopCompletionFence();
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::FinalizeShadowRendItems(int nThreadID)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

    FinalizeRendItems_ReorderShadowRendItems(nThreadID);
    FinalizeRendItems_FindShadowFrustums(nThreadID);

    m_finalizeShadowRendItemsJobExecutor[nThreadID].PopCompletionFence();
}


///////////////////////////////////////////////////////////////////////////////
void CRenderer::FinalizeRendItems_ReorderRendItems(int nThreadID)
{
    ////////////////////////////////////////////////
    // reoder item lists by their recursive pass
    for (int j = 0; j < MAX_LIST_ORDER; j++)
    {
        for (int i = 0; i < EFSLIST_NUM; i++)
        {
            // shadows don't need sorting, and PREPROCESS is sorted in mainthread
            if (i == EFSLIST_SHADOW_GEN || i == EFSLIST_PREPROCESS || i == EFSLIST_WATER || i == EFSLIST_WATER_VOLUMES)
            {
                continue;
            }

            FinalizeRendItems_ReorderRendItemList(j, i, nThreadID);
        }
    }
}

void CRenderer::FinalizeRendItems_ReorderRendItemList(int nAW, int nList, int nThreadID)
{
    CRenderView* pRenderView = gRenDev->GetRenderViewForThread(nThreadID);
    SRenderListDesc* __restrict pGeneralPassRLD        = &pRenderView->m_RenderListDesc[0];
    SRenderListDesc* __restrict pRecursivePassRLD = &pRenderView->m_RenderListDesc[1];

    auto& rRendItems = pRenderView->GetRenderItems(nAW, nList);

    size_t nRendItemsSize = rRendItems.size();
    if (nRendItemsSize)
    {
        std::sort(&rRendItems[0], &rRendItems[0] + nRendItemsSize, SCompareByOnlyStableFlagsOctreeID());
    }

    pGeneralPassRLD->m_nStartRI[nAW][nList] = 0;
    pGeneralPassRLD->m_nEndRI[nAW][nList] = 0;
    pRecursivePassRLD->m_nStartRI[nAW][nList] = 0;
    pRecursivePassRLD->m_nEndRI[nAW][nList] = 0;

    for (size_t i = 0; i < nRendItemsSize; ++i)
    {
        if (rRendItems[i].rendItemSorter.IsRecursivePass())
        {
            pGeneralPassRLD->m_nEndRI[nAW][nList] = i;
            pRecursivePassRLD->m_nStartRI[nAW][nList] = i;
            break;
        }
    }

    if (pRecursivePassRLD->m_nStartRI[nAW][nList])
    {
        pRecursivePassRLD->m_nEndRI[nAW][nList] = nRendItemsSize;
    }
    else
    {
        pGeneralPassRLD->m_nEndRI[nAW][nList] = nRendItemsSize;
    }
    pGeneralPassRLD->m_nBatchFlags[nAW][nList] = pRenderView->GetBatchFlags(0, nAW, nList);
    pRecursivePassRLD->m_nBatchFlags[nAW][nList] = pRenderView->GetBatchFlags(1, nAW, nList);

    AZ_Assert(pGeneralPassRLD->m_nEndRI[nAW][nList] - pGeneralPassRLD->m_nStartRI[nAW][nList] >= 0, "EndRI has to be bigger than StartRI");
    AZ_Assert(pRecursivePassRLD->m_nEndRI[nAW][nList] - pRecursivePassRLD->m_nStartRI[nAW][nList] >= 0, "EndRI has to be bigger than StartRI");
}

///////////////////////////////////////////////////////////////////////////////
void CRenderer::FinalizeRendItems_SortRenderLists(int nThreadID)
{
    ////////////////////////////////////////////////
    // do the actual sorting
    CRenderView* pRenderView = m_RP.m_pRenderViews[nThreadID].get();
    for (int k = 0; k < 2; k++)
    {
        SRenderListDesc* pCurRLD = &pRenderView->m_RenderListDesc[k];
        static_cast<CD3D9Renderer*>(this)->EF_SortRenderLists(pCurRLD, nThreadID, (CRenderer::CV_r_ZPassDepthSorting != 0), true);
    }
}

///////////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::InvokeShadowMapRenderJobs(ShadowMapFrustum* pCurFrustum, const SRenderingPassInfo& passInfo)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

    for (int i = 0; i < pCurFrustum->m_jobExecutedCastersList.Count(); ++i)
    {
        IShadowCaster* pEnt  = pCurFrustum->m_jobExecutedCastersList[i];

        //TOFIX reactivate OmniDirectionalShadow
        if (pCurFrustum->bOmniDirectionalShadow)
        {
            AABB aabb;
            pEnt->FillBBox(aabb);
            //!!! Reactivate proper camera
            bool bVis = passInfo.GetCamera().IsAABBVisible_F(aabb);
            if (!bVis)
            {
                continue;
            }
        }

        if ((pCurFrustum->m_Flags & DLF_DIFFUSEOCCLUSION) && pEnt->HasOcclusionmap(0, pCurFrustum->pLightOwner))
        {
            continue;
        }

        // all not yet to jobs ported types need to be processed by mainthread
        gEnv->p3DEngine->RenderRenderNode_ShadowPass(pEnt, passInfo, gRenDev->GetGenerateShadowRendItemJobExecutor());
    }
}

///////////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::StartInvokeShadowMapRenderJobs(ShadowMapFrustum* pCurFrustum, const SRenderingPassInfo& passInfo)
{
    // legacy job priority: JobManager::eLowPriority
    gRenDev->GetGenerateShadowRendItemJobExecutor()->StartJob(
        [this, pCurFrustum, passInfo]
        {
            this->InvokeShadowMapRenderJobs(pCurFrustum, passInfo);
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
void CD3D9Renderer::WaitForParticleBuffer(threadID nThreadId)
{
    FUNCTION_PROFILER_LEGACYONLY(gEnv->pSystem, PROFILE_PARTICLE)
    AZ_TRACE_METHOD();
    SRenderPipeline& rp = gRenDev->m_RP;

    if (rp.m_pParticleVertexBuffer[nThreadId])
    {
        rp.m_pParticleVertexBuffer[nThreadId]->WaitForFence();
    }
    if (rp.m_pParticleIndexBuffer[nThreadId])
    {
        rp.m_pParticleIndexBuffer[nThreadId]->WaitForFence();
    }
}
///////////////////////////////////////////////////////////////////////////////
