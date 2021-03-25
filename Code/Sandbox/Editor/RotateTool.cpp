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
#include "EditorDefs.h"

#include "RotateTool.h"

// AzToolsFramework
#include <AzToolsFramework/Entity/EditorEntityTransformBus.h>

// Editor
#include "Objects/SelectionGroup.h"
#include "NullEditTool.h"
#include "Viewport.h"
#include "Grid.h"
#include "ViewManager.h"
#include "Objects/BaseObject.h"


// This constant is used with GetScreenScaleFactor and was found experimentally.
static const float kViewDistanceScaleFactor = 0.06f;

const GUID& CRotateTool::GetClassID()
{
    // {A50E5B95-05B9-41A3-8D8E-BDA3E930A396}
    static const GUID guid = {
        0xA50E5B95, 0x05B9, 0x41A3, { 0x8D, 0x8E, 0xBD, 0xA3, 0xE9, 0x30, 0xA3, 0x96 }
    };
    return guid;
}

//! This method returns the human readable name of the class.
//! This method returns Category of this class, Category is specifying where this tool class fits best in create panel.
void CRotateTool::RegisterTool(CRegistrationContext& rc)
{
    rc.pClassFactory->RegisterClass(new CQtViewClass<CRotateTool>("EditTool.Rotate", "Select", ESYSTEM_CLASS_EDITTOOL));
}

CRotateTool::CRotateTool(CBaseObject* pObject, QWidget* parent /*= nullptr*/)
    : CObjectMode(parent)
    , m_initialViewAxisAngleRadians(0.f)
    , m_angleToCursor(0.f)
    , m_highlightAxis(AxisNone)
    , m_draggingMouse(false)
    , m_lastPosition(0, 0)
    , m_rotationAngles(0, 0, 0)
    , m_object(pObject)
    , m_bTransformChanged(false)
    , m_totalRotationAngle(0.f)
    , m_basisAxisRadius(4.f)
    , m_viewAxisRadius(5.f)
    , m_arcRotationStepRadians(DEG2RAD(5.f))
{
    m_axes[AxisX] = RotationDrawHelper::Axis(Col_Red, Col_Yellow);
    m_axes[AxisY] = RotationDrawHelper::Axis(Col_Green, Col_Yellow);
    m_axes[AxisZ] = RotationDrawHelper::Axis(Col_Blue, Col_Yellow);
    m_axes[AxisView] = RotationDrawHelper::Axis(Col_White, Col_Yellow);

    if (m_object)
    {
        m_object->AddEventListener(this);
    }

    GetIEditor()->GetObjectManager()->SetSelectCallback(this);
}

bool CRotateTool::OnSelectObject(CBaseObject* object)
{
    m_object = object;
    if (m_object)
    {
        m_object->AddEventListener(this);
    }
    return true;
}

bool CRotateTool::CanSelectObject([[maybe_unused]] CBaseObject* object)
{
    return true;
}

void CRotateTool::OnObjectEvent(CBaseObject* object, int event)
{
    if (event == CBaseObject::ON_DELETE || event == CBaseObject::ON_UNSELECT)
    {
        if (m_object && m_object == object)
        {
            m_object->RemoveEventListener(this);
            m_object = nullptr;
        }
    }
}

