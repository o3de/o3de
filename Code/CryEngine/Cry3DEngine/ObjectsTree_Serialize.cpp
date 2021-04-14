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

#include "Cry3DEngine_precompiled.h"

#include "DecalRenderNode.h"
#include "WaterVolumeRenderNode.h"
#include "DistanceCloudRenderNode.h"
#include "VisAreas.h"
#include <MathConversion.h>

#pragma pack(push,4)

// common node data
struct SRenderNodeChunk
{
    AABB          m_WSBBox = AABB();
    uint16        m_nLayerId = 0;
    int8          m_cShadowLodBias = 0;
    uint8         m_ucDummy = 0;
    uint32        m_dwRndFlags = 0;
    uint16        m_nObjectTypeIndex = 0;
    uint16        m_pad16 = 0;
    float         m_fViewDistanceMultiplier = 0.0f;
    uint8         m_ucLodRatio = 0;
    // Can't guarantee alignment for derived structs across different compilers
    // http://stackoverflow.com/questions/22404423/c-pod-struct-inheritance-are-there-any-guarantees-about-the-memory-layout-of
    uint8         m_pad8 = 0;
    uint16        m_pad16B = 0;

    AUTO_STRUCT_INFO_LOCAL
};

#define ROADCHUNKFLAG_IGNORE_TERRAIN_HOLES 1
#define ROADCHUNKFLAG_PHYSICALIZE 2

struct SDecalChunk
    : public SRenderNodeChunk
{
    int16     m_projectionType = 0;
    uint8     m_deferred = 0;
    uint8     m_pad8 = 0;
    f32       m_depth = 0.0f;
    Vec3      m_pos = Vec3(0.0f);
    Vec3      m_normal = Vec3(0.0f);
    Matrix33  m_explicitRightUpFront = Matrix33();
    f32       m_radius = 0.0f;
    int32     m_nMaterialId = 0;
    int32     m_nSortPriority = 0;

    AUTO_STRUCT_INFO_LOCAL
};

struct SWaterVolumeChunk
    : public SRenderNodeChunk
{
    // volume type and id
    int32 m_volumeTypeAndMiscBits = 0;
    uint64 m_volumeID = 0;

    // material
    int32 m_materialID = 0;

    // fog properties
    f32 m_fogDensity = 0.0f;
    Vec3 m_fogColor = Vec3(0.0f);
    Plane m_fogPlane = Plane();
    f32 m_fogShadowing = 0.0f;

    // caustic propeties
    uint8 m_caustics = 0;
    uint8 m_pad8 = 0;
    uint16 m_pad16 = 0;
    f32 m_causticIntensity = 0.0f;
    f32 m_causticTiling = 0.0f;
    f32 m_causticHeight = 0.0f;

    // render geometry
    f32 m_uTexCoordBegin = 0.0f;
    f32 m_uTexCoordEnd = 0.0f;
    f32 m_surfUScale = 0.0f;
    f32 m_surfVScale = 0.0f;
    uint32 m_numVertices = 0;

    // physics properties
    f32 m_volumeDepth = 0.0f;
    f32 m_streamSpeed = 0.0f;
    uint32 m_numVerticesPhysAreaContour = 0;

    AUTO_STRUCT_INFO_LOCAL
};

struct SWaterVolumeVertex
{
    Vec3 m_xyz;

    AUTO_STRUCT_INFO_LOCAL
};

struct SDistanceCloudChunk
    : public SRenderNodeChunk
{
    Vec3 m_pos = Vec3(0.0f);
    f32 m_sizeX = 0.0f;
    f32 m_sizeY = 0.0f;
    f32 m_rotationZ = 0.0f;
    int32 m_materialID = 0;

    AUTO_STRUCT_INFO_LOCAL
};

#pragma pack(pop)

AUTO_TYPE_INFO(EERType)

#include "TypeInfo_impl.h"
#include "ObjectsTree_Serialize_info.h"

inline void CopyCommonData(SRenderNodeChunk* pChunk, IRenderNode* pObj)
{
    pChunk->m_WSBBox = pObj->GetBBox();
    //COPY_MEMBER_SAVE(pChunk,pObj,m_fWSMaxViewDist);
    COPY_MEMBER_SAVE(pChunk, pObj, m_dwRndFlags);
    COPY_MEMBER_SAVE(pChunk, pObj, m_fViewDistanceMultiplier);
    COPY_MEMBER_SAVE(pChunk, pObj, m_ucLodRatio);
    COPY_MEMBER_SAVE(pChunk, pObj, m_cShadowLodBias);
    pChunk->m_nLayerId = pObj->GetLayerId();
}

