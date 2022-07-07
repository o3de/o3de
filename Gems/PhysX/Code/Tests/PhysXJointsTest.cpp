/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzTest/AzTest.h>

#include <Tests/PhysXGenericTestFixture.h>
#include <Tests/PhysXTestCommon.h>

#include <BoxColliderComponent.h>
#include <RigidBodyComponent.h>
#include <JointComponent.h>
#include <BallJointComponent.h>
#include <FixedJointComponent.h>
#include <HingeJointComponent.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/PhysicsSystem.h>

namespace PhysX
{
    using PhysXJointsTest = PhysX::GenericPhysicsInterfaceTest;

    template<typename JointComponentType>
    AZStd::unique_ptr<AZ::Entity> AddBodyColliderEntity( AzPhysics::SceneHandle sceneHandle,
        const AZ::Vector3& position, 
        const AZ::Vector3& initialLinearVelocity,
        AZStd::shared_ptr<JointComponentConfiguration> jointConfig = nullptr,
        AZStd::shared_ptr<JointGenericProperties> jointGenericProperties = nullptr,
        AZStd::shared_ptr<JointLimitProperties> jointLimitProperties = nullptr)
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

            JointGenericProperties defaultJointGenericProperties;
            JointLimitProperties defaultJointLimitProperties;
            entity->CreateComponent<JointComponentType>(
                    *jointConfig,
                    (jointGenericProperties)? *jointGenericProperties : defaultJointGenericProperties,
                    (jointLimitProperties)? *jointLimitProperties : defaultJointLimitProperties);
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

        auto jointConfig = AZStd::make_shared<JointComponentConfiguration>();
        jointConfig->m_leadEntity = leadEntity->GetId();
        jointConfig->m_localTransformFromFollower = jointLocalTransform;

        auto followerEntity = AddBodyColliderEntity<FixedJointComponent>(m_testSceneHandle,
            followerPosition,
            followerInitialLinearVelocity, 
            jointConfig);

        const AZ::Vector3 followerEndPosition = RunJointTest(m_defaultScene, followerEntity->GetId());

        EXPECT_GT(followerEndPosition.GetX(), followerPosition.GetX());
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

        auto jointConfig = AZStd::make_shared<JointComponentConfiguration>();
        jointConfig->m_leadEntity = leadEntity->GetId();
        jointConfig->m_localTransformFromFollower = jointLocalTransform;

        auto jointLimits = AZStd::make_shared <JointLimitProperties>();
        jointLimits->m_isLimited = false;

        auto followerEntity = AddBodyColliderEntity<HingeJointComponent>(m_testSceneHandle,
            followerPosition,
            followerInitialLinearVelocity,
            jointConfig,
            nullptr,
            jointLimits);

        const AZ::Vector3 followerEndPosition = RunJointTest(m_defaultScene, followerEntity->GetId());

        EXPECT_GT(followerEndPosition.GetX(), followerPosition.GetX());
        EXPECT_GT(abs(followerEndPosition.GetZ()), FLT_EPSILON);
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

        auto jointConfig = AZStd::make_shared<JointComponentConfiguration>();
        jointConfig->m_leadEntity = leadEntity->GetId();
        jointConfig->m_localTransformFromFollower = jointLocalTransform;

        auto jointLimits = AZStd::make_shared <JointLimitProperties>();
        jointLimits->m_isLimited = false;

        auto followerEntity = AddBodyColliderEntity<BallJointComponent>(m_testSceneHandle,
            followerPosition,
            followerInitialLinearVelocity,
            jointConfig,
            nullptr,
            jointLimits);

        const AZ::Vector3 followerEndPosition = RunJointTest(m_defaultScene, followerEntity->GetId());