void CRotateTool::Display(DisplayContext& dc)
{
    if (!m_object)
    {
        return;
    }

    const bool visible =
            !m_object->IsHidden()
        &&  !m_object->IsFrozen()
        &&  m_object->IsSelected();

    if (!visible)
    {
        GetIEditor()->SetEditTool(new NullEditTool());
        return;
    }

    RotationDrawHelper::DisplayContextScope displayContextScope(dc);
    m_hc.camera = dc.camera;
    m_hc.view = dc.view;
    m_hc.b2DViewport = static_cast<CViewport*>(dc.view)->GetType() != ET_ViewportCamera;
    dc.SetLineWidth(m_lineThickness);

    // Calculate the screen space position from which we cast a ray (center of viewport).
    int viewportWidth = 0;
    int viewportHeight = 0;
    dc.view->GetDimensions(&viewportWidth, &viewportHeight);
    m_hc.point2d = QPoint(viewportWidth / 2, viewportHeight / 2);

    // Calculate the ray from the camera position to the selection.
    dc.view->ViewToWorldRay(m_hc.point2d, m_hc.raySrc, m_hc.rayDir);

    Matrix34 objectTransform = GetTransform(GetIEditor()->GetReferenceCoordSys(), dc.view);

    AffineParts ap;
    ap.Decompose(objectTransform);

    Vec3 position = ap.pos;
    CSelectionGroup* selection = GetIEditor()->GetSelection();
    if (selection->GetCount() > 1)
    {
        position = selection->GetCenter();
    }

    float screenScale = GetScreenScale(dc.view, dc.camera);

    // X axis arc
    Vec3 cameraViewDir = (m_hc.raySrc - position).GetNormalized();
    float cameraAngle = atan2f(cameraViewDir.y, cameraViewDir.x);
    m_axes[AxisX].Draw(dc, position, ap.rot.GetColumn0(), cameraAngle, m_arcRotationStepRadians, m_basisAxisRadius, m_highlightAxis == AxisX, m_object, screenScale);

    // Y axis arc
    cameraAngle = atan2f(-cameraViewDir.z, cameraViewDir.x);
    m_axes[AxisY].Draw(dc, position, ap.rot.GetColumn1(), cameraAngle, m_arcRotationStepRadians, m_basisAxisRadius, m_highlightAxis == AxisY, m_object, screenScale);

    // View direction axis
    Vec3 cameraPos = dc.camera->GetPosition();

    Vec3 axis = cameraPos - position;
    axis.NormalizeSafe();

    // Z axis arc
    cameraAngle = atan2f(axis.y, axis.x);
    m_axes[AxisZ].Draw(dc, position, objectTransform.GetColumn2().GetNormalized(), cameraAngle, m_arcRotationStepRadians, m_basisAxisRadius, m_highlightAxis == AxisZ, m_object, screenScale);

    // FIXME: currently, rotating multiple selections using the view axis may result in severe rotation artifacts, it's necessary to make sure
    // the calculated rotation angle is smooth.
    if (!m_hc.b2DViewport && selection->GetCount() == 1 || m_object->CheckFlags(OBJFLAG_IS_PARTICLE))
    {
        // Draw view direction axis
        dc.SetColor(m_highlightAxis == AxisView ? Col_Yellow : Col_White);

        cameraViewDir = m_hc.camera->GetViewdir().normalized();
        dc.DrawArc(position, m_viewAxisRadius * GetScreenScale(dc.view, dc.camera), 0, 360.f, RAD2DEG(m_arcRotationStepRadians), cameraViewDir);
    }

    // Draw angle decorator
    if (RotationControlConfiguration::Get().RotationControl_DrawDecorators)
    {
        DrawAngleDecorator(dc);
    }

    // Display total rotation angle in degrees.
    if (!m_hc.b2DViewport && fabs(m_totalRotationAngle) > FLT_EPSILON)
    {
        QString label;
        label = QString::number(RAD2DEG(m_totalRotationAngle), 'f', 2);

        const float textScale = 1.5f;
        const ColorF textBackground = ColorF(0.2f, 0.2f, 0.2f, 0.6f);

        if (m_object->CheckFlags(OBJFLAG_IS_PARTICLE))
        {
            dc.DrawTextLabel(ap.pos, textScale, label.toUtf8().data());
        }
        else
        {
            dc.DrawTextOn2DBox(ap.pos, label.toUtf8().data(), textScale, Col_White, textBackground);
        }
    }

    // Draw debug diagnostics
    if (RotationControlConfiguration::Get().RotationControl_DebugHitTesting)
    {
        DrawHitTestGeometry(dc, m_hc);
    }

    // Draw debug tracking of the view direction angle
    if (RotationControlConfiguration::Get().RotationControl_AngleTracking)
    {
        DrawViewDirectionAngleTracking(dc, m_hc);
    }
}

void CRotateTool::DrawAngleDecorator(DisplayContext& dc)
{
    if (m_highlightAxis == AxisView)
    {
        //Vec3 cameraViewDir = dc.view->GetViewTM().GetColumn1().GetNormalized();
        Vec3 cameraViewDir = dc.camera->GetViewMatrix().GetColumn1().GetNormalized(); //Get the viewDir from the camera instead of from the view
        // FIXME: The angle and sweep calculation here is incorrect.
        float cameraAngle = atan2f(cameraViewDir.y, -cameraViewDir.x);
        float angle = m_initialViewAxisAngleRadians - cameraAngle - (g_PI / 2);
        float angleDelta = (m_angleToCursor - g_PI2 * floor(m_initialViewAxisAngleRadians / g_PI2)) - (m_initialViewAxisAngleRadians - (cameraAngle - (g_PI / 2)));

        RotationDrawHelper::AngleDecorator::Draw(dc, m_object->GetWorldPos(), cameraViewDir, m_initialViewAxisAngleRadians, angleDelta, m_arcRotationStepRadians, m_viewAxisRadius, GetScreenScale(dc.view, dc.camera));
    }
    else
    {
        if (fabs(m_totalRotationAngle) > FLT_EPSILON)
        {
            float screenScale = GetScreenScale(dc.view, dc.camera);
            switch (m_highlightAxis)
            {
            case AxisX:
                RotationDrawHelper::AngleDecorator::Draw(dc, m_object->GetWorldPos(), m_object->GetRotation().GetColumn0(), m_initialViewAxisAngleRadians, m_totalRotationAngle, m_arcRotationStepRadians, m_basisAxisRadius, screenScale);
                break;
            case AxisY:
                RotationDrawHelper::AngleDecorator::Draw(dc, m_object->GetWorldPos(), m_object->GetRotation().GetColumn1(), m_initialViewAxisAngleRadians, m_totalRotationAngle, m_arcRotationStepRadians, m_basisAxisRadius, screenScale);
                break;
            case AxisZ:
                RotationDrawHelper::AngleDecorator::Draw(dc, m_object->GetWorldPos(), m_object->GetRotation().GetColumn2(), m_initialViewAxisAngleRadians, m_totalRotationAngle, m_arcRotationStepRadians, m_basisAxisRadius, screenScale);
                break;
            default:
                break;
            }
        }
    }
}

