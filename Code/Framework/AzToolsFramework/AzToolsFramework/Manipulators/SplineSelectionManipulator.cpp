/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SplineSelectionManipulator.h"

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    SplineSelectionManipulator::Action CalculateManipulationDataAction(
        const AZ::Transform& worldFromLocal,
        const AZ::Vector3& rayOrigin,
        const AZ::Vector3& rayDirection,
        const AZStd::weak_ptr<const AZ::Spline>& spline)
    {
        SplineSelectionManipulator::Action action;
        if (const AZStd::shared_ptr<const AZ::Spline> splinePtr = spline.lock())
        {
            const auto splineQueryResult = IntersectSpline(worldFromLocal, rayOrigin, rayDirection, *splinePtr);
            action.m_localSplineHitPosition = splinePtr->GetPosition(splineQueryResult.m_splineAddress);
            action.m_splineAddress = splineQueryResult.m_splineAddress;
        }

        return action;
    }

    AZStd::shared_ptr<SplineSelectionManipulator> SplineSelectionManipulator::MakeShared()
    {
        return AZStd::shared_ptr<SplineSelectionManipulator>(aznew SplineSelectionManipulator());
    }

    SplineSelectionManipulator::SplineSelectionManipulator()
    {
        AttachLeftMouseDownImpl();
    }

    SplineSelectionManipulator::~SplineSelectionManipulator() = default;

    void SplineSelectionManipulator::InstallLeftMouseDownCallback(const MouseActionCallback& onMouseDownCallback)
    {
        m_onLeftMouseDownCallback = onMouseDownCallback;
    }

    void SplineSelectionManipulator::InstallLeftMouseUpCallback(const MouseActionCallback& onMouseUpCallback)
    {
        m_onLeftMouseUpCallback = onMouseUpCallback;
    }

    void SplineSelectionManipulator::OnLeftMouseDownImpl(
        const ViewportInteraction::MouseInteraction& interaction, float /*rayIntersectionDistance*/)
    {
        if (!interaction.m_keyboardModifiers.Ctrl())
        {
            return;
        }

        if (m_onLeftMouseDownCallback)
        {
            m_onLeftMouseDownCallback(CalculateManipulationDataAction(
                TransformUniformScale(GetSpace()), interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, m_spline));
        }
    }

    void SplineSelectionManipulator::OnLeftMouseUpImpl(const ViewportInteraction::MouseInteraction& interaction)
    {
        if (MouseOver() && m_onLeftMouseUpCallback)
        {
            m_onLeftMouseUpCallback(CalculateManipulationDataAction(
                TransformUniformScale(GetSpace()), interaction.m_mousePick.m_rayOrigin, interaction.m_mousePick.m_rayDirection, m_spline));
        }
    }

    void SplineSelectionManipulator::Draw(
        const ManipulatorManagerState& managerState,
        AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState,
        const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        // if the ctrl modifier key state has changed - set out bounds to dirty and
        // update the active state.
        if (m_keyboardModifiers != mouseInteraction.m_keyboardModifiers)
        {
            SetBoundsDirty();
            m_keyboardModifiers = mouseInteraction.m_keyboardModifiers;
        }

        if (mouseInteraction.m_keyboardModifiers.Ctrl() && !mouseInteraction.m_keyboardModifiers.Shift())
        {
            m_manipulatorView->Draw(
                GetManipulatorManagerId(), managerState, GetManipulatorId(),
                ManipulatorState{ TransformUniformScale(GetSpace()), GetNonUniformScale(), AZ::Vector3::CreateZero(), MouseOver() },
                debugDisplay, cameraState, mouseInteraction);
        }
    }

    void SplineSelectionManipulator::SetView(AZStd::unique_ptr<ManipulatorView>&& view)
    {
        m_manipulatorView = AZStd::move(view);
    }

    void SplineSelectionManipulator::SetBoundsDirtyImpl()
    {
        m_manipulatorView->SetBoundDirty(GetManipulatorManagerId());
    }

    void SplineSelectionManipulator::InvalidateImpl()
    {
        m_manipulatorView->Invalidate(GetManipulatorManagerId());
    }
} // namespace AzToolsFramework
