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

#include "EditorDefs.h"

#if defined(AZ_PLATFORM_WINDOWS)
#include <InitGuid.h>
#endif

#include "VertexSnappingModeTool.h"

// Editor
#include "Settings.h"
#include "Viewport.h"
#include "SurfaceInfoPicker.h"
#include "Material/Material.h"
#include "Util/KDTree.h"


// {3e008046-9269-41d7-82e2-07ffd7254c10}
DEFINE_GUID(VERTEXSNAPPING_MODE_GUID, 0x3e008046, 0x9269, 0x41d7, 0x82, 0xe2, 0x07, 0xff, 0xd7, 0x25, 0x4c, 0x10);

bool FindNearestVertex(CBaseObject* pObject, CKDTree* pTree, const Vec3& vWorldRaySrc, const Vec3& vWorldRayDir, Vec3& outPos, Vec3& vOutHitPosOnCube)
{
    Matrix34 worldInvTM = pObject->GetWorldTM().GetInverted();
    Vec3 vRaySrc = worldInvTM.TransformPoint(vWorldRaySrc);
    Vec3 vRayDir = worldInvTM.TransformVector(vWorldRayDir);
    Vec3 vLocalCameraPos = worldInvTM.TransformPoint(gEnv->pRenderer->GetCamera().GetPosition());
    Vec3 vPos;
    Vec3 vHitPosOnCube;

    if (pTree)
    {
        if (pTree->FindNearestVertex(vRaySrc, vRayDir, gSettings.vertexSnappingSettings.vertexCubeSize, vLocalCameraPos, vPos, vHitPosOnCube))
        {
            outPos = pObject->GetWorldTM().TransformPoint(vPos);
            vOutHitPosOnCube = pObject->GetWorldTM().TransformPoint(vHitPosOnCube);
            return true;
        }
    }
    else
    {
        // for objects without verts, the pivot is the nearest vertex
        // return true if the ray hits the bounding box
        outPos = pObject->GetWorldPos();

        AABB bbox;
        pObject->GetBoundBox(bbox);
        if (bbox.IsContainPoint(vWorldRaySrc))
        {
            // if ray starts inside bounding box, reject cases where pivot is behind the ray
            float hitDistAlongRay = vWorldRayDir.Dot(outPos - vWorldRaySrc);
            if (hitDistAlongRay >= 0.f)
            {
                vHitPosOnCube = vWorldRaySrc + (vWorldRayDir * hitDistAlongRay);
                return true;
            }
        }
        else if (Intersect::Ray_AABB(vWorldRaySrc, vWorldRayDir, bbox, vOutHitPosOnCube))
        {
            return true;
        }
    }

    return false;
}

CVertexSnappingModeTool::CVertexSnappingModeTool()
{
    m_modeStatus = eVSS_SelectFirstVertex;
    m_bHit = false;
}

CVertexSnappingModeTool::~CVertexSnappingModeTool()
{
    std::map<CBaseObjectPtr, CKDTree*>::iterator ii = m_ObjectKdTreeMap.begin();
    for (; ii != m_ObjectKdTreeMap.end(); ++ii)
    {
        delete ii->second;
    }
}

const GUID& CVertexSnappingModeTool::GetClassID()
{
    return VERTEXSNAPPING_MODE_GUID;
}

void CVertexSnappingModeTool::RegisterTool(CRegistrationContext& rc)
{
    rc.pClassFactory->RegisterClass(new CQtViewClass<CVertexSnappingModeTool>("EditTool.VertexSnappingMode", "Select", ESYSTEM_CLASS_EDITTOOL));
}

