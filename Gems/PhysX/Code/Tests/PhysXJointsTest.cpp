/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <PhysX_precompiled.h>

#include <AzTest/AzTest.h>

#include <Tests/PhysXGenericTestFixture.h>
#include <Tests/PhysXTestCommon.h>

#include <BoxColliderComponent.h>
#include <RigidBodyComponent.h>
#include <Joint.h>
#include <JointComponent.h>
#include <BallJointComponent.h>
#include <FixedJointComponent.h>
#include <HingeJointComponent.h>

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>

namespace PhysX
{
    using PhysXJointsTest = PhysX::GenericPhysicsInterfaceTest;

    template<typename JointComponentType>
    AZStd::unique_ptr<AZ::Entity> AddBodyColliderEntity( AzPhysics::SceneHandle sceneHandle,
        const AZ::Vector3& position, 
        const AZ::Vector3& initialLinearVelocity,
        AZStd::shared_ptr<GenericJointConfiguration> jointConfig = nullptr,
        AZStd::shared_ptr<GenericJointLimitsConfiguration> jointLimitsConfig = nullptr)
    {
        const char* entityName = "testEntity";
        auto entity = AZStd::make_unique<AZ::Entity>(entityName);

        AZ::TransformConfig transformConfig;
        transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
        entity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);

        auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
        auto boxShapeConfiguration = AZStd::make_shared<Physics::BoxShapeConfiguration>();
        auto boxColliderComponent = entity->CreateComponent<BoxColliderComponent>();
        boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfiguration, boxShapeConfiguration) });

        AzPhysics::RigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_initialLinearVelocity = initialLinearVelocity;
        rigidBodyConfig.m_gravityEnabled = false;

        // Make lead body very heavy
        if (!jointConfig)
        {
            rigidBodyConfig.m_mass = 9999.0f;
        }
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig, sceneHandle);

        if (jointConfig)
        {
            jointConfig->m_followerEntity = entity->GetId();

            GenericJointLimitsConfiguration defaultJointLimitsConfig;
            entity->CreateComponent<JointComponentType>(
                    *jointConfig,
                    (jointLimitsConfig)? *jointLimitsConfig : defaultJointLimitsConfig);
        }

        entity->Init();
        entity->Activate();

        return entity;
    }

    AZ::Vector3 RunJointTest(AzPhysics::Scene* scene, AZ::EntityId followerEntityId)
    {
        AZ::Vector3 followerEndPosition(0.0f, 0.0f, 0.0f);

        //Run simulation for a while - bounces box once on force volume
        const float deltaTime = AzPhysics::SystemConfiguration::DefaultFixedTimestep;
        const AZ::u32 numSteps = 240;
        PhysX::TestUtils::UpdateScene(scene, deltaTime, numSteps);
        
        AZ::TransformBus::EventResult(followerEndPosition,
            followerEntityId, 
            &AZ::TransformBus::Events::GetWorldTranslation);

        return followerEndPosition;
    }

    TEST_F(PhysXJointsTest, Joints_FixedJoint_BodiesAreConstrainedAndMoveTogether)
    {
        // Place lead on the right of follower, tie them together with fixed joint and send the lead moving to the right.
        // The follower should be pulled along if the fixed joint works.

        const AZ::Vector3 followerPosition(-1.0f, 0.0f, 0.0f);
        const AZ::Vector3 followerInitialLinearVelocity(0.0f, 0.0f, 0.0f);

        const AZ::Vector3 leadPosition(1.0f, 0.0f, 0.0f);
        const AZ::Vector3 leadInitialLinearVelocity(10.0f, 0.0f, 0.0f);

        const AZ::Transform jointLocalTransform = AZ::Transform::CreateTranslation(AZ::Vector3(1.0f, 0.0f, 0.0f));          

        // Templated joint component type is irrelevant since joint component is not created for this invocation.
        auto leadEntity = AddBodyColliderEntity<JointComponent>(m_testSceneHandle,
            leadPosition,
            leadInitialLinearVelocity);

        auto jointConfig = AZStd::make_shared<GenericJointConfiguration>();
        jointConfig->m_leadEntity = leadEntity->GetId();
        jointConfig->m_localTransformFromFollower = jointLocalTransform;

        auto followerEntity = AddBodyColliderEntity<FixedJointComponent>(m_testSceneHandle,
            followerPosition,
            followerInitialLinearVelocity, 
            jointConfig);

        const AZ::Vector3 followerEndPosition = RunJointTest(m_defaultScene, followerEntity->GetId());

        EXPECT_TRUE(followerEndPosition.GetX() > followerPosition.GetX());
    }

    TEST_F(PhysXJointsTest, Joint_HingeJoint_FollowerSwingsAroundLead)
    {
        // Place lead on the right of follower, tie them together with hinge joint and send the follower moving up.
        // The follower should swing around the lead.

        const AZ::Vector3 followerPosition(-1.0f, 0.0f, 0.0f);
        const AZ::Vector3 followerInitialLinearVelocity(0.0f, 0.0f, 10.0f);

        const AZ::Vector3 leadPosition(1.0f, 0.0f, 0.0f);
        const AZ::Vector3 leadInitialLinearVelocity(0.0f, 0.0f, 0.0f);

        const AZ::Vector3 jointLocalPosition(1.0f, 0.0f, 0.0f);
        const AZ::Quaternion jointLocalRotation = AZ::Quaternion::CreateRotationZ(90.0f);
        const AZ::Transform jointLocalTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            jointLocalRotation, 
            jointLocalPosition);

        // Templated joint component type is irrelevant since joint component is not created for this invocation.
        auto leadEntity = AddBodyColliderEntity<JointComponent>(m_testSceneHandle,
            leadPosition, 
            leadInitialLinearVelocity);

        auto jointConfig = AZStd::make_shared<GenericJointConfiguration>();
        jointConfig->m_leadEntity = leadEntity->GetId();
        jointConfig->m_localTransformFromFollower = jointLocalTransform;

        auto jointLimits = AZStd::make_shared <GenericJointLimitsConfiguration>();
        jointLimits->m_isLimited = false;

        auto followerEntity = AddBodyColliderEntity<HingeJointComponent>(m_testSceneHandle,
            followerPosition,
            followerInitialLinearVelocity,
            jointConfig,
            jointLimits);

        const AZ::Vector3 followerEndPosition = RunJointTest(m_defaultScene, followerEntity->GetId());

        EXPECT_TRUE(followerEndPosition.GetX() > followerPosition.GetX());
        EXPECT_TRUE(abs(followerEndPosition.GetZ()) > FLT_EPSILON);
    }

    TEST_F(PhysXJointsTest, Joint_BallJoint_FollowerSwingsUpAboutLead)
    {
        // Place lead on top of follower, tie them together with ball joint and send the follower moving sideways in the X and Y directions.
        // The follower should swing up about lead.

        const AZ::Vector3 followerPosition(0.0f, 0.0f, -1.0f);
        const AZ::Vector3 followerInitialLinearVelocity(10.0f, 10.0f, 0.0f);

        const AZ::Vector3 leadPosition(0.0f, 0.0f, 1.0f);
        const AZ::Vector3 leadInitialLinearVelocity(0.0f, 0.0f, 0.0f);

        const AZ::Vector3 jointLocalPosition(0.0f, 0.0f, 2.0f);
        const AZ::Quaternion jointLocalRotation = AZ::Quaternion::CreateRotationY(90.0f);
        const AZ::Transform jointLocalTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            jointLocalRotation,
            jointLocalPosition);

        // Templated joint component type is irrelevant since joint component is not created for this invocation.
        auto leadEntity = AddBodyColliderEntity<JointComponent>(m_testSceneHandle,
            leadPosition,
            leadInitialLinearVelocity);

        auto jointConfig = AZStd::make_shared<GenericJointConfiguration>();
        jointConfig->m_leadEntity = leadEntity->GetId();
        jointConfig->m_localTransformFromFollower = jointLocalTransform;

        auto jointLimits = AZStd::make_shared <GenericJointLimitsConfiguration>();
        jointLimits->m_isLimited = false;

        auto followerEntity = AddBodyColliderEntity<BallJointComponent>(m_testSceneHandle,
            followerPosition,
            followerInitialLinearVelocity,
            jointConfig,
            jointLimits);

        const AZ::Vector3 followerEndPosition = RunJointTest(m_defaultScene, followerEntity->GetId());

        EXPECT_TRUE(followerEndPosition.GetZ() > followerPosition.GetZ());
    }
}
