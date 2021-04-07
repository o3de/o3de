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

    FixedJointComponent::FixedJointComponent(const GenericJointConfiguration& config)
        : JointComponent(config)
    {
    }

    FixedJointComponent::FixedJointComponent(const GenericJointConfiguration& config,
        const GenericJointLimitsConfiguration& limitConfig)
        : JointComponent(config, limitConfig)
    {
    }

    void FixedJointComponent::InitNativeJoint()
    {
        if (m_joint)
        {
            return;
        }

        JointComponent::LeadFollowerInfo leadFollowerInfo;
        ObtainLeadFollowerInfo(leadFollowerInfo);
        if (!leadFollowerInfo.m_followerActor)
        {
            return;
        }
        PHYSX_SCENE_READ_LOCK(leadFollowerInfo.m_followerActor->getScene());
        m_joint = AZStd::make_shared<FixedJoint>(physx::PxFixedJointCreate(
            PxGetPhysics(),
            leadFollowerInfo.m_leadActor,
            leadFollowerInfo.m_leadLocal,
            leadFollowerInfo.m_followerActor,
            leadFollowerInfo.m_followerLocal),
            leadFollowerInfo.m_leadBody,
            leadFollowerInfo.m_followerBody);
    }
} // namespace PhysX
