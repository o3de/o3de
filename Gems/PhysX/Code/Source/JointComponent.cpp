/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Common/PhysicsSimulatedBody.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <PhysX/NativeTypeIdentifiers.h>
#include <PhysX/PhysXLocks.h>
#include <Source/JointComponent.h>
#include <Source/Utils.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <PhysX/MathConversion.h>

namespace PhysX
{
    JointComponentConfiguration::JointComponentConfiguration(
        AZ::Transform localTransformFromFollower,
        AZ::EntityId leadEntity,
        AZ::EntityId followerEntity)
        : m_localTransformFromFollower(localTransformFromFollower)
        , m_leadEntity(leadEntity)
        , m_followerEntity(followerEntity)
    {
    }

    void JointComponentConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JointComponentConfiguration>()
                ->Version(2)
                ->Field("Follower Local Transform", &JointComponentConfiguration::m_localTransformFromFollower)
                ->Field("Lead Entity", &JointComponentConfiguration::m_leadEntity)
                ->Field("Follower Entity", &JointComponentConfiguration::m_followerEntity)
                ;
        }
    }

    void JointComponent::Reflect(AZ::ReflectContext* context)
    {
        JointComponentConfiguration::Reflect(context);

        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<JointComponent, AZ::Component>()
                ->Version(2)
                ->Field("Joint Configuration", &JointComponent::m_configuration)
                ->Field("Joint Generic Properties", &JointComponent::m_genericProperties)
                ->Field("Joint Limits", &JointComponent::m_limits)
                ;
        }
    }

    JointComponent::JointComponent(
        const JointComponentConfiguration& configuration, 
        const JointGenericProperties& genericProperties)
        : m_configuration(configuration)
        , m_genericProperties(genericProperties)
    {
    }

    JointComponent::JointComponent(
        const JointComponentConfiguration& configuration, 
        const JointGenericProperties& genericProperties,
        const JointLimitProperties& limitProperties)
        : m_configuration(configuration)
        , m_genericProperties(genericProperties)
        , m_limits(limitProperties)
    {
    }

    void JointComponent::Activate()
    {
        if (m_configuration.m_followerEntity.IsValid())
        {
            if (m_configuration.m_followerEntity == m_configuration.m_leadEntity)
            {
                AZ_Error("JointComponent::Activate()", 
                    false, 
                    "Joint's lead entity cannot be the same as the entity in which the joint resides. Joint failed to initialize.");
                return;
            }

            AZ::EntityBus::Handler::BusConnect(m_configuration.m_followerEntity);
        }
    }

    void JointComponent::Deactivate()
    {
        AZ::EntityBus::Handler::BusDisconnect();
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (auto* scene = physicsSystem->GetScene(m_jointSceneOwner))
            {
                scene->RemoveJoint(m_jointHandle);
                m_jointSceneOwner = AzPhysics::InvalidSceneHandle;
            }
        }
    }

    AZ::Transform JointComponent::GetJointLocalPose(const physx::PxRigidActor* actor, const AZ::Transform& jointPose)
    {
        if (!actor)
        {
            AZ_Error("JointComponent::GetJointLocalPose", false, "Can't get pose for invalid actor pointer.");
            return AZ::Transform::CreateIdentity();
        }

        PHYSX_SCENE_READ_LOCK(actor->getScene());
        physx::PxTransform actorPose = actor->getGlobalPose();
        physx::PxTransform actorTranslateInv(-actorPose.p);
        physx::PxTransform actorRotateInv(actorPose.q);
        actorRotateInv = actorRotateInv.getInverse();
        return PxMathConvert(actorRotateInv * actorTranslateInv) * jointPose;
    }

    AZ::Transform JointComponent::GetJointTransform(AZ::EntityId entityId
        , const JointComponentConfiguration& jointConfig)
    {
        AZ::Transform jointTransform = PhysX::Utils::GetEntityWorldTransformWithoutScale(entityId);
        jointTransform = jointTransform * jointConfig.m_localTransformFromFollower;
        return jointTransform;
    }

    void JointComponent::ObtainLeadFollowerInfo(JointComponent::LeadFollowerInfo& info)
    {
        info = LeadFollowerInfo();

        if (!m_configuration.m_followerEntity.IsValid())
        {
            return;
        }

        if (m_configuration.m_leadEntity.IsValid())
        {
            Physics::RigidBodyRequestBus::EventResult(info.m_leadBody
                , m_configuration.m_leadEntity
                , &Physics::RigidBodyRequests::GetRigidBody);

            if (!info.m_leadBody)
            {
                const AZStd::string entityWithoutBodyWarningMsg("Rigid body not found in lead entity associated with joint. Joint treated as constraint on global position.");
                WarnInvalidJointSetup(m_configuration.m_leadEntity, entityWithoutBodyWarningMsg);
            }
        }

        Physics::RigidBodyRequestBus::EventResult(info.m_followerBody
            , m_configuration.m_followerEntity
            , &Physics::RigidBodyRequests::GetRigidBody);

        if (!info.m_followerBody)
        {
            const AZStd::string entityWithoutBodyWarningMsg("Rigid body not found in follower entity associated with joint. Please add a rigid body component to the entity.");
            WarnInvalidJointSetup(m_configuration.m_followerEntity, entityWithoutBodyWarningMsg);
            return;
        }

        if (info.m_leadBody)
        {
            info.m_leadActor = static_cast<physx::PxRigidActor*>(info.m_leadBody->GetNativePointer());
        }
        info.m_followerActor = static_cast<physx::PxRigidActor*>(info.m_followerBody->GetNativePointer());

        const AZ::Transform jointTransform = GetJointTransform(GetEntityId(), m_configuration);

        if (info.m_leadActor)
        {
            info.m_leadLocal = GetJointLocalPose(info.m_leadActor, jointTransform); // joint position & orientation in lead actor's frame.
        }
        else
        {
            info.m_leadLocal = jointTransform; // lead is null, attaching follower to global position of joint.
        }
        info.m_followerLocal = m_configuration.m_localTransformFromFollower;// joint position & orientation in follower actor's frame.
    }

    void JointComponent::WarnInvalidJointSetup(AZ::EntityId entityId, const AZStd::string& message)
    {
        const AZStd::vector<AZ::EntityId> entityIds = { entityId };
        const char* category = "PhysX Joint";

        PhysX::Utils::WarnEntityNames(entityIds, category, message.c_str());
    }

    void JointComponent::OnEntityActivated(const AZ::EntityId& entityId)
    {
        AZ::EntityBus::Handler::BusDisconnect();

        // If joint has no lead entity, it is a constraint on a global frame (position & orientation).
        // or, follower has been activated, this is the lead being activated.
        if (!m_configuration.m_leadEntity.IsValid() || entityId == m_configuration.m_leadEntity)
        {
            InitNativeJoint(); // Invoke overriden specific joint type instantiation
        }
        // Else, follower entity is activated, subscribe to be notified that lead entity is activated.
        else
        {
            AZ::EntityBus::Handler::BusConnect(m_configuration.m_leadEntity);
        }
    }
} // namespace PhysX
