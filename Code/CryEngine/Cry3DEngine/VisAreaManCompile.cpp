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

// Description : check vis


#include "Cry3DEngine_precompiled.h"

#include "ObjMan.h"
#include "VisAreas.h"

bool CVisAreaManager::GetCompiledData([[maybe_unused]] byte* pData, [[maybe_unused]] int nDataSize, [[maybe_unused]] std::vector<struct IStatObj*>** ppStatObjTable, [[maybe_unused]] std::vector<_smart_ptr<IMaterial>>** ppMatTable, [[maybe_unused]] std::vector<struct IStatInstGroup*>** ppStatInstGroupTable, [[maybe_unused]] EEndian eEndian, [[maybe_unused]] SHotUpdateInfo* pExportInfo)
{
# if !ENGINE_ENABLE_COMPILATION
    CryFatalError("serialization code removed, please enable 3DENGINE_ENABLE_COMPILATION in Cry3DEngine/StdAfx.h");
    return false;
# else
    float fStartTime = GetCurAsyncTimeSec();


    //  PrintMessage("Exporting indoor data (%s, %.2f MB) ...",

    // write header
    SVisAreaManChunkHeader* pVisAreaManagerChunkHeader = (SVisAreaManChunkHeader*)pData;
    pVisAreaManagerChunkHeader->nVersion = VISAREAMANAGER_CHUNK_VERSION;
    pVisAreaManagerChunkHeader->nDummy = 0;
    pVisAreaManagerChunkHeader->nFlags = (eEndian == eBigEndian) ? SERIALIZATION_FLAG_BIG_ENDIAN : 0;
    pVisAreaManagerChunkHeader->nFlags2 = 0;
    pVisAreaManagerChunkHeader->nChunkSize = nDataSize;

    SwapEndian(*pVisAreaManagerChunkHeader, eEndian);

    UPDATE_PTR_AND_SIZE(pData, nDataSize, sizeof(SVisAreaManChunkHeader));

    pVisAreaManagerChunkHeader->nVisAreasNum = m_lstVisAreas.Count();
    pVisAreaManagerChunkHeader->nPortalsNum = m_lstPortals.Count();
    pVisAreaManagerChunkHeader->nOcclAreasNum = m_lstOcclAreas.Count();

    for (int i = 0; i < m_lstVisAreas.Count(); i++)
    {
        m_lstVisAreas[i]->GetData(pData, nDataSize, *ppStatObjTable, *ppMatTable, *ppStatInstGroupTable, eEndian, pExportInfo);
    }

    for (int i = 0; i < m_lstPortals.Count(); i++)
    {
        m_lstPortals[i]->GetData(pData, nDataSize, *ppStatObjTable, *ppMatTable, *ppStatInstGroupTable, eEndian, pExportInfo);
    }

    for (int i = 0; i < m_lstOcclAreas.Count(); i++)
    {
        m_lstOcclAreas[i]->GetData(pData, nDataSize, *ppStatObjTable, *ppMatTable, *ppStatInstGroupTable, eEndian, pExportInfo);
    }

    SAFE_DELETE(*ppStatObjTable);
    SAFE_DELETE(*ppMatTable);
    SAFE_DELETE(*ppStatInstGroupTable);

    if (!pExportInfo)
    {
        PrintMessagePlus(" done in %.2f sec", GetCurAsyncTimeSec() - fStartTime);
    }

    assert(nDataSize == 0);
    return nDataSize == 0;
# endif
}

int CVisAreaManager::GetCompiledDataSize([[maybe_unused]] SHotUpdateInfo* pExportInfo)
{
# if !ENGINE_ENABLE_COMPILATION
    CryFatalError("serialization code removed, please enable 3DENGINE_ENABLE_COMPILATION in Cry3DEngine/StdAfx.h");
    return 0;
# else

    int nDataSize = 0;
    byte* pData = NULL;

    // get header size
    nDataSize += sizeof(SVisAreaManChunkHeader);

    for (int i = 0; i < m_lstVisAreas.Count(); i++)
    {
        m_lstVisAreas[i]->GetData(pData, nDataSize, NULL, NULL, NULL, eLittleEndian, pExportInfo);
    }

    for (int i = 0; i < m_lstPortals.Count(); i++)
    {
        m_lstPortals[i]->GetData(pData, nDataSize, NULL, NULL, NULL, eLittleEndian, pExportInfo);
    }

    for (int i = 0; i < m_lstOcclAreas.Count(); i++)
    {
        m_lstOcclAreas[i]->GetData(pData, nDataSize, NULL, NULL, NULL, eLittleEndian, pExportInfo);
    }

    return nDataSize;
# endif
}

