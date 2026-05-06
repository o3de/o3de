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
    class Transform;
}

namespace PhysX
{
    //! Interface to communicate with the articulation cache in a PhysX reduced co-ordinate articulation.
    class ArticulationCacheRequests : public AZ::ComponentBus
    {
    public:
        // TODO: Add additional getters for joints (may require degree of freedom helper function)

        //! Get the linear velocity for a link recorded to the articulation cache.
        //! The link index differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetLinkLinearVelocity(AZ::u32 linkIndex) const = 0;

        //! Get the angular velocity for a link recorded to the articulation cache.
        //! The link index differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetLinkAngularVelocity(AZ::u32 linkIndex) const = 0;
        
        //! Get the linear acceleration for a link recorded to the articulation cache.
        //! The link index differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetLinkLinearAcceleration(AZ::u32 linkIndex) const = 0;

        //! Get the angular acceleration for a link recorded to the articulation cache.
        //! The link index differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetLinkAngularAcceleration(AZ::u32 linkIndex) const = 0;

        //! Get the root link transform recorded to the articulation cache.
        virtual AZ::Transform GetRootLinkTransform() const = 0;

        //! Get the root link linear velocity recorded to the articulation cache.
        virtual AZ::Vector3 GetRootLinkLinearVelocity() const = 0;

        //! Get the root link angular velocity recorded to the articulation cache.
        virtual AZ::Vector3 GetRootLinkAngularVelocity() const = 0;

        //! Get the force for a link recorded to the articulation cache.
        //! The link index differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetLinkForce(AZ::u32 linkIndex) const = 0;

        //! Get the torque reported recorded to the articulation cache.
        //! The link index differs from the per-actuation indices used internally by PhysX.
        virtual AZ::Vector3 GetLinkTorque(AZ::u32 linkIndex) const = 0;
    };

    using ArticulationCacheRequestBus = AZ::EBus<ArticulationCacheRequests>;
} // namespace PhysX
