/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace AZ
{
    class Transform;
    class Matrix3x3;
    class Vector3;
}

namespace AzFramework
{
    //! The debug camera allows the user control over the view through mouse + keyboard and/or
    //! controller while the game still uses the view camera for everything else. This can for
    //! instance be used to debug occlusion culling as all occlusion calculates will be done
    //! from the view camera, so the debug camera makes it possible to check if hidden objects
    //! are correctly culled.
    //! This class can be useful to validate a hypothetical camera-based look-ahead asset streaming system.
    //! The developer can update the camera location using this EBus, without requiring to move the viewport camera.
    class DebugCameraInterface
        : public AZ::EBusTraits
    {
    public:
        enum class Mode
        {
            FreeFloating, //< Controls move the debug camera through the world.
            Fixed, //< The debug camera stays in the position it was navigated to and control is handed back to the game.
            Disabled, //< Debug camera is disabled.

            Unknown
        };

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Sets the debug camera in free floating, fixed or disabled mode.
        virtual void SetMode(Mode mode) = 0;
        //! Returns the current mode the debug camera is in.
        virtual Mode GetMode() const = 0;

        //! Retrieves the world position of the debug camera. This is the same position that can be retrieved
        //! from GetTransform.
        virtual void GetPosition(AZ::Vector3& result) const = 0;
        //! Retrieves the view orientation of the debub camera. This is the same orientation that can be retrieved
        //! from GetTransform.
        virtual void GetView(AZ::Matrix3x3& result) const = 0;
        //! Get the world transform for the debug camera.
        virtual void GetTransform(AZ::Transform& result) const = 0;
    };
    using DebugCameraBus = AZ::EBus<DebugCameraInterface>;

    //! The debug camera sends out notifications about some changes. This interface provides access to these.
    class DebugCameraEventsInterface
        : public AZ::EBusTraits
    {
    public:
        //! Called when the debug camera moves, usually due to user interaction.
        virtual void DebugCameraMoved(const AZ::Transform& world) {}
    };
    using DebugCameraEventsBus = AZ::EBus<DebugCameraEventsInterface>;
} // namespace AzFramework
