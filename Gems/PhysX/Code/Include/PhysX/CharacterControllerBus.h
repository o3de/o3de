/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

#if defined(CARBONATED)
#include <PhysX/Material/PhysXMaterial.h>
#endif // defined(CARBONATED)

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

#if defined(CARBONATED)
        // Methods added to setup character collider so that to differentiate character collisions from other collisions
        /// Changes collider layer and group to given names.
        virtual void SetCharacterCollisions(const AZStd::string& layer, const AZStd::string& group) = 0;

        /// Changes Physics::MaterialAsset in the slot referenced by index. Index 0 usually correponds to "Entire object" slot.
        virtual void SetMaterial(uint32_t index, const AZ::Data::Asset<Physics::MaterialAsset>& materialAsset) = 0;

        /// Changes collider Tag to given AZ::Crc32(AZStd::string tagName).
        virtual void SetTag(const AZ::Crc32& tag) = 0;
#endif // defined(CARBONATED)
    };
    using CharacterControllerRequestBus = AZ::EBus<CharacterControllerRequests>;
} // namespace PhysX
