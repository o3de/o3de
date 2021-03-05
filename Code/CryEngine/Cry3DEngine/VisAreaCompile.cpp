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

// Description : visarea node loading/saving


#include "Cry3DEngine_precompiled.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include <PakLoadDataUtils.h>

#define VISAREA_NODE_CHUNK_VERSION 2

#define VISAREA_FLAG_OCEAN_VISIBLE        BIT(0)
#define VISAREA_FLAG_IGNORE_SKY_COLOR     BIT(1)
#define VISAREA_FLAG_AFFECTEDBYOUTLIGHTS  BIT(2)
#define VISAREA_FLAG_SKYONLY              BIT(3)
#define VISAREA_FLAG_DOUBLESIDE           BIT(4)
#define VISAREA_FLAG_USEININDOORS         BIT(5)
#define VISAREA_FLAG_IGNORE_GI            BIT(6)
#define VISAREA_FLAG_IGNORE_OUTDOOR_AO    BIT(7)

#define MAX_VIS_AREA_CONNECTIONS_NUM 30

struct SVisAreaChunk
{
    int     nChunkVersion;
    AABB    boxArea, boxStatics;
    char    sName[32];
    int     nObjectsBlockSize;
    int     arrConnectionsId[MAX_VIS_AREA_CONNECTIONS_NUM];
    uint32  dwFlags;
    float  fPortalBlending;
    Vec3    vConnNormals[2];
    float fHeight;
    Vec3  vAmbColor;
    float fViewDistRatio;

    AUTO_STRUCT_INFO_LOCAL
};

#if ENGINE_ENABLE_COMPILATION
int CVisArea::GetData(byte*& pData, int& nDataSize, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial>>* pMatTable, std::vector<IStatInstGroup*>* pStatInstGroupTable, EEndian eEndian, SHotUpdateInfo* pExportInfo)
{
    if (m_pObjectsTree)
    {
        m_pObjectsTree->CleanUpTree();
    }

    if (pData)
    {
        byte* pHead = pData;
        SaveHeader(pData, nDataSize);

        // save shape points num
        int nPointsCount = m_lstShapePoints.Count();
        SwapEndian(nPointsCount, eEndian);
        memcpy(pData, &nPointsCount, sizeof(nPointsCount));
        UPDATE_PTR_AND_SIZE(pData, nDataSize, sizeof(nPointsCount));

        // save shape points
        memcpy(pData, m_lstShapePoints.GetElements(), m_lstShapePoints.GetDataSize());
        SwapEndian((Vec3*)pData, m_lstShapePoints.Count(), eEndian);
        UPDATE_PTR_AND_SIZE(pData, nDataSize, m_lstShapePoints.GetDataSize());

        SaveObjectsTree(pData, nDataSize, pStatObjTable, pMatTable, pStatInstGroupTable, eEndian, pExportInfo, pHead);
    }
    else // just count size
    {
        nDataSize += sizeof(SVisAreaChunk);

        nDataSize += sizeof(int);
        nDataSize += m_lstShapePoints.GetDataSize();

        if (m_pObjectsTree)
        {
            m_pObjectsTree->GetData(pData, nDataSize, NULL, NULL, NULL, eEndian, pExportInfo);
        }
    }
    return true;
}
#endif

int CVisArea::Load(byte*& f, int& nDataSizeLeft, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial>>* pMatTable, EEndian eEndian, SHotUpdateInfo* pExportInfo)
{
    return Load_T(f, nDataSizeLeft, pStatObjTable, pMatTable, eEndian, pExportInfo);
}

int CVisArea::Load(AZ::IO::HandleType& fileHandle, int& nDataSizeLeft, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial>>* pMatTable, EEndian eEndian, SHotUpdateInfo* pExportInfo)
{
    return Load_T(fileHandle, nDataSizeLeft, pStatObjTable, pMatTable, eEndian, pExportInfo);
}

