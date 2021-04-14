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

// Description : Render all entities in the sector together with shadows


#include "Cry3DEngine_precompiled.h"

#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "3dEngine.h"
#include "CullBuffer.h"
#include "3dEngine.h"
#include "LightEntity.h"
#include "DecalManager.h"
#include "ObjectsTree.h"

void CObjManager::RenderDecalAndRoad(IRenderNode* pEnt,
    const AABB& objBox,
    float fEntDistance,
    bool nCheckOcclusion,
    const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;

    // do not draw if marked to be not drawn or already drawn in this frame
    unsigned int nRndFlags = pEnt->GetRndFlags();

    if (nRndFlags & ERF_HIDDEN)
    {
        return;
    }

    EERType eERType = pEnt->GetRenderNodeType();

    // detect bad objects
    float fEntLengthSquared = objBox.GetSize().GetLengthSquared();
    if (eERType != eERType_Light || !_finite(fEntLengthSquared))
    {
        if (fEntLengthSquared > MAX_VALID_OBJECT_VOLUME || !_finite(fEntLengthSquared) || fEntLengthSquared <= 0)
        {
            Warning("CObjManager::RenderObject: Object has invalid bbox: %s,%s, Radius = %.2f, Center = (%.1f,%.1f,%.1f)",
                pEnt->GetName(), pEnt->GetEntityClassName(), sqrt_tpl(fEntLengthSquared) * 0.5f,
                pEnt->GetBBox().GetCenter().x, pEnt->GetBBox().GetCenter().y, pEnt->GetBBox().GetCenter().z);
            return; // skip invalid objects - usually only objects with invalid very big scale will reach this point
        }
    }

    // allocate RNTmpData for potentially visible objects
    Get3DEngine()->CheckCreateRNTmpData(&pEnt->m_pRNTmpData, pEnt, passInfo);

    if (nCheckOcclusion && pEnt->m_pOcNode)
    {
        if (GetObjManager()->IsBoxOccluded(objBox, fEntDistance * passInfo.GetInverseZoomFactor(), &pEnt->m_pRNTmpData->userData.m_OcclState,
                pEnt->m_pOcNode->m_pVisArea != NULL, eoot_OBJECT, passInfo))
        {
            return;
        }
    }

    // skip "outdoor only" objects if outdoor is not visible
    if (GetCVars()->e_CoverageBuffer == 2 && nRndFlags & ERF_OUTDOORONLY && !Get3DEngine()->GetCoverageBuffer()->IsOutdooVisible())
    {
        return;
    }

    if (pEnt->GetDrawFrame(passInfo.GetRecursiveLevel()) == passInfo.GetFrameID())
    {
        return;
    }
    pEnt->SetDrawFrame(passInfo.GetFrameID(), passInfo.GetRecursiveLevel());

    CVisArea* pVisArea = (CVisArea*)pEnt->GetEntityVisArea();

    const Vec3& vCamPos = passInfo.GetCamera().GetPosition();

    // test only near/big occluders - others will be tested on tree nodes level
    if (!objBox.IsContainPoint(vCamPos))
    {
        if (eERType == eERType_Light || fEntDistance < pEnt->m_fWSMaxViewDist * GetCVars()->e_OcclusionCullingViewDistRatio)
        {
            if (IsBoxOccluded(objBox, fEntDistance * passInfo.GetInverseZoomFactor(), &pEnt->m_pRNTmpData->userData.m_OcclState, pVisArea != NULL, eoot_OBJECT, passInfo))
            {
                return;
            }
        }
    }

    SRendParams DrawParams;
    DrawParams.pTerrainTexInfo = NULL;
    DrawParams.dwFObjFlags = 0;
    DrawParams.fDistance = fEntDistance;
    DrawParams.AmbientColor = Vec3(0, 0, 0);
    DrawParams.pRenderNode = pEnt;

    // set lights bit mask
    DrawParams.nAfterWater = IsAfterWater(objBox.GetCenter(), passInfo) ? 1 : 0;

    // draw bbox
    if (GetCVars()->e_BBoxes)// && eERType != eERType_Light)
    {
        RenderObjectDebugInfo(pEnt, fEntDistance, passInfo);
    }

    DrawParams.m_pVisArea =   pVisArea;

    DrawParams.nMaterialLayers = pEnt->GetMaterialLayers();
    DrawParams.rendItemSorter = rendItemSorter.GetValue();

    pEnt->Render(DrawParams, passInfo);
}

