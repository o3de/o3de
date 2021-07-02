/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PhysX_precompiled.h>

#include <Source/BallJointComponent.h>
#include <PhysX/MathConversion.h>
#include <PhysX/PhysXLocks.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/RigidBodyBus.h>

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

    BallJointComponent::BallJointComponent(const GenericJointConfiguration& config
        , const GenericJointLimitsConfiguration& swingLimit)
            : JointComponent(config, swingLimit)
    {
    }

    void BallJointComponent::InitNativeJoint()
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
        m_joint = AZStd::make_shared<BallJoint>(physx::PxSphericalJointCreate(
            PxGetPhysics(),
            leadFollowerInfo.m_leadActor,
            leadFollowerInfo.m_leadLocal,
            leadFollowerInfo.m_followerActor,
            leadFollowerInfo.m_followerLocal),
            leadFollowerInfo.m_leadBody,
            leadFollowerInfo.m_followerBody);

        InitSwingLimits();
    }

    void BallJointComponent::InitSwingLimits()
    {
        if (!m_joint)
        {
            return;
        }

        physx::PxSphericalJoint* ballJointNative = static_cast<physx::PxSphericalJoint*>(m_joint->GetNativePointer());
        if (!ballJointNative)
        {
            return;
        }

        if (!m_limits.m_isLimited)
        {
            ballJointNative->setSphericalJointFlag(physx::PxSphericalJointFlag::eLIMIT_ENABLED, false);
            return;
        }

        // Hard limit uses a tolerance value (distance to limit at which limit becomes active).
        // Soft limit allows angle to exceed limit but springs back with configurable spring stiffness and damping.
        physx::PxJointLimitCone swingLimit(AZ::DegToRad(m_limits.m_limitFirst)
            , AZ::DegToRad(m_limits.m_limitSecond)
            , m_limits.m_tolerance);
        if (m_limits.m_isSoftLimit)
        {
            swingLimit.stiffness = m_limits.m_stiffness;
            swingLimit.damping = m_limits.m_damping;
        }

        ballJointNative->setLimitCone(swingLimit);
        ballJointNative->setSphericalJointFlag(physx::PxSphericalJointFlag::eLIMIT_ENABLED, true);
    }
} // namespace PhysX
