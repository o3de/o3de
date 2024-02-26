/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/HingeJointComponent.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/PhysicsScene.h>

#include <PxPhysicsAPI.h>
#include <PhysX/NativeTypeIdentifiers.h>

namespace PhysX
{
    void HingeJointComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<HingeJointComponent, JointComponent>()
                ->Version(2)
            ;
        }
    }

    HingeJointComponent::HingeJointComponent(
        const JointComponentConfiguration& configuration,
        const JointGenericProperties& genericProperties,
        const JointLimitProperties& limitProperties,
        const JointMotorProperties& motorProperties)
        : JointComponent(configuration, genericProperties, limitProperties, motorProperties)
    {
    }

    void HingeJointComponent::InitNativeJoint()
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

        HingeJointConfiguration configuration;
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

        CachePhysXNativeRevoluteJoint();

        JointRequestBus::Handler::BusConnect(AZ::EntityComponentIdPair(GetEntityId(), GetId()));
    }

    void HingeJointComponent::DeinitNativeJoint()
    {
        JointRequestBus::Handler::BusDisconnect();
        m_nativeJoint = nullptr;
    }

    void HingeJointComponent::CachePhysXNativeRevoluteJoint()
    {
        if (m_nativeJoint)
        {
            return;
        }
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();
        AZ_Assert(sceneInterface, "No sceneInterface");
        const auto joint = sceneInterface->GetJointFromHandle(m_jointSceneOwner, m_jointHandle);
        AZ_Assert(joint->GetNativeType() == NativeTypeIdentifiers::HingeJoint, "It is not PhysXHingeJoint");
        physx::PxJoint* nativeJoint = static_cast<physx::PxJoint*>(joint->GetNativePointer());
        physx::PxRevoluteJoint* native = nativeJoint->is<physx::PxRevoluteJoint>();
        AZ_Assert(native, "It is not PxRevoluteJoint");
        m_nativeJoint = native;
    }

    float HingeJointComponent::GetPosition() const
    {
        return m_nativeJoint->getAngle();
    }

    float HingeJointComponent::GetVelocity() const
    {
        return m_nativeJoint->getVelocity();
    }

    AZStd::pair<float, float> HingeJointComponent::GetLimits() const
    {
        auto limit = m_nativeJoint->getLimit();
        return AZStd::pair<float, float>(limit.lower, limit.upper);
    }

    AZ::Transform HingeJointComponent::GetTransform() const
    {
        const auto worldFromLocal = m_nativeJoint->getRelativeTransform();
        return PxMathConvert(worldFromLocal);
    }

    void HingeJointComponent::SetVelocity(float velocity)
    {
        m_nativeJoint->setDriveVelocity(velocity, true);
    }

    void HingeJointComponent::SetMaximumForce(float force)
    {
        m_nativeJoint->setDriveForceLimit(force);
    }
} // namespace PhysX
