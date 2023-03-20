/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <PxPhysicsAPI.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Common/PhysicsTypes.h>

#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>

namespace AzPhysics
{
    struct SimulatedBody;
}

namespace PhysX
{
    class JointComponentConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(JointComponentConfiguration, AZ::SystemAllocator);
        AZ_TYPE_INFO(JointComponentConfiguration, "{1454F33F-AA6E-424B-A70C-9E463FBDEA19}");
        static void Reflect(AZ::ReflectContext* context);

        JointComponentConfiguration() = default;
        JointComponentConfiguration(
            AZ::Transform localTransformFromFollower,
            AZ::EntityId leadEntity,
            AZ::EntityId followerEntity);

        AZ::EntityId m_leadEntity; ///< EntityID for entity containing body that is lead to this joint constraint.
        AZ::EntityId m_followerEntity; ///< EntityID for entity containing body that is follower to this joint constraint.
        AZ::Transform m_localTransformFromFollower; ///< Joint's location and orientation in the frame (coordinate system) of the follower entity.
    };

    /// Base class for game-time generic joint components.
    class JointComponent
        : public AZ::Component
        , protected Physics::RigidBodyNotificationBus::MultiHandler
        , protected AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT(JointComponent, "{B01FD1D2-1D91-438D-874A-BF5EB7E919A8}");
        static void Reflect(AZ::ReflectContext* context);

        JointComponent() = default;
        JointComponent(
            const JointComponentConfiguration& configuration,
            const JointGenericProperties& genericProperties);
        JointComponent(
            const JointComponentConfiguration& configuration,
            const JointGenericProperties& genericProperties,
            const JointLimitProperties& limitProperties);
        JointComponent(
            const JointComponentConfiguration& configuration,
            const JointGenericProperties& genericProperties,
            const JointLimitProperties& limitProperties,
            const JointMotorProperties& motorProperties);

    protected:
        /// Struct to provide subclasses with native pointers during joint initialization.
        /// See use of GetLeadFollowerInfo().
        struct LeadFollowerInfo
        {
            physx::PxRigidActor* m_leadActor = nullptr;
            physx::PxRigidActor* m_followerActor = nullptr;
            AZ::Transform m_leadLocal = AZ::Transform::CreateIdentity();
            AZ::Transform m_followerLocal = AZ::Transform::CreateIdentity();
            AzPhysics::SimulatedBody* m_leadBody = nullptr;
            AzPhysics::SimulatedBody* m_followerBody = nullptr;
        };

        // AZ::Component overrides ...
        void Activate() override;
        void Deactivate() override;

        // Physics::RigidBodyNotifications overrides ...
        void OnPhysicsEnabled(const AZ::EntityId& entityId) override;
        void OnPhysicsDisabled(const AZ::EntityId& entityId) override;

        // Invoked when all the involved rigid bodies are enabled.
        void CreateNativeJoint();

        // Invoked when one of the involved rigid bodies is disabled.
        void DestroyNativeJoint();

        // Specific joint types will instantiate native joint pointer.
        virtual void InitNativeJoint(){};
        virtual void DeinitNativeJoint(){};

        // AZ::TickEvents overrides ...
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        AZ::Transform GetJointLocalPose(const physx::PxRigidActor* actor, const AZ::Transform& jointPose);

        AZ::Transform GetJointTransform(AZ::EntityId entityId,
            const JointComponentConfiguration& jointConfig);

        /// Used on initialization by sub-classes to get native pointers from entity IDs.
        /// This allows sub-classes to instantiate specific native types. This base class does not need knowledge of any specific joint type.
        void ObtainLeadFollowerInfo(LeadFollowerInfo& leadFollowerInfo);

        /// Issues warnings for invalid scenarios when initializing a joint from entity IDs.
        void WarnInvalidJointSetup(AZ::EntityId entityId, const AZStd::string& message);

        /// Issues info messages for potentially invalid scenarios when initializing a joint from entity IDs.
        void PrintJointSetupMessage(AZ::EntityId entityId, const AZStd::string& message);

        JointComponentConfiguration m_configuration;
        JointGenericProperties m_genericProperties;
        JointLimitProperties m_limits;
        JointMotorProperties m_motor;
        AzPhysics::JointHandle m_jointHandle = AzPhysics::InvalidJointHandle;
        AzPhysics::SceneHandle m_jointSceneOwner = AzPhysics::InvalidSceneHandle;

        // Variable to keep track of the rigid bodies' entities involved in this joint
        // and if they are enabled or disabled.
        AZStd::unordered_map<AZ::EntityId, bool> m_rigidBodyEntityMap;
    };
} // namespace PhysX
