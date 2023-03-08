/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Source/FixedJointComponent.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <PhysX/NativeTypeIdentifiers.h>
#include <PxPhysicsAPI.h>

namespace PhysX
{
    void FixedJointComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<FixedJointComponent, JointComponent>()
                ->Version(2)
                ;
        }
    }

    FixedJointComponent::FixedJointComponent(
        const JointComponentConfiguration& configuration,
        const JointGenericProperties& genericProperties)
        : JointComponent(configuration, genericProperties)
    {
    }

    FixedJointComponent::FixedJointComponent(
        const JointComponentConfiguration& configuration,
        const JointGenericProperties& genericProperties,
        const JointLimitProperties& limitProperties)
        : JointComponent(configuration, genericProperties, limitProperties)
    {
    }

    void FixedJointComponent::InitNativeJoint()
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
            AZ_TracePrintf("PhysX",
                "Entity [%s] Fixed Joint component missing lead entity. This joint will be a global constraint on the follower's global position.",
                GetEntity()->GetName().c_str());
        }

        FixedJointConfiguration configuration;
        configuration.m_parentLocalPosition = leadFollowerInfo.m_leadLocal.GetTranslation();
        configuration.m_parentLocalRotation = leadFollowerInfo.m_leadLocal.GetRotation();
        configuration.m_childLocalPosition = leadFollowerInfo.m_followerLocal.GetTranslation();
        configuration.m_childLocalRotation = leadFollowerInfo.m_followerLocal.GetRotation();

        configuration.m_genericProperties = m_genericProperties;

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_jointHandle = sceneInterface->AddJoint(
                leadFollowerInfo.m_followerBody->m_sceneOwner,
                &configuration,
                parentHandle,
                leadFollowerInfo.m_followerBody->m_bodyHandle);
            m_jointSceneOwner = leadFollowerInfo.m_followerBody->m_sceneOwner;
        }
        m_nativeJoint = FixedJointComponent::GetNativeJoint();
        if (m_nativeJoint)
        {
            JointRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));
        }
    }

    void FixedJointComponent::DeinitNativeJoint()
    {
        JointRequestBus::Handler::BusDisconnect();
        m_nativeJoint = nullptr;
    }

    physx::PxFixedJoint* FixedJointComponent::GetNativeJoint() const
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AZ_Assert(sceneInterface, "No sceneInterface");
        const auto* joint = sceneInterface->GetJointFromHandle(m_jointSceneOwner, m_jointHandle);
        AZ_Assert(joint->GetNativeType() == NativeTypeIdentifiers::FixedJoint, "It is not PhysxFixed Joint");
        physx::PxJoint* native = static_cast<physx::PxJoint*>(joint->GetNativePointer());
        physx::PxFixedJoint* nativeFixed = native->is<physx::PxFixedJoint>();
        return nativeFixed;
    }


    float FixedJointComponent::GetPosition() const
    {
        AZ_Warning("FixedJointComponent::GetPosition", false, "Cannot get position in fixed joint");
        return 0;
    }

    float FixedJointComponent::GetVelocity() const
    {
        AZ_Warning("FixedJointComponent::GetVelocity", false, "Cannot get velocity in fixed joint");
        return 0;
    }

    AZ::Transform FixedJointComponent::GetTransform() const
    {
        const auto worldFromLocal = m_nativeJoint->getRelativeTransform();
        return AZ::Transform(
            AZ::Vector3{ worldFromLocal.p.x, worldFromLocal.p.y, worldFromLocal.p.z },
            AZ::Quaternion{ worldFromLocal.q.x, worldFromLocal.q.y, worldFromLocal.q.z, worldFromLocal.q.w },
            1.f);
    }

    void FixedJointComponent::SetVelocity(float velocity)
    {
        AZ_Warning("FixedJointComponent::SetVelocity", false, "Cannot set velocity in fixed joint");
    }

    void FixedJointComponent::SetMaximumForce(float force)
    {
        AZ_Warning("FixedJointComponent::SetMaximumForce", false, "Cannot set maximum force in fixed joint");
    }

    AZStd::pair<float, float> FixedJointComponent::GetLimits() const
    {
        AZ_Warning("FixedJointComponent::GetLimits", false, "Cannot get limits in fixed joint");
        return AZStd::pair<float, float>{ -1.f, -1.f };
    }

    AZStd::pair<AZ::Vector3, AZ::Vector3> FixedJointComponent::GetForces() const
    {
        const auto constraint = m_nativeJoint->getConstraint();
        physx::PxVec3 linear{ 0.f };
        physx::PxVec3 angular{ 0.f };
        constraint->getForce(linear, angular);
        return { PxMathConvert(linear), PxMathConvert(angular) };
    }

    float FixedJointComponent::GetTargetVelocity() const
    {
        AZ_Warning("FixedJointComponent::GetTargetVelocity", false, "Cannot get GetTargetVelocity in fixed joint");
        return 0.f;
    };

} // namespace PhysX
