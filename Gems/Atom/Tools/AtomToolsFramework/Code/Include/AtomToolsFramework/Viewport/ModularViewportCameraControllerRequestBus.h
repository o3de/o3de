/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzFramework/Viewport/ViewportId.h>

namespace AZ
{
    class Transform;
}

namespace AtomToolsFramework
{
    //! Provides an interface to control the modern viewport camera controller from the Editor.
    //! @note The bus is addressed by viewport id.
    class ModularViewportCameraControllerRequests : public AZ::EBusTraits
    {
    public:
        static inline constexpr float InterpolateToTransformDuration = 1.0f;

        using BusIdType = AzFramework::ViewportId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Begins a smooth transition of the camera to the requested transform.
        //! @param worldFromLocal The transform of where the camera should end up.
        //! @return Returns true if the call began an interpolation and false otherwise. Calls to InterpolateToTransform
        //! will have no effect if an interpolation is currently in progress.
        virtual bool InterpolateToTransform(const AZ::Transform& worldFromLocal) = 0;
        //! Returns if the camera is currently interpolating to a new transform.
        virtual bool IsInterpolating() const = 0;
        //! Starts tracking a transform.
        //! Store the current camera transform and move to the next camera transform.
        virtual void StartTrackingTransform(const AZ::Transform& worldFromLocal) = 0;
        //! Stops tracking the set transform.
        //! The previously stored camera transform is restored.
        virtual void StopTrackingTransform() = 0;
        //! Returns if the tracking transform is set.
        virtual bool IsTrackingTransform() const = 0;
        //! Sets the current camera pivot, moving the camera offset with it (the camera appears
        //! to follow the pivot, staying the same distance away from it).
        virtual void SetCameraPivotAttached(const AZ::Vector3& pivot) = 0;
        //! Sets the current camera pivot, leaving the camera offset in-place (the camera will
        //! stay fixed and the pivot will appear to move around on its own).
        virtual void SetCameraPivotDetached(const AZ::Vector3& pivot) = 0;
        //! Sets the current camera offset from the pivot.
        //! @note The offset value is in the current space of the camera, not world space. Setting
        //! a negative Z value will move the camera backwards from the pivot.
        virtual void SetCameraOffset(const AZ::Vector3& offset) = 0;

    protected:
        ~ModularViewportCameraControllerRequests() = default;
    };

    using ModularViewportCameraControllerRequestBus = AZ::EBus<ModularViewportCameraControllerRequests>;
} // namespace AtomToolsFramework
