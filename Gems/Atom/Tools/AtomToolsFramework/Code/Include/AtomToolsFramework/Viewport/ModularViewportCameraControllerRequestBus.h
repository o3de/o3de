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

        //! Begin a smooth transition of the camera to the requested transform.
        //! @param worldFromLocal The transform of where the camera should end up.
        //! @return Returns true if the call began an interpolation and false otherwise. Calls to InterpolateToTransform
        //! will have no effect if an interpolation is currently in progress.
        virtual bool InterpolateToTransform(const AZ::Transform& worldFromLocal) = 0;

        //! Returns if the camera is currently interpolating to a new transform.
        virtual bool IsInterpolating() const = 0;

        //! Start tracking a transform.
        //! Store the current camera transform and move to the next camera transform.
        virtual void StartTrackingTransform(const AZ::Transform& worldFromLocal) = 0;

        //! Stop tracking the set transform.
        //! The previously stored camera transform is restored.
        virtual void StopTrackingTransform() = 0;

        //! Return if the tracking transform is set.
        virtual bool IsTrackingTransform() const = 0;

    protected:
        ~ModularViewportCameraControllerRequests() = default;
    };

    using ModularViewportCameraControllerRequestBus = AZ::EBus<ModularViewportCameraControllerRequests>;
} // namespace AtomToolsFramework
