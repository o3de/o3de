/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzManipulatorTestFramework/AzManipulatorTestFrameworkUtils.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorTransformComponentSelectionRequestBus.h>

namespace AzManipulatorTestFramework
{
    using InteractionId = AzToolsFramework::ViewportInteraction::InteractionId;
    using KeyboardModifiers = AzToolsFramework::ViewportInteraction::KeyboardModifiers;
    using MouseInteraction = AzToolsFramework::ViewportInteraction::MouseInteraction;
    using MouseInteractionEvent = AzToolsFramework::ViewportInteraction::MouseInteractionEvent;
    using MouseButton = AzToolsFramework::ViewportInteraction::MouseButton;
    using MouseButtons = AzToolsFramework::ViewportInteraction::MouseButtons;
    using MouseEvent = AzToolsFramework::ViewportInteraction::MouseEvent;
    using MousePick = AzToolsFramework::ViewportInteraction::MousePick;

    // create a default sphere view for a manipulator for simple intersection
    template<typename Manipulator>
    void SetupManipulatorView(
        AZStd::shared_ptr<Manipulator> manipulator,
        const AzToolsFramework::ManipulatorManagerId manipulatorManagerId,
        const AZ::Vector3& position,
        const float radius)
    {
        // unit sphere view
        auto sphereView = AzToolsFramework::CreateManipulatorViewSphere(
            AZ::Colors::Red, radius,
            []([[maybe_unused]] const MouseInteraction& mouseInteraction, [[maybe_unused]] const bool mouseOver,
               const AZ::Color& defaultColor)
            {
                return defaultColor;
            },
            true);

        // unit sphere bound
        AzToolsFramework::Picking::BoundShapeSphere sphereBound;
        sphereBound.m_center = position;
        sphereBound.m_radius = radius;

        // we need the view to construct the manipulator bounds after the manipulator has been registered
        auto view = sphereView.get();

        // construct view and register with manager
        AzToolsFramework::ManipulatorViews views;
        views.emplace_back(AZStd::move(sphereView));
        manipulator->SetViews(AZStd::move(views));
        manipulator->Register(manipulatorManagerId);

        // this would occur internally when the manipulator is drawn but we must do manually here to ensure that the
        // bounds will always be valid upon instantiation
        view->RefreshBound(manipulatorManagerId, manipulator->GetManipulatorId(), sphereBound);
    }

    AZStd::shared_ptr<AzToolsFramework::LinearManipulator> CreateLinearManipulator(
        const AzToolsFramework::ManipulatorManagerId manipulatorManagerId, const AZ::Vector3& position, const float radius)
    {
        auto manipulator = AzToolsFramework::LinearManipulator::MakeShared(AZ::Transform::CreateIdentity());
        manipulator->SetLocalPosition(position);

        SetupManipulatorView(manipulator, manipulatorManagerId, position, radius);

        return manipulator;
    }

    AZStd::shared_ptr<AzToolsFramework::PlanarManipulator> CreatePlanarManipulator(
        const AzToolsFramework::ManipulatorManagerId manipulatorManagerId, const AZ::Vector3& position, const float radius)
    {
        auto manipulator = AzToolsFramework::PlanarManipulator::MakeShared(AZ::Transform::CreateIdentity());
        manipulator->SetLocalPosition(position);

        SetupManipulatorView(manipulator, manipulatorManagerId, position, radius);

        return manipulator;
    }

    AzToolsFramework::ViewportInteraction::MousePick CreateMousePick(
        const AZ::Vector3& origin, const AZ::Vector3& direction, const AzFramework::ScreenPoint& screenPoint)
    {
        return { origin, direction, screenPoint };
    }

    AzToolsFramework::ViewportInteraction::MousePick BuildMousePick(
        const AzFramework::ScreenPoint& screenPoint, const AzFramework::CameraState& cameraState)
    {
        const auto nearPlaneWorldPosition = AzFramework::ScreenToWorld(screenPoint, cameraState);

        AzToolsFramework::ViewportInteraction::MousePick mousePick;
        mousePick.m_screenCoordinates = screenPoint;
        mousePick.m_rayOrigin = nearPlaneWorldPosition;
        mousePick.m_rayDirection = (nearPlaneWorldPosition - cameraState.m_position).GetNormalized();

        return mousePick;
    }

    MouseInteraction CreateMouseInteraction(
        const MousePick& mousePick, MouseButtons buttons, InteractionId interactionId, KeyboardModifiers modifiers)
    {
        AzToolsFramework::ViewportInteraction::MouseInteraction interaction;
        interaction.m_mousePick = mousePick;
        interaction.m_mouseButtons = buttons;
        interaction.m_interactionId = interactionId;
        interaction.m_keyboardModifiers = modifiers;

        return interaction;
    }

    MouseButtons CreateMouseButtons(MouseButton button)
    {
        MouseButtons buttons;
        buttons.m_mouseButtons = static_cast<AZ::u32>(button);
        return buttons;
    }

    MouseInteractionEvent CreateMouseInteractionEvent(const MouseInteraction& mouseInteraction, MouseEvent event)
    {
        return MouseInteractionEvent(mouseInteraction, event, /*captured=*/false);
    }

    void DispatchMouseInteractionEvent(const MouseInteractionEvent& event)
    {
        AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Event(
            AzToolsFramework::GetEntityContextId(),
            &AzToolsFramework::ViewportInteraction::InternalMouseViewportRequests::InternalHandleAllMouseInteractions, event);
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

    AzFramework::ScreenPoint GetCameraStateViewportCenter(const AzFramework::CameraState& cameraState)
    {
        return { aznumeric_cast<int>(cameraState.m_viewportSize.GetX() / 2.f),
                 aznumeric_cast<int>(cameraState.m_viewportSize.GetY() / 2.f) };
    }
} // namespace AzManipulatorTestFramework
