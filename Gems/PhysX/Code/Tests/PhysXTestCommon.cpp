/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Physics/Configuration/RigidBodyConfiguration.h>
#include <AzFramework/Physics/Configuration/StaticRigidBodyConfiguration.h>
#include <AzFramework/Physics/SimulatedBodies/RigidBody.h>
#include <AzFramework/Physics/SimulatedBodies/StaticRigidBody.h>
#include <PhysX/SystemComponentBus.h>

#include <BoxColliderComponent.h>
#include <CapsuleColliderComponent.h>
#include <RigidBodyComponent.h>
#include <SphereColliderComponent.h>
#include <Tests/PhysXTestUtil.h>

namespace PhysX
{
    namespace TestUtils
    {
        AZStd::optional<PhysXSystemConfiguration> Test_PhysXSettingsRegistryManager::LoadSystemConfiguration() const
        {
            return PhysXSystemConfiguration::CreateDefault();
        }
        AZStd::optional<AzPhysics::SceneConfiguration> Test_PhysXSettingsRegistryManager::LoadDefaultSceneConfiguration() const
        {
            return AzPhysics::SceneConfiguration::CreateDefault();
        }
        AZStd::optional<Debug::DebugConfiguration> Test_PhysXSettingsRegistryManager::LoadDebugConfiguration() const
        {
            return Debug::DebugConfiguration::CreateDefault();
        }

        void ResetPhysXSystem()
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                physicsSystem->RemoveAllScenes(); //Cleanup any created scenes