bool CRotateTool::HitTest(CBaseObject* object, HitContext& hc)
{
    if (!m_object)
    {
        return CObjectMode::HitTest(object, hc);
    }
    m_hc = hc;
    m_highlightAxis = AxisNone;

    float screenScale = GetScreenScale(hc.view, hc.camera);

    // Determine intersection with the axis view direction.
    CSelectionGroup* selection = GetIEditor()->GetSelection();
    if (!m_hc.b2DViewport && selection->GetCount() == 1 || m_object->CheckFlags(OBJFLAG_IS_PARTICLE))
    {
        if (m_axes[AxisView].HitTest(object, hc, m_viewAxisRadius, m_arcRotationStepRadians, hc.camera ? hc.camera->GetViewMatrix().GetInverted().GetColumn1() : hc.view->GetViewTM().GetColumn1(), screenScale))
        {
            m_highlightAxis = AxisView;
            GetIEditor()->SetAxisConstraints(AXIS_XYZ);
            return true;
        }
    }

    // Determine any intersection with a major axis.
    AffineParts ap;
    ap.Decompose(GetTransform(GetIEditor()->GetReferenceCoordSys(), hc.view));

    if (m_axes[AxisX].HitTest(object, hc, m_basisAxisRadius, m_arcRotationStepRadians, ap.rot.GetColumn0(), screenScale))
    {
        m_highlightAxis = AxisX;
        GetIEditor()->SetAxisConstraints(AXIS_X);
        return true;
    }

    if (m_axes[AxisY].HitTest(object, hc, m_basisAxisRadius, m_arcRotationStepRadians, ap.rot.GetColumn1(), screenScale))
    {
        m_highlightAxis = AxisY;
        GetIEditor()->SetAxisConstraints(AXIS_Y);
        return true;
    }

    if (m_axes[AxisZ].HitTest(object, hc, m_basisAxisRadius, m_arcRotationStepRadians, ap.rot.GetColumn2(), screenScale))
    {
        m_highlightAxis = AxisZ;
        GetIEditor()->SetAxisConstraints(AXIS_Z);
        return true;
    }

    return false;
}

void CRotateTool::DeleteThis()
{
    delete this;
}

bool CRotateTool::OnKeyDown([[maybe_unused]] CViewport* view, uint32 nChar, [[maybe_unused]] uint32 nRepCnt, [[maybe_unused]] uint32 nFlags)
{
    if (nChar == VK_ESCAPE)
    {
        GetIEditor()->GetObjectManager()->ClearSelection();
        return true;
    }
    return false;
}

Matrix34 CRotateTool::GetTransform(RefCoordSys referenceCoordinateSystem, IDisplayViewport* view)
{
    Matrix34 objectTransform = Matrix34::CreateIdentity();
    if (m_object)
    {
        switch (referenceCoordinateSystem)
        {
        case COORDS_VIEW:
            if (view)
            {
                objectTransform = view->GetViewTM();
            }
            objectTransform.SetTranslation(m_object->GetWorldTM().GetTranslation());
            break;
        case COORDS_LOCAL:
            objectTransform = m_object->GetWorldTM();
            break;
        case COORDS_PARENT:
            if (m_object->GetParent())
            {
                Matrix34 parentTM = m_object->GetParent()->GetWorldTM();
                parentTM.SetTranslation(m_object->GetWorldTM().GetTranslation());
                objectTransform = parentTM;
            }
            else
            {
                objectTransform.SetTranslation(m_object->GetWorldTM().GetTranslation());
            }
            break;
        case COORDS_WORLD:
            objectTransform.SetTranslation(m_object->GetWorldTM().GetTranslation());
            break;
        }
    }
    return objectTransform;
}

float CRotateTool::CalculateOrientation(const QPoint& p1, const QPoint& p2, const QPoint& p3)
{
    // Source: https://www.geeksforgeeks.org/orientation-3-ordered-points/
    float c = (p2.y() - p1.y()) * (p3.x() - p2.x()) - (p3.y() - p2.y()) * (p2.x() - p1.x());
    return c > 0 ? 1.0f : -1.0f;
}

CRotateTool::~CRotateTool()
{
    if (m_object)
    {
        m_object->RemoveEventListener(this);
    }
    GetIEditor()->GetObjectManager()->SetSelectCallback(nullptr);
}

bool CRotateTool::OnLButtonDown(CViewport* view, int nFlags, const QPoint& p)
{
    QPoint point = p;
    m_hc.view = view;
    m_hc.b2DViewport = view->GetType() != ET_ViewportCamera;
    m_hc.point2d = point;
    if (nFlags == OBJFLAG_IS_PARTICLE)
    {
        view->setHitcontext(point, m_hc.raySrc, m_hc.rayDir);
    }
    else
    {
        view->ViewToWorldRay(point, m_hc.raySrc, m_hc.rayDir);
    }

    if (m_hc.object && m_hc.object != m_object)
    {
        GetIEditor()->ClearSelection();
        return CObjectMode::OnLButtonDown(view, nFlags, point);
    }

    if (m_highlightAxis != AxisNone)
    {
        view->BeginUndo();
        view->CaptureMouse();
        view->SetCurrentCursor(STD_CURSOR_ROTATE);

        m_draggingMouse = true;

        // Store the starting drag angle when we first click the mouse, we will need this to know
        // how much of the rotation we need to apply.
        if (m_highlightAxis == AxisView)
        {
            Vec3 cameraViewDir = m_hc.camera->GetViewdir().GetNormalized();
            float cameraAngle = atan2f(cameraViewDir.y, -cameraViewDir.x);
            m_initialViewAxisAngleRadians = m_angleToCursor - cameraAngle - (g_PI / 2);
            m_initialViewAxisAngleRadians -= static_cast<float>(g_PI);
        }

        m_lastPosition = point;
        m_rotationAngles = Ang3(0, 0, 0);

        AzToolsFramework::EntityIdList selectedEntities;
        AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
            selectedEntities,
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

        AzToolsFramework::EditorTransformChangeNotificationBus::Broadcast(
            &AzToolsFramework::EditorTransformChangeNotificationBus::Events::OnEntityTransformChanging,
            selectedEntities);

        return true;
    }

    return CObjectMode::OnLButtonDown(view, nFlags, point);
}