inline void LoadCommonData(SRenderNodeChunk* pChunk, IRenderNode* pObj, const SLayerVisibility* pLayerVisibility)
{
    //COPY_MEMBER_LOAD(pObj,pChunk,m_fWSMaxViewDist);
    COPY_MEMBER_LOAD(pObj, pChunk, m_dwRndFlags);
    COPY_MEMBER_LOAD(pObj, pChunk, m_fViewDistanceMultiplier);
    COPY_MEMBER_LOAD(pObj, pChunk, m_ucLodRatio);
    COPY_MEMBER_LOAD(pObj, pChunk, m_cShadowLodBias);
    pObj->m_dwRndFlags &= ~(ERF_HIDDEN | ERF_SELECTED);
    if (pObj->m_dwRndFlags & ERF_CASTSHADOWMAPS)
    {
        pObj->m_dwRndFlags |= ERF_HAS_CASTSHADOWMAPS;
    }

    if (NULL != pLayerVisibility)
    {
        const uint16 newLayerId = pLayerVisibility->pLayerIdTranslation[pChunk->m_nLayerId];
        pObj->SetLayerId(newLayerId);
    }
    else
    {
        pObj->SetLayerId(pChunk->m_nLayerId);
    }
}

//////////////////////////////////////////////////////////////////////////
inline static uint8 CheckLayerVisibility(const uint16 layerId, const SLayerVisibility* pLayerVisibility)
{
    return pLayerVisibility->pLayerVisibilityMask[layerId >> 3] & (1 << (layerId & 7));
}

//////////////////////////////////////////////////////////////////////////
int COctreeNode::SaveObjects([[maybe_unused]] CMemoryBlock* pMemBlock, [[maybe_unused]] std::vector<IStatObj*>* pStatObjTable, [[maybe_unused]] std::vector<_smart_ptr<IMaterial> >* pMatTable, [[maybe_unused]] std::vector<IStatInstGroup*>* pStatInstGroupTable, [[maybe_unused]] EEndian eEndian, [[maybe_unused]] const SHotUpdateInfo* pExportInfo)
{
# if !ENGINE_ENABLE_COMPILATION
    CryFatalError("serialization code removed, please enable ENGINE_ENABLE_COMPILATION in Cry3DEngine/StdAfx.h");
    return 0;
# else
    uint32 nObjTypeMask = pExportInfo ? pExportInfo->nObjTypeMask : (uint32) ~0;

    int nBlockSize = 0;

    //  FreeAreaBrushes();

    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pObj = m_arrObjects[l].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        {
            IRenderNode* pRenderNode = pObj;
            EERType eType = pRenderNode->GetRenderNodeType();

            if (!(nObjTypeMask & (1 << eType)))
            {
                continue;
            }

            if ((pRenderNode->GetRndFlags() & ERF_COMPONENT_ENTITY) != 0)
            {
                continue;
            }

            nBlockSize += GetSingleObjectSize(pObj, pExportInfo);
        }
    }

    if (!pMemBlock || !nBlockSize)
    {
        return nBlockSize;
    }

    pMemBlock->Allocate(nBlockSize);
    byte* pPtr = (byte*)pMemBlock->GetData();

    int nDatanSize = nBlockSize;

    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        for (IRenderNode* pRenderNode = m_arrObjects[l].m_pFirstNode; pRenderNode; pRenderNode = pRenderNode->m_pNext)
        {
            EERType eType = pRenderNode->GetRenderNodeType();

            if (!(nObjTypeMask & (1 << eType)))
            {
                continue;
            }

            if ((pRenderNode->GetRndFlags() & ERF_COMPONENT_ENTITY) != 0)
            {
                continue;
            }

            SaveSingleObject(pPtr, nDatanSize, pRenderNode, pStatObjTable, pMatTable, pStatInstGroupTable, eEndian, pExportInfo);
        }
    }

    assert(pPtr == (byte*)pMemBlock->GetData() + nBlockSize);
    assert(nDatanSize == 0);

    return nBlockSize;