void CObjManager::RenderObject(IRenderNode* pEnt,
    const AABB& objBox,
    float fEntDistance,
    EERType eERType,
    const SRenderingPassInfo& passInfo, const SRendItemSorter& rendItemSorter)
{
    FUNCTION_PROFILER_3DENGINE;

    const CVars* pCVars = GetCVars();

    // do not draw if marked to be not drawn or already drawn in this frame
    unsigned int nRndFlags = pEnt->GetRndFlags();

    if (nRndFlags & ERF_HIDDEN)
    {
        return;
    }

#ifndef _RELEASE
    // check cvars
    switch (eERType)
    {
    case eERType_Decal:
        if (!passInfo.RenderDecals())
        {
            return;
        }
        break;
    case eERType_WaterVolume:
        if (!passInfo.RenderWaterVolumes())
        {
            return;
        }
        break;
    case eERType_Light:
        if (!GetCVars()->e_DynamicLights || !passInfo.RenderEntities())
        {
            return;
        }
        break;
    case eERType_Cloud:
    case eERType_DistanceCloud:
        if (!passInfo.RenderClouds())
        {
            return;
        }
        break;
    default:
        if (!passInfo.RenderEntities())
        {
            return;
        }
        break;
    }

    // detect bad objects
    float fEntLengthSquared = objBox.GetSize().GetLengthSquared();
    if (eERType != eERType_Light || !_finite(fEntLengthSquared))
    {
        if (fEntLengthSquared > MAX_VALID_OBJECT_VOLUME || !_finite(fEntLengthSquared) || fEntLengthSquared <= 0)
        {
            Warning("CObjManager::RenderObject: Object has invalid bbox: %s, %s, Radius = %.2f, Center = (%.1f,%.1f,%.1f)",
                pEnt->GetName(), pEnt->GetEntityClassName(), sqrt_tpl(fEntLengthSquared) * 0.5f,
                pEnt->GetBBox().GetCenter().x, pEnt->GetBBox().GetCenter().y, pEnt->GetBBox().GetCenter().z);
            return; // skip invalid objects - usually only objects with invalid very big scale will reach this point
        }
    }
#endif

    if (pEnt->m_dwRndFlags & ERF_COLLISION_PROXY || pEnt->m_dwRndFlags & ERF_RAYCAST_PROXY)
    {
        // Collision proxy is visible in Editor while in editing mode.
        if (!gEnv->IsEditor() || !gEnv->IsEditing())
        {
            if (GetCVars()->e_DebugDraw == 0)
            {
                return; //true;
            }
        }
    }

    // allocate RNTmpData for potentially visible objects
    Get3DEngine()->CheckCreateRNTmpData(&pEnt->m_pRNTmpData, pEnt, passInfo);
    PrefetchLine(pEnt->m_pRNTmpData, 0);    //m_pRNTmpData is >128 bytes, prefetching data used in dissolveref here

#if !defined(CONSOLE)
    // detect already culled occluder
    if ((nRndFlags & ERF_GOOD_OCCLUDER))
    {
        if (pEnt->m_pRNTmpData->userData.m_OcclState.nLastOccludedMainFrameID == passInfo.GetMainFrameID())
        {
            return;
        }

        if (pCVars->e_CoverageBufferDrawOccluders)
        {
            return;
        }
    }
#endif // !defined(CONSOLE)

    // skip "outdoor only" objects if outdoor is not visible
    if (pCVars->e_CoverageBuffer == 2 && nRndFlags & ERF_OUTDOORONLY && !Get3DEngine()->GetCoverageBuffer()->IsOutdooVisible())
    {
        return;
    }

    const int nRenderStackLevel = passInfo.GetRecursiveLevel();

    const int nDrawFrame = pEnt->GetDrawFrame(nRenderStackLevel);

    if (eERType != eERType_Light)
    {
        if (nDrawFrame == passInfo.GetFrameID())
        {
            return;
        }
        pEnt->SetDrawFrame(passInfo.GetFrameID(), nRenderStackLevel);
    }

    CVisArea* pVisArea = (CVisArea*)pEnt->GetEntityVisArea();

    const Vec3& vCamPos = passInfo.GetCamera().GetPosition();

    // test only near/big occluders - others will be tested on tree nodes level
    // Note: Not worth prefetch on rCam or objBox as both have been recently used by calling functions & will be in cache - Rich S
    if (!(nRndFlags & ERF_RENDER_ALWAYS) && !objBox.IsContainPoint(vCamPos))
    {
        if (eERType == eERType_Light || fEntDistance < pEnt->m_fWSMaxViewDist * pCVars->e_OcclusionCullingViewDistRatio)
        {
            if (IsBoxOccluded(objBox, fEntDistance * passInfo.GetInverseZoomFactor(), &pEnt->m_pRNTmpData->userData.m_OcclState, pVisArea != NULL, eoot_OBJECT, passInfo))
            {
                return;
            }
        }
    }

    if (eERType == eERType_Light)
    {
        if (nDrawFrame == passInfo.GetFrameID())
        {
            return;
        }
        pEnt->SetDrawFrame(passInfo.GetFrameID(), nRenderStackLevel);
    }

    SRendParams DrawParams;
    DrawParams.pTerrainTexInfo = NULL;
    DrawParams.dwFObjFlags = 0;
    DrawParams.fDistance = fEntDistance;
    DrawParams.AmbientColor = Vec3(0, 0, 0);
    DrawParams.pRenderNode = pEnt;
    //DrawParams.pInstance = pEnt;

    if (eERType != eERType_Light && (pEnt->m_nInternalFlags & IRenderNode::REQUIRES_NEAREST_CUBEMAP))
    {
        uint16 nCubemapTexId = CheckCachedNearestCubeProbe(pEnt);
        if (!nCubemapTexId || !pCVars->e_CacheNearestCubePicking)
        {
            nCubemapTexId = GetNearestCubeProbe(pVisArea, objBox);
        }

        CRNTmpData::SRNUserData* pUserDataRN = (pEnt->m_pRNTmpData) ? &pEnt->m_pRNTmpData->userData : 0;
        if (pUserDataRN)
        {
            pUserDataRN->nCubeMapId = nCubemapTexId;
        }

        DrawParams.nTextureID = nCubemapTexId;
    }

    if (pCVars->e_Dissolve && eERType != eERType_Light && passInfo.IsGeneralPass())
    {
        DrawParams.nDissolveRef = GetDissolveRef(fEntDistance, pEnt->m_fWSMaxViewDist);
        if (DrawParams.nDissolveRef)
        {
            DrawParams.dwFObjFlags |= FOB_DISSOLVE | FOB_DISSOLVE_OUT;
            if (DrawParams.nDissolveRef == 255)
            {
                return;
            }
        }
    }

    DrawParams.nAfterWater = IsAfterWater(objBox.GetCenter(), passInfo) ? 1 : 0;

    if (nRndFlags & ERF_SELECTED)
    {
        DrawParams.dwFObjFlags |= FOB_SELECTED;
    }

    if (pCVars->e_LodForceUpdate)
    {
        pEnt->m_pRNTmpData->userData.nWantedLod = CObjManager::GetObjectLOD(pEnt, fEntDistance);
    }

    // draw bbox
#if !defined(_RELEASE)
    if (pCVars->e_BBoxes)// && eERType != eERType_Light)
    {
        RenderObjectDebugInfo(pEnt, fEntDistance, passInfo);
    }
#endif

    if (pEnt->m_dwRndFlags & ERF_NO_DECALNODE_DECALS)
    {
        DrawParams.dwFObjFlags |= FOB_DYNAMIC_OBJECT;
        DrawParams.NoDecalReceiver = true;
    }

    DrawParams.m_pVisArea =   pVisArea;

#if !defined(_RELEASE)
    if (nRenderStackLevel < 0 || nRenderStackLevel >= MAX_RECURSION_LEVELS)
    {
        CRY_ASSERT_MESSAGE(nRenderStackLevel >= 0 && nRenderStackLevel < MAX_RECURSION_LEVELS, "nRenderStackLevel is outside max recursions level");
    }
#endif

    DrawParams.nClipVolumeStencilRef = 0;
    if (pEnt->m_pRNTmpData && pEnt->m_pRNTmpData->userData.m_pClipVolume)
    {
        DrawParams.nClipVolumeStencilRef = pEnt->m_pRNTmpData->userData.m_pClipVolume->GetStencilRef();
    }

    DrawParams.nMaterialLayers = pEnt->GetMaterialLayers();
    DrawParams.lodValue = pEnt->ComputeLod(pEnt->m_pRNTmpData->userData.nWantedLod, passInfo);
    DrawParams.rendItemSorter = rendItemSorter.GetValue();

    pEnt->Render(DrawParams, passInfo);
}