bool CVertexSnappingModeTool::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int flags)
{
    CBaseObjectPtr pExcludedObject = NULL;
    if (m_modeStatus == eVSS_MoveSelectVertexToAnotherVertex)
    {
        pExcludedObject = m_SelectionInfo.m_pObject;
    }

    m_bHit = HitTest(view, point, pExcludedObject, m_vHitVertex, m_pHitObject, m_Objects);

    if (event == eMouseLDown && m_bHit && m_pHitObject && m_modeStatus == eVSS_SelectFirstVertex)
    {
        m_modeStatus = eVSS_MoveSelectVertexToAnotherVertex;
        m_SelectionInfo.m_pObject = m_pHitObject;
        m_SelectionInfo.m_vPos = m_vHitVertex;

        GetIEditor()->BeginUndo();
        m_pHitObject->StoreUndo("Vertex Snapping", true);

        view->SetCapture();
    }

    if (m_modeStatus == eVSS_MoveSelectVertexToAnotherVertex)
    {
        if (event == eMouseLUp)
        {
            m_modeStatus = eVSS_SelectFirstVertex;

            GetIEditor()->AcceptUndo("Vertex Snapping");
            view->ReleaseMouse();
        }
        else if ((flags & MK_LBUTTON) && event == eMouseMove)
        {
            Vec3 vOffset = m_SelectionInfo.m_pObject->GetWorldPos() - m_SelectionInfo.m_vPos;
            m_SelectionInfo.m_pObject->SetWorldPos(m_vHitVertex + vOffset);
            m_SelectionInfo.m_vPos = m_SelectionInfo.m_pObject->GetWorldPos() - vOffset;
        }
    }

    return true;
}

bool CVertexSnappingModeTool::HitTest(CViewport* view, const QPoint& point, CBaseObject* pExcludedObj, Vec3& outHitPos, CBaseObjectPtr& pOutHitObject, std::vector<CBaseObjectPtr>& outObjects)
{
    if (gSettings.vertexSnappingSettings.bRenderPenetratedBoundBox)
    {
        m_DebugBoxes.clear();
    }

    pOutHitObject = NULL;
    outObjects.clear();

    //
    // Collect valid objects that mouse is over
    //

    CSurfaceInfoPicker picker;
    CSurfaceInfoPicker::CExcludedObjects excludedObjects;
    if (pExcludedObj)
    {
        excludedObjects.Add(pExcludedObj);
    }

    int nPickFlag = CSurfaceInfoPicker::ePOG_Entity;

    std::vector<CBaseObjectPtr> penetratedObjects;
    if (!picker.PickByAABB(point, nPickFlag, view, &excludedObjects, &penetratedObjects))
    {
        return false;
    }

    for (int i = 0, iCount(penetratedObjects.size()); i < iCount; ++i)
    {
        CMaterial* pMaterial = penetratedObjects[i]->GetMaterial();
        if (pMaterial)
        {
            QString matName = pMaterial->GetName();
            if (!QString::compare(matName, "Objects/sky/forest_sky_dome", Qt::CaseInsensitive))
            {
                continue;
            }
        }
        outObjects.push_back(penetratedObjects[i]);
    }

    //
    // Find the best vertex.
    //

    Vec3 vWorldRaySrc, vWorldRayDir;
    view->ViewToWorldRay(point, vWorldRaySrc, vWorldRayDir);

    std::vector<CBaseObjectPtr>::iterator ii = outObjects.begin();
    float fNearestDist = 3e10f;
    Vec3 vNearestPos;
    CBaseObjectPtr pNearestObject = NULL;
    for (ii = outObjects.begin(); ii != outObjects.end(); ++ii)
    {
        if (gSettings.vertexSnappingSettings.bRenderPenetratedBoundBox)
        {
            // add to debug boxes: the penetrated nodes of each object's kd-tree
            if (auto pTree = GetKDTree(*ii))
            {
                Matrix34 invWorldTM = (*ii)->GetWorldTM().GetInverted();
                int nIndex = m_DebugBoxes.size();

                Vec3 vLocalRaySrc = invWorldTM.TransformPoint(vWorldRaySrc);
                Vec3 vLocalRayDir = invWorldTM.TransformVector(vWorldRayDir);
                pTree->GetPenetratedBoxes(vLocalRaySrc, vLocalRayDir, m_DebugBoxes);
                for (int i = nIndex; i < m_DebugBoxes.size(); ++i)
                {
                    m_DebugBoxes[i].SetTransformedAABB((*ii)->GetWorldTM(), m_DebugBoxes[i]);
                }
            }
        }

        // find the nearest vertex on this object
        Vec3 vPos, vHitPosOnCube;
        if (FindNearestVertex(*ii, GetKDTree(*ii), vWorldRaySrc, vWorldRayDir, vPos, vHitPosOnCube))
        {
            // is this the best so far?
            float fDistance = vHitPosOnCube.GetDistance(vWorldRaySrc);
            if (fDistance < fNearestDist)
            {
                fNearestDist = fDistance;
                vNearestPos = vPos;
                pNearestObject = *ii;
            }
        }
    }

    if (fNearestDist < 3e10f)
    {
        outHitPos = vNearestPos;
        pOutHitObject = pNearestObject;
    }

    // if the mouse is over the object's pivot, use that instead of a vertex
    if (pOutHitObject)
    {
        Vec3 vPivotPos = pOutHitObject->GetWorldPos();
        Vec3 vPivotBox = GetCubeSize(view, pOutHitObject->GetWorldPos());
        AABB pivotAABB(vPivotPos - vPivotBox, vPivotPos + vPivotBox);
        Vec3 vPosOnPivotCube;
        if (Intersect::Ray_AABB(vWorldRaySrc, vWorldRayDir, pivotAABB, vPosOnPivotCube))
        {
            outHitPos = vPivotPos;
            return true;
        }
    }

    return pOutHitObject && pOutHitObject == pNearestObject;
}

