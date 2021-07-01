/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkFixture.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>

namespace UnitTest
{
    using MouseInteraction = AzToolsFramework::ViewportInteraction::MouseInteraction;
    using MouseInteractionEvent = AzToolsFramework::ViewportInteraction::MouseInteractionEvent;
    using MouseButton = AzToolsFramework::ViewportInteraction::MouseButton;
    using MouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent;
    using MousePick = AzToolsFramework::ViewportInteraction::MousePick;

    AZStd::shared_ptr<AzToolsFramework::LinearManipulator> CreateLinearManipulator(
        const AzToolsFramework::ManipulatorManagerId manipulatorManagerId)
    {
        auto manipulator = AzToolsFramework::LinearManipulator::MakeShared(AZ::Transform::CreateIdentity());

        // unit sphere view
        auto sphereView = AzToolsFramework::CreateManipulatorViewSphere(
            AZ::Colors::Red, 1.0f,
            [](const MouseInteraction& /*mouseInteraction*/, const bool /*mouseOver*/,
               const AZ::Color& defaultColor)
        {
            return defaultColor;
        }, true);

        // unit sphere bound
        AzToolsFramework::Picking::BoundShapeSphere sphereBound;
        sphereBound.m_center = AZ::Vector3::CreateZero();
        sphereBound.m_radius = 1.0f;

        // we need the view to construct the manipulator bounds after the manipulator has been registered
        auto view = sphereView.get();

        // construct view and register with manager
        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(AZStd::move(sphereView));
        manipulator->SetViews(AZStd::move(views));
        manipulator->Register(manipulatorManagerId);

        // this would occur internally when the manipulator is drawn but we must do manually here
        view->RefreshBound(manipulatorManagerId, manipulator->GetManipulatorId(), sphereBound);

        return manipulator;
    }

    MousePick CreateWorldSpaceMousePickRay(const AZ::Vector3& origin, const AZ::Vector3& direction)
    {
        MousePick mousePick;
        mousePick.m_rayOrigin = origin;
        mousePick.m_rayDirection = direction;
        return mousePick;
    }

    MouseInteraction CreateMouseInteraction(const MousePick& worldSpaceRay, MouseButton button)
    {
        AzToolsFramework::ViewportInteraction::MouseInteraction interaction;
        interaction.m_mousePick = worldSpaceRay;
        interaction.m_mouseButtons.m_mouseButtons = static_cast<AZ::u32>(button);
        return interaction;
    }

    MouseInteractionEvent CreateMouseInteractionEvent(
        const MousePick& worldSpaceRay, MouseButton button, MouseEvent event)
    {
        return MouseInteractionEvent(CreateMouseInteraction(worldSpaceRay, button), event);
    }

    void DispatchMouseInteractionEvent(const MouseInteractionEvent& event)
    {
        AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::ViewportInteraction::InternalMouseViewportRequests::InternalHandleAllMouseInteractions,
            event);
    }

    AzFramework::CameraState SetCameraStatePosition(const AZ::Vector3& position, AzFramework::CameraState& cameraState)
    {
        cameraState.m_position = position;
        return cameraState;
    }

    AzFramework::CameraState SetCameraStateDirection(const AZ::Vector3& direction, AzFramework::CameraState& cameraState)
    {
        const auto transform = AZ::Transform::CreateLookAt(cameraState.m_position, cameraState.m_position + direction);
        AzFramework::SetCameraTransform(cameraState, transform);
        return cameraState;
    }
} // namespace UnitTest

