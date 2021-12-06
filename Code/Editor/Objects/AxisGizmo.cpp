/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "AxisGizmo.h"

// Editor
#include "Viewport.h"
#include "GizmoManager.h"
#include "ViewManager.h"
#include "Settings.h"
#include "RenderHelpers/AxisHelper.h"
#include "IObjectManager.h"

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

    Matrix34 tm = GetTransformation(m_bAlwaysUseLocal ? COORDS_LOCAL : GetIEditor()->GetReferenceCoordSys(), dc.view);
    m_pAxisHelper->DrawAxis(tm, GetIEditor()->GetGlobalGizmoParameters(), dc);
    if (m_object && m_object->CheckFlags(OBJFLAG_IS_PARTICLE))
    {
        AABB objectBox;
        m_object->GetBoundBox(objectBox);

        m_pAxisHelper->DrawDome(tm, GetIEditor()->GetGlobalGizmoParameters(), dc, objectBox);
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
            Matrix34 userTM;
            userTM.SetIdentity();
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
bool CAxisGizmo::MouseCallback(CViewport* view, EMouseEvent event, QPoint& point, [[maybe_unused]] int nFlags)
{
    AZ_PROFILE_FUNCTION(Editor);

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

            return true;
        }
    }
    else if (event == eMouseMove)
    {
        if (m_bDragging)
        {
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
        }
    }

    return false;
}
