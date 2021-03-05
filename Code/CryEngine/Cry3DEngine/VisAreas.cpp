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

PodArray<CVisArea*> CVisArea::m_lUnavailableAreas;
PodArray<Vec3> CVisArea::s_tmpLstPortVertsClipped;
PodArray<Vec3> CVisArea::s_tmpLstPortVertsSS;
PodArray<Vec3> CVisArea::s_tmpPolygonA;
PodArray<IRenderNode*>  CVisArea::s_tmpLstLights;

CPolygonClipContext CVisArea::s_tmpClipContext;
PodArray<CCamera> CVisArea::s_tmpCameras;
int CVisArea::s_nGetDistanceThruVisAreasCallCounter = 0;

void CVisArea::Update(const Vec3* pPoints, int nCount, const char* szName, const SVisAreaInfo& info)
{
    assert(m_pVisAreaColdData);


    cry_strcpy(m_pVisAreaColdData->m_sName, szName);
    _strlwr_s(m_pVisAreaColdData->m_sName, sizeof(m_pVisAreaColdData->m_sName));

    m_bThisIsPortal = strstr(m_pVisAreaColdData->m_sName, "portal") != 0;
    m_bIgnoreSky = (strstr(m_pVisAreaColdData->m_sName, "ignoresky") != 0) || info.bIgnoreSkyColor;

    m_fHeight = info.fHeight;
    m_vAmbientColor = info.vAmbientColor;
    m_bAffectedByOutLights = info.bAffectedByOutLights;
    m_bSkyOnly = info.bSkyOnly;
    m_fViewDistRatio = info.fViewDistRatio;
    m_bDoubleSide = info.bDoubleSide;
    m_bUseDeepness = info.bUseDeepness;
    m_bUseInIndoors = info.bUseInIndoors;
    m_bOceanVisible = info.bOceanIsVisible;
    m_bIgnoreGI = info.bIgnoreGI;
    m_bIgnoreOutdoorAO = info.bIgnoreOutdoorAO;
    m_fPortalBlending = info.fPortalBlending;

    m_lstShapePoints.PreAllocate(nCount, nCount);

    if (nCount)
    {
        memcpy(&m_lstShapePoints[0], pPoints, sizeof(Vec3) * nCount);
    }

    // update bbox
    m_boxArea.max = SetMinBB();
    m_boxArea.min = SetMaxBB();

    for (int i = 0; i < nCount; i++)
    {
        m_boxArea.max.CheckMax(pPoints[i]);
        m_boxArea.min.CheckMin(pPoints[i]);

        m_boxArea.max.CheckMax(pPoints[i] + Vec3(0, 0, m_fHeight));
        m_boxArea.min.CheckMin(pPoints[i] + Vec3(0, 0, m_fHeight));
    }

    UpdateGeometryBBox();
    UpdateClipVolume();
}


void CVisArea::StaticReset()
{
    stl::free_container(m_lUnavailableAreas);
    stl::free_container(s_tmpLstPortVertsClipped);
    stl::free_container(s_tmpLstPortVertsSS);
    stl::free_container(s_tmpPolygonA);
    stl::free_container(s_tmpLstLights);
    stl::free_container(s_tmpCameras);
    s_tmpClipContext.Reset();
}

void CVisArea::Init()
{
    m_fGetDistanceThruVisAreasMinDistance = 10000.f;
    m_nGetDistanceThruVisAreasLastCallID = -1;
    m_pVisAreaColdData = NULL;
    m_boxStatics.min = m_boxStatics.max = m_boxArea.min = m_boxArea.max = Vec3(0, 0, 0);
    m_nRndFrameId = -1;
    m_bActive = true;
    m_fHeight = 0;
    m_vAmbientColor(0, 0, 0);
    m_vConnNormals[0] = m_vConnNormals[1] = Vec3(0, 0, 0);
    m_bAffectedByOutLights = false;
    m_fDistance = 0;
    m_bOceanVisible = m_bSkyOnly = false;
    memset(m_arrOcclCamera, 0, sizeof(m_arrOcclCamera));
    m_fViewDistRatio = 100.f;
    m_bDoubleSide = true;
    m_bUseDeepness = false;
    m_bUseInIndoors = false;
    m_bIgnoreSky = m_bThisIsPortal = false;
    m_bIgnoreGI = false;
    m_bIgnoreOutdoorAO = false;
    m_lstCurCamerasCap = 0;
    m_lstCurCamerasLen = 0;
    m_lstCurCamerasIdx = 0;
    m_nVisGUID = 0;
    m_fPortalBlending = 0.5f;
    m_nStencilRef = 0;
}

CVisArea::CVisArea()
{
    Init();
}

CVisArea::CVisArea(VisAreaGUID visGUID)
{
    Init();
    m_nVisGUID = visGUID;
}

CVisArea::~CVisArea()
{
    for (int i = 0; i < MAX_RECURSION_LEVELS; i++)
    {
        SAFE_DELETE(m_arrOcclCamera[i]);
    }

    GetVisAreaManager()->OnVisAreaDeleted(this);

    if (m_pVisAreaColdData->m_dataType == eCDT_Portal)
    {
        SPortalColdData* pPortalColdData = static_cast<SPortalColdData*>(m_pVisAreaColdData);
        if (pPortalColdData->m_pRNTmpData)
        {
            Get3DEngine()->FreeRNTmpData(&pPortalColdData->m_pRNTmpData);
        }
    }
}

float LineSegDistanceSqr(const Vec3& vPos, const Vec3& vP0, const Vec3& vP1)
{
    // Dist of line seg A(+D) from origin:
    // P = A + D t[0..1]
    // d^2(t) = (A + D t)^2 = A^2 + 2 A*D t + D^2 t^2
    // d^2\t = 2 A*D + 2 D^2 t = 0
    // tmin = -A*D / D^2 clamp_tpl(0,1)
    // Pmin = A + D tmin
    Vec3 vP = vP0 - vPos;
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
    return vP.GetLengthSquared();
}

