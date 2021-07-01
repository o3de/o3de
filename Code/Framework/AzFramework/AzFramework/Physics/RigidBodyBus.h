/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>

namespace AzPhysics
{
    struct RigidBody;
}

namespace Physics
{

    class RigidBodyRequests
        : public AZ::ComponentBus
    {
    public:
        using MutexType = AZStd::mutex;

        virtual void EnablePhysics() = 0;
        virtual void DisablePhysics() = 0;
        virtual bool IsPhysicsEnabled() const = 0;

        virtual AZ::Vector3 GetCenterOfMassWorld() const = 0;
        virtual AZ::Vector3 GetCenterOfMassLocal() const = 0;

        virtual AZ::Matrix3x3 GetInverseInertiaWorld() const = 0;
        virtual AZ::Matrix3x3 GetInverseInertiaLocal() const = 0;

        virtual float GetMass() const = 0;
        virtual float GetInverseMass() const = 0;
        virtual void SetMass(float mass) = 0;
        virtual void SetCenterOfMassOffset(const AZ::Vector3& comOffset) = 0;
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

        virtual AZ::Aabb GetAabb() const = 0;
        virtual AzPhysics::RigidBody* GetRigidBody() = 0;

        virtual AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) = 0;
    };

    using RigidBodyRequestBus = AZ::EBus<RigidBodyRequests>;


    class RigidBodyNotifications
        : public AZ::ComponentBus
    {
    public:
        virtual void OnPhysicsEnabled() = 0;
        virtual void OnPhysicsDisabled() = 0;
    };

    using RigidBodyNotificationBus = AZ::EBus<RigidBodyNotifications>;
}
