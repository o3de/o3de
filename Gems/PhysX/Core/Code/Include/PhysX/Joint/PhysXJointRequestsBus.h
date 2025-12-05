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

        virtual AZStd::pair<AZ::Vector3, AZ::Vector3> GetVelocityGeneral() const { return (AZ::Vector3::CreateAxisX(GetVelocity()), AZ::Vector3::CreateZero());}

        //! Sets drive velocity.
        //! @param velocity velocity in meters per second (for prismatic joint) or radians per second (for hinge joint)
        virtual void SetVelocity(float velocity) = 0;

        virtual void SetVelocityGeneral(const AZ::Vector3& linear, const AZ::Vector3& angular) { SetVelocity(linear.GetX()); }

        //! Gets local transformation of joint.
        //! Returns the local transform of the joint relative to its parent entity.
        //! @return Local transform representing joint position and orientation
        virtual AZ::Transform GetTransform() const = 0;

        //! Gets limits on active joint's axis.
        //! Returns the minimum and maximum limits for the joint's primary axis.
        //! @return Pair of floats (min, max) in radians for angular joints, meters for linear joints
        virtual AZStd::pair<float, float> GetLimits() const = 0;

        //! Gets limits for given axis id.
        //! The mapping from id to axis is joint specific. For multi-axis joints like D6, this allows
        //! querying limits of individual axes.
        //! @param id Axis identifier (joint-specific mapping)
        //! @return Pair of floats (min, max) in radians for angular axes, meters for linear axes
        virtual AZStd::pair<float, float> GetLimitsAxis(int id) const { return GetLimits(); }

        //! Sets maximum motor force.
        //! @param force in Newtons (for prismatic joint) or Newton-meters (for hinge joint).
        virtual void SetMaximumForce(float force) = 0;

        //! Sets maximum motor force for given axis id.
        //! The mapping from id to axis is joint specific. For multi-axis joints like D6, this allows
        //! controlling maximum force of individual axis drives.
        //! @param id Axis identifier (joint-specific mapping)
        //! @param force Maximum force in Newtons (for linear axes) or Newton-meters (for angular axes)
        virtual void SetMaximumForceAxis(int id, float force) { SetMaximumForce(force); }

        //! Returns underlying, cached native joint pointer, if available.
        //! Note that is quite unsafe to use this pointer, since you need to ensure the joint type is valid and available.
        //! Depending on the joint type, you may need to cast it to appropriate type (e.g., PxHingeJoint*, PxPrismaticJoint*, etc).
        //! Prefer using the JointRequestBus interface instead, unless you really need to access some specific functionality
        //! that is not exposed through the bus.
        virtual void * GetNativeJointPointer() const
        {
            return nullptr;
        }
    };

    using JointRequestBus = AZ::EBus<JointRequests>;
} // namespace PhysX
