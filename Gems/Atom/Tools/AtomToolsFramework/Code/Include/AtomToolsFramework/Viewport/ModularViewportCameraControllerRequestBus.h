/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        using BusIdType = AzFramework::ViewportId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Begin a smooth transition of the camera to the requested transform.
        //! @param worldFromLocal The transform of where the camera should end up.
        //! @param lookAtDistance The distance between the camera transform and the imagined look at point.
        virtual void InterpolateToTransform(const AZ::Transform& worldFromLocal, float lookAtDistance) = 0;

        //! Look at point after an interpolation has finished and no translation has occurred.
        virtual AZStd::optional<AZ::Vector3> LookAtAfterInterpolation() const = 0;

    protected:
        ~ModularViewportCameraControllerRequests() = default;
    };

    using ModularViewportCameraControllerRequestBus = AZ::EBus<ModularViewportCameraControllerRequests>;
} // namespace AtomToolsFramework
