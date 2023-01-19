/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzFramework/Viewport/ViewportId.h>

namespace AZ
{
    class Transform;
}

namespace AzFramework
{
    class CameraInput;
}

namespace AtomToolsFramework
{
    //! Provides an interface to control the modern viewport camera controller from the Editor.
    //! @note The bus is addressed by viewport id.
    class ModularViewportCameraControllerRequests : public AZ::EBusTraits
    {
    public:
        using BusIdType = AzFramework::ViewportId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Begins a smooth transition of the camera to the requested transform.
        //! @param worldFromLocal The transform of where the camera should end up.
        //! @return Returns true if the call began an interpolation and false otherwise. Calls to InterpolateToTransform
        //! will have no effect if an interpolation is currently in progress.
        virtual bool InterpolateToTransform(const AZ::Transform& worldFromLocal, float duration) = 0;
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
        //! Same as SetCameraPivotAttached only the pivot is set on the current and target cameras so no interpolation occurs.
        virtual void SetCameraPivotAttachedImmediate(const AZ::Vector3& pivot) = 0;
        //! Sets the current camera pivot, leaving the camera offset in-place (the camera will
        //! stay fixed and the pivot will appear to move around on its own).
        virtual void SetCameraPivotDetached(const AZ::Vector3& pivot) = 0;
        //! Same as SetCameraPivotDetached only the pivot is set on the current and target cameras so no interpolation occurs.
        virtual void SetCameraPivotDetachedImmediate(const AZ::Vector3& pivot) = 0;
        //! Sets the current camera offset from the pivot.
        //! @note The offset value is in the current space of the camera, not world space. Setting
        //! a negative Z value will move the camera backwards from the pivot.
        virtual void SetCameraOffset(const AZ::Vector3& offset) = 0;
        //! Same as SetCameraOffset only the offset is set on the current and target cameras so no interpolation occurs.
        virtual void SetCameraOffsetImmediate(const AZ::Vector3& offset) = 0;
        //! Transitions a camera from an orbit state (pivot and non-zero offset) to a look state (pivot and zero offset).
        virtual void LookFromOrbit() = 0;
        //! Add one or more camera inputs (behaviors) to run for the current camera.
        virtual bool AddCameras(const AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>& cameraInputs) = 0;
        //! Remove one or more camera inputs (behaviors) to stop them running for the current camera.
        virtual bool RemoveCameras(const AZStd::vector<AZStd::shared_ptr<AzFramework::CameraInput>>& cameraInputs) = 0;
        //! Reset the state of all camera inputs (clear inputs from running).
        virtual void ResetCameras() = 0;

    protected:
        ~ModularViewportCameraControllerRequests() = default;
    };

    using ModularViewportCameraControllerRequestBus = AZ::EBus<ModularViewportCameraControllerRequests>;
} // namespace AtomToolsFramework
