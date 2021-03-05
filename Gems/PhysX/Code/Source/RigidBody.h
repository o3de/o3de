/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <PxPhysicsAPI.h>
#include <Utils.h>
#include <AzFramework/Physics/RigidBody.h>
#include <PhysX/UserDataTypes.h>

namespace PhysX
{
    class RigidBodyComponent;
    class Shape;

    AZ_PUSH_DISABLE_WARNING(4996, "-Wdeprecated-declarations")

    /// PhysX specific implementation of generic physics API RigidBody class.
    class RigidBody
        : public Physics::RigidBody
    {
    public:
        friend class RigidBodyComponent;

        AZ_CLASS_ALLOCATOR(RigidBody, AZ::SystemAllocator, 0);
        AZ_RTTI(RigidBody, "{30CD41DD-9783-47A1-B935-9E5634238F45}", Physics::RigidBody);

        RigidBody() = default;
        RigidBody(const Physics::RigidBodyConfiguration& configuration);
        ~RigidBody();

        static void Reflect(AZ::ReflectContext* context);

        AZ::u32 GetShapeCount() override;
        AZStd::shared_ptr<Physics::Shape> GetShape(AZ::u32 index) override;

        AZ::Vector3 GetCenterOfMassWorld() const override;
        AZ::Vector3 GetCenterOfMassLocal() const override;

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
        AZ::Vector3 GetLinearVelocityAtWorldPoint(const AZ::Vector3& worldPoint) override;
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

        // Physics::WorldBody
        Physics::World* GetWorld() const override;
        AZ::Transform GetTransform() const override;
        void SetTransform(const AZ::Transform& transform) override;
        AZ::Vector3 GetPosition() const override;
        AZ::Quaternion GetOrientation() const override;
        AZ::Aabb GetAabb() const override;
        AZ::EntityId GetEntityId() const override;

        Physics::RayCastHit RayCast(const Physics::RayCastRequest& request) override;

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

        void AddToWorld(Physics::World&) override;
        void RemoveFromWorld(Physics::World&) override;

        void SetName(const AZStd::string& entityName);
        const AZStd::string& GetName() const;

        void AddShape(AZStd::shared_ptr<Physics::Shape> shape) override;
        void RemoveShape(AZStd::shared_ptr<Physics::Shape> shape) override;

        void UpdateMassProperties(Physics::MassComputeFlags flags = Physics::MassComputeFlags::DEFAULT,
            const AZ::Vector3* centerOfMassOffsetOverride = nullptr,
            const AZ::Matrix3x3* inertiaTensorOverride = nullptr,
            const float* massOverride = nullptr) override;

    private:
        AZStd::shared_ptr<physx::PxRigidDynamic> m_pxRigidActor;
        AZStd::vector<AZStd::shared_ptr<PhysX::Shape>> m_shapes;
        AZStd::string m_name;
        PhysX::ActorData m_actorUserData;
        bool m_startAsleep;

        void UpdateComputedCenterOfMass();
        void ComputeInertia();
        void SetInertia(const AZ::Matrix3x3& inertia);
        void SetZeroCenterOfMass();

        void CreatePhysXActor(const Physics::RigidBodyConfiguration& configuration);
        void ReleasePhysXActor();
    };

    AZ_POP_DISABLE_WARNING
} // namespace PhysX