bool CRotateTool::OnLButtonUp(CViewport* view, int nFlags, const QPoint& p)
{
    QPoint point = p;
    if (nFlags == OBJFLAG_IS_PARTICLE)
    {
        view->setHitcontext(point, m_hc.raySrc, m_hc.rayDir);
    }
    else
    {
        view->ViewToWorldRay(point, m_hc.raySrc, m_hc.rayDir);
    }

    if (m_draggingMouse)
    {
        // We are no longer dragging the mouse, so we will release it and reset any state variables.
        {
            AzToolsFramework::ScopedUndoBatch undo("Rotate");
        }
        view->AcceptUndo("Rotate Selection");
        view->ReleaseMouse();
        view->SetCurrentCursor(STD_CURSOR_DEFAULT);

        m_draggingMouse = false;
        m_totalRotationAngle = 0.f;
        m_initialViewAxisAngleRadians = 0.f;
        m_angleToCursor = 0.f;

        // Apply the transform changes to the selection.
        if (m_bTransformChanged)
        {
            CSelectionGroup* pSelection = GetIEditor()->GetSelection();
            if (pSelection)
            {
                pSelection->FinishChanges();
            }

            m_bTransformChanged = false;

            view->ResetSelectionRegion();
            // Reset selected rectangle.
            view->SetSelectionRectangle(QRect());
            view->SetAxisConstrain(GetIEditor()->GetAxisConstrains());

            AzToolsFramework::EntityIdList selectedEntities;
            AzToolsFramework::ToolsApplicationRequests::Bus::BroadcastResult(
                selectedEntities,
                &AzToolsFramework::ToolsApplicationRequests::Bus::Events::GetSelectedEntities);

            AzToolsFramework::EditorTransformChangeNotificationBus::Broadcast(
                &AzToolsFramework::EditorTransformChangeNotificationBus::Events::OnEntityTransformChanged,
                selectedEntities);
        }
    }

    return CObjectMode::OnLButtonUp(view, nFlags, point);
}

