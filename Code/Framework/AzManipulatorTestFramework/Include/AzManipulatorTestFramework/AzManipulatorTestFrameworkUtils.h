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
