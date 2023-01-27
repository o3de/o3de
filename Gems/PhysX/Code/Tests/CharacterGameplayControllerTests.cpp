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
    namespace Internal
    {
    }

    // transform for a floor centred at x = 0, y = 0, with top at level z = 0
    static const AZ::Transform DefaultFloorTransform = AZ::Transform::CreateTranslation(AZ::Vector3::CreateAxisZ(-0.5f));

    class ControllerTestBasis
    {
    public:
        ControllerTestBasis(AzPhysics::SceneHandle sceneHandle, const AZ::Transform& floorTransform = DefaultFloorTransform)
            : m_sceneHandle(sceneHandle)
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                m_testScene = physicsSystem->GetScene(m_sceneHandle);
            }
            AZ_Assert(m_testScene != nullptr, "ControllerTestBasis: the Test scene is null.");
            SetUp(floorTransform);
        }

        void SetUp(const AZ::Transform& floorTransform = DefaultFloorTransform)
        {
            m_floor = PhysX::TestUtils::AddStaticFloorToScene(m_sceneHandle, floorTransform);

            m_controllerEntity = AZStd::make_unique<AZ::Entity>("CharacterEntity");
            m_controllerEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());

            auto capsuleShapeConfiguration = AZStd::make_unique<Physics::CapsuleShapeConfiguration>();
            auto characterConfiguration = AZStd::make_unique<Physics::CharacterConfiguration>();
            characterConfiguration->m_maximumSlopeAngle = 25.0f;
            characterConfiguration->m_stepHeight = 0.2f;

            m_controllerEntity->CreateComponent<CharacterControllerComponent>(
                AZStd::move(characterConfiguration), AZStd::move(capsuleShapeConfiguration));
            m_controllerEntity->Init();
            m_controllerEntity->Activate();

            Physics::CharacterRequestBus::EventResult(m_controller, m_controllerEntity->GetId(), &Physics::CharacterRequests::GetCharacter);
        }

        void Update(const AZ::Vector3& velocity, AZ::u32 numTimeSteps = 1)
        {
            if (auto* physXSystem = GetPhysXSystem())
            {
                for (AZ::u32 i = 0; i < numTimeSteps; i++)
                {
                    Physics::CharacterRequestBus::Event(m_controllerEntity->GetId(), &Physics::CharacterRequests::AddVelocity, velocity);
                    physXSystem->Simulate(m_timeStep);
                }
            }
        }

        AzPhysics::Scene* m_testScene;
        AzPhysics::SceneHandle m_sceneHandle;
        AzPhysics::StaticRigidBody* m_floor;
        AZStd::unique_ptr<AZ::Entity> m_controllerEntity;
        Physics::Character* m_controller = nullptr;
        float m_timeStep = AzPhysics::SystemConfiguration::DefaultFixedTimestep;
    };

    TEST_F(PhysXDefaultWorldTest, CharacterGameplayController_GravitySetsWhileMoving)
    {
        PhysX::CharacterGameplayComponent* gameplayComponent = nullptr;
        AzFramework::TransformComponent* transformComponent = nullptr;

        float expectedGravityMultiplier = 2.5f;

        // Create character
        auto gameplayEntity = AZStd::make_unique<AZ::Entity>("GameplayEntity");
        {
            auto characterConfiguration = AZStd::make_unique<Physics::CharacterConfiguration>();
            auto characterShapeConfiguration = AZStd::make_unique<Physics::CapsuleShapeConfiguration>();

            characterShapeConfiguration->m_height = 1.5f;
            characterShapeConfiguration->m_radius = 0.5f;

            PhysX::CharacterGameplayConfiguration characterGameplayConfiguration;

            characterGameplayConfiguration.m_gravityMultiplier = expectedGravityMultiplier;
            characterGameplayConfiguration.m_groundDetectionBoxHeight = 0.05f;

            gameplayEntity->CreateComponent<CharacterControllerComponent>(
                AZStd::move(characterConfiguration), AZStd::move(characterShapeConfiguration));
            transformComponent = gameplayEntity->CreateComponent<AzFramework::TransformComponent>();
            gameplayComponent = gameplayEntity->CreateComponent<CharacterGameplayComponent>(characterGameplayConfiguration);

            transformComponent->SetWorldTM(AZ::Transform::Identity());
        }
        gameplayEntity->Init();
        gameplayEntity->Activate();

        // Let scene run for a few moments so the entity can be manipulated by gravity from the gameplay component
        int duration = 10;
        float timeStep = AzPhysics::SystemConfiguration::DefaultFixedTimestep;

        if (auto* physXSystem = GetPhysXSystem())
        {
            for (int i = 0; i < duration; i++)
            {
                gameplayComponent->SetGravityMultiplier(expectedGravityMultiplier + i);
                physXSystem->Simulate(timeStep);

                EXPECT_THAT(expectedGravityMultiplier + i, testing::FloatEq(gameplayComponent->GetGravityMultiplier()));
            }
        }
    }

} // namespace PhysX