void CObjManager::RenderAllObjectDebugInfo()
{
    AZ_TRACE_METHOD();
    m_arrRenderDebugInfo.CoalesceMemory();
    for (size_t i = 0; i < m_arrRenderDebugInfo.size(); ++i)
    {
        SObjManRenderDebugInfo& rRenderDebugInfo = m_arrRenderDebugInfo[i];
        if (rRenderDebugInfo.pEnt)
        {
            RenderObjectDebugInfo_Impl(rRenderDebugInfo.pEnt, rRenderDebugInfo.fEntDistance);
        }
    }
    m_arrRenderDebugInfo.resize(0);
}

void CObjManager::RemoveFromRenderAllObjectDebugInfo(IRenderNode* pEnt)
{
    for (size_t i = 0; i < m_arrRenderDebugInfo.size(); ++i)
    {
        SObjManRenderDebugInfo& rRenderDebugInfo = m_arrRenderDebugInfo[i];
        if (rRenderDebugInfo.pEnt == pEnt)
        {
            rRenderDebugInfo.pEnt = NULL;
            break;
        }
    }
}

void CObjManager::RenderObjectDebugInfo_Impl(IRenderNode* pEnt, float fEntDistance)
{
    if (GetCVars()->e_BBoxes > 0)
    {
        ColorF color(1, 1, 1, 1);

        if (GetCVars()->e_BBoxes == 2 && pEnt->GetRndFlags() & ERF_SELECTED)
        {
            color.a *= clamp_tpl(pEnt->GetImportance(), 0.5f, 1.f);
            float fFontSize = max(2.f - fEntDistance * 0.01f, 1.f);

            string sLabel = pEnt->GetDebugString();
            if (sLabel.empty())
            {
                sLabel.Format("%s/%s",
                    pEnt->GetName(), pEnt->GetEntityClassName());
            }
            GetRenderer()->DrawLabelEx(pEnt->GetBBox().GetCenter(), fFontSize, (float*)&color, true, true, "%s", sLabel.c_str());
        }

        IRenderAuxGeom* pRenAux = GetRenderer()->GetIRenderAuxGeom();
        pRenAux->SetRenderFlags(SAuxGeomRenderFlags());
        AABB    rAABB       =   pEnt->GetBBox();
        const float Bias    =   GetCVars()->e_CoverageBufferAABBExpand;
        if (Bias < 0.f)
        {
            rAABB.Expand((rAABB.max - rAABB.min) * Bias - Vec3(Bias, Bias, Bias));
        }
        else
        {
            rAABB.Expand(Vec3(Bias, Bias, Bias));
        }

        pRenAux->DrawAABB(rAABB, false, color, eBBD_Faceted);
    }
}