void CVisArea::FindSurroundingVisAreaReqursive(int nMaxReqursion, bool bSkipDisabledPortals, PodArray<IVisArea*>* pVisitedAreas, int nMaxVisitedAreas, int nDeepness, PodArray<CVisArea*>* pUnavailableAreas)
{
    pUnavailableAreas->Add(this);

    if (pVisitedAreas && pVisitedAreas->Count() < nMaxVisitedAreas)
    {
        pVisitedAreas->Add(this);
    }

    if (nMaxReqursion > (nDeepness + 1))
    {
        for (int p = 0; p < m_lstConnections.Count(); p++)
        {
            if (!bSkipDisabledPortals || m_lstConnections[p]->IsActive())
            {
                if (-1 == pUnavailableAreas->Find(m_lstConnections[p]))
                {
                    m_lstConnections[p]->FindSurroundingVisAreaReqursive(nMaxReqursion, bSkipDisabledPortals, pVisitedAreas, nMaxVisitedAreas, nDeepness + 1, pUnavailableAreas);
                }
            }
        }
    }
}

void CVisArea::FindSurroundingVisArea(int nMaxReqursion, bool bSkipDisabledPortals, PodArray<IVisArea*>* pVisitedAreas, int nMaxVisitedAreas, int nDeepness)
{
    if (pVisitedAreas)
    {
        if (pVisitedAreas->capacity() < (uint32)nMaxVisitedAreas)
        {
            pVisitedAreas->PreAllocate(nMaxVisitedAreas);
        }
    }

    m_lUnavailableAreas.Clear();
    m_lUnavailableAreas.PreAllocate(nMaxVisitedAreas, 0);

    FindSurroundingVisAreaReqursive(nMaxReqursion, bSkipDisabledPortals, pVisitedAreas, nMaxVisitedAreas, nDeepness, &m_lUnavailableAreas);
}

int CVisArea::GetVisFrameId()
{
    return m_nRndFrameId;
}

bool Is2dLinesIntersect(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
    float fDiv = (y4 - y3) * (x2 - x1) - (x4 - x3) * (y2 - y1);

    if (fabs(fDiv) < 0.00001f)
    {
        return false;
    }

    float ua = ((x4 - x3) * (y1 - y3) - (y4 - y3) * (x1 - x3)) / fDiv;
    float ub = ((x2 - x1) * (y1 - y3) - (y2 - y1) * (x1 - x3)) / fDiv;

    return ua > 0 && ua < 1 && ub > 0 && ub < 1;
}


Vec3 CVisArea::GetConnectionNormal(CVisArea* pPortal)
{
    assert(m_lstShapePoints.Count() >= 3);
    // find side of shape intersecting with portal
    int nIntersNum = 0;
    Vec3 arrNormals[2] = {Vec3(0, 0, 0), Vec3(0, 0, 0)};
    for (int v = 0; v < m_lstShapePoints.Count(); v++)
    {
        nIntersNum = 0;
        arrNormals[0] = Vec3(0, 0, 0);
        arrNormals[1] = Vec3(0, 0, 0);
        const Vec3& v0 = m_lstShapePoints[v];
        const Vec3& v1 = m_lstShapePoints[(v + 1) % m_lstShapePoints.Count()];
        for (int p = 0; p < pPortal->m_lstShapePoints.Count(); p++)
        {
            const Vec3& p0 = pPortal->m_lstShapePoints[p];
            const Vec3& p1 = pPortal->m_lstShapePoints[(p + 1) % pPortal->m_lstShapePoints.Count()];

            if (Is2dLinesIntersect(v0.x, v0.y, v1.x, v1.y, p0.x, p0.y, p1.x, p1.y))
            {
                Vec3 vNormal = (v0 - v1).GetNormalized().Cross(Vec3(0, 0, 1.f));
                if (nIntersNum < 2)
                {
                    arrNormals[nIntersNum++] = (IsShapeClockwise()) ? -vNormal : vNormal;
                }
            }
        }

        if (nIntersNum == 2)
        {
            break;
        }
    }

    if (nIntersNum == 2 &&
        //IsEquivalent(arrNormals[0] == arrNormals[1])
        arrNormals[0].IsEquivalent(arrNormals[1], VEC_EPSILON)
        )
    {
        return arrNormals[0];
    }

    {
        int nBottomPoints = 0;
        for (int p = 0; p < pPortal->m_lstShapePoints.Count() && p < 4; p++)
        {
            if (IsPointInsideVisArea(pPortal->m_lstShapePoints[p]))
            {
                nBottomPoints++;
            }
        }

        int nUpPoints = 0;
        for (int p = 0; p < pPortal->m_lstShapePoints.Count() && p < 4; p++)
        {
            if (IsPointInsideVisArea(pPortal->m_lstShapePoints[p] + Vec3(0, 0, pPortal->m_fHeight)))
            {
                nUpPoints++;
            }
        }

        if (nBottomPoints == 0 && nUpPoints == 4)
        {
            return Vec3(0, 0, 1);
        }

        if (nBottomPoints == 4 && nUpPoints == 0)
        {
            return Vec3(0, 0, -1);
        }
    }

    return Vec3(0, 0, 0);
}

