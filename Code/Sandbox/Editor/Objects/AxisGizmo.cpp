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

#include "AxisGizmo.h"

// Editor
#include "Viewport.h"
#include "GizmoManager.h"
#include "Grid.h"
#include "ViewManager.h"
#include "Settings.h"
#include "RenderHelpers/AxisHelper.h"
#include "RenderHelpers/AxisHelperExtended.h"
#include "IObjectManager.h"
#include "EditTool.h"

//////////////////////////////////////////////////////////////////////////
// CAxisGizmo implementation.
//////////////////////////////////////////////////////////////////////////
int CAxisGizmo::m_axisGizmoCount = 0;

//////////////////////////////////////////////////////////////////////////
CAxisGizmo::CAxisGizmo(CBaseObject* object)
{
    assert(object != 0);
    m_object = object;
    m_pAxisHelper.reset(new CAxisHelper);
    m_pAxisHelperExtended.reset(new CAxisHelperExtended);

    // Set selectable flag.
    SetFlags(EGIZMO_SELECTABLE | EGIZMO_TRANSFORM_MANIPULATOR);

    m_axisGizmoCount++;
    m_object->AddEventListener(this);

    m_localTM.SetIdentity();
    m_parentTM.SetIdentity();
    m_matrix.SetIdentity();

    m_bDragging = false;
    m_bAlwaysUseLocal = false;
    m_coordSysBackUp = COORDS_VIEW;
}

//////////////////////////////////////////////////////////////////////////
CAxisGizmo::CAxisGizmo()
{
    // Set selectable flag.
    SetFlags(EGIZMO_SELECTABLE);
    m_axisGizmoCount++;
    m_pAxisHelper.reset(new CAxisHelper);
    m_pAxisHelperExtended.reset(new CAxisHelperExtended);
    m_bDragging = false;
    m_bAlwaysUseLocal = false;
    m_coordSysBackUp = COORDS_VIEW;
}

//////////////////////////////////////////////////////////////////////////
CAxisGizmo::~CAxisGizmo()
{
    if (m_object)
    {
        m_object->RemoveEventListener(this);
    }
    m_axisGizmoCount--;
}

//////////////////////////////////////////////////////////////////////////
void CAxisGizmo::OnObjectEvent([[maybe_unused]] CBaseObject* object, int event)
{
    if (event == CBaseObject::ON_DELETE || event == CBaseObject::ON_UNSELECT)
    {
        // This gizmo must be deleted as well.
        GetIEditor()->GetObjectManager()->GetGizmoManager()->RemoveGizmo(this);
        return;
    }
}

//////////////////////////////////////////////////////////////////////////
void CAxisGizmo::Display(DisplayContext& dc)
{
    if (m_object)
    {
        const bool visible =
                !m_object->IsHidden()
            &&  !m_object->IsFrozen()
            &&  m_object->IsSelected();

        if (!visible)
        {
            // This gizmo must be deleted.
            DeleteThis();
            return;
        }

        if (m_object->IsSkipSelectionHelper())
        {
            return;
        }
    }

    DrawAxis(dc);
}

//////////////////////////////////////////////////////////////////////////
void CAxisGizmo::SetWorldBounds(const AABB& bbox)
{
    m_bbox = bbox;
}

//////////////////////////////////////////////////////////////////////////
void CAxisGizmo::GetWorldBounds(AABB& bbox)
{
    if (m_object)
    {
        m_object->GetBoundBox(bbox);
    }
    else
    {
        bbox.min = Vec3(-1000000, -1000000, -1000000);
        bbox.max = Vec3(1000000, 1000000, 1000000);
    }
}

