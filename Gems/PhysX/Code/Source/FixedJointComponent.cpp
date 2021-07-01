/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