template <class T>
int CVisArea::Load_T(T& f, int& nDataSizeLeft, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial>>* pMatTable, EEndian eEndian, SHotUpdateInfo* pExportInfo)
{
    int objBlockSize = 0;
    if (!LoadHeader_T(f, nDataSizeLeft, eEndian, objBlockSize))
    {
        return 0;
    }

    { // get shape points
        int nPointsCount = 0;
        if (!PakLoadDataUtils::LoadDataFromFile(&nPointsCount, 1, f, nDataSizeLeft, eEndian))
        {
            return 0;
        }

        // get shape points
        m_lstShapePoints.PreAllocate(nPointsCount, nPointsCount);
        if (!PakLoadDataUtils::LoadDataFromFile(m_lstShapePoints.GetElements(), nPointsCount, f, nDataSizeLeft, eEndian))
        {
            return 0;
        }

        UpdateClipVolume();
    }

    if (!LoadObjectsTree_T(f, nDataSizeLeft, 0, pStatObjTable, pMatTable, eEndian, pExportInfo, objBlockSize))
    {
        return 0;
    }

    return true;
}


const AABB* CVisArea::GetStaticObjectAABBox() const
{
    return &m_boxStatics;
}
#if ENGINE_ENABLE_COMPILATION
int CVisArea::SaveHeader(byte*& pData, int& nDataSize)
{
    // save node info
    SVisAreaChunk* pCunk = (SVisAreaChunk*)pData;
    pCunk->nChunkVersion = VISAREA_NODE_CHUNK_VERSION;
    pCunk->boxArea = m_boxArea;
    UpdateGeometryBBox();
    pCunk->boxStatics = m_boxStatics;
    memset(pCunk->sName, 0, sizeof(pCunk->sName));
    cry_strcpy(pCunk->sName, m_pVisAreaColdData->m_sName);
    memcpy(pCunk->vConnNormals, m_vConnNormals, sizeof(pCunk->vConnNormals));
    pCunk->fHeight = m_fHeight;
    pCunk->vAmbColor = m_vAmbientColor;
    pCunk->fViewDistRatio = m_fViewDistRatio;
    pCunk->fPortalBlending = m_fPortalBlending;

    pCunk->dwFlags = 0;
    if (m_bOceanVisible)
    {
        pCunk->dwFlags |= VISAREA_FLAG_OCEAN_VISIBLE;
    }
    if (m_bIgnoreSky)
    {
        pCunk->dwFlags |= VISAREA_FLAG_IGNORE_SKY_COLOR;
    }
    if (m_bAffectedByOutLights)
    {
        pCunk->dwFlags |= VISAREA_FLAG_AFFECTEDBYOUTLIGHTS;
    }
    if (m_bSkyOnly)
    {
        pCunk->dwFlags |= VISAREA_FLAG_SKYONLY;
    }
    if (m_bDoubleSide)
    {
        pCunk->dwFlags |= VISAREA_FLAG_DOUBLESIDE;
    }
    if (m_bUseInIndoors)
    {
        pCunk->dwFlags |= VISAREA_FLAG_USEININDOORS;
    }
    if (m_bIgnoreGI)
    {
        pCunk->dwFlags |= VISAREA_FLAG_IGNORE_GI;
    }
    if (m_bIgnoreOutdoorAO)
    {
        pCunk->dwFlags |= VISAREA_FLAG_IGNORE_OUTDOOR_AO;
    }

    // transform connections id into pointers
    PodArray<CVisArea*>& rAreas = IsPortal() ? GetVisAreaManager()->m_lstVisAreas : GetVisAreaManager()->m_lstPortals;
    for (int i = 0; i < MAX_VIS_AREA_CONNECTIONS_NUM; i++)
    {
        pCunk->arrConnectionsId[i] = -1;
    }
    for (int i = 0; i < m_lstConnections.Count() && i < MAX_VIS_AREA_CONNECTIONS_NUM; i++)
    {
        IVisArea* pArea = m_lstConnections[i];
        int nId;
        for (nId = 0; nId < rAreas.Count(); nId++)
        {
            if (pArea == rAreas[nId])
            {
                break;
            }
        }

        if (nId < rAreas.Count())
        {
            pCunk->arrConnectionsId[i] = nId;
        }
        else
        {
            pCunk->arrConnectionsId[i] = -1;
            assert(!"Undefined connction");
        }
    }

    UPDATE_PTR_AND_SIZE(pData, nDataSize, sizeof(SVisAreaChunk));

    return true;
}

