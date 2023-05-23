/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/PhysicsScene.h>

#include <PhysXCharacters/API/CharacterController.h>
#include <PhysXCharacters/API/CharacterUtils.h>
#include <PhysXCharacters/Components/CharacterControllerComponent.h>

#include <AZTestShared/Math/MathTestHelpers.h>
#include <AZTestShared/Utils/Utils.h>
#include <AzFramework/Components/TransformComponent.h>
#include <PhysX/ComponentTypeIds.h>
#include <PhysX/SystemComponentBus.h>
#include <Source/SphereColliderComponent.h>
#include <Source/CapsuleColliderComponent.h>
#include <System/PhysXSystem.h>
#include <Tests/PhysXTestFixtures.h>
#include <Tests/PhysXTestUtil.h>
#include <Tests/PhysXTestCommon.h>

namespace PhysX
{
    namespace Internal
    {
        void AddColliderComponentToEntity(AZ::Entity* entity, const Physics::ColliderConfiguration& colliderConfiguration, const Physics::ShapeConfiguration& shapeConfiguration)
        {
            Physics::ShapeType shapeType = shapeConfiguration.GetShapeType();

            switch (shapeType)
            {
            case Physics::ShapeType::Sphere:
            {
                const Physics::SphereShapeConfiguration& sphereConfiguration = static_cast<const Physics::SphereShapeConfiguration&>(shapeConfiguration);
                auto sphereColliderComponent = entity->CreateComponent<SphereColliderComponent>();
                sphereColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(
                    AZStd::make_shared<Physics::ColliderConfiguration>(colliderConfiguration),
                    AZStd::make_shared<Physics::SphereShapeConfiguration>(sphereConfiguration)) });
            }
            break;
            case Physics::ShapeType::Box:
            {
                const Physics::BoxShapeConfiguration& boxConfiguration = static_cast<const Physics::BoxShapeConfiguration&>(shapeConfiguration);
                auto boxColliderComponent = entity->CreateComponent<BoxColliderComponent>();
                boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(
                    AZStd::make_shared<Physics::ColliderConfiguration>(colliderConfiguration),
                    AZStd::make_shared<Physics::BoxShapeConfiguration>(boxConfiguration)) });
            }
            break;
            case Physics::ShapeType::Capsule:
            {
                const Physics::CapsuleShapeConfiguration& capsuleConfiguration = static_cast<const Physics::CapsuleShapeConfiguration&>(shapeConfiguration);
                auto capsuleColliderComponent = entity->CreateComponent<CapsuleColliderComponent>();
                capsuleColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(
                    AZStd::make_shared<Physics::ColliderConfiguration>(colliderConfiguration),
                    AZStd::make_shared<Physics::CapsuleShapeConfiguration>(capsuleConfiguration)) });
            }
            break;
            default:
            {
                AZ_Error("PhysX", false,
                    "AddColliderComponentToEntity(): Using Shape of type %d is not implemented.", static_cast<AZ::u8>(shapeType));
            }
            break;
            }
        }
    }

    class ControllerTestBasis
    {
    public:
        ControllerTestBasis(AzPhysics::SceneHandle sceneHandle,
            const Physics::ShapeType shapeType = Physics::ShapeType::Capsule,
            const AZ::Transform& floorTransform = DefaultFloorTransform)
            : m_sceneHandle(sceneHandle)
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                m_testScene = physicsSystem->GetScene(m_sceneHandle);
            }
            AZ_Assert(m_testScene != nullptr, "ControllerTestBasis: the Test scene is null.");
            SetUp(shapeType, floorTransform);
        }

        void SetUp(const Physics::ShapeType shapeType = Physics::ShapeType::Capsule,
            const AZ::Transform& floorTransform = DefaultFloorTransform)
        {
            m_floor = PhysX::TestUtils::AddStaticFloorToScene(m_sceneHandle, floorTransform);

            m_controllerEntity = AZStd::make_unique<AZ::Entity>("CharacterEntity");
            m_controllerEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());

            auto characterConfiguration = AZStd::make_unique<Physics::CharacterConfiguration>();
            characterConfiguration->m_maximumSlopeAngle = 25.0f;
            characterConfiguration->m_stepHeight = 0.2f;

            if (shapeType == Physics::ShapeType::Capsule)
            {
                auto capsuleShapeConfiguration = AZStd::make_unique<Physics::CapsuleShapeConfiguration>();
                m_controllerEntity->CreateComponent<CharacterControllerComponent>(AZStd::move(characterConfiguration),
                    AZStd::move(capsuleShapeConfiguration));
            }
            else
            {
                auto boxShapeConfiguration = AZStd::make_unique<Physics::BoxShapeConfiguration>(AZ::Vector3(0.5f, 0.5f, 1.0f));
                m_controllerEntity->CreateComponent<CharacterControllerComponent>(AZStd::move(characterConfiguration),
                    AZStd::move(boxShapeConfiguration));
            }

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

    Physics::ShapeType controllerShapeTypes[] = { Physics::ShapeType::Capsule, Physics::ShapeType::Box };

    TEST_F(PhysXDefaultWorldTest, CharacterControllerWhenRotationSetReturnsCorrectRotation)
    {
        ControllerTestBasis basis(m_testSceneHandle);
        basis.Update(AZ::Vector3::CreateZero());

        // Set an arbitrary character rotation and base position through separate calls.
        // We deliberately set the rotation first and the base position second so that we can also verify
        // that setting the position *after* the rotation hasn't reintroduced a regression where setting the position
        // would clear the rotation.
        const AZ::Vector3 arbitraryPosition = AZ::Vector3(300.0f, 200.0f, 100.0f);
        const AZ::Quaternion arbitraryRotation = AZ::Quaternion::CreateFromEulerDegreesXYZ(AZ::Vector3(10.0f, 20.0f, 30.0f));
        basis.m_controller->SetRotation(arbitraryRotation);
        basis.m_controller->SetBasePosition(arbitraryPosition);

        auto characterTransform = basis.m_controller->GetTransform();

        // Verify that both the position and rotation are the same as what we set.
        EXPECT_EQ(characterTransform.GetTranslation(), arbitraryPosition);
        EXPECT_EQ(characterTransform.GetRotation(), arbitraryRotation);
    }

    TEST_F(PhysXDefaultWorldTest, CharacterController_UnimpededController_MovesAtDesiredVelocity)
    {
        ControllerTestBasis basis(m_testSceneHandle);
        basis.Update(AZ::Vector3::CreateZero());
        AZ::Vector3 desiredVelocity = AZ::Vector3::CreateAxisX();

        for (int i = 0; i < 50; i++)
        {
            AZ::Vector3 basePosition = basis.m_controller->GetBasePosition();
            EXPECT_TRUE(basePosition.IsClose(AZ::Vector3::CreateAxisX(basis.m_timeStep * i)));
            basis.Update(desiredVelocity);
            EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(desiredVelocity));
        }
    }

    TEST_F(PhysXDefaultWorldTest, CharacterController_MovingDirectlyTowardsStaticBox_StoppedByBox)
    {
        ControllerTestBasis basis(m_testSceneHandle);
        AZ::Vector3 velocity = AZ::Vector3::CreateAxisX();

        PhysX::TestUtils::AddStaticUnitBoxToScene(basis.m_sceneHandle, AZ::Vector3(1.5f, 0.0f, 0.5f));

        // run the simulation for a while so the controller should get to the box and stop
        basis.Update(velocity, 50);

        // the edge of the box is at x = 1.0, we expect to stop a distance short of that given by the sum of the
        // capsule radius (0.25) and the contact offset (0.1)
        AZ::Vector3 basePosition = basis.m_controller->GetBasePosition();
        EXPECT_TRUE(basePosition.IsClose(AZ::Vector3::CreateAxisX(0.65f)));

        // run the simulation some more and check that the controller is not moving in the direction of the box
        for (int i = 0; i < 10; i++)
        {
            AZ::Vector3 newBasePosition = basis.m_controller->GetBasePosition();
            EXPECT_TRUE(newBasePosition.IsClose(basePosition));
            EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(AZ::Vector3::CreateZero()));
            basePosition = newBasePosition;
            basis.Update(velocity);
        }
    }

    TEST_F(PhysXDefaultWorldTest, CharacterController_MovingDiagonallyTowardsStaticBox_SlidesAlongBox)
    {
        ControllerTestBasis basis(m_testSceneHandle);
        AZ::Vector3 velocity = AZ::Vector3(1.0f, 1.0f, 0.0f);

        PhysX::TestUtils::AddStaticUnitBoxToScene(basis.m_sceneHandle, AZ::Vector3(1.0f, 0.5f, 0.5f));

        // run the simulation for a while so the controller should get to the box and start sliding
        basis.Update(velocity, 20);

        // the controller should be sliding in the y direction now
        for (int i = 0; i < 10; i++)
        {
            velocity = basis.m_controller->GetVelocity();
            float vx = velocity.GetX();
            float vy = velocity.GetY();
            EXPECT_NEAR(vx, 0.0f, 1e-3f);
            EXPECT_NEAR(vy, 1.0f, 1e-3f);
            basis.Update(velocity);
        }
    }

    TEST_F(PhysXDefaultWorldTest, CharacterController_MovingOnSlope_CannotMoveAboveMaximumSlopeAngle)
    {
        // create a floor sloped at 30 degrees which should just be touching a controller with base position at the 
        // origin, with radius + contact offset = 0.25 + 0.1 = 0.35
        AZ::Transform slopedFloorTransform = AZ::Transform::CreateRotationY(-AZ::Constants::Pi / 6.0f);
        slopedFloorTransform.SetTranslation(
            AZ::Vector3::CreateAxisZ(0.35f) + slopedFloorTransform.TransformPoint(AZ::Vector3::CreateAxisZ(-0.85f)));
        ControllerTestBasis basis(m_testSceneHandle, Physics::ShapeType::Capsule, slopedFloorTransform);

        // we should be able to travel at right angles to the slope
        AZ::Vector3 desiredVelocity = AZ::Vector3::CreateAxisY();

        for (int i = 0; i < 50; i++)
        {
            basis.Update(desiredVelocity);
            EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(desiredVelocity));
        }

        // we should slide if we try to travel diagonally up the slope as it is steeper than our maximum of 25 degrees
        desiredVelocity = AZ::Vector3(1.0f, 1.0f, 0.0f);

        // run a few frames to adjust to the change in direction
        basis.Update(desiredVelocity, 10);

        for (int i = 0; i < 50; i++)
        {
            basis.Update(desiredVelocity);
            AZ::Vector3 velocity = basis.m_controller->GetVelocity();
            float vx = velocity.GetX();
            float vy = velocity.GetY();
            EXPECT_NEAR(vx, 0.0f, 1e-3f);
            EXPECT_NEAR(vy, 1.0f, 1e-3f);
        }

        // shouldn't be able to travel directly up the 30 degree slope as our maximum slope angle is 25 degrees
        desiredVelocity = AZ::Vector3(1.0f, 0.0f, 0.0f);

        for (int i = 0; i < 50; i++)
        {
            basis.Update(desiredVelocity);
            EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(AZ::Vector3::CreateZero()));
        }

        // should be able to move down the slope
        desiredVelocity = AZ::Vector3(-1.0f, 0.0f, -0.5f);

        // run a few frames to adjust to the change in direction
        basis.Update(desiredVelocity, 10);

        for (int i = 0; i < 50; i++)
        {
            basis.Update(desiredVelocity);
            EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(desiredVelocity));
        }
    }

    TEST_F(PhysXDefaultWorldTest, CharacterController_Steps_StoppedByTallStep)
    {
        ControllerTestBasis basis(m_testSceneHandle);

        PhysX::TestUtils::AddStaticUnitBoxToScene(basis.m_sceneHandle, AZ::Vector3(1.0f, 0.0f, -0.3f));
        PhysX::TestUtils::AddStaticUnitBoxToScene(basis.m_sceneHandle, AZ::Vector3(2.0f, 0.0f, 0.5f));

        AZ::Vector3 desiredVelocity = AZ::Vector3::CreateAxisX();

        for (int i = 0; i < 50; i++)
        {
            basis.Update(desiredVelocity);
            AZ::Vector3 velocity = basis.m_controller->GetVelocity();
            float vx = velocity.GetX();
            float vy = velocity.GetY();
            EXPECT_NEAR(vx, 1.0f, 1e-3f);
            EXPECT_NEAR(vy, 0.0f, 1e-3f);
        }

        // expect the base of the controller to now be at the height of the short step (0.2)
        float expectedBaseHeight = 0.2f;
        float baseHeight = basis.m_controller->GetBasePosition().GetZ();
        EXPECT_NEAR(baseHeight, expectedBaseHeight, 1e-3f);

        // after another 50 updates, we should have been stopped by the tall step
        basis.Update(desiredVelocity, 50);
        EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(AZ::Vector3::CreateZero()));
        baseHeight = basis.m_controller->GetBasePosition().GetZ();
        EXPECT_NEAR(baseHeight, expectedBaseHeight, 1e-3f);
    }

    using CharacterControllerFixture = PhysXDefaultWorldTestWithParam<Physics::ShapeType>;

    TEST_P(CharacterControllerFixture, CharacterController_ResizedController_CannotFitUnderLowBox)
    {
        Physics::ShapeType shapeType = GetParam();
        ControllerTestBasis basis(m_testSceneHandle, shapeType);

        // the bottom of the box will be at height 1.0
        PhysX::TestUtils::AddStaticUnitBoxToScene(basis.m_sceneHandle, AZ::Vector3(1.0f, 0.0f, 1.5f));

        // resize the controller so that it is too tall to fit under the box
        auto controller = static_cast<CharacterController*>(basis.m_controller);
        controller->Resize(1.3f);
        EXPECT_NEAR(controller->GetHeight(), 1.3f, 1e-3f);

        const AZ::Vector3 desiredVelocity = AZ::Vector3::CreateAxisX();

        basis.Update(desiredVelocity, 50);
        // movement should be impeded by the box because the controller is too tall to go under it
        EXPECT_TRUE(basis.m_controller->GetVelocity().IsClose(AZ::Vector3::CreateZero()));

        // resize the controller to a bit less tall than the height of the bottom of the box
        // leave some leeway under the box to account for the contact offset of the controller
        controller->Resize(0.6f);
        EXPECT_NEAR(controller->GetHeight(), 0.6f, 1e-3f);

        basis.Update(desiredVelocity, 50);
        // movement should now be unimpeded because the controller is short enough to go under the box
        const AZ::Vector3 velocity = basis.m_controller->GetVelocity();
        const float vx = velocity.GetX();
        const float vy = velocity.GetY();
        EXPECT_NEAR(vx, 1.0f, 1e-3f);
        EXPECT_NEAR(vy, 0.0f, 1e-3f);
    }

    TEST_P(CharacterControllerFixture, CharacterController_ResizingToNegativeHeight_EmitsError)
    {
        Physics::ShapeType shapeType = GetParam();
        ControllerTestBasis basis(m_testSceneHandle, shapeType);
        auto controller = static_cast<CharacterController*>(basis.m_controller);
        UnitTest::ErrorHandler errorHandler("PhysX requires controller height to be positive");
        controller->Resize(-0.2f);
        EXPECT_EQ(errorHandler.GetErrorCount(), 1);
    }

    INSTANTIATE_TEST_CASE_P(PhysXCharacters, CharacterControllerFixture, ::testing::ValuesIn(controllerShapeTypes));

    TEST_F(PhysXDefaultWorldTest, CharacterController_ResizingCapsuleControllerBelowTwiceRadius_EmitsError)
    {
        ControllerTestBasis basis(m_testSceneHandle);

        auto controller = static_cast<CharacterController*>(basis.m_controller);
        // the controller will have been made with the default radius of 0.25, so any height under 0.5 should
        // be impossible
        UnitTest::ErrorHandler errorHandler("Capsule height must exceed twice its radius");
        controller->Resize(0.45f);
        EXPECT_EQ(errorHandler.GetErrorCount(), 1);

        // the controller should still have the default height of 1
        EXPECT_NEAR(controller->GetHeight(), 1.0f, 1e-3f);
    }

    TEST_F(PhysXDefaultWorldTest, CharacterController_DroppingBox_CollidesWithController)
    {
        ControllerTestBasis basis(m_testSceneHandle);

        AzPhysics::RigidBody* box = PhysX::TestUtils::AddUnitBoxToScene(m_testSceneHandle, AZ::Vector3(0.5f, 0.0f, 5.0f));

        basis.Update(AZ::Vector3::CreateZero(), 200);

        // the box and controller have default collision layer and group so should collide
        // the box was positioned to land on its edge on the controller
        // so expect the box to have bounced off the controller and traveled in the x direction
        AZ::Vector3 boxPosition = box->GetPosition();
        float x = boxPosition.GetX();
        EXPECT_GT(x, 2.0f);
    }

    TEST_F(PhysXDefaultWorldTest, CharacterController_RaycastAgainstController_ReturnsHit)
    {
        auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get();

        // raycast on an empty scene should return no hits
        AzPhysics::RayCastRequest request;
        request.m_start = AZ::Vector3(-100.0f, 0.0f, 0.25f);
        request.m_direction = AZ::Vector3(1.0f, 0.0f, 0.0f);
        request.m_distance = 200.0f;

        AzPhysics::SceneQueryHits result = sceneInterface->QueryScene(m_testSceneHandle, &request);
        EXPECT_FALSE(result);

        // now add a controller and raycast again
        ControllerTestBasis basis(m_testSceneHandle);

        // the controller won't move to its initial position with its base at the origin until one update has happened
        basis.Update(AZ::Vector3::CreateZero());

        result = sceneInterface->QueryScene(m_testSceneHandle, &request);
        EXPECT_TRUE(result);
    }

    TEST_F(PhysXDefaultWorldTest, CharacterController_DeleteCharacterInsideTrigger_RaisesExitEvent)
    {
        // Create trigger
        Physics::ColliderConfiguration triggerConfig;
        triggerConfig.m_isTrigger = true;
        Physics::BoxShapeConfiguration boxConfig;
        boxConfig.m_dimensions = AZ::Vector3(10.0f, 10.0f, 10.0f);

        auto triggerEntity = AZStd::make_unique<AZ::Entity>("TriggerEntity");
        triggerEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
        triggerEntity->CreateComponent(PhysX::StaticRigidBodyComponentTypeId);
        Internal::AddColliderComponentToEntity(triggerEntity.get(), triggerConfig, boxConfig);
        triggerEntity->Init();
        triggerEntity->Activate();

        TestTriggerAreaNotificationListener triggerListener(triggerEntity->GetId());

        // Create character
        auto characterConfiguration = AZStd::make_unique<Physics::CharacterConfiguration>();
        auto characterShapeConfiguration = AZStd::make_unique<Physics::CapsuleShapeConfiguration>();
        characterShapeConfiguration->m_height = 5.0f;
        characterShapeConfiguration->m_radius = 1.0f;

        auto characterEntity = AZStd::make_unique<AZ::Entity>("CharacterEntity");
        characterEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
        characterEntity->CreateComponent<CharacterControllerComponent>(
            AZStd::move(characterConfiguration), AZStd::move(characterShapeConfiguration));
        characterEntity->Init();
        characterEntity->Activate();

        // Update the world a bit to trigger Enter events
        TestUtils::UpdateScene(m_defaultScene, 0.1f, 10);

        // Delete the entity, and update the world to receive exit events
        characterEntity.reset();
        TestUtils::UpdateScene(m_defaultScene, 0.1f, 1);

        EXPECT_EQ(triggerListener.GetEnteredEvents().size(), 1);
        EXPECT_EQ(triggerListener.GetExitedEvents().size(), 1);
    }

    TEST_F(PhysXDefaultWorldTest, CharacterController_DisabledPhysics_DoesNotCauseError_FT)
    {
        // given a character controller 
        auto characterConfiguration = AZStd::make_unique<Physics::CharacterConfiguration>();
        auto characterShapeConfiguration = AZStd::make_unique<Physics::CapsuleShapeConfiguration>();
        characterShapeConfiguration->m_height = 5.0f;
        characterShapeConfiguration->m_radius = 1.0f;

        auto characterEntity = AZStd::make_unique<AZ::Entity>("CharacterEntity");
        characterEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
        characterEntity->CreateComponent<CharacterControllerComponent>(
            AZStd::move(characterConfiguration), AZStd::move(characterShapeConfiguration));
        characterEntity->Init();
        characterEntity->Activate();

        bool physicsEnabled = false;
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(physicsEnabled, characterEntity->GetId(),
            &AzPhysics::SimulatedBodyComponentRequestsBus::Events::IsPhysicsEnabled);
        EXPECT_TRUE(physicsEnabled);

        // when physics is disabled
        AzPhysics::SimulatedBodyComponentRequestsBus::Event(characterEntity->GetId(), &AzPhysics::SimulatedBodyComponentRequestsBus::Events::DisablePhysics);
        AzPhysics::SimulatedBodyComponentRequestsBus::EventResult(physicsEnabled, characterEntity->GetId(),
            &AzPhysics::SimulatedBodyComponentRequestsBus::Events::IsPhysicsEnabled);
        EXPECT_FALSE(physicsEnabled);

        // expect no error occurs when sending common events 
        AZ::Vector3 result;
        UnitTest::ErrorHandler errorHandler("Invalid character controller.");
        Physics::CharacterRequestBus::Event(
            characterEntity->GetId(),&Physics::CharacterRequestBus::Events::AddVelocity, AZ::Vector3::CreateZero());
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        Physics::CharacterRequestBus::EventResult(
            result, characterEntity->GetId(), &Physics::CharacterRequestBus::Events::GetBasePosition);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        Physics::CharacterRequestBus::EventResult(
            result, characterEntity->GetId(), &Physics::CharacterRequestBus::Events::GetCenterPosition);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        Physics::CharacterRequestBus::EventResult(
            result, characterEntity->GetId(), &Physics::CharacterRequestBus::Events::GetVelocity);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        CharacterControllerRequestBus::Event(
            characterEntity->GetId(), &CharacterControllerRequestBus::Events::Resize, 2.f);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        float height = -1.f;
        CharacterControllerRequestBus::EventResult(
            height, characterEntity->GetId(), &CharacterControllerRequestBus::Events::GetHeight);
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);

        AZ::TransformNotificationBus::Event(
            characterEntity->GetId(), &AZ::TransformNotificationBus::Events::OnTransformChanged,
            AZ::Transform::CreateIdentity(), AZ::Transform::CreateIdentity());
        EXPECT_EQ(errorHandler.GetErrorCount(), 0);
    }

    TEST_F(PhysXDefaultWorldTest, CharacterController_SetNoneCollisionGroupAfterCreation_DoesNotTrigger)
    {
        // Create character
        auto characterEntity = AZStd::make_unique<AZ::Entity>("CharacterEntity");
        {
            auto characterConfiguration = AZStd::make_unique<Physics::CharacterConfiguration>();
            auto characterShapeConfiguration = AZStd::make_unique<Physics::CapsuleShapeConfiguration>();
            characterShapeConfiguration->m_height = 1.5f;
            characterShapeConfiguration->m_radius = 0.5f;
            characterEntity->CreateComponent<AzFramework::TransformComponent>()->SetWorldTM(AZ::Transform::Identity());
            characterEntity->CreateComponent<CharacterControllerComponent>(
                AZStd::move(characterConfiguration), AZStd::move(characterShapeConfiguration));
        }
        characterEntity->Init();
        characterEntity->Activate();

        // Set the callback so that collision groups determine what the character interacts with
        Physics::Character* character = nullptr;
        Physics::CharacterRequestBus::EventResult(character, characterEntity->GetId(), &Physics::CharacterRequests::GetCharacter);
        if (character)
        {
            auto controller = static_cast<PhysX::CharacterController*>(character);
            controller->SetFilterFlags(physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::ePREFILTER);
            if (auto callbackManager = controller->GetCallbackManager())
            {
                callbackManager->SetObjectPreFilter(CollisionLayerBasedObjectPreFilter);
            }
        }

        // Create unit box located near character, collides with character by default
        PhysX::TestUtils::AddStaticUnitBoxToScene(m_testSceneHandle, AZ::Vector3(1.0f, 0.0f, 0.0f));

        // Assign 'None' collision group to character controller - it should not collide with the box
        AZStd::string collisionGroupName;
        Physics::CollisionRequestBus::BroadcastResult(collisionGroupName,
            &Physics::CollisionRequests::GetCollisionGroupName, AzPhysics::CollisionGroup::None);

        Physics::CollisionFilteringRequestBus::Event(
            characterEntity->GetId(), &Physics::CollisionFilteringRequests::SetCollisionGroup, collisionGroupName, AZ::Crc32());

        // Try to move character in direction of the box
        const AZ::Vector3 velocity(2.0f, 0.0f, 0.0f);
        float totalTime = 0.0f;
        float timeStep = AzPhysics::SystemConfiguration::DefaultFixedTimestep;

        if (auto* physXSystem = GetPhysXSystem())
        {
            for (int i = 0; i < 50; i++)
            {
                Physics::CharacterRequestBus::Event(characterEntity->GetId(), &Physics::CharacterRequestBus::Events::AddVelocity, velocity);
                physXSystem->Simulate(timeStep);
                totalTime += timeStep;
            }
        }

        // With 'None' collision group assigned, character is expected to pass through the box to target position
        AZ::Vector3 characterTranslation = characterEntity->GetTransform()->GetWorldTranslation();
        EXPECT_THAT(characterTranslation, UnitTest::IsCloseTolerance(velocity * totalTime, 0.01f));
    }
} // namespace PhysX