Vec3 CVertexSnappingModeTool::GetCubeSize(IDisplayViewport* pView, const Vec3& pos) const
{
    if (!pView)
    {
        return Vec3(0, 0, 0);
    }
    float fScreenFactor = pView->GetScreenScaleFactor(pos);
    return gSettings.vertexSnappingSettings.vertexCubeSize * Vec3(fScreenFactor, fScreenFactor, fScreenFactor);
}

void CVertexSnappingModeTool::Display(struct DisplayContext& dc)
{
    const ColorB SnappedColor(0xFF00FF00);
    const ColorB PivotColor(0xFF2020FF);
    const ColorB VertexColor(0xFFFFAAAA);

    // draw all objects under mouse
    dc.SetColor(VertexColor);
    for (int i = 0, iCount(m_Objects.size()); i < iCount; ++i)
    {
        AABB worldAABB;
        m_Objects[i]->GetBoundBox(worldAABB);
        if (!dc.view->IsBoundsVisible(worldAABB))
        {
            continue;
        }

        if (auto pStatObj = m_Objects[i]->GetIStatObj())
        {
            DrawVertexCubes(dc, m_Objects[i]->GetWorldTM(), pStatObj);
        }
        else
        {
            dc.DrawWireBox(worldAABB.min, worldAABB.max);
        }
    }

    // draw object being moved
    if (m_modeStatus == eVSS_MoveSelectVertexToAnotherVertex && m_SelectionInfo.m_pObject)
    {
        dc.SetColor(QColor(0xaa, 0xaa, 0xaa));
        if (auto pStatObj = m_SelectionInfo.m_pObject->GetIStatObj())
        {
            DrawVertexCubes(dc, m_SelectionInfo.m_pObject->GetWorldTM(), pStatObj);
        }
        else
        {
            AABB bounds;
            m_SelectionInfo.m_pObject->GetBoundBox(bounds);
            dc.DrawWireBox(bounds.min, bounds.max);
        }
    }

    // draw pivot of hit object
    if (m_pHitObject && (!m_bHit || m_bHit && !m_pHitObject->GetWorldPos().IsEquivalent(m_vHitVertex, 0.001f)))
    {
        dc.SetColor(PivotColor);
        dc.DepthTestOff();

        Vec3 vBoxSize = GetCubeSize(dc.view, m_pHitObject->GetWorldPos()) * 1.2f;
        AABB vertexBox(m_pHitObject->GetWorldPos() - vBoxSize, m_pHitObject->GetWorldPos() + vBoxSize);
        dc.DrawBall((vertexBox.min + vertexBox.max) * 0.5f, (vertexBox.max.x - vertexBox.min.x) * 0.5f);

        dc.DepthTestOn();
    }

    // draw the vertex (or pivot) that's being hit
    if (m_bHit)
    {
        dc.DepthTestOff();
        dc.SetColor(SnappedColor);
        Vec3 vBoxSize = GetCubeSize(dc.view, m_vHitVertex);
        if (m_vHitVertex.IsEquivalent(m_pHitObject->GetWorldPos(), 0.001f))
        {
            dc.DrawBall(m_vHitVertex, vBoxSize.x * 1.2f);
        }
        else
        {
            dc.DrawSolidBox(m_vHitVertex - vBoxSize, m_vHitVertex + vBoxSize);
        }
        dc.DepthTestOn();
    }

    // draw wireframe of hit object
    if (m_pHitObject && m_pHitObject->GetIStatObj())
    {
        SGeometryDebugDrawInfo dd;
        dd.tm = m_pHitObject->GetWorldTM();
        dd.color = ColorB(250, 0, 250, 30);
        dd.lineColor = ColorB(255, 255, 0, 160);
        dd.bExtrude = true;
        m_pHitObject->GetIStatObj()->DebugDraw(dd);
    }

    // draw debug boxes
    if (gSettings.vertexSnappingSettings.bRenderPenetratedBoundBox)
    {
        ColorB boxColor(40, 40, 40);
        for (int i = 0, iCount(m_DebugBoxes.size()); i < iCount; ++i)
        {
            dc.SetColor(boxColor);
            boxColor += ColorB(25, 25, 25);
            dc.DrawWireBox(m_DebugBoxes[i].min, m_DebugBoxes[i].max);
        }
    }
}