# endif
}

//////////////////////////////////////////////////////////////////////////
int COctreeNode::LoadObjects(byte* pPtr, byte* pEndPtr, std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial> >* pMatTable, EEndian eEndian, int nChunkVersion, const SLayerVisibility* pLayerVisibility)
{
    while (pPtr < pEndPtr)
    {
        LoadSingleObject(pPtr, pStatObjTable, pMatTable, eEndian, nChunkVersion, pLayerVisibility, m_nSID);
    }

    return 0;
}

int COctreeNode::GetSingleObjectSize(IRenderNode* pObj, [[maybe_unused]] const SHotUpdateInfo* pExportInfo)
{
    IRenderNode* pRenderNode = pObj;
    EERType eType = pRenderNode->GetRenderNodeType();

    int nBlockSize = 0;

    if (eType == eERType_Decal  && !(pRenderNode->GetRndFlags() & ERF_PROCEDURAL))
    {
        nBlockSize += sizeof(eType);
        nBlockSize += sizeof(SDecalChunk);
    }
    else if (eType == eERType_WaterVolume)
    {
        CWaterVolumeRenderNode* pWVRN(static_cast< CWaterVolumeRenderNode* >(pRenderNode));
        const SWaterVolumeSerialize* pSerParams(pWVRN->GetSerializationParams());
        if (pSerParams && pWVRN->m_hasToBeSerialised)
        {
            nBlockSize += sizeof(eType);
            nBlockSize += sizeof(SWaterVolumeChunk);
            nBlockSize += (pSerParams->m_vertices.size() + pSerParams->m_physicsAreaContour.size()) * sizeof(SWaterVolumeVertex);
            int cntAux;
            pWVRN->GetAuxSerializationDataPtr(cntAux);
            nBlockSize += cntAux * sizeof(float);
        }
    }
    else if (eType == eERType_DistanceCloud)
    {
        nBlockSize += sizeof(eType);
        nBlockSize += sizeof(SDistanceCloudChunk);
    }
    // align to 4
    while (UINT_PTR(nBlockSize) & 3)
    {
        nBlockSize++;
    }

    return nBlockSize;
}

