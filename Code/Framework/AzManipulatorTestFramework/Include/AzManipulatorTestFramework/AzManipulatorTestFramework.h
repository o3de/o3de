/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/Manipulators/ManipulatorBus.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>

namespace AzFramework
{
    class DebugDisplayRequests;
    struct CameraState;
}

namespace AzManipulatorTestFramework
{
    //! This interface is used to simulate the editor environment while the manipulators are under test.
    class ViewportInteractionInterface
    {
    public:
        virtual ~ViewportInteractionInterface() = default;
        //! Return the camera state.
        virtual AzFramework::CameraState GetCameraState() = 0;
        //! Set the camera state.
        virtual void SetCameraState(const AzFramework::CameraState& cameraState) = 0;
        //! Retrieve the debug display.
        virtual AzFramework::DebugDisplayRequests& GetDebugDisplay() = 0;
        //! Enable grid snapping.
        virtual void EnableGridSnaping() = 0;
        //! Disable grid snapping.
        virtual void DisableGridSnaping() = 0;
        //! Enable grid snapping.
        virtual void EnableAngularSnaping() = 0;
        //! Disable grid snapping.
        virtual void DisableAngularSnaping() = 0;
        //! Set the grid size.
        virtual void SetGridSize(float size) = 0;
        //! Set the angular step.
        virtual void SetAngularStep(float step) = 0;
        //! Get the viewport id.
        virtual int GetViewportId() const = 0;
    };

    //! This interface is used to simulate the manipulator manager while the manipulators are under test.
    class ManipulatorManagerInterface
    {
    public:
        virtual ~ManipulatorManagerInterface() = default;
        //! Consume and immediately act on the specified mouse event.
        virtual void ConsumeMouseInteractionEvent(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& event) = 0;
        //! Selected manipulator is currently interacting.
        virtual bool ManipulatorBeingInteracted() const = 0;
        //! Return the id of the manipulator manager.
        virtual AzToolsFramework::ManipulatorManagerId GetId() const = 0;
    };

    //! This interface is used to simulate the combined manipulator manager and editor environment while the manipulators are under test.
    class ManipulatorViewportInteraction
    {
    public:
        virtual ~ManipulatorViewportInteraction() = default;
        //! Return the const representation of the viewport interaction model.
        virtual const ViewportInteractionInterface& GetViewportInteraction() const = 0;
        //! Return the const representation of the manipulator manager.
        virtual const ManipulatorManagerInterface& GetManipulatorManager() const = 0;

        //! Convenience wrapper for getting the manipulator manager id.
        AzToolsFramework::ManipulatorManagerId GetManipulatorManagerId() const
        {
            return GetManipulatorManager().GetId();
        }

        //! Return the representation of the viewport interaction model.
        ViewportInteractionInterface& GetViewportInteraction()
        {
            return const_cast<
                ViewportInteractionInterface&>(const_cast<const ManipulatorViewportInteraction*>(this)->GetViewportInteraction());
        }

        //! Return the const representation of the manipulator manager.
        ManipulatorManagerInterface& GetManipulatorManager()
        {
            return const_cast<
                ManipulatorManagerInterface&>(const_cast<const ManipulatorViewportInteraction*>(this)->GetManipulatorManager());
        }
    };
} // namespace AzManipulatorTestFramework
