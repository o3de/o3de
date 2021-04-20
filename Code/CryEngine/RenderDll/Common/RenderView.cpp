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
#include "RenderView.h"

CRenderView::CRenderView()
{
    InitRenderItems();
}

CRenderView::~CRenderView()
{
    FreeRenderItems();
}

void CRenderView::ClearRenderItems()
{

    for (int i = 0; i < MAX_LIST_ORDER; i++)
    {
        for (int j = 0; j < EFSLIST_NUM; j++)
        {
            m_renderItems[i][j].clear();
        }
    }
}

void CRenderView::FreeRenderItems()
{
    for (int i = 0; i < MAX_LIST_ORDER; i++)
    {
        for (int j = 0; j < EFSLIST_NUM; j++)
        {
            m_renderItems[i][j].resize(0);
        }
    }
}

void CRenderView::InitRenderItems()
{
    threadID nThreadId = CryGetCurrentThreadId();

    for (int i = 0; i < MAX_LIST_ORDER; i++)
    {
        for (int j = 0; j < EFSLIST_NUM; j++)
        {
            m_renderItems[i][j].Init();
            m_renderItems[i][j].SetNonWorkerThreadID(nThreadId);
        }
    }
}

void CRenderView::PrepareForRendering()
{
    for (int i = 0; i < MAX_LIST_ORDER; i++)
    {
        for (int j = 0; j < EFSLIST_NUM; j++)
        {
            //m_renderItems[i][j].CoalesceMemory();
            //m_RenderListDesc[0].m_nEndRI[i][j] = m_renderItems[i][j].size();
        }
    }
    //ClearBatchFlags();
}

//////////////////////////////////////////////////////////////////////////
void CRenderView::PrepareForWriting()
{
    // Clear batch flags.
    ZeroStruct(m_BatchFlags);
}

//////////////////////////////////////////////////////////////////////////
CThreadSafeWorkerContainer<SRendItem>& CRenderView::GetRenderItems(int nAfterWater, int nRenderList)
{
    m_renderItems[nAfterWater][nRenderList].CoalesceMemory();
    return m_renderItems[nAfterWater][nRenderList];
}

uint32 CRenderView::GetBatchFlags(int recusrion, int nAfterWater, int nRenderList) const
{
    return m_BatchFlags[recusrion][nAfterWater][nRenderList];
}

//////////////////////////////////////////////////////////////////////////
void CRenderView::AddRenderItem(IRenderElement* pElem, CRenderObject* RESTRICT_POINTER pObj, const SShaderItem& pSH,
    uint32 nList, int nAafterWater, uint32 nBatchFlags, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
{
    nBatchFlags |= (pSH.m_nPreprocessFlags & FSPR_MASK);

    size_t nIndex = ~0;
    SRendItem* ri = m_renderItems[nAafterWater][nList].push_back_new(nIndex);

    ri->pObj = pObj;

    ri->nOcclQuery = SRendItem::kOcclQueryInvalid;
    if (nList == EFSLIST_TRANSP || nList == EFSLIST_HALFRES_PARTICLES)
    {
        ri->fDist = pObj->m_fDistance + pObj->m_fSort;
    }
    else
    {
        ri->ObjSort = (pObj->m_ObjFlags & 0xffff0000) | pObj->m_nSort;
    }
    ri->nBatchFlags = nBatchFlags;
    ri->nStencRef = pObj->m_nClipVolumeStencilRef + 1; // + 1, we start at 1. 0 is reserved for MSAAed areas.

    ri->rendItemSorter = rendItemSorter;

    CShader* pShader = (CShader*)pSH.m_pShader;
    uint32 nResID = pSH.m_pShaderResources ? ((CShaderResources*)(pSH.m_pShaderResources))->m_Id : 0;
    assert(nResID < CShader::s_ShaderResources_known.size());

    ri->SortVal = (nResID << 18) | (pShader->mfGetID() << 6) | (pSH.m_nTechnique & 0x3f);
    ri->pElem = pElem;

    // add flags atomically, if this one is not set already
    volatile LONG* pBatchFlags = reinterpret_cast<volatile LONG*>(&m_BatchFlags[passInfo.GetRecursiveLevel()][nAafterWater][nList]);
    CryInterlockedOr(pBatchFlags, nBatchFlags);

    // update shadow pass flags if needed
    if (nList == EFSLIST_SHADOW_GEN && !(nBatchFlags & FB_IGNORE_SG_MASK))
    {
        volatile LONG* pShadowGenMask = reinterpret_cast<volatile LONG*>(passInfo.ShadowGenMaskAddress());
        uint32 nOrFlags = 1 << passInfo.ShadowFrustumSide();
        CryInterlockedOr(pShadowGenMask, nOrFlags);
    }
}