int CVisArea::SaveObjectsTree(byte*& pData, int& nDataSize, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial>>* pMatTable, std::vector<IStatInstGroup*>* pStatInstGroupTable, EEndian eEndian, SHotUpdateInfo* pExportInfo, byte* pHead)
{
    SVisAreaChunk* pCunk = (SVisAreaChunk*)pHead;

    // save objects
    pCunk->nObjectsBlockSize = 0;

    // get data from objects tree
    if (m_pObjectsTree)
    {
        byte* pTmp = NULL;
        m_pObjectsTree->GetData(pTmp, pCunk->nObjectsBlockSize, NULL, NULL, NULL, eEndian, pExportInfo);
        m_pObjectsTree->GetData(pData, nDataSize, pStatObjTable, pMatTable, pStatInstGroupTable, eEndian, pExportInfo); // UPDATE_PTR_AND_SIZE is inside
    }

    SwapEndian(*pCunk, eEndian);

    return true;
}
#endif
template <class T>
int CVisArea::LoadHeader_T(T& f, int& nDataSizeLeft, EEndian eEndian, int& objBlockSize)
{
    SVisAreaChunk chunk;
    if (!PakLoadDataUtils::LoadDataFromFile(&chunk, 1, f, nDataSizeLeft, eEndian))
    {
        return 0;
    }

    assert(chunk.nChunkVersion == VISAREA_NODE_CHUNK_VERSION);
    if (chunk.nChunkVersion != VISAREA_NODE_CHUNK_VERSION)
    {
        return 0;
    }

    // get area info
    m_boxArea = chunk.boxArea;
    m_boxStatics = chunk.boxStatics;
    chunk.sName[sizeof(chunk.sName) - 1] = 0;
    cry_strcpy(m_pVisAreaColdData->m_sName, chunk.sName);
    m_bThisIsPortal = strstr(m_pVisAreaColdData->m_sName, "portal") != 0;
    m_bIgnoreSky = (strstr(m_pVisAreaColdData->m_sName, "ignoresky") != 0) || ((chunk.dwFlags & VISAREA_FLAG_IGNORE_SKY_COLOR) != 0);
    memcpy(m_vConnNormals, chunk.vConnNormals, sizeof(m_vConnNormals));
    m_fHeight = chunk.fHeight;
    m_vAmbientColor = chunk.vAmbColor;
    m_fViewDistRatio = chunk.fViewDistRatio;
    m_fPortalBlending = chunk.fPortalBlending;

    if (chunk.dwFlags == uint32(-1))
    {
        chunk.dwFlags = 0;
    }
    m_bOceanVisible = (chunk.dwFlags & VISAREA_FLAG_OCEAN_VISIBLE) != 0;

    m_bAffectedByOutLights = (chunk.dwFlags & VISAREA_FLAG_AFFECTEDBYOUTLIGHTS) != 0;
    m_bSkyOnly = (chunk.dwFlags & VISAREA_FLAG_SKYONLY) != 0;
    m_bDoubleSide = (chunk.dwFlags & VISAREA_FLAG_DOUBLESIDE) != 0;
    m_bUseInIndoors = (chunk.dwFlags & VISAREA_FLAG_USEININDOORS) != 0;
    m_bIgnoreGI = (chunk.dwFlags & VISAREA_FLAG_IGNORE_GI) != 0;
    m_bIgnoreOutdoorAO = (chunk.dwFlags & VISAREA_FLAG_IGNORE_OUTDOOR_AO) != 0;

    objBlockSize = chunk.nObjectsBlockSize;

    // convert connections id into pointers
    PodArray<CVisArea*>& rAreas = IsPortal() ? GetVisAreaManager()->m_lstVisAreas : GetVisAreaManager()->m_lstPortals;
    for (int i = 0; i < MAX_VIS_AREA_CONNECTIONS_NUM && rAreas.Count(); i++)
    {
        assert(chunk.arrConnectionsId[i] < rAreas.Count());
        if (chunk.arrConnectionsId[i] >= 0)
        {
            m_lstConnections.Add(rAreas[chunk.arrConnectionsId[i]]);
        }
    }

    return true;
}

