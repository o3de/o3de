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

// Description : Implementation of I3DEngine interface methods


#include "Cry3DEngine_precompiled.h"
#include <MathConversion.h>

#include "3dEngine.h"

#include <AzFramework/Terrain/TerrainDataRequestBus.h>

#include "VisAreas.h"
#include "ObjMan.h"
#include "Ocean.h"

#include "DecalManager.h"
#include "IndexedMesh.h"
#include "AABBSV.h"

#include "MatMan.h"

#include "CullBuffer.h"
#include "CGF/CGFLoader.h"
#include "CGF/ReadOnlyChunkFile.h"

#include "CloudRenderNode.h"
#include "CloudsManager.h"
#include "SkyLightManager.h"
#include "FogVolumeRenderNode.h"
#include "DecalRenderNode.h"
#include "TimeOfDay.h"
#include "LightEntity.h"
#include "FogVolumeRenderNode.h"
#include "ObjectsTree.h"
#include "WaterVolumeRenderNode.h"
#include "DistanceCloudRenderNode.h"
#include "VolumeObjectRenderNode.h"
#include "RenderMeshMerger.h"
#include "DeferredCollisionEvent.h"
#include "OpticsManager.h"
#include "ClipVolumeManager.h"
#include "Environment/OceanEnvironmentBus.h"


#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
#include "PrismRenderNode.h"
#endif // EXCLUDE_DOCUMENTATION_PURPOSE

//Platform specific includes
#if defined(WIN32) || defined(WIN64)
#include "CryWindows.h"
#endif

