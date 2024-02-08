/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <PxPhysicsAPI.h>
#include <Utils.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>
#include <PhysX/UserDataTypes.h>

namespace AzPhysics
{
    struct RigidBodyConfiguration;
}

namespace PhysX
{
    class RigidBodyComponent;
    class Shape;

    //! PhysX-specific settings which are not generic enough to be stored in the AzPhysics rigid body configuration.
    class RigidBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(RigidBodyConfiguration, AZ::SystemAllocator);
        AZ_RTTI(PhysX::RigidBodyConfiguration, "{006A06B1-E2E9-47DF-A49D-C60CAB3727C4}");

        RigidBodyConfiguration() = default;
        virtual ~RigidBodyConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);

        AZ::u8 m_solverPositionIterations = 4; //!< Higher values can improve stability at the cost of performance.
        AZ::u8 m_solverVelocityIterations = 1; //!< Higher values can improve stability at the cost of performance.
    };

    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")

    /// PhysX specific implementation of generic physics API RigidBody class.
    class RigidBody
        : public AzPhysics::RigidBody
    {
    public:
        friend class RigidBodyComponent;

        AZ_CLASS_ALLOCATOR(RigidBody, AZ::SystemAllocator);
        AZ_RTTI(PhysX::RigidBody, "{30CD41DD-9783-47A1-B935-9E5634238F45}", AzPhysics::RigidBody);

        RigidBody() = default;
        RigidBody(const AzPhysics::RigidBodyConfiguration& configuration);
        ~RigidBody();

        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 GetShapeCount() const override;
        AZStd::shared_ptr<Physics::Shape> GetShape(AZ::u32 index) override;
        AZStd::shared_ptr<const Physics::Shape> GetShape(AZ::u32 index) const override;

        AZ::Vector3 GetCenterOfMassWorld() const override;
        AZ::Vector3 GetCenterOfMassLocal() const override;

        AZ::Matrix3x3 GetInertiaWorld() const override;
        AZ::Matrix3x3 GetInertiaLocal() const override;
        AZ::Matrix3x3 GetInverseInertiaWorld() const override;
        AZ::Matrix3x3 GetInverseInertiaLocal() const override;

        float GetMass() const override;
        float GetInverseMass() const override;
        void SetMass(float mass) override;
        void SetCenterOfMassOffset(const AZ::Vector3& comOffset) override;
        AZ::Vector3 GetLinearVelocity() const override;
        void SetLinearVelocity(const AZ::Vector3& velocity) override;
        AZ::Vector3 GetAngularVelocity() const override;
        void SetAngularVelocity(const AZ::Vector3& angularVelocity) override;
        AZ::Vector3 GetLinearVelocityAtWorldPoint(const AZ::Vector3& worldPoint) const override;
        void ApplyLinearImpulse(const AZ::Vector3& impulse) override;
        void ApplyLinearImpulseAtWorldPoint(const AZ::Vector3& impulse, const AZ::Vector3& worldPoint) override;
        void ApplyAngularImpulse(const AZ::Vector3& angularImpulse) override;

        bool IsKinematic() const override;
        void SetKinematic(bool isKinematic) override;
        void SetKinematicTarget(const AZ::Transform& targetPosition) override;

        bool IsGravityEnabled() const override;
        void SetGravityEnabled(bool enabled) override;
        void SetSimulationEnabled(bool enabled) override;
        void SetCCDEnabled(bool enabled) override;

        // AzPhysics::SimulatedBody
        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;
        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;
        AZ::Aabb GetAabb() const override;
        AZ::EntityId GetEntityId() const override;

        AzPhysics::SceneQueryHit RayCast(const AzPhysics::RayCastRequest& request) override;

        // Physics::ReferenceBase
        AZ::Crc32 GetNativeType() const override;
        void* GetNativePointer() const override;

        // Not in API but needed to support PhysicsComponentBus
        float GetLinearDamping() const override;
        void SetLinearDamping(float damping) override;
        float GetAngularDamping() const override;
        void SetAngularDamping(float damping) override;

        //sleeping
        bool IsAwake() const override;
        void ForceAsleep() override;
        void ForceAwake() override;
        float GetSleepThreshold() const override;
        void SetSleepThreshold(float threshold) override;

        bool ShouldStartAsleep() const { return m_startAsleep; }

        void SetName(const AZStd::string& entityName);
        const AZStd::string& GetName() const;

        void AddShape(AZStd::shared_ptr<Physics::Shape> shape) override;
        void RemoveShape(AZStd::shared_ptr<Physics::Shape> shape) override;

        void UpdateMassProperties(AzPhysics::MassComputeFlags flags = AzPhysics::MassComputeFlags::DEFAULT,
            const AZ::Vector3& centerOfMassOffsetOverride = AZ::Vector3::CreateZero(),
            const AZ::Matrix3x3& inertiaTensorOverride = AZ::Matrix3x3::CreateIdentity(),
            const float massOverride = 1.0f) override;

    private:
        void CreatePhysXActor(const AzPhysics::RigidBodyConfiguration& configuration);

        void UpdateCenterOfMass(bool includeAllShapesInMassCalculation);
        void SetInertia(const AZ::Matrix3x3& inertia);

        AZStd::shared_ptr<physx::PxRigidDynamic> m_pxRigidActor;
        AZStd::vector<AZStd::shared_ptr<PhysX::Shape>> m_shapes;
        AZStd::string m_name;
        PhysX::ActorData m_actorUserData;
        bool m_startAsleep = false;
    };

    AZ_POP_DISABLE_WARNING
} // namespace PhysX