template <class T>
int CVisArea::LoadObjectsTree_T(T& f, int& nDataSizeLeft, int nSID, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial>>* pMatTable, EEndian eEndian, SHotUpdateInfo* pExportInfo, const int objBlockSize)
{
    // mark tree as invalid since new visarea was just added
    SAFE_DELETE(GetVisAreaManager()->m_pAABBTree);

    AABB* pBox = (pExportInfo && !pExportInfo->areaBox.IsReset()) ? &pExportInfo->areaBox : NULL;

    // load content of objects tree
    if (!m_bEditor && objBlockSize > 4)
    {
        int nCurDataSize = nDataSizeLeft;
        if (nCurDataSize > 0)
        {
            if (!m_pObjectsTree)
            {
                m_pObjectsTree = COctreeNode::Create(DEFAULT_SID, m_boxArea, this);
            }
            m_pObjectsTree->UpdateVisAreaSID(this, nSID);

            if (pExportInfo != NULL && pExportInfo->pVisibleLayerMask != NULL && pExportInfo->pLayerIdTranslation)
            {
                SLayerVisibility visInfo;
                visInfo.pLayerVisibilityMask = pExportInfo->pVisibleLayerMask;
                visInfo.pLayerIdTranslation = pExportInfo->pLayerIdTranslation;
                m_pObjectsTree->Load(f, nDataSizeLeft, pStatObjTable, pMatTable, eEndian, pBox, &visInfo);
            }
            else
            {
                m_pObjectsTree->Load(f, nDataSizeLeft, pStatObjTable, pMatTable, eEndian, pBox, NULL);
            }

            assert(nDataSizeLeft == (nCurDataSize - objBlockSize));
        }
    }
    else if (objBlockSize > 0)
    {
        PakLoadDataUtils::LoadDataFromFile_Seek(objBlockSize, f, nDataSizeLeft, eEndian);
    }

    return true;
}

VisAreaGUID CVisArea::GetGUIDFromFile(byte* f, EEndian eEndian)
{
    SVisAreaChunk* pChunk = (SVisAreaChunk*)f;
    SwapEndian(pChunk, sizeof(SVisAreaChunk), eEndian);

    assert(pChunk->nChunkVersion == VISAREA_NODE_CHUNK_VERSION);
    if (pChunk->nChunkVersion != VISAREA_NODE_CHUNK_VERSION)
    {
        return 0;
    }

    VisAreaGUID guid = *(VisAreaGUID*)(f + sizeof(SVisAreaChunk));
    SwapEndian(&guid, sizeof(VisAreaGUID), eEndian);
    return guid;
}

//////////////////////////////////////////////////////////////////////
// Segmented World
#if ENGINE_ENABLE_COMPILATION
int CVisArea::GetSegmentData([[maybe_unused]] byte*& pData, [[maybe_unused]] int& nDataSize, [[maybe_unused]] std::vector<IStatObj*>* pStatObjTable, [[maybe_unused]] std::vector<_smart_ptr<IMaterial>>* pMatTable, [[maybe_unused]] std::vector<IStatInstGroup*>* pStatInstGroupTable, [[maybe_unused]] EEndian eEndian, [[maybe_unused]] SHotUpdateInfo* pExportInfo)
{
    CRY_ASSERT(false); // SegmentedWorld has been removed
    return false;
}
#endif
int CSWVisArea::Load(byte*& f, int& nDataSizeLeft, [[maybe_unused]] int nSID, [[maybe_unused]] std::vector<IStatObj*>* pStatObjTable, [[maybe_unused]] std::vector<_smart_ptr<IMaterial>>* pMatTable, EEndian eEndian, [[maybe_unused]] SHotUpdateInfo* pExportInfo, [[maybe_unused]] const Vec2& indexOffset)
{
    int objBlockSize = 0;
    if (!LoadHeader_T(f, nDataSizeLeft, eEndian, objBlockSize))
    {
        return 0;
    }

    CRY_ASSERT(false); // SegmentedWorld has been removed
    return false;
}

#include "TypeInfo_impl.h"
#include "VisAreaCompile_info.h"


