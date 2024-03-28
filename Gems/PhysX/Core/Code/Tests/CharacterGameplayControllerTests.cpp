/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/PhysicsScene.h>

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AZTestShared/Utils/Utils.h>

#include <PhysXCharacters/API/CharacterController.h>
#include <PhysXCharacters/API/CharacterUtils.h>
#include <PhysXCharacters/Components/CharacterControllerComponent.h>
#include <PhysXCharacters/Components/CharacterGameplayComponent.h>

#include <PhysX/ComponentTypeIds.h>
#include <PhysX/SystemComponentBus.h>

#include <Source/CapsuleColliderComponent.h>
#include <Source/SphereColliderComponent.h>

#include <Tests/PhysXTestCommon.h>
#include <Tests/PhysXTestFixtures.h>
#include <Tests/PhysXTestUtil.h>
#include <Tests/PhysXTestEnvironment.h>

#include <System/PhysXSystem.h>

namespace PhysX
{
    // Default Values for the Gamplay Controller
    static const float DefaultGravityMultiplier = 1.0f;
    static const float DefaultGroundDetectionBoxHeight = 0.05f;

    class GameplayTestBasis
    {
    public:
        GameplayTestBasis(
            AzPhysics::SceneHandle sceneHandle,
            float gravityMultiplier = DefaultGravityMultiplier,
            float groundDetectionBoxHeight = DefaultGroundDetectionBoxHeight)
            : m_sceneHandle(sceneHandle)
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                m_testScene = physicsSystem->GetScene(m_sceneHandle);
            }
            AZ_Assert(m_testScene != nullptr, "GameplayTestBasis: the Test scene is null.");
            SetUp(gravityMultiplier, groundDetectionBoxHeight);
        }

        void SetUp(float gravityMultiplier = DefaultGravityMultiplier,
            float groundDetectionBoxHeight = DefaultGroundDetectionBoxHeight)
        {
            m_controllerEntity = AZStd::make_unique<AZ::Entity>("CharacterEntity");

            // Transform Setup
            m_controllerEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());

            // Character Controller Setup
            auto capsuleShapeConfiguration = AZStd::make_unique<Physics::CapsuleShapeConfiguration>();
            auto characterConfiguration = AZStd::make_unique<Physics::CharacterConfiguration>();
            characterConfiguration->m_maximumSlopeAngle = 25.0f;
            characterConfiguration->m_stepHeight = 0.2f;

            m_controllerEntity->CreateComponent<CharacterControllerComponent>(
                AZStd::move(characterConfiguration), AZStd::move(capsuleShapeConfiguration));

            // Gameplay Controller Setup
            PhysX::CharacterGameplayConfiguration characterGameplayConfiguration;

            characterGameplayConfiguration.m_gravityMultiplier = gravityMultiplier;
            characterGameplayConfiguration.m_groundDetectionBoxHeight = groundDetectionBoxHeight;
            m_gameplayController = m_controllerEntity->CreateComponent<CharacterGameplayComponent>(characterGameplayConfiguration);
            
            // Wrap UP
            m_controllerEntity->Init();
            m_controllerEntity->Activate();

            Physics::CharacterRequestBus::EventResult(m_controller, m_controllerEntity->GetId(), &Physics::CharacterRequests::GetCharacter);
        }

        void Update(AZ::u32 numTimeSteps = 1)
        {
            if (auto* physXSystem = GetPhysXSystem())
            {
                for (AZ::u32 i = 0; i < numTimeSteps; i++)
                {
                    physXSystem->Simulate(m_timeStep);
                }
            }
        }

        //! Add the ground to the scene via the test basis for tests where the ground is needed.
        //! This function will only ever add one ground element to the scene.
        void SetupGround(const AZ::Transform& floorTransform = DefaultFloorTransform)
        {
            m_floor = PhysX::TestUtils::AddStaticFloorToScene(m_sceneHandle, floorTransform);
        }

        AzPhysics::Scene* m_testScene = nullptr;
        AzPhysics::SceneHandle m_sceneHandle;
        AzPhysics::StaticRigidBody* m_floor = nullptr;
        AZStd::unique_ptr<AZ::Entity> m_controllerEntity;
        Physics::Character* m_controller = nullptr;
        PhysX::CharacterGameplayComponent* m_gameplayController = nullptr;
        float m_timeStep = AzPhysics::SystemConfiguration::DefaultFixedTimestep;
    };

    //! Test that verifies that the Gravity Set & Get methods operate as expected.
    TEST_F(PhysXDefaultWorldTest, CharacterGameplayController_GravitySets)
    {
        const float expectedGravityMultiplier = 2.5f;
        GameplayTestBasis basis(m_testSceneHandle, DefaultGravityMultiplier, DefaultGroundDetectionBoxHeight);

        // Set the gravity via the accessor
        basis.m_gameplayController->SetGravityMultiplier(expectedGravityMultiplier);
        EXPECT_THAT(basis.m_gameplayController->GetGravityMultiplier(), testing::FloatEq(expectedGravityMultiplier));
    }

    //! Test that verifies that the Gravity Set & Get methods operate as expected while the scene ticks
    //! and interacts with the entity & its components.
    TEST_F(PhysXDefaultWorldTest, CharacterGameplayController_GravitySetsWhileMoving)
    {
        const float expectedGravityMultiplier = 2.5f;
        GameplayTestBasis basis(m_testSceneHandle, DefaultGravityMultiplier, DefaultGroundDetectionBoxHeight);

        // Let scene run for a few moments so the entity can be manipulated by gravity from the gameplay component.
        int duration = 10;
        for (int i = 0; i < duration; i++)
        {
            basis.m_gameplayController->SetGravityMultiplier(expectedGravityMultiplier + i);
            basis.Update();

            EXPECT_THAT(basis.m_gameplayController->GetGravityMultiplier(), testing::FloatEq(expectedGravityMultiplier + i));
        }
    }

    //! Test that verifies that the Falling Velocity Set & Get methods function as expected.
    TEST_F(PhysXDefaultWorldTest, CharacterGameplayController_FallingVelocitySets)
    {
        const AZ::Vector3 expectedVelocity(0.0f, 0.0f, 22.0f);
        GameplayTestBasis basis(m_testSceneHandle, DefaultGravityMultiplier, DefaultGroundDetectionBoxHeight);

        // Set the falling velocity to the expected + original velocity to verify that the velociy will change.
        basis.m_gameplayController->SetFallingVelocity(expectedVelocity);

        // Get the end velocity and validate that it is the expected value.
        auto endVelocity = basis.m_gameplayController->GetFallingVelocity();

        EXPECT_THAT(endVelocity.GetZ(), testing::FloatNear(expectedVelocity.GetZ(), 0.001f));
    }

    //! Test the verifies that the Falling Velocity sets and uses the set value to interact with the tick incrementing.
    TEST_F(PhysXDefaultWorldTest, CharacterGameplayController_FallingVelocitySetsWhileMoving)
    {
        const AZ::Vector3 expectedVelocity(0.0f, 0.0f, 22.0f);
        GameplayTestBasis basis(m_testSceneHandle, DefaultGravityMultiplier, DefaultGroundDetectionBoxHeight);

        // Run the scene for a while and validate that the set velocity is working
        for (int i = 0; i < 10; i++)
        {
            // Calculate and set velocity based upon the original velocity, expected velocity, and loop count modifier
            auto setVelocity = expectedVelocity + AZ::Vector3(0.0f, 0.0f, float(i));
            basis.m_gameplayController->SetFallingVelocity(setVelocity);

            basis.Update();

            // Get the endVelocity & current gravity to perform calculations on.
            auto endVelocity = basis.m_gameplayController->GetFallingVelocity();
            const auto gravity = basis.m_testScene->GetGravity();

            // The end velocity should be decelerated by gt when g is gravity acceleration and t is the length of time
            const float expectedVelocityChange = gravity.GetZ() * basis.m_timeStep;

            // Verify that the end velocity is the set velocity minus the deceleration for a single time step
            EXPECT_THAT(endVelocity.GetZ(), testing::FloatNear(setVelocity.GetZ() + expectedVelocityChange, 0.001f));
        }
    }

    //! Test that verifies that the Ground Detection Set & Get methods function as expected.
    TEST_F(PhysXDefaultWorldTest, CharacterGameplayController_SetGroundDetectionHeight)
    {
        GameplayTestBasis basis(m_testSceneHandle, DefaultGravityMultiplier, DefaultGroundDetectionBoxHeight);

        const float expectedHeight = 1.2f;
        const auto originalHeight = basis.m_gameplayController->GetGroundDetectionBoxHeight();

        // Set the ground detection to the expected height.
        basis.m_gameplayController->SetGroundDetectionBoxHeight(expectedHeight);

        // Get the end height and verify that it is set to the expected value set.
        const auto endHeight = basis.m_gameplayController->GetGroundDetectionBoxHeight();

        EXPECT_THAT(endHeight, testing::Not(testing::FloatNear(originalHeight, 0.001f)));
        EXPECT_THAT(endHeight, testing::FloatNear(expectedHeight, 0.001f));
    }

    //! Test to determine if the ground is detected while moving.
    TEST_F(PhysXDefaultWorldTest, CharacterGameplayController_GroundDetectedWhileMoving)
    {
        // Create a test scene with a Ground Element.
        GameplayTestBasis basis(m_testSceneHandle, DefaultGravityMultiplier, DefaultGroundDetectionBoxHeight);
        basis.SetupGround();

        // Set the Gameplay Controller entity above the ground.
        const AZ::Transform startingPosition = AZ::Transform::CreateTranslation(AZ::Vector3(0.0f, 0.0f, 0.1f));
        basis.m_controller->SetTransform(startingPosition);

        // Verify the ground state of the Gameplay Controller is not detecting at the start.
        EXPECT_FALSE(basis.m_gameplayController->IsOnGround());

        // Let the scene run until the Gameplay Controller detects the ground.
        bool groundDetected = false;
        for (int i = 0; i < 10; i++)
        {
            basis.Update();

            if (basis.m_gameplayController->IsOnGround())
            {
                groundDetected = true;
                break;
            }
        }

        // Validate that the ground was detected while the scene was running.
        EXPECT_TRUE(groundDetected);
    }

    using PhysXDefaultWorldTestWithParamFixture = PhysXDefaultWorldTestWithParam<int>;

    //! Test that verifies that an entity with a gameplay controller is falling under gravity as expected.
    TEST_P(PhysXDefaultWorldTestWithParamFixture, CharacterGameplayController_EntityFallsUnderGravity)
    {
        GameplayTestBasis basis(m_testSceneHandle, DefaultGravityMultiplier, DefaultGroundDetectionBoxHeight);     
        
        // Let scene run for a few moments so the entity can be manipulated by gravity from the gameplay component
        const auto startPosition = basis.m_controller->GetPosition();
        const int timeStepCount = GetParam(); 
        const float timeStep = AzPhysics::SystemConfiguration::DefaultFixedTimestep;
        float totalTime = 0.0f;

        for (int i = 0; i < timeStepCount; i++)
        {
            basis.Update();
            totalTime += timeStep;
        }

        const auto endPosition = basis.m_controller->GetPosition();
        const auto gravity = basis.m_testScene->GetGravity();
        // The actual distance fallen is quadratic and the relative error is about equal to the timestep divided by the total time.
        const auto relativeError = timeStep / totalTime;
        // calculate distance fallen (d = 0.5 * g * t^2)
        const float relativeDistanceFell = 0.5f * gravity.GetZ() * (totalTime * totalTime);
        const float calculatedDistanceFell = relativeDistanceFell - (relativeDistanceFell * relativeError);

        EXPECT_THAT(endPosition.GetZ(), testing::FloatNear((startPosition.GetZ() + calculatedDistanceFell), 0.001f));
    }

    INSTANTIATE_TEST_CASE_P(PhysXDefaultWorldTest, PhysXDefaultWorldTestWithParamFixture, ::testing::Values(10, 30, 60, 90, 120, 136, 180));

} // namespace PhysX
