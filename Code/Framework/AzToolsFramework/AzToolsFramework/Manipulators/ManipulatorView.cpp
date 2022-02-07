/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ManipulatorView.h"

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/VectorConversions.h>
#include <AzCore/std/containers/array.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/AngularManipulator.h>
#include <AzToolsFramework/Manipulators/LineSegmentSelectionManipulator.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>
#include <AzToolsFramework/Manipulators/SplineSelectionManipulator.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

AZ_CVAR(
    bool,
    ed_manipulatorDisplayBoundDebug,
    false,
    nullptr,
    AZ::ConsoleFunctorFlags::Null,
    "Display additional debug drawing for manipulator bounds");

namespace AzToolsFramework
{
    const float g_defaultManipulatorSphereRadius = 0.1f;

    AZ::Transform WorldFromLocalWithUniformScale(const AZ::EntityId entityId)
    {
        AZ::Transform worldFromLocal = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldFromLocal, entityId, &AZ::TransformBus::Events::GetWorldTM);

        return TransformUniformScale(worldFromLocal);
    }

    AZ::Vector3 GetNonUniformScale(AZ::EntityId entityId)
    {
        AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
        AZ::NonUniformScaleRequestBus::EventResult(nonUniformScale, entityId, &AZ::NonUniformScaleRequests::GetScale);
        return nonUniformScale;
    }

    AZ::Vector3 ManipulatorState::TransformPoint(const AZ::Vector3& point) const
    {
        return m_worldFromLocal.TransformPoint(m_nonUniformScale * point);
    }

    AZ::Vector3 ManipulatorState::TransformDirectionNoScaling(const AZ::Vector3& direction) const
    {
        return AzToolsFramework::TransformDirectionNoScaling(m_worldFromLocal, direction);
    }

    // Take into account the location of the camera and orientate the axis so it faces the camera.
    // if we did correct the camera (shouldCorrect is true) then we know the axis facing us it negative.
    // we can use this to change the rendering for a flipped axis if we wish.
    static void CameraCorrectAxis(
        const AZ::Vector3& axis,
        AZ::Vector3& correctedAxis,
        const ManipulatorManagerState& managerState,
        const ViewportInteraction::MouseInteraction& mouseInteraction,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& localPosition,
        const AzFramework::CameraState& cameraState,
        bool* shouldCorrect = nullptr)
    {
        // do not update (flip) the axis while the manipulator is being interacted with (mouse button is held)
        if (!(mouseInteraction.m_mouseButtons.Any() && managerState.m_interacting))
        {
            // check if we actually needed to flip the axis, if so, write to shouldCorrect
            // so we know and are able to draw it differently if we wish (e.g. hollow if flipped)
            const bool correcting = ShouldFlipCameraAxis(worldFromLocal, localPosition, axis, cameraState);

            // the corrected axis, if no flip was required, output == input
            correctedAxis = correcting ? -axis : axis;

            // optional out ref to use if we care about the result
            if (shouldCorrect)
            {
                *shouldCorrect = correcting;
            }
        }
    }

    // calculate quad bound in world space.
    static Picking::BoundShapeQuad CalculateQuadBound(
        const AZ::Vector3& localPosition,
        const ManipulatorState& manipulatorState,
        const AZ::Vector3& axis1,
        const AZ::Vector3& axis2,
        const float size)
    {
        const AZ::Vector3 worldPosition = manipulatorState.TransformPoint(localPosition);
        const AZ::Vector3 endAxis1World = manipulatorState.TransformDirectionNoScaling(axis1) * size;
        const AZ::Vector3 endAxis2World = manipulatorState.TransformDirectionNoScaling(axis2) * size;

        Picking::BoundShapeQuad quadBound;
        quadBound.m_corner1 = worldPosition;
        quadBound.m_corner2 = worldPosition + endAxis1World;
        quadBound.m_corner3 = worldPosition + endAxis1World + endAxis2World;
        quadBound.m_corner4 = worldPosition + endAxis2World;
        return quadBound;
    }

    static Picking::BoundShapeQuad CalculateQuadBoundBillboard(
        const AZ::Vector3& localPosition,
        const AZ::Transform& worldFromLocal,
        const float size,
        const AzFramework::CameraState& cameraState)
    {
        const AZ::Vector3 worldPosition = worldFromLocal.TransformPoint(localPosition);

        Picking::BoundShapeQuad quadBound;
        quadBound.m_corner1 = worldPosition - size * cameraState.m_up - size * cameraState.m_side;
        quadBound.m_corner2 = worldPosition - size * cameraState.m_up + size * cameraState.m_side;
        quadBound.m_corner3 = worldPosition + size * cameraState.m_up + size * cameraState.m_side;
        quadBound.m_corner4 = worldPosition + size * cameraState.m_up - size * cameraState.m_side;
        return quadBound;
    }

    // calculate line in world space (axis and length).
    static AZStd::pair<AZ::Vector3, AZ::Vector3> CalculateLine(
        const AZ::Vector3& localPosition, const AZ::Transform& worldFromLocal, const AZ::Vector3& axis, const float length)
    {
        return { worldFromLocal.TransformPoint(localPosition),
                 TransformPositionNoScaling(worldFromLocal, localPosition + (axis * length)) };
    }

    // calculate line bound in world space (axis and length).
    static Picking::BoundShapeLineSegment CalculateLineBound(
        const AZ::Vector3& localPosition,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& axis,
        const float length,
        const float width)
    {
        Picking::BoundShapeLineSegment lineBound;
        const auto line = CalculateLine(localPosition, worldFromLocal, axis, length);
        lineBound.m_start = line.first;
        lineBound.m_end = line.second;
        lineBound.m_width = width;
        return lineBound;
    }

    // calculate line bound in world space (start and end point).
    static Picking::BoundShapeLineSegment CalculateLineBound(
        const AZ::Vector3& localStartPosition,
        const AZ::Vector3& localEndPosition,
        const ManipulatorState& manipulatorState,
        const float width)
    {
        Picking::BoundShapeLineSegment lineBound;
        lineBound.m_start = manipulatorState.TransformPoint(localStartPosition);
        lineBound.m_end = manipulatorState.TransformPoint(localEndPosition);
        lineBound.m_width = width;
        return lineBound;
    }

    // calculate cone bound in world space.
    static Picking::BoundShapeCone CalculateConeBound(
        const AZ::Vector3& localPosition,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& axis,
        const AZ::Vector3& offset,
        const float length,
        const float radius)
    {
        Picking::BoundShapeCone coneBound;
        coneBound.m_radius = radius;
        coneBound.m_height = length;
        coneBound.m_axis = TransformDirectionNoScaling(worldFromLocal, axis);
        coneBound.m_base = TransformPositionNoScaling(worldFromLocal, localPosition + offset);
        return coneBound;
    }

    // calculate box bound in world space.
    static Picking::BoundShapeBox CalculateBoxBound(
        const AZ::Vector3& localPosition,
        const AZ::Transform& worldFromLocal,
        const AZ::Quaternion& orientation,
        const AZ::Vector3& offset,
        const AZ::Vector3& halfExtents)
    {
        Picking::BoundShapeBox boxBound;
        boxBound.m_halfExtents = halfExtents;
        boxBound.m_orientation = (worldFromLocal.GetRotation() * orientation).GetNormalized();
        boxBound.m_center = TransformPositionNoScaling(worldFromLocal, localPosition + offset);
        return boxBound;
    }

    // calculate cylinder bound in world space.
    static Picking::BoundShapeCylinder CalculateCylinderBound(
        const AZ::Vector3& localPosition,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& axis,
        const float length,
        const float radius)
    {
        Picking::BoundShapeCylinder boxBound;
        boxBound.m_base = worldFromLocal.TransformPoint(localPosition);
        boxBound.m_axis = TransformDirectionNoScaling(worldFromLocal, axis);
        boxBound.m_height = length;
        boxBound.m_radius = radius;
        return boxBound;
    }

    // calculate sphere bound in world space.
    static Picking::BoundShapeSphere CalculateSphereBound(
        const AZ::Vector3& localPosition, const ManipulatorState& manipulatorState, const float radius)
    {
        Picking::BoundShapeSphere sphereBound;
        sphereBound.m_center = manipulatorState.TransformPoint(localPosition);
        sphereBound.m_radius = radius;
        return sphereBound;
    }

    // calculate torus bound in world space.
    static Picking::BoundShapeTorus CalculateTorusBound(
        const AZ::Vector3& localPosition,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& axis,
        const float radius,
        const float width)
    {
        Picking::BoundShapeTorus torusBound;
        torusBound.m_center = worldFromLocal.TransformPoint(localPosition);
        torusBound.m_minorRadius = width;
        torusBound.m_majorRadius = radius;
        torusBound.m_axis = TransformDirectionNoScaling(worldFromLocal, axis);
        return torusBound;
    }

    // calculate spline bound in world space.
    static Picking::BoundShapeSpline CalculateSplineBound(
        const AZStd::weak_ptr<const AZ::Spline>& spline, const AZ::Transform& worldFromLocal, const float width)
    {
        Picking::BoundShapeSpline splineBound;
        splineBound.m_spline = spline;
        splineBound.m_width = width;
        splineBound.m_transform = worldFromLocal;
        return splineBound;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static float LineWidth(const bool mouseOver, const float defaultWidth, const float mouseOverWidth)
    {
        const AZStd::array<float, 2> lineWidth = { { defaultWidth, mouseOverWidth } };
        return lineWidth[mouseOver];
    }

    static AZ::Color ViewColor(const bool mouseOver, const AZ::Color& defaultColor, const AZ::Color& mouseOverColor)
    {
        const AZStd::array<AZ::Color, 2> viewColor = { { defaultColor, mouseOverColor } };
        return viewColor[mouseOver].GetAsVector4();
    }

    auto defaultLineWidth = [](const bool mouseOver)
    {
        return LineWidth(mouseOver, 0.0f, 4.0f);
    };

    ManipulatorView::ManipulatorView() = default;

    ManipulatorView::ManipulatorView(const bool screenSizeFixed)
        : m_screenSizeFixed(screenSizeFixed)
    {
    }

    ManipulatorView::~ManipulatorView()
    {
        Invalidate(m_managerId);
    }

    void ManipulatorView::SetBoundDirty(const ManipulatorManagerId managerId)
    {
        ManipulatorManagerRequestBus::Event(managerId, &ManipulatorManagerRequestBus::Events::SetBoundDirty, m_boundId);

        m_boundDirty = true;
    }

    void ManipulatorView::RefreshBound(
        const ManipulatorManagerId managerId, const ManipulatorId manipulatorId, const Picking::BoundRequestShapeBase& bound)
    {
        ManipulatorManagerRequestBus::EventResult(
            m_boundId, managerId, &ManipulatorManagerRequestBus::Events::UpdateBound, manipulatorId, m_boundId, bound);

        // store the manager id if we know the bound has been registered
        m_managerId = managerId;
        // the bound will now be up to date
        m_boundDirty = false;
    }

    void ManipulatorView::RefreshBoundInternal(
        const ManipulatorManagerId managerId, const ManipulatorId manipulatorId, const Picking::BoundRequestShapeBase& bound)
    {
        // update the manipulator's bounds if necessary
        // if m_screenSizeFixed is true, any camera movement can potentially change the size
        // of the manipulator, so we update bounds every frame regardless until we have performance issue
        if (m_screenSizeFixed || m_boundDirty)
        {
            RefreshBound(managerId, manipulatorId, bound);
        }
    }

    void ManipulatorView::Invalidate(const ManipulatorManagerId managerId)
    {
        if (m_boundId != Picking::InvalidBoundId)
        {
            ManipulatorManagerRequestBus::Event(managerId, &ManipulatorManagerRequestBus::Events::DeleteManipulatorBound, m_boundId);

            m_boundId = Picking::InvalidBoundId;
        }
    }

    float ManipulatorView::ManipulatorViewScaleMultiplier(
        const AZ::Vector3& worldPosition, const AzFramework::CameraState& cameraState) const
    {
        return ScreenSizeFixed() ? CalculateScreenToWorldMultiplier(worldPosition, cameraState) : 1.0f;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    void ManipulatorViewQuad::Draw(
        const ManipulatorManagerId managerId,
        const ManipulatorManagerState& managerState,
        const ManipulatorId manipulatorId,
        const ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const AZ::Vector3 axis1 = m_axis1;
        const AZ::Vector3 axis2 = m_axis2;

        CameraCorrectAxis(
            axis1, m_cameraCorrectedAxis1, managerState, mouseInteraction, manipulatorState.m_worldFromLocal,
            manipulatorState.m_localPosition, cameraState);
        CameraCorrectAxis(
            axis2, m_cameraCorrectedAxis2, managerState, mouseInteraction, manipulatorState.m_worldFromLocal,
            manipulatorState.m_localPosition, cameraState);

        const Picking::BoundShapeQuad quadBound = CalculateQuadBound(
            manipulatorState.m_localPosition, manipulatorState, m_cameraCorrectedAxis1, m_cameraCorrectedAxis2,
            m_size *
                ManipulatorViewScaleMultiplier(
                    manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition), cameraState));

        debugDisplay.SetLineWidth(defaultLineWidth(manipulatorState.m_mouseOver));

        debugDisplay.SetColor(ViewColor(manipulatorState.m_mouseOver, m_axis1Color, m_mouseOverColor).GetAsVector4());
        debugDisplay.DrawLine(quadBound.m_corner4, quadBound.m_corner3);

        debugDisplay.SetColor(ViewColor(manipulatorState.m_mouseOver, m_axis2Color, m_mouseOverColor).GetAsVector4());
        debugDisplay.DrawLine(quadBound.m_corner2, quadBound.m_corner3);

        if (manipulatorState.m_mouseOver)
        {
            debugDisplay.SetColor(Vector3ToVector4(m_mouseOverColor.GetAsVector3(), 0.5f));

            debugDisplay.CullOff();
            debugDisplay.DrawQuad(quadBound.m_corner1, quadBound.m_corner2, quadBound.m_corner3, quadBound.m_corner4);
            debugDisplay.CullOn();
        }

        RefreshBoundInternal(managerId, manipulatorId, quadBound);
    }

    void ManipulatorViewQuadBillboard::Draw(
        const ManipulatorManagerId managerId,
        const ManipulatorManagerState& /*managerState*/,
        const ManipulatorId manipulatorId,
        const ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& /*mouseInteraction*/)
    {
        const Picking::BoundShapeQuad quadBound = CalculateQuadBoundBillboard(
            manipulatorState.m_localPosition, manipulatorState.m_worldFromLocal,
            m_size *
                ManipulatorViewScaleMultiplier(
                    manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition), cameraState),
            cameraState);

        debugDisplay.SetColor(ViewColor(manipulatorState.m_mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        debugDisplay.DrawQuad(quadBound.m_corner1, quadBound.m_corner2, quadBound.m_corner3, quadBound.m_corner4);

        RefreshBoundInternal(managerId, manipulatorId, quadBound);
    }

    void ManipulatorViewLine::Draw(
        const ManipulatorManagerId managerId,
        const ManipulatorManagerState& managerState,
        const ManipulatorId manipulatorId,
        const ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const float viewScale =
            ManipulatorViewScaleMultiplier(manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition), cameraState);

        CameraCorrectAxis(
            m_axis, m_cameraCorrectedAxis, managerState, mouseInteraction, manipulatorState.m_worldFromLocal,
            manipulatorState.m_localPosition, cameraState);

        const auto worldLine =
            CalculateLine(manipulatorState.m_localPosition, manipulatorState.m_worldFromLocal, m_cameraCorrectedAxis, m_length * viewScale);

        debugDisplay.SetColor(ViewColor(manipulatorState.m_mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        debugDisplay.SetLineWidth(defaultLineWidth(manipulatorState.m_mouseOver));
        debugDisplay.DrawLine(worldLine.first, worldLine.second);

        // ensure length of bound takes into account logical width of line (m_length - m_width)
        const Picking::BoundShapeLineSegment lineBound = CalculateLineBound(
            manipulatorState.m_localPosition, manipulatorState.m_worldFromLocal, m_cameraCorrectedAxis, (m_length - m_width) * viewScale,
            m_width * viewScale);

        if (ed_manipulatorDisplayBoundDebug)
        {
            debugDisplay.DrawBall(lineBound.m_start, lineBound.m_width, false);
            debugDisplay.DrawBall(lineBound.m_end, lineBound.m_width, false);

            const auto line = lineBound.m_end - lineBound.m_start;
            const auto height = line.GetLength();
            const auto axis = line / height;
            const auto center = lineBound.m_start + axis * height * 0.5f;
            debugDisplay.DrawSolidCylinder(center, axis, lineBound.m_width, height, false);
        }

        RefreshBoundInternal(managerId, manipulatorId, lineBound);
    }

    void ManipulatorViewLineSelect::Draw(
        const ManipulatorManagerId managerId,
        const ManipulatorManagerState& /*managerState*/,
        const ManipulatorId manipulatorId,
        const ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const float viewScale =
            ManipulatorViewScaleMultiplier(manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition), cameraState);

        const Picking::BoundShapeLineSegment lineBound =
            CalculateLineBound(m_localStart, m_localEnd, manipulatorState, m_width * viewScale);

        if (manipulatorState.m_mouseOver)
        {
            const LineSegmentSelectionManipulator::Action action = CalculateManipulationDataAction(
                manipulatorState.m_worldFromLocal, manipulatorState.m_nonUniformScale, mouseInteraction.m_mousePick.m_rayOrigin,
                mouseInteraction.m_mousePick.m_rayDirection, cameraState.m_farClip, m_localStart, m_localEnd);

            const AZ::Vector3 worldLineHitPosition = manipulatorState.TransformPoint(action.m_localLineHitPosition);
            debugDisplay.SetColor(AZ::Vector4(0.0f, 1.0f, 0.0f, 1.0f));
            debugDisplay.DrawBall(
                worldLineHitPosition, ManipulatorViewScaleMultiplier(worldLineHitPosition, cameraState) * g_defaultManipulatorSphereRadius,
                false);
        }

        RefreshBoundInternal(managerId, manipulatorId, lineBound);
    }

    void ManipulatorViewCone::Draw(
        const ManipulatorManagerId managerId,
        const ManipulatorManagerState& managerState,
        const ManipulatorId manipulatorId,
        const ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const float viewScale =
            ManipulatorViewScaleMultiplier(manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition), cameraState);

        CameraCorrectAxis(
            m_axis, m_cameraCorrectedAxis, managerState, mouseInteraction, manipulatorState.m_worldFromLocal,
            manipulatorState.m_localPosition, cameraState, &m_shouldCorrect);

        CameraCorrectAxis(
            m_offset, m_cameraCorrectedOffset, managerState, mouseInteraction, manipulatorState.m_worldFromLocal,
            manipulatorState.m_localPosition, cameraState);

        const Picking::BoundShapeCone coneBound = CalculateConeBound(
            manipulatorState.m_localPosition, manipulatorState.m_worldFromLocal, m_cameraCorrectedAxis, m_cameraCorrectedOffset * viewScale,
            m_length * viewScale, m_radius * viewScale);

        debugDisplay.SetColor(ViewColor(manipulatorState.m_mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        if (m_shouldCorrect)
        {
            debugDisplay.DrawWireCone(coneBound.m_base, coneBound.m_axis, coneBound.m_radius, coneBound.m_height);
        }
        else
        {
            debugDisplay.DrawSolidCone(coneBound.m_base, coneBound.m_axis, coneBound.m_radius, coneBound.m_height, false);
        }

        RefreshBoundInternal(managerId, manipulatorId, coneBound);
    }

    void ManipulatorViewBox::Draw(
        const ManipulatorManagerId managerId,
        const ManipulatorManagerState& managerState,
        const ManipulatorId manipulatorId,
        const ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const float viewScale =
            ManipulatorViewScaleMultiplier(manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition), cameraState);

        const AZ::Quaternion orientation = m_orientation;

        CameraCorrectAxis(
            m_offset, m_cameraCorrectedOffset, managerState, mouseInteraction, manipulatorState.m_worldFromLocal,
            manipulatorState.m_localPosition, cameraState);

        const Picking::BoundShapeBox boxBound = CalculateBoxBound(
            manipulatorState.m_localPosition, manipulatorState.m_worldFromLocal, orientation, m_cameraCorrectedOffset * viewScale,
            m_halfExtents * viewScale);

        const AZ::Vector3 xAxis = boxBound.m_orientation.TransformVector(AZ::Vector3::CreateAxisX());
        const AZ::Vector3 yAxis = boxBound.m_orientation.TransformVector(AZ::Vector3::CreateAxisY());
        const AZ::Vector3 zAxis = boxBound.m_orientation.TransformVector(AZ::Vector3::CreateAxisZ());

        debugDisplay.SetColor(ViewColor(manipulatorState.m_mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        debugDisplay.DrawSolidOBB(boxBound.m_center, xAxis, yAxis, zAxis, boxBound.m_halfExtents);

        RefreshBoundInternal(managerId, manipulatorId, boxBound);
    }

    void ManipulatorViewCylinder::Draw(
        const ManipulatorManagerId managerId,
        const ManipulatorManagerState& managerState,
        const ManipulatorId manipulatorId,
        const ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const float viewScale =
            ManipulatorViewScaleMultiplier(manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition), cameraState);

        CameraCorrectAxis(
            m_axis, m_cameraCorrectedAxis, managerState, mouseInteraction, manipulatorState.m_worldFromLocal,
            manipulatorState.m_localPosition, cameraState);

        const Picking::BoundShapeCylinder cylinderBound = CalculateCylinderBound(
            manipulatorState.m_localPosition, manipulatorState.m_worldFromLocal, m_cameraCorrectedAxis, m_length * viewScale,
            m_radius * viewScale);

        debugDisplay.SetColor(ViewColor(manipulatorState.m_mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        debugDisplay.DrawSolidCylinder(
            cylinderBound.m_base + cylinderBound.m_axis * cylinderBound.m_height * 0.5f, cylinderBound.m_axis, cylinderBound.m_radius,
            cylinderBound.m_height, false);

        RefreshBoundInternal(managerId, manipulatorId, cylinderBound);
    }

    void ManipulatorViewSphere::Draw(
        const ManipulatorManagerId managerId,
        const ManipulatorManagerState& /*managerState*/,
        const ManipulatorId manipulatorId,
        const ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const Picking::BoundShapeSphere sphereBound = CalculateSphereBound(
            manipulatorState.m_localPosition, manipulatorState,
            m_radius * ManipulatorViewScaleMultiplier(manipulatorState.TransformPoint(manipulatorState.m_localPosition), cameraState));

        if (m_depthTest)
        {
            debugDisplay.DepthTestOn();
        }

        debugDisplay.SetColor(m_decideColorFn(mouseInteraction, manipulatorState.m_mouseOver, m_color).GetAsVector4());
        debugDisplay.DrawBall(sphereBound.m_center, sphereBound.m_radius, false);

        if (m_depthTest)
        {
            debugDisplay.DepthTestOff();
        }

        RefreshBoundInternal(managerId, manipulatorId, sphereBound);
    }

    void ManipulatorViewCircle::Draw(
        const ManipulatorManagerId managerId,
        const ManipulatorManagerState& /*managerState*/,
        const ManipulatorId manipulatorId,
        const ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& /*mouseInteraction*/)
    {
        const float viewScale =
            ManipulatorViewScaleMultiplier(manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition), cameraState);

        const Picking::BoundShapeTorus torusBound = CalculateTorusBound(
            manipulatorState.m_localPosition, manipulatorState.m_worldFromLocal, m_axis, m_radius * viewScale, m_width * viewScale);

        const AZ::Transform orientation =
            AZ::Transform::CreateFromQuaternion((QuaternionFromTransformNoScaling(manipulatorState.m_worldFromLocal) *
                                                 AZ::Quaternion::CreateShortestArc(AZ::Vector3::CreateAxisZ(), m_axis))
                                                    .GetNormalized());

        // transform circle based on delta between default z up axis and other axes
        const AZ::Transform worldFromLocalWithOrientation =
            AZ::Transform::CreateTranslation(manipulatorState.m_worldFromLocal.GetTranslation()) * orientation;

        if (ed_manipulatorDisplayBoundDebug)
        {
            debugDisplay.SetColor(AZ::Colors::BlanchedAlmond);
            debugDisplay.PushMatrix(orientation);
            debugDisplay.DrawCircle(torusBound.m_center + AZ::Vector3::CreateAxisZ() * torusBound.m_minorRadius, torusBound.m_majorRadius);
            debugDisplay.DrawCircle(
                torusBound.m_center + AZ::Vector3::CreateAxisZ() * torusBound.m_minorRadius,
                torusBound.m_majorRadius + torusBound.m_minorRadius);
            debugDisplay.DrawCircle(torusBound.m_center - AZ::Vector3::CreateAxisZ() * torusBound.m_minorRadius, torusBound.m_majorRadius);
            debugDisplay.DrawCircle(
                torusBound.m_center - AZ::Vector3::CreateAxisZ() * torusBound.m_minorRadius,
                torusBound.m_majorRadius + torusBound.m_minorRadius);
            debugDisplay.PopMatrix();
        }

        debugDisplay.CullOn();
        debugDisplay.PushMatrix(worldFromLocalWithOrientation);
        debugDisplay.SetColor(ViewColor(manipulatorState.m_mouseOver, m_color, m_mouseOverColor).GetAsVector4());
        m_drawCircleFunc(
            debugDisplay, manipulatorState.m_localPosition, torusBound.m_majorRadius,
            worldFromLocalWithOrientation.GetInverse().TransformPoint(cameraState.m_position));
        debugDisplay.PopMatrix();
        debugDisplay.CullOff();

        RefreshBoundInternal(managerId, manipulatorId, torusBound);
    }

    void DrawHalfDottedCircle(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Vector3& position, const float radius, const AZ::Vector3& viewPos)
    {
        debugDisplay.DrawHalfDottedCircle(position, radius, viewPos);
    }

    void DrawFullCircle(
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AZ::Vector3& position,
        const float radius,
        [[maybe_unused]] const AZ::Vector3& viewPos)
    {
        debugDisplay.DrawCircle(position, radius);
    }

    void ManipulatorViewSplineSelect::Draw(
        const ManipulatorManagerId managerId,
        const ManipulatorManagerState& /*managerState*/,
        const ManipulatorId manipulatorId,
        const ManipulatorState& manipulatorState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const float viewScale =
            ManipulatorViewScaleMultiplier(manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition), cameraState);

        const Picking::BoundShapeSpline splineBound =
            CalculateSplineBound(m_spline, manipulatorState.m_worldFromLocal, m_width * viewScale);

        if (manipulatorState.m_mouseOver)
        {
            const SplineSelectionManipulator::Action action = CalculateManipulationDataAction(
                manipulatorState.m_worldFromLocal, mouseInteraction.m_mousePick.m_rayOrigin, mouseInteraction.m_mousePick.m_rayDirection,
                m_spline);

            const AZ::Vector3 worldSplineHitPosition = manipulatorState.m_worldFromLocal.TransformPoint(action.m_localSplineHitPosition);

            debugDisplay.SetColor(m_color.GetAsVector4());
            debugDisplay.DrawBall(
                worldSplineHitPosition,
                ManipulatorViewScaleMultiplier(worldSplineHitPosition, cameraState) * g_defaultManipulatorSphereRadius, false);
        }

        RefreshBoundInternal(managerId, manipulatorId, splineBound);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    AZStd::unique_ptr<ManipulatorViewQuad> CreateManipulatorViewQuad(
        const PlanarManipulator& planarManipulator, const AZ::Color& axis1Color, const AZ::Color& axis2Color, const float size)
    {
        AZStd::unique_ptr<ManipulatorViewQuad> viewQuad = AZStd::make_unique<ManipulatorViewQuad>();
        viewQuad->m_axis1 = planarManipulator.GetAxis1();
        viewQuad->m_axis2 = planarManipulator.GetAxis2();
        viewQuad->m_size = size;
        viewQuad->m_axis1Color = axis1Color;
        viewQuad->m_axis2Color = axis2Color;
        return viewQuad;
    }

    AZStd::unique_ptr<ManipulatorViewQuadBillboard> CreateManipulatorViewQuadBillboard(const AZ::Color& color, const float size)
    {
        AZStd::unique_ptr<ManipulatorViewQuadBillboard> viewQuad = AZStd::make_unique<ManipulatorViewQuadBillboard>();
        viewQuad->m_size = size;
        viewQuad->m_color = color;
        return viewQuad;
    }

    AZStd::unique_ptr<ManipulatorViewLine> CreateManipulatorViewLine(
        const LinearManipulator& linearManipulator, const AZ::Color& color, const float length, const float width)
    {
        AZStd::unique_ptr<ManipulatorViewLine> viewLine = AZStd::make_unique<ManipulatorViewLine>();
        viewLine->m_axis = linearManipulator.GetAxis();
        viewLine->m_length = length;
        viewLine->m_width = width;
        viewLine->m_color = color;
        return viewLine;
    }

    AZStd::unique_ptr<ManipulatorViewLineSelect> CreateManipulatorViewLineSelect(
        const LineSegmentSelectionManipulator& lineSegmentManipulator, const AZ::Color& color, const float width)
    {
        AZStd::unique_ptr<ManipulatorViewLineSelect> viewLineSelect = AZStd::make_unique<ManipulatorViewLineSelect>();
        viewLineSelect->m_localStart = lineSegmentManipulator.GetStart();
        viewLineSelect->m_localEnd = lineSegmentManipulator.GetEnd();
        viewLineSelect->m_width = width;
        viewLineSelect->m_color = color;
        return viewLineSelect;
    }

    AZStd::unique_ptr<ManipulatorViewCone> CreateManipulatorViewCone(
        const LinearManipulator& linearManipulator,
        const AZ::Color& color,
        const AZ::Vector3& offset,
        const float length,
        const float radius)
    {
        AZStd::unique_ptr<ManipulatorViewCone> viewCone = AZStd::make_unique<ManipulatorViewCone>();
        viewCone->m_axis = linearManipulator.GetAxis();
        viewCone->m_length = length;
        viewCone->m_radius = radius;
        viewCone->m_offset = offset;
        viewCone->m_color = color;
        return viewCone;
    }

    AZStd::unique_ptr<ManipulatorViewBox> CreateManipulatorViewBox(
        const AZ::Transform& transform, const AZ::Color& color, const AZ::Vector3& offset, const AZ::Vector3& halfExtents)
    {
        AZStd::unique_ptr<ManipulatorViewBox> viewBox = AZStd::make_unique<ManipulatorViewBox>();
        viewBox->m_orientation = transform.GetRotation();
        viewBox->m_halfExtents = halfExtents;
        viewBox->m_offset = offset;
        viewBox->m_color = color;
        return viewBox;
    }

    AZStd::unique_ptr<ManipulatorViewCylinder> CreateManipulatorViewCylinder(
        const LinearManipulator& linearManipulator, const AZ::Color& color, const float length, const float radius)
    {
        AZStd::unique_ptr<ManipulatorViewCylinder> viewCylinder = AZStd::make_unique<ManipulatorViewCylinder>();
        viewCylinder->m_axis = linearManipulator.GetAxis();
        viewCylinder->m_radius = radius;
        viewCylinder->m_length = length;
        viewCylinder->m_color = color;
        return viewCylinder;
    }

    AZStd::unique_ptr<ManipulatorViewSphere> CreateManipulatorViewSphere(
        const AZ::Color& color, const float radius, const DecideColorFn& decideColor, bool enableDepthTest)
    {
        AZStd::unique_ptr<ManipulatorViewSphere> viewSphere = AZStd::make_unique<ManipulatorViewSphere>();
        viewSphere->m_radius = radius;
        viewSphere->m_color = color;
        viewSphere->m_decideColorFn = decideColor;
        viewSphere->m_depthTest = enableDepthTest;
        return viewSphere;
    }

    AZStd::unique_ptr<ManipulatorViewCircle> CreateManipulatorViewCircle(
        const AngularManipulator& angularManipulator,
        const AZ::Color& color,
        const float radius,
        const float width,
        const ManipulatorViewCircle::DrawCircleFunc drawFunc)
    {
        AZStd::unique_ptr<ManipulatorViewCircle> viewCircle = AZStd::make_unique<ManipulatorViewCircle>();
        viewCircle->m_axis = angularManipulator.GetAxis();
        viewCircle->m_color = color;
        viewCircle->m_radius = radius;
        viewCircle->m_width = width;
        viewCircle->m_drawCircleFunc = drawFunc;
        return viewCircle;
    }

    AZStd::unique_ptr<ManipulatorViewSplineSelect> CreateManipulatorViewSplineSelect(
        const SplineSelectionManipulator& splineManipulator, const AZ::Color& color, const float width)
    {
        AZStd::unique_ptr<ManipulatorViewSplineSelect> viewSplineSelect = AZStd::make_unique<ManipulatorViewSplineSelect>();
        viewSplineSelect->m_spline = splineManipulator.GetSpline();
        viewSplineSelect->m_color = color;
        viewSplineSelect->m_width = width;
        return viewSplineSelect;
    }

    AZ::Vector3 CalculateViewDirection(const Manipulators& manipulators, const AZ::Vector3& worldViewPosition)
    {
        const AZ::Transform worldFromLocalWithTransform = manipulators.GetSpace() * manipulators.GetLocalTransform();

        AZ::Vector3 lookDirection = (worldFromLocalWithTransform.GetTranslation() - worldViewPosition).GetNormalized();

        return TransformDirectionNoScaling(worldFromLocalWithTransform.GetInverse(), lookDirection);
    }
} // namespace AzToolsFramework
