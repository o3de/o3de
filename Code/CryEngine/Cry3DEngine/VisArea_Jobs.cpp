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

// Description : Visibility areas

#include "Cry3DEngine_precompiled.h"

#include "StatObj.h"
#include "ObjMan.h"
#include "VisAreas.h"
#include "3dEngine.h"
#include "TimeOfDay.h"
#include "AABBSV.h"
#include "Cry_LegacyPhysUtils.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool InsidePolygon(const Vec3* __restrict polygon, int N, const Vec3& p)
{
    int counter = 0;
    int i;
    float xinters;
    const Vec3* p1 = &polygon[0];

    for (i = 1; i <= N; i++)
    {
        const Vec3* p2 = &polygon[i % N];
        if (p.y > min(p1->y, p2->y))
        {
            if (p.y <= max(p1->y, p2->y))
            {
                if (p.x <= max(p1->x, p2->x))
                {
                    if (p1->y != p2->y)
                    {
                        xinters = (p.y - p1->y) * (p2->x - p1->x) / (p2->y - p1->y) + p1->x;
                        if (p1->x == p2->x || p.x <= xinters)
                        {
                            counter++;
                        }
                    }
                }
            }
        }
        p1 = p2;
    }

    if (counter % 2 == 0)
    {
        return(false);
    }
    else
    {
        return(true);
    }
}

