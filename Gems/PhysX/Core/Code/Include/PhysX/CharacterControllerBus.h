/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace AZ
{
    class Vector3;
    class Quaternion;
} // namespace AZ

namespace PhysX
{
    /// Bus for PhysX specific character controller functionality.
    class CharacterControllerRequests
        : public AZ::ComponentBus
    {
    public:
        /// Changes the height of the controller, maintaining the base position.
        /// @param height The new height for the controller.
        virtual void Resize(float height) = 0;

        /// Gets the height of the controller.
        virtual float GetHeight() = 0;

        /// Sets the height of the controller, maintaining the center position.
        /// @param height The new height for the controller.
        virtual void SetHeight(float height) = 0;

        /// Gets the radius of the controller (for capsule controllers only).
        virtual float GetRadius() = 0;

        /// Sets the radius of the controller (for capsule controllers only).
        /// @param radius The new radius for the controller.
        virtual void SetRadius(float radius) = 0;

        /// Gets the half side extent of the controller (for box controllers only).
        virtual float GetHalfSideExtent() = 0;

        /// Sets the half side extent of the controller (for box controllers only).
        /// @param halfSideExtent The new half side extent for the controller.
        virtual void SetHalfSideExtent(float halfSideExtent) = 0;

        /// Gets the half forward extent of the controller (for box controllers only).
        virtual float GetHalfForwardExtent() = 0;

        /// Sets the half forward extent of the controller (for box controllers only).
        /// @param halfForwardExtent The new half forward extent for the controller.
        virtual void SetHalfForwardExtent(float halfForwardExtent) = 0;
    };
    using CharacterControllerRequestBus = AZ::EBus<CharacterControllerRequests>;
} // namespace PhysX
