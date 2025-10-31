/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/utils.h>

namespace PhysX
{
    //! Interface to communicate with PhysX joint's motor.
    class JointRequests : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityComponentIdPair;

        //! Returns current position from joint.
        //! It is a relative position of the entity in the direction of the free axis of the joint.
        //! For a hinge joint it is a twist angle (in radians), for the prismatic joint it is travel (in meters).
        virtual float GetPosition() const = 0;

        //! Returns velocity.
        //! It is a relative velocity of the entity in the direction of the free axis of the joint.
        //! For a hinge joint rotation velocity in radians per second. For prismatic joint it is linear velocity in
        //! meters per second.
        virtual float GetVelocity() const = 0;

        //! Sets drive velocity.
        //! @param velocity velocity in meters per second (for prismatic joint) or radians per second (for hinge joint)
        virtual void SetVelocity(float velocity) = 0;

        //! Gets local transformation of joint.
        virtual AZ::Transform GetTransform() const = 0;

        //! Gets limits on active joint's axis.
        virtual AZStd::pair<float, float> GetLimits() const = 0;

        //! Sets maximum motor force.
        //! @param force in Newtons (for prismatic joint) or Newton-meters (for hinge joint).
        virtual void SetMaximumForce(float force) = 0;
    };

    using JointRequestBus = AZ::EBus<JointRequests>;
} // namespace PhysX