void CVisArea::UpdatePortalCameraPlanes(CCamera& cam, Vec3* pVerts, bool NotForcePlaneSet, const SRenderingPassInfo& passInfo)
{
    const Vec3& vCamPos = passInfo.GetCamera().GetPosition();
    Plane plane, farPlane;

    farPlane = *passInfo.GetCamera().GetFrustumPlane(FR_PLANE_FAR);
    cam.SetFrustumPlane(FR_PLANE_FAR, farPlane);

    //plane.SetPlane(pVerts[0],pVerts[2],pVerts[1]);  // can potentially create a plane facing the wrong way
    plane.SetPlane(-farPlane.n, pVerts[0]);
    cam.SetFrustumPlane(FR_PLANE_NEAR, plane);

    plane.SetPlane(vCamPos, pVerts[3], pVerts[2]);    // update plane only if it reduces fov
    if (!NotForcePlaneSet || plane.n.Dot(cam.GetFrustumPlane(FR_PLANE_LEFT)->n) <
        cam.GetFrustumPlane(FR_PLANE_RIGHT)->n.Dot(cam.GetFrustumPlane(FR_PLANE_LEFT)->n))
    {
        cam.SetFrustumPlane(FR_PLANE_RIGHT, plane);
    }

    plane.SetPlane(vCamPos, pVerts[1], pVerts[0]);    // update plane only if it reduces fov
    if (!NotForcePlaneSet || plane.n.Dot(cam.GetFrustumPlane(FR_PLANE_RIGHT)->n) <
        cam.GetFrustumPlane(FR_PLANE_LEFT)->n.Dot(cam.GetFrustumPlane(FR_PLANE_RIGHT)->n))
    {
        cam.SetFrustumPlane(FR_PLANE_LEFT, plane);
    }

    plane.SetPlane(vCamPos, pVerts[0], pVerts[3]);    // update plane only if it reduces fov
    if (!NotForcePlaneSet || plane.n.Dot(cam.GetFrustumPlane(FR_PLANE_TOP)->n) <
        cam.GetFrustumPlane(FR_PLANE_BOTTOM)->n.Dot(cam.GetFrustumPlane(FR_PLANE_TOP)->n))
    {
        cam.SetFrustumPlane(FR_PLANE_BOTTOM, plane);
    }

    plane.SetPlane(vCamPos, pVerts[2], pVerts[1]); // update plane only if it reduces fov
    if (!NotForcePlaneSet || plane.n.Dot(cam.GetFrustumPlane(FR_PLANE_BOTTOM)->n) <
        cam.GetFrustumPlane(FR_PLANE_TOP)->n.Dot(cam.GetFrustumPlane(FR_PLANE_BOTTOM)->n))
    {
        cam.SetFrustumPlane(FR_PLANE_TOP, plane);
    }

    Vec3 arrvPortVertsCamSpace[4];
    for (int i = 0; i < 4; i++)
    {
        arrvPortVertsCamSpace[i] = pVerts[i] - cam.GetPosition();
    }
    cam.SetFrustumVertices(arrvPortVertsCamSpace);

    //cam.UpdateFrustum();

    if (GetCVars()->e_Portals == 5)
    {
        float farrColor[4] = {1, 1, 1, 1};
        //      GetRenderer()->SetMaterialColor(1,1,1,1);
        DrawLine(pVerts[0], pVerts[1]);
        GetRenderer()->DrawLabelEx(pVerts[0], 1, farrColor, false, true, "0");
        DrawLine(pVerts[1], pVerts[2]);
        GetRenderer()->DrawLabelEx(pVerts[1], 1, farrColor, false, true, "1");
        DrawLine(pVerts[2], pVerts[3]);
        GetRenderer()->DrawLabelEx(pVerts[2], 1, farrColor, false, true, "2");
        DrawLine(pVerts[3], pVerts[0]);
        GetRenderer()->DrawLabelEx(pVerts[3], 1, farrColor, false, true, "3");
    }
}

int __cdecl CVisAreaManager__CmpDistToPortal(const void* v1, const void* v2);

