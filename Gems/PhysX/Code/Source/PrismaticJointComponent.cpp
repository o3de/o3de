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
    }

    void PrismaticJointComponent::Activate()
    {
        const AZ::EntityComponentIdPair id(GetEntityId(), GetId());
        PhysX::JointInterfaceRequestBus::Handler::BusConnect(id);
        JointComponent::Activate();
    };

    void PrismaticJointComponent::Deactivate()
    {
        PhysX::JointInterfaceRequestBus::Handler::BusDisconnect();
        JointComponent::Deactivate();
    };

    physx::PxD6Joint* PrismaticJointComponent::GetPhysXD6Joint() const
    {
        if (m_native)
        {
            return m_native;
        }
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AZ_Assert(sceneInterface, "No sceneInterface");
        const auto* joint = sceneInterface->GetJointFromHandle(m_jointSceneOwner, m_jointHandle);
        physx::PxJoint* native = static_cast<physx::PxJoint*>(joint->GetNativePointer());
        if (native && native->is<physx::PxD6Joint>())
        {
            physx::PxD6Joint* nativeD6 = static_cast<physx::PxD6Joint*>(joint->GetNativePointer());
            m_native = nativeD6;
            return nativeD6;
        }
        return nullptr;
    }

    float PrismaticJointComponent::GetPosition() const
    {
        auto* native = GetPhysXD6Joint();
        if (native)
        {
            // Underlying PhysX joint is D6, but it simulates PhysXPrismatic joint.
            // The D6 joint has only X-axis unlocked, so report only X travel.
            return native->getRelativeTransform().p.x;
        }
        return 0.f;
    };

    float PrismaticJointComponent::GetVelocity() const
    {
        auto* native = GetPhysXD6Joint();
        if (native)
        {
            // Undelying PhysX joint is D6, but it simulates PhysXPrismatic joint.
            // The D6 joint has only X-axis unlocked, so report only X velocity.
            return native->getRelativeLinearVelocity().x;
        }
        return 0.f;
    };

    AZStd::pair<float, float> PrismaticJointComponent::GetLimits() const
    {
        auto* native = GetPhysXD6Joint();
        if (native)
        {
            auto limits = native->getLinearLimit(physx::PxD6Axis::eX);
            return AZStd::pair<float, float>(limits.lower, limits.upper);
        }
        return AZStd::make_pair(0.f, 0.f);
    }

    AZ::Transform PrismaticJointComponent::GetTransform() const
    {
        auto* native = GetPhysXD6Joint();
        if (native)
        {
            const auto worldFromLocal = native->getRelativeTransform();
            return AZ::Transform(
                AZ::Vector3{ worldFromLocal.p.x, worldFromLocal.p.y, worldFromLocal.p.z },
                AZ::Quaternion{ worldFromLocal.q.x, worldFromLocal.q.y, worldFromLocal.q.z, worldFromLocal.q.w },
                1.f);
        }
        return AZ::Transform();
    };

    void PrismaticJointComponent::SetVelocity(float velocity)
    {
        auto* native = GetPhysXD6Joint();
        if (native)
        {
            native->setDriveVelocity({ velocity, 0, 0 }, { 0, 0, 0 }, true);
        }
    };

    void PrismaticJointComponent::SetMaximumForce(float force)
    {
        auto* native = GetPhysXD6Joint();
        if (native)
        {
            const physx::PxD6JointDrive drive(0, PX_MAX_F32, force, true);
            native->setDrive(physx::PxD6Drive::eX, drive);
        }
    };
} // namespace PhysX
