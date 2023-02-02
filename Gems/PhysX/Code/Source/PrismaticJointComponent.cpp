/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/PrismaticJointComponent.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/PhysicsScene.h>

#include <PxPhysicsAPI.h>
#include <PhysX/NativeTypeIdentifiers.h>

namespace PhysX
{
    void PrismaticJointComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PrismaticJointComponent, JointComponent>()
                ->Version(2)
                ;
        }
    }

    PrismaticJointComponent::PrismaticJointComponent(
        const JointComponentConfiguration& configuration,
        const JointGenericProperties& genericProperties,
        const JointLimitProperties& limitProperties,
        const JointMotorProperties& motorProperties)
        : JointComponent(configuration, genericProperties, limitProperties, motorProperties)
    {
    }

    void PrismaticJointComponent::InitNativeJoint()
    {
        if (m_jointHandle != AzPhysics::InvalidJointHandle)
        {
            return;
        }

        JointComponent::LeadFollowerInfo leadFollowerInfo;
        ObtainLeadFollowerInfo(leadFollowerInfo);
        if (leadFollowerInfo.m_followerActor == nullptr ||
            leadFollowerInfo.m_followerBody == nullptr)
        {
            return;
        }

        // if there is no lead body, this will be a constraint of the follower's global position, so use invalid body handle.
        AzPhysics::SimulatedBodyHandle parentHandle = AzPhysics::InvalidSimulatedBodyHandle;
        if (leadFollowerInfo.m_leadBody != nullptr)
        {
            parentHandle = leadFollowerInfo.m_leadBody->m_bodyHandle;
        }
        else
        {
            AZ_TracePrintf(
                "PhysX", "Entity [%s] Hinge Joint component missing lead entity. This joint will be a global constraint on the follower's global position.",
                GetEntity()->GetName().c_str());
        }

        PrismaticJointConfiguration configuration;
        configuration.m_parentLocalPosition = leadFollowerInfo.m_leadLocal.GetTranslation();
        configuration.m_parentLocalRotation = leadFollowerInfo.m_leadLocal.GetRotation();
        configuration.m_childLocalPosition = leadFollowerInfo.m_followerLocal.GetTranslation();
        configuration.m_childLocalRotation = leadFollowerInfo.m_followerLocal.GetRotation();

        configuration.m_genericProperties = m_genericProperties;
        configuration.m_limitProperties = m_limits;
        configuration.m_motorProperties = m_motor;

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_jointHandle = sceneInterface->AddJoint(
                leadFollowerInfo.m_followerBody->m_sceneOwner,
                &configuration,
                parentHandle,
                leadFollowerInfo.m_followerBody->m_bodyHandle);
            m_jointSceneOwner = leadFollowerInfo.m_followerBody->m_sceneOwner;
        }

        CachePhysXD6Joint();

        JointRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));
    }

    void PrismaticJointComponent::DeinitNativeJoint()
    {
        JointRequestBus::Handler::BusDisconnect();
        m_native = nullptr;
    }

    void PrismaticJointComponent::CachePhysXD6Joint()
    {
        if (m_native)
        {
            return;
        }
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AZ_Assert(sceneInterface, "No sceneInterface");
        const auto* joint = sceneInterface->GetJointFromHandle(m_jointSceneOwner, m_jointHandle);
        AZ_Assert(joint->GetNativeType() == NativeTypeIdentifiers::PrismaticJoint, "It is not PhysXPrismaticJoint");
        physx::PxJoint* native = static_cast<physx::PxJoint*>(joint->GetNativePointer());
        physx::PxD6Joint* nativeD6 = native->is<physx::PxD6Joint>();
        AZ_Assert(nativeD6, "It is not PxD6Joint");
        m_native = nativeD6;
    }

    float PrismaticJointComponent::GetPosition() const
    {
        // Underlying PhysX joint is D6, but it simulates PhysXPrismatic joint.
        // The D6 joint has only X-axis unlocked, so report only X travel.
        return m_native->getRelativeTransform().p.x;
    };

    float PrismaticJointComponent::GetVelocity() const
    {
        // Undelying PhysX joint is D6, but it simulates PhysXPrismatic joint.
        // The D6 joint has only X-axis unlocked, so report only X velocity.
        return m_native->getRelativeLinearVelocity().x;
    };

    AZStd::pair<float, float> PrismaticJointComponent::GetLimits() const
    {
        auto limits = m_native->getLinearLimit(physx::PxD6Axis::eX);
        return AZStd::pair<float, float>(limits.lower, limits.upper);
    }

    AZ::Transform PrismaticJointComponent::GetTransform() const
    {
        const auto worldFromLocal = m_native->getRelativeTransform();
        return AZ::Transform(
            AZ::Vector3{ worldFromLocal.p.x, worldFromLocal.p.y, worldFromLocal.p.z },
            AZ::Quaternion{ worldFromLocal.q.x, worldFromLocal.q.y, worldFromLocal.q.z, worldFromLocal.q.w },
            1.f);
    };

    void PrismaticJointComponent::SetVelocity(float velocity)
    {
        m_native->setDriveVelocity({ velocity, 0.0f, 0.0f }, physx::PxVec3(0.0f), true);
    };

    void PrismaticJointComponent::SetMaximumForce(float force)
    {
        const physx::PxD6JointDrive drive(0.f , PX_MAX_F32, force, true);
        m_native->setDrive(physx::PxD6Drive::eX, drive);
    };
} // namespace PhysX
