/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/Manipulators/PlanarManipulator.h>

namespace AzManipulatorTestFramework
{
    //! Create a linear manipulator with a unit sphere bound.
    AZStd::shared_ptr<AzToolsFramework::LinearManipulator> CreateLinearManipulator(
        const AzToolsFramework::ManipulatorManagerId manipulatorManagerId,
        const AZ::Vector3& position = AZ::Vector3::CreateZero(),
        float radius = 1.0f);

    //! Create a planar manipulator with a unit sphere bound.
    AZStd::shared_ptr<AzToolsFramework::PlanarManipulator> CreatePlanarManipulator(
        const AzToolsFramework::ManipulatorManagerId manipulatorManagerId,
        const AZ::Vector3& position = AZ::Vector3::CreateZero(),
        float radius = 1.0f);

    //! Create a mouse pick from the specified ray and screen point.
    AzToolsFramework::ViewportInteraction::MousePick CreateMousePick(
        const AZ::Vector3& origin, const AZ::Vector3& direction, const AzFramework::ScreenPoint& screenPoint);

    //! Build a mouse pick from the specified mouse position and camera state.
    AzToolsFramework::ViewportInteraction::MousePick BuildMousePick(
        const AzFramework::ScreenPoint& screenPoint, const AzFramework::CameraState& cameraState);

    //! Create a mouse interaction from the specified pick, buttons, interaction id and keyboard modifiers.
    AzToolsFramework::ViewportInteraction::MouseInteraction CreateMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MousePick& mousePick,
        AzToolsFramework::ViewportInteraction::MouseButtons buttons,
        AzToolsFramework::ViewportInteraction::InteractionId interactionId,
        AzToolsFramework::ViewportInteraction::KeyboardModifiers modifiers);

    //! Create a mouse buttons from the specified mouse button.
    AzToolsFramework::ViewportInteraction::MouseButtons CreateMouseButtons(AzToolsFramework::ViewportInteraction::MouseButton button);

    //! Create a mouse interaction event from the specified interaction and event.
    AzToolsFramework::ViewportInteraction::MouseInteractionEvent CreateMouseInteractionEvent(
        const AzToolsFramework::ViewportInteraction::MouseInteraction& mouseInteraction,
        AzToolsFramework::ViewportInteraction::MouseEvent event);

    //! Dispatch a mouse event to the main manipulator manager via a bus call.
    void DispatchMouseInteractionEvent(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& event);

    //! Set the camera position of the specified camera state and return a copy of that state.
    AzFramework::CameraState SetCameraStatePosition(const AZ::Vector3& position, AzFramework::CameraState& cameraState);

    //! Set the camera direction of the specified camera state and return a copy of that state.
    AzFramework::CameraState SetCameraStateDirection(const AZ::Vector3& direction, AzFramework::CameraState& cameraState);

    //! Return the center of the viewport of the specified camera state.
    AzFramework::ScreenPoint GetCameraStateViewportCenter(const AzFramework::CameraState& cameraState);

    //! Default viewport size (1080p) in 16:9 aspect ratio.
    inline const auto DefaultViewportSize = AZ::Vector2(1920.0f, 1080.0f);
} // namespace AzManipulatorTestFramework