bool CObjManager::RayRenderMeshIntersection(IRenderMesh* pRenderMesh, const Vec3& vInPos, const Vec3& vInDir, Vec3& vOutPos, Vec3& vOutNormal, bool bFastTest, _smart_ptr<IMaterial> pMat)
{
    FUNCTION_PROFILER_3DENGINE;

    struct MeshLock
    {
        MeshLock(IRenderMesh* m = 0)
            : mesh(m)
        {
            if (m)
            {
                m->LockForThreadAccess();
            }
        }
        ~MeshLock()
        {
            if (mesh)
            {
                mesh->UnlockStream(VSF_GENERAL);
                mesh->UnlockIndexStream();
                mesh->UnLockForThreadAccess();
            }
        }
    private:
        IRenderMesh* mesh;
    };

    MeshLock rmLock(pRenderMesh);

    // get position offset and stride
    int nPosStride = 0;
    byte* pPos  = pRenderMesh->GetPosPtr(nPosStride, FSL_READ);

    // get indices
    vtx_idx* pInds = pRenderMesh->GetIndexPtr(FSL_READ);
    assert(pRenderMesh->GetIndicesCount() % 3 == 0);

    float fClosestHitDistance = -1;

    Lineseg l0(vInPos + vInDir, vInPos - vInDir);
    Lineseg l1(vInPos - vInDir, vInPos + vInDir);

    Vec3 vHitPoint(0, 0, 0);

    //  bool b2DTest = fabs(vInDir.x)<0.001f && fabs(vInDir.y)<0.001f;

    // test tris
    TRenderChunkArray& Chunks = pRenderMesh->GetChunks();
    for (int nChunkId = 0; nChunkId < Chunks.size(); nChunkId++)
    {
        CRenderChunk* pChunk = &Chunks[nChunkId];
        if (pChunk->m_nMatFlags & MTL_FLAG_NODRAW || !pChunk->pRE)
        {
            continue;
        }

        if (pMat)
        {
            const SShaderItem& shaderItem = pMat->GetShaderItem(pChunk->m_nMatID);
            if (!shaderItem.m_pShader || shaderItem.m_pShader->GetFlags() & EF_NODRAW)
            {
                continue;
            }
        }

        AABB triBox;

        int nLastIndexId = pChunk->nFirstIndexId + pChunk->nNumIndices;
        for (int i = pChunk->nFirstIndexId; i < nLastIndexId; i += 3)
        {
            assert((int)pInds[i + 0] < pRenderMesh->GetVerticesCount());
            assert((int)pInds[i + 1] < pRenderMesh->GetVerticesCount());
            assert((int)pInds[i + 2] < pRenderMesh->GetVerticesCount());

            // get vertices
            const Vec3 v0 = (*(Vec3*)&pPos[nPosStride * pInds[i + 0]]);
            const Vec3 v1 = (*(Vec3*)&pPos[nPosStride * pInds[i + 1]]);
            const Vec3 v2 = (*(Vec3*)&pPos[nPosStride * pInds[i + 2]]);
            /*
                        if(b2DTest)
                        {
                            triBox.min = triBox.max = v0;
                            triBox.Add(v1);
                            triBox.Add(v2);
                            if( vInPos.x < triBox.min.x || vInPos.x > triBox.max.x || vInPos.y < triBox.min.y || vInPos.y > triBox.max.y )
                                continue;
                        }
            */
            // make line triangle intersection
            if (Intersect::Lineseg_Triangle(l0, v0, v1, v2, vHitPoint) ||
                Intersect::Lineseg_Triangle(l1, v0, v1, v2, vHitPoint))
            {
                if (bFastTest)
                {
                    return (true);
                }

                float fDist = vHitPoint.GetDistance(vInPos);
                if (fDist < fClosestHitDistance || fClosestHitDistance < 0)
                {
                    fClosestHitDistance = fDist;
                    vOutPos = vHitPoint;
                    vOutNormal = (v1 - v0).Cross(v2 - v0);
                }
            }
        }
    }

    if (fClosestHitDistance >= 0)
    {
        vOutNormal.Normalize();
        return true;
    }

    return false;
}