                //going to stop and restart PhysXSystem to ensure a clean slate.
                //copy current config, if available, otherwise use default.
                const AzPhysics::SystemConfiguration config = physicsSystem->GetConfiguration() ? *(physicsSystem->GetConfiguration()) : AzPhysics::SystemConfiguration();
                physicsSystem->Shutdown(); //shutdown the system (this will clean everything up
                physicsSystem->Initialize(&config); //init to a fresh state.
            }
        }

        void UpdateScene(AzPhysics::Scene* scene, float timeStep, AZ::u32 numSteps)
        {
            for (AZ::u32 i = 0; i < numSteps; i++)
            {
                scene->StartSimulation(timeStep);
                scene->FinishSimulation();
            }
        }

        void UpdateScene(AzPhysics::SceneHandle sceneHandle, float timeStep, AZ::u32 numSteps)
        {
            auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get();
            UpdateScene(physicsSystem->GetScene(sceneHandle), timeStep, numSteps);
        }

        EntityPtr CreateFlatTestTerrain(AzPhysics::SceneHandle sceneHandle, float width /*= 1.0f*/, float depth /*= 1.0f*/)
        {
            // Creates a single static box with the top at height 0, starting at (0, 0) and going to (width, depth)
            AZ::Vector3 position(width * 0.5f, depth * 0.5f, -1.0f);
            AZ::Vector3 dimensions(width, depth, 1.0f);

            return CreateStaticBoxEntity(sceneHandle, position, dimensions);
        }

        EntityPtr CreateSphereEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float radius, const AzPhysics::CollisionLayer& layer /*= AzPhysics::CollisionLayer::Default*/)
        {
            auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfiguration->m_collisionLayer = layer;
            return CreateSphereEntity(sceneHandle, position, radius, colliderConfiguration);
        }

        EntityPtr CreateSphereEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const float radius, const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig)
        {
            EntityPtr entity = AZStd::make_shared<AZ::Entity>("TestSphereEntity");
            entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
            entity->Init();

            entity->Activate();

            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

            entity->Deactivate();

            auto shapeConfig = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);
            auto shpereColliderComponent = entity->CreateComponent<PhysX::SphereColliderComponent>();
            shpereColliderComponent->SetShapeConfigurationList({ AzPhysics::ShapeColliderPair(colliderConfig, shapeConfig) });

            AzPhysics::RigidBodyConfiguration rigidBodyConfig;
            rigidBodyConfig.m_computeMass = false;
            entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig, sceneHandle);

            entity->Activate();
            return entity;
        }

        EntityPtr CreateStaticSphereEntity(AzPhysics::SceneHandle sceneHandle,
            const AZ::Vector3& position,
            const float radius,
            const AzPhysics::CollisionLayer& layer /*= AzPhysics::CollisionLayer::Default*/)
        {
            auto entity = AZStd::make_shared<AZ::Entity>("TestSphereEntity");
            entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
            entity->Init();

            entity->Activate();

            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

            entity->Deactivate();

            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfig->m_collisionLayer = layer;
            auto shapeConfig = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);
            auto sphereColliderComponent = entity->CreateComponent<SphereColliderComponent>();
            sphereColliderComponent->SetShapeConfigurationList({ AzPhysics::ShapeColliderPair(colliderConfig, shapeConfig) });

            entity->CreateComponent<StaticRigidBodyComponent>(sceneHandle);

            entity->Activate();
            return entity;
        }

        EntityPtr CreateBoxEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const AZ::Vector3& dimensions,
            const AzPhysics::CollisionLayer& layer /*= AzPhysics::CollisionLayer::Default*/, bool isTrigger /*= false*/)
        {
            auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfiguration->m_collisionLayer = layer;
            colliderConfiguration->m_isTrigger = isTrigger;
            return CreateBoxEntity(sceneHandle, position, dimensions, colliderConfiguration);
        }

        EntityPtr CreateStaticBoxEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const AZ::Vector3& dimensions /*= AZ::Vector3(1.0f)*/,
            const AzPhysics::CollisionLayer& layer /*= AzPhysics::CollisionLayer::Default*/)
        {
            EntityPtr entity = AZStd::make_shared<AZ::Entity>("TestBoxEntity");
            entity->CreateComponent<AzFramework::TransformComponent>();
            entity->Init();

            entity->Activate();

            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

            entity->Deactivate();

            auto shapeConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions);
            auto boxColliderComponent = entity->CreateComponent<PhysX::BoxColliderComponent>();
            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfig->m_collisionLayer = layer;
            boxColliderComponent->SetShapeConfigurationList({ AzPhysics::ShapeColliderPair(colliderConfig, shapeConfig) });
            entity->CreateComponent<PhysX::StaticRigidBodyComponent>(sceneHandle);
            entity->Activate();
            return entity;
        }

        EntityPtr CreateCapsuleEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float height, const float radius,
            const AzPhysics::CollisionLayer& layer /*= AzPhysics::CollisionLayer::Default*/)
        {
            auto entity = AZStd::make_shared<AZ::Entity>("TestCapsuleEntity");
            entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
            entity->Init();

            entity->Activate();

            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

            entity->Deactivate();

            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfig->m_collisionLayer = layer;
            auto shapeConfig = AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius);
            auto capsuleColliderComponent = entity->CreateComponent<CapsuleColliderComponent>();
            capsuleColliderComponent->SetShapeConfigurationList({ AzPhysics::ShapeColliderPair(colliderConfig, shapeConfig) });

            AzPhysics::RigidBodyConfiguration rigidBodyConfig;
            rigidBodyConfig.m_computeMass = false;
            entity->CreateComponent<RigidBodyComponent>(rigidBodyConfig, sceneHandle);

            entity->Activate();
            return entity;
        }

        EntityPtr CreateStaticCapsuleEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float height, const float radius,
            const AzPhysics::CollisionLayer& layer /*= AzPhysics::CollisionLayer::Default*/)
        {
            auto entity = AZStd::make_shared<AZ::Entity>("TestCapsuleEntity");
            entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
            entity->Init();

            entity->Activate();

            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

            entity->Deactivate();

            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfig->m_collisionLayer = layer;
            auto shapeConfig = AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius);
            auto capsuleColliderComponent = entity->CreateComponent<CapsuleColliderComponent>();
            capsuleColliderComponent->SetShapeConfigurationList({ AzPhysics::ShapeColliderPair(colliderConfig, shapeConfig) });

            entity->CreateComponent<StaticRigidBodyComponent>(sceneHandle);

            entity->Activate();
            return entity;
        }

        AzPhysics::SimulatedBodyHandle AddStaticTriangleMeshCubeToScene(AzPhysics::SceneHandle scene, float halfExtent)
        {
            // Generate input data
            VertexIndexData cubeMeshData = GenerateCubeMeshData(halfExtent);
            AZStd::vector<AZ::u8> cookedData;
            bool cookingResult = false;
            Physics::SystemRequestBus::BroadcastResult(cookingResult, &Physics::SystemRequests::CookTriangleMeshToMemory,
                cubeMeshData.first.data(), static_cast<AZ::u32>(cubeMeshData.first.size()),
                cubeMeshData.second.data(), static_cast<AZ::u32>(cubeMeshData.second.size()),
                cookedData);
            AZ_Assert(cookingResult, "Failed to cook the cube mesh.");

            // Setup shape & collider configurations
            auto shapeConfig = AZStd::make_shared<Physics::CookedMeshShapeConfiguration>();
            shapeConfig->SetCookedMeshData(cookedData.data(), cookedData.size(),
                Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);

            AzPhysics::StaticRigidBodyConfiguration staticRigidBodyConfiguration;
            staticRigidBodyConfiguration.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(
                AZStd::make_shared<Physics::ColliderConfiguration>(), shapeConfig);

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                return sceneInterface->AddSimulatedBody(scene, &staticRigidBodyConfiguration);
            }
            return AzPhysics::InvalidSimulatedBodyHandle;
        }

        AzPhysics::SimulatedBodyHandle AddKinematicTriangleMeshCubeToScene(AzPhysics::SceneHandle scene, float halfExtent, AzPhysics::MassComputeFlags massComputeFlags)
        {
            // Generate input data
            VertexIndexData cubeMeshData = GenerateCubeMeshData(halfExtent);
            AZStd::vector<AZ::u8> cookedData;
            bool cookingResult = false;
            Physics::SystemRequestBus::BroadcastResult(cookingResult, &Physics::SystemRequests::CookTriangleMeshToMemory,
                cubeMeshData.first.data(), static_cast<AZ::u32>(cubeMeshData.first.size()),
                cubeMeshData.second.data(), static_cast<AZ::u32>(cubeMeshData.second.size()),
                cookedData);
            AZ_Assert(cookingResult, "Failed to cook the cube mesh.");

            // Setup shape & collider configurations
            auto shapeConfig = AZStd::make_shared<Physics::CookedMeshShapeConfiguration>();
            shapeConfig->SetCookedMeshData(cookedData.data(), cookedData.size(),
                Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);

            AzPhysics::RigidBodyConfiguration rigidBodyConfiguration;
            rigidBodyConfiguration.m_kinematic = true;
            rigidBodyConfiguration.SetMassComputeFlags(massComputeFlags);
            rigidBodyConfiguration.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(
                AZStd::make_shared<Physics::ColliderConfiguration>(), shapeConfig);

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                return sceneInterface->AddSimulatedBody(scene, &rigidBodyConfiguration);
            }
            return AzPhysics::InvalidSimulatedBodyHandle;
        }

        void SetCollisionLayer(EntityPtr& entity, const AZStd::string& layerName, const AZStd::string& colliderTag)
        {
            Physics::CollisionFilteringRequestBus::Event(entity->GetId(), &Physics::CollisionFilteringRequests::SetCollisionLayer, layerName, AZ::Crc32(colliderTag.c_str()));
        }
        
        void SetCollisionGroup(EntityPtr& entity, const AZStd::string& groupName, const AZStd::string& colliderTag)
        {
            Physics::CollisionFilteringRequestBus::Event(entity->GetId(), &Physics::CollisionFilteringRequests::SetCollisionGroup, groupName, AZ::Crc32(colliderTag.c_str()));
        }

        void ToggleCollisionLayer(EntityPtr& entity, const AZStd::string& layerName, bool enabled, const AZStd::string& colliderTag)
        {
            Physics::CollisionFilteringRequestBus::Event(entity->GetId(), &Physics::CollisionFilteringRequests::ToggleCollisionLayer, layerName, AZ::Crc32(colliderTag.c_str()), enabled);
        }

        EntityPtr CreateBoxEntity(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const AZ::Vector3& dimensions,
            const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig)
        {
            EntityPtr entity = AZStd::make_shared<AZ::Entity>("TestBoxEntity");
            entity->CreateComponent<AzFramework::TransformComponent>();
            entity->Init();

            entity->Activate();

            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

            entity->Deactivate();

            auto shapeConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions);
            auto boxColliderComponent = entity->CreateComponent<PhysX::BoxColliderComponent>();
            boxColliderComponent->SetShapeConfigurationList({ AzPhysics::ShapeColliderPair(colliderConfig, shapeConfig) });

            AzPhysics::RigidBodyConfiguration rigidBodyConfig;
            rigidBodyConfig.m_computeMass = false;
            entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig, sceneHandle);

            entity->Activate();
            return entity;
        }

        EntityPtr AddUnitTestBoxComponentsMix(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position, const char* name)
        {
            EntityPtr entity = AZStd::make_shared<AZ::Entity>(name);

            AZ::TransformConfig transformConfig;
            transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
            entity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);
            AzPhysics::ShapeColliderPairList shapeConfigList = { AzPhysics::ShapeColliderPair(
                AZStd::make_shared<Physics::ColliderConfiguration>(),
                AZStd::make_shared<Physics::BoxShapeConfiguration>()) };
            auto boxCollider = entity->CreateComponent<BoxColliderComponent>();
            boxCollider->SetShapeConfigurationList(shapeConfigList);

            AzPhysics::RigidBodyConfiguration rigidBodyConfig;
            entity->CreateComponent<RigidBodyComponent>(rigidBodyConfig, sceneHandle);

            // Removing and adding component can cause race condition in component activation code if dependencies are not correct
            // Simulation of user removing one collider and adding another
            entity->RemoveComponent(boxCollider);
            delete boxCollider;
            entity->CreateComponent<BoxColliderComponent>()->SetShapeConfigurationList(shapeConfigList);

            entity->Init();
            entity->Activate();

            return entity;
        }

        PhysX::PointList GeneratePyramidPoints(float length)
        {
            const PointList points
            {
                AZ::Vector3(length, 0.0f, 0.0f),
                AZ::Vector3(-length, 0.0f, 0.0f),
                AZ::Vector3(0.0f, length, 0.0f),
                AZ::Vector3(0.0f, -length, 0.0f),
                AZ::Vector3(0.0f, 0.0f, length)
            };

            return points;
        }

        PhysX::VertexIndexData GenerateCubeMeshData(float halfExtent)
        {
            const PointList points
            {
                AZ::Vector3(-halfExtent, -halfExtent,  halfExtent),
                AZ::Vector3(halfExtent, -halfExtent,  halfExtent),
                AZ::Vector3(-halfExtent,  halfExtent,  halfExtent),
                AZ::Vector3(halfExtent,  halfExtent,  halfExtent),
                AZ::Vector3(-halfExtent, -halfExtent, -halfExtent),
                AZ::Vector3(halfExtent, -halfExtent, -halfExtent),
                AZ::Vector3(-halfExtent,  halfExtent, -halfExtent),
                AZ::Vector3(halfExtent,  halfExtent, -halfExtent)
            };

            const AZStd::vector<AZ::u32> indices =
            {
                0, 1, 2, 2, 1, 3,
                2, 3, 7, 2, 7, 6,
                7, 3, 1, 1, 5, 7,
                0, 2, 4, 2, 6, 4,
                0, 4, 1, 1, 4, 5,
                4, 6, 5, 5, 6, 7
            };

            return AZStd::make_pair(points, indices);
        }

        AzPhysics::StaticRigidBody* AddStaticFloorToScene(AzPhysics::SceneHandle sceneHandle, const AZ::Transform& transform)
        {
            AzPhysics::StaticRigidBodyConfiguration staticBodyConfiguration;
            staticBodyConfiguration.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(
                AZStd::make_shared<Physics::ColliderConfiguration>(),
                AZStd::make_shared<Physics::BoxShapeConfiguration>(AZ::Vector3(20.0f, 20.0f, 1.0f)));
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                AzPhysics::SimulatedBodyHandle simBodyHandle = sceneInterface->AddSimulatedBody(sceneHandle, &staticBodyConfiguration);
                if (auto* floor = azdynamic_cast<AzPhysics::StaticRigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, simBodyHandle)))
                {
                    floor->SetTransform(transform);
                    return floor;
                }
            }
            return nullptr;
        }

        AzPhysics::StaticRigidBody* AddStaticUnitBoxToScene(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position)
        {
            AzPhysics::SimulatedBodyHandle simBodyHandle = AddStaticBoxToScene(sceneHandle, position);
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                return azdynamic_cast<AzPhysics::StaticRigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, simBodyHandle));
            }
            return nullptr;
        }

        AzPhysics::RigidBody* AddUnitBoxToScene(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position)
        {
            AzPhysics::SimulatedBodyHandle simBodyHandle = AddBoxToScene(sceneHandle, position);
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                return azdynamic_cast<AzPhysics::RigidBody*>(sceneInterface->GetSimulatedBodyFromHandle(sceneHandle, simBodyHandle));
            }
            return nullptr;
        }

        AzPhysics::SimulatedBodyHandle AddSphereToScene(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float radius /*= 0.5f*/, const AzPhysics::CollisionLayer& layer /*= AzPhysics::CollisionLayer::Default*/)
        {
            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfig->m_collisionLayer = layer;
            auto shapeConfiguration = AZStd::make_shared<Physics::SphereShapeConfiguration>();
            shapeConfiguration->m_radius = radius;
            AzPhysics::RigidBodyConfiguration rigidBodySettings;
            rigidBodySettings.m_computeMass = false;
            rigidBodySettings.m_computeInertiaTensor = false;
            rigidBodySettings.m_computeCenterOfMass = false;
            rigidBodySettings.m_mass = 1.0f;
            rigidBodySettings.m_position = position;
            rigidBodySettings.m_linearDamping = 0.0f;
            rigidBodySettings.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(colliderConfig, shapeConfiguration);

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                return sceneInterface->AddSimulatedBody(sceneHandle, &rigidBodySettings);
            }
            return AzPhysics::InvalidSimulatedBodyHandle;
        }

        AzPhysics::SimulatedBodyHandle AddCapsuleToScene(AzPhysics::SceneHandle sceneHandle, const AZ::Vector3& position,
            const float height /*= 2.0f*/, const float radius /*= 0.5f*/,
            const AzPhysics::CollisionLayer& layer /*= AzPhysics::CollisionLayer::Default*/)
        {
            AzPhysics::RigidBodyConfiguration rigidBodySettings;
            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfig->m_collisionLayer = layer;
            colliderConfig->m_rotation = AZ::Quaternion::CreateRotationX(AZ::Constants::HalfPi);
            
            rigidBodySettings.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(colliderConfig,
                AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius));
            rigidBodySettings.m_position = position;
            rigidBodySettings.m_computeMass = false;
            rigidBodySettings.m_computeInertiaTensor = false;
            rigidBodySettings.m_computeCenterOfMass = false;
            rigidBodySettings.m_mass = 1.0f;

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                return sceneInterface->AddSimulatedBody(sceneHandle, &rigidBodySettings);
            }
            return AzPhysics::InvalidSimulatedBodyHandle;
        }

        AzPhysics::SimulatedBodyHandle AddBoxToScene(AzPhysics::SceneHandle sceneHandle,
            const AZ::Vector3& position, const AZ::Vector3& dimensions /*= AZ::Vector3(1.0f)*/,
            const AzPhysics::CollisionLayer& layer /*= AzPhysics::CollisionLayer::Default*/)
        {
            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfig->m_collisionLayer = layer;
            auto shapeConfiguration = AZStd::make_shared<Physics::BoxShapeConfiguration>();
            shapeConfiguration->m_dimensions = dimensions;

            AzPhysics::RigidBodyConfiguration rigidBodySettings;
            rigidBodySettings.m_computeMass = false;
            rigidBodySettings.m_computeInertiaTensor = false;
            rigidBodySettings.m_computeCenterOfMass = false;
            rigidBodySettings.m_mass = 1.0f;
            rigidBodySettings.m_position = position;
            rigidBodySettings.m_linearDamping = 0.0f;
            rigidBodySettings.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(colliderConfig, shapeConfiguration);
            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                return sceneInterface->AddSimulatedBody(sceneHandle, &rigidBodySettings);
            }
            return AzPhysics::InvalidSimulatedBodyHandle;
        }

        AzPhysics::SimulatedBodyHandle AddStaticBoxToScene(AzPhysics::SceneHandle sceneHandle,
            const AZ::Vector3& position, const AZ::Vector3& dimensions /*= AZ::Vector3(1.0f)*/,
            const AzPhysics::CollisionLayer& layer /*= AzPhysics::CollisionLayer::Default*/)
        {
            auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfig->m_collisionLayer = layer;
            auto shapeConfiguration = AZStd::make_shared<Physics::BoxShapeConfiguration>();
            shapeConfiguration->m_dimensions = dimensions;

            AzPhysics::StaticRigidBodyConfiguration rigidBodySettings;
            rigidBodySettings.m_position = position;
            rigidBodySettings.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(colliderConfig, shapeConfiguration);

            if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
            {
                return sceneInterface->AddSimulatedBody(sceneHandle, &rigidBodySettings);
            }
            return AzPhysics::InvalidSimulatedBodyHandle;
        }

        float GetPositionElement(EntityPtr entity, int element)
        {
            AZ::Transform transform = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(transform, entity->GetId(), &AZ::TransformInterface::GetWorldTM);
            return transform.GetTranslation().GetElement(element);
        }
    }
}
