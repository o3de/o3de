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
        AZ_Printf("PrismaticJointComponent", "m_motor %d %f \n", m_motor.m_enabled, m_motor.m_forceLimit);

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

    // AZ::Component
    void PrismaticJointComponent::Activate() {
        JointComponent::Activate();
        PhysX::JointInterfaceRequestBus::Handler::BusConnect(this->GetEntityId());
    };

    void PrismaticJointComponent::Deactivate() {
        JointComponent::Deactivate();
        PhysX::JointInterfaceRequestBus::Handler::BusDisconnect();
    };

    physx::PxD6Joint* GetPhysXD6Joint(const AzPhysics::JointHandle& joint_handle,
                                                        const AzPhysics::SceneHandle& joint_scene_owner)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        const auto joint = sceneInterface->GetJointFromHandle(joint_handle,joint_scene_owner);
        physx::PxJoint* native = static_cast<physx::PxJoint*>(joint->GetNativePointer());
        if (native && native->is<physx::PxD6Joint>())
        {
            physx::PxD6Joint *native_d6 = static_cast<physx::PxD6Joint*>(joint->GetNativePointer());
            return native_d6;
        }
        return nullptr;
    }

    float PrismaticJointComponent::GetPosition(){
        auto* native = GetPhysXD6Joint(m_jointSceneOwner,m_jointHandle);
        if (native){
            return native->getRelativeTransform().p.x;
        }
        return 0.f;
    };

    float PrismaticJointComponent::GetVelocity() {
        auto* native = GetPhysXD6Joint(m_jointSceneOwner,m_jointHandle);
        if (native){
            return native->getRelativeLinearVelocity().x;
        }
        return 0.f;
    };


    AZStd::pair<float, float> PrismaticJointComponent::GetLimits(){
        auto* native = GetPhysXD6Joint(m_jointSceneOwner,m_jointHandle);
        if (native){
            auto limits = native->getLinearLimit(physx::PxD6Axis::eX);
            return AZStd::pair<float, float>(limits.lower,limits.upper);
        }
        return AZStd::pair<float, float> (0.f,0.f);
    }

    AZ::Transform PrismaticJointComponent::GetTransform(){
        auto* native = GetPhysXD6Joint(m_jointSceneOwner,m_jointHandle);
        if (native){
            auto t = native->getRelativeTransform();
            return AZ::Transform(AZ::Vector3{t.p.x,t.p.y,t.p.z},
                                 AZ::Quaternion{t.q.x, t.q.y, t.q.z, t.q.w}, 1.f);
        }
        return AZ::Transform();
    };

    void PrismaticJointComponent::SetVelocity(float velocity)
    {
        auto* native = GetPhysXD6Joint(m_jointSceneOwner,m_jointHandle);
        if (native){
           native->setDriveVelocity({velocity,0,0},{0,0,0},true);
        }
    };

    void PrismaticJointComponent::SetMaximumForce(float force)
    {
        auto* native = GetPhysXD6Joint(m_jointSceneOwner,m_jointHandle);
        if (native){

            physx::PxD6JointDrive drive (0,PX_MAX_F32,force,true);
            native->setDrive(physx::PxD6Drive::eX, drive);
        }
    };
} // namespace PhysX