//////////////////////////////////////////////////////////////////////////
void CAxisGizmo::DrawAxis(DisplayContext& dc)
{
    m_pAxisHelper->SetHighlightAxis(m_highlightAxis);
    // Only enable axis planes when editor is in Move mode.
    int nEditMode = GetIEditor()->GetEditMode();
    int nModeFlags = 0;
    switch (nEditMode)
    {
    case eEditModeMove:
        nModeFlags |= CAxisHelper::MOVE_MODE;
        break;
    case eEditModeRotate:
        nModeFlags |= CAxisHelper::ROTATE_MODE;
        nModeFlags &= ~(CAxisHelper::ROTATE_CIRCLE_MODE);
        break;
    case eEditModeRotateCircle:
        nModeFlags |= CAxisHelper::ROTATE_CIRCLE_MODE;
        nModeFlags &= ~(CAxisHelper::ROTATE_MODE);
        break;
    case eEditModeScale:
        nModeFlags |= CAxisHelper::SCALE_MODE;
        break;
    case eEditModeSelect:
        nModeFlags |= CAxisHelper::SELECT_MODE;
        break;
    case eEditModeSelectArea:
        nModeFlags |= CAxisHelper::SELECT_MODE;
        break;
    }

    //nModeFlags |= CAxisHelper::MOVE_MODE | CAxisHelper::ROTATE_MODE | CAxisHelper::SCALE_MODE;

    m_pAxisHelper->SetMode(nModeFlags);

    Matrix34 tm = GetTransformation(m_bAlwaysUseLocal ? COORDS_LOCAL : GetIEditor()->GetReferenceCoordSys(), dc.view);
    m_pAxisHelper->DrawAxis(tm, GetIEditor()->GetGlobalGizmoParameters(), dc);
    if (m_object && m_object->CheckFlags(OBJFLAG_IS_PARTICLE))
    {
        AABB objectBox;
        m_object->GetBoundBox(objectBox);

        m_pAxisHelper->DrawDome(tm, GetIEditor()->GetGlobalGizmoParameters(), dc, objectBox);
    }
    //////////////////////////////////////////////////////////////////////////
    // Draw extended infinite-axis gizmo
    //////////////////////////////////////////////////////////////////////////
    if (!(dc.flags & DISPLAY_2D) &&
        (nModeFlags == CAxisHelper::MOVE_MODE ||
         nModeFlags == CAxisHelper::ROTATE_MODE))
    {
        bool bClickedShift = CheckVirtualKey(Qt::Key_Shift);
        if (bClickedShift && (m_axisGizmoCount == 1 || m_highlightAxis || (m_axisGizmoCount == 2 && m_object && m_object->IsSkipSelectionHelper())))
        {
            bool bClickedAlt = CheckVirtualKey(Qt::Key_Menu);
            bool bUsePhysicalProxy = !bClickedAlt;
            m_pAxisHelperExtended->DrawAxes(dc, tm, bUsePhysicalProxy);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
const Matrix34& CAxisGizmo::GetMatrix() const
{
    if (m_object)
    {
        m_matrix.SetTranslation(m_object->GetWorldTM().GetTranslation());
    }
    return m_matrix;
}

//////////////////////////////////////////////////////////////////////////
bool CAxisGizmo::HitTest(HitContext& hc)
{
    Matrix34 tm = GetTransformation(m_bAlwaysUseLocal ? COORDS_LOCAL : GetIEditor()->GetReferenceCoordSys(), hc.view);

    CAxisHelper axis;
    bool bRes = m_pAxisHelper->HitTest(tm, GetIEditor()->GetGlobalGizmoParameters(), hc);
    if (bRes)
    {
        hc.object = m_object;
    }

    m_highlightAxis = m_pAxisHelper->GetHighlightAxis();

    return bRes;
}

//////////////////////////////////////////////////////////////////////////
bool CAxisGizmo::HitTestManipulator(HitContext& hc)
{
    return HitTest(hc);
}

//////////////////////////////////////////////////////////////////////////
void CAxisGizmo::SetTransformation(RefCoordSys coordSys, const Matrix34& tm)
{
    switch (coordSys)
    {
    case COORDS_WORLD:
        SetMatrix(tm);
        break;
    case COORDS_LOCAL:
        m_localTM = tm;
        {
            Matrix34 wtm;
            wtm.SetIdentity();
            wtm.SetTranslation(m_localTM.GetTranslation());
            SetMatrix(wtm);
            m_userTM = tm;
        }
        m_parentTM = m_localTM;
        break;
    case COORDS_PARENT:
        m_parentTM = tm;
        break;
    case COORDS_USERDEFINED:
        m_userTM = tm;
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
Matrix34 CAxisGizmo::GetTransformation(RefCoordSys coordSys, IDisplayViewport* view /*=nullptr*/) const
{
    if (m_object)
    {
        switch (coordSys)
        {
        case COORDS_VIEW:
            if (view)
            {
                Matrix34 tm = view->GetViewTM();
                tm.SetTranslation(m_object->GetWorldTM().GetTranslation());
                return tm;
            }
            return GetMatrix();
            break;
        case COORDS_LOCAL:
            return m_object->GetWorldTM();
            break;
        case COORDS_PARENT:
            //return m_parentTM;
            if (m_object->GetParent())
            {
                Matrix34 parentTM = m_object->GetParent()->GetWorldTM();
                parentTM.SetTranslation(m_object->GetWorldTM().GetTranslation());
                return parentTM;
            }
            else
            {
                return GetMatrix();
            }
            break;
        case COORDS_WORLD:
            return GetMatrix();
            break;
        case COORDS_USERDEFINED:
        {
            Matrix34 userTM = GetIEditor()->GetViewManager()->GetGrid()->GetMatrix();
            userTM.SetTranslation(m_object->GetWorldTM().GetTranslation());
            return userTM;
        }
        break;
        }
    }
    else
    {
        switch (coordSys)
        {
        case COORDS_VIEW:
            return GetMatrix();
            break;
        case COORDS_LOCAL:
            return m_localTM;
            break;
        case COORDS_PARENT:
            return m_parentTM;
            break;
        case COORDS_WORLD:
            return GetMatrix();
            break;
        case COORDS_USERDEFINED:
            return m_userTM;
            break;
        }
    }
    return GetMatrix();
}

//////////////////////////////////////////////////////////////////////////
bool CAxisGizmo::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, int nFlags)
{
    AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::Editor);

    if (event == eMouseLDown)
    {
        HitContext hc;
        hc.view = view;
        hc.b2DViewport = view->GetType() != ET_ViewportCamera;
        hc.point2d = point;
        view->ViewToWorldRay(point, hc.raySrc, hc.rayDir);
        if (HitTest(hc))
        {
            if (event != eMouseLDown)
            {
                return false;
            }

            // On Left mouse down.

            // Hit axis gizmo.
            GetIEditor()->SetAxisConstraints((AxisConstrains)hc.axis);
            view->SetAxisConstrain(hc.axis);

            if (m_bAlwaysUseLocal)
            {
                m_coordSysBackUp = GetIEditor()->GetReferenceCoordSys();
                GetIEditor()->SetReferenceCoordSys(COORDS_LOCAL);
            }

            view->SetConstructionMatrix(COORDS_LOCAL, GetTransformation(COORDS_LOCAL));
            view->SetConstructionMatrix(COORDS_PARENT, GetTransformation(COORDS_PARENT));
            view->SetConstructionMatrix(COORDS_USERDEFINED, GetTransformation(COORDS_USERDEFINED));

            view->BeginUndo();
            view->CaptureMouse();
            m_bDragging = true;
            m_cMouseDownPos = point;
            m_initPos = GetTransformation(COORDS_WORLD).GetTranslation();

            switch (hc.manipulatorMode)
            {
            case 1:
                view->SetCurrentCursor(STD_CURSOR_MOVE);
                GetIEditor()->SetEditMode(eEditModeMove);
                break;
            case 2:
                view->SetCurrentCursor(STD_CURSOR_ROTATE);
                GetIEditor()->SetEditMode(eEditModeRotate);
                break;
            case 3:
                view->SetCurrentCursor(STD_CURSOR_SCALE);
                GetIEditor()->SetEditMode(eEditModeScale);
                break;
            }

            CEditTool* pEditTool = view->GetEditTool();
            if (pEditTool)
            {
                pEditTool->OnManipulatorMouseEvent(view, this, event, point, nFlags);
            }

            return true;
        }
    }
    else if (event == eMouseMove)
    {
        if (m_bDragging)
        {
            bool bCallBack = true;
            Vec3 vDragValue(0, 0, 0);
            // Dragging transform manipulator.
            switch (GetIEditor()->GetEditMode())
            {
            case eEditModeMove:
            {
                view->SetCurrentCursor(STD_CURSOR_MOVE);

                if (view->GetAxisConstrain() == AXIS_TERRAIN)
                {
                    if (nFlags & MK_CONTROL)
                    {
                        bool bCollideWithTerrain;
                        Vec3 posOnTerrain = view->ViewToWorld(point, &bCollideWithTerrain, true);
                        if (!bCollideWithTerrain)
                        {
                            return true;
                        }
                        vDragValue = posOnTerrain - m_initPos;
                    }
                    else
                    {
                        Vec3 p1 = view->SnapToGrid(view->ViewToWorld(m_cMouseDownPos));
                        Vec3 p2 = view->SnapToGrid(view->ViewToWorld(point));
                        vDragValue = p2 - p1;
                        vDragValue.z = 0;
                    }
                }
                else
                {
                    Vec3 p1 = view->MapViewToCP(m_cMouseDownPos);
                    Vec3 p2 = view->MapViewToCP(point);
                    if (p1.IsZero() || p2.IsZero())
                    {
                        return true;
                    }
                    vDragValue = view->GetCPVector(p1, p2);
                }
            }
            break;
            case eEditModeRotate:
            {
                view->SetCurrentCursor(STD_CURSOR_ROTATE);

                Ang3 ang(0, 0, 0);
                float ax = (point.x() - m_cMouseDownPos.x());
                float ay = (point.y() - m_cMouseDownPos.y());
                switch (view->GetAxisConstrain())
                {
                case AXIS_X:
                    ang.x = ay;
                    break;
                case AXIS_Y:
                    ang.y = ay;
                    break;
                case AXIS_Z:
                    ang.z = ay;
                    break;
                case AXIS_XY:
                    ang(ax, ay, 0);
                    break;
                case AXIS_XZ:
                    ang(ax, 0, ay);
                    break;
                case AXIS_YZ:
                    ang(0, ay, ax);
                    break;
                case AXIS_TERRAIN:
                    ang(ax, ay, 0);
                    break;
                }
                ;
                ang = gSettings.pGrid->SnapAngle(ang);
                vDragValue = Vec3(DEG2RAD(ang));
            }
            break;
            case eEditModeScale:
            {
                Vec3 scl(0, 0, 0);
                float ay = 1.0f - 0.01f * (point.y() - m_cMouseDownPos.y());
                if (ay < 0.01f)
                {
                    ay = 0.01f;
                }
                scl(ay, ay, ay);
                switch (view->GetAxisConstrain())
                {
                case AXIS_X:
                    scl(ay, 1, 1);
                    break;
                case AXIS_Y:
                    scl(1, ay, 1);
                    break;
                case AXIS_Z:
                    scl(1, 1, ay);
                    break;
                case AXIS_XY:
                    scl(ay, ay, ay);
                    break;
                case AXIS_XZ:
                    scl(ay, ay, ay);
                    break;
                case AXIS_YZ:
                    scl(ay, ay, ay);
                    break;
                case AXIS_XYZ:
                    scl(ay, ay, ay);
                    break;
                case AXIS_TERRAIN:
                    scl(ay, ay, ay);
                    break;
                }
                ;
                view->SetCurrentCursor(STD_CURSOR_SCALE);
                vDragValue = scl;
            }
            break;
            case eEditModeRotateCircle:
            {
                Matrix34 tm = GetTransformation(m_bAlwaysUseLocal ? COORDS_LOCAL : GetIEditor()->GetReferenceCoordSys());
                Vec3 v0, v1;
                Vec3 vHitNormal;
                if (m_pAxisHelper->HitTestForRotationCircle(tm, view, m_cMouseDownPos, 0.05f, &v0, &vHitNormal) && m_pAxisHelper->HitTestForRotationCircle(tm, view, point, 2.0f, &v1, &vHitNormal))
                {
                    Vec3 vDir0 = (v0 - tm.GetTranslation()).GetNormalized();
                    Vec3 vDir1 = (v1 - tm.GetTranslation()).GetNormalized();

                    Vec3 vCurlDir = vDir0.Cross(vDir1).GetNormalized();
                    if (vHitNormal.Dot(vCurlDir) > 0)
                    {
                        vDragValue = Vec3(std::acos(vDir0.Dot(vDir1)), 0, 0);
                    }
                    else
                    {
                        vDragValue = Vec3(-std::acos(vDir0.Dot(vDir1)), 0, 0);
                    }
                }
                else
                {
                    bCallBack = false;
                }
            }
            break;
            }

            CEditTool* pEditTool = view->GetEditTool();
            if (pEditTool && bCallBack)
            {
                pEditTool->OnManipulatorDrag(view, this, m_cMouseDownPos, point, vDragValue);
            }

            return true;
        }
        else
        {
            // Hit test current transform manipulator, to highlight when mouse over.
            HitContext hc;
            hc.view = view;
            hc.b2DViewport = view->GetType() != ET_ViewportCamera;
            hc.point2d = point;
            view->ViewToWorldRay(point, hc.raySrc, hc.rayDir);
            bool bHit = false;
            if (HitTest(hc))
            {
                switch (hc.manipulatorMode)
                {
                case 1:
                    view->SetCurrentCursor(STD_CURSOR_MOVE);
                    break;
                case 2:
                    view->SetCurrentCursor(STD_CURSOR_ROTATE);
                    break;
                case 3:
                    view->SetCurrentCursor(STD_CURSOR_SCALE);
                    break;
                }
                bHit = true;
            }

            CEditTool* pEditTool = view->GetEditTool();
            if (pEditTool)
            {
                pEditTool->OnManipulatorMouseEvent(view, this, event, point, nFlags, bHit);
            }
        }
    }
    else if (event == eMouseLUp)
    {
        if (m_bDragging)
        {
            view->AcceptUndo("Manipulator Drag");
            view->ReleaseMouse();
            m_bDragging = false;

            if (m_bAlwaysUseLocal)
            {
                GetIEditor()->SetReferenceCoordSys(m_coordSysBackUp);
            }

            CEditTool* pEditTool = view->GetEditTool();
            if (pEditTool)
            {
                pEditTool->OnManipulatorMouseEvent(view, this, event, point, nFlags);
            }
        }
    }

    return false;
}
