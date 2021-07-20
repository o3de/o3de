/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SurfaceManipulator.h"

#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    SurfaceManipulator::StartInternal SurfaceManipulator::CalculateManipulationDataStart(
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& worldSurfacePosition,
        const AZ::Vector3& localStartPosition,
        const bool snapping,
        const float gridSize,
        const int viewportId)
    {
        const AZ::Transform worldFromLocalUniform = AzToolsFramework::TransformUniformScale(worldFromLocal);
        const AZ::Transform localFromWorldUniform = worldFromLocalUniform.GetInverse();

        const AZ::Vector3 localFinalSurfacePosition = snapping
            // note: gridSize is not scaled by scaleRecip here as localStartPosition is
            // unscaled itself so the position returned by CalculateSnappedTerrainPosition
            // must be in the same space (if localStartPosition were also scaled, gridSize
            // would need to be multiplied by scaleRecip)
            ? CalculateSnappedTerrainPosition(worldSurfacePosition, worldFromLocalUniform, viewportId, gridSize)
            : localFromWorldUniform.TransformPoint(worldSurfacePosition);

        // delta/offset between initial vertex position and terrain pick position
        const AZ::Vector3 localSurfaceOffset = localFinalSurfacePosition - localStartPosition;

        StartInternal startInternal;
        startInternal.m_snapOffset = localSurfaceOffset;
        startInternal.m_localPosition = localStartPosition + localSurfaceOffset;
        startInternal.m_localHitPosition = localFromWorldUniform.TransformVector(worldSurfacePosition);
        return startInternal;
    }

    SurfaceManipulator::Action SurfaceManipulator::CalculateManipulationDataAction(
        const StartInternal& startInternal,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& worldSurfacePosition,
        const bool snapping,
        const float gridSize,
        const ViewportInteraction::KeyboardModifiers keyboardModifiers,
        const int viewportId)
    {
        const AZ::Transform worldFromLocalUniform = AzToolsFramework::TransformUniformScale(worldFromLocal);
        const AZ::Transform localFromWorldUniform = worldFromLocalUniform.GetInverse();

        const float scaleRecip = ScaleReciprocal(worldFromLocalUniform);

        const AZ::Vector3 localFinalSurfacePosition = snapping
            ? CalculateSnappedTerrainPosition(worldSurfacePosition, worldFromLocalUniform, viewportId, gridSize * scaleRecip)
            : localFromWorldUniform.TransformPoint(worldSurfacePosition);

        Action action;
        action.m_start.m_localPosition = startInternal.m_localPosition;
        action.m_start.m_snapOffset = startInternal.m_snapOffset;
        action.m_current.m_localOffset = localFinalSurfacePosition - startInternal.m_localPosition;
        // record what modifier keys are held during this action
        action.m_modifiers = keyboardModifiers;
        return action;
    }

    AZStd::shared_ptr<SurfaceManipulator> SurfaceManipulator::MakeShared(const AZ::Transform& worldFromLocal)
    {
        return AZStd::shared_ptr<SurfaceManipulator>(aznew SurfaceManipulator(worldFromLocal));
    }

    SurfaceManipulator::SurfaceManipulator(const AZ::Transform& worldFromLocal)
    {
        SetSpace(worldFromLocal);
        AttachLeftMouseDownImpl();
    }

    void SurfaceManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }
    void SurfaceManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void SurfaceManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    void SurfaceManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, float /*rayIntersectionDistance*/)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(GetSpace());

        const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

        AZ::Vector3 worldSurfacePosition;
        ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
            worldSurfacePosition, interaction.m_interactionId.m_viewportId,
            &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::PickTerrain,
            interaction.m_mousePick.m_screenCoordinates);

        m_startInternal = CalculateManipulationDataStart(
            worldFromLocalUniformScale, worldSurfacePosition, GetLocalPosition(), gridSnapParams.m_gridSnap, gridSnapParams.m_gridSize,
            interaction.m_interactionId.m_viewportId);

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                m_startInternal, worldFromLocalUniformScale, worldSurfacePosition, gridSnapParams.m_gridSnap, gridSnapParams.m_gridSize,
                interaction.m_keyboardModifiers, interaction.m_interactionId.m_viewportId));
        }
    }

    void SurfaceManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            AZ::Vector3 worldSurfacePosition = AZ::Vector3::CreateZero();
            ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
                worldSurfacePosition, interaction.m_interactionId.m_viewportId,
                &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::PickTerrain,
                interaction.m_mousePick.m_screenCoordinates);

            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_startInternal, TransformUniformScale(GetSpace()), worldSurfacePosition, gridSnapParams.m_gridSnap,
                gridSnapParams.m_gridSize, interaction.m_keyboardModifiers, interaction.m_interactionId.m_viewportId));
        }
    }

    void SurfaceManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            AZ::Vector3 worldSurfacePosition = AZ::Vector3::CreateZero();
            ViewportInteraction::MainEditorViewportInteractionRequestBus::EventResult(
                worldSurfacePosition, interaction.m_interactionId.m_viewportId,
                &ViewportInteraction::MainEditorViewportInteractionRequestBus::Events::PickTerrain,
                interaction.m_mousePick.m_screenCoordinates);

            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);

            m_onMouseMoveCallback(CalculateManipulationDataAction(
                m_startInternal, TransformUniformScale(GetSpace()), worldSurfacePosition, gridSnapParams.m_gridSnap,
                gridSnapParams.m_gridSize, interaction.m_keyboardModifiers, interaction.m_interactionId.m_viewportId));
        }
    }

    void SurfaceManipulator::SetBoundsDirtyImpl()
    {
        m_manipulatorView->SetBoundDirty(GetManipulatorManagerId());
    }

    void SurfaceManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        m_manipulatorView->Draw(
            GetManipulatorManagerId(), managerState, GetManipulatorId(),
            { TransformUniformScale(GetSpace()), GetNonUniformScale(), GetLocalPosition(), MouseOver() }, debugDisplay, cameraState,
            mouseInteraction);
    }

    void SurfaceManipulator::InvalidateImpl()
    {
        m_manipulatorView->Invalidate(GetManipulatorManagerId());
    }

    void SurfaceManipulator::SetView(AZStd::unique_ptr<ManipulatorView>&& view)
    {
        m_manipulatorView = AZStd::move(view);
    }
} // namespace AzToolsFramework
