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

// Description : Backend part of geometry cache rendering


#include "RenderDll_precompiled.h"

#include <Common/RendererDefs.h>
#include <Cry_Math.h>

#if defined(USE_GEOM_CACHES)

#include "RendElement.h"
#include "CREGeomCache.h"
#include "I3DEngine.h"
#include "../Renderer.h"
#include "../Common/PostProcess/PostEffects.h"

StaticInstance<CREGeomCache::UpdateList> CREGeomCache::sm_updateList[2];

CREGeomCache::CREGeomCache()
 : m_geomCacheVertexFormat(eVF_P3F_C4B_T2F)
{
    m_bUpdateFrame[0] = false;
    m_bUpdateFrame[1] = false;
    m_transformUpdateState[0] = 0;
    m_transformUpdateState[1] = 0;

    mfSetType(eDATA_GeomCache);
    mfUpdateFlags(FCEF_TRANSFORM);
}

CREGeomCache::~CREGeomCache()
{
    CryAutoLock<CryCriticalSection> lock1(sm_updateList[0]->m_mutex);
    CryAutoLock<CryCriticalSection> lock2(sm_updateList[1]->m_mutex);

    stl::find_and_erase(sm_updateList[0]->m_geoms, this);
    stl::find_and_erase(sm_updateList[1]->m_geoms, this);
}

void CREGeomCache::InitializeRenderElement(const uint numMeshes, _smart_ptr<IRenderMesh>* pMeshes, uint16 materialId)
{
    m_bUpdateFrame[0] = false;
    m_bUpdateFrame[1] = false;

    m_meshFillData[0].clear();
    m_meshFillData[1].clear();
    m_meshRenderData.clear();

    m_meshFillData[0].reserve(numMeshes);
    m_meshFillData[1].reserve(numMeshes);
    m_meshRenderData.reserve(numMeshes);

    for (uint i = 0; i < numMeshes; ++i)
    {
        SMeshRenderData meshRenderData;
        meshRenderData.m_pRenderMesh = pMeshes[i];
        m_meshRenderData.push_back(meshRenderData);
        m_meshFillData[0].push_back(meshRenderData);
        m_meshFillData[1].push_back(meshRenderData);
    }

    m_materialId = materialId;
}

void CREGeomCache::mfPrepare(bool bCheckOverflow)
{
    FUNCTION_PROFILER_RENDER_FLAT

    CRenderer* const pRenderer = gRenDev;

    if (bCheckOverflow)
    {
        pRenderer->FX_CheckOverflow(0, 0, this);
    }

    pRenderer->m_RP.m_CurVFormat = GetVertexFormat();
    pRenderer->m_RP.m_pRE = this;
    pRenderer->m_RP.m_FirstVertex = 0;
    pRenderer->m_RP.m_FirstIndex =  0;
    pRenderer->m_RP.m_RendNumIndices = 0;
    pRenderer->m_RP.m_RendNumVerts = 0;
}

void CREGeomCache::SetupMotionBlur(CRenderObject* pRenderObject, const SRenderingPassInfo& passInfo)
{
    CMotionBlur::SetupObject(pRenderObject, passInfo);

    if (pRenderObject->m_fDistance < CRenderer::CV_r_MotionBlurMaxViewDist)
    {
        pRenderObject->m_ObjFlags |= FOB_VERTEX_VELOCITY | FOB_MOTION_BLUR;
    }
}