bool CVisAreaManager::Load(AZ::IO::HandleType& fileHandle, int& nDataSize, struct SVisAreaManChunkHeader* pVisAreaManagerChunkHeader, std::vector<struct IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial>>* pMatTable)
{
    bool bRes;

    // in case of small data amount (console game) load entire file into memory in single operation
    if (nDataSize < 4 * 1024 * 1024)
    {
        auto pMemBlock = gEnv->pCryPak->PoolAllocMemoryBlock(nDataSize + 8, "LoadIndoors");
        byte* pPtr = (byte*)pMemBlock->m_address.get();
        while (UINT_PTR(pPtr) & 3)
        {
            pPtr++;
        }

        if (GetPak()->FReadRaw(pPtr, 1, nDataSize - sizeof(SVisAreaManChunkHeader), fileHandle) != nDataSize - sizeof(SVisAreaManChunkHeader))
        {
            return false;
        }

        bRes = Load_T(pPtr, nDataSize, pVisAreaManagerChunkHeader, pStatObjTable, pMatTable, false, NULL);
    }
    else
    {
        bRes = Load_T(fileHandle, nDataSize, pVisAreaManagerChunkHeader, pStatObjTable, pMatTable, false, NULL);
    }

    return bRes;
}

bool CVisAreaManager::SetCompiledData(byte* pData, int nDataSize, std::vector<struct IStatObj*>** ppStatObjTable, std::vector<_smart_ptr<IMaterial>>** ppMatTable, bool bHotUpdate, SHotUpdateInfo* pExportInfo)
{
    SVisAreaManChunkHeader* pChunkHeader = (SVisAreaManChunkHeader*)pData;
    pData += sizeof(SVisAreaManChunkHeader);

    SwapEndian(*pChunkHeader, eLittleEndian);

    bool bRes = Load_T(pData, nDataSize, pChunkHeader, *ppStatObjTable, *ppMatTable, bHotUpdate, pExportInfo);

    SAFE_DELETE(*ppStatObjTable);
    SAFE_DELETE(*ppMatTable);

    return bRes;
}

void CVisAreaManager::UnregisterEngineObjectsInArea(const SHotUpdateInfo* pExportInfo, PodArray<IRenderNode*>& arrUnregisteredObjects, bool bOnlyEngineObjects)
{
    for (int i = 0; i < m_lstVisAreas.Count(); i++)
    {
        if (m_lstVisAreas[i]->m_pObjectsTree)
        {
            m_lstVisAreas[i]->m_pObjectsTree->UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, bOnlyEngineObjects);
        }
    }

    for (int i = 0; i < m_lstPortals.Count(); i++)
    {
        if (m_lstPortals[i]->m_pObjectsTree)
        {
            m_lstPortals[i]->m_pObjectsTree->UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, bOnlyEngineObjects);
        }
    }

    for (int i = 0; i < m_lstOcclAreas.Count(); i++)
    {
        if (m_lstOcclAreas[i]->m_pObjectsTree)
        {
            m_lstOcclAreas[i]->m_pObjectsTree->UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, bOnlyEngineObjects);
        }
    }
}

void CVisAreaManager::OnVisAreaDeleted(IVisArea* pArea)
{
    for (int i = 0, num = m_lstCallbacks.size(); i < num; i++)
    {
        m_lstCallbacks[i]->OnVisAreaDeleted(pArea);
    }

    m_lstActiveOcclVolumes.Delete((CVisArea*)pArea);
    m_lstIndoorActiveOcclVolumes.Delete((CVisArea*)pArea);
    m_lstActiveEntransePortals.Delete((CVisArea*)pArea);
}

