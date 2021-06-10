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
#include <PhysX_precompiled.h>

#include <Source/FixedJointComponent.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzCore/Interface/Interface.h>

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
        const ApiJointGenericProperties& genericProperties)
        : JointComponent(configuration, genericProperties)
    {
    }

    FixedJointComponent::FixedJointComponent(
        const JointComponentConfiguration& configuration, 
        const ApiJointGenericProperties& genericProperties,
        const ApiJointLimitProperties& limitProperties)
        : JointComponent(configuration, genericProperties, limitProperties)
    {
    }

    void FixedJointComponent::InitNativeJoint()
    {
        if (m_jointHandle != AzPhysics::InvalidApiJointHandle)
        {
            return;
        }

        JointComponent::LeadFollowerInfo leadFollowerInfo;
        ObtainLeadFollowerInfo(leadFollowerInfo);
        if (!leadFollowerInfo.m_followerActor)
        {
            return;
        }

        AZ::Transform parentLocal = PxMathConvert(leadFollowerInfo.m_leadLocal);
        AZ::Transform childLocal = PxMathConvert(leadFollowerInfo.m_followerLocal);

        FixedApiJointConfiguration configuration;

        configuration.m_parentLocalPosition = parentLocal.GetTranslation();
        configuration.m_parentLocalRotation = parentLocal.GetRotation();
        configuration.m_childLocalPosition = childLocal.GetTranslation();
        configuration.m_childLocalRotation = childLocal.GetRotation();

        configuration.m_genericProperties = m_genericProperties;

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            m_jointHandle = sceneInterface->AddJoint(
                leadFollowerInfo.m_followerBody->m_sceneOwner,
                &configuration,  
                leadFollowerInfo.m_leadBody->m_bodyHandle, 
                leadFollowerInfo.m_followerBody->m_bodyHandle);
            m_jointSceneOwner = leadFollowerInfo.m_followerBody->m_sceneOwner;
        }
    }
} // namespace PhysX
