/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzFramework/Viewport/ViewportId.h>
#include <AzCore/EBus/EBus.h>

namespace AZ
{
    class Matrix4x4;
    class Transform;
    class ReflectContext;
} // namespace AZ

namespace AzFramework
{
    class ViewportRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy  = AZ::EBusAddressPolicy::ById;
        using BusIdType = ViewportId;

        static void Reflect(AZ::ReflectContext* context);

        virtual ~ViewportRequests() {}

        //! Gets the current camera's world to view matrix.
        virtual const AZ::Matrix4x4& GetCameraViewMatrix() const = 0;
        //! Sets the current camera's world to view matrix.
        virtual void SetCameraViewMatrix(const AZ::Matrix4x4& matrix) = 0;
        //! Gets the current camera's projection (view to clip) matrix.
        virtual const AZ::Matrix4x4& GetCameraProjectionMatrix() const = 0;
        //! Sets the current camera's projection (view to clip) matrix.
        virtual void SetCameraProjectionMatrix(const AZ::Matrix4x4& matrix) = 0;
        //! Convenience method, gets the AZ::Transform corresponding to this camera's world to view matrix.
        virtual AZ::Transform GetCameraTransform() const = 0;
        //! Convenience method, sets the camera's world to view matrix from this AZ::Transform.
        virtual void SetCameraTransform(const AZ::Transform& transform) = 0;
    };

    using ViewportRequestBus = AZ::EBus<ViewportRequests>;

} //namespace AzFramework