bool CObjManager::RayStatObjIntersection(IStatObj* pStatObj, const Matrix34& objMatrix, _smart_ptr<IMaterial> pMat,
    Vec3 vStart, Vec3 vEnd, Vec3& vClosestHitPoint, float& fClosestHitDistance, bool bFastTest)
{
    assert(pStatObj);

    CStatObj* pCStatObj = (CStatObj*)pStatObj;

    if (!pCStatObj || pCStatObj->m_nFlags & STATIC_OBJECT_HIDDEN)
    {
        return false;
    }

    Matrix34 matInv = objMatrix.GetInverted();
    Vec3 vOSStart = matInv.TransformPoint(vStart);
    Vec3 vOSEnd = matInv.TransformPoint(vEnd);
    Vec3 vOSHitPoint(0, 0, 0), vOSHitNorm(0, 0, 0);

    Vec3 vBoxHitPoint;
    if (!Intersect::Ray_AABB(Ray(vOSStart, vOSEnd - vOSStart), pCStatObj->GetAABB(), vBoxHitPoint))
    {
        return false;
    }

    bool bHitDetected = false;

    if (IRenderMesh* pRenderMesh = pStatObj->GetRenderMesh())
    {
        if (CObjManager::RayRenderMeshIntersection(pRenderMesh, vOSStart, vOSEnd - vOSStart, vOSHitPoint, vOSHitNorm, bFastTest, pMat))
        {
            bHitDetected = true;
            Vec3 vHitPoint = objMatrix.TransformPoint(vOSHitPoint);
            float fDist = vHitPoint.GetDistance(vStart);
            if (fDist < fClosestHitDistance)
            {
                fClosestHitDistance = fDist;
                vClosestHitPoint = vHitPoint;
            }
        }
    }
    else
    {
        // multi-sub-objects
        for (int s = 0, num = pCStatObj->SubObjectCount(); s < num; s++)
        {
            IStatObj::SSubObject& subObj = pCStatObj->SubObject(s);
            if (subObj.pStatObj && !subObj.bHidden && subObj.nType == STATIC_SUB_OBJECT_MESH)
            {
                Matrix34 subObjMatrix = objMatrix * subObj.tm;
                if (RayStatObjIntersection(subObj.pStatObj, subObjMatrix, pMat, vStart, vEnd, vClosestHitPoint, fClosestHitDistance, bFastTest))
                {
                    bHitDetected = true;
                }
            }
        }
    }

    return bHitDetected;
}