void CVisArea::PreRender(int nReqursionLevel,
    CCamera CurCamera, CVisArea* pParent, CVisArea* pCurPortal,
    bool* pbOutdoorVisible, PodArray<CCamera>* plstOutPortCameras, bool* pbSkyVisible, bool* pbOceanVisible,
    PodArray<CVisArea*>& lstVisibleAreas,
    const SRenderingPassInfo& passInfo)
{
    // mark as rendered
    if (m_nRndFrameId != passInfo.GetFrameID())
    {
        m_lstCurCamerasIdx = 0;
        m_lstCurCamerasLen = 0;
        if (m_lstCurCamerasCap)
        {
            m_lstCurCamerasIdx = s_tmpCameras.size();
            s_tmpCameras.resize(s_tmpCameras.size() + m_lstCurCamerasCap);
        }
    }

    m_nRndFrameId = passInfo.GetFrameID();

    if (m_bAffectedByOutLights)
    {
        GetVisAreaManager()->m_bSunIsNeeded = true;
    }

    if (m_lstCurCamerasLen == m_lstCurCamerasCap)
    {
        int newIdx = s_tmpCameras.size();

        m_lstCurCamerasCap +=  max(1, m_lstCurCamerasCap / 2);
        s_tmpCameras.resize(newIdx + m_lstCurCamerasCap);
        if (m_lstCurCamerasLen)
        {
            memcpy(&s_tmpCameras[newIdx], &s_tmpCameras[m_lstCurCamerasIdx], m_lstCurCamerasLen * sizeof(CCamera));
        }

        m_lstCurCamerasIdx = newIdx;
    }
    s_tmpCameras[m_lstCurCamerasIdx + m_lstCurCamerasLen] = CurCamera;
    ++m_lstCurCamerasLen;

    if (lstVisibleAreas.Find(this) < 0)
    {
        lstVisibleAreas.Add(this);
        m_nStencilRef = GetRenderer()->EF_AddDeferredClipVolume(this);
    }

    // check recursion and portal activity
    if (!nReqursionLevel || !m_bActive)
    {
        return;
    }

    if (pParent && m_bThisIsPortal && m_lstConnections.Count() == 1 && // detect entrance
        !IsPointInsideVisArea(passInfo.GetCamera().GetPosition()))   // detect camera in outdoors
    {
        AABB boxAreaEx = m_boxArea;
        float fZNear = CurCamera.GetNearPlane();
        boxAreaEx.min -= Vec3(fZNear, fZNear, fZNear);
        boxAreaEx.max += Vec3(fZNear, fZNear, fZNear);
        if (!CurCamera.IsAABBVisible_E(boxAreaEx)) // if portal is invisible
        {
            return; // stop recursion
        }
    }

    bool bCanSeeThruThisArea = true;

    // prepare new camera for next areas
    if (m_bThisIsPortal && m_lstConnections.Count() &&
        (this != pCurPortal || !pCurPortal->IsPointInsideVisArea(CurCamera.GetPosition())))
    {
        Vec3 vCenter = m_boxArea.GetCenter();
        float fRadius = m_boxArea.GetRadius();

        Vec3 vPortNorm = (!pParent || pParent == m_lstConnections[0] || m_lstConnections.Count() == 1) ?
            m_vConnNormals[0] : m_vConnNormals[1];

        // exit/entrance portal has only one normal in direction to outdoors, so flip it to the camera
        if (m_lstConnections.Count() == 1 && !pParent)
        {
            vPortNorm = -vPortNorm;
        }

        // back face check
        Vec3 vPortToCamDir = CurCamera.GetPosition() - vCenter;
        if (vPortToCamDir.Dot(vPortNorm) < 0)
        {
            return;
        }

        if (!m_bDoubleSide)
        {
            if (vPortToCamDir.Dot(m_vConnNormals[0]) < 0)
            {
                return;
            }
        }

        Vec3 arrPortVerts[4];
        Vec3 arrPortVertsOtherSide[4];
        bool barrPortVertsOtherSideValid = false;
        if (pParent && !vPortNorm.IsEquivalent(Vec3(0, 0, 0), VEC_EPSILON) && vPortNorm.z)
        { // up/down portal
            int nEven = IsShapeClockwise();
            if (vPortNorm.z > 0)
            {
                nEven = !nEven;
            }
            for (int i = 0; i < 4; i++)
            {
                arrPortVerts[i] = m_lstShapePoints[nEven ? (3 - i) : i] + Vec3(0, 0, m_fHeight) * (vPortNorm.z > 0);
                arrPortVertsOtherSide[i] = m_lstShapePoints[nEven ? (3 - i) : i] + Vec3(0, 0, m_fHeight) * (vPortNorm.z < 0);
            }
            barrPortVertsOtherSideValid = true;
        }
        else if (!vPortNorm.IsEquivalent(Vec3(0, 0, 0), VEC_EPSILON)    && vPortNorm.z == 0)
        { // basic portal
            Vec3 arrInAreaPoint[2] = {Vec3(0, 0, 0), Vec3(0, 0, 0)};
            int arrInAreaPointId[2] = {-1, -1};
            int nInAreaPointCounter = 0;

            Vec3 arrOutAreaPoint[2] = {Vec3(0, 0, 0), Vec3(0, 0, 0)};
            int nOutAreaPointCounter = 0;

            // find 2 points of portal in this area (or in this outdoors)
            for (int i = 0; i < m_lstShapePoints.Count() && nInAreaPointCounter < 2; i++)
            {
                Vec3 vTestPoint = m_lstShapePoints[i] + Vec3(0, 0, m_fHeight * 0.5f);
                CVisArea* pAnotherArea = m_lstConnections[0];
                if ((pParent && (pParent->IsPointInsideVisArea(vTestPoint))) ||
                    (!pParent && (!pAnotherArea->IsPointInsideVisArea(vTestPoint))))
                {
                    arrInAreaPointId[nInAreaPointCounter] = i;
                    arrInAreaPoint[nInAreaPointCounter++] = m_lstShapePoints[i];
                }
            }

            // find 2 points of portal not in this area (or not in this outdoors)
            for (int i = 0; i < m_lstShapePoints.Count() && nOutAreaPointCounter < 2; i++)
            {
                Vec3 vTestPoint = m_lstShapePoints[i] + Vec3(0, 0, m_fHeight * 0.5f);
                CVisArea* pAnotherArea = m_lstConnections[0];
                if ((pParent && (pParent->IsPointInsideVisArea(vTestPoint))) ||
                    (!pParent && (!pAnotherArea->IsPointInsideVisArea(vTestPoint))))
                {
                }
                else
                {
                    arrOutAreaPoint[nOutAreaPointCounter++] = m_lstShapePoints[i];
                }
            }

            if (nInAreaPointCounter == 2)
            { // success, take into account volume and portal shape versts order
                int nEven = IsShapeClockwise();
                if (arrInAreaPointId[1] - arrInAreaPointId[0] != 1)
                {
                    nEven = !nEven;
                }

                arrPortVerts[0] = arrInAreaPoint[nEven];
                arrPortVerts[1] = arrInAreaPoint[nEven] + Vec3(0, 0, m_fHeight);
                arrPortVerts[2] = arrInAreaPoint[!nEven] + Vec3(0, 0, m_fHeight);
                arrPortVerts[3] = arrInAreaPoint[!nEven];

                nEven = !nEven;

                arrPortVertsOtherSide[0] = arrOutAreaPoint[nEven];
                arrPortVertsOtherSide[1] = arrOutAreaPoint[nEven] + Vec3(0, 0, m_fHeight);
                arrPortVertsOtherSide[2] = arrOutAreaPoint[!nEven] + Vec3(0, 0, m_fHeight);
                arrPortVertsOtherSide[3] = arrOutAreaPoint[!nEven];
                barrPortVertsOtherSideValid = true;
            }
            else
            { // something wrong
                Warning("CVisArea::PreRender: Invalid portal: %s", m_pVisAreaColdData->m_sName);
                return;
            }
        }
        else if (!pParent && vPortNorm.z == 0 && m_lstConnections.Count() == 1)
        { // basic entrance portal
            Vec3 vBorder = (vPortNorm.Cross(Vec3(0, 0, 1.f))).GetNormalized() * fRadius;
            arrPortVerts[0] = vCenter - Vec3(0, 0, 1.f) * fRadius - vBorder;
            arrPortVerts[1] = vCenter + Vec3(0, 0, 1.f) * fRadius - vBorder;
            arrPortVerts[2] = vCenter + Vec3(0, 0, 1.f) * fRadius + vBorder;
            arrPortVerts[3] = vCenter - Vec3(0, 0, 1.f) * fRadius + vBorder;
        }
        else if (!pParent && vPortNorm.z != 0 && m_lstConnections.Count() == 1)
        { // up/down entrance portal
            Vec3 vBorder = (vPortNorm.Cross(Vec3(0, 1, 0.f))).GetNormalized() * fRadius;
            arrPortVerts[0] = vCenter - Vec3(0, 1, 0.f) * fRadius + vBorder;
            arrPortVerts[1] = vCenter + Vec3(0, 1, 0.f) * fRadius + vBorder;
            arrPortVerts[2] = vCenter + Vec3(0, 1, 0.f) * fRadius - vBorder;
            arrPortVerts[3] = vCenter - Vec3(0, 1, 0.f) * fRadius - vBorder;
        }
        else
        { // something wrong or areabox portal - use simple solution
            if (vPortNorm.IsEquivalent(Vec3(0, 0, 0), VEC_EPSILON))
            {
                vPortNorm = (vCenter - passInfo.GetCamera().GetPosition()).GetNormalized();
            }

            Vec3 vBorder = (vPortNorm.Cross(Vec3(0, 0, 1.f))).GetNormalized() * fRadius;
            arrPortVerts[0] = vCenter - Vec3(0, 0, 1.f) * fRadius - vBorder;
            arrPortVerts[1] = vCenter + Vec3(0, 0, 1.f) * fRadius - vBorder;
            arrPortVerts[2] = vCenter + Vec3(0, 0, 1.f) * fRadius + vBorder;
            arrPortVerts[3] = vCenter - Vec3(0, 0, 1.f) * fRadius + vBorder;
        }

        if (GetCVars()->e_Portals == 4) // make color recursion dependent
        {
            GetRenderer()->SetMaterialColor(1, 1, passInfo.IsGeneralPass(), 1);
        }

        Vec3 vPortalFaceCenter = (arrPortVerts[0] + arrPortVerts[1] + arrPortVerts[2] + arrPortVerts[3]) / 4;
        vPortToCamDir = CurCamera.GetPosition() - vPortalFaceCenter;
        if (vPortToCamDir.GetNormalized().Dot(vPortNorm) < -0.01f)
        {
            UpdatePortalBlendInfo();
            return;
        }

        const bool Upright  =   (fabsf(vPortNorm.z) < FLT_EPSILON);
        CCamera camParent = CurCamera;

        // clip portal quad by camera planes
        PodArray<Vec3>& lstPortVertsClipped = s_tmpLstPortVertsClipped;
        lstPortVertsClipped.Clear();
        lstPortVertsClipped.AddList(arrPortVerts, 4);
        ClipPortalVerticesByCameraFrustum(&lstPortVertsClipped, camParent);

        AABB aabb;
        aabb.Reset();

        if (lstPortVertsClipped.Count() > 2)
        {
            // find screen space bounds of clipped portal
            for (int i = 0; i < lstPortVertsClipped.Count(); i++)
            {
                Vec3 vSS;
                GetRenderer()->ProjectToScreen(lstPortVertsClipped[i].x, lstPortVertsClipped[i].y, lstPortVertsClipped[i].z, &vSS.x, &vSS.y, &vSS.z);
                vSS.y = 100 - vSS.y;
                aabb.Add(vSS);
            }
        }
        else
        {
            if (!CVisArea::IsSphereInsideVisArea(CurCamera.GetPosition(), CurCamera.GetNearPlane()))
            {
                bCanSeeThruThisArea = false;
            }
        }

        if (lstPortVertsClipped.Count() > 2 && aabb.min.z > 0.01f)
        {
            PodArray<Vec3>& lstPortVertsSS = s_tmpLstPortVertsSS;
            lstPortVertsSS.PreAllocate(4, 4);

            // get 3d positions of portal bounds
            {
                int i = 0;
                float w = (float)GetRenderer()->GetWidth();
                float h = (float)GetRenderer()->GetHeight();
                float d = 0.01f;

                GetRenderer()->UnProjectFromScreen(aabb.min.x * w / 100, aabb.min.y * h / 100, d, &lstPortVertsSS[i].x, &lstPortVertsSS[i].y, &lstPortVertsSS[i].z);
                i++;
                GetRenderer()->UnProjectFromScreen(aabb.min.x * w / 100, aabb.max.y * h / 100, d, &lstPortVertsSS[i].x, &lstPortVertsSS[i].y, &lstPortVertsSS[i].z);
                i++;
                GetRenderer()->UnProjectFromScreen(aabb.max.x * w / 100, aabb.max.y * h / 100, d, &lstPortVertsSS[i].x, &lstPortVertsSS[i].y, &lstPortVertsSS[i].z);
                i++;
                GetRenderer()->UnProjectFromScreen(aabb.max.x * w / 100, aabb.min.y * h / 100, d, &lstPortVertsSS[i].x, &lstPortVertsSS[i].y, &lstPortVertsSS[i].z);
                i++;

                CurCamera.m_ScissorInfo.x1 = uint16(CLAMP(aabb.min.x * w / 100, 0, w));
                CurCamera.m_ScissorInfo.y1 = uint16(CLAMP(aabb.min.y * h / 100, 0, h));
                CurCamera.m_ScissorInfo.x2 = uint16(CLAMP(aabb.max.x * w / 100, 0, w));
                CurCamera.m_ScissorInfo.y2 = uint16(CLAMP(aabb.max.y * h / 100, 0, h));
            }

            if (GetCVars()->e_Portals == 4)
            {
                for (int i = 0; i < lstPortVertsSS.Count(); i++)
                {
                    float farrColor[4] = { float((nReqursionLevel & 1) > 0), float((nReqursionLevel & 2) > 0), float((nReqursionLevel & 4) > 0), 1};
                    ColorF c(farrColor[0], farrColor[1], farrColor[2], farrColor[3]);
                    DrawSphere(lstPortVertsSS[i], 0.002f, c);
                    GetRenderer()->DrawLabelEx(lstPortVertsSS[i], 0.1f, farrColor, false, true, "%d", i);
                }
            }

            UpdatePortalCameraPlanes(CurCamera, lstPortVertsSS.GetElements(), Upright, passInfo);

            bCanSeeThruThisArea =
                (CurCamera.m_ScissorInfo.x1 < CurCamera.m_ScissorInfo.x2) &&
                (CurCamera.m_ScissorInfo.y1 < CurCamera.m_ScissorInfo.y2);
        }

        if (m_bUseDeepness && bCanSeeThruThisArea && barrPortVertsOtherSideValid)
        {
            Vec3 vOtherSideBoxMax = SetMinBB();
            Vec3 vOtherSideBoxMin = SetMaxBB();
            for (int i = 0; i < 4; i++)
            {
                vOtherSideBoxMin.CheckMin(arrPortVertsOtherSide[i] - Vec3(0.01f, 0.01f, 0.01f));
                vOtherSideBoxMax.CheckMax(arrPortVertsOtherSide[i] + Vec3(0.01f, 0.01f, 0.01f));
            }

            bCanSeeThruThisArea = CurCamera.IsAABBVisible_E(AABB(vOtherSideBoxMin, vOtherSideBoxMax));
        }

        if (bCanSeeThruThisArea && pParent && m_lstConnections.Count() == 1)
        { // set this camera for outdoor
            if (nReqursionLevel >= 1)
            {
                if (!m_bSkyOnly)
                {
                    if (plstOutPortCameras)
                    {
                        plstOutPortCameras->Add(CurCamera);
                        plstOutPortCameras->Last().m_pPortal = this;
                    }
                    if (pbOutdoorVisible)
                    {
                        *pbOutdoorVisible = true;
                    }
                }
                else if (pbSkyVisible)
                {
                    *pbSkyVisible = true;
                }
            }

            UpdatePortalBlendInfo();
            return;
        }
    }

    // sort portals by distance
    if (!m_bThisIsPortal && m_lstConnections.Count())
    {
        for (int p = 0; p < m_lstConnections.Count(); p++)
        {
            CVisArea* pNeibVolume = m_lstConnections[p];
            pNeibVolume->m_fDistance = CurCamera.GetPosition().GetDistance((pNeibVolume->m_boxArea.min + pNeibVolume->m_boxArea.max) * 0.5f);
        }

        qsort(&m_lstConnections[0], m_lstConnections.Count(),
            sizeof(m_lstConnections[0]), CVisAreaManager__CmpDistToPortal);
    }

    if (m_bOceanVisible && pbOceanVisible)
    {
        *pbOceanVisible = true;
    }

    // recurse to connections
    for (int p = 0; p < m_lstConnections.Count(); p++)
    {
        CVisArea* pNeibVolume = m_lstConnections[p];
        if (pNeibVolume != pParent)
        {
            if (!m_bThisIsPortal)
            { // skip far portals
                float fRadius = (pNeibVolume->m_boxArea.max - pNeibVolume->m_boxArea.min).GetLength() * 0.5f * GetFloatCVar(e_ViewDistRatioPortals) / 60.f;
                if (pNeibVolume->m_fDistance * passInfo.GetZoomFactor() > fRadius * pNeibVolume->m_fViewDistRatio)
                {
                    continue;
                }

                Vec3 vPortNorm = (this == pNeibVolume->m_lstConnections[0] || pNeibVolume->m_lstConnections.Count() == 1) ?
                    pNeibVolume->m_vConnNormals[0] : pNeibVolume->m_vConnNormals[1];

                // back face check
                Vec3 vPortToCamDir = CurCamera.GetPosition() - pNeibVolume->GetAABBox()->GetCenter();
                if (vPortToCamDir.Dot(vPortNorm) < 0)
                {
                    continue;
                }
            }

            if ((bCanSeeThruThisArea || m_lstConnections.Count() == 1) && (m_bThisIsPortal || CurCamera.IsAABBVisible_F(pNeibVolume->m_boxStatics)))
            {
                pNeibVolume->PreRender(nReqursionLevel - 1, CurCamera, this, pCurPortal, pbOutdoorVisible, plstOutPortCameras, pbSkyVisible, pbOceanVisible, lstVisibleAreas, passInfo);
            }
        }
    }

    if (m_bThisIsPortal)
    {
        UpdatePortalBlendInfo();
    }
}

