/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Source/BallJointComponent.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/PhysicsScene.h>

#include <PxPhysicsAPI.h>

namespace PhysX
{
    void BallJointComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BallJointComponent, JointComponent>()
                ->Version(2)
                ;
        }
    }

    BallJointComponent::BallJointComponent(
        const JointComponentConfiguration& configuration,
        const JointGenericProperties& genericProperties,
        const JointLimitProperties& limitProperties)
        : JointComponent(configuration, genericProperties, limitProperties)
    {
    }

    void BallJointComponent::InitNativeJoint()
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
                "PhysX", "Entity [%s] Ball Joint component missing lead entity. This joint will be a global constraint on the follower's global position.",
                GetEntity()->GetName().c_str());
        }


        BallJointConfiguration configuration;
        configuration.m_parentLocalPosition = leadFollowerInfo.m_leadLocal.GetTranslation();
        configuration.m_parentLocalRotation = leadFollowerInfo.m_leadLocal.GetRotation();
        configuration.m_childLocalPosition = leadFollowerInfo.m_followerLocal.GetTranslation();
        configuration.m_childLocalRotation = leadFollowerInfo.m_followerLocal.GetRotation();

        configuration.m_genericProperties = m_genericProperties;
        configuration.m_limitProperties = m_limits;

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
} // namespace PhysX
