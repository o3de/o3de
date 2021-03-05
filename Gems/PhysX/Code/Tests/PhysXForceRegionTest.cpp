/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include <PhysX_precompiled.h>

#include <AzTest/AzTest.h>

#include <BoxColliderComponent.h>
#include <ForceRegionComponent.h>
#include <RigidBodyComponent.h>
#include <StaticRigidBodyComponent.h>

#include <PhysX/ForceRegionComponentBus.h>

#include <LmbrCentral/Shape/SplineComponentBus.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Transform.h>

#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/RigidBodyBus.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/TriggerBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/WorldEventhandler.h>
#include <AzFramework/Physics/PhysicsSystem.h>

#include <Tests/PhysXTestCommon.h>

namespace PhysX
{
    enum ForceType : AZ::u8
    {
        WorldSpaceForce
        , LocalSpaceForce
        , PointForce
        , SplineFollowForce
        , SimpleDragForce
        , LinearDampingForce
    };

    class PhysXForceRegionTest
        : public::testing::Test
        , protected Physics::DefaultWorldBus::Handler
        , protected Physics::WorldEventHandler
    {
        void SetUp() override
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
                sceneConfiguration.m_legacyId = Physics::DefaultPhysicsWorldId;
                m_testSceneHandle = physicsSystem->AddScene(sceneConfiguration);
                if (m_defaultScene = physicsSystem->GetScene(m_testSceneHandle))
                {
                    m_defaultScene->GetLegacyWorld()->SetEventHandler(this);
                }
            }

            Physics::DefaultWorldBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            Physics::DefaultWorldBus::Handler::BusDisconnect();