//! return list of visareas connected to specified visarea (can return portals and sectors)
int CVisArea::GetRealConnections(IVisArea** pAreas, int nMaxConnNum, [[maybe_unused]] bool bSkipDisabledPortals)
{
    int nOut = 0;
    for (int nArea = 0; nArea < m_lstConnections.Count(); nArea++)
    {
        if (nOut < nMaxConnNum)
        {
            pAreas[nOut] = (IVisArea*)m_lstConnections[nArea];
        }
        nOut++;
    }
    return nOut;
}

//! return list of sectors conected to specified sector or portal (returns sectors only)
// todo: change the way it returns data
int CVisArea::GetVisAreaConnections(IVisArea** pAreas, int nMaxConnNum, bool bSkipDisabledPortals)
{
    int nOut = 0;
    if (IsPortal())
    {
        /*      for(int nArea=0; nArea<m_lstConnections.Count(); nArea++)
                {
                    if(nOut<nMaxConnNum)
                        pAreas[nOut] = (IVisArea*)m_lstConnections[nArea];
                    nOut++;
                }*/
        return min(nMaxConnNum, GetRealConnections(pAreas, nMaxConnNum, bSkipDisabledPortals));
    }
    else
    {
        for (int nPort = 0; nPort < m_lstConnections.Count(); nPort++)
        {
            CVisArea* pPortal = m_lstConnections[nPort];
            assert(pPortal->IsPortal());
            for (int nArea = 0; nArea < pPortal->m_lstConnections.Count(); nArea++)
            {
                if (pPortal->m_lstConnections[nArea] != this)
                {
                    if (!bSkipDisabledPortals || pPortal->IsActive())
                    {
                        if (nOut < nMaxConnNum)
                        {
                            pAreas[nOut] = (IVisArea*)pPortal->m_lstConnections[nArea];
                        }
                        nOut++;
                        break; // take first valid connection
                    }
                }
            }
        }
    }

    return min(nMaxConnNum, nOut);
}

