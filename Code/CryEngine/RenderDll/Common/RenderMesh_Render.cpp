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
#include <AzCore/Debug/Profiler.h>

#include "RenderMesh.h"

#include "PostProcess/PostEffects.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
namespace
{
    inline uint32 GetCurrentRenderFrameID(const SRenderingPassInfo& passInfo)
    {
        return gRenDev->m_RP.m_TI[passInfo.ThreadID()].m_nFrameUpdateID;
    };
}


///////////////////////////////////////////////////////////////////////////////
void CRenderMesh::Render(CRenderObject* pObj, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
{
    _smart_ptr<IMaterial> pMaterial = pObj->m_pCurrMaterial;

    if (!pMaterial || !m_nVerts || !m_nInds || m_Chunks.empty() || (m_nFlags & FRM_ALLOCFAILURE) != 0)
    {
        return;
    }

    FUNCTION_PROFILER_FAST(GetISystem(), PROFILE_RENDERER, g_bProfilerEnabled);

    IF (!CanRender(), 0)
    {
        return;
    }

    CRenderer*  __restrict rd = gRenDev;
    bool bSkinned = (GetChunksSkinned().size() && (pObj->m_ObjFlags & (FOB_SKINNED)));

    uint64 nMeshSubSetMask = 0;
#if !defined(_RELEASE)
    const char* szExcl = CRenderer::CV_r_excludemesh->GetString();
    if (szExcl[0] && m_sSource)
    {
        char szMesh[1024];
        cry_strcpy(szMesh, this->m_sSource);
        azstrlwr(szMesh, AZ_ARRAY_SIZE(szMesh));
        if (szExcl[0] == '!')
        {
            if (!strstr(&szExcl[1], m_sSource))
            {
                return;
            }
        }
        else
        if (strstr(szExcl, m_sSource))
        {
            return;
        }
    }
#endif

    if (rd->m_pDefaultMaterial && pMaterial)
    {
        pMaterial = rd->m_pDefaultMaterial;
    }

    assert(pMaterial);

    if (!pMaterial)
    {
        return;
    }

    m_nLastRenderFrameID = GetCurrentRenderFrameID(passInfo);

    //////////////////////////////////////////////////////////////////////////
    if (!m_meshSubSetIndices.empty() && abs((int)m_nLastRenderFrameID - (int)m_nLastSubsetGCRenderFrameID) > DELETE_SUBSET_MESHES_AFTER_NOTUSED_FRAMES)
    {
        m_deferredSubsetGarbageCollection[passInfo.ThreadID()].push_back(this);
    }

    //////////////////////////////////////////////////////////////////////////
    bool bRenderBreakableWithMultipleDrawCalls = false;
    if (pObj->m_ObjFlags & FOB_MESH_SUBSET_INDICES && m_nVerts >= 3)
    {
        SRenderObjData* pOD = pObj->GetObjData();
        if (pOD)
        {
            if (pOD->m_nSubObjHideMask != 0)
            {
                IRenderMesh* pRM = GetRenderMeshForSubsetMask(pOD, pOD->m_nSubObjHideMask, pMaterial, passInfo);
                // if pRM is null, it means that this subset rendermesh is not computed yet, thus we render it with multiple draw calls
                if (pRM)
                {
                    static_cast<CRenderMesh*>(pRM)->CRenderMesh::Render(pObj, passInfo, rendItemSorter);
                    pOD->m_nSubObjHideMask = 0;
                    return;
                }
                // compute the needed mask
                const uint32 ni = m_ChunksSubObjects.size();
                nMeshSubSetMask = pOD->m_nSubObjHideMask & (((uint64)1 << ni) - 1);
                pOD->m_nSubObjHideMask = 0;
                bRenderBreakableWithMultipleDrawCalls = true;
            }
        }
    }

    int nList = EFSLIST_GENERAL;

    const int nAW = (pObj->m_ObjFlags & FOB_AFTER_WATER) || (pObj->m_ObjFlags & FOB_NEAREST) ? 1 : 0;

    //////////////
    if (gRenDev->CV_r_MotionVectors && passInfo.IsGeneralPass() && ((pObj->m_ObjFlags & FOB_DYNAMIC_OBJECT) != 0))
    {
        CMotionBlur::SetupObject(pObj, passInfo);
    }

    TRenderChunkArray* pChunks = bSkinned ? &m_ChunksSkinned : &m_Chunks;
    // for rendering with multiple drawcalls, use ChunksSubObjects
    if (bRenderBreakableWithMultipleDrawCalls)
    {
        pChunks = &m_ChunksSubObjects;
    }

    const uint32 ni = (uint32)pChunks->size();

    CRenderChunk* pPrevChunk = NULL;
    for (uint32 i = 0; i < ni; i++)
    {
        CRenderChunk* pChunk = &pChunks->at(i);
        CRendElementBase*  __restrict pREMesh = pChunk->pRE;

        SShaderItem& ShaderItem         = pMaterial->GetShaderItem(pChunk->m_nMatID);
        CShaderResources* pR  = (CShaderResources*)ShaderItem.m_pShaderResources;
        CShader*  __restrict pS        = (CShader*)ShaderItem.m_pShader;

        if (pR && pR->IsDeforming())
        {
            pObj->m_ObjFlags |= FOB_MOTION_BLUR;
        }

        // don't render this chunk if the hide mask for it is set
        if (bRenderBreakableWithMultipleDrawCalls && (nMeshSubSetMask & ((uint64)1 << pChunk->nSubObjectIndex)))
        {
            goto SkipChunk;
        }

        if (pREMesh == NULL || pS == NULL || pR == NULL)
        {
            goto SkipChunk;
        }

        if (pS->m_Flags2 & EF2_NODRAW)
        {
            goto SkipChunk;
        }

        if (passInfo.IsShadowPass() && (pR->m_ResFlags & MTL_FLAG_NOSHADOW))
        {
            goto SkipChunk;
        }

        if (passInfo.IsShadowPass() && !passInfo.IsDisableRenderChunkMerge() && CRenderMesh::RenderChunkMergeAbleInShadowPass(pPrevChunk, pChunk, pMaterial))
        {
            continue; // skip the merged chunk, but keep the PrevChunkReference for further merging
        }
        PrefetchLine(pREMesh, 0);
        PrefetchLine(pObj, 0);
        rd->EF_AddEf_NotVirtual(pREMesh, ShaderItem, pObj, passInfo, nList, nAW, rendItemSorter);

        pPrevChunk = pChunk;
        continue;
SkipChunk:
        pPrevChunk = NULL;
    }
}

void CRenderMesh::AddShadowPassMergedChunkIndicesAndVertices(CRenderChunk* pCurrentChunk, _smart_ptr<IMaterial> pMaterial, int& rNumVertices, int& rNumIndices)
{
    if (m_Chunks.size() == 0)
    {
        return;
    }

    if (gRenDev->m_RP.m_pCurObject->m_ObjFlags & (FOB_SKINNED))
    {
        return;
    }

    if (pMaterial == NULL)
    {
        return;
    }

    AUTO_LOCK(pMaterial->GetSubMaterialResizeLock());
    for (uint32 i = (uint32)(pCurrentChunk - &m_Chunks[0]) + 1; i < (uint32)m_Chunks.size(); ++i)
    {
        if (!CRenderMesh::RenderChunkMergeAbleInShadowPass(pCurrentChunk, &m_Chunks[i], pMaterial))
        {
            return;
        }

        rNumVertices += m_Chunks[i].nNumVerts;
        rNumIndices += m_Chunks[i].nNumIndices;
    }
}

bool CRenderMesh::RenderChunkMergeAbleInShadowPass(CRenderChunk* pPreviousChunk, CRenderChunk* pCurrentChunk, _smart_ptr<IMaterial> pMaterial)
{
    if IsCVarConstAccess(constexpr) (!CRenderer::CV_r_MergeShadowDrawcalls)
    {
        return false;
    }

    if (pPreviousChunk == NULL || pCurrentChunk == NULL || pMaterial == NULL)
    {
        return false;
    }

    SShaderItem& rCurrentShaderItem     = pMaterial->GetShaderItem(pCurrentChunk->m_nMatID);
    SShaderItem& rPreviousShaderItem    = pMaterial->GetShaderItem(pPreviousChunk->m_nMatID);

    CShaderResources* pCurrentShaderResource  = (CShaderResources*)rCurrentShaderItem.m_pShaderResources;
    CShaderResources* pPreviousShaderResource = (CShaderResources*)rPreviousShaderItem.m_pShaderResources;

    CShader* pCurrentShader       = (CShader*)rCurrentShaderItem.m_pShader;
    CShader* pPreviousShader    = (CShader*)rPreviousShaderItem.m_pShader;

    if (!pCurrentShaderResource || !pPreviousShaderResource || !pCurrentShader || !pPreviousShader)
    {
        return false;
    }

    bool bCurrentAlphaTested        = pCurrentShaderResource->CShaderResources::IsAlphaTested();
    bool bPreviousAlphaTested       = pPreviousShaderResource->CShaderResources::IsAlphaTested();

    if (bCurrentAlphaTested != bPreviousAlphaTested)
    {
        return false;
    }

    if (bCurrentAlphaTested)
    {
        const SEfResTexture* pCurrentResTex =  pCurrentShaderResource->GetTextureResource(EFTT_DIFFUSE);
        const SEfResTexture* pPreviousResTex =  pPreviousShaderResource->GetTextureResource(EFTT_DIFFUSE);

        const CTexture* pCurrentDiffuseTex  = pCurrentResTex ? pCurrentResTex->m_Sampler.m_pTex : nullptr;
        const CTexture* pPreviousDiffuseTex = pPreviousResTex ? pPreviousResTex->m_Sampler.m_pTex : nullptr;

        if (pCurrentDiffuseTex != pPreviousDiffuseTex)
        {
            return false;
        }
    }

    if (((pPreviousShaderResource->m_ResFlags & MTL_FLAG_NOSHADOW) != 0) || ((pCurrentShaderResource->m_ResFlags & MTL_FLAG_NOSHADOW) != 0))
    {
        return false;
    }

    if ((pPreviousShaderResource->m_ResFlags & MTL_FLAG_2SIDED) != (pCurrentShaderResource->m_ResFlags & MTL_FLAG_2SIDED))
    {
        return false;
    }

    if (((pPreviousShader->m_Flags & EF_NODRAW) != 0) || ((pCurrentShader->m_Flags & EF_NODRAW) != 0))
    {
        return false;
    }

    return true;
}

// break-ability support
IRenderMesh* CRenderMesh::GetRenderMeshForSubsetMask([[maybe_unused]] SRenderObjData* pOD, uint64 nMeshSubSetMask, _smart_ptr<IMaterial> pMaterial, const SRenderingPassInfo& passInfo)
{
    // TODO: If only one bit is set in mask - there is no need to build new index buffer - small part of main index buffer can be re-used
    // TODO: Add auto releasing of not used for long time index buffers
    // TODO: Take into account those induces when computing render mesh memory size for CGF streaming
    // TODO: Support for multiple materials

    assert(nMeshSubSetMask != 0);

    IRenderMesh* pSrcRM = this;
    TRenderChunkArray& renderChunks = m_ChunksSubObjects;

    uint32 nChunkCount = renderChunks.size();
    nMeshSubSetMask &= (((uint64)1 << nChunkCount) - 1);

    // try to find the index mesh in the already finished list
    const MeshSubSetIndices::iterator meshSubSet = m_meshSubSetIndices.find(nMeshSubSetMask);

    if (meshSubSet != m_meshSubSetIndices.end())
    {
        return meshSubSet->second;
    }

    // subset mesh was not found, start job to create one
    SMeshSubSetIndicesJobEntry* pSubSetJob = m_meshSubSetRenderMeshJobs[passInfo.ThreadID()].push_back_new();
    pSubSetJob->m_pSrcRM = pSrcRM;
    pSubSetJob->m_pIndexRM = NULL;
    pSubSetJob->m_nMeshSubSetMask = nMeshSubSetMask;

    pSubSetJob->jobExecutor.StartJob([pSubSetJob]()
    {
        pSubSetJob->CreateSubSetRenderMesh();
    }); // Legacy JobManager used SJobState::SetBlocking

    return NULL;
}

void CRenderMesh::FinalizeRendItems(int nThreadID)
{
    // perform all requiered garbage collections
    m_deferredSubsetGarbageCollection[nThreadID].CoalesceMemory();
    for (size_t i = 0; i < m_deferredSubsetGarbageCollection[nThreadID].size(); ++i)
    {
        if (m_deferredSubsetGarbageCollection[nThreadID][i])
        {
            m_deferredSubsetGarbageCollection[nThreadID][i]->GarbageCollectSubsetRenderMeshes();
        }
    }
    m_deferredSubsetGarbageCollection[nThreadID].resize(0);

    // add all newly generated subset meshes
    bool bJobsStillRunning = false;
    size_t nNumSubSetRenderMeshJobs = m_meshSubSetRenderMeshJobs[nThreadID].size();
    for (size_t i = 0; i < nNumSubSetRenderMeshJobs; ++i)
    {
        SMeshSubSetIndicesJobEntry& rSubSetJob = m_meshSubSetRenderMeshJobs[nThreadID][i];
        if (rSubSetJob.jobExecutor.IsRunning())
        {
            bJobsStillRunning = true;
        }
        else if (rSubSetJob.m_pSrcRM) // finished job, which needs to be assigned
        {
            CRenderMesh* pSrcMesh = static_cast<CRenderMesh*>(rSubSetJob.m_pSrcRM.get());
            // check that we didn't create the same subset mesh twice, if we did, clean up the duplicate
            if (pSrcMesh->m_meshSubSetIndices.find(rSubSetJob.m_nMeshSubSetMask) == pSrcMesh->m_meshSubSetIndices.end())
            {
                pSrcMesh->m_meshSubSetIndices.insert(std::make_pair(rSubSetJob.m_nMeshSubSetMask, rSubSetJob.m_pIndexRM));
            }

            // mark job as assigned
            rSubSetJob.m_pIndexRM = NULL;
            rSubSetJob.m_pSrcRM = NULL;
        }
    }
    if (!bJobsStillRunning)
    {
        m_meshSubSetRenderMeshJobs[nThreadID].resize(0);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void CRenderMesh::ClearJobResources()
{
    for (int i = 0; i < RT_COMMAND_BUF_COUNT; ++i)
    {
        for (size_t j = 0; j < m_deferredSubsetGarbageCollection[i].size(); ++j)
        {
            if (m_deferredSubsetGarbageCollection[i][j])
            {
                m_deferredSubsetGarbageCollection[i][j]->GarbageCollectSubsetRenderMeshes();
            }
        }
        stl::free_container(m_deferredSubsetGarbageCollection[i]);

        for (size_t j = 0; j < m_deferredSubsetGarbageCollection[i].size(); ++j)
        {
            m_meshSubSetRenderMeshJobs[i][j].jobExecutor.WaitForCompletion();
        }
        stl::free_container(m_meshSubSetRenderMeshJobs[i]);
    }
}

void SMeshSubSetIndicesJobEntry::CreateSubSetRenderMesh()
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Renderer);

    CRenderMesh* pSrcMesh = static_cast<CRenderMesh*>(m_pSrcRM.get());

    TRenderChunkArray& renderChunks = pSrcMesh->m_ChunksSubObjects;
    uint32 nChunkCount = renderChunks.size();


    pSrcMesh->LockForThreadAccess();
    if (vtx_idx* pInds = pSrcMesh->GetIndexPtr(FSL_READ))
    {
        TRenderChunkArray newChunks;
        newChunks.reserve(3);

        int nMatId = -1;
        PodArray<vtx_idx> lstIndices;
        for (uint32 c = 0; c < nChunkCount; c++)
        {
            CRenderChunk& srcChunk = renderChunks[c];
            if (0 == (m_nMeshSubSetMask & ((uint64)1 << srcChunk.nSubObjectIndex)))
            {
                uint32 nLastIndex = lstIndices.size();
                lstIndices.AddList(&pInds[srcChunk.nFirstIndexId], srcChunk.nNumIndices);
                if (newChunks.empty() || nMatId != srcChunk.m_nMatID)
                {
                    // New chunk needed.
                    newChunks.push_back(srcChunk);
                    newChunks.back().nFirstIndexId = nLastIndex;
                    newChunks.back().nNumIndices = 0;
                    newChunks.back().nNumVerts = 0;
                    newChunks.back().pRE = 0;
                }
                nMatId = srcChunk.m_nMatID;
                newChunks.back().nNumIndices += srcChunk.nNumIndices;
                newChunks.back().nNumVerts = max((int)srcChunk.nFirstVertId + (int)srcChunk.nNumVerts - (int)newChunks.back().nFirstVertId, (int)newChunks.back().nNumVerts);
            }
        }
        pSrcMesh->UnLockForThreadAccess();

        IRenderMesh::SInitParamerers params;
        SVF_P3S_C4B_T2S tempVertex;
        params.pVertBuffer = &tempVertex;
        params.nVertexCount = 1;
        params.vertexFormat = eVF_P3S_C4B_T2S;
        params.pIndices = lstIndices.GetElements();
        params.nIndexCount = lstIndices.Count();
        params.nPrimetiveType = prtTriangleList;
        params.eType = eRMT_Static;
        params.nRenderChunkCount = 1;
        params.bOnlyVideoBuffer = false;
        params.bPrecache = false;
        _smart_ptr<IRenderMesh> pIndexMesh = gRenDev->CreateRenderMesh(pSrcMesh->m_sType, pSrcMesh->m_sSource, &params);
        pIndexMesh->SetVertexContainer(pSrcMesh);
        if (!newChunks.empty())
        {
            pIndexMesh->SetRenderChunks(&newChunks.front(), newChunks.size(), false);
            pIndexMesh->SetBBox(pSrcMesh->m_vBoxMin, pSrcMesh->m_vBoxMax);
        }
        m_pIndexRM = pIndexMesh;
    }
}

