/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Vector3.h>
#include <AzFramework/Physics/Common/PhysicsSceneQueries.h>
#include <AzFramework/Physics/Components/SimulatedBodyComponentBus.h>

namespace AzPhysics
{
    struct RigidBody;
}

namespace Physics
{
    //! Requests interface for a rigid body (static or dynamic).
    class RigidBodyRequests
        : public AZ::ComponentBus
    {
    public:
        using MutexType = AZStd::recursive_mutex;

        virtual void EnablePhysics() = 0;
        virtual void DisablePhysics() = 0;
        virtual bool IsPhysicsEnabled() const = 0;

        virtual AZ::Vector3 GetCenterOfMassWorld() const = 0;
        virtual AZ::Vector3 GetCenterOfMassLocal() const = 0;

        virtual AZ::Matrix3x3 GetInertiaWorld() const = 0;
        virtual AZ::Matrix3x3 GetInertiaLocal() const = 0;
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

    //! Notifications interface for a rigid body (static or dynamic).
    class RigidBodyNotifications
        : public AZ::ComponentBus
    {
    private:
        template<class Bus>
        struct RigidBodyNotificationsConnectionPolicy : public AZ::EBusConnectionPolicy<Bus>
        {
            static void Connect(
                typename Bus::BusPtr& busPtr,
                typename Bus::Context& context,
                typename Bus::HandlerNode& handler,
                typename Bus::Context::ConnectLockGuard& connectLock,
                const typename Bus::BusIdType& id = 0)
            {
                AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                AZ::Entity* entity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, id);
                if (entity)
                {
                    // Only immediately dispatch if the entity is already active, otherwise when
                    // entity will get activated it will send the notifications itself.
                    const AZ::Entity::State entityState = entity->GetState();
                    if (entityState == AZ::Entity::State::Active)
                    {
                        // Only immediately dispatch if the entity is a RigidBodyRequestBus' handler.
                        AzPhysics::SimulatedBodyComponentRequestsBus::EnumerateHandlersId(
                            id,
                            [&handler, id](const AzPhysics::SimulatedBodyComponentRequests* simulatedBodyhandler)
                            {
                                if (simulatedBodyhandler->IsPhysicsEnabled())
                                {
                                    handler->OnPhysicsEnabled(id);
                                }
                                else
                                {
                                    handler->OnPhysicsDisabled(id);
                                }
                                return true;
                            });
                    }
                }
            }
        };

    public:
        //! With this connection policy, RigidBodyNotifications::OnPhysicsEnabled and
        //! RigidBodyNotifications::OnPhysicsDisabled events will be immediately
        //! dispatched when a handler connects to the bus.
        template<class Bus>
        using ConnectionPolicy = RigidBodyNotificationsConnectionPolicy<Bus>;

        virtual void OnPhysicsEnabled([[maybe_unused]] const AZ::EntityId& entityId)
        {
        }

        virtual void OnPhysicsDisabled([[maybe_unused]] const AZ::EntityId& entityId)
        {
        }
    };

    using RigidBodyNotificationBus = AZ::EBus<RigidBodyNotifications>;
}
