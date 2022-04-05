/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AngularManipulator.h"

#include <AzCore/Math/Plane.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorDebug.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

#pragma optimize("", off)
#pragma inline_depth(0)

namespace AzToolsFramework
{
    static const float CircularRotateThresholdDegrees = 80.0f;

    AngularManipulator::ActionInternal AngularManipulator::CalculateManipulationDataStart(
        const Fixed& fixed,
        const AZ::Transform& worldFromLocal,
        const AZ::Transform& localTransform,
        const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection,
        const float rayDistance)
    {
        const AZ::Transform worldFromLocalWithTransform = worldFromLocal * localTransform;
        const AZ::Vector3 worldAxis = TransformDirectionNoScaling(worldFromLocalWithTransform, fixed.m_axis);

        ActionInternal actionInternal;
        // default plane normal and point used for rotation
        actionInternal.m_start.m_planeNormal = worldAxis;
        actionInternal.m_start.m_planePoint = worldFromLocalWithTransform.GetTranslation();

        // if angular manipulator axis is at right angles to us, use initial ray direction
        // as plane normal and use hit position on manipulator as plane point
        const float pickAngle = AZ::RadToDeg(AZ::Acos(AZ::Abs(rayDirection.Dot(worldAxis))));
        if (pickAngle > CircularRotateThresholdDegrees)
        {
            actionInternal.m_start.m_planeNormal = -rayDirection;
            actionInternal.m_start.m_planePoint = rayOrigin + rayDirection * rayDistance;
        }

        // store initial world hit position
        Internal::CalculateRayPlaneIntersectingPoint(
            rayOrigin, rayDirection, actionInternal.m_start.m_planePoint, actionInternal.m_start.m_planeNormal,
            actionInternal.m_start.m_worldHitPosition);

        // store entity transform (to go from local to world space)
        // and store our own starting local transform
        actionInternal.m_start.m_worldFromLocal = worldFromLocal;
        actionInternal.m_start.m_localTransform = localTransform;
        actionInternal.m_current.m_radians = 0.0f;
        actionInternal.m_current.m_worldHitPosition = actionInternal.m_start.m_worldHitPosition;

        return actionInternal;
    }

    AngularManipulator::Action AngularManipulator::CalculateManipulationDataAction(
        const Fixed& fixed,
        ActionInternal& actionInternal,
        const AZ::Transform& worldFromLocal,
        const AZ::Transform& localTransform,
        const bool snapping,
        const float angleStepDegrees,
        const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection,
        const ViewportInteraction::KeyboardModifiers keyboardModifiers)
    {
        const AZ::Transform worldFromLocalWithTransform = worldFromLocal * localTransform;
        const AZ::Vector3 worldAxis = TransformDirectionNoScaling(worldFromLocalWithTransform, fixed.m_axis);

        AZ::Vector3 worldHitPosition = actionInternal.m_start.m_worldHitPosition;
        Internal::CalculateRayPlaneIntersectingPoint(
            rayOrigin, rayDirection, actionInternal.m_start.m_planePoint, actionInternal.m_start.m_planeNormal, worldHitPosition);

        // get vector from center of rotation for current and previous frame
        const AZ::Vector3 center = worldFromLocalWithTransform.GetTranslation();
        const AZ::Vector3 currentWorldHitVector = (worldHitPosition - center).GetNormalizedSafe();
        const AZ::Vector3 previousWorldHitVector = (actionInternal.m_current.m_worldHitPosition - center).GetNormalizedSafe();

        // calculate which direction we rotated
        const AZ::Vector3 worldAxisRight = worldAxis.Cross(previousWorldHitVector);
        const float rotateSign = Sign(currentWorldHitVector.Dot(worldAxisRight));
        // how far did we rotate this frame
        const float rotationAngleRad = AZ::Acos(AZ::GetMin<float>(1.0f, currentWorldHitVector.Dot(previousWorldHitVector)));
        actionInternal.m_current.m_worldHitPosition = worldHitPosition;

        // if we're snapping, only increment current radians when we know
        // preSnapRadians is greater than the angleStep
        if (snapping && AZStd::abs(angleStepDegrees) > 0.0f)
        {
            actionInternal.m_current.m_preSnapRadians += rotationAngleRad * rotateSign;

            const float angleStepRad = AZ::DegToRad(angleStepDegrees);
            const float preSnapRotateSign = Sign(actionInternal.m_current.m_preSnapRadians);
            // if we move more than angleStep in a frame, make sure we catch up
            while (AZStd::abs(actionInternal.m_current.m_preSnapRadians) >= angleStepRad)
            {
                actionInternal.m_current.m_radians += angleStepRad * preSnapRotateSign;
                actionInternal.m_current.m_preSnapRadians -= angleStepRad * preSnapRotateSign;
            }
        }
        else
        {
            // no snapping, just update current radius immediately
            actionInternal.m_current.m_radians += rotationAngleRad * rotateSign;
        }

        Action action;
        action.m_start.m_space = actionInternal.m_start.m_worldFromLocal.GetRotation().GetNormalized();
        action.m_start.m_rotation = actionInternal.m_start.m_localTransform.GetRotation().GetNormalized();
        action.m_start.m_worldHitPosition = actionInternal.m_start.m_worldHitPosition;
        action.m_current.m_delta = AZ::Quaternion::CreateFromAxisAngle(fixed.m_axis, actionInternal.m_current.m_radians).GetNormalized();
        action.m_current.m_worldHitPosition = actionInternal.m_current.m_worldHitPosition;
        action.m_modifiers = keyboardModifiers;

        return action;
    }

