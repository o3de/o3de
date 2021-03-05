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
        : m_worldFromLocal(worldFromLocal)
    {
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
        const AZ::Transform& worldFromLocal, const AZ::Transform& localTransform,
        const ViewportInteraction::MouseInteraction& interaction,
        const AZStd::vector<LinearManipulator::Fixed>& fixedAxes,
        const AZStd::vector<LinearManipulator::Starter>& starterStates, const GridSnapAction& gridSnapAction)
    {
        MultiLinearManipulator::Action action;
        action.m_viewportId = interaction.m_interactionId.m_viewportId;
        // build up action state for each axis
        for (size_t fixedIndex = 0; fixedIndex < fixedAxes.size(); ++fixedIndex)
        {
            action.m_actions.push_back(
                CalculateLinearManipulationDataAction(
                    fixedAxes[fixedIndex], starterStates[fixedIndex], worldFromLocal, localTransform, gridSnapAction,
                    interaction));
        }

        return action;
    }

    void MultiLinearManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, const float rayIntersectionDistance)
    {
        const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(m_worldFromLocal);

        const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);
        const AzFramework::CameraState cameraState = GetCameraState(interaction.m_interactionId.m_viewportId);

        // build up initial start state for each axis
        for (const auto& fixed : m_fixedAxes)
        {
            // note: m_localTransform must not be made uniform as it may contain a local scale we want to snap
            const auto linearStart = CalculateLinearManipulationDataStart(
                fixed, worldFromLocalUniformScale, m_localTransform,
                GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt()), interaction,
                rayIntersectionDistance, cameraState);

            m_starters.push_back(linearStart);
        }

        if (m_onLeftMouseDownCallback)
        {
            const GridSnapAction gridSnapAction = GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt());
            // pass action containing all linear actions for each axis to handler
            m_onLeftMouseDownCallback(BuildMultiLinearManipulatorAction(
                worldFromLocalUniformScale, m_localTransform, interaction, m_fixedAxes, m_starters, gridSnapAction));
        }
    }

    void MultiLinearManipulator::OnMouseMoveImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onMouseMoveCallback)
        {
            const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(m_worldFromLocal);
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);
            const GridSnapAction gridSnapAction = GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt());

            m_onMouseMoveCallback(BuildMultiLinearManipulatorAction(
                worldFromLocalUniformScale, m_localTransform, interaction, m_fixedAxes, m_starters, gridSnapAction));
        }
    }

    void MultiLinearManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (m_onLeftMouseUpCallback)
        {
            const AZ::Transform worldFromLocalUniformScale = TransformUniformScale(m_worldFromLocal);
            const GridSnapParameters gridSnapParams = GridSnapSettings(interaction.m_interactionId.m_viewportId);
            const GridSnapAction gridSnapAction = GridSnapAction(gridSnapParams, interaction.m_keyboardModifiers.Alt());

            m_onLeftMouseUpCallback(BuildMultiLinearManipulatorAction(
                worldFromLocalUniformScale, m_localTransform, interaction, m_fixedAxes, m_starters, gridSnapAction));

            m_starters.clear();
        }
    }

    void MultiLinearManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        if (cl_manipulatorDrawDebug)
        {
            const AZ::Transform combined = TransformUniformScale(m_worldFromLocal) * m_localTransform;
            for (const auto& fixed : m_fixedAxes)
            {
                DrawAxis(
                    debugDisplay, combined.GetTranslation(), TransformDirectionNoScaling(combined, fixed.m_axis));
            }
        }

        for (auto& view : m_manipulatorViews)
        {
            view->Draw(
                GetManipulatorManagerId(), managerState,
                GetManipulatorId(), {
                    m_worldFromLocal * m_localTransform,
                    AZ::Vector3::CreateZero(), MouseOver()
                },
                debugDisplay, cameraState, mouseInteraction);
        }
    }

    void MultiLinearManipulator::AddAxis(const AZ::Vector3& axis)
    {
        m_fixedAxes.push_back(LinearManipulator::Fixed{axis});
    }

    void MultiLinearManipulator::AddAxes(const AZStd::vector<AZ::Vector3>& axes)
    {
        AZStd::transform(
            axes.begin(), axes.end(),
            AZStd::back_inserter(m_fixedAxes),
            [](const AZ::Vector3& axis)
            {
                return LinearManipulator::Fixed{axis};
            });
    }

    void MultiLinearManipulator::SetSpace(const AZ::Transform& worldFromLocal)
    {
        m_worldFromLocal = worldFromLocal;
    }

    void MultiLinearManipulator::SetLocalTransform(const AZ::Transform& localTransform)
    {
        m_localTransform = localTransform;
    }

    void MultiLinearManipulator::SetLocalPosition(const AZ::Vector3& localPosition)
    {
        m_localTransform.SetTranslation(localPosition);
    }

    void MultiLinearManipulator::SetLocalOrientation(const AZ::Quaternion& localOrientation)
    {
        m_localTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            localOrientation, m_localTransform.GetTranslation());
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
