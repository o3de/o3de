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

#include <Tests/PhysXTestCommon.h>

#include <AzFramework/Physics/RigidBody.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/World.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Components/TransformComponent.h>
#include <PhysX/SystemComponentBus.h>

#include <BoxColliderComponent.h>
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

        EntityPtr CreateFlatTestTerrain(float width /*= 1.0f*/, float depth /*= 1.0f*/)
        {
            // Creates a single static box with the top at height 0, starting at (0, 0) and going to (width, depth)
            AZ::Vector3 position(width * 0.5f, depth * 0.5f, -1.0f);
            AZ::Vector3 dimensions(width, depth, 1.0f);

            return CreateStaticBoxEntity(position, dimensions);
        }

        EntityPtr CreateSphereEntity(const AZ::Vector3& position, const float radius, const AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig)
        {
            EntityPtr entity = AZStd::make_shared<AZ::Entity>("TestSphereEntity");
            entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
            entity->Init();

            entity->Activate();

            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

            entity->Deactivate();

            auto shapeConfig = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);
            auto shpereColliderComponent = entity->CreateComponent<PhysX::SphereColliderComponent>();
            shpereColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

            Physics::RigidBodyConfiguration rigidBodyConfig;
            entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

            entity->Activate();
            return entity;
        }

        EntityPtr CreateBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions, bool isTrigger)
        {
            auto colliderConfiguration = AZStd::make_shared<Physics::ColliderConfiguration>();
            colliderConfiguration->m_isTrigger = isTrigger;
            return CreateBoxEntity(position, dimensions, colliderConfiguration);
        }

        EntityPtr CreateStaticBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions)
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
            boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });
            entity->CreateComponent<PhysX::StaticRigidBodyComponent>();
            entity->Activate();
            return entity;
        }

        AZStd::unique_ptr<Physics::RigidBodyStatic> CreateStaticTriangleMeshCube(float halfExtent)
        {
            // Create static rigid body
            Physics::RigidBodyConfiguration rigidBodyConfiguration;
            AZStd::unique_ptr<Physics::RigidBodyStatic> rigidBody = AZ::Interface<Physics::System>::Get()->CreateStaticRigidBody(rigidBodyConfiguration);
            AZ_Assert(rigidBody != nullptr, "Failed to create a rigid body");

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
            Physics::CookedMeshShapeConfiguration shapeConfig;
            shapeConfig.SetCookedMeshData(cookedData.data(), cookedData.size(),
                Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh);

            Physics::ColliderConfiguration colliderConfig;

            // Create the first shape
            AZStd::shared_ptr<Physics::Shape> firstShape = AZ::Interface<Physics::System>::Get()->CreateShape(colliderConfig, shapeConfig);
            AZ_Assert(firstShape != nullptr, "Failed to create a shape from cooked data");

            rigidBody->AddShape(firstShape);
            return rigidBody;
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

        EntityPtr CreateBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions,
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
            boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

            Physics::RigidBodyConfiguration rigidBodyConfig;
            entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

            entity->Activate();
            return entity;
        }

        EntityPtr AddUnitTestBoxComponentsMix(const AZ::Vector3& position, const char* name)
        {
            EntityPtr entity = AZStd::make_shared<AZ::Entity>(name);

            AZ::TransformConfig transformConfig;
            transformConfig.m_worldTransform = AZ::Transform::CreateTranslation(position);
            entity->CreateComponent<AzFramework::TransformComponent>()->SetConfiguration(transformConfig);
            Physics::ShapeConfigurationList shapeConfigList = { AZStd::make_pair(
                AZStd::make_shared<Physics::ColliderConfiguration>(),
                AZStd::make_shared<Physics::BoxShapeConfiguration>()) };
            auto boxCollider = entity->CreateComponent<BoxColliderComponent>();
            boxCollider->SetShapeConfigurationList(shapeConfigList);

            Physics::RigidBodyConfiguration rigidBodyConfig;
            entity->CreateComponent<RigidBodyComponent>(rigidBodyConfig);

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

    }
}
