/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/optional.h>
#include <AzFramework/Viewport/ViewportId.h>

namespace AZ
{
    class Matrix4x4;
    class Matrix3x4;
    class Transform;
    class ReflectContext;
} // namespace AZ

namespace AzFramework
{
    class ViewportRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ViewportId;

        static void Reflect(AZ::ReflectContext* context);

        //! Gets the current camera's world to view matrix.
        virtual const AZ::Matrix4x4& GetCameraViewMatrix() const = 0;
        //! Gets the current camera's world to view matrix as a Matrix3x4.
        virtual AZ::Matrix3x4 GetCameraViewMatrixAsMatrix3x4() const = 0;
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

    protected:
        ~ViewportRequests() = default;
    };

    using ViewportRequestBus = AZ::EBus<ViewportRequests>;

    //! The additional padding around the viewport when a viewport border is active.
    struct ViewportBorderPadding
    {
        float m_top;
        float m_bottom;
        float m_left;
        float m_right;
    };

    //! For performing queries about the state of the viewport border.
    class ViewportBorderRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = ViewportId;

        //! Returns if a viewport border is in effect and what the current dimensions (padding) of the border are.
        virtual AZStd::optional<ViewportBorderPadding> GetViewportBorderPadding() const = 0;

    protected:
        ~ViewportBorderRequests() = default;
    };

    using ViewportBorderRequestBus = AZ::EBus<ViewportBorderRequests>;
} // namespace AzFramework
