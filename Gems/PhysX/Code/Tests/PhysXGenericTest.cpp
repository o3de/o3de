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

#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>

#include <PhysXTestEnvironment.h>
#include <PhysXTestCommon.h>

#include <Physics/PhysicsTests.h>
#include <Material.h>
#include <BoxColliderComponent.h>
#include <CapsuleColliderComponent.h>
#include <RigidBodyComponent.h>
#include <SphereColliderComponent.h>
#include <StaticRigidBodyComponent.h>

namespace Physics
{
    void GenericPhysicsFixture::SetUpInternal()
    {
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            AzPhysics::SceneConfiguration sceneConfiguration = physicsSystem->GetDefaultSceneConfiguration();
            sceneConfiguration.m_legacyId = Physics::DefaultPhysicsWorldId;
            m_testSceneHandle = physicsSystem->AddScene(sceneConfiguration);
            m_defaultScene = physicsSystem->GetScene(m_testSceneHandle);
        }

        Physics::DefaultWorldBus::Handler::BusConnect();
    }

    void GenericPhysicsFixture::TearDownInternal()
    {
        PhysX::MaterialManagerRequestsBus::Broadcast(&PhysX::MaterialManagerRequestsBus::Events::ReleaseAllMaterials);
        Physics::DefaultWorldBus::Handler::BusDisconnect();
        m_defaultScene = nullptr;
        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            physicsSystem->RemoveScene(m_testSceneHandle);
        }
        m_testSceneHandle = AzPhysics::InvalidSceneHandle;
        PhysX::TestUtils::ResetPhysXSystem();
    }

    AZStd::shared_ptr<World> GenericPhysicsFixture::GetDefaultWorld()
    {
        return m_defaultScene->GetLegacyWorld();
    }

    AZ::Entity* GenericPhysicsFixture::AddSphereEntity(const AZ::Vector3& position, const float radius,
        const AzPhysics::CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestSphereEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = layer;
        auto shapeConfig = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);
        auto sphereColliderComponent = entity->CreateComponent<PhysX::SphereColliderComponent>();
        sphereColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

        RigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_computeMass = false;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsFixture::AddBoxEntity(const AZ::Vector3& position, const AZ::Vector3& dimensions,
        const AzPhysics::CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestBoxEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = layer;
        auto shapeConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions);
        auto boxColliderComponent = entity->CreateComponent<PhysX::BoxColliderComponent>();
        boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

        RigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_computeMass = false;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsFixture::AddStaticSphereEntity(const AZ::Vector3& position, const float radius,
        const AzPhysics::CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestSphereEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = layer;
        auto shapeConfig = AZStd::make_shared<Physics::SphereShapeConfiguration>(radius);
        auto sphereColliderComponent = entity->CreateComponent<PhysX::SphereColliderComponent>();
        sphereColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

        entity->CreateComponent<PhysX::StaticRigidBodyComponent>();

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsFixture::AddStaticBoxEntity(const AZ::Vector3& position,
        const AZ::Vector3& dimensions, const AzPhysics::CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestStaticBoxEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = layer;
        auto shapeConfig = AZStd::make_shared<Physics::BoxShapeConfiguration>(dimensions);
        auto boxColliderComponent = entity->CreateComponent<PhysX::BoxColliderComponent>();
        boxColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

        entity->CreateComponent<PhysX::StaticRigidBodyComponent>();

        entity->Activate();
        return entity;
    }

    AZ::Entity* GenericPhysicsFixture::AddStaticCapsuleEntity(const AZ::Vector3& position, const float height,
        const float radius, const AzPhysics::CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestCapsuleEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = layer;
        auto shapeConfig = AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius);
        auto capsuleColliderComponent = entity->CreateComponent<PhysX::CapsuleColliderComponent>();
        capsuleColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

        entity->CreateComponent<PhysX::StaticRigidBodyComponent>();

        entity->Activate();
        return entity;
    }


    AZ::Entity* GenericPhysicsFixture::AddCapsuleEntity(const AZ::Vector3& position, const float height,
        const float radius, const AzPhysics::CollisionLayer& layer)
    {
        auto entity = aznew AZ::Entity("TestCapsuleEntity");
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, position);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = layer;
        auto shapeConfig = AZStd::make_shared<Physics::CapsuleShapeConfiguration>(height, radius);
        auto capsuleColliderComponent = entity->CreateComponent<PhysX::CapsuleColliderComponent>();
        capsuleColliderComponent->SetShapeConfigurationList({ AZStd::make_pair(colliderConfig, shapeConfig) });

        RigidBodyConfiguration rigidBodyConfig;
        rigidBodyConfig.m_computeMass = false;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();
        return entity;
    }

    AZStd::unique_ptr<AZ::Entity> GenericPhysicsFixture::AddMultiShapeEntity(const MultiShapeConfig& config)
    {
        AZStd::unique_ptr<AZ::Entity> entity(aznew AZ::Entity("TestShapeEntity"));
        entity->CreateComponent(AZ::Uuid::CreateString("{22B10178-39B6-4C12-BB37-77DB45FDD3B6}")); // TransformComponent
        entity->Init();

        entity->Activate();

        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetWorldTranslation, config.m_position);
        AZ::TransformBus::Event(entity->GetId(), &AZ::TransformBus::Events::SetLocalRotation, config.m_rotation);

        entity->Deactivate();

        auto colliderConfig = AZStd::make_shared<Physics::ColliderConfiguration>();
        colliderConfig->m_collisionLayer = config.m_layer;

        Physics::ShapeConfigurationList shapeconfigurationList;

        struct Visitor
        {
            AZStd::shared_ptr<Physics::ColliderConfiguration>& colliderConfig;
            Physics::ShapeConfigurationList& shapeconfigurationList;

            void operator()(const MultiShapeConfig::ShapeList::ShapeData::Box& box) const
            {
                shapeconfigurationList.push_back
                ({
                    colliderConfig,
                    AZStd::make_shared<Physics::BoxShapeConfiguration>(box.m_extent)
                });
            }
            void operator()(const MultiShapeConfig::ShapeList::ShapeData::Sphere& sphere) const
            {
                shapeconfigurationList.push_back
                ({
                    colliderConfig,
                    AZStd::make_shared<Physics::SphereShapeConfiguration>(sphere.m_radius)
                });
            }
            void operator()(const MultiShapeConfig::ShapeList::ShapeData::Capsule& capsule) const
            {
                shapeconfigurationList.push_back
                ({
                    colliderConfig,
                    AZStd::make_shared<Physics::CapsuleShapeConfiguration>(capsule.m_height, capsule.m_radius)
                });
            }
            void operator()(const AZStd::monostate&) const
            {
                AZ_Assert(false, "Invalid shape type");
            }
        };
        Visitor addShapeConfig = {colliderConfig, shapeconfigurationList};

        for (const MultiShapeConfig::ShapeList::ShapeData& shapeConfig : config.m_shapes.m_shapesData)
        {
            AZStd::visit(addShapeConfig, shapeConfig.m_data);
        }

        auto colliderComponent = entity->CreateComponent<PhysX::BaseColliderComponent>();
        colliderComponent->SetShapeConfigurationList(shapeconfigurationList);

        RigidBodyConfiguration rigidBodyConfig;
        entity->CreateComponent<PhysX::RigidBodyComponent>(rigidBodyConfig);

        entity->Activate();

        for (int i = 0; i < colliderComponent->GetShapes().size(); ++i)
        {
            colliderComponent->GetShapes()[i]->SetLocalPose(config.m_shapes.m_shapesData[i].m_offset, AZ::Quaternion::CreateIdentity());
        }

        return AZStd::unique_ptr<AZ::Entity>{ AZStd::move(entity) };
    }

} // namespace Physics