    AZStd::shared_ptr<AngularManipulator> AngularManipulator::MakeShared(const AZ::Transform& worldFromLocal)
    {
        return AZStd::shared_ptr<AngularManipulator>(aznew AngularManipulator(worldFromLocal));
    }

    AngularManipulator::AngularManipulator(const AZ::Transform& worldFromLocal)
    {
        SetSpace(worldFromLocal);
        AttachLeftMouseDownImpl();
    }

    void AngularManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void AngularManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void AngularManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void AngularManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, const float rayIntersectionDistance)
    {
        const bool snapping = AngleSnapping(interaction.m_interactionId.m_viewportId);
        const float angleStep = AngleStep(interaction.m_interactionId.m_viewportId);

        // calculate initial state when mouse press first happens
        m_actionInternal = CalculateManipulationDataStart(
            m_fixed, TransformNormalizedScale(GetSpace()), TransformNormalizedScale(GetLocalTransform()),
            interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, rayIntersectionDistance);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_fixed, m_actionInternal, m_actionInternal.m_start.m_worldFromLocal, m_actionInternal.m_start.m_localTransform, snapping,
                angleStep, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, interaction.m_keyboardModifiers));
        }
    }

    void AngularManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            // calculate delta rotation
            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_fixed, m_actionInternal, m_actionInternal.m_start.m_worldFromLocal, m_actionInternal.m_start.m_localTransform,
                AngleSnapping(interaction.m_interactionId.m_viewportId), AngleStep(interaction.m_interactionId.m_viewportId),
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, interaction.m_keyboardModifiers));
        }
    }

    void AngularManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_fixed, m_actionInternal, m_actionInternal.m_start.m_worldFromLocal, m_actionInternal.m_start.m_localTransform,
                AngleSnapping(interaction.m_interactionId.m_viewportId), AngleStep(interaction.m_interactionId.m_viewportId),
                interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, interaction.m_keyboardModifiers));
        }
    }

    void AngularManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        [[maybe_unused]] auto action = CalculateManipulationDataAction(
            m_fixed, m_actionInternal, m_actionInternal.m_start.m_worldFromLocal, m_actionInternal.m_start.m_localTransform,
            AngleSnapping(mouseInteraction.m_interactionId.m_viewportId), AngleStep(mouseInteraction.m_interactionId.m_viewportId),
            mouseInteraction.m_mousePick.m_rayOrigin, mouseInteraction.m_mousePick.m_rayDirection, mouseInteraction.m_keyboardModifiers);

        const auto manipulatorState =
            ManipulatorState{ ApplySpace(GetLocalTransform()), GetNonUniformScale(), AZ::Vector3::CreateZero(), MouseOver() };

        m_manipulatorView->Draw(
            GetManipulatorManagerId(), managerState, GetManipulatorId(), manipulatorState, debugDisplay, cameraState, mouseInteraction);

        const auto worldPosition = manipulatorState.m_worldFromLocal.TransformPoint(manipulatorState.m_localPosition);
        const auto localFromWorld = manipulatorState.m_worldFromLocal.GetInverse();
        const auto initialLocalHitPosition = localFromWorld.TransformPoint(action.m_start.m_worldHitPosition);
        const auto currentLocalHitPosition = localFromWorld.TransformPoint(action.m_current.m_worldHitPosition);

        const auto plane = AZ::Plane::CreateFromNormalAndPoint(m_fixed.m_axis, localFromWorld.GetTranslation());
        const auto initialPointOnPlane = plane.GetProjected(initialLocalHitPosition);
        const auto currentPointOnPlane = plane.GetProjected(currentLocalHitPosition);

        const float viewScale = CalculateScreenToWorldMultiplier(worldPosition, cameraState);

        const AZ::Transform orientation =
            AZ::Transform::CreateFromQuaternion((QuaternionFromTransformNoScaling(manipulatorState.m_worldFromLocal) *
                                                 AZ::Quaternion::CreateShortestArc(m_fixed.m_axis, GetAxis()))
                                                    .GetNormalized());

        // transform circle based on delta between default z up axis and other axes
        const AZ::Transform worldFromLocalWithOrientation =
            AZ::Transform::CreateTranslation(manipulatorState.m_worldFromLocal.GetTranslation()) * orientation;

        if (PerformingAction())
        {
            if (ed_manipulatorDrawDebug)
            {
                // display the exact hit (ray intersection) of the mouse pick on the manipulator
                // DrawTransformAxes(debugDisplay, GetSpace() * AZ::Transform::CreateTranslation(action.m_start.m_worldHitPosition));

                DrawTransformAxes(
                    debugDisplay,
                    GetSpace() * AZ::Transform::CreateTranslation(manipulatorState.m_worldFromLocal.TransformPoint(initialPointOnPlane)));
            }

            const auto initialPointToCenter = (initialPointOnPlane - manipulatorState.m_localPosition).GetNormalized();
            const auto currentPointToCenter = (currentPointOnPlane - manipulatorState.m_localPosition).GetNormalized();

            debugDisplay.CullOn();
            debugDisplay.PushMatrix(worldFromLocalWithOrientation);
            debugDisplay.SetColor(AZ::Colors::CornflowerBlue);

            const auto totalAngle = AZ::DegToRad(360.0f);
            const auto stepIncrement = totalAngle / 360.0f;

            const auto right =
                AZ::Quaternion::CreateFromAxisAngle(m_fixed.m_axis, AZ::DegToRad(90.0f)).TransformVector(initialPointToCenter);

            const auto angle = action.m_current.m_delta.GetAngle();
            const auto sign = Sign(currentPointToCenter.Dot(right));
            const auto rotationDirection = angle < AZ::Constants::Pi ? sign : -sign;

            const auto angleStr = AZStd::string::format("Angle %f", angle);
            debugDisplay.Draw2dTextLabel(100, 100, 1.0f, angleStr.c_str());

            for (auto step = 0.0f; step < angle; step += stepIncrement)
            {
                const auto first =
                    AZ::Quaternion::CreateFromAxisAngle(m_fixed.m_axis, step * rotationDirection).TransformVector(initialPointToCenter);
                const auto second = AZ::Quaternion::CreateFromAxisAngle(m_fixed.m_axis, (step - stepIncrement) * rotationDirection)
                                        .TransformVector(initialPointToCenter);

                debugDisplay.DrawTri(
                    manipulatorState.m_localPosition, manipulatorState.m_localPosition + first * 2.0f * viewScale,
                    manipulatorState.m_localPosition + second * 2.0f * viewScale);
            }

            debugDisplay.PopMatrix();
            debugDisplay.CullOff();
        }

        // annotation drawing
    }

    void AngularManipulator::SetAxis(const AZ::Vector3& axis)
    {
        m_fixed.m_axis = axis;
    }

    void AngularManipulator::SetView(AZStd::unique_ptr<ManipulatorView>&& view)
    {
        m_manipulatorView = AZStd::move(view);
    }

    void AngularManipulator::SetBoundsDirtyImpl()
    {
        m_manipulatorView->SetBoundDirty(GetManipulatorManagerId());
    }

    void AngularManipulator::InvalidateImpl()
    {
        m_manipulatorView->Invalidate(GetManipulatorManagerId());
    }

} // namespace AzToolsFramework

#pragma optimize("", on)
#pragma inline_depth()