void COctreeNode::SaveSingleObject(byte*& pPtr, int& nDatanSize, IRenderNode* pEnt, [[maybe_unused]] std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial> >* pMatTable, [[maybe_unused]] std::vector<IStatInstGroup*>* pStatInstGroupTable, EEndian eEndian, [[maybe_unused]] const SHotUpdateInfo* pExportInfo)
{
    EERType eType = pEnt->GetRenderNodeType();

    if (eERType_Decal == eType  && !(pEnt->GetRndFlags() & ERF_PROCEDURAL))
    {
        AddToPtr(pPtr, nDatanSize, eType, eEndian);

        CDecalRenderNode* pObj((CDecalRenderNode*)pEnt);

        SDecalChunk chunk;

        CopyCommonData(&chunk, pObj);

        // decal properties
        const SDecalProperties* pDecalProperties(pObj->GetDecalProperties());
        assert(pDecalProperties);
        chunk.m_projectionType = pDecalProperties->m_projectionType;
        chunk.m_deferred = pDecalProperties->m_deferred;
        chunk.m_pos = pDecalProperties->m_pos;
        chunk.m_normal = pDecalProperties->m_normal;
        chunk.m_explicitRightUpFront = pDecalProperties->m_explicitRightUpFront;
        chunk.m_radius = pDecalProperties->m_radius;
        chunk.m_depth = pDecalProperties->m_depth;

        chunk.m_nMaterialId = CObjManager::GetItemId(pMatTable, pObj->GetMaterial());
        chunk.m_nSortPriority = pDecalProperties->m_sortPrio;

        AddToPtr(pPtr, nDatanSize, chunk, eEndian);
    }
    else if (eERType_WaterVolume == eType)
    {
        CWaterVolumeRenderNode* pObj((CWaterVolumeRenderNode*)pEnt);

        if (pObj->m_hasToBeSerialised)
        {
            // get access to serialization parameters
            const SWaterVolumeSerialize* pSerData(pObj->GetSerializationParams());
            if (pSerData)
            {
                //assert( pSerData ); // trying to save level outside the editor?

                // save type
                AddToPtr(pPtr, nDatanSize, eType, eEndian);

                // save node data
                SWaterVolumeChunk chunk;
                int cntAux;
                float* pAuxData = pObj->GetAuxSerializationDataPtr(cntAux);

                CopyCommonData(&chunk, pObj);

                chunk.m_volumeTypeAndMiscBits = (pSerData->m_volumeType & 0xFFFF) | ((pSerData->m_capFogAtVolumeDepth ? 1 : 0) << 16) | ((pSerData->m_fogColorAffectedBySun ? 0 : 1) << 17) | (cntAux << 24);
                chunk.m_volumeID = pSerData->m_volumeID;

                chunk.m_materialID = CObjManager::GetItemId(pMatTable, pSerData->m_pMaterial);

                chunk.m_fogDensity = pSerData->m_fogDensity;
                chunk.m_fogColor = pSerData->m_fogColor;
                chunk.m_fogPlane = pSerData->m_fogPlane;
                chunk.m_fogShadowing = pSerData->m_fogShadowing;

                chunk.m_caustics = pSerData->m_caustics;
                chunk.m_causticIntensity = pSerData->m_causticIntensity;
                chunk.m_causticTiling = pSerData->m_causticTiling;
                chunk.m_causticHeight = pSerData->m_causticHeight;

                chunk.m_uTexCoordBegin = pSerData->m_uTexCoordBegin;
                chunk.m_uTexCoordEnd = pSerData->m_uTexCoordEnd;
                chunk.m_surfUScale = pSerData->m_surfUScale;
                chunk.m_surfVScale = pSerData->m_surfVScale;
                chunk.m_numVertices = pSerData->m_vertices.size();

                chunk.m_volumeDepth = pSerData->m_volumeDepth;
                chunk.m_streamSpeed = pSerData->m_streamSpeed;
                chunk.m_numVerticesPhysAreaContour = pSerData->m_physicsAreaContour.size();

                AddToPtr(pPtr, nDatanSize, chunk, eEndian);
                AddToPtr(pPtr, pAuxData, cntAux, eEndian);
                nDatanSize -= cntAux * sizeof(pAuxData[0]);

                // save vertices
                for (size_t i(0); i < pSerData->m_vertices.size(); ++i)
                {
                    SWaterVolumeVertex v;
                    v.m_xyz = pSerData->m_vertices[i];

                    AddToPtr(pPtr, nDatanSize, v, eEndian);
                }

                // save physics area contour vertices
                for (size_t i(0); i < pSerData->m_physicsAreaContour.size(); ++i)
                {
                    SWaterVolumeVertex v;
                    v.m_xyz = pSerData->m_physicsAreaContour[i];

                    AddToPtr(pPtr, nDatanSize, v, eEndian);
                }
            }
        }
    }
    else if (eERType_DistanceCloud == eType)
    {
        AddToPtr(pPtr, nDatanSize, eType, eEndian);

        CDistanceCloudRenderNode* pObj((CDistanceCloudRenderNode*)pEnt);

        SDistanceCloudChunk chunk;

        CopyCommonData(&chunk, pObj);

        // distance cloud properties
        SDistanceCloudProperties properties(pObj->GetProperties());
        chunk.m_pos = properties.m_pos;
        chunk.m_sizeX = properties.m_sizeX;
        chunk.m_sizeY = properties.m_sizeY;
        chunk.m_rotationZ = properties.m_rotationZ;
        chunk.m_materialID = CObjManager::GetItemId(pMatTable, pObj->GetMaterial());

        AddToPtr(pPtr, nDatanSize, chunk, eEndian);
    }

    // align to 4
    while (UINT_PTR(pPtr) & 3)
    {
        byte emptyByte = 222;
        memcpy(pPtr, &emptyByte, sizeof(emptyByte));
        pPtr += sizeof(emptyByte);
        nDatanSize -= sizeof(emptyByte);
    }
}

