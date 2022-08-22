/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <AzToolsFramework/Brushes/PaintBrushRequestBus.h>
#include <AzToolsFramework/Manipulators/PaintBrushManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorSnapping.h>
#include <AzToolsFramework/Manipulators/ManipulatorView.h>

namespace AzToolsFramework
{
    AZStd::shared_ptr<PaintBrushManipulator> PaintBrushManipulator::MakeShared(
        const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return AZStd::shared_ptr<PaintBrushManipulator>(aznew PaintBrushManipulator(worldFromLocal, entityComponentIdPair));
    }

    PaintBrushManipulator::PaintBrushManipulator(
        const AZ::Transform& worldFromLocal, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        SetSpace(worldFromLocal);

        // The PaintBrush manipulator uses a circle projected into world space to represent the brush.

        float radius = 1.0f;
        PaintBrushRequestBus::EventResult(radius, entityComponentIdPair, &AzToolsFramework::PaintBrushRequestBus::Events::GetRadius);

        const AZ::Color manipulatorColor = AZ::Color(1.0f, 0.0f, 0.0f, 1.0f);
        const float manipulatorWidth = 0.05f;
        SetView(
            AzToolsFramework::CreateManipulatorViewProjectedCircle(*this, manipulatorColor, radius, manipulatorWidth));
    }

    void PaintBrushManipulator::Draw(
        const ManipulatorManagerState& managerState, AzFramework::DebugDisplayRequests& debugDisplay,
        const AzFramework::CameraState& cameraState, const ViewportInteraction::MouseInteraction& mouseInteraction)
    {
        m_manipulatorView->Draw(
            GetManipulatorManagerId(), managerState, GetManipulatorId(),
            { GetSpace(), GetNonUniformScale(), AZ::Vector3::CreateZero(), MouseOver() }, debugDisplay, cameraState,
            mouseInteraction);
    }

    void PaintBrushManipulator::SetView(AZStd::shared_ptr<ManipulatorViewProjectedCircle> view)
    {
        m_manipulatorView = AZStd::move(view);
    }

    void PaintBrushManipulator::SetRadius(const float radius)
    {
        m_manipulatorView->SetRadius(radius);
    }
} // namespace AzToolsFramework