bool CRotateTool::OnMouseMove(CViewport* view, int nFlags, const QPoint& p)
{
    QPoint point = p;
    if (!m_object)
    {
        return CObjectMode::OnMouseMove(view, nFlags, point);
    }

    // Prevent the opening of the context menu during a mouse move.
    m_openContext = false;

    // We calculate the mouse drag direction vector's angle from the object to the mouse position.
    QPoint objectCenter;
    if (nFlags != OBJFLAG_IS_PARTICLE)
    {
        objectCenter = view->WorldToView(GetIEditor()->GetSelection()->GetCenter());
    }
    else
    if (parent() && parent()->isWidgetType())
    {
        QWidget *wParent = static_cast<QWidget*>(parent());

        // HACK: This is only valid for the particle editor and needs refactored.
        const QRect rect = wParent->contentsRect();
        objectCenter = view->WorldToViewParticleEditor(m_object->GetWorldPos(), rect.width(), rect.height());
    }

    Vec2 dragDirection = Vec2(point.x() - objectCenter.x(), point.y() - objectCenter.y());
    dragDirection.Normalize();

    float angleToCursor = (atan2f(dragDirection.y, dragDirection.x));
    m_angleToCursor = angleToCursor - g_PI2 * floor(angleToCursor / g_PI2);

    if (m_draggingMouse)
    {
        GetIEditor()->RestoreUndo();

        view->SetCurrentCursor(STD_CURSOR_ROTATE);

        RefCoordSys referenceCoordSys = GetIEditor()->GetReferenceCoordSys();

        if (m_highlightAxis == AxisView)
        {
            // Calculate the angular difference between the starting rotation angle, taking into account the camera's angle to ensure a smooth rotation.
            Vec3 cameraViewDir = m_hc.camera->GetViewdir();
            float cameraAngle = atan2f(cameraViewDir.y, cameraViewDir.x);
            float angleDelta = (m_angleToCursor - g_PI2 * floor(m_initialViewAxisAngleRadians / g_PI2)) - (m_initialViewAxisAngleRadians - (cameraAngle - (g_PI / 2)));

            // Snap the angle is necessary
            angleDelta = view->GetViewManager()->GetGrid()->SnapAngle(RAD2DEG(angleDelta));

            if (nFlags != OBJFLAG_IS_PARTICLE)
            {
                Matrix34 viewRotation = Matrix34::CreateRotationAA(DEG2RAD(angleDelta), cameraViewDir);
                GetIEditor()->GetSelection()->Rotate(viewRotation, COORDS_WORLD);
            }
            else
            {
                Quat quatRotation = Quat::CreateRotationAA(DEG2RAD(angleDelta), cameraViewDir);
                m_object->SetRotation(quatRotation);
            }

            m_bTransformChanged = true;
        }
        else
        if (m_highlightAxis != AxisNone)
        {
            float distanceMoved = (point - m_lastPosition).manhattanLength(); // screen-space distance dragged
            float distanceToCenter = (m_lastPosition - objectCenter).manhattanLength(); // screen-space distance to object center
            float roationDelta = RAD2DEG(atan2f(distanceMoved, distanceToCenter)); // unsigned rotation angle
            float orientation = CalculateOrientation(objectCenter, m_lastPosition, point); // Calculate if rotation dragging gizmo clockwise or counter-clockwise

            m_lastPosition = point;

            // Calculate orientation of the object's axis towards camera
            Vec3 directionToObject = (GetIEditor()->GetSelection()->GetCenter() - m_hc.camera->GetMatrix().GetTranslation()).normalize();

            float directionX = 1.0f;
            float directionY = 1.0f;
            float directionZ = 1.0f;

            switch (referenceCoordSys)
            {
            case COORDS_LOCAL:
                directionX = directionToObject.Dot(m_object->GetWorldTM().GetColumn0()) > 0 ? -1.0f : 1.0f;
                directionY = directionToObject.Dot(m_object->GetWorldTM().GetColumn1()) > 0 ? -1.0f : 1.0f;
                directionZ = directionToObject.Dot(m_object->GetWorldTM().GetColumn2()) > 0 ? -1.0f : 1.0f;
                break;
            case COORDS_PARENT:
                if (m_object->GetParent())
                {
                    directionX = directionToObject.Dot(m_object->GetParent()->GetWorldTM().GetColumn0()) > 0 ? -1.0f : 1.0f;
                    directionY = directionToObject.Dot(m_object->GetParent()->GetWorldTM().GetColumn1()) > 0 ? -1.0f : 1.0f;
                    directionZ = directionToObject.Dot(m_object->GetParent()->GetWorldTM().GetColumn2()) > 0 ? -1.0f : 1.0f;
                }
                else
                {
                    directionX = directionToObject.Dot(m_object->GetWorldTM().GetColumn0()) > 0 ? -1.0f : 1.0f;
                    directionY = directionToObject.Dot(m_object->GetWorldTM().GetColumn1()) > 0 ? -1.0f : 1.0f;
                    directionZ = directionToObject.Dot(m_object->GetWorldTM().GetColumn2()) > 0 ? -1.0f : 1.0f;
                }
                break;
            case COORDS_VIEW:
            case COORDS_WORLD:
                directionX = directionToObject.Dot(Vec3(1, 0, 0)) > 0 ? -1.0f : 1.0f;
                directionY = directionToObject.Dot(Vec3(0, 1, 0)) > 0 ? -1.0f : 1.0f;
                directionZ = directionToObject.Dot(Vec3(0, 0, 1)) > 0 ? -1.0f : 1.0f;
                break;
            }

            switch (m_highlightAxis)
            {
            case AxisX:
                m_rotationAngles.x += roationDelta * directionX * orientation;
                break;
            case AxisY:
                m_rotationAngles.y += roationDelta * directionY * orientation;
                break;
            case AxisZ:
                m_rotationAngles.z += roationDelta * directionZ * orientation;
                break;
            default:
                break;
            }

            // Snap the angle if necessary
            m_rotationAngles = view->GetViewManager()->GetGrid()->SnapAngle(m_rotationAngles);

            // Compute the total amount rotated
            Vec3 vDragValue = Vec3(m_rotationAngles);
            m_totalRotationAngle = DEG2RAD(vDragValue.len());

            // Apply the rotation
            if (nFlags != OBJFLAG_IS_PARTICLE)
            {
                GetIEditor()->GetSelection()->Rotate(m_rotationAngles, referenceCoordSys);
            }
            else
            {
                Quat currentRotation = (m_object->GetRotation());
                Quat rotateTM = currentRotation * Quat::CreateRotationXYZ(DEG2RAD(-m_rotationAngles / 50.0f));
                m_object->SetRotation(rotateTM);
            }

            m_bTransformChanged = fabs(m_totalRotationAngle) > FLT_EPSILON;
        }
    }
    else
    {
        // If we are not yet dragging the mouse, do the hit testing to highlight the axis the mouse is over.
        m_hc.view = view;
        m_hc.b2DViewport = view->GetType() != ET_ViewportCamera;
        m_hc.point2d = point;

        if (nFlags != OBJFLAG_IS_PARTICLE)
        {
            view->ViewToWorldRay(point, m_hc.raySrc, m_hc.rayDir);
        }
        else
        {
            view->setHitcontext(point, m_hc.raySrc, m_hc.rayDir);
        }

        if (HitTest(m_object, m_hc))
        {
            // Display a cursor that makes it clear to the user that he is over an axis that can be rotated.
            view->SetCurrentCursor(STD_CURSOR_ROTATE);
        }
        else
        {
            // Nothing has been hit, reset the cursor back to default in case it was changed previously.
            view->SetCurrentCursor(STD_CURSOR_DEFAULT);
        }
    }

    // We always consider the rotation tool's OnMove event handled
    return true;
}

float CRotateTool::GetScreenScale(IDisplayViewport* view, CCamera* camera /*=nullptr*/)
{
    Matrix34 objectTransform = GetTransform(GetIEditor()->GetReferenceCoordSys(), view);

    AffineParts ap;
    ap.Decompose(objectTransform);

    if (m_object && m_object->CheckFlags(OBJFLAG_IS_PARTICLE))
    {
        return view->GetScreenScaleFactor(*camera, ap.pos) * kViewDistanceScaleFactor;
    }

    return static_cast<CViewport*>(view)->GetScreenScaleFactor(ap.pos) * kViewDistanceScaleFactor;
}