void COctreeNode::LoadSingleObject(byte*& pPtr, [[maybe_unused]] std::vector<IStatObj*>* pStatObjTable, std::vector<_smart_ptr<IMaterial> >* pMatTable, EEndian eEndian, [[maybe_unused]] int nChunkVersion, const SLayerVisibility* pLayerVisibility, int nSID)
{
    EERType eType = *StepData<EERType>(pPtr, eEndian);

    // For these structures, our Endian swapping is built in to the member copy.
    if (eERType_Decal == eType)
    {
        SDecalChunk* pChunk(StepData<SDecalChunk>(pPtr, eEndian));
        if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags))
        {
            return;
        }

        if (Get3DEngine()->IsLayerSkipped(pChunk->m_nLayerId))
        {
            return;
        }

        CDecalRenderNode* pObj(new CDecalRenderNode());

        // common node data
        pObj->SetBBox(pChunk->m_WSBBox);
        LoadCommonData(pChunk, pObj, pLayerVisibility);

        // decal properties
        SDecalProperties properties;

        switch (pChunk->m_projectionType)
        {
        case SDecalProperties::eProjectOnTerrainAndStaticObjects:
        {
            properties.m_projectionType = SDecalProperties::eProjectOnTerrainAndStaticObjects;
            break;
        }
        case SDecalProperties::eProjectOnTerrain:
        {
            properties.m_projectionType = SDecalProperties::eProjectOnTerrain;
            break;
        }
        case SDecalProperties::ePlanar:
        default:
        {
            properties.m_projectionType = SDecalProperties::ePlanar;
            break;
        }
        }
        memcpy(&properties.m_pos, &pChunk->m_pos, sizeof (pChunk->m_pos));
        memcpy(&properties.m_normal, &pChunk->m_normal, sizeof (pChunk->m_normal));
        memcpy(&properties.m_explicitRightUpFront, &pChunk->m_explicitRightUpFront, sizeof (pChunk->m_explicitRightUpFront));
        memcpy(&properties.m_radius, &pChunk->m_radius, sizeof(pChunk->m_radius));
        memcpy(&properties.m_depth, &pChunk->m_depth, sizeof(pChunk->m_depth));

        _smart_ptr<IMaterial> pMaterial(CObjManager::GetItemPtr(pMatTable, pChunk->m_nMaterialId));
        assert(pMaterial);
        properties.m_pMaterialName = pMaterial ? pMaterial->GetName() : "";
        properties.m_sortPrio = pChunk->m_nSortPriority;
        properties.m_deferred = pChunk->m_deferred;

        pObj->SetDecalProperties(properties);

        // set object visibility
        if (NULL != pLayerVisibility)
        {
            CRY_ASSERT(pChunk->m_nLayerId != 0);
            if (!CheckLayerVisibility(pObj->GetLayerId(), pLayerVisibility))
            {
                pObj->SetRndFlags(ERF_HIDDEN, true);
            }
        }
        else if (Get3DEngine()->IsAreaActivationInUse())
        {
            // keep everything deactivated, game will activate it later
            pObj->SetRndFlags(ERF_HIDDEN, true);
        }

        //if( pObj->GetMaterialID() != pChunk->m_materialID )
        if (pObj->GetMaterial() != pMaterial)
        {
            const char* pMatName("_can't_resolve_material_name_");
            int32 matID(pChunk->m_nMaterialId);
            if (matID >= 0 && matID < (int)pMatTable->size())
            {
                pMatName = pMaterial ? pMaterial->GetName() : "";
            }
            Warning("Warning: Removed placement decal at (%4.2f, %4.2f, %4.2f) with invalid material \"%s\"!\n", pChunk->m_pos.x, pChunk->m_pos.y, pChunk->m_pos.z, pMatName);
            pObj->ReleaseNode();
        }
        else
        {
            Get3DEngine()->RegisterEntity(pObj, nSID, nSID);
            GetObjManager()->GetDecalsToPrecreate().push_back(pObj);
        }
    }
    else if (eERType_WaterVolume == eType)
    {
        // read common info
        SWaterVolumeChunk* pChunk(StepData<SWaterVolumeChunk>(pPtr, eEndian));
        if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags) || Get3DEngine()->IsLayerSkipped(pChunk->m_nLayerId))
        {
            for (uint32 j(0); j < pChunk->m_numVertices; ++j)
            {
                StepData<SWaterVolumeVertex>(pPtr, eEndian);
            }

            for (uint32 j(0); j < pChunk->m_numVerticesPhysAreaContour; ++j)
            {
                StepData<SWaterVolumeVertex>(pPtr, eEndian);
            }

            return;
        }

        CWaterVolumeRenderNode* pObj(new CWaterVolumeRenderNode());

        int auxCntSrc = pChunk->m_volumeTypeAndMiscBits >> 24, auxCntDst;
        float* pAuxDataDst = pObj->GetAuxSerializationDataPtr(auxCntDst);
        StepData<float>(pPtr, auxCntSrc, eEndian);
        memcpy(pAuxDataDst, pAuxDataDst, min(auxCntSrc, auxCntDst) * sizeof(float));

        // read common node data
        pObj->SetBBox(pChunk->m_WSBBox);
        LoadCommonData(pChunk, pObj, pLayerVisibility);

        // read vertices
        std::vector<Vec3> vertices;
        vertices.reserve(pChunk->m_numVertices);
        for (uint32 j(0); j < pChunk->m_numVertices; ++j)
        {
            SWaterVolumeVertex* pVertex(StepData<SWaterVolumeVertex>(pPtr, eEndian));
            vertices.push_back(pVertex->m_xyz);
        }

        // read physics area contour vertices
        std::vector<Vec3> physicsAreaContour;
        physicsAreaContour.reserve(pChunk->m_numVerticesPhysAreaContour);
        for (uint32 j(0); j < pChunk->m_numVerticesPhysAreaContour; ++j)
        {
            SWaterVolumeVertex* pVertex(StepData<SWaterVolumeVertex>(pPtr, eEndian));
            physicsAreaContour.push_back(pVertex->m_xyz);
        }

        const int volumeType = pChunk->m_volumeTypeAndMiscBits & 0xFFFF;
        const bool capFogAtVolumeDepth = ((pChunk->m_volumeTypeAndMiscBits & 0x10000) != 0) ? true : false;
        const bool fogColorAffectedBySun = ((pChunk->m_volumeTypeAndMiscBits & 0x20000) == 0) ? true : false;
        const bool enableCaustics = (pChunk->m_caustics != 0) ? true : false;

        // create volume
        switch (volumeType)
        {
        case IWaterVolumeRenderNode::eWVT_River:
        {
            pObj->CreateRiver(pChunk->m_volumeID, &vertices[0], pChunk->m_numVertices, pChunk->m_uTexCoordBegin, pChunk->m_uTexCoordEnd, Vec2(pChunk->m_surfUScale, pChunk->m_surfVScale), pChunk->m_fogPlane, false, nSID);
            break;
        }
        case IWaterVolumeRenderNode::eWVT_Area:
        {
            pObj->CreateArea(pChunk->m_volumeID, &vertices[0], pChunk->m_numVertices, Vec2(pChunk->m_surfUScale, pChunk->m_surfVScale),  pChunk->m_fogPlane, false, nSID);
            break;
        }
        case IWaterVolumeRenderNode::eWVT_Ocean:
        {
            assert(!"COctreeNode::SerializeObjects( ... ) -- Water volume of type \"Ocean\" not supported yet!");
            break;
        }
        default:
        {
            assert(!"COctreeNode::SerializeObjects( ... ) -- Invalid water volume type!");
            break;
        }
        }

        // set material
        _smart_ptr<IMaterial> pMaterial(CObjManager::GetItemPtr(pMatTable, pChunk->m_materialID));


        // set properties
        pObj->SetFogDensity(pChunk->m_fogDensity);
        pObj->SetFogColor(pChunk->m_fogColor);
        pObj->SetFogColorAffectedBySun(fogColorAffectedBySun);
        pObj->SetFogShadowing(pChunk->m_fogShadowing);

        pObj->SetVolumeDepth(pChunk->m_volumeDepth);
        pObj->SetStreamSpeed(pChunk->m_streamSpeed);
        pObj->SetCapFogAtVolumeDepth(capFogAtVolumeDepth);

        pObj->SetCaustics(enableCaustics);
        pObj->SetCausticIntensity(pChunk->m_causticIntensity);
        pObj->SetCausticTiling(pChunk->m_causticTiling);
        pObj->SetCausticHeight(pChunk->m_causticHeight);

        // set object visibility
        if (NULL != pLayerVisibility)
        {
            CRY_ASSERT(pChunk->m_nLayerId != 0);
            if (!CheckLayerVisibility(pObj->GetLayerId(), pLayerVisibility))
            {
                pObj->SetRndFlags(ERF_HIDDEN, true);
            }
        }
        else if (Get3DEngine()->IsAreaActivationInUse())
        {
            // keep everything deactivated, game will activate it later
            pObj->SetRndFlags(ERF_HIDDEN, true);
        }

        // set physics
        if (!physicsAreaContour.empty())
        {
            switch (volumeType)
            {
            case IWaterVolumeRenderNode::eWVT_River:
            {
                pObj->SetRiverPhysicsArea(&physicsAreaContour[0], physicsAreaContour.size());
                break;
            }
            case IWaterVolumeRenderNode::eWVT_Area:
            {
                pObj->SetAreaPhysicsArea(&physicsAreaContour[0], physicsAreaContour.size());
                break;
            }
            case IWaterVolumeRenderNode::eWVT_Ocean:
            {
                assert(!"COctreeNode::SerializeObjects( ... ) -- Water volume of type \"Ocean\" not supported yet!");
                break;
            }
            default:
            {
                assert(!"COctreeNode::SerializeObjects( ... ) -- Invalid water volume type!");
                break;
            }
            }

            if (!(Get3DEngine()->IsAreaActivationInUse() && GetCVars()->e_ObjectLayersActivationPhysics) && !(pChunk->m_dwRndFlags & ERF_NO_PHYSICS))
            {
                pObj->Physicalize();
            }
        }

        // set material
        pObj->SetMaterial(pMaterial);

        Get3DEngine()->RegisterEntity(pObj, nSID, nSID);
    }
    else if (eERType_DistanceCloud == eType)
    {
        SDistanceCloudChunk* pChunk(StepData<SDistanceCloudChunk>(pPtr, eEndian));
        if (!CheckRenderFlagsMinSpec(pChunk->m_dwRndFlags))
        {
            return;
        }

        if (Get3DEngine()->IsLayerSkipped(pChunk->m_nLayerId))
        {
            return;
        }

        CDistanceCloudRenderNode* pObj(new CDistanceCloudRenderNode());

        // common node data
        AABB box;
        memcpy(&box, &pChunk->m_WSBBox, sizeof(AABB));
        pObj->SetBBox(box);
        LoadCommonData(pChunk, pObj, pLayerVisibility);

        // distance cloud properties
        SDistanceCloudProperties properties;

        properties.m_pos = pChunk->m_pos;
        properties.m_sizeX = pChunk->m_sizeX;
        properties.m_sizeY = pChunk->m_sizeY;
        properties.m_rotationZ = pChunk->m_rotationZ;

        _smart_ptr<IMaterial> pMaterial(CObjManager::GetItemPtr(pMatTable, pChunk->m_materialID));
        assert(pMaterial);
        properties.m_pMaterialName = pMaterial ? pMaterial->GetName() : "";

        pObj->SetProperties(properties);

        // set object visibility
        if (NULL != pLayerVisibility)
        {
            CRY_ASSERT(pChunk->m_nLayerId != 0);
            if (!CheckLayerVisibility(pObj->GetLayerId(), pLayerVisibility))
            {
                pObj->SetRndFlags(ERF_HIDDEN, true);
            }
        }
        else if (Get3DEngine()->IsAreaActivationInUse())
        {
            // keep everything deactivated, game will activate it later
            pObj->SetRndFlags(ERF_HIDDEN, true);
        }

        if (pObj->GetMaterial() != pMaterial)
        {
            const char* pMatName("_can't_resolve_material_name_");
            int32 matID(pChunk->m_materialID);
            if (matID >= 0 && matID < (int32)(*pMatTable).size())
            {
                pMatName = pMaterial ? pMaterial->GetName() : "";
            }
            Warning("Warning: Removed distance cloud at (%4.2f, %4.2f, %4.2f) with invalid material \"%s\"!\n", pChunk->m_pos.x, pChunk->m_pos.y, pChunk->m_pos.z, pMatName);
            pObj->ReleaseNode();
        }
        else
        {
            Get3DEngine()->RegisterEntity(pObj, nSID, nSID);
        }
    }
    else
    {
        assert(!"Unsupported object type");
    }

    // align to 4
    while (UINT_PTR(pPtr) & 3)
    {
        assert(*pPtr == 222);
        pPtr++;
    }
}

