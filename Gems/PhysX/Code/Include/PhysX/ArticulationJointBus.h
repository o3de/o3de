/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/std/utils.h>
#include <PhysX/ArticulationTypes.h>

namespace PhysX
{
    //! Interface to communicate with a joint in a PhysX reduced co-ordinate articulation.
    class ArticulationJointRequests : public AZ::ComponentBus
    {
    public:
        //! Set whether the degree of freedom for the specified axis is disabled, enabled with limits or enabled without limits.
        virtual void SetMotion(ArticulationJointAxis jointAxis, ArticulationJointMotionType jointMotionType) = 0;

        //! Get whether the degree of freedom for the specified axis is disabled, enabled with limits or enabled without limits.
        virtual ArticulationJointMotionType GetMotion(ArticulationJointAxis jointAxis) const = 0;

        //! Set the lower and upper limits for the motion corresponding to the specified axis.
        //! For linear degrees of freedom, the limits will be distances in meters.
        //! For rotational degrees of freedom, the limits will be angles in radians.
        virtual void SetLimit(ArticulationJointAxis jointAxis, AZStd::pair<float, float> limitPair) = 0;

        //! Get the lower and upper limits for the motion corresponding to the specified axis.
        //! For linear degrees of freedom, the limits will be distances in meters.
        //! For rotational degrees of freedom, the limits will be angles in radians.
        virtual AZStd::pair<float, float> GetLimit(ArticulationJointAxis jointAxis) const = 0;

        //! Set the drive stiffness for the joint drive for the motion associated with the given axis.
        //! The drive stiffness affects how strongly the joint drive responds to the difference between the target and actual joint position
        //! or rotation.
        virtual void SetDriveStiffness(ArticulationJointAxis jointAxis, float stiffness) = 0;

        //! Get the drive stiffness for the joint drive for the motion associated with the given axis.
        //! The drive stiffness affects how strongly the joint drive responds to the difference between the target and actual joint position
        //! or rotation.
        virtual float GetDriveStiffness(ArticulationJointAxis jointAxis) const = 0;

        //! Set the drive damping for the joint drive for the motion associated with the given axis.
        //! The drive stiffness affects how strongly the joint drive responds to the difference between the target and actual joint
        //! velocity or angular velocity.
        virtual void SetDriveDamping(ArticulationJointAxis jointAxis, float damping) = 0;

        //! Get the drive damping for the joint drive for the motion associated with the given axis.
        //! The drive stiffness affects how strongly the joint drive responds to the difference between the target and actual joint
        //! velocity or angular velocity.
        virtual float GetDriveDamping(ArticulationJointAxis jointAxis) const = 0;

        //! Set the maximum force for the joint drive for the motion associated with the given axis.
        virtual void SetMaxForce(ArticulationJointAxis jointAxis, float maxForce) = 0;

        //! Get the maximum force for the joint drive for the motion associated with the given axis.
        virtual float GetMaxForce(ArticulationJointAxis jointAxis) const = 0;

        //! Set whether the joint drive for the motion associated with the given axis operates in terms of acceleration (true) or force
        //! (false).
        virtual void SetIsAccelerationDrive(ArticulationJointAxis jointAxis, bool isAccelerationDrive) = 0;

        //! Get whether the joint drive for the motion associated with the given axis operates in terms of acceleration (true) or force
        //! (false).
        virtual bool IsAccelerationDrive(ArticulationJointAxis jointAxis) const = 0;

        //! Set the target position for the motion associated with the given axis.
        //! The target will be a position in meters for a linear degree of freedom, or an angle in radians for a rotational degree of
        //! freedom.
        virtual void SetDriveTarget(ArticulationJointAxis jointAxis, float target) = 0;

        //! Get the target position for the motion associated with the given axis.
        //! The target will be a position in meters for a linear degree of freedom, or an angle in radians for a rotational degree of
        //! freedom.
        virtual float GetDriveTarget(ArticulationJointAxis jointAxis) const = 0;
        
        //! Set the target velocity for the motion associated with the given axis.
        //! The target velocity will be a linear velocity in meters per second for a linear degree of freedom, or an angular velocity in
        //! radians per second for a rotational degree of freedom.
        virtual void SetDriveTargetVelocity(ArticulationJointAxis jointAxis, float targetVelocity) = 0;

        //! Get the target velocity for the motion associated with the given axis.
        //! The target velocity will be a linear velocity in meters per second for a linear degree of freedom, or an angular velocity in
        //! radians per second for a rotational degree of freedom.
        virtual float GetDriveTargetVelocity(ArticulationJointAxis jointAxis) const = 0;

        //! Set the joint friction coefficient for all the degrees of freedom of this joint.
        virtual void SetFrictionCoefficient(float frictionCoefficient) = 0;

        //! Get the joint friction coefficient for all the degrees of freedom of this joint.
        virtual float GetFrictionCoefficient() const = 0;

        //! Set the maximum joint velocity for all the degrees of freedom of this joint.
        virtual void SetMaxJointVelocity(float maxJointVelocity) = 0;

        //! Get the maximum joint velocity for all the degrees of freedom of this joint.
        virtual float GetMaxJointVelocity() const = 0;

        //! Get the current joint position.
        virtual float GetJointPosition(ArticulationJointAxis jointAxis) const = 0;

        //! Get the current joint position.
        virtual float GetJointVelocity(ArticulationJointAxis jointAxis) const = 0;


    };

    using ArticulationJointRequestBus = AZ::EBus<ArticulationJointRequests>;
} // namespace PhysX