        EXPECT_GT(followerEndPosition.GetZ(), followerPosition.GetZ());
    }

    TEST_F(PhysXJointsTest, Joint_BallJoint_GlobalConstraint)
    {
        // Place an entity in the world with a rigid body, physx collider, and a ball joint components.
        // Do not set a lead entity on the ball joint component.
        // Set entity's initial velocity to 10 in the X and Y directions on the rigid body component.
        // The entity should swing up on the global constraint.

        const AZ::Vector3 followerPosition(0.0f, 0.0f, -1.0f);
        const AZ::Vector3 followerInitialLinearVelocity(10.0f, 10.0f, 0.0f);

        const AZ::Vector3 jointLocalPosition(0.0f, 0.0f, 2.0f);
        const AZ::Quaternion jointLocalRotation = AZ::Quaternion::CreateRotationY(90.0f);
        const AZ::Transform jointLocalTransform = AZ::Transform::CreateFromQuaternionAndTranslation(jointLocalRotation, jointLocalPosition);

        //we want a global constraint, so leave the lead entity unset.
        auto jointConfig = AZStd::make_shared<JointComponentConfiguration>();
        jointConfig->m_localTransformFromFollower = jointLocalTransform;

        auto jointLimits = AZStd::make_shared<JointLimitProperties>();
        jointLimits->m_isLimited = false;

        auto followerEntity = AddBodyColliderEntity<BallJointComponent>(
            m_testSceneHandle, followerPosition, followerInitialLinearVelocity, jointConfig, nullptr, jointLimits);

        const AZ::Vector3 followerEndPosition = RunJointTest(m_defaultScene, followerEntity->GetId());

        EXPECT_GT(followerEndPosition.GetZ(), followerPosition.GetZ());
    }

    TEST_F(PhysXJointsTest, Joint_HingeJoint_GlobalConstraint)
    {
        // Place an entity in the world with a rigid body, physx collider, and a hinge joint components.
        // Do not set a lead entity on the hinge joint component.
        // Set entity's initial velocity to 10 in the X and Y directions on the rigid body component.
        // The entity should swing up on the global constraint.

        const AZ::Vector3 followerPosition(0.0f, 0.0f, -1.0f);
        const AZ::Vector3 followerInitialLinearVelocity(10.0f, 10.0f, 0.0f);

        const AZ::Vector3 jointLocalPosition(0.0f, 0.0f, 2.0f);
        const AZ::Quaternion jointLocalRotation = AZ::Quaternion::CreateFromEulerAnglesDegrees(AZ::Vector3(0.0f, 180.0f, 90.0f));
        const AZ::Transform jointLocalTransform = AZ::Transform::CreateFromQuaternionAndTranslation(jointLocalRotation, jointLocalPosition);

        // do not set the lead entity as that makes this a global constraint
        auto jointConfig = AZStd::make_shared<JointComponentConfiguration>();
        jointConfig->m_localTransformFromFollower = jointLocalTransform;

        auto jointLimits = AZStd::make_shared<JointLimitProperties>();
        jointLimits->m_isLimited = false;

        auto followerEntity = AddBodyColliderEntity<HingeJointComponent>(
            m_testSceneHandle, followerPosition, followerInitialLinearVelocity, jointConfig, nullptr, jointLimits);

        const AZ::Vector3 followerEndPosition = RunJointTest(m_defaultScene, followerEntity->GetId());

        EXPECT_GT(followerEndPosition.GetZ(), followerPosition.GetZ());
    }

    template<class JointConfigurationType>
    class PhysXJointsApiTest : public PhysX::GenericPhysicsInterfaceTest
    {
    public:
        
        void SetUp() override
        {
            PhysX::GenericPhysicsInterfaceTest::SetUp();

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                AzPhysics::RigidBodyConfiguration parentConfiguration;
                AzPhysics::RigidBodyConfiguration childConfiguration;

                auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
                auto shapeConfiguration = AZStd::make_shared<Physics::BoxShapeConfiguration>(AZ::Vector3(1.0f, 1.0f, 1.0f));

                parentConfiguration.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(colliderConfig, shapeConfiguration);
                childConfiguration.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(colliderConfig, shapeConfiguration);
                
                // Put the child body a bit to the lower side of X to avoid it colliding with parent
                childConfiguration.m_position.SetX(childConfiguration.m_position.GetX() - 2.0f);
                m_childInitialPos = childConfiguration.m_position;
                parentConfiguration.m_initialLinearVelocity.SetX(10.0f);

                m_parentBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &parentConfiguration);
                m_childBodyHandle = sceneInterface->AddSimulatedBody(m_testSceneHandle, &childConfiguration);
            }
        }

        void TearDown() override
        {
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                sceneInterface->RemoveSimulatedBody(m_testSceneHandle, m_parentBodyHandle);
                sceneInterface->RemoveSimulatedBody(m_testSceneHandle, m_childBodyHandle);    
            }

            PhysX::GenericPhysicsInterfaceTest::TearDown();
        }

        AzPhysics::SimulatedBodyHandle m_parentBodyHandle = AzPhysics::InvalidJointHandle;
        AzPhysics::SimulatedBodyHandle m_childBodyHandle = AzPhysics::InvalidJointHandle;
        AZ::Vector3 m_childInitialPos;
    };

    using JointTypes = testing::Types<
        D6JointLimitConfiguration, 
        FixedJointConfiguration, 
        BallJointConfiguration, 
        HingeJointConfiguration>;
    TYPED_TEST_CASE(PhysXJointsApiTest, JointTypes);

    TYPED_TEST(PhysXJointsApiTest, Joint_ChildFollowsParent)
    {
        TypeParam jointConfiguration;
        AzPhysics::JointHandle jointHandle = AzPhysics::InvalidJointHandle;

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            jointHandle = sceneInterface->AddJoint(this->m_testSceneHandle, &jointConfiguration, this->m_parentBodyHandle, this->m_childBodyHandle);
        }

        EXPECT_NE(jointHandle, AzPhysics::InvalidJointHandle);

        // run physics to trigger the the move of parent body
        TestUtils::UpdateScene(this->m_testSceneHandle, AzPhysics::SystemConfiguration::DefaultFixedTimestep, 1);

        AZ::Vector3 childCurrentPos;

        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            auto* childBody = sceneInterface->GetSimulatedBodyFromHandle(this->m_testSceneHandle, this->m_childBodyHandle);
            childCurrentPos = childBody->GetPosition();
        }

        EXPECT_GT(childCurrentPos.GetX(), this->m_childInitialPos.GetX());
    }
}
