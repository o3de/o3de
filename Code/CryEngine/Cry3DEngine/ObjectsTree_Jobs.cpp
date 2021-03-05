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

// Description : Octree parts used on Jobs to prevent scanning too many files for micro functions

#include "Cry3DEngine_precompiled.h"

#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "IndexedMesh.h"
#include "ObjectsTree.h"

#include "DecalRenderNode.h"
#include "FogVolumeRenderNode.h"
#include "Ocean.h"
#include "LightEntity.h"
#include "WaterVolumeRenderNode.h"
#include "DistanceCloudRenderNode.h"
#include "Environment/OceanEnvironmentBus.h"

#define CHECK_OBJECTS_BOX_WARNING_SIZE (1.0e+10f)
#define fNodeMinSize (8.f)
#define fObjectToNodeSizeRatio (1.f / 8.f)
#define fMinShadowCasterViewDist (8.f)

namespace LegacyInternal
{
    // File scoped LegacyJobExecutor instance used to run all RenderContent jobs
    static AZ::LegacyJobExecutor* s_renderContentJobExecutor;
};

//////////////////////////////////////////////////////////////////////////
COctreeNode::COctreeNode(int nSID, const AABB& box, CVisArea* pVisArea, COctreeNode* pParent)
    : m_nOccludedFrameId(0), m_renderFlags(0), m_errTypesBitField(0), m_fObjectsMaxViewDist(0.0f), m_nLastVisFrameId(0)
    , nFillShadowCastersSkipFrameId(0), m_fNodeDistance(0.0f), m_nManageVegetationsFrameId(0)
    , m_bHasLights(0), m_bNodeCompletelyInFrustum(0)
{
    memset(m_arrChilds, 0, sizeof(m_arrChilds));
    memset(m_arrObjects, 0, sizeof(m_arrObjects));
    memset(&m_lstCasters, 0, sizeof(m_lstCasters));

    m_pRNTmpData = NULL;
    m_nSID = nSID;
    m_vNodeCenter = box.GetCenter();
    m_vNodeAxisRadius = box.GetSize() * 0.5f;
    m_objectsBox.min = box.max;
    m_objectsBox.max = box.min;

#if !defined(_RELEASE)
    // Check if bounding box is crazy
#define CHECK_OBJECTS_BOX_WARNING_SIZE (1.0e+10f)
    if (GetCVars()->e_CheckOctreeObjectsBoxSize != 0) // pParent is checked as silly sized things are added to the root (e.g. the sun)
    {
        if (pParent && !m_objectsBox.IsReset() && (m_objectsBox.min.len() > CHECK_OBJECTS_BOX_WARNING_SIZE || m_objectsBox.max.len() > CHECK_OBJECTS_BOX_WARNING_SIZE))
        {
            CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR_DBGBRK, "COctreeNode being created with a huge m_objectsBox: [%f %f %f] -> [%f %f %f]\n", m_objectsBox.min.x, m_objectsBox.min.y, m_objectsBox.min.z, m_objectsBox.max.x, m_objectsBox.max.y, m_objectsBox.max.z);
        }
    }
#endif

    m_pVisArea = pVisArea;
    m_pParent = pParent;

    m_fpSunDirX = 63;
    m_fpSunDirZ = 0;
    m_fpSunDirYs = 0;

    m_pStaticInstancingInfo = nullptr;
    m_bStaticInstancingIsDirty = false;
}

//////////////////////////////////////////////////////////////////////////
COctreeNode* COctreeNode::Create(int nSID, const AABB& box, struct CVisArea* pVisArea, COctreeNode* pParent)
{
    return new COctreeNode(nSID, box, pVisArea, pParent);
}