bool CVisArea::IsPortalValid()
{
    int nCount = m_lstConnections.Count();
    if (nCount > 2 || nCount == 0)
    {
        return false;
    }

    for (int i = 0; i < nCount; i++)
    {
        if (m_vConnNormals[i].IsEquivalent(Vec3(0, 0, 0), VEC_EPSILON))
        {
            return false;
        }
    }

    if (nCount > 1)
    {
        if (m_vConnNormals[0].Dot(m_vConnNormals[1]) > -0.99f)
        {
            return false;
        }
    }

    return true;
}

bool CVisArea::IsPortalIntersectAreaInValidWay(CVisArea* pPortal)
{
    const Vec3& v1Min = pPortal->m_boxArea.min;
    const Vec3& v1Max = pPortal->m_boxArea.max;
    const Vec3& v2Min = m_boxArea.min;
    const Vec3& v2Max = m_boxArea.max;

    if (v1Max.x > v2Min.x && v2Max.x > v1Min.x)
    {
        if (v1Max.y > v2Min.y && v2Max.y > v1Min.y)
        {
            if (v1Max.z > v2Min.z && v2Max.z > v1Min.z)
            {
                // vertical portal
                for (int v = 0; v < m_lstShapePoints.Count(); v++)
                {
                    int nIntersNum = 0;
                    bool arrIntResult[4] = { 0, 0, 0, 0 };
                    for (int p = 0; p < pPortal->m_lstShapePoints.Count() && p < 4; p++)
                    {
                        const Vec3& v0 = m_lstShapePoints[v];
                        const Vec3& v1 = m_lstShapePoints[(v + 1) % m_lstShapePoints.Count()];
                        const Vec3& p0 = pPortal->m_lstShapePoints[p];
                        const Vec3& p1 = pPortal->m_lstShapePoints[(p + 1) % pPortal->m_lstShapePoints.Count()];

                        if (Is2dLinesIntersect(v0.x, v0.y, v1.x, v1.y, p0.x, p0.y, p1.x, p1.y))
                        {
                            nIntersNum++;
                            arrIntResult[p] = true;
                        }
                    }
                    if (nIntersNum == 2 && arrIntResult[0] == arrIntResult[2] && arrIntResult[1] == arrIntResult[3])
                    {
                        return true;
                    }
                }

                // horisontal portal
                {
                    int nBottomPoints = 0, nUpPoints = 0;
                    for (int p = 0; p < pPortal->m_lstShapePoints.Count() && p < 4; p++)
                    {
                        if (IsPointInsideVisArea(pPortal->m_lstShapePoints[p]))
                        {
                            nBottomPoints++;
                        }
                    }

                    for (int p = 0; p < pPortal->m_lstShapePoints.Count() && p < 4; p++)
                    {
                        if (IsPointInsideVisArea(pPortal->m_lstShapePoints[p] + Vec3(0, 0, pPortal->m_fHeight)))
                        {
                            nUpPoints++;
                        }
                    }

                    if (nBottomPoints == 0 && nUpPoints == 4)
                    {
                        return true;
                    }

                    if (nBottomPoints == 4 && nUpPoints == 0)
                    {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}


/*
void CVisArea::SetTreeId(int nTreeId)
{
    if(m_nTreeId == nTreeId)
        return;

    m_nTreeId = nTreeId;

    for(int p=0; p<m_lstConnections.Count(); p++)
        m_lstConnections[p]->SetTreeId(nTreeId);
}
*/
bool CVisArea::IsShapeClockwise()
{
    float fClockWise =
        (m_lstShapePoints[0].x - m_lstShapePoints[1].x) * (m_lstShapePoints[2].y - m_lstShapePoints[1].y) -
        (m_lstShapePoints[0].y - m_lstShapePoints[1].y) * (m_lstShapePoints[2].x - m_lstShapePoints[1].x);

    return fClockWise > 0;
}

void CVisArea::DrawAreaBoundsIntoCBuffer([[maybe_unused]] CCullBuffer* pCBuffer)
{
    assert(!"temprary not supported");

    /*  if(m_lstShapePoints.Count()!=4)
        return;

      Vec3 arrVerts[8];
      int arrIndices[24];

      int v=0;
      int i=0;
      for(int p=0; p<4 && p<m_lstShapePoints.Count(); p++)
      {
        arrVerts[v++] = m_lstShapePoints[p];
        arrVerts[v++] = m_lstShapePoints[p] + Vec3(0,0,m_fHeight);

        arrIndices[i++] = (p*2+0)%8;
        arrIndices[i++] = (p*2+1)%8;
        arrIndices[i++] = (p*2+2)%8;
        arrIndices[i++] = (p*2+3)%8;
        arrIndices[i++] = (p*2+2)%8;
        arrIndices[i++] = (p*2+1)%8;
      }

      Matrix34 mat;
      mat.SetIdentity();
      pCBuffer->AddMesh(arrVerts,8,arrIndices,24,&mat);*/
}

void CVisArea::ClipPortalVerticesByCameraFrustum(PodArray<Vec3>* pPolygon, const CCamera& cam)
{
    Plane planes[5] = {
        *cam.GetFrustumPlane(FR_PLANE_RIGHT),
        *cam.GetFrustumPlane(FR_PLANE_LEFT),
        *cam.GetFrustumPlane(FR_PLANE_TOP),
        *cam.GetFrustumPlane(FR_PLANE_BOTTOM),
        *cam.GetFrustumPlane(FR_PLANE_NEAR)
    };

    const PodArray<Vec3>& clipped = s_tmpClipContext.Clip(*pPolygon, planes, 4);

    pPolygon->Clear();
    pPolygon->AddList(clipped);
}

void CVisArea::GetMemoryUsage(ICrySizer* pSizer)
{
    //  pSizer->AddContainer(m_lstEntities[STATIC_OBJECTS]);
    //pSizer->AddContainer(m_lstEntities[DYNAMIC_OBJECTS]);

    // TODO: include obects tree


    if (m_pObjectsTree)
    {
        SIZER_COMPONENT_NAME(pSizer, "IndoorObjectsTree");
        m_pObjectsTree->GetMemoryUsage(pSizer);
    }

    //  for(int nStatic=0; nStatic<2; nStatic++)
    //for(int i=0; i<m_lstEntities[nStatic].Count(); i++)
    //    nSize += m_lstEntities[nStatic][i].pNode->GetMemoryUsage();

    pSizer->AddObject(this, sizeof(*this));
}

void CVisArea::AddConnectedAreas(PodArray<CVisArea*>& lstAreas, int nMaxRecursion)
{
    if (lstAreas.Find(this) < 0)
    {
        lstAreas.Add(this);

        // add connected areas
        if (nMaxRecursion)
        {
            nMaxRecursion--;

            for (int p = 0; p < m_lstConnections.Count(); p++)
            {
                m_lstConnections[p]->AddConnectedAreas(lstAreas, nMaxRecursion);
            }
        }
    }
}

bool CVisArea::GetDistanceThruVisAreas(AABB vCurBoxIn, IVisArea* pTargetArea, const AABB& targetBox, int nMaxReqursion, float& fResDist)
{
    return GetDistanceThruVisAreasReq(vCurBoxIn, 0, pTargetArea, targetBox, nMaxReqursion, fResDist, NULL, s_nGetDistanceThruVisAreasCallCounter++);
}

float DistanceAABB(const AABB& aBox, const AABB& bBox)
{
    float result = 0;

    for (int i = 0; i < 3; ++i)
    {
        const float& aMin = aBox.min[i];
        const float& aMax = aBox.max[i];
        const float& bMin = bBox.min[i];
        const float& bMax = bBox.max[i];

        if (aMin > bMax)
        {
            const float delta = bMax - aMin;

            result += delta * delta;
        }
        else if (bMin > aMax)
        {
            const float delta = aMax - bMin;

            result += delta * delta;
        }
        // else the projection intervals overlap.
    }

    return sqrt(result);
}

bool CVisArea::GetDistanceThruVisAreasReq(AABB vCurBoxIn, float fCurDistIn, IVisArea* pTargetArea, const AABB& targetBox, int nMaxReqursion, float& fResDist, CVisArea* pPrevArea, int nCallID)
{
    if (pTargetArea == this || (pTargetArea == NULL && IsConnectedToOutdoor()))
    { // target area is found
        fResDist = min(fResDist, fCurDistIn + DistanceAABB(vCurBoxIn, targetBox));
        return true;
    }

    // if we already visited this area and last time input distance was smaller - makes no sence to continue
    if (nCallID == m_nGetDistanceThruVisAreasLastCallID)
    {
        if (fCurDistIn >= m_fGetDistanceThruVisAreasMinDistance)
        {
            return false;
        }
    }

    m_nGetDistanceThruVisAreasLastCallID = nCallID;
    m_fGetDistanceThruVisAreasMinDistance = fCurDistIn;

    fResDist = FLT_MAX;

    bool bFound = false;

    if (nMaxReqursion > 1)
    {
        for (int p = 0; p < m_lstConnections.Count(); p++)
        {
            if ((m_lstConnections[p] != pPrevArea) && m_lstConnections[p]->IsActive())
            {
                AABB vCurBox = vCurBoxIn;
                float fCurDist = fCurDistIn;
                float dist = FLT_MAX;

                if (IsPortal())
                {
                    vCurBox = vCurBoxIn;
                    fCurDist = fCurDistIn;
                }
                else
                {
                    vCurBox = *m_lstConnections[p]->GetAABBox();
                    fCurDist = fCurDistIn + DistanceAABB(vCurBox, vCurBoxIn);
                }

                if (m_lstConnections[p]->GetDistanceThruVisAreasReq(vCurBox, fCurDist, pTargetArea, targetBox, nMaxReqursion - 1, dist, this, nCallID))
                {
                    bFound     = true;
                    fResDist = min(fResDist, dist);
                }
            }
        }
    }

    return bFound;
}

void CVisArea::OffsetPosition(const Vec3& delta)
{
    m_boxArea.Move(delta);
    m_boxStatics.Move(delta);
    for (int i = 0; i < m_lstShapePoints.Count(); i++)
    {
        m_lstShapePoints[i] += delta;
    }
    if (m_pObjectsTree)
    {
        m_pObjectsTree->OffsetObjects(delta);
    }
}


