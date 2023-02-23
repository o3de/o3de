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
            float groundDetectionBoxHeight = DefaultGroundDetectionBoxHeight,
            const AZ::Transform& floorTransform = DefaultFloorTransform)
            : m_sceneHandle(sceneHandle)
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                m_testScene = physicsSystem->GetScene(m_sceneHandle);
            }
            AZ_Assert(m_testScene != nullptr, "GameplayTestBasis: the Test scene is null.");
            SetUp(gravityMultiplier, groundDetectionBoxHeight, floorTransform);
        }

        void SetUp(float gravityMultiplier = DefaultGravityMultiplier,
            float groundDetectionBoxHeight = DefaultGroundDetectionBoxHeight,
            const AZ::Transform& floorTransform = DefaultFloorTransform)
        {
            m_floor = PhysX::TestUtils::AddStaticFloorToScene(m_sceneHandle, floorTransform);

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

        
        //! Function to remove the ground for tests where the ground can interfere with the intent of the test. 
        void RemoveGround()
        {
            m_testScene->RemoveSimulatedBody(m_floor->m_bodyHandle);
        }

        AzPhysics::Scene* m_testScene = nullptr;
        AzPhysics::SceneHandle m_sceneHandle;
        AzPhysics::StaticRigidBody* m_floor = nullptr;
        AZStd::unique_ptr<AZ::Entity> m_controllerEntity;
        Physics::Character* m_controller = nullptr;
        PhysX::CharacterGameplayComponent* m_gameplayController = nullptr;
        float m_timeStep = AzPhysics::SystemConfiguration::DefaultFixedTimestep;
    };

    TEST_F(PhysXDefaultWorldTest, CharacterGameplayController_GravitySets)
    {
        const float expectedGravityMultiplier = 2.5f;

        GameplayTestBasis basis(m_testSceneHandle, DefaultGravityMultiplier, DefaultGroundDetectionBoxHeight, DefaultFloorTransform);
        basis.m_gameplayController->SetGravityMultiplier(expectedGravityMultiplier);
        EXPECT_THAT(expectedGravityMultiplier, testing::FloatEq(basis.m_gameplayController->GetGravityMultiplier()));
    }

    TEST_F(PhysXDefaultWorldTest, CharacterGameplayController_GravitySetsWhileMoving)
    {
        const float expectedGravityMultiplier = 2.5f;

        GameplayTestBasis basis(m_testSceneHandle, DefaultGravityMultiplier, DefaultGroundDetectionBoxHeight, DefaultFloorTransform);

        // Let scene run for a few moments so the entity can be manipulated by gravity from the gameplay component
        int duration = 10;

        for (int i = 0; i < duration; i++)
        {
            basis.m_gameplayController->SetGravityMultiplier(expectedGravityMultiplier + i);
            basis.Update();

            EXPECT_THAT(expectedGravityMultiplier + i, testing::FloatEq(basis.m_gameplayController->GetGravityMultiplier()));
        }
    }

    using PhysXDefaultWorldTestWithParamFixture = PhysXDefaultWorldTestWithParam<int>;

    TEST_P(PhysXDefaultWorldTestWithParamFixture, CharacterGameplayController_EntityFallsUnderGravity)
    {
        GameplayTestBasis basis(m_testSceneHandle, DefaultGravityMultiplier, DefaultGroundDetectionBoxHeight, DefaultFloorTransform);

        // Remove the Ground so we can can test falling without worrying about accidental collisions with objects.
        basis.RemoveGround();        
        
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