void CRotateTool::DrawHitTestGeometry(DisplayContext& dc, HitContext& hc)
{
    AffineParts ap;
    ap.Decompose(GetTransform(GetIEditor()->GetReferenceCoordSys(), dc.view));

    Vec3 position = ap.pos;
    CSelectionGroup* selection = GetIEditor()->GetSelection();
    if (selection->GetCount() > 1 && !m_object->CheckFlags(OBJFLAG_IS_PARTICLE))
    {
        position = selection->GetCenter();
    }

    float screenScale = GetScreenScale(dc.view, dc.camera);

    // Draw debug test surface for each axis.
    m_axes[AxisX].DebugDrawHitTestSurface(dc, hc, position, m_basisAxisRadius, m_arcRotationStepRadians, ap.rot.GetColumn0(), screenScale);
    m_axes[AxisY].DebugDrawHitTestSurface(dc, hc, position, m_basisAxisRadius, m_arcRotationStepRadians, ap.rot.GetColumn1(), screenScale);
    m_axes[AxisZ].DebugDrawHitTestSurface(dc, hc, position, m_basisAxisRadius, m_arcRotationStepRadians, ap.rot.GetColumn2(), screenScale);

    // We don't render the view axis rotation for multiple selection.
    if (!hc.b2DViewport && selection->GetCount() == 1)
    {
        Vec3 cameraViewDir = hc.view->GetViewTM().GetColumn1().GetNormalized();
        m_axes[AxisView].DebugDrawHitTestSurface(dc, hc, position, m_viewAxisRadius, m_arcRotationStepRadians, cameraViewDir, screenScale);
    }
}

void CRotateTool::DrawViewDirectionAngleTracking(DisplayContext& dc, HitContext& hc)
{
    Vec3 a;
    Vec3 b;

    // Calculate a basis for the camera view direction.
    Vec3 cameraViewDir = hc.view->GetViewTM().GetColumn1().GetNormalized();
    GetBasisVectors(cameraViewDir, a, b);

    // Calculates the camera view direction angle.
    float angle = m_angleToCursor;
    float cameraAngle = atan2f(cameraViewDir.y, -cameraViewDir.x);

    // Ensures the angle remains camera aligned.
    angle -= cameraAngle - (g_PI / 2);

    // The position will be either the object's center or the selection's center.
    Vec3 position = GetTransform(GetIEditor()->GetReferenceCoordSys(), dc.view).GetTranslation();
    CSelectionGroup* selection = GetIEditor()->GetSelection();
    if (selection->GetCount() > 1 && !m_object->CheckFlags(OBJFLAG_IS_PARTICLE))
    {
        position = selection->GetCenter();
    }

    float screenScale = GetScreenScale(dc.view, dc.camera);

    const float cosAngle = cos(angle);
    const float sinAngle = sin(angle);

    // The resulting position will be in a circular orientation based on the resulting angle.
    Vec3 p0;
    p0.x = position.x + (cosAngle * a.x + sinAngle * b.x) * m_viewAxisRadius * screenScale;
    p0.y = position.y + (cosAngle * a.y + sinAngle * b.y) * m_viewAxisRadius * screenScale;
    p0.z = position.z + (cosAngle * a.z + sinAngle * b.z) * m_viewAxisRadius * screenScale;

    const float ballRadius = 0.1f * screenScale;
    dc.SetColor(Col_Magenta);
    dc.DrawBall(p0, ballRadius);
}

namespace RotationDrawHelper
{
    Axis::Axis(const ColorF& defaultColor, const ColorF& highlightColor)
    {
        m_colors[StateDefault] = defaultColor;
        m_colors[StateHighlight] = highlightColor;
    }

    void Axis::Draw(DisplayContext& dc, const Vec3& position, const Vec3& axis, float angleRadians, float angleStepRadians, float radius, bool highlighted, CBaseObject* object, float screenScale)
    {
        if (static_cast<CViewport*>(dc.view)->GetType() != ET_ViewportCamera || object->CheckFlags(OBJFLAG_IS_PARTICLE))
        {
            bool set = dc.SetDrawInFrontMode(true);

            // Draw the front facing arc
            dc.SetColor(!highlighted ? m_colors[StateDefault] : m_colors[StateHighlight]);
            dc.DrawArc(position, radius * screenScale, 0.f, 360.f, RAD2DEG(angleStepRadians), axis);

            dc.SetDrawInFrontMode(set);
        }
        else
        {
            // Draw the front facing arc
            dc.SetColor(!highlighted ? m_colors[StateDefault] : m_colors[StateHighlight]);
            dc.DrawArc(position, radius * screenScale, RAD2DEG(angleRadians) - 90.f, 180.f, RAD2DEG(angleStepRadians), axis);

            // Draw the back side
            dc.SetColor(!highlighted ? Col_Gray : m_colors[StateHighlight]);
            dc.DrawArc(position, radius * screenScale, RAD2DEG(angleRadians) + 90.f, 180.f, RAD2DEG(angleStepRadians), axis);
        }

        static bool drawAxisMidPoint = false;
        if (drawAxisMidPoint)
        {
            const float kBallRadius = 0.085f;
            Vec3 a;
            Vec3 b;
            GetBasisVectors(axis, a, b);

            float cosAngle = cos(angleRadians);
            float sinAngle = sin(angleRadians);

            Vec3 offset;
            offset.x = position.x + (cosAngle * a.x + sinAngle * b.x) * screenScale * radius;
            offset.y = position.y + (cosAngle * a.y + sinAngle * b.y) * screenScale * radius;
            offset.z = position.z + (cosAngle * a.z + sinAngle * b.z) * screenScale * radius;

            dc.SetColor(!highlighted ? m_colors[StateDefault] : m_colors[StateHighlight]);
            dc.DrawBall(offset, kBallRadius * screenScale);
        }
    }

