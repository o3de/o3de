/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SurfaceManipulator.h"

#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    SurfaceManipulator::StartInternal SurfaceManipulator::CalculateManipulationDataStart(
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& worldSurfacePosition,
        const AZ::Vector3& localStartPosition,
        [[maybe_unused]] const bool snapping,
        [[maybe_unused]] const float gridSize,
        [[maybe_unused]] const int viewportId)
    {
        const AZ::Transform localFromWorld = worldFromLocal.GetInverse();

        StartInternal startInternal;
        startInternal.m_snapOffset = AZ::Vector3::CreateZero();
        startInternal.m_localPosition = localStartPosition;
        startInternal.m_localHitPosition = localFromWorld.TransformPoint(worldSurfacePosition);
        return startInternal;
    }

    SurfaceManipulator::Action SurfaceManipulator::CalculateManipulationDataAction(
        const StartInternal& startInternal,
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& worldSurfacePosition,
        [[maybe_unused]] const bool snapping,
        [[maybe_unused]] const float gridSize,
        const ViewportInteraction::KeyboardModifiers keyboardModifiers,
        [[maybe_unused]] const int viewportId)
    {
        const AZ::Transform localFromWorld = worldFromLocal.GetInverse();
        const AZ::Vector3 localFinalSurfacePosition = localFromWorld.TransformPoint(worldSurfacePosition);

        Action action;
        action.m_start.m_localPosition = startInternal.m_localPosition;
        action.m_start.m_snapOffset = AZ::Vector3::CreateZero();
        action.m_current.m_localOffset = localFinalSurfacePosition - startInternal.m_localPosition;
        action.m_modifiers = keyboardModifiers; // record what modifier keys are held during this action
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

        // only cast rays against objects (entities/meshes etc.) we can actually see
        m_rayRequest.m_onlyVisible = true;
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
        const ViewportInteraction::MouseInteraction& interaction, [[maybe_unused]] float rayIntersectionDistance)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(GetSpace());

        const AzFramework::ViewportId viewportId = interaction.m_interactionId.m_viewportId;

        const auto& entityComponentIdPairs = EntityComponentIdPairs();
        m_rayRequest.m_entityFilter.m_ignoreEntities.clear();
        m_rayRequest.m_entityFilter.m_ignoreEntities.reserve(entityComponentIdPairs.size());
        AZStd::transform(
            entityComponentIdPairs.begin(), entityComponentIdPairs.end(),
            AZStd::inserter(m_rayRequest.m_entityFilter.m_ignoreEntities, m_rayRequest.m_entityFilter.m_ignoreEntities.begin()),
            [](const AZ::EntityComponentIdPair& entityComponentIdPair)
            {
                return entityComponentIdPair.GetEntityId();
            });

        // calculate the start and end of the ray
        RefreshRayRequest(
            m_rayRequest, ViewportInteraction::ViewportScreenToWorldRay(viewportId, interaction.m_mousePick.m_screenCoordinates),
            EditorPickRayLength);

        const GridSnapParameters gridSnapParams = GridSnapSettings(viewportId);
        const AZ::Vector3 worldSurfacePosition = FindClosestPickIntersection(m_rayRequest, GetDefaultEntityPlacementDistance());

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
            const AzFramework::ViewportId viewportId = interaction.m_interactionId.m_viewportId;

            const GridSnapParameters gridSnapParams = GridSnapSettings(viewportId);
            const AZ::Vector3 worldSurfacePosition = FindClosestPickIntersection(m_rayRequest, GetDefaultEntityPlacementDistance());

            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                m_startInternal, TransformUniformScale(GetSpace()), worldSurfacePosition, gridSnapParams.m_gridSnap,
                gridSnapParams.m_gridSize, interaction.m_keyboardModifiers, viewportId));
        }
    }

    void SurfaceManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            const AzFramework::ViewportId viewportId = interaction.m_interactionId.m_viewportId;

            // update the start and end of the ray
            RefreshRayRequest(
                m_rayRequest, ViewportInteraction::ViewportScreenToWorldRay(viewportId, interaction.m_mousePick.m_screenCoordinates),
                EditorPickRayLength);

            const GridSnapParameters gridSnapParams = GridSnapSettings(viewportId);
            const AZ::Vector3 worldSurfacePosition = FindClosestPickIntersection(m_rayRequest, GetDefaultEntityPlacementDistance());

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
            ManipulatorState{ TransformUniformScale(GetSpace()), GetNonUniformScale(), GetLocalPosition(), MouseOver() }, debugDisplay,
            cameraState, mouseInteraction);
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
