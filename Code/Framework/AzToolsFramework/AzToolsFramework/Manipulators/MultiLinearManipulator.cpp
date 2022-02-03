/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MultiLinearManipulator.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorDebug.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Maths/TransformUtils.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(MultiLinearManipulator, AZ::SystemAllocator, 0)

    AZStd::shared_ptr<MultiLinearManipulator> MultiLinearManipulator::MakeShared(const AZ::Transform& worldFromLocal)
    {
        return AZStd::shared_ptr<MultiLinearManipulator>(aznew MultiLinearManipulator(worldFromLocal));
    }

    MultiLinearManipulator::MultiLinearManipulator(const AZ::Transform& worldFromLocal)
    {
        SetSpace(worldFromLocal);
        AttachLeftMouseDownImpl();
    }

    MultiLinearManipulator::~MultiLinearManipulator()
    {
        ClearAxes();
    }

    void MultiLinearManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void MultiLinearManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void MultiLinearManipulator::InstallMouseMoveCallback(const MouseActionCallback& onMouseMoveCallback)
    {
        m_onMouseMoveCallback = onMouseMoveCallback;
    }

    static MultiLinearManipulator::Action BuildMultiLinearManipulatorAction(
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& nonUniformScale,
        const AZ::Transform& localTransform,
        const ViewportInteraction::MouseInteraction& interaction,
        const AZStd::vector<LinearManipulator::Fixed>& fixedAxes,
        const AZStd::vector<LinearManipulator::Starter>& starterStates,
        const GridSnapParameters& gridSnapParams)
    {
        MultiLinearManipulator::Action action;
        action.m_viewportId = interaction.m_interactionId.m_viewportId;
        // build up action state for each axis
        for (size_t fixedIndex = 0; fixedIndex < fixedAxes.size(); ++fixedIndex)
        {
            action.m_actions.push_back(CalculateLinearManipulationDataAction(
                fixedAxes[fixedIndex], starterStates[fixedIndex], worldFromLocal, nonUniformScale, localTransform, gridSnapParams,
                interaction));
        }

        return action;
    }

    void MultiLinearManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, const float rayIntersectionDistance)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(GetSpace());
        const AzFramework::CameraState cameraState = GetCameraState(interaction.m_interactionId.m_viewportId);

        // build up initial start state for each axis
        for (const auto& fixed : m_fixedAxes)
        {
            // note: m_localTransform must not be made uniform as it may contain a local scale we want to snap
            const auto linearStart = CalculateLinearManipulationDataStart(
                fixed, worldFromLocalUniformScale, GetNonUniformScale(), GetLocalTransform(), interaction, rayIntersectionDistance,
                cameraState);

            m_starters.push_back(linearStart);
        }

        if (m_onLeftMouseDownCallback)
        {
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);
            // pass action containing all linear actions for each axis to handler
            m_onLeftMouseDownCallback(BuildMultiLinearManipulatorAction(
                worldFromLocalUniformScale, GetNonUniformScale(), GetLocalTransform(), interaction, m_fixedAxes, m_starters,
                gridSnapParams));
        }
    }

    void MultiLinearManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(GetSpace());
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);
            m_onMouseMoveCallback(BuildMultiLinearManipulatorAction(
                worldFromLocalUniformScale, GetNonUniformScale(), GetLocalTransform(), interaction, m_fixedAxes, m_starters,
                gridSnapParams));
        }
    }

    void MultiLinearManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(GetSpace());
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);
            m_onLeftMouseUpCallback(BuildMultiLinearManipulatorAction(
                worldFromLocalUniformScale, GetNonUniformScale(), GetLocalTransform(), interaction, m_fixedAxes, m_starters,
                gridSnapParams));

            m_starters.clear();
        }
    }

    void MultiLinearManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        if (ed_manipulatorDrawDebug)
        {
            const AZ::Transform combined = TransformUniformScale(GetSpace()) * GetLocalTransform();
            for (const auto& fixed : m_fixedAxes)
            {
                DrawAxis(debugDisplay, combined.GetTranslation(), TransformDirectionNoScaling(combined, fixed.m_axis));
            }
        }

        for (auto& view : m_manipulatorViews)
        {
            view->Draw(
                GetManipulatorManagerId(), managerState, GetManipulatorId(),
                ManipulatorState{ ApplySpace(GetLocalTransform()), GetNonUniformScale(), AZ::Vector3::CreateZero(), MouseOver() },
                debugDisplay, cameraState, mouseInteraction);
        }
    }

    void MultiLinearManipulator::AddAxis(const AZ::Vector3& axis)
    {
        m_fixedAxes.push_back(LinearManipulator::Fixed{ axis });
    }

    void MultiLinearManipulator::AddAxes(const AZStd::vector<AZ::Vector3>& axes)
    {
        AZStd::transform(
            axes.begin(), axes.end(), AZStd::back_inserter(m_fixedAxes),
            [](const AZ::Vector3& axis)
            {
                return LinearManipulator::Fixed{ axis };
            });
    }

    void MultiLinearManipulator::ClearAxes()
    {
        m_fixedAxes.clear();
    }

    void MultiLinearManipulator::InvalidateImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->Invalidate(GetManipulatorManagerId());
        }
    }

    void MultiLinearManipulator::SetBoundsDirtyImpl()
    {
        for (auto& view : m_manipulatorViews)
        {
            view->SetBoundDirty(GetManipulatorManagerId());
        }
    }
} // namespace AzToolsFramework