template <class T>
bool CVisAreaManager::Load_T(T& f, int& nDataSize, SVisAreaManChunkHeader* pVisAreaManagerChunkHeader, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial>>* pMatTable, [[maybe_unused]] bool bHotUpdate, SHotUpdateInfo* pExportInfo)
{
    if (pVisAreaManagerChunkHeader->nVersion != VISAREAMANAGER_CHUNK_VERSION)
    {
        Error("CVisAreaManager::SetCompiledData: version of file is %d, expected version is %d", pVisAreaManagerChunkHeader->nVersion, (int)VISAREAMANAGER_CHUNK_VERSION);
        return 0;
    }

    if (pVisAreaManagerChunkHeader->nChunkSize != nDataSize)
    {
        Error("CVisAreaManager::SetCompiledData: data size mismatch (%d != %d)", pVisAreaManagerChunkHeader->nChunkSize, nDataSize);
        return 0;
    }


    EEndian eEndian = (pVisAreaManagerChunkHeader->nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian;

    PodArray<IRenderNode*> arrUnregisteredObjects;
    UnregisterEngineObjectsInArea(pExportInfo, arrUnregisteredObjects, true);

    PodArray<IRenderNode*> arrUnregisteredEntities;
    UnregisterEngineObjectsInArea(NULL, arrUnregisteredEntities, false);

    DeleteAllVisAreas();

    SAFE_DELETE(m_pAABBTree);
    m_pCurArea = m_pCurPortal = 0;

    { // construct areas
        m_lstVisAreas.PreAllocate(pVisAreaManagerChunkHeader->nVisAreasNum, pVisAreaManagerChunkHeader->nVisAreasNum);
        m_lstPortals.PreAllocate(pVisAreaManagerChunkHeader->nPortalsNum, pVisAreaManagerChunkHeader->nPortalsNum);
        m_lstOcclAreas.PreAllocate(pVisAreaManagerChunkHeader->nOcclAreasNum, pVisAreaManagerChunkHeader->nOcclAreasNum);

        nDataSize -= sizeof(SVisAreaManChunkHeader);

        //    if(bHotUpdate)
        //    PrintMessage("Importing indoor data (%s, %.2f MB) ...",
        m_visAreas.PreAllocate(pVisAreaManagerChunkHeader->nVisAreasNum);
        m_visAreaColdData.PreAllocate(pVisAreaManagerChunkHeader->nVisAreasNum);

        m_portals.PreAllocate(pVisAreaManagerChunkHeader->nPortalsNum);
        m_portalColdData.PreAllocate(pVisAreaManagerChunkHeader->nPortalsNum);

        m_occlAreas.PreAllocate(pVisAreaManagerChunkHeader->nOcclAreasNum);
        m_occlAreaColdData.PreAllocate(pVisAreaManagerChunkHeader->nOcclAreasNum);

        for (int i = 0; i < m_lstVisAreas.Count(); i++)
        {
            m_lstVisAreas[i] = CreateTypeVisArea();
        }
        for (int i = 0; i < m_lstPortals.Count(); i++)
        {
            m_lstPortals[i] = CreateTypePortal();
        }
        for (int i = 0; i < m_lstOcclAreas.Count(); i++)
        {
            m_lstOcclAreas[i] = CreateTypeOcclArea();
        }
    }

    { // load areas content
        for (int i = 0; i < m_lstVisAreas.Count(); i++)
        {
            m_lstVisAreas[i]->Load(f, nDataSize, pStatObjTable, pMatTable, eEndian, pExportInfo);
        }

        for (int i = 0; i < m_lstPortals.Count(); i++)
        {
            m_lstPortals[i]->Load(f, nDataSize, pStatObjTable, pMatTable, eEndian, pExportInfo);
        }

        for (int i = 0; i < m_lstOcclAreas.Count(); i++)
        {
            m_lstOcclAreas[i]->Load(f, nDataSize, pStatObjTable, pMatTable, eEndian, pExportInfo);
        }
    }

    for (int i = 0; i < arrUnregisteredObjects.Count(); i++)
    {
        arrUnregisteredObjects[i]->ReleaseNode();
    }
    arrUnregisteredObjects.Reset();

    for (int i = 0; i < arrUnregisteredEntities.Count(); i++)
    {
        Get3DEngine()->RegisterEntity(arrUnregisteredEntities[i]);
    }
    arrUnregisteredEntities.Reset();

    SAFE_DELETE(m_pAABBTree);
    m_pCurArea = m_pCurPortal = 0;
    UpdateConnections();

    return nDataSize == 0;
}

//////////////////////////////////////////////////////////////////////
// Segmented World
inline bool IsContainBox2D(const AABB& base, const AABB& test)
{
    if (base.min.x < test.max.x && base.max.x > test.min.x &&
        base.min.y < test.max.y && base.max.y > test.min.y)
    {
        return true;
    }

    return false;
}

void CVisAreaManager::PrepareSegmentData(const AABB& box)
{
    m_segVisAreas.Clear();
    for (int v = 0; v < m_lstVisAreas.Count(); v++)
    {
        if (IsContainBox2D(m_lstVisAreas[v]->m_boxArea, box))
        {
            m_segVisAreas.Add(m_lstVisAreas[v]);
        }
    }

    m_segPortals.Clear();
    for (int p = 0; p < m_lstPortals.Count(); p++)
    {
        if (IsContainBox2D(m_lstPortals[p]->m_boxArea, box))
        {
            m_segPortals.Add(m_lstPortals[p]);
        }
    }

    m_segOcclAreas.Clear();
    for (int o = 0; o < m_lstOcclAreas.Count(); o++)
    {
        if (IsContainBox2D(m_lstOcclAreas[o]->m_boxArea, box))
        {
            m_segOcclAreas.Add(m_lstOcclAreas[o]);
        }
    }
}

bool CVisAreaManager::StreamCompiledData(uint8* pData, int nDataSize, int nSID, std::vector<struct IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial>>* pMatTable, std::vector<struct IStatInstGroup*>* pStatInstGroupTable, const Vec2& vIndexOffset)
{
    SVisAreaManChunkHeader* pVisAreaManagerChunkHeader = (SVisAreaManChunkHeader*)pData;

    pData += sizeof(SVisAreaManChunkHeader);
    nDataSize -= sizeof(SVisAreaManChunkHeader);

    assert(pVisAreaManagerChunkHeader->nVersion == VISAREAMANAGER_CHUNK_VERSION);

    EEndian eEndian = (pVisAreaManagerChunkHeader->nFlags & SERIALIZATION_FLAG_BIG_ENDIAN) ? eBigEndian : eLittleEndian;

    SAFE_DELETE(m_pAABBTree);
    m_pCurArea = m_pCurPortal = 0;

    for (int i = 0; i < pVisAreaManagerChunkHeader->nVisAreasNum; i++)
    {
        VisAreaGUID guid = CVisArea::GetGUIDFromFile(pData, eEndian);
        CSWVisArea* pVisArea = (CSWVisArea*)FindVisAreaByGuid(guid, m_lstVisAreas);
        if (!pVisArea)
        {
            pVisArea = CreateVisAreaFromPool(m_lstVisAreas, m_visAreas, m_visAreaColdData, false);
            m_lstVisAreas.Add(pVisArea);
        }
        pVisArea->AddRef();
        m_visAreaSegmentData[nSID].m_visAreaIndices.push_back(pVisArea->m_nSlotID);
        pVisArea->Load(pData, nDataSize, nSID, pStatObjTable, pMatTable, eEndian, 0, vIndexOffset);
    }

    for (int i = 0; i < pVisAreaManagerChunkHeader->nPortalsNum; i++)
    {
        VisAreaGUID guid = CVisArea::GetGUIDFromFile(pData, eEndian);
        CSWVisArea* pVisArea = (CSWVisArea*)FindVisAreaByGuid(guid, m_lstPortals);
        if (!pVisArea)
        {
            pVisArea = CreateVisAreaFromPool(m_lstPortals, m_portals, m_portalColdData, true);
            m_lstPortals.Add(pVisArea);
        }
        pVisArea->AddRef();
        m_portalSegmentData[nSID].m_visAreaIndices.push_back(pVisArea->m_nSlotID);
        pVisArea->Load(pData, nDataSize, nSID, pStatObjTable, pMatTable, eEndian, 0, vIndexOffset);
    }

    for (int i = 0; i < pVisAreaManagerChunkHeader->nOcclAreasNum; i++)
    {
        VisAreaGUID guid = CVisArea::GetGUIDFromFile(pData, eEndian);
        CSWVisArea* pVisArea = (CSWVisArea*)FindVisAreaByGuid(guid, m_lstOcclAreas);
        if (!pVisArea)
        {
            pVisArea = CreateVisAreaFromPool(m_lstOcclAreas, m_occlAreas, m_occlAreaColdData, false);
            m_lstOcclAreas.Add(pVisArea);
        }
        pVisArea->AddRef();
        m_occlAreaSegmentData[nSID].m_visAreaIndices.push_back(pVisArea->m_nSlotID);
        pVisArea->Load(pData, nDataSize, nSID, pStatObjTable, pMatTable, eEndian, 0, vIndexOffset);
    }

    SAFE_DELETE(m_pAABBTree);
    m_pCurArea = m_pCurPortal = 0;

    SAFE_DELETE(pStatObjTable);
    SAFE_DELETE(pMatTable);
    SAFE_DELETE(pStatInstGroupTable);

    return !(nDataSize == 0);
}

CSWVisArea* CVisAreaManager::FindFreeVisAreaFromPool(PodArray<CVisArea*, ReservedVisAreaBytes>& visAreas)
{
    for (int i = 0; i < visAreas.Count(); i++)
    {
        CSWVisArea* pVisArea = (CSWVisArea*)visAreas[i];

        if (!pVisArea->NumRefs())
        {
            pVisArea->m_nSlotID = i;
            return pVisArea;
        }
    }

    return NULL;
}

template<class T>
CSWVisArea* CVisAreaManager::CreateVisAreaFromPool(PodArray<CVisArea*>& lstVisAreas, PodArray<CVisArea*, ReservedVisAreaBytes>& visAreas, PodArray<T>& visAreaColdData, bool bIsPortal)
{
    CSWVisArea* pVisArea = FindFreeVisAreaFromPool(visAreas);
    if (!pVisArea)
    {
        int nVisArea = visAreas.Count();
        visAreas.PreAllocate(nVisArea * 2);
        visAreaColdData.PreAllocate(nVisArea * 2);

        ResetVisAreaList(lstVisAreas, visAreas, visAreaColdData);

        pVisArea = CreateTypeArea(visAreas, visAreaColdData, bIsPortal);

        // assign the slot id once the pool is enlarged
        pVisArea->m_nSlotID = nVisArea;
    }

    return pVisArea;
}

template<class T>
void CVisAreaManager::ResetVisAreaList(PodArray<CVisArea*>& lstVisAreas, PodArray<CVisArea*, ReservedVisAreaBytes>& visAreas, PodArray<T>& visAreaColdData)
{
    for (int i = 0; i < visAreas.Count(); i++)
    {
        CVisArea* pVisArea = visAreas[i];
        if (pVisArea->m_pObjectsTree)
        {
            pVisArea->m_pObjectsTree->SetVisArea(pVisArea);
        }
        pVisArea->SetColdDataPtr(&visAreaColdData[i]);
        lstVisAreas[i] = pVisArea;
    }
}

template<class T>
CSWVisArea* CVisAreaManager::CreateTypeArea(PodArray<CVisArea*, ReservedVisAreaBytes>& visAreas, PodArray<T>& visAreaColdData, bool bIsPortal)
{
    CSWVisArea* pNewVisArea = new CSWVisArea();
    SGenericColdData* pColdData     = &visAreaColdData.AddNew();

    visAreas.Add(pNewVisArea);
    pColdData->m_dataType = bIsPortal ? eCDT_Portal : eCDT_Generic;
    pNewVisArea->SetColdDataPtr(pColdData);

    return pNewVisArea;
}

