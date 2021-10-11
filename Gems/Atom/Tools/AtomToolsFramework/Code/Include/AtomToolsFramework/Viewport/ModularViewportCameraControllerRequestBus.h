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
        using BusIdType = AzFramework::ViewportId;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        //! Begin a smooth transition of the camera to the requested transform.
        //! @param worldFromLocal The transform of where the camera should end up.
        virtual void InterpolateToTransform(const AZ::Transform& worldFromLocal) = 0;

        //! Return the current reference frame.
        //! @note If a reference frame has not been set or a frame has been cleared, this is just the identity.
        virtual AZ::Transform GetReferenceFrame() const = 0;

        //! Set a new reference frame other than the identity for the camera controller.
        virtual void SetReferenceFrame(const AZ::Transform& worldFromLocal) = 0;

        //! Clear the current reference frame to restore the identity.
        virtual void ClearReferenceFrame() = 0;

    protected:
        ~ModularViewportCameraControllerRequests() = default;
    };

    using ModularViewportCameraControllerRequestBus = AZ::EBus<ModularViewportCameraControllerRequests>;
} // namespace AtomToolsFramework