///////////////////////////////////////////////////////////////////////////////
bool InsideSpherePolygon(Vec3* polygon, int N, Sphere& S)
{
    int i;
    Vec3 p1, p2;

    p1 = polygon[0];
    for (i = 1; i <= N; i++)
    {
        p2 = polygon[i % N];
        Vec3 vA(p1 - S.center);
        Vec3 vD(p2 - p1);
        vA.z = vD.z = 0;
        vD.NormalizeSafe();
        float fB = vD.Dot(vA);
        float fC = vA.Dot(vA) - S.radius * S.radius;
        if (fB * fB >= fC)           //at least 1 root
        {
            return(true);
        }
        p1 = p2;
    }

    return(false);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void CVisAreaManager::GetNearestCubeProbe(float& fMinDistance, int& nMaxPriority, CLightEntity*& pNearestLight, const AABB* pBBox)
{
    {
        uint32 dwSize = m_lstVisAreas.Count();

        for (uint32 dwI = 0; dwI < dwSize; ++dwI)
        {
            if (m_lstVisAreas[dwI]->m_pObjectsTree)
            {
                if (!pBBox || Overlap::AABB_AABB(*m_lstVisAreas[dwI]->GetAABBox(), *pBBox))
                {
                    m_lstVisAreas[dwI]->m_pObjectsTree->GetNearestCubeProbe(fMinDistance, nMaxPriority, pNearestLight, pBBox);
                }
            }
        }
    }

    {
        uint32 dwSize = m_lstPortals.Count();

        for (uint32 dwI = 0; dwI < dwSize; ++dwI)
        {
            if (m_lstPortals[dwI]->m_pObjectsTree)
            {
                if (!pBBox || Overlap::AABB_AABB(*m_lstPortals[dwI]->GetAABBox(), *pBBox))
                {
                    m_lstPortals[dwI]->m_pObjectsTree->GetNearestCubeProbe(fMinDistance, nMaxPriority, pNearestLight, pBBox);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void CVisAreaManager::GetObjectsByType(PodArray<IRenderNode*>& lstObjects, EERType objType, const AABB* pBBox, ObjectTreeQueryFilterCallback filterCallback)
{
    {
        uint32 dwSize = m_lstVisAreas.Count();

        for (uint32 dwI = 0; dwI < dwSize; ++dwI)
        {
            if (m_lstVisAreas[dwI]->m_pObjectsTree)
            {
                if (!pBBox || Overlap::AABB_AABB(*m_lstVisAreas[dwI]->GetAABBox(), *pBBox))
                {
                    m_lstVisAreas[dwI]->m_pObjectsTree->GetObjectsByType(lstObjects, objType, pBBox, filterCallback);
                }
            }
        }
    }

    {
        uint32 dwSize = m_lstPortals.Count();

        for (uint32 dwI = 0; dwI < dwSize; ++dwI)
        {
            if (m_lstPortals[dwI]->m_pObjectsTree)
            {
                if (!pBBox || Overlap::AABB_AABB(*m_lstPortals[dwI]->GetAABBox(), *pBBox))
                {
                    m_lstPortals[dwI]->m_pObjectsTree->GetObjectsByType(lstObjects, objType, pBBox, filterCallback);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
bool CVisAreaManager::IsOccludedByOcclVolumes(const AABB& objBox, const SRenderingPassInfo& passInfo, bool bCheckOnlyIndoorVolumes)
{
    PodArray<CVisArea*>& rList = bCheckOnlyIndoorVolumes ? m_lstIndoorActiveOcclVolumes : m_lstActiveOcclVolumes;

    for (int i = 0; i < rList.Count(); i++)
    {
        CCamera* pCamera = rList[i]->m_arrOcclCamera[passInfo.GetRecursiveLevel()];
        bool bAllIn = 0;
        if (pCamera)
        {
            // reduce code size by skipping many cache lookups in the camera functions as well as the camera constructor
            char bufCamera[sizeof(CCamera)] _ALIGN(128);
            memcpy(bufCamera, pCamera, sizeof(CCamera));
            if (alias_cast<CCamera*>(bufCamera)->IsAABBVisible_EH(objBox, &bAllIn))
            {
                if (bAllIn)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
IVisArea* CVisAreaManager::GetVisAreaFromPos(const Vec3& vPos)
{
    FUNCTION_PROFILER_3DENGINE;

    if (!m_pAABBTree)
    {
        UpdateAABBTree();
    }

    return m_pAABBTree->FindVisarea(vPos);
}

///////////////////////////////////////////////////////////////////////////////
bool CVisArea::IsBoxOverlapVisArea(const AABB& objBox)
{
    if (!Overlap::AABB_AABB(objBox, m_boxArea))
    {
        return false;
    }

    PodArray<Vec3>& polygonA = s_tmpPolygonA;
    polygonA.Clear();
    polygonA.Add(Vec3(objBox.min.x, objBox.min.y, objBox.min.z));
    polygonA.Add(Vec3(objBox.min.x, objBox.max.y, objBox.min.z));
    polygonA.Add(Vec3(objBox.max.x, objBox.max.y, objBox.min.z));
    polygonA.Add(Vec3(objBox.max.x, objBox.min.y, objBox.min.z));
    return Overlap::Polygon_Polygon2D< PodArray<Vec3> >(polygonA, m_lstShapePoints);
}

#define PORTAL_GEOM_BBOX_EXTENT 1.5f

///////////////////////////////////////////////////////////////////////////////
void CVisArea::UpdateGeometryBBox()
{
    m_boxStatics.max = m_boxArea.max;
    m_boxStatics.min = m_boxArea.min;

    if (IsPortal())
    { // fix for big objects passing portal
        m_boxStatics.max += Vec3(PORTAL_GEOM_BBOX_EXTENT, PORTAL_GEOM_BBOX_EXTENT, PORTAL_GEOM_BBOX_EXTENT);
        m_boxStatics.min -= Vec3(PORTAL_GEOM_BBOX_EXTENT, PORTAL_GEOM_BBOX_EXTENT, PORTAL_GEOM_BBOX_EXTENT);
    }

    if (m_pObjectsTree)
    {
        PodArray<IRenderNode*> lstObjects;
        m_pObjectsTree->GetObjectsByType(lstObjects, eERType_StaticMeshRenderComponent, NULL);

        for (int i = 0; i < lstObjects.Count(); i++)
        {
            AABB aabb;
            lstObjects[i]->FillBBox(aabb);
            m_boxStatics.Add(aabb);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void CVisArea::UpdateClipVolume()
{
    if (m_lstShapePoints.Count() < 3)
    {
        return;
    }

    const int nPoints = m_lstShapePoints.size();
    const int nVertexCount = nPoints * 2;
    const int nIndexCount = (2 * nPoints + 2 * (nPoints - 2)) * 3; // 2*nPoints for sides, nPoints-2 for top and nPoints-2 for bottom

    m_pClipVolumeMesh = NULL;

    if (nPoints < 3)
    {
        return;
    }

    std::vector<SVF_P3F_C4B_T2F> vertices;
    std::vector<vtx_idx> indices;

    vertices.resize(nVertexCount);
    indices.resize(nIndexCount);

    std::vector<Vec2> triangulationPoints;
    triangulationPoints.resize(nPoints + 1);
    MARK_UNUSED triangulationPoints[nPoints].x;

    for (int i = 0; i < nPoints; ++i)
    {
        int nPointIdx = IsShapeClockwise() ? nPoints - 1 - i : i;

        vertices[i].xyz = m_lstShapePoints[nPointIdx];
        vertices[i].color.dcolor = 0xFFFFFFFF;
        vertices[i].st = Vec2(ZERO);

        vertices[i + nPoints].xyz = m_lstShapePoints[nPointIdx] + Vec3(0, 0, m_fHeight);
        vertices[i + nPoints].color.dcolor = 0xFFFFFFFF;
        vertices[i + nPoints].st = Vec2(ZERO);

        triangulationPoints[i] = Vec2(m_lstShapePoints[nPointIdx]);
    }

    // triangulate shape first
    std::vector<int> triangleIndices;
    triangleIndices.resize((nPoints - 2) * 3);
    MARK_UNUSED triangleIndices[triangleIndices.size() - 1];

    int nTris = LegacyCryPhysicsUtils::TriangulatePoly(&triangulationPoints[0], triangulationPoints.size(), &triangleIndices[0], triangleIndices.size());

    if (nTris == nPoints - 2) // triangulation success?
    {
        // top and bottom faces
        size_t nCurIndex = 0;
        for (; nCurIndex < triangleIndices.size(); nCurIndex += 3)
        {
            indices[nCurIndex + 2] = triangleIndices[nCurIndex + 0];
            indices[nCurIndex + 1] = triangleIndices[nCurIndex + 1];
            indices[nCurIndex + 0] = triangleIndices[nCurIndex + 2];

            indices[triangleIndices.size() + nCurIndex + 0] = triangleIndices[nCurIndex + 0] + nPoints;
            indices[triangleIndices.size() + nCurIndex + 1] = triangleIndices[nCurIndex + 1] + nPoints;
            indices[triangleIndices.size() + nCurIndex + 2] = triangleIndices[nCurIndex + 2] + nPoints;
        }

        nCurIndex = 2 * triangleIndices.size();

        // side faces
        for (int i = 0; i < nPoints; i++)
        {
            vtx_idx bl = i;
            vtx_idx br = (i + 1) % nPoints;

            indices[nCurIndex++] = bl;
            indices[nCurIndex++] = br + nPoints;
            indices[nCurIndex++] = bl + nPoints;

            indices[nCurIndex++] = bl;
            indices[nCurIndex++] = br;
            indices[nCurIndex++] = br + nPoints;
        }

        m_pClipVolumeMesh = gEnv->pRenderer->CreateRenderMeshInitialized(&vertices[0], vertices.size(),
                eVF_P3F_C4B_T2F, &indices[0], indices.size(), prtTriangleList,
                "ClipVolume", GetName(), eRMT_Dynamic);
    }
}


void CVisArea::GetClipVolumeMesh(_smart_ptr<IRenderMesh>& renderMesh, Matrix34& worldTM) const
{
    renderMesh = m_pClipVolumeMesh;
    worldTM = Matrix34(IDENTITY);
}

uint CVisArea::GetClipVolumeFlags() const
{
    int nFlags = IClipVolume::eClipVolumeIsVisArea;
    nFlags |= IsConnectedToOutdoor() ? IClipVolume::eClipVolumeConnectedToOutdoor : 0;
    nFlags |= IsAffectedByOutLights() ? IClipVolume::eClipVolumeAffectedBySun : 0;
    nFlags |= IsIgnoringGI() ? IClipVolume::eClipVolumeIgnoreGI : 0;
    nFlags |= IsIgnoringOutdoorAO()   ? IClipVolume::eClipVolumeIgnoreOutdoorAO    : 0;

    return nFlags;
}


///////////////////////////////////////////////////////////////////////////////
void CVisArea::UpdatePortalBlendInfo()
{
    if (m_bThisIsPortal && m_lstConnections.Count() > 0 && GetCVars()->e_PortalsBlend > 0 && m_fPortalBlending > 0)
    {
        SClipVolumeBlendInfo blendInfo;
        Vec3 vPlanePoints[2][3];
        uint nPointCount[2] = {0, 0};

        for (int p = 0; p < m_lstShapePoints.Count(); ++p)
        {
            Vec3 vTestPoint = m_lstShapePoints[p] + Vec3(0, 0, m_fHeight * 0.5f);
            int nVisAreaIndex = (m_lstConnections[0] && m_lstConnections[0]->IsPointInsideVisArea(vTestPoint)) ? 0 : 1;

            if (nPointCount[nVisAreaIndex] < 2)
            {
                vPlanePoints[nVisAreaIndex][nPointCount[nVisAreaIndex]] = m_lstShapePoints[p];
                nPointCount[nVisAreaIndex]++;
            }
        }

        for (int i = 0; i < 2; ++i)
        {
            if (nPointCount[i] == 2)
            {
                if (IsShapeClockwise())
                {
                    std::swap(vPlanePoints[i][0], vPlanePoints[i][1]);
                }

                blendInfo.blendPlanes[i] = Plane::CreatePlane(vPlanePoints[i][0], vPlanePoints[i][0] + Vec3(0, 0, m_fHeight), vPlanePoints[i][1]);
                blendInfo.blendVolumes[i] = i < m_lstConnections.Count() ? m_lstConnections[i] : NULL;

                // make sure plane normal points inside portal
                if (nPointCount[(i + 1) % 2] > 0)
                {
                    Vec3 c(ZERO);
                    for (uint j = 0; j < nPointCount[(i + 1) % 2]; ++j)
                    {
                        c += vPlanePoints[(i + 1) % 2][j];
                    }
                    c /= (float)nPointCount[(i + 1) % 2];

                    if (blendInfo.blendPlanes[i].DistFromPlane(c) < 0)
                    {
                        blendInfo.blendPlanes[i].n = -blendInfo.blendPlanes[i].n;
                        blendInfo.blendPlanes[i].d = -blendInfo.blendPlanes[i].d;
                    }
                }
            }
            else
            {
                blendInfo.blendPlanes[i] = Plane(Vec3(ZERO), 0);
                blendInfo.blendVolumes[i] = NULL;
            }
        }

        // weight planes by user specified importance. works because we renormalize in the shader
        float planeWeight = clamp_tpl(m_fPortalBlending, 1e-5f, 1.f - 1e-5f);

        blendInfo.blendPlanes[0].n *= planeWeight;
        blendInfo.blendPlanes[0].d *= planeWeight;
        blendInfo.blendPlanes[1].n *= 1 - planeWeight;
        blendInfo.blendPlanes[1].d *= 1 - planeWeight;

        GetRenderer()->EF_SetDeferredClipVolumeBlendData(this, blendInfo);
    }
}

///////////////////////////////////////////////////////////////////////////////
bool CVisAreaManager::SetEntityArea(IRenderNode* pEnt, const AABB& objBox, const float fObjRadiusSqr)
{
    assert(pEnt);

    Vec3 vEntCenter = Get3DEngine()->GetEntityRegisterPoint(pEnt);

    // find portal containing object center
    CVisArea* pVisArea = NULL;

    const int kNumPortals = m_lstPortals.Count();
    for (int v = 0; v < kNumPortals; v++)
    {
        CVisArea* pCurrentPortal = m_lstPortals[v];
        PrefetchLine(pCurrentPortal, 256);
        PrefetchLine(pCurrentPortal, 384);
        if (pCurrentPortal->IsPointInsideVisArea(vEntCenter))
        {
            pVisArea = pCurrentPortal;
            if (!pVisArea->m_pObjectsTree)
            {
                pVisArea->m_pObjectsTree = COctreeNode::Create(DEFAULT_SID, pVisArea->m_boxArea, pVisArea);
            }
            pVisArea->m_pObjectsTree->InsertObject(pEnt, objBox, fObjRadiusSqr, vEntCenter);
            break;
        }
    }

    if (!pVisArea && pEnt->m_dwRndFlags & ERF_REGISTER_BY_BBOX)
    {
        AABB aabb;
        pEnt->FillBBox(aabb);

        for (int v = 0; v < kNumPortals; v++)
        {
            CVisArea* pCurrentPortal = m_lstPortals[v];
            PrefetchLine(pCurrentPortal, 256);
            PrefetchLine(pCurrentPortal, 384);
            if (pCurrentPortal->IsBoxOverlapVisArea(aabb))
            {
                pVisArea = pCurrentPortal;
                if (!pVisArea->m_pObjectsTree)
                {
                    pVisArea->m_pObjectsTree = COctreeNode::Create(DEFAULT_SID, pVisArea->m_boxArea, pVisArea);
                }

                pVisArea->m_pObjectsTree->InsertObject(pEnt, objBox, fObjRadiusSqr, vEntCenter);
                break;
            }
        }
    }

    if (!pVisArea) // if portal not found - find volume
    {
        const int kNumVisAreas = m_lstVisAreas.Count();
        for (int v = 0; v < kNumVisAreas; v++)
        {
            CVisArea* pCurrentVisArea = m_lstVisAreas[v];
            PrefetchLine(pCurrentVisArea, 256);
            PrefetchLine(pCurrentVisArea, 384);
            if (pCurrentVisArea->IsPointInsideVisArea(vEntCenter))
            {
                pVisArea = pCurrentVisArea;
                if (!pVisArea->m_pObjectsTree)
                {
                    pVisArea->m_pObjectsTree = COctreeNode::Create(DEFAULT_SID, pVisArea->m_boxArea, pVisArea);
                }
                pVisArea->m_pObjectsTree->InsertObject(pEnt, objBox, fObjRadiusSqr, vEntCenter);
                break;
            }
        }
    }

    if (pVisArea && 
        (pEnt->GetRenderNodeType() == eERType_StaticMeshRenderComponent)
       ) // update bbox of exit portal
    {
        if (pVisArea->IsPortal())
        {
            pVisArea->UpdateGeometryBBox();
        }
    }

    return pVisArea != 0;
}

///////////////////////////////////////////////////////////////////////////////
CVisArea* SAABBTreeNode::FindVisarea(const Vec3& vPos)
{
    if (nodeBox.IsContainPoint(vPos))
    {
        if (nodeAreas.Count())
        { // leaf
            for (int i = 0; i < nodeAreas.Count(); i++)
            {
                if (nodeAreas[i]->m_bActive && nodeAreas[i]->IsPointInsideVisArea(vPos))
                {
                    return nodeAreas[i];
                }
            }
        }
        else
        { // node
            for (int i = 0; i < 2; i++)
            {
                if (arrChilds[i])
                {
                    if (CVisArea* pArea = arrChilds[i]->FindVisarea(vPos))
                    {
                        return pArea;
                    }
                }
            }
        }
    }

    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Current scheme: Will never move sphere center, just clip radius, even to 0.
bool CVisArea::ClipToVisArea(bool bInside, Sphere& sphere, Vec3 const& vNormal)
{
    FUNCTION_PROFILER_3DENGINE;

    /*
        Clip        PointZ  PointXY

        In          In          In                  inside, clip Z and XY
        In          In          Out                 outside, return 0
        In          Out         In                  outside, return 0
        In          Out         Out                 outside, return 0

        Out         In          In                  inside, return 0
        Out         In          Out                 outside, clip XY
        Out         Out         In                  outside, clip Z
        Out         Out         Out                 outside, clip XY
    */

    bool bClipXY = false, bClipZ = false;
    if (bInside)
    {
        // Clip to 0 if center outside.
        if (!IsPointInsideVisArea(sphere.center))
        {
            sphere.radius = 0.f;
            return true;
        }
        bClipXY = bClipZ = true;
    }
    else
    {
        if (Overlap::Point_AABB(sphere.center, m_boxArea))
        {
            if (InsidePolygon(m_lstShapePoints.begin(), m_lstShapePoints.size(), sphere.center))
            {
                sphere.radius = 0.f;
                return true;
            }
            else
            {
                bClipXY = true;
            }
        }
        else if (InsidePolygon(m_lstShapePoints.begin(), m_lstShapePoints.size(), sphere.center))
        {
            bClipZ = true;
        }
        else
        {
            bClipXY = true;
        }
    }

    float fOrigRadius = sphere.radius;
    if (bClipZ)
    {
        // Check against vertical planes.
        float fDist = min(abs(m_boxArea.max.z - sphere.center.z), abs(sphere.center.z - m_boxArea.min.z));
        float fRadiusScale = sqrt_tpl(max(1.f - sqr(vNormal.z), 0.f));
        if (fDist < sphere.radius * fRadiusScale)
        {
            sphere.radius = fDist / fRadiusScale;
            if (sphere.radius <= 0.f)
            {
                return true;
            }
        }
    }

    if (bClipXY)
    {
        Vec3 vP1 = m_lstShapePoints[0];
        vP1.z = clamp_tpl(sphere.center.z, m_boxArea.min.z, m_boxArea.max.z);
        for (int n = m_lstShapePoints.Count() - 1; n >= 0; n--)
        {
            Vec3 vP0 = m_lstShapePoints[n];
            vP0.z = vP1.z;

            // Compute nearest vector from center to plane.
            Vec3 vP = vP0 - sphere.center;
            Vec3 vD = vP1 - vP0;
            float fN = -(vP * vD);
            if (fN > 0.f)
            {
                float fD = vD.GetLengthSquared();
                if (fN >= fD)
                {
                    vP += vD;
                }
                else
                {
                    vP += vD * (fN / fD);
                }
            }

            // Check distance only in planar direction.
            float fDist = vP.GetLength() * 0.99f;
            float fRadiusScale = fDist > 0.f ? sqrt_tpl(max(1.f - sqr(vNormal * vP) / sqr(fDist), 0.f)) : 1.f;
            if (fDist < sphere.radius * fRadiusScale)
            {
                sphere.radius = fDist / fRadiusScale;
                if (sphere.radius <= 0.f)
                {
                    return true;
                }
            }

            vP1 = vP0;
        }
    }

    return sphere.radius < fOrigRadius;
}

///////////////////////////////////////////////////////////////////////////////
bool CVisArea::IsPointInsideVisArea(const Vec3& vPos) const
{
    const int kNumPoints = m_lstShapePoints.Count();
    if (kNumPoints)
    {
        const Vec3* pLstShapePoints = &m_lstShapePoints[0];
        PrefetchLine(pLstShapePoints, 0);
        if (Overlap::Point_AABB(vPos, m_boxArea))
        {
            if (InsidePolygon(pLstShapePoints, kNumPoints, vPos))
            {
                return true;
            }
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
bool CVisArea::IsSphereInsideVisArea(const Vec3& vPos, const f32 fRadius)
{
    Sphere S(vPos, fRadius);
    if (Overlap::Sphere_AABB(S, m_boxArea))
    {
        if (InsidePolygon(&m_lstShapePoints[0], m_lstShapePoints.Count(), vPos) || InsideSpherePolygon(&m_lstShapePoints[0], m_lstShapePoints.Count(), S))
        {
            return true;
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
const AABB* CVisArea::GetAABBox() const
{
    return &m_boxArea;
}

///////////////////////////////////////////////////////////////////////////////
bool CVisArea::IsPortal() const
{
    return m_bThisIsPortal;
}

float CVisArea::CalcSignedArea()
{
    float fArea = 0;
    for (int i = 0; i < m_lstShapePoints.Count(); i++)
    {
        const Vec3& v0 = m_lstShapePoints[i];
        const Vec3& v1 = m_lstShapePoints[(i + 1) % m_lstShapePoints.Count()];
        fArea += v0.x * v1.y - v1.x * v0.y;
    }
    return fArea / 2;
}

void CVisArea::GetShapePoints(const Vec3*& pPoints, size_t& nPoints)
{
    if (m_lstShapePoints.empty())
    {
        pPoints = 0;
        nPoints = 0;
    }
    else
    {
        pPoints = &m_lstShapePoints[0];
        nPoints = m_lstShapePoints.size();
    }
}

float CVisArea::GetHeight()
{
    return m_fHeight;
}

///////////////////////////////////////////////////////////////////////////////
bool CVisArea::FindVisArea(IVisArea* pAnotherArea, int nMaxRecursion, bool bSkipDisabledPortals)
{
    // collect visited areas in order to prevent visiting it again
    StaticDynArray<CVisArea*, 1024> lVisitedParents;

    return FindVisAreaReqursive(pAnotherArea, nMaxRecursion, bSkipDisabledPortals, lVisitedParents);
}

///////////////////////////////////////////////////////////////////////////////
bool CVisArea::FindVisAreaReqursive(IVisArea* pAnotherArea, int nMaxReqursion, bool bSkipDisabledPortals, StaticDynArray<CVisArea*, 1024>& arrVisitedParents)
{
    arrVisitedParents.push_back(this);

    if (pAnotherArea == this)
    {
        return true;
    }

    if (pAnotherArea == NULL && IsConnectedToOutdoor())
    {
        return true;
    }

    bool bFound = false;

    if (nMaxReqursion > 1)
    {
        for (int p = 0; p < m_lstConnections.Count(); p++)
        {
            if (!bSkipDisabledPortals || m_lstConnections[p]->IsActive())
            {
                if (std::find(arrVisitedParents.begin(), arrVisitedParents.end(), m_lstConnections[p]) == arrVisitedParents.end())
                {
                    if (m_lstConnections[p]->FindVisAreaReqursive(pAnotherArea, nMaxReqursion - 1, bSkipDisabledPortals, arrVisitedParents))
                    {
                        bFound = true;
                        break;
                    }
                }
            }
        }
    }

    return bFound;
}

///////////////////////////////////////////////////////////////////////////////
bool CVisArea::IsConnectedToOutdoor() const
{
    if (IsPortal()) // check if this portal has just one conection
    {
        return m_lstConnections.Count() == 1;
    }

    // find portals with just one conection
    for (int p = 0; p < m_lstConnections.Count(); p++)
    {
        CVisArea* pPortal = m_lstConnections[p];
        if (pPortal->m_lstConnections.Count() == 1)
        {
            return true;
        }
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

