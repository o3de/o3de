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
#include <D6JointComponent.h>
#include <FixedJointComponent.h>
#include <HingeJointComponent.h>
#include <PhysX/Joint/Configuration/PhysXJointConfiguration.h>
#include <PhysX/Joint/PhysXJointRequestsBus.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/math.h>
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

    // D6 drive tests
    //
    // The follower is a 1 DOF slider: linear X is free, everything else locked, so the drive on the X axis
    // is the only thing that can move it. Gravity is off and the lead is heavy and stationary, which makes
    // the follower's X displacement a direct readout of what the drive did.

    namespace D6DriveTest
    {
        const AZ::Vector3 LeadPosition(5.0f, 0.0f, 0.0f);
        const AZ::Vector3 FollowerPosition(-5.0f, 0.0f, 0.0f);
        const AZ::Vector3 JointLocalPosition(5.0f, 0.0f, 0.0f);
        const float LargeForceLimit = 1.0e6f;
    }

    AZStd::unique_ptr<AZ::Entity> AddD6FollowerEntity(
        AzPhysics::SceneHandle sceneHandle,
        const AZ::Vector3& position,
        const AZ::Vector3& initialLinearVelocity,
        AZStd::shared_ptr<JointComponentConfiguration> jointConfig,
        const D6JointComponentConfiguration& d6Config)
    {
        auto entity = AZStd::make_unique<AZ::Entity>("d6TestEntity");

        AZ::TransformConfig transformConfig;
        transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
        entity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);

        auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
        auto boxShapeConfiguration = AZStd::make_shared<Physics::BoxShapeConfiguration>();
        auto* boxColliderComponent = entity->CreateComponent<BoxColliderComponent>();
        boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfiguration, boxShapeConfiguration) });

        AzPhysics::RigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_initialLinearVelocity = initialLinearVelocity;
        rigidBodyConfig.m_gravityEnabled = false;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig, sceneHandle);

        jointConfig->m_followerEntity = entity->GetId();
        const JointGenericProperties genericProperties;
        entity->CreateComponent<D6JointComponent>(*jointConfig, genericProperties, d6Config);

        entity->Init();
        entity->Activate();

        return entity;
    }

    D6JointComponentConfiguration MakeLinearXDriveConfig(
        D6JointDriveType driveType, float stiffness, float damping, float forceLimit)
    {
        D6JointComponentConfiguration d6Config;
        d6Config.m_motionX = D6JointAxis::Free;
        d6Config.m_driveX.m_driveType = driveType;
        d6Config.m_driveX.m_stiffness = stiffness;
        d6Config.m_driveX.m_damping = damping;
        d6Config.m_driveX.m_forceLimit = forceLimit;
        return d6Config;
    }

    struct D6DriveTestBodies
    {
        AZStd::unique_ptr<AZ::Entity> m_lead;
        AZStd::unique_ptr<AZ::Entity> m_follower;
    };

    //! yOffset separates independent lead/follower pairs sharing one scene.
    D6DriveTestBodies CreateD6DriveTestBodies(
        AzPhysics::SceneHandle sceneHandle,
        const D6JointComponentConfiguration& d6Config,
        const AZ::Vector3& followerInitialLinearVelocity = AZ::Vector3::CreateZero(),
        float yOffset = 0.0f)
    {
        const AZ::Vector3 offset(0.0f, yOffset, 0.0f);

        auto lead = AddBodyColliderEntity<JointComponent>(
            sceneHandle, D6DriveTest::LeadPosition + offset, AZ::Vector3::CreateZero());

        auto jointConfig = AZStd::make_shared<JointComponentConfiguration>();
        jointConfig->m_leadEntity = lead->GetId();
        jointConfig->m_localTransformFromFollower = AZ::Transform::CreateTranslation(D6DriveTest::JointLocalPosition);

        auto follower = AddD6FollowerEntity(
            sceneHandle, D6DriveTest::FollowerPosition + offset, followerInitialLinearVelocity, jointConfig, d6Config);

        return { AZStd::move(lead), AZStd::move(follower) };
    }

    AZ::EntityComponentIdPair GetD6JointId(const AZ::Entity& followerEntity)
    {
        const auto* jointComponent = followerEntity.FindComponent<D6JointComponent>();
        EXPECT_NE(jointComponent, nullptr);
        return AZ::EntityComponentIdPair(
            followerEntity.GetId(), jointComponent ? jointComponent->GetId() : AZ::InvalidComponentId);
    }

    void SetDriveVelocityX(const AZ::Entity& followerEntity, float velocityX)
    {
        PhysX::JointRequestBus::Event(
            GetD6JointId(followerEntity),
            &PhysX::JointRequests::SetVelocityGeneral,
            AZ::Vector3(velocityX, 0.0f, 0.0f),
            AZ::Vector3::CreateZero());
    }

    TEST_F(PhysXJointsTest, Joint_D6Joint_VelocityDriveMovesFollowerAlongFreeAxis)
    {
        auto bodies = CreateD6DriveTestBodies(
            m_testSceneHandle, MakeLinearXDriveConfig(D6JointDriveType::Velocity, 0.0f, 1000.0f, D6DriveTest::LargeForceLimit));

        SetDriveVelocityX(*bodies.m_follower, 0.5f);

        const AZ::Vector3 followerEndPosition = RunJointTest(m_defaultScene, bodies.m_follower->GetId());

        const float displacement = followerEndPosition.GetX() - D6DriveTest::FollowerPosition.GetX();
        EXPECT_GT(AZStd::abs(displacement), 0.5f);
    }

    TEST_F(PhysXJointsTest, Joint_D6Joint_WithoutDriveFollowerStaysAtRest)
    {
        // same free axis, but no drive to act on it
        auto bodies = CreateD6DriveTestBodies(
            m_testSceneHandle, MakeLinearXDriveConfig(D6JointDriveType::None, 0.0f, 1000.0f, D6DriveTest::LargeForceLimit));

        SetDriveVelocityX(*bodies.m_follower, 0.5f);

        const AZ::Vector3 followerEndPosition = RunJointTest(m_defaultScene, bodies.m_follower->GetId());

        const float displacement = followerEndPosition.GetX() - D6DriveTest::FollowerPosition.GetX();
        EXPECT_NEAR(displacement, 0.0f, 0.01f);
    }

    TEST_F(PhysXJointsTest, Joint_D6Joint_VelocityDriveReversesWithSign)
    {
        const D6JointComponentConfiguration d6Config =
            MakeLinearXDriveConfig(D6JointDriveType::Velocity, 0.0f, 1000.0f, D6DriveTest::LargeForceLimit);

        auto forwardBodies = CreateD6DriveTestBodies(m_testSceneHandle, d6Config, AZ::Vector3::CreateZero(), 0.0f);
        auto reverseBodies = CreateD6DriveTestBodies(m_testSceneHandle, d6Config, AZ::Vector3::CreateZero(), 10.0f);

        SetDriveVelocityX(*forwardBodies.m_follower, 0.5f);
        SetDriveVelocityX(*reverseBodies.m_follower, -0.5f);

        const AZ::Vector3 forwardEndPosition = RunJointTest(m_defaultScene, forwardBodies.m_follower->GetId());

        AZ::Vector3 reverseEndPosition = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(
            reverseEndPosition, reverseBodies.m_follower->GetId(), &AZ::TransformBus::Events::GetWorldTranslation);

        const float forwardDisplacement = forwardEndPosition.GetX() - D6DriveTest::FollowerPosition.GetX();
        const float reverseDisplacement = reverseEndPosition.GetX() - D6DriveTest::FollowerPosition.GetX();

        EXPECT_LT(forwardDisplacement * reverseDisplacement, 0.0f);
    }

    // Both followers are launched away from the joint; the drives are identical apart from the force limit,
    // so the one allowed to exert force must end up measurably less far along than the one that cannot.
    TEST_F(PhysXJointsTest, Joint_D6Joint_PositionDriveResistsInitialVelocityUnlessForceLimited)
    {
        const AZ::Vector3 followerInitialLinearVelocity(-2.0f, 0.0f, 0.0f);

        auto drivenBodies = CreateD6DriveTestBodies(
            m_testSceneHandle,
            MakeLinearXDriveConfig(D6JointDriveType::Position, 10000.0f, 1000.0f, D6DriveTest::LargeForceLimit),
            followerInitialLinearVelocity,
            0.0f);
        auto forceLimitedBodies = CreateD6DriveTestBodies(
            m_testSceneHandle,
            MakeLinearXDriveConfig(D6JointDriveType::Position, 10000.0f, 1000.0f, 0.0001f),
            followerInitialLinearVelocity,
            10.0f);

        const AZ::Vector3 drivenEndPosition = RunJointTest(m_defaultScene, drivenBodies.m_follower->GetId());

        AZ::Vector3 forceLimitedEndPosition = AZ::Vector3::CreateZero();
        AZ::TransformBus::EventResult(
            forceLimitedEndPosition, forceLimitedBodies.m_follower->GetId(), &AZ::TransformBus::Events::GetWorldTranslation);

        const float drivenDisplacement = drivenEndPosition.GetX() - D6DriveTest::FollowerPosition.GetX();
        const float forceLimitedDisplacement = forceLimitedEndPosition.GetX() - D6DriveTest::FollowerPosition.GetX();

        EXPECT_LT(forceLimitedDisplacement, -1.0f);
        EXPECT_GT(drivenDisplacement - forceLimitedDisplacement, 1.0f);
    }

    TEST_F(PhysXJointsTest, Joint_D6Joint_GetVelocityGeneralReturnsCommandedDriveVelocity)
    {
        auto bodies = CreateD6DriveTestBodies(
            m_testSceneHandle, MakeLinearXDriveConfig(D6JointDriveType::Velocity, 0.0f, 1000.0f, D6DriveTest::LargeForceLimit));

        const AZ::Vector3 commandedLinear(0.5f, 0.0f, 0.0f);
        const AZ::Vector3 commandedAngular(0.0f, 0.0f, 0.25f);

        PhysX::JointRequestBus::Event(
            GetD6JointId(*bodies.m_follower),
            &PhysX::JointRequests::SetVelocityGeneral,
            commandedLinear,
            commandedAngular);

        AZStd::pair<AZ::Vector3, AZ::Vector3> readBack;
        PhysX::JointRequestBus::EventResult(
            readBack, GetD6JointId(*bodies.m_follower), &PhysX::JointRequests::GetVelocityGeneral);

        EXPECT_NEAR(readBack.first.GetX(), commandedLinear.GetX(), 1e-3f);
        EXPECT_NEAR(readBack.first.GetY(), commandedLinear.GetY(), 1e-3f);
        EXPECT_NEAR(readBack.first.GetZ(), commandedLinear.GetZ(), 1e-3f);
        EXPECT_NEAR(readBack.second.GetX(), commandedAngular.GetX(), 1e-3f);
        EXPECT_NEAR(readBack.second.GetY(), commandedAngular.GetY(), 1e-3f);
        EXPECT_NEAR(readBack.second.GetZ(), commandedAngular.GetZ(), 1e-3f);
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
    TYPED_TEST_SUITE(PhysXJointsApiTest, JointTypes);

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