//////////////////////////////////////////////////////////////////////////
bool COctreeNode::HasObjects()
{
    for (int l = 0; l < eRNListType_ListsNum; l++)
    {
        if (m_arrObjects[l].m_pFirstNode)
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::RenderContent(int nRenderMask, const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter, const CCamera* pCam)
{
    if (GetCVars()->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
    {
        GetObjManager()->AddCullJobProducer();
    }

    if (!LegacyInternal::s_renderContentJobExecutor)
    {
        LegacyInternal::s_renderContentJobExecutor = new AZ::LegacyJobExecutor;
    }

    LegacyInternal::s_renderContentJobExecutor->StartJob(
        [this, nRenderMask, passInfo, rendItemSorter, pCam]
        {
            this->RenderContentJobEntry(nRenderMask, passInfo, rendItemSorter, pCam);
        }
    );
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::Shutdown()
{
    WaitForContentJobCompletion();
}

void COctreeNode::WaitForContentJobCompletion()
{
    //Deleting it calls WaitForCompletion(), and the next call to RenderContent() will create a new instance
    delete LegacyInternal::s_renderContentJobExecutor;
    LegacyInternal::s_renderContentJobExecutor = nullptr;
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::RenderContentJobEntry(int nRenderMask, const SRenderingPassInfo& passInfo, SRendItemSorter rendItemSorter, const CCamera* pCam)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ThreeDEngine);

    SSectorTextureSet* pTerrainTexInfo = NULL;

    if (m_arrObjects[eRNListType_DecalsAndRoads].m_pFirstNode && (passInfo.RenderDecals()))
    {
        RenderDecalsAndRoads(&m_arrObjects[eRNListType_DecalsAndRoads], *pCam, nRenderMask, m_bNodeCompletelyInFrustum != 0, pTerrainTexInfo, passInfo, rendItemSorter);
    }

    if (m_arrObjects[eRNListType_Unknown].m_pFirstNode)
    {
        RenderCommonObjects(&m_arrObjects[eRNListType_Unknown], *pCam, nRenderMask, m_bNodeCompletelyInFrustum != 0, pTerrainTexInfo, passInfo, rendItemSorter);
    }

    if (GetCVars()->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
    {
        GetObjManager()->RemoveCullJobProducer();
    }
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::RenderDecalsAndRoads(TDoublyLinkedList<IRenderNode>* lstObjects, const CCamera& rCam, [[maybe_unused]] int nRenderMask, bool bNodeCompletelyInFrustum, [[maybe_unused]] SSectorTextureSet* pTerrainTexInfo, const SRenderingPassInfo& passInfo, SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;

    CVars* pCVars = GetCVars();
    AABB objBox;
    const Vec3 vCamPos = rCam.GetPosition();

    bool bCheckPerObjectOcclusion = true;

    for (IRenderNode* pObj = lstObjects->m_pFirstNode, * pNext; pObj; pObj = pNext)
    {
        rendItemSorter.IncreaseObjectCounter();
        pNext = pObj->m_pNext;

        if (pObj->m_pNext)
        {
            cryPrefetchT0SSE(pObj->m_pNext);
        }

        IF (pObj->m_dwRndFlags & ERF_HIDDEN, 0)
        {
            continue;
        }

        pObj->FillBBox(objBox);

        if (bNodeCompletelyInFrustum || rCam.IsAABBVisible_FM(objBox))
        {
            float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, objBox)) * passInfo.GetZoomFactor();
            assert(fEntDistance >= 0 && _finite(fEntDistance));
            if (fEntDistance < pObj->m_fWSMaxViewDist)
            {
#if !defined(_RELEASE)
                EERType rnType = pObj->GetRenderNodeType();
                if (!passInfo.RenderDecals() && rnType == eERType_Decal)
                {
                    continue;
                }
#endif // _RELEASE

                if (pCVars->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
                {
                    // if object is visible, write to output queue for main thread processing
                    if (GetObjManager()->CheckOcclusion_TestAABB(objBox, fEntDistance))
                    {
                        GetObjManager()->PushIntoCullOutputQueue(SCheckOcclusionOutput::CreateDecalsAndRoadsOutput(pObj, objBox, fEntDistance, bCheckPerObjectOcclusion, rendItemSorter));
                    }
                }
                else
                {
                    GetObjManager()->RenderDecalAndRoad(pObj, objBox, fEntDistance, bCheckPerObjectOcclusion, passInfo, rendItemSorter);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::RenderCommonObjects(TDoublyLinkedList<IRenderNode>* lstObjects, const CCamera& rCam, int nRenderMask, bool bNodeCompletelyInFrustum, SSectorTextureSet* pTerrainTexInfo, const SRenderingPassInfo& passInfo, SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;

    CVars* pCVars = GetCVars();
    AABB objBox;
    const Vec3 vCamPos = rCam.GetPosition();

    for (IRenderNode* pObj = lstObjects->m_pFirstNode, * pNext; pObj; pObj = pNext)
    {
        rendItemSorter.IncreaseObjectCounter();
        pNext = pObj->m_pNext;

        if (pObj->m_pNext)
        {
            cryPrefetchT0SSE(pObj->m_pNext);
        }

        IF (pObj->m_dwRndFlags & ERF_HIDDEN, 0)
        {
            continue;
        }

        pObj->FillBBox(objBox);
        EERType rnType = pObj->GetRenderNodeType();

        if (bNodeCompletelyInFrustum || rCam.IsAABBVisible_FM(objBox))
        {
            float fEntDistance = sqrt_tpl(Distance::Point_AABBSq(vCamPos, objBox)) * passInfo.GetZoomFactor();
            assert(fEntDistance >= 0 && _finite(fEntDistance));
            if (fEntDistance < pObj->m_fWSMaxViewDist)
            {
                if (nRenderMask & OCTREENODE_RENDER_FLAG_OBJECTS_ONLY_ENTITIES)
                {
                    if (rnType != eERType_RenderComponent && rnType != eERType_DynamicMeshRenderComponent && rnType != eERType_SkinnedMeshRenderComponent)
                    {
                        if (rnType == eERType_Light)
                        {
                            CLightEntity* pEnt = (CLightEntity*)pObj;
                            if (!pEnt->GetEntityVisArea() &&  !(pEnt->m_light.m_Flags & DLF_THIS_AREA_ONLY))
                            { // not "this area only" outdoor light affects everything
                            }
                            else
                            {
                                continue;
                            }
                        }
                        else
                        {
                            continue;
                        }
                    }
                }

                if (rnType == eERType_Light)
                {
                    bool bLightVisible = true;
                    CLightEntity* pLightEnt = (CLightEntity*)pObj;

                    // first check against camera view frustum
                    CDLight* pLight = &pLightEnt->m_light;
                    if (pLight->m_Flags & DLF_DEFERRED_CUBEMAPS)
                    {
                        OBB obb(OBB::CreateOBBfromAABB(Matrix33(pLight->m_ObjMatrix), AABB(-pLight->m_ProbeExtents, pLight->m_ProbeExtents)));
                        bLightVisible = passInfo.GetCamera().IsOBBVisible_F(pLight->m_Origin, obb);
                    }
                    else if (pLightEnt->m_light.m_Flags & DLF_AREA_LIGHT)
                    {
                        // OBB test for area lights.
                        Vec3 vBoxMax(pLight->m_fBaseRadius, pLight->m_fBaseRadius + pLight->m_fAreaWidth, pLight->m_fBaseRadius + pLight->m_fAreaHeight);
                        Vec3 vBoxMin(-0.1f, -(pLight->m_fBaseRadius + pLight->m_fAreaWidth), -(pLight->m_fBaseRadius + pLight->m_fAreaHeight));

                        OBB obb(OBB::CreateOBBfromAABB(Matrix33(pLight->m_ObjMatrix), AABB(vBoxMin, vBoxMax)));
                        bLightVisible = rCam.IsOBBVisible_F(pLight->m_Origin, obb);
                    }
                    else
                    {
                        bLightVisible = rCam.IsSphereVisible_F(Sphere(pLight->m_BaseOrigin, pLight->m_fBaseRadius));
                    }

                    if (!bLightVisible)
                    {
                        continue;
                    }
                }

                if (pCVars->e_StatObjBufferRenderTasks == 1 && passInfo.IsGeneralPass())
                {
                    // If object is visible
                    if (rnType == eERType_DistanceCloud || GetObjManager()->CheckOcclusion_TestAABB(objBox, fEntDistance))
                    {
                        if (pObj->CanExecuteRenderAsJob())
                        {
                            // If object can be executed as a job, call RenderObject directly from this job
                            GetObjManager()->RenderObject(pObj, objBox, fEntDistance, eERType_RenderComponent, passInfo, rendItemSorter);
                        }
                        else
                        {
                            // Otherwise, write to output queue for main thread processing
                            GetObjManager()->PushIntoCullOutputQueue(SCheckOcclusionOutput::CreateCommonObjectOutput(pObj, objBox, fEntDistance, pTerrainTexInfo, rendItemSorter));
                        }
                    }
                }
                else
                {
                    GetObjManager()->RenderObject(pObj, objBox, fEntDistance, rnType, passInfo, rendItemSorter);
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::UnlinkObject(IRenderNode* pObj)
{
    ERNListType eListType = pObj->GetRenderNodeListId(pObj->GetRenderNodeType());

    assert(eListType >= 0 && eListType < eRNListType_ListsNum);
    TDoublyLinkedList<IRenderNode>& rList = m_arrObjects[eListType];

    assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
    assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
    assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);

    if (pObj->m_pNext || pObj->m_pPrev || pObj == rList.m_pLastNode || pObj == rList.m_pFirstNode)
    {
        rList.remove(pObj);
    }

    assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
    assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);
    assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
    assert(!pObj->m_pNext && !pObj->m_pPrev);
}

bool COctreeNode::DeleteObject(IRenderNode* pObj)
{
    FUNCTION_PROFILER_3DENGINE;

    if (pObj->m_pOcNode && pObj->m_pOcNode != this)
    {
        return ((COctreeNode*)(pObj->m_pOcNode))->DeleteObject(pObj);
    }

    UnlinkObject(pObj);

    if (m_removeVegetationCastersOneByOne)
    {
        for (int i = 0; i < m_lstCasters.Count(); i++)
        {
            if (m_lstCasters[i].pNode == pObj)
            {
                m_lstCasters.Delete(i);
                break;
            }
        }
    }

    C3DEngine* p3DEngine = Get3DEngine();
    bool bSafeToUse = p3DEngine ? p3DEngine->IsObjectTreeReady() : false;

    pObj->m_pOcNode = NULL;
    pObj->m_nSID = -1;

    if (bSafeToUse && IsEmpty() && m_arrEmptyNodes.Find(this) < 0)
    {
        m_arrEmptyNodes.Add(this);
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::InsertObject(IRenderNode* pObj, const AABB& objBox, const float fObjRadiusSqr, const Vec3& vObjCenter)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ThreeDEngineDetailed);

    COctreeNode* pCurrentNode = this;

    const EERType eType = pObj->GetRenderNodeType();
    const uint32 renderFlags = (pObj->GetRndFlags() & (ERF_GOOD_OCCLUDER | ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS));

    const bool bTypeLight = (eType == eERType_Light);
    const float fWSMaxViewDist = pObj->m_fWSMaxViewDist;

    Vec3 vObjectCentre = vObjCenter;

    while (true)
    {
        PrefetchLine(&pCurrentNode->m_arrChilds[0], 0);

#if !defined(_RELEASE)
        if (GetCVars()->e_CheckOctreeObjectsBoxSize != 0) // pCurrentNode->m_pParent is checked as silly sized things are added to the root (e.g. the sun)
        {
            if (pCurrentNode->m_pParent && !objBox.IsReset() && (objBox.min.len() > CHECK_OBJECTS_BOX_WARNING_SIZE || objBox.max.len() > CHECK_OBJECTS_BOX_WARNING_SIZE))
            {
                CryWarning(VALIDATOR_MODULE_3DENGINE, VALIDATOR_ERROR_DBGBRK, "Huge object being added to a COctreeNode, name: '%s', objBox: [%f %f %f] -> [%f %f %f]\n", pObj->GetName(), objBox.min.x, objBox.min.y, objBox.min.z, objBox.max.x, objBox.max.y, objBox.max.z);
            }
        }
#endif

        // parent bbox includes all children
        pCurrentNode->m_objectsBox.Add(objBox);

        pCurrentNode->m_fObjectsMaxViewDist = max(pCurrentNode->m_fObjectsMaxViewDist, fWSMaxViewDist);

        pCurrentNode->m_renderFlags |= renderFlags;

        pCurrentNode->m_bHasLights |= (bTypeLight);

        if (pCurrentNode->m_vNodeAxisRadius.x * 2.0f > fNodeMinSize) // store voxels and roads in root
        {
            float nodeRadius = sqrt(pCurrentNode->GetNodeRadius2());

            if (fObjRadiusSqr < sqr(nodeRadius * fObjectToNodeSizeRatio))
            {
                int nChildId =
                    ((vObjCenter.x > pCurrentNode->m_vNodeCenter.x) ? 4 : 0) |
                    ((vObjCenter.y > pCurrentNode->m_vNodeCenter.y) ? 2 : 0) |
                    ((vObjCenter.z > pCurrentNode->m_vNodeCenter.z) ? 1 : 0);

                if (!pCurrentNode->m_arrChilds[nChildId])
                {
                    pCurrentNode->m_arrChilds[nChildId] = COctreeNode::Create(pCurrentNode->m_nSID, pCurrentNode->GetChildBBox(nChildId), pCurrentNode->m_pVisArea, pCurrentNode);
                }

                pCurrentNode = pCurrentNode->m_arrChilds[nChildId];

                continue;
            }
        }

        break;
    }

    //disable as it leads to some corruption on 360
    //  PrefetchLine(&pObj->m_pOcNode, 0);  //Writing to m_pOcNode was a common L2 cache miss

    pCurrentNode->LinkObject(pObj, eType);

    pObj->m_pOcNode = pCurrentNode;
    pObj->m_nSID = pCurrentNode->m_nSID;

    // only mark octree nodes as not compiled during loading and in the editor
    // otherwise update node (and parent node) flags on per added object basis
    if (m_bLevelLoadingInProgress || gEnv->IsEditor())
    {
        // Do nothing
    }
    else
    {
        pCurrentNode->UpdateObjects(pObj);
    }

    pCurrentNode->m_nManageVegetationsFrameId = 0;
}

//////////////////////////////////////////////////////////////////////////
AABB COctreeNode::GetChildBBox(int nChildId)
{
    int x = (nChildId / 4);
    int y = (nChildId - x * 4) / 2;
    int z = (nChildId - x * 4 - y * 2);
    const Vec3& vSize = m_vNodeAxisRadius;
    Vec3 vOffset = vSize;
    vOffset.x *= x;
    vOffset.y *= y;
    vOffset.z *= z;
    AABB childBox;
    childBox.min = m_vNodeCenter - vSize + vOffset;
    childBox.max = childBox.min + vSize;
    return childBox;
}

//////////////////////////////////////////////////////////////////////////
bool COctreeNode::IsEmpty()
{
    if (m_pParent)
    {
        if (!m_arrChilds[0] && !m_arrChilds[1] && !m_arrChilds[2] && !m_arrChilds[3])
        {
            if (!m_arrChilds[4] && !m_arrChilds[5] && !m_arrChilds[6] && !m_arrChilds[7])
            {
                if (!HasObjects())
                {
                    return true;
                }
            }
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
bool COctreeNode::IsRightNode(const AABB& objBox, const float fObjRadiusSqr, [[maybe_unused]] float fObjMaxViewDist)
{
    const AABB& nodeBox = GetNodeBox();
    if (!Overlap::Point_AABB(objBox.GetCenter(), nodeBox))
    {
        if (m_pParent)
        {
            return false; // fail if center is not inside or node bbox
        }
    }
    if (2 != Overlap::AABB_AABB_Inside(objBox, m_objectsBox))
    {
        return false; // fail if not completely inside of objects bbox
    }
    float fNodeRadiusRated = GetNodeRadius2() * sqr(fObjectToNodeSizeRatio);

    if (fObjRadiusSqr > fNodeRadiusRated * 4.f)
    {
        if (m_pParent)
        {
            return false; // fail if object is too big and we need to register it some of parents
        }
    }
    if (m_vNodeAxisRadius.x * 2.0f > fNodeMinSize)
    {
        if (fObjRadiusSqr < fNodeRadiusRated)
        {
            //      if(fObjMaxViewDist < m_fNodeRadius*GetCVars()->e_ViewDistRatioVegetation*fObjectToNodeSizeRatio)
            return false; // fail if object is too small and we need to register it some of childs
        }
    }
    return true;
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::LinkObject(IRenderNode* pObj, EERType eERType, bool bPushFront)
{
    ERNListType eListType = pObj->GetRenderNodeListId(eERType);

    TDoublyLinkedList<IRenderNode>& rList = m_arrObjects[eListType];

    assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
    assert(!pObj->m_pNext && !pObj->m_pPrev);
    assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
    assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);

    if (bPushFront)
    {
        rList.insertBeginning(pObj);
    }
    else
    {
        rList.insertEnd(pObj);
    }

    assert(!rList.m_pFirstNode || !rList.m_pFirstNode->m_pPrev);
    assert(!rList.m_pLastNode || !rList.m_pLastNode->m_pNext);
    assert(pObj != pObj->m_pPrev && pObj != pObj->m_pNext);
}

//////////////////////////////////////////////////////////////////////////
void COctreeNode::UpdateObjects(IRenderNode* pObj)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::ThreeDEngineDetailed);

    float fObjMaxViewDistance = 0;
    size_t numCasters = 0;
    IObjManager* pObjManager = GetObjManager();

    bool bVegetHasAlphaTrans = false;
    int nFlags = pObj->GetRndFlags();
    EERType eRType = pObj->GetRenderNodeType();

    IF (nFlags & ERF_HIDDEN, 0)
    {
        return;
    }

    I3DEngine* p3DEngine = GetISystem()->GetI3DEngine();
    const Vec3& sunDir = p3DEngine->GetSunDirNormalized();
    uint32 sunDirX = (uint32)(sunDir.x * 63.5f + 63.5f);
    uint32 sunDirZ = (uint32)(sunDir.z * 63.5f + 63.5f);
    uint32 sunDirYs = (uint32)(sunDir.y < 0.0f ? 1 : 0);

    pObj->m_nInternalFlags &= ~(IRenderNode::REQUIRES_FORWARD_RENDERING | IRenderNode::REQUIRES_NEAREST_CUBEMAP);

    // update max view distances
    const float fNewMaxViewDist = pObj->GetMaxViewDist();
    pObj->m_fWSMaxViewDist = fNewMaxViewDist;

    if (eRType != eERType_Light && eRType != eERType_Cloud && eRType != eERType_FogVolume && eRType != eERType_Decal && eRType != eERType_DistanceCloud)
    {
        {
            CMatInfo* pMatInfo = (CMatInfo*)pObj->GetMaterial().get();
            if (pMatInfo)
            {
                if (bVegetHasAlphaTrans || pMatInfo->IsForwardRenderingRequired())
                {
                    pObj->m_nInternalFlags |= IRenderNode::REQUIRES_FORWARD_RENDERING;
                }

                if (pMatInfo->IsNearestCubemapRequired())
                {
                    pObj->m_nInternalFlags |= IRenderNode::REQUIRES_NEAREST_CUBEMAP;
                }
            }

            if (eRType == eERType_RenderComponent || eRType == eERType_DynamicMeshRenderComponent || eRType == eERType_SkinnedMeshRenderComponent)
            {
                int nSlotCount = pObj->GetSlotCount();

                for (int s = 0; s < nSlotCount; s++)
                {
                    if (CMatInfo* pMat = (CMatInfo*)pObj->GetEntitySlotMaterial(s).get())
                    {
                        if (pMat->IsForwardRenderingRequired())
                        {
                            pObj->m_nInternalFlags |= IRenderNode::REQUIRES_FORWARD_RENDERING;
                        }
                        if (pMat->IsNearestCubemapRequired())
                        {
                            pObj->m_nInternalFlags |= IRenderNode::REQUIRES_NEAREST_CUBEMAP;
                        }
                    }

                    if (IStatObj* pStatObj = pObj->GetEntityStatObj(s))
                    {
                        if (CMatInfo* pMat = (CMatInfo*)pStatObj->GetMaterial().get())
                        {
                            if (pMat->IsForwardRenderingRequired())
                            {
                                pObj->m_nInternalFlags |= IRenderNode::REQUIRES_FORWARD_RENDERING;
                            }
                            if (pMat->IsNearestCubemapRequired())
                            {
                                pObj->m_nInternalFlags |= IRenderNode::REQUIRES_NEAREST_CUBEMAP;
                            }
                        }
                    }
                }
            }
        }
    }

    bool bUpdateParentShadowFlags = false;
    bool bUpdateParentOcclusionFlags = false;

    // fill shadow casters list
    const bool bHasPerObjectShadow = GetCVars()->e_ShadowsPerObject && p3DEngine->GetPerObjectShadow(pObj);
    if (nFlags & ERF_CASTSHADOWMAPS && fNewMaxViewDist > fMinShadowCasterViewDist && eRType != eERType_Light && !bHasPerObjectShadow)
    {
        bUpdateParentShadowFlags = true;

        float fMaxCastDist = fNewMaxViewDist * GetCVars()->e_ShadowsCastViewDistRatio;
        m_lstCasters.Add(SCasterInfo(pObj, fMaxCastDist, eRType));
    }

    fObjMaxViewDistance = max(fObjMaxViewDistance, fNewMaxViewDist);

    // traverse the octree upwards and fill in new flags
    COctreeNode* pNode = this;
    bool bContinue = false;
    do
    {
        // update max view dist
        if (pNode->m_fObjectsMaxViewDist < fObjMaxViewDistance)
        {
            pNode->m_fObjectsMaxViewDist = fObjMaxViewDistance;
            bContinue = true;
        }

        // update shadow flags
        if (bUpdateParentShadowFlags && (pNode->m_renderFlags & ERF_CASTSHADOWMAPS) == 0)
        {
            pNode->m_renderFlags |= ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS;
            bContinue = true;
        }

        pNode = pNode->m_pParent;
    } while (pNode != NULL && bContinue);

    m_fpSunDirX = sunDirX;
    m_fpSunDirZ = sunDirZ;
    m_fpSunDirYs = sunDirYs;
}

//////////////////////////////////////////////////////////////////////////
int16 CObjManager::GetNearestCubeProbe(IVisArea* pVisArea, const AABB& objBox, bool bSpecular)
{
    // Only used for alpha blended geometry - but still should be optimized further
    float fMinDistance = FLT_MAX;
    int nMaxPriority = -1;
    CLightEntity* pNearestLight = NULL;
    int16 nDefaultId = Get3DEngine()->GetBlackCMTexID();

    if (!pVisArea)
    {
        if (Get3DEngine()->IsObjectTreeReady())
        {
            Get3DEngine()->GetObjectTree()->GetNearestCubeProbe(fMinDistance, nMaxPriority, pNearestLight, &objBox);
        }
    }
    else
    {
        Get3DEngine()->GetVisAreaManager()->GetNearestCubeProbe(fMinDistance, nMaxPriority, pNearestLight, &objBox);
    }

    if (pNearestLight)
    {
        ITexture* pTexCM = bSpecular ? pNearestLight->m_light.GetSpecularCubemap() : pNearestLight->m_light.GetDiffuseCubemap();
        // Return cubemap ID or -1 if invalid
        return (pTexCM && pTexCM->GetTextureType() >= eTT_Cube) ? pTexCM->GetTextureID() : nDefaultId;
    }

    // No cubemap found
    return nDefaultId;
}

//////////////////////////////////////////////////////////////////////////
bool CObjManager::IsAfterWater(const Vec3& vPos, const SRenderingPassInfo& passInfo)
{
    // considered "after water" if any of the following is true:
    //    - Position & camera are on the same side of the water surface on recursive level 0
    //    - Position & camera are on opposite sides of the water surface on recursive level 1

    float fWaterLevel;
    if (OceanToggle::IsActive())
    {
        if (!OceanRequest::OceanIsEnabled())
        {
            return true;
        }
        fWaterLevel = OceanRequest::GetOceanLevel();
    }
    else
    {
        fWaterLevel = GetOcean() ? GetOcean()->GetWaterLevel() : WATER_LEVEL_UNKNOWN;
    }

    return (0.5f - passInfo.GetRecursiveLevel()) * (0.5f - passInfo.IsCameraUnderWater()) * (vPos.z - fWaterLevel) > 0;
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::RenderObjectDebugInfo(IRenderNode* pEnt, float fEntDistance,  const SRenderingPassInfo& passInfo)
{
    if (!passInfo.IsGeneralPass())
    {
        return;
    }

    m_arrRenderDebugInfo.push_back(SObjManRenderDebugInfo(pEnt, fEntDistance));
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::RemoveCullJobProducer()
{
    m_CheckOcclusionOutputQueue.RemoveProducer();
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::AddCullJobProducer()
{
    m_CheckOcclusionOutputQueue.AddProducer();
}

//////////////////////////////////////////////////////////////////////////
bool CObjManager::CheckOcclusion_TestAABB(const AABB& rAABB, float fEntDistance)
{
    return m_CullThread.TestAABB(rAABB, fEntDistance);
}


//////////////////////////////////////////////////////////////////////////
bool CObjManager::CheckOcclusion_TestQuad(const Vec3& vCenter, const Vec3& vAxisX, const Vec3& vAxisY)
{
    return m_CullThread.TestQuad(vCenter, vAxisX, vAxisY);
}

#ifndef _RELEASE
//////////////////////////////////////////////////////////////////////////
void CObjManager::CoverageBufferDebugDraw()
{
    m_CullThread.CoverageBufferDebugDraw();
}
#endif

//////////////////////////////////////////////////////////////////////////
bool CObjManager::LoadOcclusionMesh(const char* pFileName)
{
    return m_CullThread.LoadLevel(pFileName);
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::PushIntoCullQueue(const SCheckOcclusionJobData& rCheckOcclusionData)
{
#if !defined(_RELEASE)
    IF (!m_CullThread.IsActive(), 0)
    {
        __debugbreak();
    }
    IF (rCheckOcclusionData.type == SCheckOcclusionJobData::QUIT, 0)
    {
        m_CullThread.SetActive(false);
    }
#endif
    // Prevent our queue from filling up, and make sure to always leave room for the "QUIT" message.
    // If we try to add nodes to a full queue, it's possible for deadlocks to occur.  The CheckOcclusionQueue 
    // is filled from the main thread, and will block if the queue is full.  The queue is emptied from a culling 
    // thread, but the CheckOcclusionOutputQueue is filled from the culling thread and will block if *its* queue is 
    // full.  The main thread is the one that empties the output queue thread, so queues that are full on both sides, 
    // mixed with bad timing, can cause a deadlock.  Such are the perils of lockless fixed size queues. :(
    // Rather than locking up, we will instead emit a warning and over-cull by not even submitting our geometry for
    // potential rendering.
    if ((m_CheckOcclusionQueue.FreeCount() > 1) ||
        (rCheckOcclusionData.type == SCheckOcclusionJobData::QUIT))
    {
        m_CheckOcclusionQueue.Push(rCheckOcclusionData);
    }
    else
    {
        // If this warning is hit in the editor, it's likely because of editing terrain.  Edited terrain draws at the highest
        // LOD, which means you'll need to set this to (heightmap height * width) / (32 * 32) at a bare minimum to have a 
        // large enough buffer.  It will need to be even larger if you have significant amounts of static geometry in the level too.
        // If this warning is hit in-game, you'll just need to use trial and error to determine a large enough size.
        AZ_Warning("Cull", false,
            "Occlusion Queue is full - need to set the e_CheckOcclusionQueueSize CVar value larger (current value = %u).", 
            m_CheckOcclusionQueue.BufferSize());
    }

}

//////////////////////////////////////////////////////////////////////////
void CObjManager::PopFromCullQueue(SCheckOcclusionJobData* pCheckOcclusionData)
{
    m_CheckOcclusionQueue.Pop(pCheckOcclusionData);
}

//////////////////////////////////////////////////////////////////////////
void CObjManager::PushIntoCullOutputQueue(const SCheckOcclusionOutput& rCheckOcclusionOutput)
{
    // Prevent our output queue from filling up.  If we try to add nodes to a full queue, it's possible for deadlocks 
    // to occur.  (See explanation in PushIntoCullQueue() above)
    // Rather than locking up, we will instead emit a warning and over-cull by not even submitting our geometry for
    // potential rendering.
    if (m_CheckOcclusionOutputQueue.FreeCount() > 0)
    {
        m_CheckOcclusionOutputQueue.Push(rCheckOcclusionOutput);
    }
    else
    {
        // If this warning is hit in the editor, it's likely because of editing terrain.  This should likely be set to 2x to 4x the
        // size of e_CheckOcclusionQueueSize.
        // If this warning is hit in-game, you'll just need to use trial and error to determine a large enough size.
        AZ_Warning("Cull", false,
            "Occlusion Output Queue is full - need to set the e_CheckOcclusionOutputQueueSize CVar value larger (current value = %u).", 
            m_CheckOcclusionOutputQueue.BufferSize());
    }
}

//////////////////////////////////////////////////////////////////////////
bool CObjManager::PopFromCullOutputQueue(SCheckOcclusionOutput* pCheckOcclusionOutput)
{
    return m_CheckOcclusionOutputQueue.Pop(pCheckOcclusionOutput);
}

///////////////////////////////////////////////////////////////////////////////
uint8 CObjManager::GetDissolveRef(float fDist, float fMVD)
{
    float fDissolveDist = 1.0f / CLAMP(0.1f * fMVD, GetFloatCVar(e_DissolveDistMin), GetFloatCVar(e_DissolveDistMax));

    return (uint8)SATURATEB((1.0f + (fDist - fMVD) * fDissolveDist) * 255.f);
}

///////////////////////////////////////////////////////////////////////////////
float CObjManager::GetLodDistDissolveRef(SLodDistDissolveTransitionState* pState, float curDist, int nNewLod, [[maybe_unused]] const SRenderingPassInfo& passInfo)
{
    float fDissolveDistbandClamped = min(GetFloatCVar(e_DissolveDistband), curDist * .4f) + 0.001f;

    if (!pState->fStartDist)
    {
        pState->fStartDist = curDist;
        pState->nOldLod = nNewLod;
        pState->nNewLod = nNewLod;

        pState->bFarside = (pState->nNewLod < pState->nOldLod && pState->nNewLod != -1) || pState->nOldLod == -1;
    }
    else if (pState->nNewLod != nNewLod)
    {
        pState->nNewLod = nNewLod;
        pState->fStartDist = curDist;

        pState->bFarside = (pState->nNewLod < pState->nOldLod && pState->nNewLod != -1) || pState->nOldLod == -1;
    }
    else if ((pState->nOldLod != pState->nNewLod))
    {
        // transition complete
        if (
            (!pState->bFarside && curDist - pState->fStartDist > fDissolveDistbandClamped) ||
            (pState->bFarside && pState->fStartDist - curDist > fDissolveDistbandClamped)
            )
        {
            pState->nOldLod = pState->nNewLod;
        }
        // with distance based transitions we can always 'fail' back to the previous LOD.
        else if (
            (!pState->bFarside && curDist < pState->fStartDist) ||
            (pState->bFarside && curDist > pState->fStartDist)
            )
        {
            pState->nNewLod = pState->nOldLod;
        }
    }

    if (pState->nOldLod == pState->nNewLod)
    {
        return 0.f;
    }
    else
    {
        if (pState->bFarside)
        {
            return SATURATE(((pState->fStartDist - curDist) * (1.f / fDissolveDistbandClamped)));
        }
        else
        {
            return SATURATE(((curDist - pState->fStartDist) * (1.f / fDissolveDistbandClamped)));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
int CObjManager::GetObjectLOD(const IRenderNode* pObj, float fDistance)
{
    SFrameLodInfo frameLodInfo = Get3DEngine()->GetFrameLodInfo();
    int resultLod = MAX_STATOBJ_LODS_NUM - 1;
    bool boundingBBoxBased = ((pObj->GetRndFlags() & ERF_LOD_BBOX_BASED) != 0);
    // If it is bounding box based, it does not use face area data.
    bool useLodFaceArea = (GetCVars()->e_LodFaceArea != 0)  && !boundingBBoxBased;
  
    if (useLodFaceArea)
    {
        float distances[SMeshLodInfo::s_nMaxLodCount];
        useLodFaceArea = pObj->GetLodDistances(frameLodInfo, distances);
        if (useLodFaceArea)
        {
            for (uint i = 0; i < MAX_STATOBJ_LODS_NUM - 1; ++i)
            {
                if (fDistance < distances[i])
                {
                    resultLod = i;
                    break;
                }
            }
        }
    }

    if (!useLodFaceArea)
    {
        const float fLodRatioNorm = pObj->GetLodRatioNormalized();
        const float fRadius = pObj->GetBBox().GetRadius();
        resultLod = (int)(fDistance * (fLodRatioNorm * fLodRatioNorm) / (max(frameLodInfo.fLodRatio * min(fRadius, GetFloatCVar(e_LodCompMaxSize)), 0.001f)));
    }

    return resultLod;
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::GetObjectsByFlags(uint dwFlags, PodArray<IRenderNode*>& lstObjects)
{
    unsigned int nCurrentObject(eRNListType_First);
    for (nCurrentObject = eRNListType_First; nCurrentObject < eRNListType_ListsNum; ++nCurrentObject)
    {
        for (IRenderNode* pObj = m_arrObjects[nCurrentObject].m_pFirstNode; pObj; pObj = pObj->m_pNext)
        {
            if ((pObj->GetRndFlags() & dwFlags) == dwFlags)
            {
                lstObjects.Add(pObj);
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->GetObjectsByFlags(dwFlags, lstObjects);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::GetObjectsByType(PodArray<IRenderNode*>& lstObjects, EERType objType, const AABB* pBBox, ObjectTreeQueryFilterCallback filterCallback)
{
    if (objType == eERType_Light && !m_bHasLights)
    {
        return;
    }

    if (pBBox && !Overlap::AABB_AABB(*pBBox, GetObjectsBBox()))
    {
        return;
    }

    ERNListType eListType = IRenderNode::GetRenderNodeListId(objType);

    for (IRenderNode* pObj = m_arrObjects[eListType].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        if (pObj->GetRenderNodeType() == objType)
        {
            AABB box;
            pObj->FillBBox(box);
            if (!pBBox || Overlap::AABB_AABB(*pBBox, box))
            {
                // Check the filterCallback to perform a final validation that we want this object
                // to appear in our results list.  If there's no filterCallback, then always add
                // the object.
                if (!filterCallback || filterCallback(pObj, objType))
                {
                    lstObjects.Add(pObj);
                }
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->GetObjectsByType(lstObjects, objType, pBBox, filterCallback);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void COctreeNode::GetNearestCubeProbe(float& fMinDistance, int& nMaxPriority, CLightEntity*& pNearestLight, const AABB* pBBox)
{
    if (!m_bHasLights)
    {
        return;
    }

    if (!pBBox || pBBox && !Overlap::AABB_AABB(*pBBox, GetObjectsBBox()))
    {
        return;
    }

    Vec3 vCenter = pBBox->GetCenter();

    ERNListType eListType = IRenderNode::GetRenderNodeListId(eERType_Light);

    for (IRenderNode* pObj = m_arrObjects[eListType].m_pFirstNode; pObj; pObj = pObj->m_pNext)
    {
        if (pObj->GetRenderNodeType() == eERType_Light)
        {
            AABB box;
            pObj->FillBBox(box);
            if (Overlap::AABB_AABB(*pBBox, box))
            {
                CLightEntity* pLightEnt = (CLightEntity*)pObj;
                CDLight* pLight = &pLightEnt->m_light;

                if (pLight->m_Flags & DLF_DEFERRED_CUBEMAPS)
                {
                    Vec3 vCenterRel = vCenter - pLight->GetPosition();
                    Vec3 vCenterOBBSpace;
                    vCenterOBBSpace.x = pLightEnt->m_Matrix.GetColumn0().GetNormalized().dot(vCenterRel);
                    vCenterOBBSpace.y = pLightEnt->m_Matrix.GetColumn1().GetNormalized().dot(vCenterRel);
                    vCenterOBBSpace.z = pLightEnt->m_Matrix.GetColumn2().GetNormalized().dot(vCenterRel);

                    // Check if object center is within probe OBB
                    Vec3 vProbeExtents = pLight->m_ProbeExtents;
                    if (fabs(vCenterOBBSpace.x) < vProbeExtents.x && fabs(vCenterOBBSpace.y) < vProbeExtents.y && fabs(vCenterOBBSpace.z) < vProbeExtents.z)
                    {
                        if (pLight->m_nSortPriority > nMaxPriority 
                            && pLight->m_fProbeAttenuation > 0) // Don't return a probe that is disabled/invisible. In particular this provides better results when lighting particles.
                        {
                            pNearestLight = (CLightEntity*)pObj;
                            nMaxPriority = pLight->m_nSortPriority;
                            fMinDistance = 0;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < 8; i++)
    {
        if (m_arrChilds[i])
        {
            m_arrChilds[i]->GetNearestCubeProbe(fMinDistance, nMaxPriority, pNearestLight, pBBox);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
bool CObjManager::IsBoxOccluded(const AABB& objBox,
    [[maybe_unused]] float fDistance,
    OcclusionTestClient* const __restrict pOcclTestVars,
    bool/* bIndoorOccludersOnly*/,
    [[maybe_unused]] EOcclusionObjectType eOcclusionObjectType,
    const SRenderingPassInfo& passInfo)
{
    // if object was visible during last frames
    const uint32 mainFrameID = passInfo.GetMainFrameID();

    if (GetCVars()->e_OcclusionLazyHideFrames)
    {
        //This causes massive spikes in draw calls when rotating
        if (pOcclTestVars->nLastVisibleMainFrameID > mainFrameID - GetCVars()->e_OcclusionLazyHideFrames)
        {
            // prevent checking all objects in same frame
            int nId = (int)(UINT_PTR(pOcclTestVars) / 256);
            if ((nId & 7) != (mainFrameID & 7))
            {
                return false;
            }
        }
    }

    // use fast and reliable test right here
    CVisAreaManager* pVisAreaManager = GetVisAreaManager();
    if (GetCVars()->e_OcclusionVolumes && pVisAreaManager && pVisAreaManager->IsOccludedByOcclVolumes(objBox, passInfo))
    {
#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
        // do not set the lastOccludedMainFrameID because it is camera agnostic so the main pass might occlude
        // objects that should only be occluded in the render scene to texture pass
        if (!passInfo.IsRenderSceneToTexturePass())
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
        {
            pOcclTestVars->nLastOccludedMainFrameID = mainFrameID;
        }
        return true;
    }

#if AZ_RENDER_TO_TEXTURE_GEM_ENABLED
    // don't use coverage buffer results if we are checking occlusion for a render to texture camera
    // because the render to texture pass does not currently write to the coverage buffer and
    // the frame IDs will not work correctly.
    if (GetCVars()->e_CoverageBuffer && !passInfo.IsRenderSceneToTexturePass())
#else
    if (GetCVars()->e_CoverageBuffer)
#endif // if AZ_RENDER_TO_TEXTURE_GEM_ENABLED

    {
        return pOcclTestVars->nLastOccludedMainFrameID == mainFrameID - 1;
    }

    pOcclTestVars->nLastVisibleMainFrameID = mainFrameID;

    return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CLightEntity::FillBBox(AABB& aabb)
{
    aabb = CLightEntity::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CLightEntity::GetRenderNodeType()
{
    return eERType_Light;
}

///////////////////////////////////////////////////////////////////////////////
float CLightEntity::GetMaxViewDist()
{
    if (m_light.m_Flags & DLF_SUN)
    {
        return 10.f * DISTANCE_TO_THE_SUN;
    }
    return max(GetCVars()->e_ViewDistMin, CLightEntity::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatioLights * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CLightEntity::GetPos([[maybe_unused]] bool bWorldOnly) const
{
    assert(bWorldOnly);
    return m_light.m_Origin;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void COcean::FillBBox(AABB& aabb)
{
    aabb = COcean::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType COcean::GetRenderNodeType()
{
    return eERType_WaterVolume;
}

///////////////////////////////////////////////////////////////////////////////
Vec3 COcean::GetPos(bool) const
{
    return Vec3(0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> COcean::GetMaterial([[maybe_unused]] Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CFogVolumeRenderNode::FillBBox(AABB& aabb)
{
    aabb = CFogVolumeRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CFogVolumeRenderNode::GetRenderNodeType()
{
    return eERType_FogVolume;
}

///////////////////////////////////////////////////////////////////////////////
float CFogVolumeRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CFogVolumeRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CFogVolumeRenderNode::GetPos([[maybe_unused]] bool bWorldOnly) const
{
    return m_pos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CFogVolumeRenderNode::GetMaterial([[maybe_unused]] Vec3* pHitPos)
{
    return 0;
}
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CDecalRenderNode::FillBBox(AABB& aabb)
{
    aabb = CDecalRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CDecalRenderNode::GetRenderNodeType()
{
    return eERType_Decal;
}

///////////////////////////////////////////////////////////////////////////////
float CDecalRenderNode::GetMaxViewDist()
{
    return m_decalProperties.m_maxViewDist * GetViewDistanceMultiplier();
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CDecalRenderNode::GetPos([[maybe_unused]] bool bWorldOnly) const
{
    return m_pos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CDecalRenderNode::GetMaterial([[maybe_unused]] Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CWaterVolumeRenderNode::FillBBox(AABB& aabb)
{
    aabb = CWaterVolumeRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CWaterVolumeRenderNode::GetRenderNodeType()
{
    return eERType_WaterVolume;
}

///////////////////////////////////////////////////////////////////////////////
float CWaterVolumeRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CWaterVolumeRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CWaterVolumeRenderNode::GetPos([[maybe_unused]] bool bWorldOnly) const
{
    return m_center;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CWaterVolumeRenderNode::GetMaterial([[maybe_unused]] Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CDistanceCloudRenderNode::FillBBox(AABB& aabb)
{
    aabb = CDistanceCloudRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CDistanceCloudRenderNode::GetRenderNodeType()
{
    return eERType_DistanceCloud;
}

///////////////////////////////////////////////////////////////////////////////
float CDistanceCloudRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CDistanceCloudRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CDistanceCloudRenderNode::GetPos([[maybe_unused]] bool bWorldOnly) const
{
    return m_pos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CDistanceCloudRenderNode::GetMaterial([[maybe_unused]] Vec3* pHitPos)
{
    return m_pMaterial;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if !defined(EXCLUDE_DOCUMENTATION_PURPOSE)
#include "PrismRenderNode.h"

///////////////////////////////////////////////////////////////////////////////
void CPrismRenderNode::FillBBox(AABB& aabb)
{
    aabb = CPrismRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CPrismRenderNode::GetRenderNodeType()
{
    return eERType_PrismObject;
}

///////////////////////////////////////////////////////////////////////////////
float CPrismRenderNode::GetMaxViewDist()
{
    return 1000.0f;
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CPrismRenderNode::GetPos([[maybe_unused]] bool bWorldOnly) const
{
    return m_mat.GetTranslation();
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CPrismRenderNode::GetMaterial([[maybe_unused]] Vec3* pHitPos)
{
    return m_pMaterial;
}

#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "VolumeObjectRenderNode.h"
#include "CloudRenderNode.h"

///////////////////////////////////////////////////////////////////////////////
void CVolumeObjectRenderNode::FillBBox(AABB& aabb)
{
    aabb = CVolumeObjectRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CVolumeObjectRenderNode::GetRenderNodeType()
{
    return eERType_VolumeObject;
}

///////////////////////////////////////////////////////////////////////////////
float CVolumeObjectRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CVolumeObjectRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

///////////////////////////////////////////////////////////////////////////////
Vec3 CVolumeObjectRenderNode::GetPos([[maybe_unused]] bool bWorldOnly) const
{
    return m_pos;
}

///////////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CVolumeObjectRenderNode::GetMaterial([[maybe_unused]] Vec3* pHitPos)
{
    return m_pMaterial;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CCloudRenderNode::FillBBox(AABB& aabb)
{
    aabb = CCloudRenderNode::GetBBox();
}

///////////////////////////////////////////////////////////////////////////////
EERType CCloudRenderNode::GetRenderNodeType()
{
    return eERType_Cloud;
}

//////////////////////////////////////////////////////////////////////////
float CCloudRenderNode::GetMaxViewDist()
{
    return max(GetCVars()->e_ViewDistMin, CCloudRenderNode::GetBBox().GetRadius() * GetCVars()->e_ViewDistRatio * GetViewDistanceMultiplier());
}

//////////////////////////////////////////////////////////////////////////
Vec3 CCloudRenderNode::GetPos([[maybe_unused]] bool bWorldOnly) const
{
    return m_pos;
}

//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial> CCloudRenderNode::GetMaterial([[maybe_unused]] Vec3* pHitPos)
{
    return m_pMaterial;
}