    void Axis::GenerateHitTestGeometry([[maybe_unused]] HitContext& hc, const Vec3& position, float radius, float angleStepRadians, const Vec3& axis, float screenScale)
    {
        m_vertices.clear();

        // The number of vertices relies on the angleStepRadians, the smaller the angle, the higher the vertex count.
        int numVertices = static_cast<int>(std::ceil(g_PI2 / angleStepRadians));

        Vec3 a;
        Vec3 b;
        GetBasisVectors(axis, a, b);

        // The geometry is calculated by computing a circle aligned to the specified axis.
        float angle = 0.f;
        for (int i = 0; i < numVertices; ++i)
        {
            float cosAngle = cos(angle);
            float sinAngle = sin(angle);

            Vec3 p;
            p.x = position.x + (cosAngle * a.x + sinAngle * b.x) * radius * screenScale;
            p.y = position.y + (cosAngle * a.y + sinAngle * b.y) * radius * screenScale;
            p.z = position.z + (cosAngle * a.z + sinAngle * b.z) * radius * screenScale;
            m_vertices.push_back(p);

            angle += angleStepRadians;
        }
    }

    bool Axis::IntersectRayWithQuad(const Ray& ray, Vec3 quad[4], Vec3& contact)
    {
        contact = Vec3();

        // Tests ray vs. two quads, the front facing quad and a back facing quad.
        // will return true if an intersection occurs and the world space position of the contact.
        return (Intersect::Ray_Triangle(ray, quad[0], quad[1], quad[2], contact) || Intersect::Ray_Triangle(ray, quad[0], quad[2], quad[3], contact) ||
                Intersect::Ray_Triangle(ray, quad[0], quad[2], quad[1], contact) || Intersect::Ray_Triangle(ray, quad[0], quad[3], quad[2], contact));
    }

    bool Axis::HitTest(CBaseObject* object, HitContext& hc, float radius, float angleStepRadians, const Vec3& axis, float screenScale)
    {
        AffineParts ap;
        ap.Decompose(object->GetWorldTM());

        Vec3 position = ap.pos;

        CSelectionGroup* selection = GetIEditor()->GetSelection();
        if (selection->GetCount() > 1 && !object->CheckFlags(OBJFLAG_IS_PARTICLE))
        {
            position = selection->GetCenter();
        }

        // Generate intersection testing geometry
        GenerateHitTestGeometry(hc, position, radius, angleStepRadians, axis, screenScale);

        Ray ray;
        ray.origin = hc.raySrc;
        ray.direction = hc.rayDir;

        // Calculate the face normal with the first two vertices in the intersection geometry.
        Vec3 vdir0 = (m_vertices[0] - m_vertices[1]).GetNormalized();
        Vec3 vdir1 = (m_vertices[2] - m_vertices[1]).GetNormalized();

        Vec3 normal;
        if (!hc.b2DViewport)
        {
            normal = hc.view->GetViewTM().GetColumn1();
        }
        else
        {
            normal = hc.view->GetConstructionPlane()->n;
        }

        float shortestDistance = std::numeric_limits<float>::max();
        size_t numVertices = m_vertices.size();
        for (size_t i = 0; i < numVertices; ++i)
        {
            const Vec3& v0 = m_vertices[i];
            const Vec3& v1 = m_vertices[(i + 1) % numVertices];
            Vec3 right = (v0 - v1).Cross(normal).GetNormalized() * screenScale * m_hitTestWidth;

            // Calculates the quad vertices aligned to the face normal.
            Vec3 quad[4];
            quad[0] = v0 + right;
            quad[1] = v1 + right;
            quad[2] = v1 - right;
            quad[3] = v0 - right;

            Vec3 contact;
            if (IntersectRayWithQuad(ray, quad, contact))
            {
                Vec3 intersectionPoint;
                if (PointToLineDistance(v0, v1, contact, intersectionPoint))
                {
                    // Ensure the intersection is within the quad's extents
                    float distanceToIntersection = intersectionPoint.GetDistance(contact);
                    if (distanceToIntersection < shortestDistance)
                    {
                        shortestDistance = distanceToIntersection;
                    }
                }
            }
        }

        // if shortestDistance is less than the maximum possible distance, we have an intersection.
        if (shortestDistance < std::numeric_limits<float>::max() - FLT_EPSILON)
        {
            hc.object = object;
            hc.dist = shortestDistance;
            return true;
        }

        return false;
    }