bool CREGeomCache::Update(const int flags, const bool bTessellation)
{
    FUNCTION_PROFILER_RENDER_FLAT

    // Wait until render node update has finished
    const int threadId = gRenDev->m_RP.m_nProcessThreadID;
    while (m_transformUpdateState[threadId])
    {
        CrySleep(0);
    }

    // Check if update was successful and if so copy data to render buffer
    if (m_bUpdateFrame[threadId])
    {
        m_meshRenderData = m_meshFillData[threadId];
    }

    const uint numMeshes = m_meshFillData[threadId].size();
    bool bRet = true;

    for (uint nMesh = 0; nMesh < numMeshes; ++nMesh)
    {
        SMeshRenderData& meshData = m_meshFillData[threadId][nMesh];
        CRenderMesh* const pRenderMesh = static_cast<CRenderMesh*>(meshData.m_pRenderMesh.get());

        if (pRenderMesh && pRenderMesh->m_Modified[threadId].linked())
        {
            // Sync the async render mesh update. This waits for the fill thread started from main thread if it's still running.
            // We need to do this manually, because geom caches don't use CREMesh.
            pRenderMesh->SyncAsyncUpdate(threadId);

            CRenderMesh* pVertexContainer = pRenderMesh->_GetVertexContainer();
            bool bSucceed = pRenderMesh->RT_CheckUpdate(pVertexContainer, flags | VSM_MASK, bTessellation);
            if (bSucceed)
            {
                pRenderMesh->m_Modified[threadId].erase();
            }

            if (!bSucceed || !pVertexContainer->_HasVBStream(VSF_GENERAL))
            {
                bRet = false;
            }
        }
    }

    return bRet;
}

void CREGeomCache::UpdateModified()
{
    FUNCTION_PROFILER_RENDER_FLAT

    const int threadId = gRenDev->m_RP.m_nProcessThreadID;
    AZ_Assert(threadId >= 0 && threadId <= 2, "Container is not expecting this index");
    CryAutoLock<CryCriticalSection> lock(sm_updateList[threadId]->m_mutex);

    for (auto iter = sm_updateList[threadId]->m_geoms.begin();
         iter != sm_updateList[threadId]->m_geoms.end(); iter = sm_updateList[threadId]->m_geoms.erase(iter))
    {
        CREGeomCache* pRenderElement = *iter;
        pRenderElement->Update(0, false);
    }
}

bool CREGeomCache::mfUpdate(int Flags, bool bTessellation)
{
    const bool bRet = Update(Flags, bTessellation);

    const int threadId = gRenDev->m_RP.m_nProcessThreadID;
    CryAutoLock<CryCriticalSection> lock(sm_updateList[threadId]->m_mutex);
    stl::find_and_erase(sm_updateList[threadId]->m_geoms, this);

    m_Flags &= ~FCEF_DIRTY;
    return bRet;
}

volatile int* CREGeomCache::SetAsyncUpdateState(int& threadId)
{
    FUNCTION_PROFILER_RENDER_FLAT

    ASSERT_IS_MAIN_THREAD(gRenDev->m_pRT);
    threadId = gRenDev->m_RP.m_nFillThreadID;

    m_bUpdateFrame[threadId] = false;

    CryAutoLock<CryCriticalSection> lock(sm_updateList[threadId]->m_mutex);
    stl::push_back_unique(sm_updateList[threadId]->m_geoms, this);

    CryInterlockedIncrement(&m_transformUpdateState[threadId]);
    return &m_transformUpdateState[threadId];
}

DynArray<CREGeomCache::SMeshRenderData>* CREGeomCache::GetMeshFillDataPtr()
{
    FUNCTION_PROFILER_RENDER_FLAT

    assert(gEnv->IsEditor() || !gRenDev->m_pRT->IsRenderThread(true));
    const int threadId = gRenDev->m_RP.m_nFillThreadID;
    return &m_meshFillData[threadId];
}

DynArray<CREGeomCache::SMeshRenderData>* CREGeomCache::GetRenderDataPtr()
{
    FUNCTION_PROFILER_RENDER_FLAT

    assert(gEnv->IsEditor() || !gRenDev->m_pRT->IsRenderThread(true));
    return &m_meshRenderData;
}

void CREGeomCache::DisplayFilledBuffer(const int threadId)
{
    if (m_bUpdateFrame[threadId])
    {
        // You need to call SetAsyncUpdateState before DisplayFilledBuffer
        __debugbreak();
    }
    m_bUpdateFrame[threadId] = true;
}

AZ::Vertex::Format CREGeomCache::GetVertexFormat() const
{
    return m_geomCacheVertexFormat;
}

bool CREGeomCache::GetGeometryInfo(SGeometryInfo &streams)
{
    ZeroStruct(streams);
    streams.vertexFormat = GetVertexFormat();
    streams.nFirstIndex = 0;
    streams.nFirstVertex = 0;
    streams.nNumIndices = 0;
    streams.primitiveType = eptTriangleList;
    streams.streamMask = 0;
    return true;
}

#endif