void CVertexSnappingModeTool::DrawVertexCubes(DisplayContext& dc, const Matrix34& tm, IStatObj* pStatObj)
{
    if (!pStatObj)
    {
        return;
    }

    IIndexedMesh* pIndexedMesh = pStatObj->GetIndexedMesh();
    if (pIndexedMesh)
    {
        IIndexedMesh::SMeshDescription md;
        pIndexedMesh->GetMeshDescription(md);
        for (int k = 0; k < md.m_nVertCount; ++k)
        {
            Vec3 vPos(0, 0, 0);
            if (md.m_pVerts)
            {
                vPos = md.m_pVerts[k];
            }
            else if (md.m_pVertsF16)
            {
                vPos = md.m_pVertsF16[k].ToVec3();
            }
            else
            {
                continue;
            }
            vPos = tm.TransformPoint(vPos);
            Vec3 vBoxSize = GetCubeSize(dc.view, vPos);
            if (!m_bHit || !m_vHitVertex.IsEquivalent(vPos, 0.001f))
            {
                dc.DrawSolidBox(vPos - vBoxSize, vPos + vBoxSize);
            }
        }
    }

    for (int i = 0, iSubStatObjNum(pStatObj->GetSubObjectCount()); i < iSubStatObjNum; ++i)
    {
        IStatObj::SSubObject* pSubObj = pStatObj->GetSubObject(i);
        if (pSubObj)
        {
            DrawVertexCubes(dc, tm * pSubObj->localTM, pSubObj->pStatObj);
        }
    }
}

CKDTree* CVertexSnappingModeTool::GetKDTree(CBaseObject* pObject)
{
    auto existingTree = m_ObjectKdTreeMap.find(pObject);
    if (existingTree != m_ObjectKdTreeMap.end())
    {
        return existingTree->second;
    }

    // Don't build a kd-tree for objects without verts
    CKDTree* pTree = nullptr;
    if (auto pStatObj = pObject->GetIStatObj())
    {
        pTree = new CKDTree();
        pTree->Build(pObject->GetIStatObj());
    }

    m_ObjectKdTreeMap[pObject] = pTree;
    return pTree;
}

#include <EditMode/moc_VertexSnappingModeTool.cpp>