    void Axis::DebugDrawHitTestSurface(DisplayContext& dc, HitContext& hc, const Vec3& position, float radius, float angleStepRadians, const Vec3& axis, float screenScale)
    {
        // Generate the geometry for rendering.
        GenerateHitTestGeometry(hc, position, radius, angleStepRadians, axis, screenScale);

        // Calculate the face normal with the first two vertices in the intersection geometry.
        Vec3 vdir0 = (m_vertices[0] - m_vertices[1]).GetNormalized();
        Vec3 vdir1 = (m_vertices[2] - m_vertices[1]).GetNormalized();

        Vec3 normal;
        if (!hc.b2DViewport)
        {
            normal = hc.view->GetViewTM().GetColumn1();
        }
        else
        {
            normal = hc.view->GetConstructionPlane()->n;
        }

        float shortestDistance = std::numeric_limits<float>::max();

        Ray ray;
        ray.origin = hc.raySrc;
        ray.direction = hc.rayDir;

        size_t numVertices = m_vertices.size();
        for (size_t i = 0; i < numVertices; ++i)
        {
            const Vec3& v0 = m_vertices[i];
            const Vec3& v1 = m_vertices[(i + 1) % numVertices];
            Vec3 right = (v0 - v1).Cross(normal).GetNormalized() * screenScale * m_hitTestWidth;

            // Calculates the quad vertices aligned to the face normal.
            Vec3 quad[4];
            quad[0] = v0 + right;
            quad[1] = v1 + right;
            quad[2] = v1 - right;
            quad[3] = v0 - right;

            // Draw double sided quad to ensure it is always visible regardless of camera orientation.
            dc.DrawQuad(quad[0], quad[1], quad[2], quad[3]);
            dc.DrawQuad(quad[3], quad[2], quad[1], quad[0]);

            Vec3 contact;
            if (IntersectRayWithQuad(ray, quad, contact))
            {
                Vec3 intersectionPoint;
                if (PointToLineDistance(v0, v1, contact, intersectionPoint))
                {
                    // Ensure the intersection is within the quad's extents
                    float distanceToIntersection = intersectionPoint.GetDistance(contact);
                    if (distanceToIntersection < shortestDistance)
                    {
                        shortestDistance = distanceToIntersection;

                        // Highlight the quad at which an intersection occurred.
                        auto c = dc.GetColor();
                        dc.SetColor(Col_Red);
                        dc.DrawQuad(quad[0], quad[1], quad[2], quad[3]);
                        dc.DrawQuad(quad[3], quad[2], quad[1], quad[0]);
                        dc.SetColor(c);
                    }
                }
            }
        }
    }


    namespace AngleDecorator
    {
        void Draw(DisplayContext& dc, const Vec3& position, const Vec3& axisToAlign, float startAngleRadians, float sweepAngleRadians, float stepAngleRadians, float radius, float screenScale)
        {
            float angle = startAngleRadians;

            if (fabs(sweepAngleRadians) < FLT_EPSILON || sweepAngleRadians < stepAngleRadians)
            {
                return;
            }

            if (sweepAngleRadians > g_PI)
            {
                sweepAngleRadians = g_PI - (fabs(sweepAngleRadians - g_PI));
                stepAngleRadians = -stepAngleRadians;
            }

            Vec3 a;
            Vec3 b;
            GetBasisVectors(axisToAlign, a, b);

            float cosAngle = cos(angle);
            float sinAngle = sin(angle);

            // Pre-calculate the first vertex, this is useful for rendering the first handle ball.
            Vec3 p0;
            p0.x = position.x + (cosAngle * a.x + sinAngle * b.x) * radius * screenScale;
            p0.y = position.y + (cosAngle * a.y + sinAngle * b.y) * radius * screenScale;
            p0.z = position.z + (cosAngle * a.z + sinAngle * b.z) * radius * screenScale;

            const float ballRadius = 0.1f * screenScale;

            // TODO: colors should be configurable properties
            dc.SetColor(0.f, 1.f, 0.f, 1.f);
            dc.DrawBall(p0, ballRadius);

            float alpha = 0.5f;
            dc.SetColor(0.8f, 0.8f, 0.8f, 0.5f);

            // Number of vertices is defined by stepAngleRadians, the smaller the step the higher vertex count.
            int numVertices = static_cast<int>(fabs(sweepAngleRadians / stepAngleRadians));
            if (numVertices >= 2)
            {
                Vec3 p1;
                for (int i = 0; i < numVertices; ++i)
                {
                    // We pre-calculated the first vertex, so we can advance the angle
                    angle += stepAngleRadians;

                    const float cosAngle2 = cos(angle);
                    const float sinAngle2 = sin(angle);

                    p1.x = position.x + (cosAngle2 * a.x + sinAngle2 * b.x) * radius * screenScale;
                    p1.y = position.y + (cosAngle2 * a.y + sinAngle2 * b.y) * radius * screenScale;
                    p1.z = position.z + (cosAngle2 * a.z + sinAngle2 * b.z) * radius * screenScale;

                    // Draws a triangle from the object's position to p0 and p1.
                    dc.SetColor(0.8f, 0.8f, 0.8f, alpha);
                    dc.DrawTri(position, p0, p1);

                    alpha += 0.5f * (i / numVertices);
                    p0 = p1;
                }

                // Draw the end handle ball.
                dc.SetColor(1.f, 0.f, 0.f, 1.f);
                dc.DrawBall(p1, ballRadius);
            }
        }
    }
}

RotationControlConfiguration::RotationControlConfiguration()
{
    DefineConstIntCVar(RotationControl_DrawDecorators, 0, VF_NULL, "Toggles the display of the angular decorator.");
    DefineConstIntCVar(RotationControl_DebugHitTesting, 0, VF_NULL, "Renders the hit testing geometry used for mouse input control.");
    DefineConstIntCVar(RotationControl_AngleTracking, 0, VF_NULL, "Displays a sphere aligned to the mouse cursor direction for debugging.");
}

#include <moc_RotateTool.cpp>
