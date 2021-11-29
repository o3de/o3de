/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>

namespace Physics
{
    class ShapeConfiguration;
    class Shape;
}

namespace AzPhysics
{
    //! Dynamic rigid body.
    struct RigidBody
        : public SimulatedBody
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_RTTI(AzPhysics::RigidBody, "{156E459F-7BB7-4B4E-ADA0-2130D96B7E80}", AzPhysics::SimulatedBody);

        RigidBody() = default;

        virtual void AddShape(AZStd::shared_ptr<Physics::Shape> shape) = 0;
        virtual void RemoveShape(AZStd::shared_ptr<Physics::Shape> shape) = 0;
        virtual AZ::u32 GetShapeCount() { return 0; }
        virtual AZStd::shared_ptr<Physics::Shape> GetShape([[maybe_unused]]AZ::u32 index) { return nullptr; }

        virtual AZ::Vector3 GetCenterOfMassWorld() const = 0;
        virtual AZ::Vector3 GetCenterOfMassLocal() const = 0;

        virtual AZ::Matrix3x3 GetInverseInertiaWorld() const = 0;
        virtual AZ::Matrix3x3 GetInverseInertiaLocal() const = 0;

        virtual float GetMass() const = 0;
        virtual float GetInverseMass() const = 0;
        virtual void SetMass(float mass) = 0;
        virtual void SetCenterOfMassOffset(const AZ::Vector3& comOffset) = 0;

        //! Retrieves the velocity at center of mass; only linear velocity, no rotational velocity contribution.
        virtual AZ::Vector3 GetLinearVelocity() const = 0;
        virtual void SetLinearVelocity(const AZ::Vector3& velocity) = 0;
        virtual AZ::Vector3 GetAngularVelocity() const = 0;
        virtual void SetAngularVelocity(const AZ::Vector3& angularVelocity) = 0;
        virtual AZ::Vector3 GetLinearVelocityAtWorldPoint(const AZ::Vector3& worldPoint) const = 0;
        virtual void ApplyLinearImpulse(const AZ::Vector3& impulse) = 0;
        virtual void ApplyLinearImpulseAtWorldPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldPoint) = 0;
        virtual void ApplyAngularImpulse(const AZ::Vector3& angularImpulse) = 0;

        virtual float GetLinearDamping() const = 0;
        virtual void SetLinearDamping(float damping) = 0;
        virtual float GetAngularDamping() const = 0;
        virtual void SetAngularDamping(float damping) = 0;

        virtual bool IsAwake() const = 0;
        virtual void ForceAsleep() = 0;
        virtual void ForceAwake() = 0;
        virtual float GetSleepThreshold() const = 0;
        virtual void SetSleepThreshold(float threshold) = 0;

        virtual bool IsKinematic() const = 0;
        virtual void SetKinematic(bool kinematic) = 0;
        virtual void SetKinematicTarget(const AZ::Transform& targetPosition) = 0;

        virtual bool IsGravityEnabled() const = 0;
        virtual void SetGravityEnabled(bool enabled) = 0;
        virtual void SetSimulationEnabled(bool enabled) = 0;
        virtual void SetCCDEnabled(bool enabled) = 0;

        //! Recalculates mass, inertia and center of mass based on the flags passed.
        //! @param flags MassComputeFlags specifying which properties should be recomputed.
        //! @param centerOfMassOffsetOverride Optional override of the center of mass. Note: This parameter will be ignored if COMPUTE_COM is passed in flags.
        //! @param inertiaTensorOverride Optional override of the inertia. Note: This parameter will be ignored if COMPUTE_INERTIA is passed in flags.
        //! @param massOverride Optional override of the mass. Note: This parameter will be ignored if COMPUTE_MASS is passed in flags.
        virtual void UpdateMassProperties(MassComputeFlags flags = MassComputeFlags::DEFAULT,
            const AZ::Vector3& centerOfMassOffsetOverride = AZ::Vector3::CreateZero(),
            const AZ::Matrix3x3& inertiaTensorOverride = AZ::Matrix3x3::CreateIdentity(),
            const float massOverride = 1.0f) = 0;
    };
   
} // namespace AzPhysics
