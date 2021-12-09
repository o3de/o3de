/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LinearManipulator.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorDebug.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    LinearManipulator::Starter CalculateLinearManipulationDataStart(
        const LinearManipulator::Fixed& fixed,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& nonUniformScale,
        const AZ::Transform& localTransform,
        const ViewportInteraction::MouseInteraction& interaction,
        const float intersectionDistance,
        const AzFramework::CameraState& cameraState)
    {
        const ManipulatorInteraction manipulatorInteraction = BuildManipulatorInteraction(
            worldFromLocal, nonUniformScale, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection);

        const AZ::Vector3 axis = TransformDirectionNoScaling(localTransform, fixed.m_axis);
        const AZ::Vector3 rayCrossAxis = manipulatorInteraction.m_localRayDirection.Cross(axis);

        LinearManipulator::Start start;
        LinearManipulator::StartTransition startTransition;
        // initialize m_localHitPosition to handle edge case where CalculateRayPlaneIntersectingPoint
        // fails because ray is parallel to the plane
        // localTransform is in the reference frame of the object being manipulated (i.e. the world rotation, translation and uniform scale
        // from the world transform have been extracted), but non-uniform scale has to be accounted for separately
        start.m_localHitPosition = nonUniformScale * localTransform.GetTranslation();
        startTransition.m_localNormal = rayCrossAxis.Cross(axis).GetNormalizedSafe();

        // initial intersect point
        const AZ::Vector3 localIntersectionPoint =
            manipulatorInteraction.m_localRayOrigin + manipulatorInteraction.m_localRayDirection * intersectionDistance;

        Internal::CalculateRayPlaneIntersectingPoint(
            manipulatorInteraction.m_localRayOrigin, manipulatorInteraction.m_localRayDirection, localIntersectionPoint,
            startTransition.m_localNormal, start.m_localHitPosition);

        start.m_screenPosition = interaction.m_mousePick.m_screenCoordinates;
        start.m_localPosition = localTransform.GetTranslation();
        start.m_localScale = AZ::Vector3(localTransform.GetUniformScale());
        ;
        start.m_localAxis = axis;
        // sign to determine which side of the linear axis we pressed
        // (useful to know when the visual axis flips to face the camera)
        start.m_sign = AZ::GetSign((start.m_localHitPosition - localTransform.GetTranslation()).Dot(axis));

        startTransition.m_screenToWorldScale =
            1.0f / CalculateScreenToWorldMultiplier((worldFromLocal * localTransform).GetTranslation(), cameraState);

        return { startTransition, start };
    }

    LinearManipulator::Action CalculateLinearManipulationDataAction(
        const LinearManipulator::Fixed& fixed,
        const LinearManipulator::Starter& starter,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& nonUniformScale,
        const AZ::Transform& localTransform,
        const GridSnapParameters& gridSnapParams,
        const ViewportInteraction::MouseInteraction& interaction)
    {
        const ManipulatorInteraction manipulatorInteraction = BuildManipulatorInteraction(
            worldFromLocal, nonUniformScale, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection);

        const auto& [startTransition, start] = starter;

        // as CalculateRayPlaneIntersectingPoint may fail, ensure localHitPosition is initialized with
        // the starting hit position so the manipulator returns to the original location it was pressed
        // if an invalid ray intersection is attempted
        AZ::Vector3 localHitPosition = start.m_localHitPosition;
        Internal::CalculateRayPlaneIntersectingPoint(
            manipulatorInteraction.m_localRayOrigin, manipulatorInteraction.m_localRayDirection, start.m_localHitPosition,
            startTransition.m_localNormal, localHitPosition);

        localHitPosition = Internal::TryConstrainHitPositionToView(
            localHitPosition, start.m_localHitPosition, worldFromLocal.GetInverse(),
            GetCameraState(interaction.m_interactionId.m_viewportId));

        const AZ::Vector3 axis = TransformDirectionNoScaling(localTransform, fixed.m_axis);
        // the local positions have been transformed to the reference frame of the object being manipulated, but they appear in the world
        // with non-uniform scale applied, the object being manipulated will want to work with unscaled deltas, so we need to divide by
        // the non-uniform scale here
        const AZ::Vector3 hitDelta = (localHitPosition - start.m_localHitPosition) / nonUniformScale;
        const AZ::Vector3 unsnappedOffset = axis * axis.Dot(hitDelta);

        const float scaleRecip =
            manipulatorInteraction.m_scaleReciprocal * fixed.m_axis.Dot(manipulatorInteraction.m_nonUniformScaleReciprocal);
        const float gridSize = gridSnapParams.m_gridSize;
        const bool snapping = gridSnapParams.m_gridSnap;

        LinearManipulator::Action action;
        action.m_fixed = fixed;
        action.m_start = start;
        action.m_current.m_localPositionOffset =
            snapping ? CalculateSnappedAmount(unsnappedOffset, axis, gridSize * scaleRecip) : unsnappedOffset;
        action.m_current.m_screenPosition = interaction.m_mousePick.m_screenCoordinates;
        action.m_viewportId = interaction.m_interactionId.m_viewportId;

        const AZ::Quaternion localRotation = QuaternionFromTransformNoScaling(localTransform);
        const AZ::Vector3 scaledUnsnappedOffset =
            unsnappedOffset * startTransition.m_screenToWorldScale * NonUniformScaleReciprocal(nonUniformScale);

        // how much to adjust the scale based on movement
        const AZ::Quaternion invLocalRotation = localRotation.GetInverseFull();
        action.m_current.m_localScaleOffset = snapping
            ? invLocalRotation.TransformVector(CalculateSnappedAmount(scaledUnsnappedOffset, axis, gridSize * scaleRecip))
            : invLocalRotation.TransformVector(scaledUnsnappedOffset);

        // record what modifier keys are held during this action
        action.m_modifiers = interaction.m_keyboardModifiers;

        return action;
    }

    AZStd::shared_ptr<LinearManipulator> LinearManipulator::MakeShared(const AZ::Transform& worldFromLocal)
    {
        return AZStd::shared_ptr<LinearManipulator>(aznew LinearManipulator(worldFromLocal));
    }

    LinearManipulator::LinearManipulator(const AZ::Transform& worldFromLocal)
    {
        SetSpace(worldFromLocal);
        AttachLeftMouseDownImpl();
    }

    void LinearManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void LinearManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void LinearManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void LinearManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, const float rayIntersectionDistance)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(GetSpace());

        // note: m_localTransform must not be made uniform as it may contain a local scale we want to snap
        m_starter = CalculateLinearManipulationDataStart(
            m_fixed, worldFromLocalUniformScale, GetNonUniformScale(), GetLocalTransform(), interaction, rayIntersectionDistance,
            GetCameraState(interaction.m_interactionId.m_viewportId));

        if (m_onLeftMouseDownCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            m_onLeftMouseDownCallback(CalculateLinearManipulationDataAction(
                m_fixed, m_starter, worldFromLocalUniformScale, GetNonUniformScale(), GetLocalTransform(), gridSnapParams, interaction));
        }
    }

    void LinearManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            // note: m_localTransform must not be made uniform as it may contain a local scale we want to snap
            m_onMouseMoveCallback(CalculateLinearManipulationDataAction(
                m_fixed, m_starter, TransformUniformScale(GetSpace()), GetNonUniformScale(), GetLocalTransform(), gridSnapParams,
                interaction));
        }
    }

    void LinearManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            // note: m_localTransform must not be made uniform as it may contain a local scale we want to snap
            m_onLeftMouseUpCallback(CalculateLinearManipulationDataAction(
                m_fixed, m_starter, TransformUniformScale(GetSpace()), GetNonUniformScale(), GetLocalTransform(), gridSnapParams,
                interaction));
        }
    }

    void LinearManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        const AZ::Transform localTransform = m_useVisualsOverride
            ? AZ::Transform::CreateFromQuaternionAndTranslation(m_visualOrientationOverride, GetLocalPosition())
            : GetLocalTransform();

        if (ed_manipulatorDrawDebug)
        {
            if (PerformingAction())
            {
                const GridSnapParameters gridSnapParams = GridSnapSettings(mouseInteraction.m_interactionId.m_viewportId);

                const auto action = CalculateLinearManipulationDataAction(
                    m_fixed, m_starter, TransformUniformScale(GetSpace()), GetNonUniformScale(), GetLocalTransform(), gridSnapParams,
                    mouseInteraction);

                // display the exact hit (ray intersection) of the mouse pick on the manipulator
                DrawTransformAxes(
                    debugDisplay,
                    TransformUniformScale(GetSpace()) *
                        AZ::Transform::CreateTranslation(
                            action.m_start.m_localHitPosition + GetNonUniformScale() * action.m_current.m_localPositionOffset));
            }

            AZ::Transform combined = GetLocalTransform();
            combined.SetTranslation(GetNonUniformScale() * combined.GetTranslation());
            combined = GetSpace() * combined;

            DrawTransformAxes(debugDisplay, combined);
            DrawAxis(debugDisplay, combined.GetTranslation(), TransformDirectionNoScaling(combined, m_fixed.m_axis));
        }

        for (auto& view : m_manipulatorViews)
        {
            auto nonUniformScale = GetNonUniformScale();

            view->Draw(
                GetManipulatorManagerId(), managerState, GetManipulatorId(),
                ManipulatorState{ ApplySpace(localTransform), GetNonUniformScale(), AZ::Vector3::CreateZero(), MouseOver() }, debugDisplay,
                cameraState, mouseInteraction);
        }
    }

    void LinearManipulator::SetAxis(const AZ::Vector3& axis)
    {
        m_fixed.m_axis = axis;
    }

    void LinearManipulator::InvalidateImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->Invalidate(GetManipulatorManagerId());
        }
    }

    void LinearManipulator::SetBoundsDirtyImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->SetBoundDirty(GetManipulatorManagerId());
        }
    }
} // namespace AzToolsFramework