            //Cleanup the test scene
            m_defaultScene = nullptr;
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                physicsSystem->RemoveScene(m_testSceneHandle);
            }
            m_testSceneHandle = AzPhysics::InvalidSceneHandle;
            TestUtils::ResetPhysXSystem();
        }

        // DefaultWorldBus
        AZStd::shared_ptr<Physics::World> GetDefaultWorld() override
        {
            return m_defaultScene->GetLegacyWorld();
        }

        // WorldEventHandler
        void OnCollisionBegin(const Physics::CollisionEvent&) override //Not used in force region tests
        {
        }

        void OnCollisionPersist(const Physics::CollisionEvent&) override //Not used in force region tests
        {
        }

        void OnCollisionEnd(const Physics::CollisionEvent&) override //Not used in force region tests
        {
        }

        void OnTriggerEnter(const Physics::TriggerEvent& triggerEvent) override
        {
            Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId()
                , &Physics::TriggerNotifications::OnTriggerEnter
                , triggerEvent);
        }

        void OnTriggerExit(const Physics::TriggerEvent& triggerEvent) override
        {
            Physics::TriggerNotificationBus::QueueEvent(triggerEvent.m_triggerBody->GetEntityId()
                , &Physics::TriggerNotifications::OnTriggerExit
                , triggerEvent);
        }

        AzPhysics::Scene* m_defaultScene = nullptr;
        AzPhysics::SceneHandle m_testSceneHandle = AzPhysics::InvalidSceneHandle;

    public:
        AzPhysics::Scene* GetTestScene() const
        {
            return m_defaultScene;
        }
    };

    AZStd::unique_ptr<AZ::Entity> AddTestRigidBodyCollider(AZ::Vector3& position
        , ForceType forceType
        , const char* name = "TestObjectEntity")
    {
        AZStd::unique_ptr<AZ::Entity> entity(aznew AZ::Entity(name));

        AZ::TransformConfig transformConfig;
        if (forceType == PointForce)
        {
            position.SetX(0.05f);
        }
        transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
        entity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);

        auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
        auto boxShapeConfiguration = AZStd::make_shared<Physics::BoxShapeConfiguration>();
        auto boxColliderComponent = entity->CreateComponent<BoxColliderComponent>();
        boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfiguration, boxShapeConfiguration) });

        Physics::RigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_computeMass = false;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Init();
        entity->Activate();

        return entity;
    }

    namespace DefaultForceRegionParams
    {
        const AZ::Vector3 ForceDirection(0.0f, 0.0f, 1.0f);
        const AZ::Vector3 RotationY(.0f, 90.0f, .0f);
        const float ForceMagnitude = 100.0f;
        const float DampingRatio = 0.0f;
        const float Frequency = 1.0f;
        const float TargetSpeed = 1.0f;
        const float LookAhead = 0.0f;
        const float DragCoefficient = 1.0f;
        const float VolumeDensity = 5.0f;
        const float Damping = 10.0f;
    }

    template<typename ColliderType>
    AZStd::unique_ptr<AZ::Entity> AddForceRegion(const AZ::Vector3& position, ForceType forceType)
    {
        AZStd::unique_ptr<AZ::Entity> forceRegionEntity(aznew AZ::Entity("ForceRegion"));

        AZ::TransformConfig transformConfig;
        transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
        forceRegionEntity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);

        auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfiguration->m_isTrigger = true;
        auto shapeConfiguration = AZStd::make_shared<typename ColliderType::Configuration>();

        auto colliderComponent = forceRegionEntity->CreateComponent<ColliderType>();
        colliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfiguration, shapeConfiguration) });

        // We need StaticRigidBodyComponent to get shapes from collider component added to PhysX world
        forceRegionEntity->CreateComponent<StaticRigidBodyComponent>();

        forceRegionEntity->CreateComponent<ForceRegionComponent>();

        if (forceType == SplineFollowForce)
        {
            forceRegionEntity->CreateComponent("{F0905297-1E24-4044-BFDA-BDE3583F1E57}");//SplineComponent
        }

        forceRegionEntity->Init();
        forceRegionEntity->Activate();

        if (forceType == WorldSpaceForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceWorldSpace
                , DefaultForceRegionParams::ForceDirection
                , DefaultForceRegionParams::ForceMagnitude);
        }
        else if (forceType == LocalSpaceForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceLocalSpace
                , DefaultForceRegionParams::ForceDirection
                , DefaultForceRegionParams::ForceMagnitude);
            AZ::TransformBus::Event(forceRegionEntity->GetId()
                , &AZ::TransformBus::Events::SetLocalRotation
                , DefaultForceRegionParams::RotationY);
        }
        else if (forceType == PointForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForcePoint
                , DefaultForceRegionParams::ForceMagnitude);
        }
        else if (forceType == SplineFollowForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceSplineFollow
                , DefaultForceRegionParams::DampingRatio
                , DefaultForceRegionParams::Frequency
                , DefaultForceRegionParams::TargetSpeed
                , DefaultForceRegionParams::LookAhead
            );

            const AZStd::vector<AZ::Vector3> vertices =
            {
                AZ::Vector3(0.0f, 0.0f, 12.5f),
                AZ::Vector3(0.25f, 0.25f, 12.0f),
                AZ::Vector3(0.5f, 0.5f, 12.0f)
            };

            LmbrCentral::SplineComponentRequestBus::Event(forceRegionEntity->GetId()
                , &LmbrCentral::SplineComponentRequestBus::Events::SetVertices, vertices);
        }
        else if (forceType == SimpleDragForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceSimpleDrag
                , DefaultForceRegionParams::DragCoefficient
                , DefaultForceRegionParams::VolumeDensity);
        }
        else if (forceType == LinearDampingForce)
        {
            PhysX::ForceRegionRequestBus::Event(forceRegionEntity->GetId()
                , &PhysX::ForceRegionRequests::AddForceLinearDamping
                , DefaultForceRegionParams::Damping);
        }

        return forceRegionEntity;
    }

    template<typename ColliderType>
    AZ::Vector3 TestForceVolume(AzPhysics::Scene* scene, ForceType forceType)
    {
        AZ::Vector3 velocity = AZ::Vector3::CreateZero();

        AZ::Vector3 position(0.0f, 0.0f, 16.0f);
        auto rigidBodyCollider = AddTestRigidBodyCollider(position, forceType, "TestBox");
        auto forceRegion = AddForceRegion<ColliderType>(AZ::Vector3(0.0f, 0.0f, 12.0f), forceType);

        //Run simulation for a while - bounces box once on force volume
        constexpr const float deltaTime = 1.0f / 180.0f;
        TestUtils::UpdateScene(scene, deltaTime, 240);

        Physics::RigidBodyRequestBus::EventResult(velocity
            , rigidBodyCollider->GetId()
            , &Physics::RigidBodyRequestBus::Events::GetLinearVelocity);

        return velocity;
    }

    template<typename ColliderType>
    void TestAppliesSameMagnitude(AzPhysics::Scene* scene, ForceType forceType)
    {
        struct ForceRegionMagnitudeChecker
            : public ForceRegionNotificationBus::Handler
        {
            ForceRegionMagnitudeChecker()  { ForceRegionNotificationBus::Handler::BusConnect(); }
            ~ForceRegionMagnitudeChecker() { ForceRegionNotificationBus::Handler::BusDisconnect(); }

            void OnCalculateNetForce(AZ::EntityId, AZ::EntityId, const AZ::Vector3&, float netForceMagnitude)
            {
                // This callback can potentially be called in every frame, so just only catch the first failure to avoid spamming
                if (!m_failed)
                {
                    const float forceRegionMaxError = 0.05f; // Force region uses fast approximation for length calculations, hence the error
                    const bool result = AZ::IsClose(netForceMagnitude, DefaultForceRegionParams::ForceMagnitude, forceRegionMaxError);
                    if (!result)
                    {
                        m_failed = true;
                    }
                    EXPECT_TRUE(result);
                }
            }

            bool m_failed = false;
        };
        ForceRegionMagnitudeChecker magnitudeChecker;
        TestForceVolume<ColliderType>(scene, forceType);
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_WorldSpaceForce_EntityVelocityZPositive)
    {
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(GetTestScene(), WorldSpaceForce);
        // World space force direction: AZ::Vector3(0.0f, 0.0f, 1.0f)
        EXPECT_TRUE(entityVelocity.GetZ() > 0.0f); // World space force causes box to bounce upwards
        EXPECT_NEAR(entityVelocity.GetX(), 0.0f, AZ::Constants::FloatEpsilon);
        EXPECT_NEAR(entityVelocity.GetY(), 0.0f, AZ::Constants::FloatEpsilon);
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_LocalSpaceForce_EntityVelocityZPositive)
    {
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(GetTestScene(), LocalSpaceForce);
        // Local space force direction: AZ::Vector3(0.0f, 0.0f, 1.0f)
        // Force region was rotated about Y-axis by 90 deg
        EXPECT_TRUE(entityVelocity.GetX() > 0.0f); // Falling body should be moving in positive X direction since force region is rotated.
        EXPECT_NEAR(entityVelocity.GetY(), 0.0f, AZ::Constants::FloatEpsilon);
        EXPECT_TRUE(entityVelocity.GetZ() < 0.0f); // Gravity
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_PointForce_EntityVelocityZPositive)
    {
        // Falling body was positioned at AZ::Vector3(0.05f, 0.0f, 16.0f)
        // Force region was positioned at AZ::Vector3(0.0f, 0.0f, 12.0f)
        // PointForce causes box to bounce upwards and to the right.
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(GetTestScene(), PointForce);
        EXPECT_TRUE(entityVelocity.GetX() > 0.0f);
        EXPECT_NEAR(entityVelocity.GetY(), 0.0f, AZ::Constants::FloatEpsilon);
        EXPECT_TRUE(entityVelocity.GetZ() > 0.0f); 
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_SplineFollowForce_EntityVelocitySpecificValue)
    {
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(GetTestScene(), SplineFollowForce);
        // Follow spline direction towards positive X and Y.
        EXPECT_TRUE(entityVelocity.GetX() > 0.0f); 
        EXPECT_TRUE(entityVelocity.GetY() > 0.0f);
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_SimpleDragForce_EntityVelocitySpecificValue)
    {
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(GetTestScene(), SimpleDragForce);
        EXPECT_TRUE(entityVelocity.GetZ() > -12.65f); // Falling velocity should be slower than free fall velocity, which is -12.65.
        EXPECT_NEAR(entityVelocity.GetX(), 0.0f, AZ::Constants::FloatEpsilon); // Dragging should not change original direction.
        EXPECT_NEAR(entityVelocity.GetY(), 0.0f, AZ::Constants::FloatEpsilon); // Dragging should not change original direction.
    } 

    TEST_F(PhysXForceRegionTest, ForceRegion_LinearDampingForce_EntityVelocitySpecificValue)
    {
        AZ::Vector3 entityVelocity = TestForceVolume<BoxColliderComponent>(GetTestScene(), LinearDampingForce);
        EXPECT_TRUE(entityVelocity.GetZ() > -12.65f); // Falling velocity should be slower than free fall velocity, which is -12.65.
        EXPECT_NEAR(entityVelocity.GetX(), 0.0f, AZ::Constants::FloatEpsilon); // Damping should not change original direction.
        EXPECT_NEAR(entityVelocity.GetY(), 0.0f, AZ::Constants::FloatEpsilon); // Damping should not change original direction.
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_PointForce_AppliesSameMagnitude)
    {
        TestAppliesSameMagnitude<BoxColliderComponent>(GetTestScene(), PointForce);
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_WorldSpaceForce_AppliesSameMagnitude)
    {
        TestAppliesSameMagnitude<BoxColliderComponent>(GetTestScene(), WorldSpaceForce);
    }

    TEST_F(PhysXForceRegionTest, ForceRegion_LocalSpaceForce_AppliesSameMagnitude)
    {
        TestAppliesSameMagnitude<BoxColliderComponent>(GetTestScene(), LocalSpaceForce);
    }
}

