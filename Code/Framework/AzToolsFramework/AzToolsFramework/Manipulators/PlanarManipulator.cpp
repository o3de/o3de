/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PlanarManipulator.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorDebug.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    PlanarManipulator::StartInternal PlanarManipulator::CalculateManipulationDataStart(
        const Fixed& fixed,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& nonUniformScale,
        const AZ::Transform& localTransform,
        const ViewportInteraction::MouseInteraction& interaction,
        const float intersectionDistance)
    {
        const ManipulatorInteraction manipulatorInteraction = BuildManipulatorInteraction(
            worldFromLocal, nonUniformScale, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection);

        const AZ::Vector3 normal = TransformDirectionNoScaling(localTransform, fixed.m_normal);

        // initial intersect point
        const AZ::Vector3 localIntersectionPoint =
            manipulatorInteraction.m_localRayOrigin + manipulatorInteraction.m_localRayDirection * intersectionDistance;

        StartInternal startInternal;
        Internal::CalculateRayPlaneIntersectingPoint(
            manipulatorInteraction.m_localRayOrigin, manipulatorInteraction.m_localRayDirection, localIntersectionPoint, normal,
            startInternal.m_localHitPosition);

        startInternal.m_localPosition = localTransform.GetTranslation();

        return startInternal;
    }

    PlanarManipulator::Action PlanarManipulator::CalculateManipulationDataAction(
        const Fixed& fixed,
        const StartInternal& startInternal,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& nonUniformScale,
        const AZ::Transform& localTransform,
        const GridSnapParameters& gridSnapParams,
        const ViewportInteraction::MouseInteraction& interaction)
    {
        const ManipulatorInteraction manipulatorInteraction = BuildManipulatorInteraction(
            worldFromLocal, nonUniformScale, interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection);

        const AZ::Vector3 normal = TransformDirectionNoScaling(localTransform, fixed.m_normal);

        // as CalculateRayPlaneIntersectingPoint may fail, ensure localHitPosition is initialized with
        // the starting hit position so the manipulator returns to the original location it was pressed
        // if an invalid ray intersection is attempted
        AZ::Vector3 localHitPosition = startInternal.m_localHitPosition;
        Internal::CalculateRayPlaneIntersectingPoint(
            manipulatorInteraction.m_localRayOrigin, manipulatorInteraction.m_localRayDirection, startInternal.m_localHitPosition, normal,
            localHitPosition);

        localHitPosition = Internal::TryConstrainHitPositionToView(
            localHitPosition, startInternal.m_localHitPosition, worldFromLocal.GetInverse(),
            GetCameraState(interaction.m_interactionId.m_viewportId));

        const AZ::Vector3 axis1 = TransformDirectionNoScaling(localTransform, fixed.m_axis1);
        const AZ::Vector3 axis2 = TransformDirectionNoScaling(localTransform, fixed.m_axis2);

        const AZ::Vector3 hitDelta = (localHitPosition - startInternal.m_localHitPosition) / nonUniformScale;
        const AZ::Vector3 unsnappedOffset = axis1.Dot(hitDelta) * axis1 + axis2.Dot(hitDelta) * axis2;

        const AZ::Vector3 nonUniformScaleRecip = manipulatorInteraction.m_nonUniformScaleReciprocal;
        const float scaleRecip = manipulatorInteraction.m_scaleReciprocal;
        const float gridSize = gridSnapParams.m_gridSize;
        const bool snapping = gridSnapParams.m_gridSnap;

        Action action;
        action.m_fixed = fixed;
        action.m_start.m_localPosition = startInternal.m_localPosition;
        action.m_start.m_localHitPosition = startInternal.m_localHitPosition;
        action.m_current.m_localOffset = snapping
            ? CalculateSnappedAmount(unsnappedOffset, axis1, gridSize * scaleRecip * nonUniformScaleRecip.Dot(fixed.m_axis1)) +
                CalculateSnappedAmount(unsnappedOffset, axis2, gridSize * scaleRecip * nonUniformScaleRecip.Dot(fixed.m_axis2))
            : unsnappedOffset;

        // record what modifier keys are held during this action
        action.m_modifiers = interaction.m_keyboardModifiers;

        return action;
    }

    AZStd::shared_ptr<PlanarManipulator> PlanarManipulator::MakeShared(const AZ::Transform& worldFromLocal)
    {
        return AZStd::shared_ptr<PlanarManipulator>(aznew PlanarManipulator(worldFromLocal));
    }

    PlanarManipulator::PlanarManipulator(const AZ::Transform& worldFromLocal)
    {
        SetSpace(worldFromLocal);
        AttachLeftMouseDownImpl();
    }

    void PlanarManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void PlanarManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void PlanarManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void PlanarManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, const float rayIntersectionDistance)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(GetSpace());

        m_startInternal = CalculateManipulationDataStart(
            m_fixed, worldFromLocalUniformScale, GetNonUniformScale(), TransformNormalizedScale(GetLocalTransform()), interaction,
            rayIntersectionDistance);

        if (m_onLeftMouseDownCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, worldFromLocalUniformScale, GetNonUniformScale(), TransformNormalizedScale(GetLocalTransform()),
                gridSnapParams, interaction));
        }
    }

    void PlanarManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, TransformUniformScale(GetSpace()), GetNonUniformScale(),
                TransformNormalizedScale(GetLocalTransform()), gridSnapParams, interaction));
        }
    }

    void PlanarManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_fixed, m_startInternal, TransformUniformScale(GetSpace()), GetNonUniformScale(),
                TransformNormalizedScale(GetLocalTransform()), gridSnapParams, interaction));
        }
    }

    void PlanarManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        if (ed_manipulatorDrawDebug)
        {
            if (PerformingAction())
            {
                const GridSnapParameters gridSnapParams = GridSnapSettings(mouseInteraction.m_interactionId.m_viewportId);
                const auto action = CalculateManipulationDataAction(
                    m_fixed, m_startInternal, TransformUniformScale(GetSpace()), GetNonUniformScale(),
                    TransformNormalizedScale(GetLocalTransform()), gridSnapParams, mouseInteraction);

                // display the exact hit (ray intersection) of the mouse pick on the manipulator
                DrawTransformAxes(
                    debugDisplay,
                    TransformUniformScale(GetSpace()) *
                        AZ::Transform::CreateTranslation(
                            action.m_start.m_localHitPosition + GetNonUniformScale() * action.m_current.m_localOffset));
            }

            AZ::Transform combined = GetLocalTransform();
            combined.SetTranslation(GetNonUniformScale() * combined.GetTranslation());
            combined = GetSpace() * combined;

            DrawTransformAxes(debugDisplay, combined);

            DrawAxis(debugDisplay, combined.GetTranslation(), TransformDirectionNoScaling(GetLocalTransform(), m_fixed.m_axis1));
            DrawAxis(debugDisplay, combined.GetTranslation(), TransformDirectionNoScaling(GetLocalTransform(), m_fixed.m_axis2));
        }

        for (auto& view : m_manipulatorViews)
        {
            view->Draw(
                GetManipulatorManagerId(), managerState, GetManipulatorId(),
                ManipulatorState{ ApplySpace(GetLocalTransform()), GetNonUniformScale(), AZ::Vector3::CreateZero(), MouseOver() },
                debugDisplay, cameraState, mouseInteraction);
        }
    }

    void PlanarManipulator::SetAxes(const AZ::Vector3& axis1, const AZ::Vector3& axis2)
    {
        m_fixed.m_axis1 = axis1;
        m_fixed.m_axis2 = axis2;
        m_fixed.m_normal = axis1.Cross(axis2);
    }

    void PlanarManipulator::InvalidateImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->Invalidate(GetManipulatorManagerId());
        }
    }

    void PlanarManipulator::SetBoundsDirtyImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->SetBoundDirty(GetManipulatorManagerId());
        }
    }
} // namespace AzToolsFramework