///////////////////////////////////////////////////////////////////////////////
void C3DEngine::CheckAddLight(CDLight* pLight, const SRenderingPassInfo& passInfo)
{
    if (pLight->m_Id < 0)
    {
        GetRenderer()->EF_ADDDlight(pLight, passInfo);
        assert(pLight->m_Id >= 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
float C3DEngine::GetLightAmount(CDLight* pLight, const AABB& objBox)
{
    // find amount of light
    float fDist = sqrt_tpl(Distance::Point_AABBSq(pLight->m_Origin, objBox));
    float fLightAttenuation = (pLight->m_Flags & DLF_DIRECTIONAL) ? 1.f : 1.f - (fDist) / (pLight->m_fRadius);
    if (fLightAttenuation < 0)
    {
        fLightAttenuation = 0;
    }

    float fLightAmount =
        (pLight->m_Color.r + pLight->m_Color.g + pLight->m_Color.b) * 0.233f +
        (pLight->GetSpecularMult()) * 0.1f;

    return fLightAmount * fLightAttenuation;
}

///////////////////////////////////////////////////////////////////////////////
float C3DEngine::GetWaterLevel()
{
    if (OceanToggle::IsActive())
    {
        return OceanRequest::GetOceanLevel();
    }
    return m_pOcean ? m_pOcean->GetWaterLevel() : WATER_LEVEL_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////
bool C3DEngine::IsTessellationAllowed(const CRenderObject* pObj, const SRenderingPassInfo& passInfo, bool bIgnoreShadowPass) const
{
#ifdef MESH_TESSELLATION_ENGINE
    assert(pObj && GetCVars());
    bool rendererTessellation;
    GetRenderer()->EF_Query(EFQ_MeshTessellation, rendererTessellation);
    if (pObj->m_fDistance < GetCVars()->e_TessellationMaxDistance
        && GetCVars()->e_Tessellation
        && rendererTessellation
        && !(pObj->m_ObjFlags & FOB_DISSOLVE)) // dissolve is not working with tessellation for now
    {
        bool bAllowTessellation = true;

        // Check if rendering into shadow map and enable tessellation only if allowed
        if (!bIgnoreShadowPass && passInfo.IsShadowPass())
        {
            if (IsTessellationAllowedForShadowMap(passInfo))
            {
                // NOTE: This might be useful for game projects
                // Use tessellation only for objects visible in main view
                // Shadows will switch to non-tessellated when caster gets out of view
                IRenderNode* pRN = (IRenderNode*)pObj->m_pRenderNode;
                if (pRN)
                {
                    bAllowTessellation = (pRN->IsRenderNode() && (pRN->GetDrawFrame() > passInfo.GetFrameID() - 10));
                }
            }
            else
            {
                bAllowTessellation = false;
            }
        }

        return bAllowTessellation;
    }
#endif //#ifdef MESH_TESSELLATION_ENGINE

    return false;
}

///////////////////////////////////////////////////////////////////////////////
void C3DEngine::CreateRNTmpData(CRNTmpData** ppInfo, IRenderNode* pRNode, const SRenderingPassInfo& passInfo)
{
    // m_checkCreateRNTmpData lock scope
    {
        AUTO_LOCK(m_checkCreateRNTmpData);
        FUNCTION_PROFILER_3DENGINE;

        if (*ppInfo)
        {
            return; // check if another thread already intialized ppInfo
        }
        // make sure element is allocated
        if (m_LTPRootFree.pNext == &m_LTPRootFree)
        {
            CRNTmpData* pNew = new CRNTmpData; //m_RNTmpDataPools.GetNewElement();
            pNew->Link(&m_LTPRootFree);
        }

        // move element from m_LTPRootFree to m_LTPRootUsed
        CRNTmpData* pElem = m_LTPRootFree.pNext;
        pElem->Unlink();
        pElem->Link(&m_LTPRootUsed);

        pElem->pOwnerRef = ppInfo;
        pElem->nFrameInfoId = GetFrameInfoId(ppInfo, passInfo.GetMainFrameID());

        assert(!pElem->pOwnerRef || !(*pElem->pOwnerRef));
        memset(&pElem->userData, 0, sizeof(pElem->userData));


        // Add a memory barrier that the write to nFrameInfoID is visible
        // before *ppInfo is written, else we have a race condtition in
        // CheckCreateRNTmpData as we don't use a lock there
        // for performance reasons
        MemoryBarrier();

        *ppInfo = pElem;
    }

    if (pRNode)
    {
        pRNode->OnRenderNodeBecomeVisible(passInfo); // Internally uses the just assigned RNTmpData pointer i.e IRenderNode::m_pRNTmpData ...

        if (IVisArea* pVisArea = pRNode->GetEntityVisArea())
        {
            pRNode->m_pRNTmpData->userData.m_pClipVolume = pVisArea;
        }
        else if (GetClipVolumeManager()->IsClipVolumeRequired(pRNode))
        {
            GetClipVolumeManager()->UpdateEntityClipVolume(pRNode->GetPos(), pRNode);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void C3DEngine::RenderRenderNode_ShadowPass(IShadowCaster* pShadowCaster, const SRenderingPassInfo& passInfo, [[maybe_unused]] AZ::LegacyJobExecutor* pJobExecutor)
{
    assert(passInfo.IsShadowPass());

    SRendItemSorter rendItemSorter = SRendItemSorter::CreateShadowPassRendItemSorter(passInfo);

    if (!pShadowCaster->IsRenderNode())
    {
        const Vec3 vCamPos = passInfo.GetCamera().GetPosition();
        const AABB objBox = pShadowCaster->GetBBoxVirtual();

        SRendParams rParams;
        rParams.fDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, objBox)) * passInfo.GetZoomFactor();
        rParams.lodValue = pShadowCaster->ComputeLod(0, passInfo);
        rParams.rendItemSorter = rendItemSorter.GetValue();

        pShadowCaster->Render(rParams, passInfo);
        return;
    }

    IRenderNode* pRenderNode = static_cast<IRenderNode*>(pShadowCaster);
    if ((pRenderNode->m_dwRndFlags & ERF_HIDDEN) != 0)
    {
        return;
    }

    int nStaticObjectLod = -1;
    if (passInfo.GetShadowMapType() == SRenderingPassInfo::SHADOW_MAP_CACHED)
    {
        nStaticObjectLod = GetCVars()->e_ShadowsCacheObjectLod;
    }
    else if (passInfo.GetShadowMapType() == SRenderingPassInfo::SHADOW_MAP_CACHED_MGPU_COPY)
    {
        nStaticObjectLod = pRenderNode->m_cStaticShadowLod;
    }

    Get3DEngine()->CheckCreateRNTmpData(&pRenderNode->m_pRNTmpData, pRenderNode, passInfo);

    int wantedLod = pRenderNode->m_pRNTmpData->userData.nWantedLod;

    if (GetCVars()->e_LodForceUpdate && m_pObjManager)
    {
        const Vec3 vCamPos = passInfo.GetCamera().GetPosition();
        const AABB objBox = pRenderNode->GetBBoxVirtual();
        float fDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, objBox)) * passInfo.GetZoomFactor();
        wantedLod = m_pObjManager->GetObjectLOD(pRenderNode, fDistance);
    }

    if (pRenderNode->GetShadowLodBias() != IRenderNode::SHADOW_LODBIAS_DISABLE)
    {
        if (passInfo.IsShadowPass() && (pRenderNode->GetDrawFrame(0) < (passInfo.GetFrameID() - 10)))
        {
            wantedLod += GetCVars()->e_ShadowsLodBiasInvis;
        }
        wantedLod += GetCVars()->e_ShadowsLodBiasFixed;
        wantedLod += pRenderNode->GetShadowLodBias();
    }

    if (nStaticObjectLod >= 0)
    {
        wantedLod = nStaticObjectLod;
    }

    {
        const Vec3 vCamPos = passInfo.GetCamera().GetPosition();
        const AABB objBox = pRenderNode->GetBBoxVirtual();
        SRendParams rParams;
        rParams.fDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, objBox)) * passInfo.GetZoomFactor();
        rParams.lodValue = pRenderNode->ComputeLod(wantedLod, passInfo);
        rParams.rendItemSorter = rendItemSorter.GetValue();
        rParams.pRenderNode = pRenderNode;
        pRenderNode->Render(rParams, passInfo);
    }
}

///////////////////////////////////////////////////////////////////////////////
ITimeOfDay* C3DEngine::GetTimeOfDay()
{
    CTimeOfDay* tod = m_pTimeOfDay;

    if (!tod)
    {
        tod = new CTimeOfDay;
        m_pTimeOfDay = tod;
    }

    return tod;
}

///////////////////////////////////////////////////////////////////////////////
void C3DEngine::TraceFogVolumes(const Vec3& vPos, const AABB& objBBox, SFogVolumeData& fogVolData, const SRenderingPassInfo& passInfo, bool fogVolumeShadingQuality)
{
    CFogVolumeRenderNode::TraceFogVolumes(vPos, objBBox, fogVolData, passInfo, fogVolumeShadingQuality);
}

///////////////////////////////////////////////////////////////////////////////
void C3DEngine::AsyncOctreeUpdate(IRenderNode* pEnt, int nSID, [[maybe_unused]] int nSIDConsideredSafe, uint32 nFrameID, bool bUnRegisterOnly)
{
    FUNCTION_PROFILER_3DENGINE;

#ifdef _DEBUG // crash test basically
    const char* szClass = pEnt->GetEntityClassName();
    const char* szName = pEnt->GetName();
    if (!szName[0] && !szClass[0])
    {
        Warning("I3DEngine::RegisterEntity: Entity undefined"); // do not register undefined objects
    }
    //  if(strstr(szName,"Dude"))
    //  int y=0;
#endif

    IF (bUnRegisterOnly, 0)
    {
        UnRegisterEntityImpl(pEnt);
        return;
    }
    ;

    AABB aabb;
    pEnt->FillBBox(aabb);
    float fObjRadiusSqr = aabb.GetRadiusSqr();
    EERType eERType = pEnt->GetRenderNodeType();

#ifdef SUPP_HMAP_OCCL
    if (pEnt->m_pRNTmpData)
    {
        pEnt->m_pRNTmpData->userData.m_OcclState.vLastVisPoint.Set(0, 0, 0);
    }
#endif


    const unsigned int dwRndFlags = pEnt->GetRndFlags();

    if (!(dwRndFlags & ERF_RENDER_ALWAYS) && !(dwRndFlags & ERF_CASTSHADOWMAPS))
    {
        if (GetCVars()->e_ObjFastRegister && pEnt->m_pOcNode && ((COctreeNode*)pEnt->m_pOcNode)->IsRightNode(aabb, fObjRadiusSqr, pEnt->m_fWSMaxViewDist))
        { // same octree node
            Vec3 vEntCenter = GetEntityRegisterPoint(pEnt);

            IVisArea* pVisArea = pEnt->GetEntityVisArea();
            if (pVisArea && pVisArea->IsPointInsideVisArea(vEntCenter))
            {
                return; // same visarea
            }
            IVisArea* pVisAreaFromPos = (!m_pVisAreaManager || dwRndFlags & ERF_OUTDOORONLY) ? NULL : GetVisAreaManager()->GetVisAreaFromPos(vEntCenter);
            if (pVisAreaFromPos == pVisArea)
            {
                // NOTE: can only get here when pVisArea==NULL due to 'same visarea' check above. So check for changed clip volume
                if (GetClipVolumeManager()->IsClipVolumeRequired(pEnt))
                {
                    GetClipVolumeManager()->UpdateEntityClipVolume(vEntCenter, pEnt);
                }

                return; // same visarea or same outdoor
            }
        }
    }

    if (pEnt->m_pOcNode)
    {
        UnRegisterEntityImpl(pEnt);
    }
    else if (GetCVars()->e_StreamCgf && (eERType == eERType_RenderComponent || eERType == eERType_DynamicMeshRenderComponent || eERType == eERType_GeomCache))
    { //  Temporary solution: Force streaming priority update for objects that was not registered before
      //  and was not visible before since usual prediction system was not able to detect them

        if ((uint32)pEnt->GetDrawFrame(0) < nFrameID - 16)
        {
            // defer the render node streaming priority update still we have a correct 3D Engine camera
            int nElementID = m_deferredRenderComponentStreamingPriorityUpdates.Find(pEnt);
            if (nElementID == -1)  // only add elements once
            {
                m_deferredRenderComponentStreamingPriorityUpdates.push_back(pEnt);
            }
        }
    }

    pEnt->m_fWSMaxViewDist = pEnt->GetMaxViewDist();

    bool useVisAreas = true;

    if (eERType != eERType_Light)
    {
        if (fObjRadiusSqr > sqr(MAX_VALID_OBJECT_VOLUME) || !_finite(fObjRadiusSqr))
        {
            Warning("I3DEngine::RegisterEntity: Object has invalid bbox: name: %s, class name: %s, GetRadius() = %.2f",
                pEnt->GetName(), pEnt->GetEntityClassName(), fObjRadiusSqr);
            return; // skip invalid objects - usually only objects with invalid very big scale will reach this point
        }

        if (dwRndFlags & ERF_RENDER_ALWAYS)
        {
            if (m_lstAlwaysVisible.Find(pEnt) < 0)
            {
                m_lstAlwaysVisible.Add(pEnt);
            }

            if (dwRndFlags & ERF_HUD)
            {
                return;
            }
        }

        if (pEnt->m_dwRndFlags & ERF_OUTDOORONLY)
        {
            useVisAreas = false;
        }
    }
    else
    {
        CLightEntity* pLight = (CLightEntity*)pEnt;
        uint32 lightFlag = pLight->m_light.m_Flags;
        if ((lightFlag & DLF_ATTACH_TO_SUN) || //If the light is attached to the sun, we need to make sure it renders even the entity is not in view port
            (lightFlag & (DLF_IGNORES_VISAREAS | DLF_DEFERRED_LIGHT | DLF_THIS_AREA_ONLY)) == (DLF_IGNORES_VISAREAS | DLF_DEFERRED_LIGHT)
            )
        {
            if (m_lstAlwaysVisible.Find(pEnt) < 0)
            {
                m_lstAlwaysVisible.Add(pEnt);
            }
        }

        if (lightFlag & DLF_IGNORES_VISAREAS)
        {
            useVisAreas = false;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Check for occlusion proxy.
    {
        CStatObj* pStatObj = (CStatObj*)pEnt->GetEntityStatObj();
        if (pStatObj)
        {
            if (pStatObj->m_bHaveOcclusionProxy)
            {
                pEnt->m_dwRndFlags |=   ERF_GOOD_OCCLUDER;
                pEnt->m_nInternalFlags |=   IRenderNode::HAS_OCCLUSION_PROXY;
            }
        }
    }
    //////////////////////////////////////////////////////////////////////////
    if (!useVisAreas || !(m_pVisAreaManager && m_pVisAreaManager->SetEntityArea(pEnt, aabb, fObjRadiusSqr)))
    {
        if (m_pObjectsTree == nullptr)
        {
            AZ::Aabb terrainAabb = AZ::Aabb::CreateFromPoint(AZ::Vector3::CreateZero());
            AzFramework::Terrain::TerrainDataRequestBus::BroadcastResult(terrainAabb, &AzFramework::Terrain::TerrainDataRequests::GetTerrainAabb);
            m_pObjectsTree = COctreeNode::Create(nSID, AABB(Vec3(0, 0, 0), Vec3(terrainAabb.GetXExtent(), terrainAabb.GetYExtent(), terrainAabb.GetZExtent())), NULL);
        }

        m_pObjectsTree->InsertObject(pEnt, aabb, fObjRadiusSqr, aabb.GetCenter());
    }

    // update clip volume: use vis area if we have one, otherwise check if we're in the same volume as before. check other volumes as last resort only
    if (pEnt->m_pRNTmpData)
    {
        Vec3 vEntCenter = GetEntityRegisterPoint(pEnt);
        CRNTmpData::SRNUserData& userData = pEnt->m_pRNTmpData->userData;

        if (IVisArea* pVisArea = pEnt->GetEntityVisArea())
        {
            userData.m_pClipVolume = pVisArea;
        }
        else if (GetClipVolumeManager()->IsClipVolumeRequired(pEnt))
        {
            GetClipVolumeManager()->UpdateEntityClipVolume(vEntCenter, pEnt);
        }
    }

    // register decals, to clean up longer not renderes decals and their render meshes
    if (eERType == eERType_Decal)
    {
        m_decalRenderNodes.push_back((IDecalRenderNode*)pEnt);
    }
}

///////////////////////////////////////////////////////////////////////////////
bool C3DEngine::UnRegisterEntityImpl(IRenderNode* pEnt)
{
    // make sure we don't try to update the streaming priority if an object
    // was added and removed in the same frame
    int nElementID = m_deferredRenderComponentStreamingPriorityUpdates.Find(pEnt);
    if (nElementID != -1)
    {
        m_deferredRenderComponentStreamingPriorityUpdates.DeleteFastUnsorted(nElementID);
    }

    FUNCTION_PROFILER_3DENGINE;

#ifdef _DEBUG // crash test basically
    const char* szClass = pEnt->GetEntityClassName();
    const char* szName = pEnt->GetName();
    if (!szName[0] && !szClass[0])
    {
        Warning("C3DEngine::RegisterEntity: Entity undefined");
    }
#endif

    EERType eRenderNodeType = pEnt->GetRenderNodeType();

    bool bFound = false;

    if (pEnt->m_pOcNode)
    {
        bFound = ((COctreeNode*)pEnt->m_pOcNode)->DeleteObject(pEnt);
    }

    if (pEnt->m_dwRndFlags & ERF_RENDER_ALWAYS || (eRenderNodeType == eERType_Light) || (eRenderNodeType == eERType_FogVolume))
    {
        m_lstAlwaysVisible.Delete(pEnt);
    }

    if (eRenderNodeType == eERType_Decal)
    {
        std::vector<IDecalRenderNode*>::iterator it = std::find(m_decalRenderNodes.begin(), m_decalRenderNodes.end(), (IDecalRenderNode*)pEnt);
        if (it != m_decalRenderNodes.end())
        {
            m_decalRenderNodes.erase(it);
        }
    }

    if (CClipVolumeManager* pClipVolumeManager = GetClipVolumeManager())
    {
        pClipVolumeManager->UnregisterRenderNode(pEnt);
    }

    return bFound;
}

///////////////////////////////////////////////////////////////////////////////
Vec3 C3DEngine::GetEntityRegisterPoint(IRenderNode* pEnt)
{
    AABB aabb;
    pEnt->FillBBox(aabb);

    Vec3 vPoint;

    if (pEnt->m_dwRndFlags & ERF_REGISTER_BY_POSITION)
    {
        vPoint = pEnt->GetPos();

        if (pEnt->GetRenderNodeType() != eERType_Light)
        {
            // check for valid position
            if (aabb.GetDistanceSqr(vPoint) > sqr(128.f))
            {
                Warning("I3DEngine::RegisterEntity: invalid entity position: Name: %s, Class: %s, Pos=(%.1f,%.1f,%.1f), BoxMin=(%.1f,%.1f,%.1f), BoxMax=(%.1f,%.1f,%.1f)",
                    pEnt->GetName(), pEnt->GetEntityClassName(),
                    pEnt->GetPos().x, pEnt->GetPos().y, pEnt->GetPos().z,
                    pEnt->GetBBox().min.x, pEnt->GetBBox().min.y, pEnt->GetBBox().min.z,
                    pEnt->GetBBox().max.x, pEnt->GetBBox().max.y, pEnt->GetBBox().max.z
                    );
            }
            // clamp by bbox
            vPoint.CheckMin(aabb.max);
            vPoint.CheckMax(aabb.min + Vec3(0, 0, .5f));
        }
    }
    else
    {
        vPoint = aabb.GetCenter();
    }

    return vPoint;
}

///////////////////////////////////////////////////////////////////////////////
Vec3 C3DEngine::GetSunDirNormalized() const
{
    return m_vSunDirNormalized;
}


