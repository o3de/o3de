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
#include <AzCore/std/utils.h>
#include <AzCore/Math/Transform.h>

namespace PhysX
{
    //! Interface to communicate with PhysX joint's motor.
    class JointInterface : public AZ::EBusTraits
    {
    public:
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::EntityId;

        virtual ~JointInterface() = default;

        //! Returns current position from joint
        virtual float GetPosition() = 0;

        //! Returns velocity of joint
        virtual float GetVelocity() = 0;

        //! Sets drive velocity.
        //! @param velocity velocity in meters per second or radians per second
        virtual void SetVelocity(float velocity) = 0;

        //! Gets local transformation of joint.
        virtual AZ::Transform GetTransform() = 0;

        //! Gets limits on active joint's axis.
        virtual AZStd::pair<float, float> GetLimits() = 0;

        //! Sets maximum motor force.
        //! @param force in Newtons or Newton-meters.
        virtual void SetMaximumForce(float force) = 0;

    };

    using JointInterfaceRequestBus = AZ::EBus<JointInterface>;
} // namespace ROS2